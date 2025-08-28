#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include "VelocityPitchRangeManager.h"
#include "../sequencer/PatternSelection.h"

/**
 * SampleLayeringSystem - Advanced sample layering and independent sequencing
 * 
 * Provides comprehensive multi-sample layering capabilities:
 * - Independent sequencing patterns for layered samples
 * - Advanced layering modes with crossfades and blend controls
 * - Per-layer parameter automation and modulation
 * - Dynamic layer activation based on velocity, pitch, and time
 * - Integration with velocity/pitch range system for complex arrangements
 * - Hardware-optimized for STM32 H7 embedded platform
 * 
 * Features:
 * - Up to 16 independent sample layers per track
 * - Per-layer sequencing with independent pattern lengths
 * - Advanced blend modes (additive, multiplicative, spectral)
 * - Dynamic layer activation with envelope control
 * - Layer grouping and bus routing for complex arrangements
 * - Real-time layer parameter control with smoothing
 * - Integration with tape squashing for dynamic layer creation
 * - Memory-efficient layer management with voice pooling
 */
class SampleLayeringSystem {
public:
    // Layer activation modes
    enum class LayerActivationMode {
        ALWAYS_ACTIVE,      // Layer always plays when triggered
        VELOCITY_GATED,     // Activate based on velocity threshold
        PITCH_GATED,        // Activate based on pitch range
        PROBABILITY,        // Random activation with probability
        STEP_SEQUENCED,     // Activate based on independent step sequence
        ENVELOPE_GATED,     // Activate based on envelope state
        MODULATION_GATED,   // Activate based on modulation source
        CONDITIONAL        // Activate based on complex conditions
    };
    
    // Layer blend modes
    enum class LayerBlendMode {
        ADDITIVE,           // Simple addition of layers
        MULTIPLICATIVE,     // Multiply layers together
        CROSSFADE,          // Linear crossfade between layers
        EQUAL_POWER,        // Equal-power mixing
        SPECTRAL_BLEND,     // Frequency-domain blending
        DYNAMIC_MIX,        // Dynamic mixing based on content
        SIDE_CHAIN,         // One layer gates/modulates another
        PARALLEL_COMPRESS   // Parallel compression mixing
    };
    
    // Layer sequencing mode
    enum class LayerSequencingMode {
        SHARED_PATTERN,     // All layers share same pattern
        INDEPENDENT_STEPS,  // Each layer has independent step pattern
        OFFSET_PATTERN,     // Layers offset from main pattern
        POLYRHYTHM,         // Different pattern lengths per layer
        EUCLIDEAN,          // Euclidean rhythm generation per layer
        PROBABILITY_STEPS,  // Probabilistic step activation
        CONDITIONAL_STEPS   // Complex conditional step logic
    };
    
    // Sample layer definition
    struct SampleLayer {
        uint8_t layerId;                    // Unique layer identifier
        uint8_t sampleSlot;                 // Sample loader slot
        LayerActivationMode activationMode; // How layer is activated
        LayerBlendMode blendMode;           // How layer blends with others
        LayerSequencingMode sequencingMode; // Independent sequencing behavior
        
        // Activation parameters
        float velocityThreshold;            // Velocity threshold for gating
        float velocityMax;                  // Maximum velocity for layer
        uint8_t pitchMin;                   // Minimum pitch for activation
        uint8_t pitchMax;                   // Maximum pitch for activation
        float probability;                  // Activation probability (0.0-1.0)
        
        // Blend parameters
        float layerGain;                    // Layer gain (-∞ to +12dB)
        float layerPan;                     // Layer pan position (-1.0 to 1.0)
        float blendAmount;                  // Blend strength (0.0-1.0)
        float crossfadePosition;            // Crossfade position for blend modes
        
        // Sequencing parameters
        std::vector<bool> stepPattern;      // Independent step pattern
        uint8_t patternLength;              // Length of independent pattern
        int8_t patternOffset;               // Offset from main pattern (-16 to +16)
        uint8_t euclideanSteps;             // Number of steps for Euclidean rhythm
        uint8_t euclideanHits;              // Number of hits in Euclidean pattern
        uint8_t euclideanRotation;          // Rotation offset for Euclidean pattern
        
        // Timing parameters
        float layerDelay;                   // Layer delay in milliseconds (0-100ms)
        float microTiming;                  // Micro-timing adjustment (-50% to +50%)
        bool reversePlayback;               // Play layer in reverse
        float playbackRate;                 // Playback rate multiplier (0.5-2.0)
        
        // Modulation parameters
        float pitchModulation;              // Pitch modulation depth (semitones)
        float gainModulation;               // Gain modulation depth (dB)
        float panModulation;                // Pan modulation depth
        float filterModulation;             // Filter modulation depth
        
        // State
        bool isActive;                      // Current activation state
        uint8_t currentStep;                // Current step in independent pattern
        float currentGain;                  // Current smoothed gain
        float targetGain;                   // Target gain for smoothing
        uint32_t activationTime;            // Time when layer was activated
        
        SampleLayer() :
            layerId(255),
            sampleSlot(255),
            activationMode(LayerActivationMode::ALWAYS_ACTIVE),
            blendMode(LayerBlendMode::ADDITIVE),
            sequencingMode(LayerSequencingMode::SHARED_PATTERN),
            velocityThreshold(0.0f),
            velocityMax(1.0f),
            pitchMin(0),
            pitchMax(127),
            probability(1.0f),
            layerGain(1.0f),
            layerPan(0.0f),
            blendAmount(1.0f),
            crossfadePosition(0.5f),
            patternLength(16),
            patternOffset(0),
            euclideanSteps(16),
            euclideanHits(8),
            euclideanRotation(0),
            layerDelay(0.0f),
            microTiming(0.0f),
            reversePlayback(false),
            playbackRate(1.0f),
            pitchModulation(0.0f),
            gainModulation(0.0f),
            panModulation(0.0f),
            filterModulation(0.0f),
            isActive(false),
            currentStep(0),
            currentGain(0.0f),
            targetGain(1.0f),
            activationTime(0) {
                stepPattern.resize(16, false);
            }
    };
    
    // Layer group for bus routing
    struct LayerGroup {
        uint8_t groupId;                    // Group identifier
        std::vector<uint8_t> layerIds;      // Layers in this group
        float groupGain;                    // Group master gain
        float groupPan;                     // Group master pan
        LayerBlendMode groupBlendMode;      // How group blends with others
        bool groupMute;                     // Group mute state
        bool groupSolo;                     // Group solo state
        
        LayerGroup() :
            groupId(255),
            groupGain(1.0f),
            groupPan(0.0f),
            groupBlendMode(LayerBlendMode::ADDITIVE),
            groupMute(false),
            groupSolo(false) {}
    };
    
    // Layering configuration
    struct LayeringConfig {
        uint8_t maxLayers;                  // Maximum concurrent layers
        uint8_t maxGroups;                  // Maximum layer groups
        float globalGainLimit;              // Global gain limiting (dB)
        bool enableAutoGainCompensation;    // Auto-compensate for layer count
        bool enableVoicePooling;            // Share voices between layers
        uint8_t voicePoolSize;              // Size of shared voice pool
        float parameterSmoothingTime;       // Parameter smoothing time (ms)
        bool enableLayerSolo;               // Allow layer solo functionality
        bool enableLayerMute;               // Allow layer mute functionality
        
        LayeringConfig() :
            maxLayers(8),
            maxGroups(4),
            globalGainLimit(0.0f),
            enableAutoGainCompensation(true),
            enableVoicePooling(true),
            voicePoolSize(16),
            parameterSmoothingTime(10.0f),
            enableLayerSolo(true),
            enableLayerMute(true) {}
    };
    
    // Layer activation result
    struct LayerActivationResult {
        std::vector<uint8_t> activatedLayers;  // Layers that were activated
        std::vector<float> layerGains;          // Gain for each activated layer
        std::vector<float> layerDelays;         // Delay for each activated layer
        std::vector<uint8_t> sampleSlots;       // Sample slots for each layer
        LayerBlendMode effectiveBlendMode;      // Final blend mode used
        float totalGainCompensation;            // Applied gain compensation
        
        LayerActivationResult() :
            effectiveBlendMode(LayerBlendMode::ADDITIVE),
            totalGainCompensation(1.0f) {}
    };
    
    SampleLayeringSystem();
    ~SampleLayeringSystem() = default;
    
    // Configuration
    void setLayeringConfig(const LayeringConfig& config);
    const LayeringConfig& getLayeringConfig() const { return config_; }
    
    // Layer Management
    bool addLayer(const SampleLayer& layer);
    bool removeLayer(uint8_t layerId);
    bool updateLayer(uint8_t layerId, const SampleLayer& layer);
    void clearAllLayers();
    SampleLayer getLayer(uint8_t layerId) const;
    std::vector<uint8_t> getAllLayerIds() const;
    uint8_t getLayerCount() const;
    
    // Layer Group Management
    bool createGroup(const LayerGroup& group);
    bool removeGroup(uint8_t groupId);
    bool updateGroup(uint8_t groupId, const LayerGroup& group);
    bool addLayerToGroup(uint8_t layerId, uint8_t groupId);
    bool removeLayerFromGroup(uint8_t layerId, uint8_t groupId);
    std::vector<LayerGroup> getAllGroups() const;
    
    // Layer Activation
    LayerActivationResult activateLayers(float velocity, uint8_t midiNote, uint8_t currentStep);
    void updateLayerStates(uint8_t currentStep);
    void deactivateAllLayers();
    void deactivateLayer(uint8_t layerId);
    
    // Sequencing Control
    void updateLayerSequencing(uint8_t currentStep);
    void resetLayerSequencing();
    bool isLayerActiveAtStep(uint8_t layerId, uint8_t step) const;
    void setLayerStepPattern(uint8_t layerId, const std::vector<bool>& pattern);
    std::vector<bool> getLayerStepPattern(uint8_t layerId) const;
    
    // Parameter Control
    void setLayerGain(uint8_t layerId, float gain);
    void setLayerPan(uint8_t layerId, float pan);
    void setLayerBlendAmount(uint8_t layerId, float amount);
    void setGroupGain(uint8_t groupId, float gain);
    void setGroupPan(uint8_t groupId, float pan);
    
    // Mute/Solo Control
    void muteLayer(uint8_t layerId, bool mute);
    void soloLayer(uint8_t layerId, bool solo);
    void muteGroup(uint8_t groupId, bool mute);
    void soloGroup(uint8_t groupId, bool solo);
    bool isLayerMuted(uint8_t layerId) const;
    bool isLayerSoloed(uint8_t layerId) const;
    
    // Blend Mode Control
    void setLayerBlendMode(uint8_t layerId, LayerBlendMode mode);
    void setGlobalBlendMode(LayerBlendMode mode);
    float calculateBlendWeight(LayerBlendMode mode, float blendAmount, float layerGain) const;
    
    // Audio Processing
    void processLayeredAudio(float* outputBuffer, uint32_t sampleCount, 
                            const LayerActivationResult& activationResult);
    void mixLayerToBuffer(uint8_t layerId, float* buffer, uint32_t sampleCount, 
                         float gain, float delay);
    
    // Euclidean Rhythm Generation
    std::vector<bool> generateEuclideanPattern(uint8_t steps, uint8_t hits, uint8_t rotation = 0) const;
    void setLayerEuclideanPattern(uint8_t layerId, uint8_t steps, uint8_t hits, uint8_t rotation = 0);
    
    // Pattern Analysis
    float calculatePatternDensity(uint8_t layerId) const;
    uint8_t countActiveSteps(uint8_t layerId) const;
    std::vector<uint8_t> findPatternOnsets(uint8_t layerId) const;
    
    // Integration
    void integrateWithVelocityPitchRangeManager(VelocityPitchRangeManager* rangeManager);
    void integrateWithAutoSampleLoader(AutoSampleLoader* sampleLoader);
    void setSampleAccessCallback(std::function<const AutoSampleLoader::SamplerSlot&(uint8_t)> callback);
    
    // Callbacks
    using LayerActivatedCallback = std::function<void(uint8_t layerId, float velocity)>;
    using LayerDeactivatedCallback = std::function<void(uint8_t layerId)>;
    using LayerParameterChangedCallback = std::function<void(uint8_t layerId, const std::string& param, float value)>;
    using PatternUpdatedCallback = std::function<void(uint8_t layerId, const std::vector<bool>& pattern)>;
    
    void setLayerActivatedCallback(LayerActivatedCallback callback);
    void setLayerDeactivatedCallback(LayerDeactivatedCallback callback);
    void setLayerParameterChangedCallback(LayerParameterChangedCallback callback);
    void setPatternUpdatedCallback(PatternUpdatedCallback callback);
    
    // Preset Management
    struct LayeringPreset {
        std::string name;
        std::vector<SampleLayer> layers;
        std::vector<LayerGroup> groups;
        LayeringConfig config;
    };
    
    bool saveLayeringPreset(const std::string& name);
    bool loadLayeringPreset(const std::string& name);
    std::vector<std::string> getAvailablePresets() const;
    
    // Performance Analysis
    size_t getEstimatedMemoryUsage() const;
    uint8_t getActiveLayerCount() const;
    float getCombinedProcessingLoad() const;
    
private:
    // Configuration
    LayeringConfig config_;
    
    // Layer storage
    std::map<uint8_t, SampleLayer> layers_;
    std::map<uint8_t, LayerGroup> groups_;
    uint8_t nextLayerId_;
    uint8_t nextGroupId_;
    
    // Layer state
    std::vector<uint8_t> activeLayers_;
    std::vector<uint8_t> mutedLayers_;
    std::vector<uint8_t> soloedLayers_;
    
    // Integration
    VelocityPitchRangeManager* rangeManager_;
    AutoSampleLoader* sampleLoader_;
    std::function<const AutoSampleLoader::SamplerSlot&(uint8_t)> sampleAccessCallback_;
    
    // Callbacks
    LayerActivatedCallback layerActivatedCallback_;
    LayerDeactivatedCallback layerDeactivatedCallback_;
    LayerParameterChangedCallback layerParameterChangedCallback_;
    PatternUpdatedCallback patternUpdatedCallback_;
    
    // Preset storage
    std::map<std::string, LayeringPreset> presets_;
    
    // Internal methods
    bool shouldActivateLayer(const SampleLayer& layer, float velocity, uint8_t midiNote, uint8_t currentStep) const;
    bool evaluateVelocityGating(const SampleLayer& layer, float velocity) const;
    bool evaluatePitchGating(const SampleLayer& layer, uint8_t midiNote) const;
    bool evaluateProbabilityGating(const SampleLayer& layer) const;
    bool evaluateStepSequencing(const SampleLayer& layer, uint8_t currentStep) const;
    
    // Blend calculation
    float calculateAdditiveBlend(const std::vector<float>& layerSamples) const;
    float calculateMultiplicativeBlend(const std::vector<float>& layerSamples) const;
    float calculateCrossfadeBlend(const std::vector<float>& layerSamples, float position) const;
    float calculateEqualPowerBlend(const std::vector<float>& layerSamples, const std::vector<float>& weights) const;
    
    // Sequencing helpers
    void updateIndependentSequencing(SampleLayer& layer, uint8_t currentStep);
    void updateOffsetSequencing(SampleLayer& layer, uint8_t currentStep);
    void updatePolyrhythmSequencing(SampleLayer& layer, uint8_t currentStep);
    void updateEuclideanSequencing(SampleLayer& layer, uint8_t currentStep);
    
    // Parameter smoothing
    void updateParameterSmoothing(SampleLayer& layer);
    void smoothParameter(float& current, float target, float smoothingTime);
    
    // Voice management
    uint8_t allocateLayerVoice(uint8_t layerId);
    void releaseLayerVoice(uint8_t layerId, uint8_t voiceId);
    bool hasAvailableVoices() const;
    
    // Gain compensation
    float calculateAutoGainCompensation(uint8_t activeLayerCount) const;
    void applyGlobalGainLimit(float* buffer, uint32_t sampleCount);
    
    // Mute/Solo logic
    bool isEffectivelyMuted(uint8_t layerId) const;
    void updateMuteSoloStates();
    
    // Validation helpers
    bool validateLayer(const SampleLayer& layer) const;
    bool validateGroup(const LayerGroup& group) const;
    void sanitizeLayerParameters(SampleLayer& layer) const;
    
    // Notification helpers
    void notifyLayerActivated(uint8_t layerId, float velocity);
    void notifyLayerDeactivated(uint8_t layerId);
    void notifyLayerParameterChanged(uint8_t layerId, const std::string& parameter, float value);
    void notifyPatternUpdated(uint8_t layerId, const std::vector<bool>& pattern);
    
    // Utility methods
    uint32_t getCurrentTimeMs() const;
    float generateRandomFloat() const;  // For probability calculations
    
    // Constants
    static constexpr uint8_t MAX_LAYERS = 16;
    static constexpr uint8_t MAX_GROUPS = 8;
    static constexpr uint8_t MAX_PATTERN_LENGTH = 64;
    static constexpr float MAX_LAYER_DELAY_MS = 100.0f;
    static constexpr float MIN_PLAYBACK_RATE = 0.25f;
    static constexpr float MAX_PLAYBACK_RATE = 4.0f;
};