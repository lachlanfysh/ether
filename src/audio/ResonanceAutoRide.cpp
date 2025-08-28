#include "ResonanceAutoRide.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ResonanceAutoRide::ResonanceAutoRide() {
    config_ = Config(); // Use default configuration
    initialized_ = false;
    
    currentAutoResonance_ = 0.0f;
    currentCutoffOpening_ = 0.0f;
    effectiveResonance_ = config_.minResonance;
    effectiveCutoff_ = config_.minCutoffHz;
}

bool ResonanceAutoRide::initialize(const Config& config) {
    // Validate configuration
    if (config.minCutoffHz >= config.maxCutoffHz ||
        config.minResonance >= config.maxResonance ||
        config.autoRideAmount < 0.0f || config.autoRideAmount > 1.0f ||
        config.cutoffOpeningAmount < 0.0f || config.cutoffOpeningAmount > 1.0f) {
        return false;
    }
    
    config_ = config;
    reset();
    initialized_ = true;
    return true;
}

void ResonanceAutoRide::shutdown() {
    if (!initialized_) {
        return;
    }
    
    reset();
    initialized_ = false;
}

void ResonanceAutoRide::reset() {
    currentAutoResonance_ = 0.0f;
    currentCutoffOpening_ = 0.0f;
    effectiveResonance_ = config_.minResonance;
    effectiveCutoff_ = config_.minCutoffHz;
}

float ResonanceAutoRide::processResonance(float baseCutoffHz, float baseResonance) {
    if (!initialized_ || !config_.enabled) {
        effectiveResonance_ = baseResonance;
        return baseResonance;
    }
    
    // Clamp input cutoff to valid range
    float clampedCutoff = clamp(baseCutoffHz, config_.minCutoffHz, config_.maxCutoffHz);
    
    // Calculate normalized cutoff position (0 = min, 1 = max)
    float normalizedCutoff = normalizeFrequency(clampedCutoff);
    
    // Calculate auto-ride resonance based on curve type
    float autoResonance = 0.0f;
    
    switch (config_.curveType) {
        case CurveType::EXPONENTIAL:
            autoResonance = exponentialCurve(normalizedCutoff);
            break;
        case CurveType::LOGARITHMIC:
            autoResonance = logarithmicCurve(normalizedCutoff);
            break;
        case CurveType::S_CURVE:
            autoResonance = sCurve(normalizedCutoff);
            break;
        case CurveType::LINEAR:
            autoResonance = linearCurve(normalizedCutoff);
            break;
        case CurveType::CUSTOM:
            autoResonance = customCurve(normalizedCutoff);
            break;
    }
    
    // Scale by auto-ride amount
    autoResonance *= config_.autoRideAmount;
    
    // Map to additional resonance amount (0 to max-min range)
    float additionalResonance = autoResonance * (config_.maxResonance - config_.minResonance);
    
    // Add to base resonance
    float blendedResonance = baseResonance + additionalResonance;
    
    // Smooth the result to avoid zipper noise
    currentAutoResonance_ = currentAutoResonance_ * SMOOTHING_FACTOR + 
                           additionalResonance * (1.0f - SMOOTHING_FACTOR);
    
    effectiveResonance_ = clamp(blendedResonance, config_.minResonance, config_.maxResonance);
    
    return effectiveResonance_;
}

float ResonanceAutoRide::processCutoffOpening(float baseCutoffHz, float targetResonance) {
    if (!initialized_ || !config_.enabled || config_.cutoffOpeningAmount == 0.0f) {
        effectiveCutoff_ = baseCutoffHz;
        return baseCutoffHz;
    }
    
    // Calculate how much to open the cutoff based on target resonance
    float normalizedResonance = mapRange(targetResonance, config_.minResonance, config_.maxResonance, 0.0f, 1.0f);
    
    // Higher resonance = more cutoff opening
    float openingAmount = normalizedResonance * config_.cutoffOpeningAmount;
    
    // Calculate cutoff boost (exponential for musical feel)
    float cutoffMultiplier = 1.0f + (openingAmount * openingAmount * 2.0f); // Up to 3x boost
    
    float openedCutoff = baseCutoffHz * cutoffMultiplier;
    
    // Smooth the result
    currentCutoffOpening_ = currentCutoffOpening_ * SMOOTHING_FACTOR + 
                           openingAmount * (1.0f - SMOOTHING_FACTOR);
    
    effectiveCutoff_ = clamp(openedCutoff, config_.minCutoffHz, config_.maxCutoffHz);
    
    return effectiveCutoff_;
}

void ResonanceAutoRide::setConfig(const Config& config) {
    if (config.minCutoffHz < config.maxCutoffHz &&
        config.minResonance < config.maxResonance &&
        config.autoRideAmount >= 0.0f && config.autoRideAmount <= 1.0f &&
        config.cutoffOpeningAmount >= 0.0f && config.cutoffOpeningAmount <= 1.0f) {
        config_ = config;
    }
}

void ResonanceAutoRide::setAutoRideAmount(float amount) {
    config_.autoRideAmount = clamp(amount, 0.0f, 1.0f);
}

void ResonanceAutoRide::setCutoffOpeningAmount(float amount) {
    config_.cutoffOpeningAmount = clamp(amount, 0.0f, 1.0f);
}

void ResonanceAutoRide::setCurveType(CurveType type) {
    config_.curveType = type;
}

void ResonanceAutoRide::setEnabled(bool enabled) {
    config_.enabled = enabled;
}

// Static utility functions

float ResonanceAutoRide::calculateAutoRideResonance(float cutoffHz, const Config& config) {
    if (!config.enabled) {
        return config.minResonance;
    }
    
    float clampedCutoff = std::max(config.minCutoffHz, std::min(config.maxCutoffHz, cutoffHz));
    float normalizedCutoff = (clampedCutoff - config.minCutoffHz) / (config.maxCutoffHz - config.minCutoffHz);
    
    // Use exponential curve as default for static calculation
    float curve = 1.0f - std::exp(-3.0f * (1.0f - normalizedCutoff));
    curve *= config.autoRideAmount;
    
    // Return additional resonance amount (to be added to base)
    return curve * (config.maxResonance - config.minResonance);
}

float ResonanceAutoRide::calculateCutoffOpening(float targetResonance, const Config& config) {
    if (!config.enabled || config.cutoffOpeningAmount == 0.0f) {
        return 1.0f; // No opening
    }
    
    float normalizedResonance = (targetResonance - config.minResonance) / 
                               (config.maxResonance - config.minResonance);
    normalizedResonance = std::max(0.0f, std::min(1.0f, normalizedResonance));
    
    float openingAmount = normalizedResonance * config.cutoffOpeningAmount;
    return 1.0f + (openingAmount * openingAmount * 2.0f); // Exponential opening
}

// Private methods

float ResonanceAutoRide::exponentialCurve(float normalizedCutoff) const {
    // Exponential curve: more resonance at lower cutoffs
    // f(x) = 1 - e^(-k * (1-x)) where k controls steepness
    const float steepness = 3.0f; // Tuned for musical response
    return 1.0f - std::exp(-steepness * (1.0f - normalizedCutoff));
}

float ResonanceAutoRide::logarithmicCurve(float normalizedCutoff) const {
    // Logarithmic curve: gentler response
    // f(x) = log(1 + k * (1-x)) / log(1 + k)
    const float steepness = 9.0f; // log base adjustment
    return std::log(1.0f + steepness * (1.0f - normalizedCutoff)) / std::log(1.0f + steepness);
}

float ResonanceAutoRide::sCurve(float normalizedCutoff) const {
    // S-curve using smoothstep: smooth transitions
    float inverted = 1.0f - normalizedCutoff;
    return inverted * inverted * (3.0f - 2.0f * inverted);
}

float ResonanceAutoRide::linearCurve(float normalizedCutoff) const {
    // Simple linear relationship (inverted)
    return 1.0f - normalizedCutoff;
}

float ResonanceAutoRide::customCurve(float normalizedCutoff) const {
    // Custom curve - could be user-configurable in the future
    // For now, use a combination of exponential and S-curve
    float expo = exponentialCurve(normalizedCutoff);
    float smooth = sCurve(normalizedCutoff);
    return 0.7f * expo + 0.3f * smooth; // Blend for musical character
}

float ResonanceAutoRide::clamp(float value, float min, float max) const {
    return std::max(min, std::min(max, value));
}

float ResonanceAutoRide::mapRange(float value, float inMin, float inMax, float outMin, float outMax) const {
    if (inMax == inMin) return outMin; // Avoid division by zero
    
    float normalized = (value - inMin) / (inMax - inMin);
    normalized = clamp(normalized, 0.0f, 1.0f);
    return outMin + normalized * (outMax - outMin);
}

float ResonanceAutoRide::normalizeFrequency(float frequency) const {
    return mapRange(frequency, config_.minCutoffHz, config_.maxCutoffHz, 0.0f, 1.0f);
}