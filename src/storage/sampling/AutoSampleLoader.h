#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include "../audio/RealtimeAudioBouncer.h"

/**
 * AutoSampleLoader - Automatic sample loading into next available sampler slot
 * 
 * Provides automatic sample management and loading functionality:
 * - Automatic detection of next available sampler slot
 * - Sample format conversion and optimization for real-time playback
 * - Integration with tape squashing workflow for seamless sample loading
 * - Sample metadata management and naming conventions
 * - Memory management and slot allocation optimization
 * - Hardware-optimized for STM32 H7 embedded platform
 * 
 * Features:
 * - Intelligent slot selection algorithm (round-robin, least-recently-used, priority-based)
 * - Automatic sample rate conversion and bit depth optimization
 * - Sample trimming and loop point detection
 * - Metadata preservation (name, source, creation time, parameters)
 * - Slot usage tracking and memory optimization
 * - Integration with sequencer for immediate sample playback
 * - Conflict resolution for slot allocation
 * - Sample backup and recovery for overwritten samples
 */
class AutoSampleLoader {
public:
    // Sample slot allocation strategies
    enum class SlotAllocationStrategy {
        NEXT_AVAILABLE,     // Use next chronologically available slot
        ROUND_ROBIN,        // Cycle through slots in order
        LEAST_RECENTLY_USED, // Replace least recently used sample
        PRIORITY_BASED,     // Use priority scoring for slot selection
        USER_PREFERENCE,    // Prefer user-designated slots
        MEMORY_OPTIMIZED    // Optimize for memory usage
    };
    
    // Sample loading options
    struct SampleLoadingOptions {
        SlotAllocationStrategy strategy;    // Slot allocation strategy
        bool enableFormatConversion;        // Convert sample format if needed
        bool enableAutoTrim;                // Trim silence from start/end
        bool enableLoopDetection;           // Detect and set loop points
        bool enableNormalization;           // Normalize sample level
        float targetLevel;                  // Target level for normalization (-dBFS)
        bool preserveOriginal;              // Keep backup of original sample
        bool enableAutoNaming;              // Generate automatic sample names
        std::string nameTemplate;           // Template for auto-generated names
        uint8_t preferredSlot;             // Preferred slot (255 = auto)
        
        SampleLoadingOptions() :
            strategy(SlotAllocationStrategy::NEXT_AVAILABLE),
            enableFormatConversion(true),
            enableAutoTrim(true),
            enableLoopDetection(false),
            enableNormalization(true),
            targetLevel(-12.0f),
            preserveOriginal(false),
            enableAutoNaming(true),
            nameTemplate("Sample_{slot}_{timestamp}"),
            preferredSlot(255) {}
    };
    
    // Sample metadata
    struct SampleMetadata {
        std::string name;               // Sample name
        std::string sourcePath;         // Source file path (if applicable)
        std::string sourceDescription;  // Description of source (e.g., "Tape Squash T2-5")
        uint32_t creationTime;         // Creation timestamp
        RealtimeAudioBouncer::AudioFormat format;  // Audio format
        uint32_t sampleCount;          // Number of samples per channel
        float peakLevel;               // Peak level in sample
        float rmsLevel;                // RMS level in sample
        bool hasLoopPoints;            // Whether sample has loop points
        uint32_t loopStart;            // Loop start point (samples)
        uint32_t loopEnd;              // Loop end point (samples)
        std::vector<std::string> tags; // Searchable tags
        
        SampleMetadata() :
            creationTime(0),
            sampleCount(0),
            peakLevel(-96.0f),
            rmsLevel(-96.0f),
            hasLoopPoints(false),
            loopStart(0),
            loopEnd(0) {}
    };
    
    // Sampler slot information
    struct SamplerSlot {
        uint8_t slotId;                // Slot ID (0-15)
        bool isOccupied;               // Whether slot contains a sample
        std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData;  // Sample audio data
        SampleMetadata metadata;       // Sample metadata
        uint32_t lastAccessTime;       // Last time sample was played
        uint32_t loadTime;            // Time when sample was loaded
        size_t memoryUsage;           // Memory used by this sample
        bool isProtected;             // Whether sample is protected from overwriting
        
        SamplerSlot() :
            slotId(255),
            isOccupied(false),
            lastAccessTime(0),
            loadTime(0),
            memoryUsage(0),
            isProtected(false) {}
    };
    
    // Sample loading result
    struct LoadingResult {
        bool success;                   // Whether loading was successful
        uint8_t assignedSlot;          // Slot where sample was loaded
        std::string sampleName;        // Final sample name
        size_t memoryUsed;             // Memory used by loaded sample
        std::string errorMessage;      // Error message if loading failed
        bool replacedExistingSample;   // Whether an existing sample was replaced
        SampleMetadata replacedSampleMetadata;  // Metadata of replaced sample
        
        LoadingResult() :
            success(false),
            assignedSlot(255),
            memoryUsed(0),
            replacedExistingSample(false) {}
    };
    
    AutoSampleLoader();
    ~AutoSampleLoader() = default;
    
    // Configuration
    void setSampleLoadingOptions(const SampleLoadingOptions& options);
    const SampleLoadingOptions& getSampleLoadingOptions() const { return loadingOptions_; }
    
    // Sample Loading
    LoadingResult loadSample(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData,
                            const std::string& sourceName = "");
    LoadingResult loadSampleToSlot(uint8_t slot, 
                                  std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData,
                                  const std::string& sourceName = "");
    
    // Slot Management
    uint8_t findNextAvailableSlot() const;
    uint8_t findOptimalSlot(const SampleLoadingOptions& options) const;
    bool isSlotAvailable(uint8_t slot) const;
    bool isSlotProtected(uint8_t slot) const;
    void setSlotProtected(uint8_t slot, bool isProtected);
    
    // Sample Information
    const SamplerSlot& getSlot(uint8_t slot) const;
    std::vector<uint8_t> getOccupiedSlots() const;
    std::vector<uint8_t> getAvailableSlots() const;
    size_t getTotalMemoryUsage() const;
    size_t getAvailableMemory() const;
    
    // Sample Management
    bool removeSample(uint8_t slot);
    bool moveSample(uint8_t fromSlot, uint8_t toSlot);
    void clearAllSamples();
    void clearUnprotectedSamples();
    
    // Sample Access Tracking
    void notifySlotAccessed(uint8_t slot);
    uint32_t getSlotLastAccessTime(uint8_t slot) const;
    std::vector<uint8_t> getSlotsByLastAccess() const;  // Sorted by access time
    
    // Sample Processing
    std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> processSample(
        std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> input,
        const SampleLoadingOptions& options) const;
    bool trimSampleSilence(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audio) const;
    bool normalizeSample(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audio, float targetLevel) const;
    bool detectLoopPoints(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audio, 
                         uint32_t& loopStart, uint32_t& loopEnd) const;
    
    // Metadata Management
    SampleMetadata generateMetadata(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData,
                                   const std::string& sourceName, const SampleLoadingOptions& options) const;
    void updateSlotMetadata(uint8_t slot, const SampleMetadata& metadata);
    std::vector<uint8_t> findSamplesByTag(const std::string& tag) const;
    std::vector<uint8_t> findSamplesByName(const std::string& namePattern) const;
    
    // Integration
    void integrateWithSequencer(class SequencerEngine* sequencer);
    void integrateWithTapeSquashing(class TapeSquashingUI* tapeSquashing);
    
    // Callbacks
    using LoadingCompleteCallback = std::function<void(const LoadingResult&)>;
    using SlotOverwriteCallback = std::function<bool(uint8_t slot, const SampleMetadata& existingMetadata)>;
    using MemoryWarningCallback = std::function<void(size_t usedMemory, size_t totalMemory)>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    
    void setLoadingCompleteCallback(LoadingCompleteCallback callback);
    void setSlotOverwriteCallback(SlotOverwriteCallback callback);
    void setMemoryWarningCallback(MemoryWarningCallback callback);
    void setErrorCallback(ErrorCallback callback);
    
    // Memory Management
    void optimizeMemoryUsage();
    bool hasEnoughMemoryForSample(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData) const;
    void setMemoryWarningThreshold(float threshold);  // 0.0-1.0 (percentage of total memory)
    
private:
    // Slot storage
    static constexpr uint8_t MAX_SLOTS = 16;
    std::array<SamplerSlot, MAX_SLOTS> slots_;
    mutable std::mutex slotsMutex_;
    
    // Configuration
    SampleLoadingOptions loadingOptions_;
    
    // Memory tracking
    size_t totalMemoryLimit_;
    std::atomic<size_t> currentMemoryUsage_;
    float memoryWarningThreshold_;
    
    // Integration
    class SequencerEngine* sequencer_;
    class TapeSquashingUI* tapeSquashing_;
    
    // Callbacks
    LoadingCompleteCallback loadingCompleteCallback_;
    SlotOverwriteCallback slotOverwriteCallback_;
    MemoryWarningCallback memoryWarningCallback_;
    ErrorCallback errorCallback_;
    
    // Default slot for failed lookups
    static SamplerSlot defaultSlot_;
    
    // Internal methods
    uint8_t selectSlotByStrategy(const SampleLoadingOptions& options) const;
    uint8_t findNextAvailableSlotInternal() const;
    uint8_t findLeastRecentlyUsedSlot() const;
    uint8_t findBestSlotByPriority(const SampleLoadingOptions& options) const;
    
    // Sample processing helpers
    bool convertSampleFormat(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audio,
                            const RealtimeAudioBouncer::AudioFormat& targetFormat) const;
    float calculateSampleRMS(const std::vector<float>& audioData) const;
    float findSamplePeak(const std::vector<float>& audioData) const;
    
    // Naming helpers
    std::string generateSampleName(uint8_t slot, const std::string& sourceName, 
                                  const SampleLoadingOptions& options) const;
    std::string expandNameTemplate(const std::string& nameTemplate, uint8_t slot, 
                                  const std::string& sourceName) const;
    
    // Memory calculation
    size_t calculateSampleMemoryUsage(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData) const;
    void updateMemoryUsage();
    
    // Validation helpers
    bool validateSlot(uint8_t slot) const;
    bool validateSampleData(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData) const;
    
    // Error handling and notification
    void notifyLoadingComplete(const LoadingResult& result);
    void notifySlotOverwrite(uint8_t slot, const SampleMetadata& metadata);
    void notifyMemoryWarning();
    void notifyError(const std::string& error);
    
    // Utility methods
    uint32_t getCurrentTimeMs() const;
    float linearToDb(float linear) const;
    float dbToLinear(float db) const;
    
    // Constants
    static constexpr size_t DEFAULT_MEMORY_LIMIT = 64 * 1024 * 1024;  // 64MB
    static constexpr float DEFAULT_SILENCE_THRESHOLD = -60.0f;        // -60dB silence threshold
    static constexpr float DEFAULT_MEMORY_WARNING_THRESHOLD = 0.8f;   // 80% memory warning
    static constexpr uint32_t LOOP_DETECTION_MIN_SAMPLES = 1000;      // Minimum samples for loop
};