#pragma once
#include <cstdint>
#include <unordered_map>
#include <functional>
#include <vector>
#include "../../interface/IVelocityModulationView.h"

/**
 * VelocityParameterScaling - Per-parameter velocity scaling and polarity configuration
 * 
 * Provides fine-grained control over how velocity affects each synthesizer parameter:
 * - Individual velocity scaling factors per parameter (0.1× to 5× sensitivity)
 * - Per-parameter polarity configuration (positive, negative, bipolar)
 * - Custom velocity range mapping (input range → output range)
 * - Parameter-specific velocity curve assignment
 * - Velocity deadzone and threshold configuration
 * - Dynamic velocity scaling based on parameter context
 * 
 * Features:
 * - Parameter categories with default scaling presets
 * - Real-time velocity scaling adjustment with visual feedback
 * - Velocity range compression/expansion per parameter
 * - Context-aware scaling (filter cutoff vs resonance have different optimal scaling)
 * - Velocity scaling presets for common parameter types
 * - Integration with existing velocity modulation systems
 */
class VelocityParameterScaling {
public:
    // Parameter categories for default scaling presets
    enum class ParameterCategory {
        FILTER_CUTOFF,      // Typically benefits from exponential scaling
        FILTER_RESONANCE,   // Usually needs moderate scaling with limiting
        OSCILLATOR_LEVEL,   // Linear scaling works well
        ENVELOPE_ATTACK,    // Exponential scaling for musical response
        ENVELOPE_DECAY,     // Exponential scaling for musical response
        ENVELOPE_SUSTAIN,   // Linear scaling typically preferred
        ENVELOPE_RELEASE,   // Exponential scaling for musical response
        LFO_RATE,          // Logarithmic scaling for wide range coverage
        LFO_DEPTH,         // Linear scaling works well
        DISTORTION_DRIVE,   // Logarithmic scaling to avoid harsh jumps
        DELAY_TIME,        // Linear scaling for precise timing control
        REVERB_SIZE,       // Linear scaling works well
        REVERB_DAMPING,    // Linear scaling works well
        PITCH_BEND,        // Linear scaling with bipolar center
        DETUNE,            // Linear scaling with bipolar center
        PAN,               // Linear scaling with bipolar center
        VOLUME,            // Exponential scaling for perceived loudness
        CUSTOM             // User-defined scaling configuration
    };
    
    // Velocity range mapping configuration
    struct VelocityRange {
        float inputMin;         // Minimum input velocity (0.0-1.0)
        float inputMax;         // Maximum input velocity (0.0-1.0)
        float outputMin;        // Minimum output value (0.0-1.0)
        float outputMax;        // Maximum output value (0.0-1.0)
        bool clampOutput;       // Clamp output to 0.0-1.0 range
        
        VelocityRange() :
            inputMin(0.0f), inputMax(1.0f),
            outputMin(0.0f), outputMax(1.0f),
            clampOutput(true) {}
            
        VelocityRange(float inMin, float inMax, float outMin, float outMax) :
            inputMin(inMin), inputMax(inMax),
            outputMin(outMin), outputMax(outMax),
            clampOutput(true) {}
    };
    
    // Per-parameter velocity scaling configuration
    struct ParameterScalingConfig {
        ParameterCategory category;             // Parameter category for defaults
        float velocityScale;                    // Velocity sensitivity multiplier (0.1-5.0)
        VelocityModulationUI::ModulationPolarity polarity; // Modulation direction
        VelocityRange velocityRange;            // Input/output range mapping
        float deadzone;                         // Velocity deadzone threshold (0.0-0.2)
        float threshold;                        // Minimum velocity for activation (0.0-1.0)
        float hysteresis;                       // Hysteresis for threshold (0.0-0.1)
        bool invertVelocity;                    // Invert velocity response
        bool enableDynamicScaling;              // Enable context-aware scaling
        float centerPoint;                      // Center point for bipolar modulation (0.0-1.0)
        float asymmetry;                        // Asymmetry factor for bipolar (-1.0 to +1.0)
        
        // Advanced scaling parameters
        float compressionRatio;                 // Velocity compression (1.0-10.0, 1.0=no compression)
        float expansionRatio;                   // Velocity expansion (0.1-1.0, 1.0=no expansion)
        float softKnee;                         // Soft knee for compression (0.0-1.0)
        bool enableAutoScaling;                 // Auto-adjust scaling based on usage
        
        ParameterScalingConfig() :
            category(ParameterCategory::CUSTOM),
            velocityScale(1.0f),
            polarity(VelocityModulationUI::ModulationPolarity::POSITIVE),
            velocityRange(),
            deadzone(0.0f),
            threshold(0.0f),
            hysteresis(0.02f),
            invertVelocity(false),
            enableDynamicScaling(false),
            centerPoint(0.5f),
            asymmetry(0.0f),
            compressionRatio(1.0f),
            expansionRatio(1.0f),
            softKnee(0.1f),
            enableAutoScaling(false) {}
    };
    
    // Velocity scaling preset definitions
    struct ScalingPreset {
        std::string name;
        ParameterScalingConfig config;
        std::string description;
        
        ScalingPreset(const std::string& n, const ParameterScalingConfig& c, const std::string& d) :
            name(n), config(c), description(d) {}
    };
    
    // Velocity scaling result for analysis
    struct ScalingResult {
        float originalVelocity;     // Original velocity input (0.0-1.0)
        float scaledVelocity;       // Velocity after scaling (0.0-1.0)
        float finalValue;           // Final parameter value (0.0-1.0)
        bool thresholdPassed;       // Whether velocity exceeded threshold
        bool inDeadzone;            // Whether velocity is in deadzone
        float compressionAmount;    // Amount of compression applied (0.0-1.0)
        float expansionAmount;      // Amount of expansion applied (0.0-1.0)
        ParameterCategory category; // Parameter category used
        
        ScalingResult() :
            originalVelocity(0.0f), scaledVelocity(0.0f), finalValue(0.0f),
            thresholdPassed(false), inDeadzone(false),
            compressionAmount(0.0f), expansionAmount(0.0f),
            category(ParameterCategory::CUSTOM) {}
    };
    
    VelocityParameterScaling();
    ~VelocityParameterScaling() = default;
    
    // Parameter configuration
    void setParameterScaling(uint32_t parameterId, const ParameterScalingConfig& config);
    void setParameterCategory(uint32_t parameterId, ParameterCategory category);
    void setParameterVelocityScale(uint32_t parameterId, float scale);
    void setParameterPolarity(uint32_t parameterId, VelocityModulationUI::ModulationPolarity polarity);
    void setParameterVelocityRange(uint32_t parameterId, const VelocityRange& range);
    void setParameterDeadzone(uint32_t parameterId, float deadzone);
    void setParameterThreshold(uint32_t parameterId, float threshold);
    
    const ParameterScalingConfig& getParameterScaling(uint32_t parameterId) const;
    ParameterCategory getParameterCategory(uint32_t parameterId) const;
    float getParameterVelocityScale(uint32_t parameterId) const;
    bool hasParameterScaling(uint32_t parameterId) const;
    
    // Velocity scaling calculation
    ScalingResult calculateParameterScaling(uint32_t parameterId, float velocity, float baseValue);
    float applyVelocityScaling(uint32_t parameterId, float velocity);
    float applyVelocityRange(const VelocityRange& range, float velocity);
    float applyVelocityCompression(float velocity, float ratio, float softKnee);
    float applyVelocityExpansion(float velocity, float ratio, float softKnee);
    float applyBipolarScaling(float velocity, float centerPoint, float asymmetry);
    
    // Category-based default configurations
    void applyDefaultScalingForCategory(uint32_t parameterId, ParameterCategory category);
    ParameterScalingConfig getDefaultConfigForCategory(ParameterCategory category) const;
    void updateCategoryDefaults(ParameterCategory category, const ParameterScalingConfig& config);
    
    // Preset management
    void addScalingPreset(const ScalingPreset& preset);
    void removeScalingPreset(const std::string& presetName);
    void applyScalingPreset(uint32_t parameterId, const std::string& presetName);
    std::vector<ScalingPreset> getAvailablePresets() const;
    std::vector<ScalingPreset> getPresetsForCategory(ParameterCategory category) const;
    
    // Batch operations
    void setAllParametersScale(float scale);
    void setAllParametersPolarity(VelocityModulationUI::ModulationPolarity polarity);
    void applyCategoryScalingToAll(ParameterCategory category);
    void resetAllParametersToDefaults();
    
    // Auto-scaling and analysis
    void enableAutoScaling(uint32_t parameterId, bool enabled);
    void analyzeVelocityUsage(uint32_t parameterId, float velocity);
    void updateAutoScaling(uint32_t parameterId);
    void resetVelocityAnalysis(uint32_t parameterId);
    
    // System management
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    void reset();
    void removeParameter(uint32_t parameterId);
    
    // Statistics and monitoring
    size_t getConfiguredParameterCount() const;
    std::vector<uint32_t> getParametersInCategory(ParameterCategory category) const;
    float getAverageVelocityScale() const;
    std::unordered_map<ParameterCategory, int> getCategoryCounts() const;
    
    // Utility functions
    static std::string categoryToString(ParameterCategory category);
    static ParameterCategory stringToCategory(const std::string& categoryStr);
    static bool isValidScale(float scale);
    static float clampScale(float scale);
    
private:
    // System state
    bool enabled_;
    
    // Parameter configurations
    std::unordered_map<uint32_t, ParameterScalingConfig> parameterConfigs_;
    std::unordered_map<ParameterCategory, ParameterScalingConfig> categoryDefaults_;
    std::vector<ScalingPreset> scalingPresets_;
    
    // Auto-scaling analysis data
    struct VelocityAnalysis {
        std::vector<float> velocityHistory_;
        float minVelocity_;
        float maxVelocity_;
        float averageVelocity_;
        uint32_t sampleCount_;
        float recommendedScale_;
        
        VelocityAnalysis() : minVelocity_(1.0f), maxVelocity_(0.0f), 
                           averageVelocity_(0.0f), sampleCount_(0), recommendedScale_(1.0f) {}
    };
    
    std::unordered_map<uint32_t, VelocityAnalysis> velocityAnalysis_;
    
    // Default configuration
    static ParameterScalingConfig defaultConfig_;
    
    // Internal methods
    void initializeCategoryDefaults();
    void initializeScalingPresets();
    float applySoftKneeCompression(float velocity, float ratio, float knee);
    float applySoftKneeExpansion(float velocity, float ratio, float knee);
    bool isInDeadzone(float velocity, float deadzone);
    bool passesThreshold(float velocity, float threshold, float hysteresis, bool& state);
    void updateVelocityAnalysis(uint32_t parameterId, float velocity);
    float calculateRecommendedScale(const VelocityAnalysis& analysis);
    
    // Constants
    static constexpr float MIN_VELOCITY_SCALE = 0.1f;
    static constexpr float MAX_VELOCITY_SCALE = 5.0f;
    static constexpr float MIN_DEADZONE = 0.0f;
    static constexpr float MAX_DEADZONE = 0.2f;
    static constexpr float MIN_THRESHOLD = 0.0f;
    static constexpr float MAX_THRESHOLD = 1.0f;
    static constexpr float MIN_COMPRESSION_RATIO = 1.0f;
    static constexpr float MAX_COMPRESSION_RATIO = 10.0f;
    static constexpr float MIN_EXPANSION_RATIO = 0.1f;
    static constexpr float MAX_EXPANSION_RATIO = 1.0f;
    static constexpr int MAX_ANALYSIS_HISTORY = 100;
};