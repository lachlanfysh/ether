#pragma once
#include <cmath>

/**
 * ParameterSmoother - Smooth parameter changes to avoid clicks
 */
class ParameterSmoother {
public:
    ParameterSmoother() = default;
    ~ParameterSmoother() = default;
    
    void initialize(float sampleRate, float smoothTimeMs) {
        sampleRate_ = sampleRate;
        setSmoothTime(smoothTimeMs);
    }
    
    void setSmoothTime(float timeMs) {
        if (sampleRate_ > 0.0f && timeMs > 0.0f) {
            float samples = timeMs * 0.001f * sampleRate_;
            coefficient_ = std::exp(-1.0f / samples);
        } else {
            coefficient_ = 0.0f;
        }
    }
    
    void setValue(float value) {
        currentValue_ = value;
        targetValue_ = value;
    }
    
    void setTarget(float target) {
        targetValue_ = target;
    }
    
    float process() {
        currentValue_ = currentValue_ * coefficient_ + targetValue_ * (1.0f - coefficient_);
        return currentValue_;
    }
    
    void reset() {
        currentValue_ = 0.0f;
        targetValue_ = 0.0f;
    }
    
    float getCurrentValue() const { return currentValue_; }
    float getTargetValue() const { return targetValue_; }
    
private:
    float sampleRate_ = 44100.0f;
    float coefficient_ = 0.0f;
    float currentValue_ = 0.0f;
    float targetValue_ = 0.0f;
};