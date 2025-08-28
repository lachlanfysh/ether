#pragma once
#include <cstdint>
#include <unordered_map>
#include <functional>
#include <vector>
#include "../../interface/IVelocityModulationView.h"

/**
 * VelocityDepthControl - Unified velocity modulation depth management (0-200%)
 * 
 * Provides centralized control over velocity modulation depth across all parameters:
 * - Unified depth range from 0% (no modulation) to 200% (double modulation)
 * - Per-parameter depth override capability
 * - Global depth scaling that affects all parameters proportionally
 * - Depth limiting and safety controls to prevent extreme modulation
 * - Real-time depth adjustment with smooth parameter transitions
 * - Integration with V-icon system for visual depth indication
 * - Preset-based depth configurations for different musical contexts
 * 
 * Features:
 * - Master depth control affecting all velocity-modulated parameters
 * - Individual parameter depth scaling with visual feedback
 * - Depth limiting and safety controls (prevents runaway modulation)
 * - Smooth depth transitions to avoid audio artifacts
 * - Integration with velocity latch and scaling systems
 * - Depth presets for different performance styles
 * - Real-time depth monitoring and adjustment
 */
class VelocityDepthControl {
public:
    // Depth control modes
    enum class DepthMode {
        ABSOLUTE,           // Direct depth value (0-200%)
        RELATIVE,           // Relative to base parameter value
        SCALED,             // Scaled by parameter's natural range
        LIMITED,            // Limited to safe ranges per parameter type
        DYNAMIC             // Dynamically adjusted based on musical context
    };
    
    // Depth safety levels
    enum class SafetyLevel {
        NONE,               // No depth limiting
        CONSERVATIVE,       // Conservative limits to prevent harsh modulation
        MODERATE,           // Moderate limits allowing expressive modulation
        AGGRESSIVE,         // Higher limits for extreme expression
        CUSTOM              // User-defined limits
    };
    
    // Per-parameter depth configuration
    struct ParameterDepthConfig {
        float baseDepth;                // Base modulation depth (0.0-2.0 = 0-200%)
        float maxAllowedDepth;          // Maximum allowed depth for this parameter
        float minAllowedDepth;          // Minimum allowed depth for this parameter
        DepthMode depthMode;            // How depth is applied to this parameter
        SafetyLevel safetyLevel;        // Safety limiting level
        bool enableDepthModulation;     // Allow real-time depth modulation
        float depthSmoothingTime;       // Smoothing time for depth changes (ms)
        bool linkToMasterDepth;         // Follow master depth changes
        float masterDepthScale;         // Scale factor for master depth (0.0-2.0)
        
        ParameterDepthConfig() :
            baseDepth(1.0f),
            maxAllowedDepth(2.0f),
            minAllowedDepth(0.0f),
            depthMode(DepthMode::ABSOLUTE),
            safetyLevel(SafetyLevel::MODERATE),
            enableDepthModulation(true),
            depthSmoothingTime(10.0f),
            linkToMasterDepth(true),
            masterDepthScale(1.0f) {}
    };
    
    // Global depth configuration
    struct GlobalDepthConfig {
        float masterDepth;              // Master depth affecting all parameters (0.0-2.0)
        SafetyLevel globalSafetyLevel;  // Global safety level
        bool enableMasterDepthControl;  // Enable master depth control
        float maxGlobalDepth;           // Maximum master depth allowed
        float depthTransitionTime;      // Time for depth transitions (ms)
        bool enableDepthLimiting;       // Enable automatic depth limiting
        float emergencyDepthLimit;      // Emergency limit to prevent damage (0.0-1.0)
        
        GlobalDepthConfig() :
            masterDepth(1.0f),
            globalSafetyLevel(SafetyLevel::MODERATE),
            enableMasterDepthControl(true),
            maxGlobalDepth(2.0f),
            depthTransitionTime(50.0f),
            enableDepthLimiting(true),
            emergencyDepthLimit(1.5f) {}
    };
    
    // Depth calculation result
    struct DepthResult {
        float requestedDepth;           // Originally requested depth
        float actualDepth;              // Actual depth after limiting/processing
        float effectiveDepth;           // Final effective depth for parameter
        bool wasLimited;                // Whether depth was limited by safety
        bool wasSmoothened;             // Whether depth change was smoothed
        SafetyLevel appliedSafetyLevel; // Safety level that was applied
        float limitingAmount;           // Amount of limiting applied (0.0-1.0)
        
        DepthResult() :
            requestedDepth(0.0f), actualDepth(0.0f), effectiveDepth(0.0f),
            wasLimited(false), wasSmoothened(false),
            appliedSafetyLevel(SafetyLevel::MODERATE), limitingAmount(0.0f) {}
    };
    
    // Depth preset for different musical contexts
    struct DepthPreset {
        std::string name;
        std::string description;
        GlobalDepthConfig globalConfig;
        std::unordered_map<uint32_t, ParameterDepthConfig> parameterConfigs;
        
        DepthPreset(const std::string& n, const std::string& d) :
            name(n), description(d) {}
    };
    
    VelocityDepthControl();
    ~VelocityDepthControl() = default;
    
    // Global depth control
    void setMasterDepth(float depth);
    float getMasterDepth() const { return globalConfig_.masterDepth; }
    void setGlobalConfig(const GlobalDepthConfig& config);
    const GlobalDepthConfig& getGlobalConfig() const { return globalConfig_; }
    
    // Per-parameter depth configuration
    void setParameterDepthConfig(uint32_t parameterId, const ParameterDepthConfig& config);
    void setParameterBaseDepth(uint32_t parameterId, float depth);
    void setParameterMaxDepth(uint32_t parameterId, float maxDepth);
    void setParameterDepthMode(uint32_t parameterId, DepthMode mode);
    void setParameterSafetyLevel(uint32_t parameterId, SafetyLevel level);
    
    const ParameterDepthConfig& getParameterDepthConfig(uint32_t parameterId) const;
    float getParameterBaseDepth(uint32_t parameterId) const;
    bool hasParameterDepthConfig(uint32_t parameterId) const;
    
    // Depth calculation and application
    DepthResult calculateEffectiveDepth(uint32_t parameterId, float requestedDepth);
    float applyDepthToModulation(uint32_t parameterId, float baseModulation, float velocity);
    float getEffectiveParameterDepth(uint32_t parameterId) const;
    
    // Safety and limiting
    float applySafetyLimiting(uint32_t parameterId, float depth, SafetyLevel level);
    bool isDepthSafe(uint32_t parameterId, float depth) const;
    float getMaxSafeDepth(uint32_t parameterId, SafetyLevel level) const;
    void emergencyDepthLimit(float maxDepth);  // Emergency limiting for all parameters
    
    // Real-time depth modulation
    void updateDepthSmoothing(float deltaTime);
    void setRealTimeDepthModulation(uint32_t parameterId, float depthModulation);
    float getRealTimeDepthModulation(uint32_t parameterId) const;
    
    // Preset management
    void addDepthPreset(const DepthPreset& preset);
    void removeDepthPreset(const std::string& presetName);
    void applyDepthPreset(const std::string& presetName);
    std::vector<DepthPreset> getAvailableDepthPresets() const;
    DepthPreset getCurrentDepthSettings() const;
    
    // Batch operations
    void setAllParametersDepth(float depth);
    void setAllParametersSafetyLevel(SafetyLevel level);
    void linkAllParametersToMaster(bool linked);
    void resetAllParametersToDefaults();
    
    // System management
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    void reset();
    void removeParameter(uint32_t parameterId);
    
    // Statistics and monitoring
    size_t getConfiguredParameterCount() const;
    float getAverageDepth() const;
    size_t getParametersOverDepth(float depthThreshold) const;
    std::vector<uint32_t> getParametersWithExcessiveDepth(float threshold = 1.5f) const;
    float getSystemDepthLoad() const;  // Estimate of total depth processing load
    
    // Integration with other velocity systems
    void integrateWithVelocityLatch(class VelocityLatchSystem* latchSystem);
    void integrateWithParameterScaling(class VelocityParameterScaling* scalingSystem);
    void integrateWithVelocityUI(std::shared_ptr<IVelocityModulationView> panel);
    
    // Callbacks for depth changes
    using DepthChangeCallback = std::function<void(uint32_t parameterId, float oldDepth, float newDepth)>;
    void setDepthChangeCallback(DepthChangeCallback callback);
    
private:
    // System state
    bool enabled_;
    GlobalDepthConfig globalConfig_;
    
    // Per-parameter configurations
    std::unordered_map<uint32_t, ParameterDepthConfig> parameterConfigs_;
    std::unordered_map<uint32_t, float> currentSmoothDepths_;
    std::unordered_map<uint32_t, float> targetDepths_;
    std::unordered_map<uint32_t, float> realTimeDepthMod_;
    
    // Depth presets
    std::vector<DepthPreset> depthPresets_;
    
    // Integration with other systems
    class VelocityLatchSystem* latchSystem_;
    class VelocityParameterScaling* scalingSystem_;
    std::shared_ptr<IVelocityModulationView> uiPanel_;
    
    // Callbacks
    DepthChangeCallback depthChangeCallback_;
    
    // Default configurations
    static ParameterDepthConfig defaultParameterConfig_;
    static GlobalDepthConfig defaultGlobalConfig_;
    
    // Internal methods
    void initializeDepthPresets();
    void updateDepthSmoothingForParameter(uint32_t parameterId, float deltaTime);
    float calculateSafetyLimit(uint32_t parameterId, SafetyLevel level) const;
    void notifyDepthChange(uint32_t parameterId, float oldDepth, float newDepth);
    float applyDepthSmoothing(float current, float target, float smoothingTime, float deltaTime);
    
    // Safety level configurations
    struct SafetyLimits {
        float conservative;  // Conservative limit (typically 0.8 = 80%)
        float moderate;      // Moderate limit (typically 1.2 = 120%)  
        float aggressive;    // Aggressive limit (typically 1.8 = 180%)
    };
    
    static std::unordered_map<uint32_t, SafetyLimits> parameterSafetyLimits_;
    
    // Constants
    static constexpr float MIN_DEPTH = 0.0f;
    static constexpr float MAX_DEPTH = 2.0f;
    static constexpr float DEFAULT_DEPTH = 1.0f;
    static constexpr float EMERGENCY_LIMIT = 1.5f;
    static constexpr float MIN_SMOOTHING_TIME = 1.0f;    // 1ms minimum
    static constexpr float MAX_SMOOTHING_TIME = 1000.0f; // 1s maximum
    static constexpr float DEFAULT_SMOOTHING_TIME = 10.0f; // 10ms default
};