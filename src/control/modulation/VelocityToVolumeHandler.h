#pragma once
#include <cstdint>
#include <functional>

/**
 * VelocityToVolumeHandler - Special case velocity→volume handling with disable option
 * 
 * Provides dedicated handling for velocity-to-volume modulation with:
 * - Configurable velocity curves (linear, exponential, logarithmic)
 * - Per-voice velocity-to-volume scaling with disable option
 * - Integration with existing velocity modulation system
 * - Hardware-optimized volume calculation for real-time performance
 * - Volume compensation when velocity→volume is disabled
 * 
 * Features:
 * - Global velocity→volume enable/disable
 * - Per-voice velocity→volume override
 * - Multiple velocity curve types for musical expression
 * - Volume scaling and offset controls
 * - Integration with voice manager for per-note volume control
 * - Smooth transitions when enabling/disabling velocity→volume
 */
class VelocityToVolumeHandler {
public:
    // Velocity-to-volume curve types
    enum class VelocityCurve {
        LINEAR,         // Direct linear mapping (velocity = volume)
        EXPONENTIAL,    // Exponential curve for perceived loudness
        LOGARITHMIC,    // Logarithmic curve for gentle response
        S_CURVE,        // S-curve for balanced response
        CUSTOM          // User-defined curve points
    };
    
    // Velocity-to-volume configuration
    struct VelocityVolumeConfig {
        bool enabled;               // Global velocity→volume enable
        VelocityCurve curve;        // Velocity curve type
        float scale;                // Volume scale factor (0.1-2.0)
        float offset;               // Volume offset (-1.0 to +1.0)
        float minVolume;            // Minimum volume level (0.0-1.0)
        float maxVolume;            // Maximum volume level (0.0-1.0)
        bool compensateWhenDisabled; // Boost volume when vel→vol disabled
        float compensationAmount;   // Compensation boost amount
        
        VelocityVolumeConfig() :
            enabled(true),
            curve(VelocityCurve::EXPONENTIAL),
            scale(1.0f),
            offset(0.0f),
            minVolume(0.0f),
            maxVolume(1.0f),
            compensateWhenDisabled(true),
            compensationAmount(0.2f) {}
    };
    
    // Per-voice velocity-to-volume override
    struct VoiceVolumeOverride {
        bool hasOverride;           // Whether this voice has custom settings
        bool enabledOverride;       // Override for enabled state
        float scaleOverride;        // Override for scale factor
        VelocityCurve curveOverride; // Override for curve type
        
        VoiceVolumeOverride() :
            hasOverride(false),
            enabledOverride(true),
            scaleOverride(1.0f),
            curveOverride(VelocityCurve::EXPONENTIAL) {}
    };
    
    VelocityToVolumeHandler();
    ~VelocityToVolumeHandler() = default;
    
    // Global configuration
    void setGlobalConfig(const VelocityVolumeConfig& config);
    const VelocityVolumeConfig& getGlobalConfig() const { return globalConfig_; }
    void setEnabled(bool enabled);
    bool isEnabled() const { return globalConfig_.enabled; }
    
    // Velocity curve configuration
    void setVelocityCurve(VelocityCurve curve);
    VelocityCurve getVelocityCurve() const { return globalConfig_.curve; }
    void setVelocityScale(float scale);
    float getVelocityScale() const { return globalConfig_.scale; }
    void setVolumeRange(float minVolume, float maxVolume);
    
    // Per-voice overrides
    void setVoiceOverride(uint32_t voiceId, const VoiceVolumeOverride& override);
    void removeVoiceOverride(uint32_t voiceId);
    bool hasVoiceOverride(uint32_t voiceId) const;
    const VoiceVolumeOverride& getVoiceOverride(uint32_t voiceId) const;
    
    // Volume calculation
    float calculateVolumeFromVelocity(float velocity, uint32_t voiceId = UINT32_MAX);
    float applyVelocityCurve(float velocity, VelocityCurve curve);
    float getCompensatedVolume(float baseVolume);
    
    // Integration with velocity modulation system
    void integrateWithVelocitySystem(class VelocityLatchSystem* latchSystem);
    void updateVolumeModulation(uint32_t voiceId, float velocity, float& volumeOutput);
    
    // Curve customization
    void setCustomCurvePoints(const std::vector<float>& curvePoints);
    std::vector<float> getCustomCurvePoints() const { return customCurvePoints_; }
    
    // System management
    void reset();
    void clearAllVoiceOverrides();
    
    // Statistics and monitoring
    size_t getActiveVoiceOverrides() const { return voiceOverrides_.size(); }
    float getAverageVolumeScale() const;
    bool isVelocityToVolumeActive() const;
    
    // Callbacks for volume changes
    using VolumeChangeCallback = std::function<void(uint32_t voiceId, float velocity, float volume)>;
    void setVolumeChangeCallback(VolumeChangeCallback callback);
    
private:
    // System state
    VelocityVolumeConfig globalConfig_;
    std::unordered_map<uint32_t, VoiceVolumeOverride> voiceOverrides_;
    std::vector<float> customCurvePoints_;
    
    // Integration
    class VelocityLatchSystem* latchSystem_;
    VolumeChangeCallback volumeChangeCallback_;
    
    // Default override instance
    static VoiceVolumeOverride defaultOverride_;
    
    // Internal curve calculation methods
    float applyLinearCurve(float velocity);
    float applyExponentialCurve(float velocity);
    float applyLogarithmicCurve(float velocity);
    float applySCurve(float velocity);
    float applyCustomCurve(float velocity);
    
    // Helper methods
    float clampVolume(float volume);
    float interpolateCustomCurve(float velocity);
    void notifyVolumeChange(uint32_t voiceId, float velocity, float volume);
    
    // Constants
    static constexpr float MIN_SCALE = 0.1f;
    static constexpr float MAX_SCALE = 2.0f;
    static constexpr float MIN_OFFSET = -1.0f;
    static constexpr float MAX_OFFSET = 1.0f;
    static constexpr float DEFAULT_COMPENSATION = 0.2f;
    static constexpr size_t MAX_CUSTOM_CURVE_POINTS = 32;
};