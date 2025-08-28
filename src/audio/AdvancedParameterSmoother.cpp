#include "AdvancedParameterSmoother.h"
#include <algorithm>
#include <cmath>
#include <chrono>

AdvancedParameterSmoother::AdvancedParameterSmoother() {
    config_.smoothType = SmoothType::AUDIBLE;
    config_.curveType = CurveType::EXPONENTIAL;
    config_.fastTimeMs = 2.0f;
    config_.audibleTimeMs = 20.0f;
    config_.adaptiveThreshold = 0.1f;
    config_.jumpThreshold = 0.3f;
    config_.enableJumpPrevention = true;
    config_.maxChangePerSample = 0.01f;
    
    sampleRate_ = 44100.0f;
    initialized_ = false;
    
    currentValue_ = 0.0f;
    targetValue_ = 0.0f;
    previousTarget_ = 0.0f;
    
    coefficient_ = 0.0f;
    increment_ = 0.0f;
    smoothingTime_ = config_.audibleTimeMs;
    remainingSamples_ = 0;
    
    jumpDetected_ = false;
    jumpStartValue_ = 0.0f;
    jumpTargetValue_ = 0.0f;
    
    changeVelocity_ = 0.0f;
    lastChangeTime_ = 0.0f;
}

void AdvancedParameterSmoother::initialize(float sampleRate, const Config& config) {
    sampleRate_ = sampleRate;
    config_ = config;
    
    // Clamp configuration values
    config_.fastTimeMs = clamp(config.fastTimeMs, MIN_SMOOTH_TIME_MS, MAX_SMOOTH_TIME_MS);
    config_.audibleTimeMs = clamp(config.audibleTimeMs, MIN_SMOOTH_TIME_MS, MAX_SMOOTH_TIME_MS);
    config_.adaptiveThreshold = clamp(config.adaptiveThreshold, 0.001f, 1.0f);
    config_.jumpThreshold = clamp(config.jumpThreshold, 0.01f, 1.0f);
    config_.maxChangePerSample = clamp(config.maxChangePerSample, 0.0001f, 0.1f);
    
    updateSmoothingTime();
    calculateCoefficients();
    
    initialized_ = true;
}

void AdvancedParameterSmoother::setSampleRate(float sampleRate) {
    if (sampleRate_ != sampleRate) {
        sampleRate_ = sampleRate;
        calculateCoefficients();
    }
}

void AdvancedParameterSmoother::setConfig(const Config& config) {
    config_ = config;
    updateSmoothingTime();
    calculateCoefficients();
}

void AdvancedParameterSmoother::setSmoothType(SmoothType type) {
    if (config_.smoothType != type) {
        config_.smoothType = type;
        updateSmoothingTime();
        calculateCoefficients();
    }
}

void AdvancedParameterSmoother::setCurveType(CurveType type) {
    config_.curveType = type;
}

void AdvancedParameterSmoother::setSmoothTime(float timeMs) {
    smoothingTime_ = clamp(timeMs, MIN_SMOOTH_TIME_MS, MAX_SMOOTH_TIME_MS);
    calculateCoefficients();
}

void AdvancedParameterSmoother::setValue(float value) {
    currentValue_ = value;
    targetValue_ = value;
    previousTarget_ = value;
    jumpDetected_ = false;
    remainingSamples_ = 0;
}

void AdvancedParameterSmoother::setTarget(float target) {
    previousTarget_ = targetValue_;
    
    // Check for jump
    if (config_.enableJumpPrevention && detectJump(target)) {
        handleJump(target);
        return;
    }
    
    targetValue_ = target;
    
    // Calculate change velocity for adaptive smoothing
    float change = std::abs(target - currentValue_);
    float currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count() * 0.001f;
    
    if (lastChangeTime_ > 0.0f) {
        float timeDelta = currentTime - lastChangeTime_;
        if (timeDelta > 0.0f) {
            float velocity = change / timeDelta;
            changeVelocity_ = changeVelocity_ * VELOCITY_SMOOTH + velocity * (1.0f - VELOCITY_SMOOTH);
        }
    }
    
    lastChangeTime_ = currentTime;
    
    // Update smoothing time for adaptive mode
    if (config_.smoothType == SmoothType::ADAPTIVE) {
        float adaptiveTime = calculateAdaptiveSmoothTime(change, changeVelocity_);
        setSmoothTime(adaptiveTime);
    } else {
        updateSmoothingTime();
        calculateCoefficients();
    }
    
    // Calculate remaining samples
    if (smoothingTime_ > 0.0f) {
        remainingSamples_ = static_cast<int>((smoothingTime_ * 0.001f) * sampleRate_);
    } else {
        remainingSamples_ = 0;
    }
}

void AdvancedParameterSmoother::setTargetImmediate(float target) {
    currentValue_ = target;
    targetValue_ = target;
    previousTarget_ = target;
    jumpDetected_ = false;
    remainingSamples_ = 0;
}

float AdvancedParameterSmoother::process() {
    if (config_.smoothType == SmoothType::INSTANT || remainingSamples_ <= 0) {
        currentValue_ = targetValue_;
        return currentValue_;
    }
    
    // Handle jump prevention
    if (jumpDetected_) {
        processJumpPrevention();
        return currentValue_;
    }
    
    // Calculate progress
    float totalSamples = (smoothingTime_ * 0.001f) * sampleRate_;
    float progress = (totalSamples - remainingSamples_) / totalSamples;
    progress = clamp(progress, 0.0f, 1.0f);
    
    // Apply curve
    float curvedProgress = applyCurve(progress);
    
    // Update current value
    currentValue_ = lerp(currentValue_, targetValue_, curvedProgress * coefficient_);
    
    // Apply max change per sample limit
    float maxChange = config_.maxChangePerSample;
    float change = currentValue_ - previousTarget_;
    if (std::abs(change) > maxChange) {
        currentValue_ = previousTarget_ + (change > 0 ? maxChange : -maxChange);
    }
    
    remainingSamples_--;
    
    // Check if we've reached the target
    if (remainingSamples_ <= 0 || std::abs(currentValue_ - targetValue_) < 1e-6f) {
        currentValue_ = targetValue_;
        remainingSamples_ = 0;
    }
    
    return currentValue_;
}

void AdvancedParameterSmoother::processBlock(float* values, int numSamples, float target) {
    setTarget(target);
    
    for (int i = 0; i < numSamples; i++) {
        values[i] = process();
    }
}

void AdvancedParameterSmoother::processBlock(float* output, const float* targets, int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        setTarget(targets[i]);
        output[i] = process();
    }
}

float AdvancedParameterSmoother::getSmoothingProgress() const {
    if (remainingSamples_ <= 0) {
        return 1.0f;
    }
    
    float totalSamples = (smoothingTime_ * 0.001f) * sampleRate_;
    return (totalSamples - remainingSamples_) / totalSamples;
}

float AdvancedParameterSmoother::getRemainingTime() const {
    return (remainingSamples_ / sampleRate_) * 1000.0f;
}

void AdvancedParameterSmoother::reset() {
    currentValue_ = 0.0f;
    targetValue_ = 0.0f;
    previousTarget_ = 0.0f;
    jumpDetected_ = false;
    remainingSamples_ = 0;
    changeVelocity_ = 0.0f;
    lastChangeTime_ = 0.0f;
}

void AdvancedParameterSmoother::reset(float value) {
    setValue(value);
}

void AdvancedParameterSmoother::freezeAtCurrent() {
    targetValue_ = currentValue_;
    remainingSamples_ = 0;
    jumpDetected_ = false;
}

void AdvancedParameterSmoother::snapToTarget() {
    currentValue_ = targetValue_;
    remainingSamples_ = 0;
    jumpDetected_ = false;
}

void AdvancedParameterSmoother::processMultiple(AdvancedParameterSmoother* smoothers, int count) {
    // Batch process multiple smoothers for efficiency
    for (int i = 0; i < count; i++) {
        smoothers[i].process();
    }
}

// Private method implementations

void AdvancedParameterSmoother::calculateCoefficients() {
    if (sampleRate_ <= 0.0f || smoothingTime_ <= 0.0f) {
        coefficient_ = 1.0f;
        increment_ = 0.0f;
        return;
    }
    
    float samples = (smoothingTime_ * 0.001f) * sampleRate_;
    
    switch (config_.curveType) {
        case CurveType::LINEAR:
            coefficient_ = 1.0f / samples;
            break;
        case CurveType::EXPONENTIAL:
            coefficient_ = 1.0f - std::exp(-1.0f / samples);
            break;
        case CurveType::S_CURVE:
        case CurveType::LOGARITHMIC:
            coefficient_ = 1.0f / samples; // Will be modified by curve application
            break;
    }
    
    coefficient_ = clamp(coefficient_, MIN_COEFFICIENT, MAX_COEFFICIENT);
}

void AdvancedParameterSmoother::updateSmoothingTime() {
    switch (config_.smoothType) {
        case SmoothType::FAST:
            smoothingTime_ = config_.fastTimeMs;
            break;
        case SmoothType::AUDIBLE:
            smoothingTime_ = config_.audibleTimeMs;
            break;
        case SmoothType::ADAPTIVE:
            // Will be calculated dynamically
            smoothingTime_ = config_.audibleTimeMs;
            break;
        case SmoothType::INSTANT:
            smoothingTime_ = 0.0f;
            break;
    }
}

float AdvancedParameterSmoother::calculateAdaptiveSmoothTime(float change, float velocity) const {
    if (change < config_.adaptiveThreshold) {
        return config_.fastTimeMs;
    }
    
    // Base time on change magnitude
    float baseTime = lerp(config_.fastTimeMs, config_.audibleTimeMs, 
                         clamp(change / config_.adaptiveThreshold, 0.0f, 1.0f));
    
    // Adjust based on velocity (faster changes need more smoothing)
    float velocityFactor = clamp(velocity * 0.1f, 0.5f, 2.0f);
    
    return baseTime * velocityFactor;
}

float AdvancedParameterSmoother::applyCurve(float linearProgress) const {
    switch (config_.curveType) {
        case CurveType::LINEAR:
            return applyLinearCurve(linearProgress);
        case CurveType::EXPONENTIAL:
            return applyExponentialCurve(linearProgress);
        case CurveType::S_CURVE:
            return applySCurve(linearProgress);
        case CurveType::LOGARITHMIC:
            return applyLogarithmicCurve(linearProgress);
        default:
            return linearProgress;
    }
}

float AdvancedParameterSmoother::applyLinearCurve(float t) const {
    return t;
}

float AdvancedParameterSmoother::applyExponentialCurve(float t) const {
    return 1.0f - std::exp(-t * 3.0f);
}

float AdvancedParameterSmoother::applySCurve(float t) const {
    // Smooth S-curve using sigmoid-like function
    float adjusted = (t - 0.5f) * S_CURVE_SHARPNESS;
    return 1.0f / (1.0f + std::exp(-adjusted));
}

float AdvancedParameterSmoother::applyLogarithmicCurve(float t) const {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    
    return std::log(1.0f + t * 9.0f) / std::log(10.0f);
}

bool AdvancedParameterSmoother::detectJump(float newTarget) const {
    float change = std::abs(newTarget - currentValue_);
    return change > config_.jumpThreshold;
}

void AdvancedParameterSmoother::handleJump(float newTarget) {
    jumpDetected_ = true;
    jumpStartValue_ = currentValue_;
    jumpTargetValue_ = newTarget;
    targetValue_ = newTarget;
    
    // Use longer smoothing time for jumps
    float jumpSmoothTime = config_.audibleTimeMs * 2.0f;
    setSmoothTime(jumpSmoothTime);
}

void AdvancedParameterSmoother::processJumpPrevention() {
    // Gradually move toward jump target with limited rate
    float direction = (jumpTargetValue_ > currentValue_) ? 1.0f : -1.0f;
    float maxChange = config_.maxChangePerSample * 0.5f; // Slower for jumps
    
    currentValue_ += direction * maxChange;
    
    // Check if we've reached the target
    bool reachedTarget = (direction > 0.0f && currentValue_ >= jumpTargetValue_) ||
                        (direction < 0.0f && currentValue_ <= jumpTargetValue_);
    
    if (reachedTarget) {
        currentValue_ = jumpTargetValue_;
        jumpDetected_ = false;
    }
}

float AdvancedParameterSmoother::clamp(float value, float min, float max) const {
    return std::max(min, std::min(max, value));
}

float AdvancedParameterSmoother::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}