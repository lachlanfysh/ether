#include "AutoSampleLoader.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <chrono>

// Static member definition
AutoSampleLoader::SamplerSlot AutoSampleLoader::defaultSlot_;

AutoSampleLoader::AutoSampleLoader() {
    loadingOptions_ = SampleLoadingOptions();
    
    totalMemoryLimit_ = DEFAULT_MEMORY_LIMIT;
    currentMemoryUsage_.store(0);
    memoryWarningThreshold_ = DEFAULT_MEMORY_WARNING_THRESHOLD;
    
    sequencer_ = nullptr;
    tapeSquashing_ = nullptr;
    
    // Initialize all slots as empty
    for (uint8_t i = 0; i < MAX_SLOTS; ++i) {
        slots_[i] = SamplerSlot();
        slots_[i].slotId = i;
        slots_[i].isOccupied = false;
    }
}

// Configuration
void AutoSampleLoader::setSampleLoadingOptions(const SampleLoadingOptions& options) {
    loadingOptions_ = options;
    
    // Validate and clamp options
    loadingOptions_.targetLevel = std::max(-48.0f, std::min(loadingOptions_.targetLevel, 0.0f));
    if (loadingOptions_.preferredSlot >= MAX_SLOTS) {
        loadingOptions_.preferredSlot = 255;  // Auto-select
    }
}

// Sample Loading
AutoSampleLoader::LoadingResult AutoSampleLoader::loadSample(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData,
                                                            const std::string& sourceName) {
    LoadingResult result;
    
    if (!validateSampleData(audioData)) {
        result.success = false;
        result.errorMessage = "Invalid sample data";
        notifyError(result.errorMessage);
        return result;
    }
    
    if (!hasEnoughMemoryForSample(audioData)) {
        result.success = false;
        result.errorMessage = "Insufficient memory for sample";
        notifyError(result.errorMessage);
        return result;
    }
    
    uint8_t selectedSlot = selectSlotByStrategy(loadingOptions_);
    if (selectedSlot >= MAX_SLOTS) {
        result.success = false;
        result.errorMessage = "No available slots found";
        notifyError(result.errorMessage);
        return result;
    }
    
    return loadSampleToSlot(selectedSlot, audioData, sourceName);
}

AutoSampleLoader::LoadingResult AutoSampleLoader::loadSampleToSlot(uint8_t slot, 
                                                                  std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData,
                                                                  const std::string& sourceName) {
    LoadingResult result;
    
    if (!validateSlot(slot)) {
        result.success = false;
        result.errorMessage = "Invalid slot number";
        notifyError(result.errorMessage);
        return result;
    }
    
    if (!validateSampleData(audioData)) {
        result.success = false;
        result.errorMessage = "Invalid sample data";
        notifyError(result.errorMessage);
        return result;
    }
    
    // Check if slot is protected
    if (isSlotProtected(slot)) {
        result.success = false;
        result.errorMessage = "Slot is protected from overwriting";
        notifyError(result.errorMessage);
        return result;
    }
    
    std::lock_guard<std::mutex> lock(slotsMutex_);
    
    // Check if replacing existing sample
    bool replacingExisting = slots_[slot].isOccupied;
    SampleMetadata replacedMetadata;
    if (replacingExisting) {
        replacedMetadata = slots_[slot].metadata;
        
        // Ask for confirmation if callback is set
        if (slotOverwriteCallback_) {
            if (!slotOverwriteCallback_(slot, replacedMetadata)) {
                result.success = false;
                result.errorMessage = "Sample overwrite cancelled by user";
                return result;
            }
        }
    }
    
    // Process sample if needed
    auto processedAudio = processSample(audioData, loadingOptions_);
    if (!processedAudio) {
        result.success = false;
        result.errorMessage = "Sample processing failed";
        notifyError(result.errorMessage);
        return result;
    }
    
    // Generate sample name and metadata
    std::string sampleName = generateSampleName(slot, sourceName, loadingOptions_);
    SampleMetadata metadata = generateMetadata(processedAudio, sourceName, loadingOptions_);
    metadata.name = sampleName;
    
    // Calculate memory usage
    size_t memoryUsage = calculateSampleMemoryUsage(processedAudio);
    
    // Update slot
    slots_[slot].slotId = slot;
    slots_[slot].isOccupied = true;
    slots_[slot].audioData = processedAudio;
    slots_[slot].metadata = metadata;
    slots_[slot].loadTime = getCurrentTimeMs();
    slots_[slot].lastAccessTime = getCurrentTimeMs();
    slots_[slot].memoryUsage = memoryUsage;
    slots_[slot].isProtected = false;
    
    // Update memory tracking
    updateMemoryUsage();
    
    // Prepare result
    result.success = true;
    result.assignedSlot = slot;
    result.sampleName = sampleName;
    result.memoryUsed = memoryUsage;
    result.replacedExistingSample = replacingExisting;
    result.replacedSampleMetadata = replacedMetadata;
    
    // Notify completion
    notifyLoadingComplete(result);
    
    return result;
}

// Slot Management
uint8_t AutoSampleLoader::findNextAvailableSlot() const {
    std::lock_guard<std::mutex> lock(slotsMutex_);
    return findNextAvailableSlotInternal();
}

uint8_t AutoSampleLoader::findOptimalSlot(const SampleLoadingOptions& options) const {
    return selectSlotByStrategy(options);
}

bool AutoSampleLoader::isSlotAvailable(uint8_t slot) const {
    if (!validateSlot(slot)) return false;
    
    std::lock_guard<std::mutex> lock(slotsMutex_);
    return !slots_[slot].isOccupied;
}

bool AutoSampleLoader::isSlotProtected(uint8_t slot) const {
    if (!validateSlot(slot)) return false;
    
    std::lock_guard<std::mutex> lock(slotsMutex_);
    return slots_[slot].isProtected;
}

void AutoSampleLoader::setSlotProtected(uint8_t slot, bool isProtected) {
    if (!validateSlot(slot)) return;
    
    std::lock_guard<std::mutex> lock(slotsMutex_);
    slots_[slot].isProtected = isProtected;
}

// Sample Information
const AutoSampleLoader::SamplerSlot& AutoSampleLoader::getSlot(uint8_t slot) const {
    if (!validateSlot(slot)) return defaultSlot_;
    
    std::lock_guard<std::mutex> lock(slotsMutex_);
    return slots_[slot];
}

std::vector<uint8_t> AutoSampleLoader::getOccupiedSlots() const {
    std::lock_guard<std::mutex> lock(slotsMutex_);
    std::vector<uint8_t> occupied;
    
    for (uint8_t i = 0; i < MAX_SLOTS; ++i) {
        if (slots_[i].isOccupied) {
            occupied.push_back(i);
        }
    }
    
    return occupied;
}

std::vector<uint8_t> AutoSampleLoader::getAvailableSlots() const {
    std::lock_guard<std::mutex> lock(slotsMutex_);
    std::vector<uint8_t> available;
    
    for (uint8_t i = 0; i < MAX_SLOTS; ++i) {
        if (!slots_[i].isOccupied) {
            available.push_back(i);
        }
    }
    
    return available;
}

size_t AutoSampleLoader::getTotalMemoryUsage() const {
    return currentMemoryUsage_.load();
}

size_t AutoSampleLoader::getAvailableMemory() const {
    size_t used = currentMemoryUsage_.load();
    return (used < totalMemoryLimit_) ? (totalMemoryLimit_ - used) : 0;
}

// Sample Management
bool AutoSampleLoader::removeSample(uint8_t slot) {
    if (!validateSlot(slot)) return false;
    
    std::lock_guard<std::mutex> lock(slotsMutex_);
    
    if (!slots_[slot].isOccupied) {
        return false;  // Slot already empty
    }
    
    // Clear slot
    slots_[slot].isOccupied = false;
    slots_[slot].audioData.reset();
    slots_[slot].metadata = SampleMetadata();
    slots_[slot].memoryUsage = 0;
    slots_[slot].isProtected = false;
    
    // Update memory usage
    updateMemoryUsage();
    
    return true;
}

bool AutoSampleLoader::moveSample(uint8_t fromSlot, uint8_t toSlot) {
    if (!validateSlot(fromSlot) || !validateSlot(toSlot)) return false;
    if (fromSlot == toSlot) return true;
    
    std::lock_guard<std::mutex> lock(slotsMutex_);
    
    if (!slots_[fromSlot].isOccupied) {
        return false;  // Source slot is empty
    }
    
    if (slots_[toSlot].isOccupied) {
        return false;  // Destination slot is occupied
    }
    
    // Move sample data
    slots_[toSlot] = slots_[fromSlot];
    slots_[toSlot].slotId = toSlot;
    
    // Clear source slot
    slots_[fromSlot] = SamplerSlot();
    slots_[fromSlot].slotId = fromSlot;
    slots_[fromSlot].isOccupied = false;
    
    return true;
}

void AutoSampleLoader::clearAllSamples() {
    std::lock_guard<std::mutex> lock(slotsMutex_);
    
    for (uint8_t i = 0; i < MAX_SLOTS; ++i) {
        slots_[i] = SamplerSlot();
        slots_[i].slotId = i;
        slots_[i].isOccupied = false;
    }
    
    currentMemoryUsage_.store(0);
}

void AutoSampleLoader::clearUnprotectedSamples() {
    std::lock_guard<std::mutex> lock(slotsMutex_);
    
    for (uint8_t i = 0; i < MAX_SLOTS; ++i) {
        if (slots_[i].isOccupied && !slots_[i].isProtected) {
            slots_[i] = SamplerSlot();
            slots_[i].slotId = i;
            slots_[i].isOccupied = false;
        }
    }
    
    updateMemoryUsage();
}

// Sample Access Tracking
void AutoSampleLoader::notifySlotAccessed(uint8_t slot) {
    if (!validateSlot(slot)) return;
    
    std::lock_guard<std::mutex> lock(slotsMutex_);
    if (slots_[slot].isOccupied) {
        slots_[slot].lastAccessTime = getCurrentTimeMs();
    }
}

uint32_t AutoSampleLoader::getSlotLastAccessTime(uint8_t slot) const {
    if (!validateSlot(slot)) return 0;
    
    std::lock_guard<std::mutex> lock(slotsMutex_);
    return slots_[slot].lastAccessTime;
}

std::vector<uint8_t> AutoSampleLoader::getSlotsByLastAccess() const {
    std::lock_guard<std::mutex> lock(slotsMutex_);
    
    std::vector<std::pair<uint8_t, uint32_t>> slotTimes;
    for (uint8_t i = 0; i < MAX_SLOTS; ++i) {
        if (slots_[i].isOccupied) {
            slotTimes.emplace_back(i, slots_[i].lastAccessTime);
        }
    }
    
    // Sort by last access time (oldest first)
    std::sort(slotTimes.begin(), slotTimes.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    std::vector<uint8_t> sortedSlots;
    for (const auto& entry : slotTimes) {
        sortedSlots.push_back(entry.first);
    }
    
    return sortedSlots;
}

// Sample Processing
std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> AutoSampleLoader::processSample(
    std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> input,
    const SampleLoadingOptions& options) const {
    
    if (!input) return nullptr;
    
    // Create copy for processing
    auto processed = std::make_shared<RealtimeAudioBouncer::CapturedAudio>(*input);
    
    // Apply processing options
    if (options.enableAutoTrim) {
        trimSampleSilence(processed);
    }
    
    if (options.enableNormalization) {
        normalizeSample(processed, options.targetLevel);
    }
    
    return processed;
}

bool AutoSampleLoader::trimSampleSilence(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audio) const {
    if (!audio || audio->audioData.empty()) return false;
    
    float threshold = dbToLinear(DEFAULT_SILENCE_THRESHOLD);
    uint32_t channels = audio->channels;
    uint32_t samples = audio->sampleCount;
    
    // Find first non-silent sample
    uint32_t trimStart = 0;
    for (uint32_t i = 0; i < samples; ++i) {
        bool isSilent = true;
        for (uint32_t ch = 0; ch < channels; ++ch) {
            if (std::abs(audio->audioData[i * channels + ch]) > threshold) {
                isSilent = false;
                break;
            }
        }
        if (!isSilent) {
            trimStart = i;
            break;
        }
    }
    
    // Find last non-silent sample
    uint32_t trimEnd = samples;
    for (int32_t i = samples - 1; i >= static_cast<int32_t>(trimStart); --i) {
        bool isSilent = true;
        for (uint32_t ch = 0; ch < channels; ++ch) {
            if (std::abs(audio->audioData[i * channels + ch]) > threshold) {
                isSilent = false;
                break;
            }
        }
        if (!isSilent) {
            trimEnd = i + 1;
            break;
        }
    }
    
    // Apply trimming if needed
    if (trimStart > 0 || trimEnd < samples) {
        uint32_t newSampleCount = trimEnd - trimStart;
        std::vector<float> trimmedData;
        trimmedData.reserve(newSampleCount * channels);
        
        for (uint32_t i = trimStart; i < trimEnd; ++i) {
            for (uint32_t ch = 0; ch < channels; ++ch) {
                trimmedData.push_back(audio->audioData[i * channels + ch]);
            }
        }
        
        audio->audioData = std::move(trimmedData);
        audio->sampleCount = newSampleCount;
        
        return true;
    }
    
    return false;
}

bool AutoSampleLoader::normalizeSample(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audio, float targetLevel) const {
    if (!audio || audio->audioData.empty()) return false;
    
    float currentPeak = findSamplePeak(audio->audioData);
    if (currentPeak <= 0.0f) return false;
    
    float targetLinear = dbToLinear(targetLevel);
    float gainMultiplier = targetLinear / currentPeak;
    
    // Apply gain
    for (float& sample : audio->audioData) {
        sample *= gainMultiplier;
    }
    
    // Update metadata
    audio->peakLevel = targetLevel;
    audio->rmsLevel += linearToDb(gainMultiplier);  // Adjust RMS by gain amount
    
    return true;
}

// Metadata Management
AutoSampleLoader::SampleMetadata AutoSampleLoader::generateMetadata(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData,
                                                                    const std::string& sourceName, 
                                                                    const SampleLoadingOptions& options) const {
    SampleMetadata metadata;
    
    if (audioData) {
        metadata.format = audioData->format;
        metadata.sampleCount = audioData->sampleCount;
        metadata.peakLevel = audioData->peakLevel;
        metadata.rmsLevel = audioData->rmsLevel;
    }
    
    metadata.sourceDescription = sourceName;
    metadata.creationTime = getCurrentTimeMs();
    
    // Add tags based on source
    if (sourceName.find("Crush") != std::string::npos || 
        sourceName.find("Tape") != std::string::npos) {
        metadata.tags.push_back("crushed");
        metadata.tags.push_back("tape_squash");
    }
    
    return metadata;
}

// Callbacks
void AutoSampleLoader::setLoadingCompleteCallback(LoadingCompleteCallback callback) {
    loadingCompleteCallback_ = callback;
}

void AutoSampleLoader::setSlotOverwriteCallback(SlotOverwriteCallback callback) {
    slotOverwriteCallback_ = callback;
}

void AutoSampleLoader::setMemoryWarningCallback(MemoryWarningCallback callback) {
    memoryWarningCallback_ = callback;
}

void AutoSampleLoader::setErrorCallback(ErrorCallback callback) {
    errorCallback_ = callback;
}

// Memory Management
bool AutoSampleLoader::hasEnoughMemoryForSample(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData) const {
    if (!audioData) return false;
    
    size_t requiredMemory = calculateSampleMemoryUsage(audioData);
    size_t availableMemory = getAvailableMemory();
    
    return requiredMemory <= availableMemory;
}

// Internal methods
uint8_t AutoSampleLoader::selectSlotByStrategy(const SampleLoadingOptions& options) const {
    switch (options.strategy) {
        case SlotAllocationStrategy::NEXT_AVAILABLE:
            return findNextAvailableSlot();
            
        case SlotAllocationStrategy::LEAST_RECENTLY_USED:
            return findLeastRecentlyUsedSlot();
            
        case SlotAllocationStrategy::ROUND_ROBIN:
        case SlotAllocationStrategy::PRIORITY_BASED:
        case SlotAllocationStrategy::USER_PREFERENCE:
        case SlotAllocationStrategy::MEMORY_OPTIMIZED:
        default:
            return findNextAvailableSlot();  // Default to next available
    }
}

uint8_t AutoSampleLoader::findNextAvailableSlotInternal() const {
    for (uint8_t i = 0; i < MAX_SLOTS; ++i) {
        if (!slots_[i].isOccupied) {
            return i;
        }
    }
    return MAX_SLOTS;  // No slots available
}

uint8_t AutoSampleLoader::findLeastRecentlyUsedSlot() const {
    uint32_t oldestTime = UINT32_MAX;
    uint8_t oldestSlot = MAX_SLOTS;
    
    for (uint8_t i = 0; i < MAX_SLOTS; ++i) {
        if (slots_[i].isOccupied && !slots_[i].isProtected) {
            if (slots_[i].lastAccessTime < oldestTime) {
                oldestTime = slots_[i].lastAccessTime;
                oldestSlot = i;
            }
        }
    }
    
    return oldestSlot;
}

std::string AutoSampleLoader::generateSampleName(uint8_t slot, const std::string& sourceName, 
                                                 const SampleLoadingOptions& options) const {
    if (!options.enableAutoNaming) {
        return sourceName.empty() ? "Sample" : sourceName;
    }
    
    return expandNameTemplate(options.nameTemplate, slot, sourceName);
}

std::string AutoSampleLoader::expandNameTemplate(const std::string& nameTemplate, uint8_t slot, 
                                                 const std::string& sourceName) const {
    std::string result = nameTemplate;
    
    // Replace placeholders
    size_t pos = 0;
    while ((pos = result.find("{slot}", pos)) != std::string::npos) {
        result.replace(pos, 6, std::to_string(slot + 1));  // 1-based slot numbers
        pos += 1;
    }
    
    pos = 0;
    while ((pos = result.find("{timestamp}", pos)) != std::string::npos) {
        result.replace(pos, 11, std::to_string(getCurrentTimeMs()));
        pos += 1;
    }
    
    pos = 0;
    while ((pos = result.find("{source}", pos)) != std::string::npos) {
        std::string cleanSource = sourceName;
        if (cleanSource.empty()) cleanSource = "Unknown";
        result.replace(pos, 8, cleanSource);
        pos += cleanSource.length();
    }
    
    return result;
}

size_t AutoSampleLoader::calculateSampleMemoryUsage(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData) const {
    if (!audioData) return 0;
    
    return audioData->audioData.size() * sizeof(float);
}

void AutoSampleLoader::updateMemoryUsage() {
    size_t totalUsage = 0;
    
    for (uint8_t i = 0; i < MAX_SLOTS; ++i) {
        if (slots_[i].isOccupied) {
            totalUsage += slots_[i].memoryUsage;
        }
    }
    
    size_t oldUsage = currentMemoryUsage_.exchange(totalUsage);
    
    // Check for memory warning
    if (totalUsage > oldUsage && 
        static_cast<float>(totalUsage) / totalMemoryLimit_ > memoryWarningThreshold_) {
        notifyMemoryWarning();
    }
}

bool AutoSampleLoader::validateSlot(uint8_t slot) const {
    return slot < MAX_SLOTS;
}

bool AutoSampleLoader::validateSampleData(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData) const {
    return audioData && 
           audioData->sampleCount > 0 && 
           !audioData->audioData.empty() &&
           audioData->audioData.size() == audioData->sampleCount * audioData->channels;
}

// Notification methods
void AutoSampleLoader::notifyLoadingComplete(const LoadingResult& result) {
    if (loadingCompleteCallback_) {
        loadingCompleteCallback_(result);
    }
}

void AutoSampleLoader::notifyMemoryWarning() {
    if (memoryWarningCallback_) {
        memoryWarningCallback_(currentMemoryUsage_.load(), totalMemoryLimit_);
    }
}

void AutoSampleLoader::notifyError(const std::string& error) {
    if (errorCallback_) {
        errorCallback_(error);
    }
}

// Utility methods
uint32_t AutoSampleLoader::getCurrentTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

float AutoSampleLoader::linearToDb(float linear) const {
    if (linear <= 0.0f) return -96.0f;
    return 20.0f * std::log10(linear);
}

float AutoSampleLoader::dbToLinear(float db) const {
    return std::pow(10.0f, db / 20.0f);
}

float AutoSampleLoader::findSamplePeak(const std::vector<float>& audioData) const {
    if (audioData.empty()) return 0.0f;
    
    float peak = 0.0f;
    for (float sample : audioData) {
        peak = std::max(peak, std::abs(sample));
    }
    
    return peak;
}