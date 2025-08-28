#pragma once
#include <cmath>

// Minimal stub implementation for testing
class AdvancedParameterSmoother {
public:
    enum class SmoothType {
        FAST,
        AUDIBLE,
        ADAPTIVE,
        INSTANT
    };
    
    enum class CurveType {
        LINEAR,
        EXPONENTIAL,
        S_CURVE,
        LOGARITHMIC
    };
    
    struct Config {
        SmoothType smoothType;
        CurveType curveType;
        float fastTimeMs;
        float audibleTimeMs;
        float adaptiveThreshold;
        float jumpThreshold;
        bool enableJumpPrevention;
        float maxChangePerSample;
        
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
    
    AdvancedParameterSmoother() : currentValue_(0.0f), targetValue_(0.0f), sampleRate_(48000.0f) {}
    ~AdvancedParameterSmoother() = default;
    
    void initialize(float sampleRate, const Config& config = Config()) {
        sampleRate_ = sampleRate;
        config_ = config;
    }
    
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
    }
    
    void setValue(float value) {
        currentValue_ = value;
        targetValue_ = value;
    }
    
    void setTarget(float target) {
        targetValue_ = target;
    }
    
    float process() {
        if (std::abs(currentValue_ - targetValue_) < 1e-6f) {
            return currentValue_;
        }
        
        // Simple linear interpolation for testing
        float smoothingSpeed = 0.01f; // Adjust based on sample rate and smooth time
        float diff = targetValue_ - currentValue_;
        currentValue_ += diff * smoothingSpeed;
        
        return currentValue_;
    }
    
    bool isSmoothing() const {
        return std::abs(currentValue_ - targetValue_) > 1e-6f;
    }
    
    float getCurrentValue() const { return currentValue_; }
    float getTargetValue() const { return targetValue_; }

private:
    Config config_;
    float currentValue_;
    float targetValue_;
    float sampleRate_;
};