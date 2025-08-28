#include "PreviewSystem.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <fstream>

namespace Preview {

//-----------------------------------------------------------------------------
// ScatterBak Implementation
//-----------------------------------------------------------------------------

ScatterBak::ScatterBak() : loaded_(false), sampleCount_(0) {}

ScatterBak::~ScatterBak() {
    unload();
}

bool ScatterBak::load(const char* path) {
    unload();
    
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    
    // Read entire file into memory
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    bakData_.resize(fileSize);
    file.read(reinterpret_cast<char*>(bakData_.data()), fileSize);
    file.close();
    
    if (!parseHeader()) {
        bakData_.clear();
        return false;
    }
    
    buildSpatialIndex();
    loaded_ = true;
    return true;
}

void ScatterBak::unload() {
    bakData_.clear();
    index_.clear();
    stubData_.clear();
    idToIndex_.clear();
    spatialBins_.clear();
    knnData_.clear();
    loaded_ = false;
    sampleCount_ = 0;
}

const int16_t* ScatterBak::stub(uint64_t id) const {
    if (!loaded_) return nullptr;
    
    auto it = idToIndex_.find(id);
    if (it == idToIndex_.end()) return nullptr;
    
    const BakIndexRow& row = index_[it->second];
    if (row.stubOfs + row.stubLenFrames * sizeof(int16_t) > stubData_.size() * sizeof(int16_t)) {
        return nullptr;
    }
    
    return &stubData_[row.stubOfs / sizeof(int16_t)];
}

size_t ScatterBak::stubLength(uint64_t id) const {
    if (!loaded_) return 0;
    
    auto it = idToIndex_.find(id);
    if (it == idToIndex_.end()) return 0;
    
    return index_[it->second].stubLenFrames;
}

float ScatterBak::stubGain(uint64_t id) const {
    if (!loaded_) return 1.0f;
    
    auto it = idToIndex_.find(id);
    if (it == idToIndex_.end()) return 1.0f;
    
    return static_cast<float>(index_[it->second].gainQ15) / 32768.0f;
}

bool ScatterBak::coords(uint64_t id, int16_t& x, int16_t& y) const {
    if (!loaded_) return false;
    
    auto it = idToIndex_.find(id);
    if (it == idToIndex_.end()) return false;
    
    const BakIndexRow& row = index_[it->second];
    x = row.x;
    y = row.y;
    return true;
}

uint8_t ScatterBak::knn(uint64_t id, uint16_t outIdx[16]) const {
    if (!loaded_) return 0;
    
    auto it = idToIndex_.find(id);
    if (it == idToIndex_.end()) return 0;
    
    size_t idx = it->second;
    if (idx >= knnData_.size()) return 0;
    
    std::memcpy(outIdx, knnData_[idx].data(), 16 * sizeof(uint16_t));
    
    // Count valid neighbors (assuming 0xFFFF marks end)
    uint8_t count = 0;
    for (int i = 0; i < 16; ++i) {
        if (outIdx[i] == 0xFFFF) break;
        count++;
    }
    
    return count;
}

bool ScatterBak::parseHeader() {
    // Simplified .bak format parsing
    // In reality this would parse a proper binary format
    if (bakData_.size() < 64) return false;
    
    // Mock parsing - assume header contains count and offsets
    size_t headerSize = 64;
    const uint32_t* header = reinterpret_cast<const uint32_t*>(bakData_.data());
    
    sampleCount_ = header[0];
    uint32_t indexOffset = header[1];
    uint32_t stubOffset = header[2];
    uint32_t coordOffset = header[3];
    uint32_t knnOffset = header[4];
    
    if (sampleCount_ == 0 || sampleCount_ > 10000) return false; // Sanity check
    
    // Parse index
    index_.resize(sampleCount_);
    if (indexOffset + sampleCount_ * sizeof(BakIndexRow) > bakData_.size()) return false;
    
    std::memcpy(index_.data(), 
               bakData_.data() + indexOffset, 
               sampleCount_ * sizeof(BakIndexRow));
    
    // Build ID lookup
    for (size_t i = 0; i < sampleCount_; ++i) {
        idToIndex_[index_[i].id] = i;
    }
    
    // Parse stub data
    size_t stubDataSize = (bakData_.size() - stubOffset) / sizeof(int16_t);
    stubData_.resize(stubDataSize);
    std::memcpy(stubData_.data(),
               bakData_.data() + stubOffset,
               stubDataSize * sizeof(int16_t));
    
    // Parse k-NN data
    if (knnOffset > 0 && knnOffset < bakData_.size()) {
        knnData_.resize(sampleCount_);
        size_t knnDataSize = sampleCount_ * 16 * sizeof(uint16_t);
        if (knnOffset + knnDataSize <= bakData_.size()) {
            std::memcpy(knnData_.data(),
                       bakData_.data() + knnOffset,
                       knnDataSize);
        }
    }
    
    return true;
}

void ScatterBak::buildSpatialIndex() {
    // Build spatial bins for fast nearest neighbor queries
    // Simplified version - in reality would use more sophisticated spatial data structure
    
    const int GRID_SIZE = 32;  // 32x32 grid
    spatialBins_.resize(GRID_SIZE * GRID_SIZE);
    
    for (size_t i = 0; i < sampleCount_; ++i) {
        const BakIndexRow& row = index_[i];
        
        // Map coordinates to grid (assuming -32768 to 32767 range)
        int gridX = std::clamp((row.x + 32768) / 2048, 0, GRID_SIZE - 1);
        int gridY = std::clamp((row.y + 32768) / 2048, 0, GRID_SIZE - 1);
        
        spatialBins_[gridY * GRID_SIZE + gridX].push_back(static_cast<uint16_t>(i));
    }
}

//-----------------------------------------------------------------------------
// PreviewCache Implementation
//-----------------------------------------------------------------------------

PreviewCache::PreviewCache() 
    : pakFile_(nullptr), cardGrade_(CardGrade::OK), maxCacheSize_(16 * 1024 * 1024), // 16MB
      currentCacheSize_(0), stopLoading_(false), cacheHits_(0), cacheMisses_(0) {}

PreviewCache::~PreviewCache() {
    closePak();
}

void PreviewCache::openPak(const char* path) {
    closePak();
    
    pakPath_ = path;
    pakFile_ = fopen(path, "rb");
    if (!pakFile_) return;
    
    if (parsePakHeader()) {
        // Start background loading thread
        stopLoading_.store(false);
        loadThread_ = std::thread(&PreviewCache::loadingThread, this);
    }
}

void PreviewCache::closePak() {
    if (loadThread_.joinable()) {
        stopLoading_.store(true);
        loadThread_.join();
    }
    
    if (pakFile_) {
        fclose(pakFile_);
        pakFile_ = nullptr;
    }
    
    cache_.clear();
    pakIndex_.clear();
    idToIndex_.clear();
    currentCacheSize_ = 0;
}

void PreviewCache::prefetch(uint64_t id) {
    if (!pakFile_) return;
    
    auto it = idToIndex_.find(id);
    if (it == idToIndex_.end()) return;
    
    const PreviewIdx& idx = pakIndex_[it->second];
    
    // Check if already cached or loading
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        auto cacheIt = cache_.find(id);
        if (cacheIt != cache_.end()) return; // Already cached or loading
    }
    
    // Add to pending reads
    PendingRead read;
    read.id = id;
    read.offset = idx.ofs;
    read.length = idx.lenMs;
    read.requestTime = std::chrono::steady_clock::now();
    
    {
        std::lock_guard<std::mutex> lock(readMutex_);
        pendingReads_.push_back(read);
    }
}

const int16_t* PreviewCache::body(uint64_t id, size_t& nFrames) {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    
    auto it = cache_.find(id);
    if (it == cache_.end() || it->second.loading) {
        cacheMisses_.fetch_add(1);
        nFrames = 0;
        return nullptr;
    }
    
    cacheHits_.fetch_add(1);
    it->second.lastAccess = std::chrono::steady_clock::now();
    nFrames = it->second.data.size();
    return it->second.data.data();
}

void PreviewCache::setCardGrade(CardGrade grade) {
    cardGrade_ = grade;
    
    // Adjust cache size based on grade
    switch (grade) {
        case CardGrade::Gold:
            maxCacheSize_ = 32 * 1024 * 1024;  // 32MB
            break;
        case CardGrade::OK:
            maxCacheSize_ = 16 * 1024 * 1024;  // 16MB  
            break;
        case CardGrade::Slow:
            maxCacheSize_ = 8 * 1024 * 1024;   // 8MB
            break;
    }
}

void PreviewCache::loadingThread() {
    while (!stopLoading_.load()) {
        std::vector<PendingRead> readsToProcess;
        
        // Get pending reads
        {
            std::lock_guard<std::mutex> lock(readMutex_);
            readsToProcess.swap(pendingReads_);
        }
        
        // Process reads
        for (const auto& read : readsToProcess) {
            if (stopLoading_.load()) break;
            performRead(read);
        }
        
        // Sleep if no work
        if (readsToProcess.empty()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}

void PreviewCache::performRead(const PendingRead& read) {
    // Create cache entry  
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        cache_[read.id].loading = true;
    }
    
    // Perform aligned read
    size_t alignedOffset = (read.offset / 4096) * 4096;  // 4KB alignment
    size_t readSize = std::max(4096UL, static_cast<size_t>(read.length * 44.1f)); // Approximate
    
    std::vector<uint8_t> readBuffer(readSize);
    
    if (fseek(pakFile_, alignedOffset, SEEK_SET) == 0) {
        size_t bytesRead = fread(readBuffer.data(), 1, readSize, pakFile_);
        
        if (bytesRead > 0) {
            // Convert to 16-bit samples (simplified)
            size_t sampleOffset = read.offset - alignedOffset;
            size_t sampleCount = std::min((bytesRead - sampleOffset) / sizeof(int16_t), 
                                        static_cast<size_t>(read.length * 22.05f)); // 22.05kHz
            
            std::vector<int16_t> samples(sampleCount);
            if (sampleOffset + sampleCount * sizeof(int16_t) <= bytesRead) {
                std::memcpy(samples.data(), 
                           readBuffer.data() + sampleOffset, 
                           sampleCount * sizeof(int16_t));
            }
            
            // Update cache
            {
                std::lock_guard<std::mutex> lock(cacheMutex_);
                auto& entry = cache_[read.id];
                entry.data = std::move(samples);
                entry.loading = false;
                entry.lastAccess = std::chrono::steady_clock::now();
                
                currentCacheSize_ += entry.data.size() * sizeof(int16_t);
                
                // Evict if necessary
                while (currentCacheSize_ > maxCacheSize_ && cache_.size() > 1) {
                    evictLRU();
                }
            }
        }
    }
}

bool PreviewCache::parsePakHeader() {
    // Simplified .pak format parsing
    if (!pakFile_) return false;
    
    uint32_t header[8];
    if (fread(header, sizeof(uint32_t), 8, pakFile_) != 8) return false;
    
    uint32_t sampleCount = header[0];
    uint32_t indexOffset = header[1];
    
    if (sampleCount == 0 || sampleCount > 10000) return false;
    
    // Read index
    if (fseek(pakFile_, indexOffset, SEEK_SET) != 0) return false;
    
    pakIndex_.resize(sampleCount);
    if (fread(pakIndex_.data(), sizeof(PreviewIdx), sampleCount, pakFile_) != sampleCount) {
        return false;
    }
    
    // Build ID lookup
    for (size_t i = 0; i < sampleCount; ++i) {
        idToIndex_[pakIndex_[i].id] = i;
    }
    
    return true;
}

void PreviewCache::evictLRU() {
    auto oldestIt = cache_.begin();
    auto oldestTime = oldestIt->second.lastAccess;
    
    for (auto it = cache_.begin(); it != cache_.end(); ++it) {
        if (!it->second.loading && it->second.lastAccess < oldestTime) {
            oldestTime = it->second.lastAccess;
            oldestIt = it;
        }
    }
    
    if (oldestIt != cache_.end()) {
        currentCacheSize_ -= oldestIt->second.data.size() * sizeof(int16_t);
        cache_.erase(oldestIt);
    }
}

//-----------------------------------------------------------------------------
// PreviewPlayer Implementation  
//-----------------------------------------------------------------------------

PreviewPlayer::PreviewPlayer() 
    : sampleRate_(48000.0f), cardGrade_(CardGrade::OK), maxVoices_(16), 
      maxBodyStreams_(2), softLimiter_(1.0f) {}

PreviewPlayer::~PreviewPlayer() {}

void PreviewPlayer::init(float sampleRate) {
    sampleRate_ = sampleRate;
    setCardGrade(cardGrade_); // Refresh limits
}

void PreviewPlayer::playStub(uint64_t id) {
    if (!scatterBak_ || !scatterBak_->isLoaded()) return;
    
    Voice* voice = allocateVoice();
    if (!voice) return;
    
    const int16_t* stubData = scatterBak_->stub(id);
    if (!stubData) return;
    
    voice->id = id;
    voice->active = true;
    voice->usingStub = true;
    voice->data = stubData;
    voice->length = scatterBak_->stubLength(id);
    voice->position = 0;
    voice->gain = scatterBak_->stubGain(id);
    voice->fadeGain = 1.0f;
    voice->startTime = std::chrono::steady_clock::now();
}

void PreviewPlayer::bridgeWhenReady(uint64_t id) {
    if (!previewCache_) return;
    
    Voice* voice = findVoice(id);
    if (!voice || !voice->usingStub) return;
    
    size_t bodyFrames;
    const int16_t* bodyData = previewCache_->body(id, bodyFrames);
    if (!bodyData) return;
    
    // Crossfade from stub to body
    voice->usingStub = false;
    voice->data = bodyData;
    voice->length = bodyFrames;
    voice->position = 0;
    voice->fadeGain = 0.0f; // Start crossfade
}

void PreviewPlayer::renderMix(float* output, size_t frames) {
    // Clear output
    std::memset(output, 0, frames * sizeof(float));
    
    int activeVoices = 0;
    float mixBuffer[frames];
    
    for (auto& voice : voices_) {
        if (!voice.active) continue;
        
        activeVoices++;
        std::memset(mixBuffer, 0, frames * sizeof(float));
        
        // Render voice
        for (size_t i = 0; i < frames && voice.position < voice.length; ++i) {
            float sample = static_cast<float>(voice.data[voice.position]) / 32768.0f;
            sample *= voice.gain;
            
            // Apply fade for crossfades
            if (voice.fadeGain < 1.0f) {
                float deltaTime = 1.0f / sampleRate_;
                voice.fadeGain = std::min(1.0f, voice.fadeGain + deltaTime / (FADE_TIME_MS / 1000.0f));
                sample *= voice.fadeGain;
            }
            
            mixBuffer[i] = sample;
            voice.position++;
        }
        
        // Check if voice finished
        if (voice.position >= voice.length) {
            voice.active = false;
        }
        
        // Mix into output with scaling for voice count
        float voiceScale = 1.0f / std::sqrt(activeVoices);
        for (size_t i = 0; i < frames; ++i) {
            output[i] += mixBuffer[i] * voiceScale;
        }
    }
    
    // Apply soft limiter
    applySoftLimiter(output, frames);
}

PreviewPlayer::Voice* PreviewPlayer::allocateVoice() {
    // Find inactive voice
    for (auto& voice : voices_) {
        if (!voice.active) return &voice;
    }
    
    // Steal oldest voice
    return findOldestVoice();
}

PreviewPlayer::Voice* PreviewPlayer::findVoice(uint64_t id) {
    for (auto& voice : voices_) {
        if (voice.active && voice.id == id) return &voice;
    }
    return nullptr;
}

void PreviewPlayer::applySoftLimiter(float* output, size_t frames) {
    for (size_t i = 0; i < frames; ++i) {
        float input = output[i];
        float absInput = std::abs(input);
        
        if (absInput > LIMITER_THRESHOLD) {
            float excess = absInput - LIMITER_THRESHOLD;
            float compressed = LIMITER_THRESHOLD + excess * 0.1f;
            output[i] = (input >= 0.0f) ? compressed : -compressed;
        }
        
        // Update limiter state for attack/release
        softLimiter_ = softLimiter_ * 0.999f + absInput * 0.001f;
    }
}

//-----------------------------------------------------------------------------
// PreviewSystem Implementation
//-----------------------------------------------------------------------------

PreviewSystem::PreviewSystem() 
    : sampleRate_(48000.0f), initialized_(false), cardGrade_(CardGrade::OK) {}

PreviewSystem::~PreviewSystem() {
    shutdown();
}

bool PreviewSystem::init(float sampleRate, const std::string& bakPath, const std::string& pakPath) {
    shutdown();
    
    sampleRate_ = sampleRate;
    
    // Create components
    scatterBak_ = std::make_shared<ScatterBak>();
    previewCache_ = std::make_shared<PreviewCache>();
    previewPlayer_ = std::make_shared<PreviewPlayer>();
    arbiter_ = std::make_shared<PreviewArbiter>();
    
    // Initialize components
    if (!scatterBak_->load(bakPath.c_str())) return false;
    
    previewCache_->openPak(pakPath.c_str());
    previewCache_->setCardGrade(cardGrade_);
    
    previewPlayer_->init(sampleRate);
    previewPlayer_->setScatterBak(scatterBak_);
    previewPlayer_->setPreviewCache(previewCache_);
    previewPlayer_->setCardGrade(cardGrade_);
    
    arbiter_->init(sampleRate);
    arbiter_->setCardGrade(cardGrade_);
    arbiter_->setScatterBak(scatterBak_);
    arbiter_->setPreviewPlayer(previewPlayer_);
    
    initialized_ = true;
    lastTickTime_ = std::chrono::steady_clock::now();
    
    return true;
}

void PreviewSystem::onMotion(float x, float y) {
    if (!initialized_) return;
    
    auto now = std::chrono::steady_clock::now();
    float deltaMs = std::chrono::duration<float, std::milli>(now - lastTickTime_).count();
    
    if (deltaMs >= TICK_INTERVAL_MS) {
        float timestamp = std::chrono::duration<float>(now.time_since_epoch()).count();
        arbiter_->tick(x, y, timestamp);
        lastTickTime_ = now;
    }
}

void PreviewSystem::renderAudio(float* output, size_t frames) {
    if (!initialized_ || !previewPlayer_) {
        std::memset(output, 0, frames * sizeof(float));
        return;
    }
    
    previewPlayer_->renderMix(output, frames);
}

bool PreviewSystem::isReady() const {
    return initialized_ && scatterBak_ && scatterBak_->isLoaded();
}

} // namespace Preview