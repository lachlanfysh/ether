#pragma once
#include <cmath>
#include <array>

/**
 * AdvancedParameterSmoother - Sophisticated parameter smoothing system
 * 
 * Features:
 * - Adaptive smoothing times based on parameter type and change magnitude
 * - Multiple smoothing curves (linear, exponential, s-curve)
 * - Fast mode for real-time parameters (1-5ms)
 * - Audible mode for user-facing parameters (10-40ms)
 * - Jump detection and intelligent ramping
 * - CPU-optimized with SIMD-ready operations
 */
class AdvancedParameterSmoother {
public:
    enum class SmoothType {
        FAST,           // 1-5ms for real-time automation
        AUDIBLE,        // 10-40ms for user parameters
        ADAPTIVE,       // Automatically choose based on change rate
        INSTANT         // No smoothing
    };
    
    enum class CurveType {
        LINEAR,         // Linear interpolation
        EXPONENTIAL,    // Exponential decay (most natural)
        S_CURVE,        // S-curve for smooth acceleration/deceleration
        LOGARITHMIC     // Logarithmic curve for frequency-like parameters
    };
    
    struct Config {
        SmoothType smoothType;
        CurveType curveType;
        float fastTimeMs;        // Fast smoothing time
        float audibleTimeMs;     // Audible smoothing time
        float adaptiveThreshold; // Change threshold for adaptive mode
        float jumpThreshold;     // Jump detection threshold
        bool enableJumpPrevention; // Prevent parameter jumps
        float maxChangePerSample;  // Maximum change per sample
        
        Config() : 
            smoothType(SmoothType::AUDIBLE),
            curveType(CurveType::EXPONENTIAL),
            fastTimeMs(2.0f),
            audibleTimeMs(20.0f),
            adaptiveThreshold(0.1f),
            jumpThreshold(0.3f),
            enableJumpPrevention(true),
            maxChangePerSample(0.01f) {}
    };
    
    AdvancedParameterSmoother();
    ~AdvancedParameterSmoother() = default;
    
    // Initialization
    void initialize(float sampleRate, const Config& config = Config());
    void setSampleRate(float sampleRate);
    
    // Configuration
    void setConfig(const Config& config);
    const Config& getConfig() const { return config_; }
    
    void setSmoothType(SmoothType type);
    void setCurveType(CurveType type);
    void setSmoothTime(float timeMs);
    
    // Value control
    void setValue(float value);
    void setTarget(float target);
    void setTargetImmediate(float target); // Skip smoothing for this change
    
    // Processing
    float process();
    void processBlock(float* values, int numSamples, float target);
    void processBlock(float* output, const float* targets, int numSamples);
    
    // Analysis
    bool isSmoothing() const { return std::abs(currentValue_ - targetValue_) > 1e-6f; }
    float getCurrentValue() const { return currentValue_; }
    float getTargetValue() const { return targetValue_; }
    float getSmoothingProgress() const; // 0.0 to 1.0
    float getRemainingTime() const;     // Time until target reached (ms)
    
    // Advanced features
    void reset();
    void reset(float value);
    void freezeAtCurrent(); // Stop smoothing at current value
    void snapToTarget();    // Immediately jump to target
    
    // Batch processing for multiple parameters
    static void processMultiple(AdvancedParameterSmoother* smoothers, int count);
    
private:
    Config config_;
    float sampleRate_ = 44100.0f;
    bool initialized_ = false;
    
    // State
    float currentValue_ = 0.0f;
    float targetValue_ = 0.0f;
    float previousTarget_ = 0.0f;
    
    // Smoothing calculation
    float coefficient_ = 0.0f;
    float increment_ = 0.0f;
    float smoothingTime_ = 0.0f;
    int remainingSamples_ = 0;
    
    // Jump detection
    bool jumpDetected_ = false;
    float jumpStartValue_ = 0.0f;
    float jumpTargetValue_ = 0.0f;
    
    // Adaptive behavior
    float changeVelocity_ = 0.0f;
    float lastChangeTime_ = 0.0f;
    
    // Internal methods
    void calculateCoefficients();
    void updateSmoothingTime();
    float calculateAdaptiveSmoothTime(float change, float velocity) const;
    
    // Curve processing
    float applyCurve(float linearProgress) const;
    float applyLinearCurve(float t) const;
    float applyExponentialCurve(float t) const;
    float applySCurve(float t) const;
    float applyLogarithmicCurve(float t) const;
    
    // Jump handling
    bool detectJump(float newTarget) const;
    void handleJump(float newTarget);
    void processJumpPrevention();
    
    // Utility functions
    float clamp(float value, float min, float max) const;
    float lerp(float a, float b, float t) const;
    
    // Constants
    static constexpr float MIN_COEFFICIENT = 1e-6f;
    static constexpr float MAX_COEFFICIENT = 0.99f;
    static constexpr float MIN_SMOOTH_TIME_MS = 0.1f;
    static constexpr float MAX_SMOOTH_TIME_MS = 1000.0f;
    static constexpr float VELOCITY_SMOOTH = 0.95f;
    static constexpr float S_CURVE_SHARPNESS = 2.0f;
};