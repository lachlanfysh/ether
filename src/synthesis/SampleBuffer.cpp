#include "SampleBuffer.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <chrono>

namespace Sample {

// Static cache instance
std::shared_ptr<SampleCache> SampleBuffer::globalCache_ = nullptr;

//-----------------------------------------------------------------------------
// RingBuffer Implementation
//-----------------------------------------------------------------------------

RingBuffer::RingBuffer(size_t capacity) 
    : capacity_(nextPowerOf2(capacity)), mask_(capacity_ - 1) {
    buffer_.resize(capacity_);
}

RingBuffer::~RingBuffer() = default;

size_t RingBuffer::nextPowerOf2(size_t n) {
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

size_t RingBuffer::write(const int16_t* data, size_t frames) {
    const size_t writePos = writePos_.load(std::memory_order_acquire);
    const size_t readPos = readPos_.load(std::memory_order_acquire);
    
    size_t available_space = (readPos - writePos - 1) & mask_;
    size_t to_write = std::min(frames, available_space);
    
    if (to_write == 0) return 0;
    
    size_t writeIndex = writePos & mask_;
    size_t firstChunk = std::min(to_write, capacity_ - writeIndex);
    
    std::memcpy(&buffer_[writeIndex], data, firstChunk * sizeof(int16_t));
    if (to_write > firstChunk) {
        std::memcpy(&buffer_[0], data + firstChunk, (to_write - firstChunk) * sizeof(int16_t));
    }
    
    writePos_.store((writePos + to_write) & mask_, std::memory_order_release);
    return to_write;
}

size_t RingBuffer::read(int16_t* dest, size_t frames) {
    const size_t writePos = writePos_.load(std::memory_order_acquire);
    const size_t readPos = readPos_.load(std::memory_order_acquire);
    
    size_t available_data = (writePos - readPos) & mask_;
    size_t to_read = std::min(frames, available_data);
    
    if (to_read == 0) return 0;
    
    size_t readIndex = readPos & mask_;
    size_t firstChunk = std::min(to_read, capacity_ - readIndex);
    
    std::memcpy(dest, &buffer_[readIndex], firstChunk * sizeof(int16_t));
    if (to_read > firstChunk) {
        std::memcpy(dest + firstChunk, &buffer_[0], (to_read - firstChunk) * sizeof(int16_t));
    }
    
    readPos_.store((readPos + to_read) & mask_, std::memory_order_release);
    return to_read;
}

size_t RingBuffer::available() const {
    const size_t writePos = writePos_.load(std::memory_order_acquire);
    const size_t readPos = readPos_.load(std::memory_order_acquire);
    return (writePos - readPos) & mask_;
}

size_t RingBuffer::space() const {
    const size_t writePos = writePos_.load(std::memory_order_acquire);
    const size_t readPos = readPos_.load(std::memory_order_acquire);
    return (readPos - writePos - 1) & mask_;
}

void RingBuffer::reset() {
    writePos_.store(0, std::memory_order_release);
    readPos_.store(0, std::memory_order_release);
}

//-----------------------------------------------------------------------------
// LagrangeResampler Implementation
//-----------------------------------------------------------------------------

LagrangeResampler::LagrangeResampler() 
    : ratio_(1.0f), phase_(0.0), initialized_(false) {
    std::memset(history_, 0, sizeof(history_));
}

LagrangeResampler::~LagrangeResampler() = default;

void LagrangeResampler::setPitchRatio(float ratio) {
    ratio_ = std::clamp(ratio, 0.25f, 4.0f);  // ±2 octaves
}

void LagrangeResampler::reset() {
    phase_ = 0.0;
    std::memset(history_, 0, sizeof(history_));
    initialized_ = false;
}

int16_t LagrangeResampler::interpolate(const int16_t* samples, double frac) const {
    // 4-point Lagrange interpolation
    double y0 = samples[0];
    double y1 = samples[1];
    double y2 = samples[2];
    double y3 = samples[3];
    
    double c0 = y1;
    double c1 = 0.5 * (y2 - y0);
    double c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
    double c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);
    
    double result = c0 + c1 * frac + c2 * frac * frac + c3 * frac * frac * frac;
    return static_cast<int16_t>(std::clamp(result, -32768.0, 32767.0));
}

void LagrangeResampler::process(const int16_t* input, size_t inputFrames, 
                               int16_t* output, size_t& outputFrames, bool loop) {
    if (ratio_ == 1.0f) {
        // No resampling needed
        size_t copyFrames = std::min(inputFrames, outputFrames);
        std::memcpy(output, input, copyFrames * sizeof(int16_t));
        outputFrames = copyFrames;
        return;
    }
    
    size_t outputIndex = 0;
    size_t maxOutput = outputFrames;
    
    for (size_t i = 0; i < inputFrames && outputIndex < maxOutput; ) {
        // Update history buffer
        if (!initialized_ && i >= 3) {
            std::memcpy(history_, &input[i - 3], 4 * sizeof(int16_t));
            initialized_ = true;
        }
        
        if (initialized_) {
            // Interpolate current sample
            double frac = phase_ - std::floor(phase_);
            output[outputIndex] = interpolate(history_, frac);
            outputIndex++;
        }
        
        // Advance phase
        phase_ += ratio_;
        
        // Update input position and history
        while (phase_ >= 1.0 && i < inputFrames) {
            phase_ -= 1.0;
            
            // Shift history
            std::memmove(history_, history_ + 1, 3 * sizeof(int16_t));
            history_[3] = input[i];
            i++;
        }
    }
    
    outputFrames = outputIndex;
}

//-----------------------------------------------------------------------------
// WavLoader Implementation
//-----------------------------------------------------------------------------

bool WavLoader::parseHeader(FILE* file, WavHeader& header, SampleInfo& info) {
    if (fread(&header, sizeof(WavHeader), 1, file) != 1) {
        return false;
    }
    
    // Validate RIFF header
    if (std::memcmp(header.riff, "RIFF", 4) != 0 || 
        std::memcmp(header.wave, "WAVE", 4) != 0 ||
        std::memcmp(header.fmt, "fmt ", 4) != 0) {
        return false;
    }
    
    // Only support PCM format
    if (header.format != 1) {
        return false;
    }
    
    // Validate bit depth
    if (header.bitsPerSample != 16 && header.bitsPerSample != 24) {
        return false;
    }
    
    // Find data chunk
    if (std::memcmp(header.data, "data", 4) != 0) {
        // Search for data chunk
        char chunk[4];
        uint32_t chunkSize;
        while (fread(chunk, 4, 1, file) == 1) {
            fread(&chunkSize, 4, 1, file);
            if (std::memcmp(chunk, "data", 4) == 0) {
                header.dataSize = chunkSize;
                break;
            }
            fseek(file, chunkSize, SEEK_CUR);
        }
    }
    
    // Fill sample info
    info.sampleRate = header.sampleRate;
    info.channels = header.channels;
    info.bitDepth = header.bitsPerSample;
    info.totalFrames = header.dataSize / (header.channels * header.bitsPerSample / 8);
    info.durationSeconds = static_cast<float>(info.totalFrames) / info.sampleRate;
    info.isValid = true;
    
    return true;
}

bool WavLoader::loadSampleInfo(const std::string& filePath, SampleInfo& info) {
    FILE* file = fopen(filePath.c_str(), "rb");
    if (!file) return false;
    
    WavHeader header;
    bool success = parseHeader(file, header, info);
    info.filePath = filePath;
    
    fclose(file);
    return success;
}

bool WavLoader::loadToRAM(const std::string& filePath, std::vector<int16_t>& buffer, SampleInfo& info) {
    FILE* file = fopen(filePath.c_str(), "rb");
    if (!file) return false;
    
    WavHeader header;
    if (!parseHeader(file, header, info)) {
        fclose(file);
        return false;
    }
    
    // Allocate buffer for interleaved 16-bit samples
    size_t totalSamples = info.totalFrames * info.channels;
    buffer.resize(totalSamples);
    
    if (info.bitDepth == 16) {
        // Direct read for 16-bit
        size_t samplesRead = fread(buffer.data(), sizeof(int16_t), totalSamples, file);
        if (samplesRead != totalSamples) {
            fclose(file);
            return false;
        }
    } else {
        // Convert from 24-bit to 16-bit
        std::vector<uint8_t> tempBuffer(header.dataSize);
        if (fread(tempBuffer.data(), 1, header.dataSize, file) != header.dataSize) {
            fclose(file);
            return false;
        }
        convertTo16Bit(tempBuffer.data(), buffer.data(), info.totalFrames, 
                      info.channels, info.bitDepth);
    }
    
    fclose(file);
    info.filePath = filePath;
    return true;
}

void WavLoader::convertTo16Bit(const uint8_t* input, int16_t* output, size_t frames, 
                              int channels, int bitDepth) {
    if (bitDepth == 24) {
        for (size_t i = 0; i < frames * channels; ++i) {
            // Convert 24-bit to 16-bit (little-endian)
            int32_t sample = (input[i * 3 + 0]) | 
                           (input[i * 3 + 1] << 8) | 
                           (input[i * 3 + 2] << 16);
            
            // Sign extend
            if (sample & 0x800000) {
                sample |= 0xFF000000;
            }
            
            // Scale down to 16-bit
            output[i] = static_cast<int16_t>(sample >> 8);
        }
    }
}

//-----------------------------------------------------------------------------
// SampleCache Implementation  
//-----------------------------------------------------------------------------

SampleCache::SampleCache(size_t maxSizeBytes) : maxSize_(maxSizeBytes), currentSize_(0) {}

SampleCache::~SampleCache() = default;

bool SampleCache::get(const std::string& key, std::vector<int16_t>& buffer, SampleInfo& info) {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        buffer = it->second.buffer;
        info = it->second.info;
        it->second.lastAccess = std::chrono::steady_clock::now();
        return true;
    }
    
    return false;
}

void SampleCache::put(const std::string& key, const std::vector<int16_t>& buffer, const SampleInfo& info) {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    
    size_t entrySize = calculateSize(buffer);
    
    // Evict if necessary
    while (currentSize_ + entrySize > maxSize_ && !cache_.empty()) {
        evictLRU();
    }
    
    // Add new entry
    if (currentSize_ + entrySize <= maxSize_) {
        cache_[key] = {buffer, info, std::chrono::steady_clock::now(), entrySize};
        currentSize_ += entrySize;
    }
}

void SampleCache::evictLRU() {
    auto oldestIt = cache_.begin();
    auto oldestTime = oldestIt->second.lastAccess;
    
    for (auto it = cache_.begin(); it != cache_.end(); ++it) {
        if (it->second.lastAccess < oldestTime) {
            oldestTime = it->second.lastAccess;
            oldestIt = it;
        }
    }
    
    currentSize_ -= oldestIt->second.size;
    cache_.erase(oldestIt);
}

size_t SampleCache::calculateSize(const std::vector<int16_t>& buffer) const {
    return buffer.size() * sizeof(int16_t);
}

void SampleCache::clear() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    cache_.clear();
    currentSize_ = 0;
}

//-----------------------------------------------------------------------------
// SampleBuffer Implementation
//-----------------------------------------------------------------------------

SampleBuffer::SampleBuffer() 
    : mode_(Mode::RAM), loaded_(false), playing_(false), loop_(false),
      playPosition_(0), fileHandle_(nullptr), stopStreaming_(false),
      pitchRatio_(1.0f), hasStub_(false), hasPreview_(false) {
    
    resampler_ = std::make_unique<LagrangeResampler>();
}

SampleBuffer::~SampleBuffer() {
    unload();
}

bool SampleBuffer::load(const std::string& filePath, float thresholdMB) {
    unload();
    
    // Load sample info first
    if (!WavLoader::loadSampleInfo(filePath, info_)) {
        return false;
    }
    
    // Estimate file size
    size_t estimatedSize = info_.totalFrames * info_.channels * sizeof(int16_t);
    float sizeMB = static_cast<float>(estimatedSize) / (1024.0f * 1024.0f);
    
    // Check cache first
    if (globalCache_) {
        std::vector<int16_t> cachedBuffer;
        SampleInfo cachedInfo;
        if (globalCache_->get(filePath, cachedBuffer, cachedInfo)) {
            ramBuffer_ = std::move(cachedBuffer);
            info_ = cachedInfo;
            mode_ = Mode::RAM;
            loaded_ = true;
            return true;
        }
    }
    
    if (sizeMB <= thresholdMB) {
        // Load to RAM
        if (WavLoader::loadToRAM(filePath, ramBuffer_, info_)) {
            mode_ = Mode::RAM;
            loaded_ = true;
            
            // Add to cache
            if (globalCache_) {
                globalCache_->put(filePath, ramBuffer_, info_);
            }
            
            return true;
        }
    } else {
        // Set up streaming
        if (WavLoader::openForStreaming(filePath, info_, fileHandle_)) {
            mode_ = Mode::STREAMING;
            ringBuffer_ = std::make_unique<RingBuffer>(48000 * 4);  // 4 seconds buffer
            loaded_ = true;
            return true;
        }
    }
    
    return false;
}

void SampleBuffer::startPlayback(float startPosition, bool loop) {
    if (!loaded_) return;
    
    loop_.store(loop);
    playing_.store(true);
    
    if (mode_ == Mode::RAM) {
        size_t startFrame = static_cast<size_t>(startPosition * info_.totalFrames);
        playPosition_.store(startFrame * info_.channels);
    } else {
        // Start streaming thread
        stopStreaming_.store(false);
        if (streamThread_.joinable()) streamThread_.join();
        streamThread_ = std::thread(&SampleBuffer::streamingThread, this);
    }
}

void SampleBuffer::stopPlayback() {
    playing_.store(false);
    
    if (mode_ == Mode::STREAMING) {
        stopStreaming_.store(true);
        if (streamThread_.joinable()) {
            streamThread_.join();
        }
    }
}

void SampleBuffer::streamingThread() {
    constexpr size_t CHUNK_SIZE = 1024;
    int16_t chunk[CHUNK_SIZE];
    
    while (!stopStreaming_.load() && fileHandle_) {
        if (ringBuffer_->space() >= CHUNK_SIZE) {
            size_t framesRead = WavLoader::readFrames(fileHandle_, chunk, CHUNK_SIZE / info_.channels);
            if (framesRead > 0) {
                ringBuffer_->write(chunk, framesRead * info_.channels);
            } else {
                // End of file
                if (loop_.load()) {
                    // Restart file
                    // TODO: Implement file seeking for loop
                } else {
                    break;
                }
            }
        } else {
            // Buffer full, wait a bit
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}

void SampleBuffer::renderSamples(int16_t* output, size_t frames, float gain) {
    if (!loaded_ || !playing_.load()) {
        std::memset(output, 0, frames * sizeof(int16_t));
        return;
    }
    
    // Apply gain (convert to fixed point for efficiency)
    int32_t gainFixed = static_cast<int32_t>(gain * 32768.0f);
    
    size_t rendered = 0;
    if (mode_ == Mode::RAM) {
        rendered = renderRAM(output, frames, gain);
    } else {
        rendered = renderStreaming(output, frames, gain);
    }
    
    // Fill remaining with silence
    if (rendered < frames) {
        std::memset(output + rendered, 0, (frames - rendered) * sizeof(int16_t));
    }
    
    // Apply resampling if needed
    if (pitchRatio_ != 1.0f && rendered > 0) {
        std::vector<int16_t> temp(frames);
        std::memcpy(temp.data(), output, rendered * sizeof(int16_t));
        
        size_t outputFrames = frames;
        resampler_->process(temp.data(), rendered, output, outputFrames, loop_.load());
    }
}

size_t SampleBuffer::renderRAM(int16_t* output, size_t frames, float gain) {
    size_t pos = playPosition_.load();
    size_t totalSamples = ramBuffer_.size();
    
    if (pos >= totalSamples) {
        playing_.store(false);
        return 0;
    }
    
    size_t samplesToRender = std::min(frames, totalSamples - pos);
    
    // Copy with gain
    int32_t gainFixed = static_cast<int32_t>(gain * 32768.0f);
    for (size_t i = 0; i < samplesToRender; ++i) {
        int32_t sample = (ramBuffer_[pos + i] * gainFixed) >> 15;
        output[i] = static_cast<int16_t>(std::clamp(sample, -32768, 32767));
    }
    
    // Update position
    size_t newPos = pos + samplesToRender;
    if (newPos >= totalSamples) {
        if (loop_.load()) {
            newPos = 0;
        } else {
            playing_.store(false);
        }
    }
    playPosition_.store(newPos);
    
    return samplesToRender;
}

size_t SampleBuffer::renderStreaming(int16_t* output, size_t frames, float gain) {
    if (!ringBuffer_) return 0;
    
    size_t rendered = ringBuffer_->read(output, frames);
    
    // Apply gain
    int32_t gainFixed = static_cast<int32_t>(gain * 32768.0f);
    for (size_t i = 0; i < rendered; ++i) {
        int32_t sample = (output[i] * gainFixed) >> 15;
        output[i] = static_cast<int16_t>(std::clamp(sample, -32768, 32767));
    }
    
    return rendered;
}

void SampleBuffer::setCacheInstance(std::shared_ptr<SampleCache> cache) {
    globalCache_ = cache;
}

std::shared_ptr<SampleCache> SampleBuffer::getCacheInstance() {
    return globalCache_;
}

} // namespace Sample