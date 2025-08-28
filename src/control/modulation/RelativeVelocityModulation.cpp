#include "RelativeVelocityModulation.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <chrono>

// Static member definition
RelativeVelocityModulation::VelocityModulationConfig RelativeVelocityModulation::defaultConfig_;

RelativeVelocityModulation::RelativeVelocityModulation() {
    enabled_ = true;
    sampleRate_ = DEFAULT_SAMPLE_RATE;
    profilingEnabled_ = false;
    totalSampleCount_ = 0;
}

// Configuration management
void RelativeVelocityModulation::setParameterConfig(uint32_t parameterId, const VelocityModulationConfig& config) {
    parameterConfigs_[parameterId] = config;
    
    // Initialize state for this parameter
    velocityHistory_[parameterId].clear();
    velocityHistory_[parameterId].reserve(config.historyLength);
    smoothedValues_[parameterId] = 0.0f;
    peakHoldValues_[parameterId] = 0.0f;
    envelopeStates_[parameterId] = 0.0f;
    thresholdStates_[parameterId] = false;
    lastUpdateTimes_[parameterId] = std::chrono::steady_clock::now();
}

const RelativeVelocityModulation::VelocityModulationConfig& RelativeVelocityModulation::getParameterConfig(uint32_t parameterId) const {
    auto it = parameterConfigs_.find(parameterId);
    return (it != parameterConfigs_.end()) ? it->second : defaultConfig_;
}

void RelativeVelocityModulation::removeParameterConfig(uint32_t parameterId) {
    parameterConfigs_.erase(parameterId);
    velocityHistory_.erase(parameterId);
    smoothedValues_.erase(parameterId);
    peakHoldValues_.erase(parameterId);
    envelopeStates_.erase(parameterId);
    thresholdStates_.erase(parameterId);
    lastUpdateTimes_.erase(parameterId);
    processingTimes_.erase(parameterId);
}

bool RelativeVelocityModulation::hasParameterConfig(uint32_t parameterId) const {
    return parameterConfigs_.find(parameterId) != parameterConfigs_.end();
}

// Velocity modulation calculation
RelativeVelocityModulation::ModulationResult RelativeVelocityModulation::calculateModulation(uint32_t parameterId, float baseValue, uint8_t velocity) {
    if (!enabled_) {
        ModulationResult result;
        result.modulatedValue = baseValue;
        return result;
    }
    
    auto configIt = parameterConfigs_.find(parameterId);
    if (configIt == parameterConfigs_.end()) {
        ModulationResult result;
        result.modulatedValue = baseValue;
        return result;
    }
    
    const auto& config = configIt->second;
    ModulationResult result;
    
    // Start profiling if enabled
    auto startTime = profilingEnabled_ ? std::chrono::high_resolution_clock::now() : std::chrono::high_resolution_clock::time_point{};
    
    // Normalize velocity
    result.rawVelocity = normalizeVelocity(velocity);
    
    // Apply velocity inversion if enabled
    float processedVel = config.invertVelocity ? (1.0f - result.rawVelocity) : result.rawVelocity;
    
    // Apply threshold and hysteresis
    processedVel = applyThreshold(processedVel, config.threshold / 127.0f, config.hysteresis / 127.0f);
    
    // Apply velocity curve
    processedVel = applyCurve(processedVel, config.curveType, config.curveAmount);
    
    // Apply velocity scaling and offset
    processedVel = scaleAndOffset(processedVel, config.velocityScale, config.velocityOffset);
    
    // Apply quantization if enabled
    if (config.enableQuantization) {
        processedVel = quantizeVelocity(processedVel, config.quantizationSteps);
    }
    
    // Apply smoothing
    result.smoothedVelocity = applySmoothing(parameterId, processedVel);
    result.processedVelocity = result.smoothedVelocity;
    
    // Calculate modulation based on mode
    float deltaTime = calculateDeltaTime(parameterId);
    
    switch (config.mode) {
        case ModulationMode::ABSOLUTE:
            result.modulatedValue = baseValue + (result.processedVelocity * config.modulationDepth);
            result.modulationAmount = result.processedVelocity * config.modulationDepth;
            break;
            
        case ModulationMode::RELATIVE:
            result.modulatedValue = calculateRelativeModulation(parameterId, baseValue, baseValue + config.modulationDepth, velocity);
            result.modulationAmount = result.modulatedValue - baseValue;
            break;
            
        case ModulationMode::ADDITIVE:
            result.modulatedValue = calculateAdditiveModulation(parameterId, baseValue, velocity);
            result.modulationAmount = result.modulatedValue - baseValue;
            break;
            
        case ModulationMode::MULTIPLICATIVE:
            result.modulatedValue = calculateMultiplicativeModulation(parameterId, baseValue, velocity);
            result.modulationAmount = result.modulatedValue - baseValue;
            break;
            
        case ModulationMode::ENVELOPE:
            result.modulatedValue = calculateEnvelopeModulation(parameterId, baseValue, velocity, deltaTime);
            result.modulationAmount = result.modulatedValue - baseValue;
            break;
            
        case ModulationMode::BIPOLAR_CENTER:
            result.modulatedValue = calculateBipolarModulation(parameterId, config.centerPoint, velocity);
            result.modulationAmount = result.modulatedValue - config.centerPoint;
            break;
    }
    
    // Clamp final result
    result.modulatedValue = clampValue(result.modulatedValue);
    result.isActive = (std::abs(result.modulationAmount) > 0.001f);
    result.sampleCount = ++totalSampleCount_;
    
    // Update timing
    lastUpdateTimes_[parameterId] = std::chrono::steady_clock::now();
    
    // End profiling if enabled
    if (profilingEnabled_) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
        processingTimes_[parameterId] += duration.count();
    }
    
    return result;
}

float RelativeVelocityModulation::calculateRelativeModulation(uint32_t parameterId, float currentValue, float targetValue, uint8_t velocity) {
    const auto& config = getParameterConfig(parameterId);
    float normalizedVel = normalizeVelocity(velocity);
    
    // Calculate relative change from current to target
    float change = targetValue - currentValue;
    float modulatedChange = change * normalizedVel * config.modulationDepth;
    
    return clampValue(currentValue + modulatedChange);
}

float RelativeVelocityModulation::calculateAdditiveModulation(uint32_t parameterId, float baseValue, uint8_t velocity) {
    const auto& config = getParameterConfig(parameterId);
    float normalizedVel = normalizeVelocity(velocity);
    
    // Apply curve and scaling
    float processedVel = applyCurve(normalizedVel, config.curveType, config.curveAmount);
    processedVel = scaleAndOffset(processedVel, config.velocityScale, config.velocityOffset);
    
    // Add modulation based on polarity
    float modulation = processedVel * config.modulationDepth;
    if (config.polarity == VelocityModulationUI::ModulationPolarity::NEGATIVE) {
        modulation = -modulation;
    } else if (config.polarity == VelocityModulationUI::ModulationPolarity::BIPOLAR) {
        modulation = (processedVel - 0.5f) * 2.0f * config.modulationDepth;
    }
    
    return clampValue(baseValue + modulation);
}

float RelativeVelocityModulation::calculateMultiplicativeModulation(uint32_t parameterId, float baseValue, uint8_t velocity) {
    const auto& config = getParameterConfig(parameterId);
    float normalizedVel = normalizeVelocity(velocity);
    
    // Apply curve and scaling
    float processedVel = applyCurve(normalizedVel, config.curveType, config.curveAmount);
    processedVel = scaleAndOffset(processedVel, config.velocityScale, config.velocityOffset);
    
    // Scale base value by velocity
    float scaleFactor = 1.0f + (processedVel - 0.5f) * config.modulationDepth;
    
    return clampValue(baseValue * scaleFactor);
}

float RelativeVelocityModulation::calculateEnvelopeModulation(uint32_t parameterId, float baseValue, uint8_t velocity, float deltaTime) {
    const auto& config = getParameterConfig(parameterId);
    float normalizedVel = normalizeVelocity(velocity);
    
    // Get current envelope state
    float& envelopeState = envelopeStates_[parameterId];
    
    // Calculate target from velocity
    float target = normalizedVel * config.modulationDepth;
    
    // Apply envelope timing
    float attackRate = 1.0f / std::max(config.attackTime / 1000.0f, ENVELOPE_MIN_TIME);
    float releaseRate = 1.0f / std::max(config.releaseTime / 1000.0f, ENVELOPE_MIN_TIME);
    
    if (target > envelopeState) {
        // Attack phase
        envelopeState += (target - envelopeState) * attackRate * deltaTime;
    } else {
        // Release phase
        envelopeState += (target - envelopeState) * releaseRate * deltaTime;
    }
    
    envelopeState = clampValue(envelopeState);
    
    return clampValue(baseValue + envelopeState);
}

float RelativeVelocityModulation::calculateBipolarModulation(uint32_t parameterId, float centerValue, uint8_t velocity) {
    const auto& config = getParameterConfig(parameterId);
    float normalizedVel = normalizeVelocity(velocity);
    
    // Apply curve and scaling
    float processedVel = applyCurve(normalizedVel, config.curveType, config.curveAmount);
    processedVel = scaleAndOffset(processedVel, config.velocityScale, config.velocityOffset);
    
    // Create bipolar modulation around center point
    float modulation = (processedVel - 0.5f) * config.modulationDepth;
    
    return clampValue(centerValue + modulation);
}

// Velocity curve processing
float RelativeVelocityModulation::applyCurve(float velocity, CurveType curveType, float curveAmount) const {
    switch (curveType) {
        case CurveType::LINEAR:
            return applyLinearCurve(velocity);
        case CurveType::EXPONENTIAL:
            return applyExponentialCurve(velocity, curveAmount);
        case CurveType::LOGARITHMIC:
            return applyLogarithmicCurve(velocity, curveAmount);
        case CurveType::S_CURVE:
            return applySCurve(velocity, curveAmount);
        case CurveType::POWER_CURVE:
            return applyPowerCurve(velocity, curveAmount);
        case CurveType::STEPPED:
            return applySteppedCurve(velocity, static_cast<int>(curveAmount));
        case CurveType::CUSTOM_LUT:
            // Would use custom lookup table in real implementation
            return applyLinearCurve(velocity);
    }
    return velocity;
}

float RelativeVelocityModulation::applyLinearCurve(float velocity) const {
    return velocity;
}

float RelativeVelocityModulation::applyExponentialCurve(float velocity, float amount) const {
    float clampedAmount = std::max(MIN_CURVE_AMOUNT, std::min(amount, MAX_CURVE_AMOUNT));
    return std::pow(velocity, 1.0f / clampedAmount);
}

float RelativeVelocityModulation::applyLogarithmicCurve(float velocity, float amount) const {
    float clampedAmount = std::max(MIN_CURVE_AMOUNT, std::min(amount, MAX_CURVE_AMOUNT));
    return std::pow(velocity, clampedAmount);
}

float RelativeVelocityModulation::applySCurve(float velocity, float amount) const {
    float clampedAmount = std::max(MIN_CURVE_AMOUNT, std::min(amount, MAX_CURVE_AMOUNT));
    float x = velocity * 2.0f - 1.0f;  // Map to -1 to +1
    float curved = std::tanh(x * clampedAmount) / std::tanh(clampedAmount);
    return (curved + 1.0f) * 0.5f;  // Map back to 0 to 1
}

float RelativeVelocityModulation::applyPowerCurve(float velocity, float exponent) const {
    float clampedExponent = std::max(MIN_CURVE_AMOUNT, std::min(exponent, MAX_CURVE_AMOUNT));
    return std::pow(velocity, clampedExponent);
}

float RelativeVelocityModulation::applySteppedCurve(float velocity, int steps) const {
    int clampedSteps = std::max(2, std::min(steps, 16));
    float stepSize = 1.0f / static_cast<float>(clampedSteps - 1);
    int stepIndex = static_cast<int>(velocity * (clampedSteps - 1));
    return stepIndex * stepSize;
}

float RelativeVelocityModulation::applyCustomLUT(float velocity, const std::vector<float>& lut) const {
    if (lut.empty()) return velocity;
    
    float index = velocity * (lut.size() - 1);
    int lowerIndex = static_cast<int>(index);
    int upperIndex = std::min(lowerIndex + 1, static_cast<int>(lut.size() - 1));
    float fraction = index - lowerIndex;
    
    return interpolateLinear(fraction, lut[lowerIndex], lut[upperIndex]);
}

// Velocity smoothing and filtering
float RelativeVelocityModulation::applySmoothing(uint32_t parameterId, float velocity) {
    const auto& config = getParameterConfig(parameterId);
    
    switch (config.smoothingType) {
        case SmoothingType::NONE:
            return velocity;
        case SmoothingType::LOW_PASS:
            return applyLowPassFilter(parameterId, velocity, config.smoothingAmount);
        case SmoothingType::MOVING_AVERAGE:
            return applyMovingAverage(parameterId, velocity, config.historyLength);
        case SmoothingType::EXPONENTIAL:
            return applyExponentialSmoothing(parameterId, velocity, config.smoothingAmount);
        case SmoothingType::PEAK_HOLD:
            return applyPeakHold(parameterId, velocity, config.smoothingAmount);
        case SmoothingType::RMS_AVERAGE:
            return applyRMSAverage(parameterId, velocity, config.historyLength);
    }
    return velocity;
}

float RelativeVelocityModulation::applyLowPassFilter(uint32_t parameterId, float velocity, float smoothingAmount) {
    float& smoothed = smoothedValues_[parameterId];
    float alpha = std::max(MIN_SMOOTHING, std::min(smoothingAmount, MAX_SMOOTHING));
    smoothed = alpha * velocity + (1.0f - alpha) * smoothed;
    return smoothed;
}

float RelativeVelocityModulation::applyMovingAverage(uint32_t parameterId, float velocity, int historyLength) {
    (void)historyLength; // Used by calling code to configure history length
    updateVelocityHistory(parameterId, velocity);
    
    auto& history = velocityHistory_[parameterId];
    if (history.empty()) return velocity;
    
    float sum = std::accumulate(history.begin(), history.end(), 0.0f);
    return sum / static_cast<float>(history.size());
}

float RelativeVelocityModulation::applyExponentialSmoothing(uint32_t parameterId, float velocity, float decay) {
    float& smoothed = smoothedValues_[parameterId];
    float alpha = 1.0f - std::max(MIN_SMOOTHING, std::min(decay, MAX_SMOOTHING));
    smoothed = alpha * velocity + (1.0f - alpha) * smoothed;
    return smoothed;
}

float RelativeVelocityModulation::applyPeakHold(uint32_t parameterId, float velocity, float decay) {
    float& peak = peakHoldValues_[parameterId];
    
    if (velocity > peak) {
        peak = velocity;  // New peak
    } else {
        // Decay existing peak
        float decayRate = std::max(MIN_SMOOTHING, std::min(decay, MAX_SMOOTHING));
        peak = std::max(velocity, peak - decayRate / sampleRate_);
    }
    
    return peak;
}

float RelativeVelocityModulation::applyRMSAverage(uint32_t parameterId, float velocity, int historyLength) {
    (void)historyLength; // Used by calling code to configure history length
    updateVelocityHistory(parameterId, velocity);
    
    auto& history = velocityHistory_[parameterId];
    if (history.empty()) return velocity;
    
    float sumSquares = 0.0f;
    for (float v : history) {
        sumSquares += v * v;
    }
    
    return std::sqrt(sumSquares / static_cast<float>(history.size()));
}

// Velocity quantization and processing
float RelativeVelocityModulation::quantizeVelocity(float velocity, int steps) const {
    int clampedSteps = std::max(2, std::min(steps, 16));
    float stepSize = 1.0f / static_cast<float>(clampedSteps - 1);
    int stepIndex = static_cast<int>(velocity * (clampedSteps - 1) + 0.5f);  // Round to nearest
    return stepIndex * stepSize;
}

float RelativeVelocityModulation::applyThreshold(float velocity, float threshold, float hysteresis) const {
    (void)hysteresis; // TODO: Implement hysteresis for threshold crossing
    // Simple threshold with hysteresis would require state tracking per parameter
    // For now, implement basic threshold
    return (velocity >= threshold) ? velocity : 0.0f;
}

float RelativeVelocityModulation::scaleAndOffset(float velocity, float scale, float offset) const {
    float scaled = velocity * scale + offset;
    return clampValue(scaled);
}

// System management
void RelativeVelocityModulation::setSampleRate(float sampleRate) {
    sampleRate_ = std::max(1000.0f, std::min(sampleRate, 192000.0f));
}

void RelativeVelocityModulation::reset() {
    velocityHistory_.clear();
    smoothedValues_.clear();
    peakHoldValues_.clear();
    envelopeStates_.clear();
    thresholdStates_.clear();
    lastUpdateTimes_.clear();
    processingTimes_.clear();
    totalSampleCount_ = 0;
}

// Performance monitoring
size_t RelativeVelocityModulation::getActiveParameterCount() const {
    return parameterConfigs_.size();
}

float RelativeVelocityModulation::getCPUUsageEstimate() const {
    if (!profilingEnabled_ || processingTimes_.empty()) {
        // Estimate based on parameter count and complexity
        size_t paramCount = getActiveParameterCount();
        return static_cast<float>(paramCount) * 0.0005f;  // ~0.05% per parameter
    }
    
    // Calculate from actual profiling data
    uint64_t totalTime = 0;
    for (const auto& [parameterId, time] : processingTimes_) {
        totalTime += time;
    }
    
    // Convert to percentage of CPU time
    float totalSeconds = static_cast<float>(totalTime) / 1e9f;  // nanoseconds to seconds
    float samplePeriod = 1.0f / sampleRate_;
    return (totalSeconds / samplePeriod) * 100.0f;
}

// Private utility methods
float RelativeVelocityModulation::clampValue(float value) const {
    return std::max(0.0f, std::min(value, 1.0f));
}

float RelativeVelocityModulation::normalizeVelocity(uint8_t velocity) const {
    return static_cast<float>(velocity) / 127.0f;
}

float RelativeVelocityModulation::interpolateLinear(float x, float y0, float y1) const {
    return y0 + x * (y1 - y0);
}

float RelativeVelocityModulation::calculateDeltaTime(uint32_t parameterId) const {
    auto it = lastUpdateTimes_.find(parameterId);
    if (it == lastUpdateTimes_.end()) {
        return 1.0f / sampleRate_;  // Default to one sample period
    }
    
    auto now = std::chrono::steady_clock::now();
    auto duration = now - it->second;
    return std::chrono::duration<float>(duration).count();
}

void RelativeVelocityModulation::updateVelocityHistory(uint32_t parameterId, float velocity) {
    const auto& config = getParameterConfig(parameterId);
    auto& history = velocityHistory_[parameterId];
    
    history.push_back(velocity);
    
    // Limit history length
    while (history.size() > static_cast<size_t>(config.historyLength)) {
        history.erase(history.begin());
    }
}

void RelativeVelocityModulation::updateEnvelopeState(uint32_t parameterId, float target, float deltaTime) {
    (void)parameterId; (void)target; (void)deltaTime;
    // This is handled in calculateEnvelopeModulation
}

// Batch operations
void RelativeVelocityModulation::updateAllModulations(const std::unordered_map<uint32_t, float>& baseValues, uint8_t velocity) {
    for (const auto& [parameterId, baseValue] : baseValues) {
        if (hasParameterConfig(parameterId)) {
            calculateModulation(parameterId, baseValue, velocity);
        }
    }
}

void RelativeVelocityModulation::resetAllSmoothing() {
    for (auto& [parameterId, smoothedValue] : smoothedValues_) {
        smoothedValue = 0.0f;
    }
    for (auto& [parameterId, peakValue] : peakHoldValues_) {
        peakValue = 0.0f;
    }
}

void RelativeVelocityModulation::clearAllHistory() {
    for (auto& [parameterId, history] : velocityHistory_) {
        history.clear();
    }
}