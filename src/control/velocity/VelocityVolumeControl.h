#pragma once
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

/**
 * VelocityVolumeControl - Special case velocity→volume handling with disable option
 * 
 * Provides specialized velocity-to-volume control that can be enabled or disabled:
 * - Direct velocity-to-volume mapping using various curve types
 * - Per-voice velocity volume scaling with real-time responsiveness
 * - Global enable/disable for velocity→volume (allowing pure synthesis control)
 * - Integration with synthesizer volume envelopes and gain staging
 * - Performance-optimized real-time volume calculations
 * - Velocity curve shaping specifically tailored for musical expression
 * 
 * Key Features:
 * - Traditional MIDI velocity→volume behavior when enabled
 * - Complete velocity→volume bypass when disabled (for advanced synthesis)
 * - Multiple velocity response curves (linear, exponential, S-curve, etc.)
 * - Per-engine velocity→volume scaling and offset controls
 * - Integration with voice management and polyphonic processing
 * - Real-time velocity curve modification during performance
 * - Separate velocity→volume and velocity→modulation processing
 */
class VelocityVolumeControl {
public:
    // Velocity-to-volume curve types
    enum class VolumeCurveType {
        LINEAR,             // Linear velocity to volume (default MIDI behavior)
        EXPONENTIAL,        // Exponential curve (more dynamic range)
        LOGARITHMIC,        // Logarithmic curve (compressed dynamics)
        S_CURVE,           // S-shaped curve (gentle at extremes)
        POWER_LAW,         // Power law curve (configurable exponent)
        STEPPED,           // Stepped/quantized velocity levels
        CUSTOM_TABLE       // User-defined lookup table
    };
    
    // Velocity processing modes
    enum class VelocityProcessingMode {
        DIRECT,            // Direct velocity value processing
        SMOOTHED,          // Smoothed velocity transitions
        SCALED,            // Pre-scaled velocity input
        ENVELOPE_DRIVEN    // Velocity drives volume envelope
    };
    
    // Volume calculation configuration
    struct VolumeConfig {
        bool enableVelocityToVolume;        // Master enable for velocity→volume
        VolumeCurveType curveType;           // Velocity curve type
        float curveAmount;                   // Curve shaping parameter (0.1-10.0)
        float velocityScale;                 // Input velocity scaling (0.0-2.0)
        float velocityOffset;                // Input velocity offset (-1.0-1.0)
        float volumeMin;                     // Minimum output volume (0.0-1.0)
        float volumeMax;                     // Maximum output volume (0.0-1.0)
        float volumeRange;                   // Dynamic range compression (0.0-1.0)
        VelocityProcessingMode processingMode;
        float smoothingTime;                 // Velocity smoothing time (ms)
        bool invertVelocity;                // Invert velocity response
        
        VolumeConfig() :
            enableVelocityToVolume(true),
            curveType(VolumeCurveType::LINEAR),
            curveAmount(1.0f),
            velocityScale(1.0f),
            velocityOffset(0.0f),
            volumeMin(0.0f),
            volumeMax(1.0f),
            volumeRange(1.0f),
            processingMode(VelocityProcessingMode::DIRECT),
            smoothingTime(0.0f),
            invertVelocity(false) {}
    };
    
    // Per-voice volume state
    struct VoiceVolumeState {
        uint32_t voiceId;
        uint8_t originalVelocity;           // Original MIDI velocity
        float processedVelocity;            // Velocity after curve processing
        float calculatedVolume;             // Final calculated volume
        float smoothedVolume;               // Smoothed volume for transitions
        bool volumeOverridden;              // Volume manually overridden
        float overrideVolume;               // Manual volume override value
        uint64_t lastUpdateTime;            // Last update timestamp
        
        VoiceVolumeState() :
            voiceId(0), originalVelocity(64), processedVelocity(0.5f),
            calculatedVolume(0.5f), smoothedVolume(0.5f),
            volumeOverridden(false), overrideVolume(1.0f), lastUpdateTime(0) {}
    };
    
    // Volume calculation result
    struct VolumeResult {
        float volume;                       // Final volume value (0.0-1.0)
        float velocityComponent;            // Velocity contribution to volume
        float baseComponent;                // Base/minimum volume component
        bool wasSmoothed;                   // Volume change was smoothed
        bool wasLimited;                    // Volume was limited by range settings
        VolumeCurveType appliedCurve;      // Curve type that was applied
        
        VolumeResult() :
            volume(1.0f), velocityComponent(1.0f), baseComponent(0.0f),
            wasSmoothed(false), wasLimited(false),
            appliedCurve(VolumeCurveType::LINEAR) {}
    };
    
    VelocityVolumeControl();
    ~VelocityVolumeControl() = default;
    
    // Global velocity→volume control
    void setGlobalVelocityToVolumeEnabled(bool enabled);
    bool isGlobalVelocityToVolumeEnabled() const { return globalConfig_.enableVelocityToVolume; }
    void setGlobalVolumeConfig(const VolumeConfig& config);
    const VolumeConfig& getGlobalVolumeConfig() const { return globalConfig_; }
    
    // Per-engine configuration
    void setEngineVolumeConfig(uint32_t engineId, const VolumeConfig& config);
    const VolumeConfig& getEngineVolumeConfig(uint32_t engineId) const;
    bool hasEngineVolumeConfig(uint32_t engineId) const;
    void removeEngineVolumeConfig(uint32_t engineId);
    
    // Volume calculation
    VolumeResult calculateVolume(uint32_t voiceId, uint8_t velocity, uint32_t engineId = 0);
    float calculateDirectVolume(uint8_t velocity, const VolumeConfig& config) const;
    float calculateSmoothedVolume(uint32_t voiceId, float targetVolume, float deltaTime);
    
    // Voice management
    void addVoice(uint32_t voiceId, uint8_t velocity, uint32_t engineId = 0);
    void updateVoiceVelocity(uint32_t voiceId, uint8_t newVelocity);
    void removeVoice(uint32_t voiceId);
    void clearAllVoices();
    
    // Voice volume overrides
    void setVoiceVolumeOverride(uint32_t voiceId, float volume);
    void clearVoiceVolumeOverride(uint32_t voiceId);
    bool hasVoiceVolumeOverride(uint32_t voiceId) const;
    float getVoiceVolume(uint32_t voiceId) const;
    
    // Velocity curve processing
    float applyCurve(float velocity, VolumeCurveType curveType, float curveAmount) const;
    float applyLinearCurve(float velocity) const;
    float applyExponentialCurve(float velocity, float amount) const;
    float applyLogarithmicCurve(float velocity, float amount) const;
    float applySCurve(float velocity, float amount) const;
    float applyPowerLawCurve(float velocity, float exponent) const;
    float applySteppedCurve(float velocity, int steps) const;
    float applyCustomTableCurve(float velocity, const std::vector<float>& table) const;
    
    // Curve modification
    void setCustomCurveTable(const std::vector<float>& table);
    const std::vector<float>& getCustomCurveTable() const { return customCurveTable_; }
    void generateCurveTable(VolumeCurveType type, float amount, size_t tableSize = 128);
    
    // System management
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    void setSampleRate(float sampleRate) { sampleRate_ = sampleRate; }
    float getSampleRate() const { return sampleRate_; }
    void reset();
    
    // Performance monitoring
    void updateAllVoices(float deltaTime);
    size_t getActiveVoiceCount() const;
    float getAverageVolume() const;
    size_t getVoicesWithOverrides() const;
    
    // Batch operations
    void setAllVoicesVolume(float volume);
    void updateAllVoicesSmoothing(float deltaTime);
    void applyGlobalVolumeScale(float scale);
    void resetAllVoicesToVelocityVolume();
    
    // Integration callbacks
    using VolumeChangeCallback = std::function<void(uint32_t voiceId, float oldVolume, float newVolume)>;
    void setVolumeChangeCallback(VolumeChangeCallback callback);
    
    // Debugging and analysis
    std::vector<uint32_t> getActiveVoiceIds() const;
    VoiceVolumeState getVoiceState(uint32_t voiceId) const;
    void dumpVoiceStates() const;
    
private:
    // System state
    bool enabled_;
    float sampleRate_;
    VolumeConfig globalConfig_;
    std::unordered_map<uint32_t, VolumeConfig> engineConfigs_;
    
    // Voice tracking
    std::unordered_map<uint32_t, VoiceVolumeState> voiceStates_;
    
    // Custom curve table
    std::vector<float> customCurveTable_;
    
    // Callbacks
    VolumeChangeCallback volumeChangeCallback_;
    
    // Internal methods
    float normalizeVelocity(uint8_t velocity) const;
    float scaleAndOffsetVelocity(float velocity, const VolumeConfig& config) const;
    float applyVolumeRange(float volume, const VolumeConfig& config) const;
    void notifyVolumeChange(uint32_t voiceId, float oldVolume, float newVolume);
    float interpolateTableValue(float index, const std::vector<float>& table) const;
    const VolumeConfig& getEffectiveConfig(uint32_t engineId) const;
    
    // Constants
    static constexpr float MIN_VELOCITY = 0.0f;
    static constexpr float MAX_VELOCITY = 1.0f;
    static constexpr float MIN_VOLUME = 0.0f;
    static constexpr float MAX_VOLUME = 1.0f;
    static constexpr float DEFAULT_SAMPLE_RATE = 48000.0f;
    static constexpr float MIN_CURVE_AMOUNT = 0.1f;
    static constexpr float MAX_CURVE_AMOUNT = 10.0f;
    static constexpr float MIN_SMOOTHING_TIME = 0.0f;
    static constexpr float MAX_SMOOTHING_TIME = 1000.0f; // 1 second max
    static constexpr size_t DEFAULT_CURVE_TABLE_SIZE = 128;
};