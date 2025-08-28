#pragma once
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>
#include <chrono>
#include "../../interface/IVelocityModulationView.h"

/**
 * RelativeVelocityModulation - Advanced velocity modulation calculation system
 * 
 * Provides sophisticated velocity modulation beyond simple absolute scaling:
 * - Relative modulation (velocity affects change from current value, not absolute target)
 * - Velocity curve shaping (exponential, logarithmic, S-curve, custom curves)
 * - Velocity history averaging and smoothing for consistent modulation
 * - Parameter-specific velocity response scaling and offset
 * - Bidirectional modulation (both positive and negative from center point)
 * - Velocity ramping and envelope following for dynamic modulation
 * 
 * Features:
 * - Multiple velocity calculation modes (absolute, relative, additive, multiplicative)
 * - Advanced curve shaping with customizable response characteristics
 * - Real-time velocity smoothing and history-based averaging
 * - Per-parameter velocity response calibration and scaling
 * - Integration with existing VelocityLatchSystem for seamless operation
 * - CPU-optimized calculations suitable for real-time audio processing
 */
class RelativeVelocityModulation {
public:
    // Velocity modulation calculation modes
    enum class ModulationMode {
        ABSOLUTE,       // Direct velocity-to-parameter mapping (0-127 → 0.0-1.0)
        RELATIVE,       // Velocity modulates change from current parameter value
        ADDITIVE,       // Velocity adds/subtracts from base parameter value
        MULTIPLICATIVE, // Velocity scales base parameter value proportionally
        ENVELOPE,       // Velocity creates envelope-style modulation over time
        BIPOLAR_CENTER  // Velocity modulates bidirectionally from center point
    };
    
    // Velocity curve shaping types
    enum class CurveType {
        LINEAR,         // Direct linear mapping
        EXPONENTIAL,    // Exponential curve (more sensitivity at low velocities)
        LOGARITHMIC,    // Logarithmic curve (more sensitivity at high velocities)
        S_CURVE,        // S-shaped curve (gentle at extremes, steep in middle)
        POWER_CURVE,    // Customizable power curve with adjustable exponent
        STEPPED,        // Quantized steps for discrete velocity zones
        CUSTOM_LUT      // Custom lookup table for arbitrary curve shapes
    };
    
    // Velocity smoothing and filtering options
    enum class SmoothingType {
        NONE,           // No smoothing (immediate response)
        LOW_PASS,       // Simple low-pass filter smoothing
        MOVING_AVERAGE, // Moving average over N samples
        EXPONENTIAL,    // Exponential smoothing with decay constant
        PEAK_HOLD,      // Peak hold with slow decay
        RMS_AVERAGE     // RMS averaging for energy-based smoothing
    };
    
    // Per-parameter velocity modulation configuration
    struct VelocityModulationConfig {
        ModulationMode mode;                    // Calculation mode
        CurveType curveType;                    // Velocity curve shape
        float curveAmount;                      // Curve intensity (0.1-10.0)
        float velocityScale;                    // Velocity sensitivity scaling (0.1-5.0)
        float velocityOffset;                   // Velocity offset (-1.0 to +1.0)
        float modulationDepth;                  // Modulation depth multiplier (0.0-2.0)
        float centerPoint;                      // Center point for bipolar modulation (0.0-1.0)
        bool invertVelocity;                    // Invert velocity response
        SmoothingType smoothingType;            // Velocity smoothing method
        float smoothingAmount;                  // Smoothing intensity (0.0-1.0)
        int historyLength;                      // History buffer length (1-32 samples)
        VelocityModulationUI::ModulationPolarity polarity; // Modulation direction
        
        // Advanced parameters
        float attackTime;                       // Attack time for envelope mode (0-1000ms)
        float releaseTime;                      // Release time for envelope mode (0-5000ms)
        float threshold;                        // Velocity threshold for activation (0-127)
        float hysteresis;                       // Hysteresis amount for threshold (0-20)
        bool enableQuantization;                // Enable velocity quantization
        int quantizationSteps;                  // Number of quantization steps (2-16)
        
        VelocityModulationConfig() :
            mode(ModulationMode::ABSOLUTE),
            curveType(CurveType::LINEAR),
            curveAmount(1.0f),
            velocityScale(1.0f),
            velocityOffset(0.0f),
            modulationDepth(1.0f),
            centerPoint(0.5f),
            invertVelocity(false),
            smoothingType(SmoothingType::LOW_PASS),
            smoothingAmount(0.1f),
            historyLength(4),
            polarity(VelocityModulationUI::ModulationPolarity::POSITIVE),
            attackTime(10.0f),
            releaseTime(100.0f),
            threshold(1.0f),
            hysteresis(5.0f),
            enableQuantization(false),
            quantizationSteps(8) {}
    };
    
    // Velocity modulation calculation result
    struct ModulationResult {
        float modulatedValue;       // Final modulated parameter value (0.0-1.0)
        float rawVelocity;          // Raw velocity input (0.0-1.0)
        float processedVelocity;    // Processed velocity after curve/scaling (0.0-1.0)
        float modulationAmount;     // Modulation amount applied (-2.0 to +2.0)
        bool isActive;              // Whether modulation is currently active
        float smoothedVelocity;     // Smoothed velocity value (0.0-1.0)
        uint32_t sampleCount;       // Number of samples processed
        
        ModulationResult() :
            modulatedValue(0.0f),
            rawVelocity(0.0f),
            processedVelocity(0.0f),
            modulationAmount(0.0f),
            isActive(false),
            smoothedVelocity(0.0f),
            sampleCount(0) {}
    };
    
    RelativeVelocityModulation();
    ~RelativeVelocityModulation() = default;
    
    // Configuration management
    void setParameterConfig(uint32_t parameterId, const VelocityModulationConfig& config);
    const VelocityModulationConfig& getParameterConfig(uint32_t parameterId) const;
    void removeParameterConfig(uint32_t parameterId);
    bool hasParameterConfig(uint32_t parameterId) const;
    
    // Velocity modulation calculation
    ModulationResult calculateModulation(uint32_t parameterId, float baseValue, uint8_t velocity);
    float calculateRelativeModulation(uint32_t parameterId, float currentValue, float targetValue, uint8_t velocity);
    float calculateAdditiveModulation(uint32_t parameterId, float baseValue, uint8_t velocity);
    float calculateMultiplicativeModulation(uint32_t parameterId, float baseValue, uint8_t velocity);
    float calculateEnvelopeModulation(uint32_t parameterId, float baseValue, uint8_t velocity, float deltaTime);
    float calculateBipolarModulation(uint32_t parameterId, float centerValue, uint8_t velocity);
    
    // Velocity curve processing
    float applyCurve(float velocity, CurveType curveType, float curveAmount) const;
    float applyLinearCurve(float velocity) const;
    float applyExponentialCurve(float velocity, float amount) const;
    float applyLogarithmicCurve(float velocity, float amount) const;
    float applySCurve(float velocity, float amount) const;
    float applyPowerCurve(float velocity, float exponent) const;
    float applySteppedCurve(float velocity, int steps) const;
    float applyCustomLUT(float velocity, const std::vector<float>& lut) const;
    
    // Velocity smoothing and filtering
    float applySmoothing(uint32_t parameterId, float velocity);
    float applyLowPassFilter(uint32_t parameterId, float velocity, float smoothingAmount);
    float applyMovingAverage(uint32_t parameterId, float velocity, int historyLength);
    float applyExponentialSmoothing(uint32_t parameterId, float velocity, float decay);
    float applyPeakHold(uint32_t parameterId, float velocity, float decay);
    float applyRMSAverage(uint32_t parameterId, float velocity, int historyLength);
    
    // Velocity quantization and processing
    float quantizeVelocity(float velocity, int steps) const;
    float applyThreshold(float velocity, float threshold, float hysteresis) const;
    float scaleAndOffset(float velocity, float scale, float offset) const;
    
    // Batch operations
    void updateAllModulations(const std::unordered_map<uint32_t, float>& baseValues, uint8_t velocity);
    void resetAllSmoothing();
    void clearAllHistory();
    
    // System management
    void setSampleRate(float sampleRate);
    float getSampleRate() const { return sampleRate_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    void reset();
    
    // Performance monitoring
    size_t getActiveParameterCount() const;
    float getCPUUsageEstimate() const;
    void enableProfiling(bool enabled) { profilingEnabled_ = enabled; }
    
private:
    // System state
    bool enabled_;
    float sampleRate_;
    bool profilingEnabled_;
    
    // Per-parameter configurations and state
    std::unordered_map<uint32_t, VelocityModulationConfig> parameterConfigs_;
    std::unordered_map<uint32_t, std::vector<float>> velocityHistory_;
    std::unordered_map<uint32_t, float> smoothedValues_;
    std::unordered_map<uint32_t, float> peakHoldValues_;
    std::unordered_map<uint32_t, float> envelopeStates_;
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> lastUpdateTimes_;
    std::unordered_map<uint32_t, bool> thresholdStates_;
    
    // Default configuration
    static VelocityModulationConfig defaultConfig_;
    
    // Internal utility methods
    float clampValue(float value) const;
    float normalizeVelocity(uint8_t velocity) const;
    float interpolateLinear(float x, float y0, float y1) const;
    float calculateDeltaTime(uint32_t parameterId) const;
    void updateVelocityHistory(uint32_t parameterId, float velocity);
    void updateEnvelopeState(uint32_t parameterId, float target, float deltaTime);
    
    // Performance monitoring
    mutable std::unordered_map<uint32_t, uint64_t> processingTimes_;
    mutable uint32_t totalSampleCount_;
    
    // Constants
    static constexpr float MIN_VELOCITY = 0.0f;
    static constexpr float MAX_VELOCITY = 1.0f;
    static constexpr float MIN_CURVE_AMOUNT = 0.1f;
    static constexpr float MAX_CURVE_AMOUNT = 10.0f;
    static constexpr float MIN_SMOOTHING = 0.0f;
    static constexpr float MAX_SMOOTHING = 1.0f;
    static constexpr int MIN_HISTORY_LENGTH = 1;
    static constexpr int MAX_HISTORY_LENGTH = 32;
    static constexpr float DEFAULT_SAMPLE_RATE = 48000.0f;
    static constexpr float ENVELOPE_MIN_TIME = 0.001f;   // 1ms minimum
    static constexpr float ENVELOPE_MAX_TIME = 5.0f;     // 5s maximum
};