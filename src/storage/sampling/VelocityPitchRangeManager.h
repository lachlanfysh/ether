#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include "AutoSampleLoader.h"

/**
 * VelocityPitchRangeManager - Advanced velocity and pitch range assignment for multi-samples
 * 
 * Provides comprehensive range-based sample triggering capabilities:
 * - Velocity-sensitive sample switching with smooth crossfades
 * - Pitch-based sample selection across keyboard ranges
 * - Round-robin cycling within velocity/pitch regions
 * - Advanced layering with crossfade zones and blend modes
 * - Integration with multi-sample track system for complex arrangements
 * - Hardware-optimized for STM32 H7 embedded platform
 * 
 * Features:
 * - Multi-dimensional sample mapping (velocity × pitch × round-robin)
 * - Configurable crossfade zones between velocity layers
 * - Pitch-based sample switching for realistic instrument modeling
 * - Dynamic range assignment with real-time adjustment
 * - Memory-efficient range lookup with O(log n) performance
 * - Integration with velocity modulation and tape squashing workflow
 * - Support for up to 64 samples per track with intelligent management
 * - Real-time safe range calculation and sample selection
 */
class VelocityPitchRangeManager {
public:
    // Range assignment modes
    enum class RangeMode {
        VELOCITY_ONLY,      // Sample selection based on velocity only
        PITCH_ONLY,         // Sample selection based on pitch only
        VELOCITY_PITCH,     // 2D mapping using both velocity and pitch
        ROUND_ROBIN,        // Round-robin cycling within ranges
        VELOCITY_ROUND_ROBIN, // Velocity layers with round-robin per layer
        PITCH_ROUND_ROBIN,    // Pitch zones with round-robin per zone
        FULL_MATRIX         // Full 3D matrix: velocity × pitch × round-robin
    };
    
    // Crossfade behavior between ranges
    enum class CrossfadeMode {
        NONE,               // Hard switching between samples
        LINEAR,             // Linear crossfade between adjacent ranges
        EQUAL_POWER,        // Equal-power crossfade (constant energy)
        EXPONENTIAL,        // Exponential crossfade (musical scaling)
        CUSTOM_CURVE        // User-defined crossfade curve
    };
    
    // Sample range definition
    struct SampleRange {
        uint8_t sampleSlot;             // Sample loader slot ID
        float velocityMin;              // Minimum velocity (0.0-1.0)
        float velocityMax;              // Maximum velocity (0.0-1.0)
        uint8_t pitchMin;               // Minimum MIDI note (0-127)
        uint8_t pitchMax;               // Maximum MIDI note (0-127)
        uint8_t roundRobinGroup;        // Round-robin group ID (0-15)
        uint8_t priority;               // Selection priority (0-255)
        float gain;                     // Range-specific gain adjustment
        float pitchOffset;              // Pitch offset in semitones
        float panPosition;              // Pan position (-1.0 to 1.0)
        bool allowLayering;             // Can layer with other ranges
        CrossfadeMode crossfadeMode;    // Crossfade behavior
        float crossfadeWidth;           // Crossfade zone width (0.0-1.0)
        
        SampleRange() :
            sampleSlot(255),
            velocityMin(0.0f),
            velocityMax(1.0f),
            pitchMin(0),
            pitchMax(127),
            roundRobinGroup(0),
            priority(128),
            gain(1.0f),
            pitchOffset(0.0f),
            panPosition(0.0f),
            allowLayering(false),
            crossfadeMode(CrossfadeMode::EQUAL_POWER),
            crossfadeWidth(0.1f) {}
    };
    
    // Range selection result
    struct RangeSelectionResult {
        std::vector<uint8_t> selectedSlots;    // Selected sample slots
        std::vector<float> blendWeights;       // Blend weights for each slot
        std::vector<float> gainAdjustments;    // Gain adjustments per slot
        std::vector<float> pitchAdjustments;   // Pitch adjustments per slot
        std::vector<float> panAdjustments;     // Pan adjustments per slot
        RangeMode usedMode;                    // Range mode that was used
        bool hasRoundRobin;                    // Whether round-robin was involved
        uint8_t roundRobinPosition;            // Current round-robin position
        
        RangeSelectionResult() :
            usedMode(RangeMode::VELOCITY_ONLY),
            hasRoundRobin(false),
            roundRobinPosition(0) {}
    };
    
    // Range assignment configuration
    struct RangeConfig {
        RangeMode mode;                        // Range assignment mode
        uint8_t maxSimultaneousSlots;          // Max slots that can play together
        float globalCrossfadeTime;             // Global crossfade time (seconds)
        bool enableVelocitySmoothing;          // Smooth velocity-based transitions
        bool enablePitchSmoothing;             // Smooth pitch-based transitions
        float velocitySmoothingTime;           // Velocity smoothing time (seconds)
        float pitchSmoothingTime;              // Pitch smoothing time (seconds)
        bool enableDynamicRangeAdjustment;     // Allow real-time range editing
        uint8_t roundRobinResetNote;           // MIDI note that resets round-robin
        
        RangeConfig() :
            mode(RangeMode::VELOCITY_PITCH),
            maxSimultaneousSlots(4),
            globalCrossfadeTime(0.05f),
            enableVelocitySmoothing(true),
            enablePitchSmoothing(false),
            velocitySmoothingTime(0.02f),
            pitchSmoothingTime(0.1f),
            enableDynamicRangeAdjustment(true),
            roundRobinResetNote(36) {}  // C2
    };
    
    VelocityPitchRangeManager();
    ~VelocityPitchRangeManager() = default;
    
    // Configuration
    void setRangeConfig(const RangeConfig& config);
    const RangeConfig& getRangeConfig() const { return config_; }
    void setRangeMode(RangeMode mode);
    
    // Range Management
    bool addSampleRange(const SampleRange& range);
    bool removeSampleRange(uint8_t sampleSlot);
    bool updateSampleRange(uint8_t sampleSlot, const SampleRange& range);
    void clearAllRanges();
    
    // Range Information
    std::vector<SampleRange> getAllRanges() const;
    SampleRange getSampleRange(uint8_t sampleSlot) const;
    bool hasSampleRange(uint8_t sampleSlot) const;
    uint8_t getRangeCount() const;
    
    // Sample Selection
    RangeSelectionResult selectSamples(float velocity, uint8_t midiNote, uint8_t channel = 0);
    RangeSelectionResult selectSamplesWithContext(float velocity, uint8_t midiNote, 
                                                  uint8_t channel, const std::string& context);
    
    // Range Analysis
    std::vector<uint8_t> getSamplesInVelocityRange(float velocityMin, float velocityMax) const;
    std::vector<uint8_t> getSamplesInPitchRange(uint8_t pitchMin, uint8_t pitchMax) const;
    std::vector<uint8_t> getOverlappingSamples(float velocity, uint8_t midiNote) const;
    
    // Auto-Assignment
    void autoAssignVelocityRanges(const std::vector<uint8_t>& sampleSlots, 
                                  uint8_t layerCount = 4);
    void autoAssignPitchRanges(const std::vector<uint8_t>& sampleSlots, 
                               uint8_t keyMin = 36, uint8_t keyMax = 96);
    void autoAssignMatrix(const std::vector<uint8_t>& sampleSlots, 
                          uint8_t velocityLayers = 3, uint8_t pitchZones = 4);
    
    // Range Optimization
    void optimizeRangeAssignments();
    void detectAndFixGaps();
    void detectAndFixOverlaps(bool allowControlledOverlaps = true);
    void normalizeRangeWeights();
    
    // Round-Robin Management
    void resetRoundRobin();
    void resetRoundRobinForGroup(uint8_t group);
    void advanceRoundRobin(uint8_t group);
    uint8_t getCurrentRoundRobinPosition(uint8_t group) const;
    
    // Crossfade Management
    void setCrossfadeMode(CrossfadeMode mode);
    void setCrossfadeWidth(float width);
    void setCustomCrossfadeCurve(const std::vector<float>& curve);
    float calculateCrossfadeWeight(float position, float rangeMin, float rangeMax, 
                                   CrossfadeMode mode, float width) const;
    
    // Real-time Parameter Updates
    void updateVelocity(float velocity, bool smoothTransition = true);
    void updatePitch(uint8_t midiNote, bool smoothTransition = false);
    void updateGlobalGain(float gain);
    void updateGlobalPitch(float semitones);
    
    // Integration
    void integrateWithAutoSampleLoader(AutoSampleLoader* sampleLoader);
    void setSampleAccessCallback(std::function<const AutoSampleLoader::SamplerSlot&(uint8_t)> callback);
    void integrateWithMultiSampleTrack(class MultiSampleTrack* track);
    
    // Callbacks
    using RangeSelectedCallback = std::function<void(const RangeSelectionResult&)>;
    using RangeUpdatedCallback = std::function<void(uint8_t sampleSlot, const SampleRange&)>;
    using RoundRobinAdvancedCallback = std::function<void(uint8_t group, uint8_t position)>;
    
    void setRangeSelectedCallback(RangeSelectedCallback callback);
    void setRangeUpdatedCallback(RangeUpdatedCallback callback);
    void setRoundRobinAdvancedCallback(RoundRobinAdvancedCallback callback);
    
    // Preset Management
    struct RangePreset {
        std::string name;
        std::vector<SampleRange> ranges;
        RangeConfig config;
    };
    
    bool saveRangePreset(const std::string& name);
    bool loadRangePreset(const std::string& name);
    std::vector<std::string> getAvailablePresets() const;
    bool deleteRangePreset(const std::string& name);
    
    // Performance Analysis
    size_t getEstimatedMemoryUsage() const;
    float getAverageSelectionTime() const;
    void resetPerformanceCounters();
    
private:
    // Configuration
    RangeConfig config_;
    
    // Range storage
    std::map<uint8_t, SampleRange> sampleRanges_;  // sampleSlot -> range
    std::vector<std::vector<float>> customCrossfadeCurves_;
    
    // Round-robin state
    std::map<uint8_t, uint8_t> roundRobinPositions_;  // group -> position
    std::map<uint8_t, std::vector<uint8_t>> roundRobinGroups_;  // group -> sample slots
    
    // Current state
    float lastVelocity_;
    uint8_t lastMidiNote_;
    uint32_t lastSelectionTime_;
    
    // Integration
    AutoSampleLoader* sampleLoader_;
    std::function<const AutoSampleLoader::SamplerSlot&(uint8_t)> sampleAccessCallback_;
    class MultiSampleTrack* multiTrack_;
    
    // Callbacks
    RangeSelectedCallback rangeSelectedCallback_;
    RangeUpdatedCallback rangeUpdatedCallback_;
    RoundRobinAdvancedCallback roundRobinAdvancedCallback_;
    
    // Preset storage
    std::map<std::string, RangePreset> presets_;
    
    // Performance tracking
    uint32_t selectionCount_;
    uint32_t totalSelectionTime_;
    
    // Internal methods
    std::vector<uint8_t> findCandidateRanges(float velocity, uint8_t midiNote) const;
    std::vector<uint8_t> filterByPriority(const std::vector<uint8_t>& candidates) const;
    std::vector<uint8_t> applyRoundRobinSelection(const std::vector<uint8_t>& candidates, uint8_t group);
    
    // Range calculation helpers
    bool isVelocityInRange(float velocity, const SampleRange& range) const;
    bool isPitchInRange(uint8_t midiNote, const SampleRange& range) const;
    float calculateRangeWeight(float velocity, uint8_t midiNote, const SampleRange& range) const;
    
    // Crossfade calculation
    float calculateLinearCrossfade(float position, float rangeMin, float rangeMax, float width) const;
    float calculateEqualPowerCrossfade(float position, float rangeMin, float rangeMax, float width) const;
    float calculateExponentialCrossfade(float position, float rangeMin, float rangeMax, float width) const;
    float calculateCustomCrossfade(float position, float rangeMin, float rangeMax, 
                                   const std::vector<float>& curve) const;
    
    // Auto-assignment helpers
    void distributeVelocityRanges(const std::vector<uint8_t>& slots, uint8_t layerCount);
    void distributePitchRanges(const std::vector<uint8_t>& slots, uint8_t keyMin, uint8_t keyMax);
    void createMatrixAssignment(const std::vector<uint8_t>& slots, 
                               uint8_t velocityLayers, uint8_t pitchZones);
    
    // Optimization helpers
    void fillVelocityGaps();
    void fillPitchGaps();
    void resolveOverlaps();
    void adjustRangeWeights();
    
    // Round-robin helpers
    void updateRoundRobinGroups();
    uint8_t selectRoundRobinSample(uint8_t group);
    void validateRoundRobinGroups();
    
    // Validation helpers
    bool validateSampleRange(const SampleRange& range) const;
    bool validateRangeConfig(const RangeConfig& config) const;
    void sanitizeRangeValues(SampleRange& range) const;
    
    // Performance helpers
    uint32_t getCurrentTimeMs() const;
    void updatePerformanceCounters(uint32_t selectionTime);
    
    // Notification helpers
    void notifyRangeSelected(const RangeSelectionResult& result);
    void notifyRangeUpdated(uint8_t sampleSlot, const SampleRange& range);
    void notifyRoundRobinAdvanced(uint8_t group, uint8_t position);
    
    // Constants
    static constexpr uint8_t MAX_SAMPLE_SLOTS = 64;
    static constexpr uint8_t MAX_ROUND_ROBIN_GROUPS = 16;
    static constexpr uint8_t MAX_ROUND_ROBIN_SIZE = 16;
    static constexpr float MIN_CROSSFADE_WIDTH = 0.001f;
    static constexpr float MAX_CROSSFADE_WIDTH = 0.5f;
    static constexpr uint32_t PERFORMANCE_HISTORY_SIZE = 1000;
};