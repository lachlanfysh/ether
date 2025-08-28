#include "VelocityVolumeControl.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <chrono>

VelocityVolumeControl::VelocityVolumeControl() {
    enabled_ = true;
    sampleRate_ = DEFAULT_SAMPLE_RATE;
    globalConfig_ = VolumeConfig();
    
    // Initialize with default linear curve table
    generateCurveTable(VolumeCurveType::LINEAR, 1.0f, DEFAULT_CURVE_TABLE_SIZE);
}

// Global velocity→volume control
void VelocityVolumeControl::setGlobalVelocityToVolumeEnabled(bool enabled) {
    globalConfig_.enableVelocityToVolume = enabled;
    
    // Update all voices if velocity→volume is disabled
    if (!enabled) {
        for (auto& [voiceId, state] : voiceStates_) {
            if (!state.volumeOverridden) {
                // Set to maximum volume when velocity→volume is disabled
                float oldVolume = state.calculatedVolume;
                state.calculatedVolume = globalConfig_.volumeMax;
                state.smoothedVolume = state.calculatedVolume;
                notifyVolumeChange(voiceId, oldVolume, state.calculatedVolume);
            }
        }
    }
}

void VelocityVolumeControl::setGlobalVolumeConfig(const VolumeConfig& config) {
    VolumeConfig clampedConfig = config;
    
    // Clamp values to safe ranges
    clampedConfig.curveAmount = std::max(MIN_CURVE_AMOUNT, std::min(clampedConfig.curveAmount, MAX_CURVE_AMOUNT));
    clampedConfig.velocityScale = std::max(0.0f, std::min(clampedConfig.velocityScale, 2.0f));
    clampedConfig.velocityOffset = std::max(-1.0f, std::min(clampedConfig.velocityOffset, 1.0f));
    clampedConfig.volumeMin = std::max(MIN_VOLUME, std::min(clampedConfig.volumeMin, MAX_VOLUME));
    clampedConfig.volumeMax = std::max(MIN_VOLUME, std::min(clampedConfig.volumeMax, MAX_VOLUME));
    clampedConfig.volumeRange = std::max(0.0f, std::min(clampedConfig.volumeRange, 1.0f));
    clampedConfig.smoothingTime = std::max(MIN_SMOOTHING_TIME, std::min(clampedConfig.smoothingTime, MAX_SMOOTHING_TIME));
    
    // Ensure min <= max
    if (clampedConfig.volumeMin > clampedConfig.volumeMax) {
        std::swap(clampedConfig.volumeMin, clampedConfig.volumeMax);
    }
    
    globalConfig_ = clampedConfig;
}

// Per-engine configuration
void VelocityVolumeControl::setEngineVolumeConfig(uint32_t engineId, const VolumeConfig& config) {
    VolumeConfig clampedConfig = config;
    
    // Apply same clamping as global config
    clampedConfig.curveAmount = std::max(MIN_CURVE_AMOUNT, std::min(clampedConfig.curveAmount, MAX_CURVE_AMOUNT));
    clampedConfig.velocityScale = std::max(0.0f, std::min(clampedConfig.velocityScale, 2.0f));
    clampedConfig.velocityOffset = std::max(-1.0f, std::min(clampedConfig.velocityOffset, 1.0f));
    clampedConfig.volumeMin = std::max(MIN_VOLUME, std::min(clampedConfig.volumeMin, MAX_VOLUME));
    clampedConfig.volumeMax = std::max(MIN_VOLUME, std::min(clampedConfig.volumeMax, MAX_VOLUME));
    clampedConfig.volumeRange = std::max(0.0f, std::min(clampedConfig.volumeRange, 1.0f));
    clampedConfig.smoothingTime = std::max(MIN_SMOOTHING_TIME, std::min(clampedConfig.smoothingTime, MAX_SMOOTHING_TIME));
    
    if (clampedConfig.volumeMin > clampedConfig.volumeMax) {
        std::swap(clampedConfig.volumeMin, clampedConfig.volumeMax);
    }
    
    engineConfigs_[engineId] = clampedConfig;
}

const VelocityVolumeControl::VolumeConfig& VelocityVolumeControl::getEngineVolumeConfig(uint32_t engineId) const {
    auto it = engineConfigs_.find(engineId);
    return (it != engineConfigs_.end()) ? it->second : globalConfig_;
}

bool VelocityVolumeControl::hasEngineVolumeConfig(uint32_t engineId) const {
    return engineConfigs_.find(engineId) != engineConfigs_.end();
}

void VelocityVolumeControl::removeEngineVolumeConfig(uint32_t engineId) {
    engineConfigs_.erase(engineId);
}

// Volume calculation
VelocityVolumeControl::VolumeResult VelocityVolumeControl::calculateVolume(uint32_t voiceId, uint8_t velocity, uint32_t engineId) {
    VolumeResult result;
    
    if (!enabled_) {
        result.volume = MAX_VOLUME;
        return result;
    }
    
    const VolumeConfig& config = getEffectiveConfig(engineId);
    
    // Check if velocity→volume is disabled
    if (!config.enableVelocityToVolume) {
        result.volume = config.volumeMax;
        result.velocityComponent = 0.0f;
        result.baseComponent = config.volumeMax;
        return result;
    }
    
    // Calculate direct volume from velocity
    float directVolume = calculateDirectVolume(velocity, config);
    result.velocityComponent = directVolume;
    result.appliedCurve = config.curveType;
    
    // Apply smoothing if enabled and voice exists
    auto voiceIt = voiceStates_.find(voiceId);
    if (voiceIt != voiceStates_.end() && config.smoothingTime > 0.0f) {
        float deltaTime = 1.0f / sampleRate_; // Assume single sample for now
        float smoothedVolume = calculateSmoothedVolume(voiceId, directVolume, deltaTime);
        result.volume = smoothedVolume;
        result.wasSmoothed = (std::abs(smoothedVolume - directVolume) > 0.001f);
    } else {
        result.volume = directVolume;
    }
    
    // Check if volume was limited by range
    float unlimitedVolume = result.volume;
    result.volume = applyVolumeRange(result.volume, config);
    result.wasLimited = (std::abs(result.volume - unlimitedVolume) > 0.001f);
    
    // Update voice state if it exists
    if (voiceIt != voiceStates_.end()) {
        float oldVolume = voiceIt->second.calculatedVolume;
        voiceIt->second.originalVelocity = velocity;
        voiceIt->second.processedVelocity = normalizeVelocity(velocity);
        voiceIt->second.calculatedVolume = result.volume;
        voiceIt->second.smoothedVolume = result.volume;
        voiceIt->second.lastUpdateTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        
        notifyVolumeChange(voiceId, oldVolume, result.volume);
    }
    
    return result;
}

float VelocityVolumeControl::calculateDirectVolume(uint8_t velocity, const VolumeConfig& config) const {
    if (!config.enableVelocityToVolume) {
        return config.volumeMax;
    }
    
    // Normalize and scale velocity
    float normalizedVel = normalizeVelocity(velocity);
    float scaledVel = scaleAndOffsetVelocity(normalizedVel, config);
    
    // Apply inversion if enabled
    if (config.invertVelocity) {
        scaledVel = 1.0f - scaledVel;
    }
    
    // Apply velocity curve
    float curvedVel = applyCurve(scaledVel, config.curveType, config.curveAmount);
    
    // Map to volume range
    float volume = config.volumeMin + curvedVel * (config.volumeMax - config.volumeMin);
    
    // Apply dynamic range compression if configured
    if (config.volumeRange < 1.0f) {
        float center = (config.volumeMin + config.volumeMax) * 0.5f;
        float deviation = volume - center;
        volume = center + deviation * config.volumeRange;
    }
    
    return std::max(MIN_VOLUME, std::min(volume, MAX_VOLUME));
}

float VelocityVolumeControl::calculateSmoothedVolume(uint32_t voiceId, float targetVolume, float deltaTime) {
    auto voiceIt = voiceStates_.find(voiceId);
    if (voiceIt == voiceStates_.end()) {
        return targetVolume;
    }
    
    const VolumeConfig& config = getEffectiveConfig(0); // Use global config for smoothing
    if (config.smoothingTime <= 0.0f) {
        return targetVolume;
    }
    
    float& currentVolume = voiceIt->second.smoothedVolume;
    float smoothingRate = 1.0f / (config.smoothingTime / 1000.0f); // Convert ms to seconds
    float alpha = 1.0f - std::exp(-deltaTime * smoothingRate);
    
    currentVolume = currentVolume + alpha * (targetVolume - currentVolume);
    return currentVolume;
}

// Voice management
void VelocityVolumeControl::addVoice(uint32_t voiceId, uint8_t velocity, uint32_t engineId) {
    VoiceVolumeState state;
    state.voiceId = voiceId;
    state.originalVelocity = velocity;
    state.processedVelocity = normalizeVelocity(velocity);
    
    // Calculate initial volume
    const VolumeConfig& config = getEffectiveConfig(engineId);
    state.calculatedVolume = calculateDirectVolume(velocity, config);
    state.smoothedVolume = state.calculatedVolume;
    state.lastUpdateTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    voiceStates_[voiceId] = state;
}

void VelocityVolumeControl::updateVoiceVelocity(uint32_t voiceId, uint8_t newVelocity) {
    auto voiceIt = voiceStates_.find(voiceId);
    if (voiceIt == voiceStates_.end()) {
        return;
    }
    
    float oldVolume = voiceIt->second.calculatedVolume;
    voiceIt->second.originalVelocity = newVelocity;
    voiceIt->second.processedVelocity = normalizeVelocity(newVelocity);
    
    // Recalculate volume based on new velocity
    const VolumeConfig& config = getEffectiveConfig(0); // Use global config
    voiceIt->second.calculatedVolume = calculateDirectVolume(newVelocity, config);
    voiceIt->second.smoothedVolume = voiceIt->second.calculatedVolume; // Update smoothed volume too
    
    notifyVolumeChange(voiceId, oldVolume, voiceIt->second.calculatedVolume);
}

void VelocityVolumeControl::removeVoice(uint32_t voiceId) {
    voiceStates_.erase(voiceId);
}

void VelocityVolumeControl::clearAllVoices() {
    voiceStates_.clear();
}

// Voice volume overrides
void VelocityVolumeControl::setVoiceVolumeOverride(uint32_t voiceId, float volume) {
    auto voiceIt = voiceStates_.find(voiceId);
    if (voiceIt == voiceStates_.end()) {
        return;
    }
    
    float oldVolume = voiceIt->second.volumeOverridden ? 
        voiceIt->second.overrideVolume : voiceIt->second.calculatedVolume;
    
    voiceIt->second.volumeOverridden = true;
    voiceIt->second.overrideVolume = std::max(MIN_VOLUME, std::min(volume, MAX_VOLUME));
    
    notifyVolumeChange(voiceId, oldVolume, voiceIt->second.overrideVolume);
}

void VelocityVolumeControl::clearVoiceVolumeOverride(uint32_t voiceId) {
    auto voiceIt = voiceStates_.find(voiceId);
    if (voiceIt == voiceStates_.end() || !voiceIt->second.volumeOverridden) {
        return;
    }
    
    float oldVolume = voiceIt->second.overrideVolume;
    voiceIt->second.volumeOverridden = false;
    
    notifyVolumeChange(voiceId, oldVolume, voiceIt->second.calculatedVolume);
}

bool VelocityVolumeControl::hasVoiceVolumeOverride(uint32_t voiceId) const {
    auto voiceIt = voiceStates_.find(voiceId);
    return (voiceIt != voiceStates_.end()) ? voiceIt->second.volumeOverridden : false;
}

float VelocityVolumeControl::getVoiceVolume(uint32_t voiceId) const {
    auto voiceIt = voiceStates_.find(voiceId);
    if (voiceIt == voiceStates_.end()) {
        return MAX_VOLUME;
    }
    
    return voiceIt->second.volumeOverridden ? 
        voiceIt->second.overrideVolume : voiceIt->second.smoothedVolume;
}

// Velocity curve processing
float VelocityVolumeControl::applyCurve(float velocity, VolumeCurveType curveType, float curveAmount) const {
    switch (curveType) {
        case VolumeCurveType::LINEAR:
            return applyLinearCurve(velocity);
        case VolumeCurveType::EXPONENTIAL:
            return applyExponentialCurve(velocity, curveAmount);
        case VolumeCurveType::LOGARITHMIC:
            return applyLogarithmicCurve(velocity, curveAmount);
        case VolumeCurveType::S_CURVE:
            return applySCurve(velocity, curveAmount);
        case VolumeCurveType::POWER_LAW:
            return applyPowerLawCurve(velocity, curveAmount);
        case VolumeCurveType::STEPPED:
            return applySteppedCurve(velocity, static_cast<int>(curveAmount));
        case VolumeCurveType::CUSTOM_TABLE:
            return applyCustomTableCurve(velocity, customCurveTable_);
    }
    return velocity;
}

float VelocityVolumeControl::applyLinearCurve(float velocity) const {
    return velocity;
}

float VelocityVolumeControl::applyExponentialCurve(float velocity, float amount) const {
    float clampedAmount = std::max(MIN_CURVE_AMOUNT, std::min(amount, MAX_CURVE_AMOUNT));
    return std::pow(velocity, 1.0f / clampedAmount);
}

float VelocityVolumeControl::applyLogarithmicCurve(float velocity, float amount) const {
    float clampedAmount = std::max(MIN_CURVE_AMOUNT, std::min(amount, MAX_CURVE_AMOUNT));
    return std::pow(velocity, clampedAmount);
}

float VelocityVolumeControl::applySCurve(float velocity, float amount) const {
    float clampedAmount = std::max(MIN_CURVE_AMOUNT, std::min(amount, MAX_CURVE_AMOUNT));
    float x = velocity * 2.0f - 1.0f;  // Map to -1 to +1
    float curved = std::tanh(x * clampedAmount) / std::tanh(clampedAmount);
    return (curved + 1.0f) * 0.5f;  // Map back to 0 to 1
}

float VelocityVolumeControl::applyPowerLawCurve(float velocity, float exponent) const {
    float clampedExponent = std::max(MIN_CURVE_AMOUNT, std::min(exponent, MAX_CURVE_AMOUNT));
    return std::pow(velocity, clampedExponent);
}

float VelocityVolumeControl::applySteppedCurve(float velocity, int steps) const {
    int clampedSteps = std::max(2, std::min(steps, 32));
    float stepSize = 1.0f / static_cast<float>(clampedSteps - 1);
    int stepIndex = static_cast<int>(velocity * (clampedSteps - 1) + 0.5f);  // Round to nearest
    return stepIndex * stepSize;
}

float VelocityVolumeControl::applyCustomTableCurve(float velocity, const std::vector<float>& table) const {
    if (table.empty()) {
        return velocity;
    }
    
    float index = velocity * (table.size() - 1);
    return interpolateTableValue(index, table);
}

// Curve modification
void VelocityVolumeControl::setCustomCurveTable(const std::vector<float>& table) {
    customCurveTable_ = table;
    
    // Clamp all values to valid range
    for (float& value : customCurveTable_) {
        value = std::max(MIN_VELOCITY, std::min(value, MAX_VELOCITY));
    }
}

void VelocityVolumeControl::generateCurveTable(VolumeCurveType type, float amount, size_t tableSize) {
    customCurveTable_.clear();
    customCurveTable_.reserve(tableSize);
    
    for (size_t i = 0; i < tableSize; ++i) {
        float velocity = static_cast<float>(i) / static_cast<float>(tableSize - 1);
        float curvedValue = applyCurve(velocity, type, amount);
        customCurveTable_.push_back(curvedValue);
    }
}

// System management
void VelocityVolumeControl::reset() {
    voiceStates_.clear();
    engineConfigs_.clear();
    globalConfig_ = VolumeConfig();
    generateCurveTable(VolumeCurveType::LINEAR, 1.0f, DEFAULT_CURVE_TABLE_SIZE);
}

// Performance monitoring
void VelocityVolumeControl::updateAllVoices(float deltaTime) {
    for (auto& [voiceId, state] : voiceStates_) {
        if (!state.volumeOverridden) {
            const VolumeConfig& config = getEffectiveConfig(0);
            if (config.smoothingTime > 0.0f) {
                calculateSmoothedVolume(voiceId, state.calculatedVolume, deltaTime);
            }
        }
    }
}

size_t VelocityVolumeControl::getActiveVoiceCount() const {
    return voiceStates_.size();
}

float VelocityVolumeControl::getAverageVolume() const {
    if (voiceStates_.empty()) {
        return 0.0f;
    }
    
    float sum = 0.0f;
    for (const auto& [voiceId, state] : voiceStates_) {
        sum += getVoiceVolume(voiceId);
    }
    
    return sum / static_cast<float>(voiceStates_.size());
}

size_t VelocityVolumeControl::getVoicesWithOverrides() const {
    size_t count = 0;
    for (const auto& [voiceId, state] : voiceStates_) {
        if (state.volumeOverridden) {
            count++;
        }
    }
    return count;
}

// Batch operations
void VelocityVolumeControl::setAllVoicesVolume(float volume) {
    float clampedVolume = std::max(MIN_VOLUME, std::min(volume, MAX_VOLUME));
    
    for (auto& [voiceId, state] : voiceStates_) {
        float oldVolume = getVoiceVolume(voiceId);
        state.volumeOverridden = true;
        state.overrideVolume = clampedVolume;
        notifyVolumeChange(voiceId, oldVolume, clampedVolume);
    }
}

void VelocityVolumeControl::updateAllVoicesSmoothing(float deltaTime) {
    updateAllVoices(deltaTime);
}

void VelocityVolumeControl::applyGlobalVolumeScale(float scale) {
    float clampedScale = std::max(0.0f, std::min(scale, 2.0f));
    
    for (auto& [voiceId, state] : voiceStates_) {
        float oldVolume = getVoiceVolume(voiceId);
        
        if (state.volumeOverridden) {
            state.overrideVolume = std::max(MIN_VOLUME, std::min(state.overrideVolume * clampedScale, MAX_VOLUME));
            notifyVolumeChange(voiceId, oldVolume, state.overrideVolume);
        } else {
            state.calculatedVolume = std::max(MIN_VOLUME, std::min(state.calculatedVolume * clampedScale, MAX_VOLUME));
            state.smoothedVolume = state.calculatedVolume;
            notifyVolumeChange(voiceId, oldVolume, state.calculatedVolume);
        }
    }
}

void VelocityVolumeControl::resetAllVoicesToVelocityVolume() {
    for (auto& [voiceId, state] : voiceStates_) {
        if (state.volumeOverridden) {
            float oldVolume = state.overrideVolume;
            state.volumeOverridden = false;
            notifyVolumeChange(voiceId, oldVolume, state.calculatedVolume);
        }
    }
}

void VelocityVolumeControl::setVolumeChangeCallback(VolumeChangeCallback callback) {
    volumeChangeCallback_ = callback;
}

// Debugging and analysis
std::vector<uint32_t> VelocityVolumeControl::getActiveVoiceIds() const {
    std::vector<uint32_t> voiceIds;
    voiceIds.reserve(voiceStates_.size());
    
    for (const auto& [voiceId, state] : voiceStates_) {
        voiceIds.push_back(voiceId);
    }
    
    return voiceIds;
}

VelocityVolumeControl::VoiceVolumeState VelocityVolumeControl::getVoiceState(uint32_t voiceId) const {
    auto voiceIt = voiceStates_.find(voiceId);
    return (voiceIt != voiceStates_.end()) ? voiceIt->second : VoiceVolumeState();
}

void VelocityVolumeControl::dumpVoiceStates() const {
    std::cout << "=== VelocityVolumeControl Voice States ===\n";
    std::cout << "Active voices: " << voiceStates_.size() << "\n";
    
    for (const auto& [voiceId, state] : voiceStates_) {
        std::cout << "Voice " << voiceId << ": ";
        std::cout << "vel=" << static_cast<int>(state.originalVelocity);
        std::cout << " vol=" << state.calculatedVolume;
        if (state.volumeOverridden) {
            std::cout << " (overridden to " << state.overrideVolume << ")";
        }
        std::cout << "\n";
    }
}

// Private utility methods
float VelocityVolumeControl::normalizeVelocity(uint8_t velocity) const {
    return static_cast<float>(velocity) / 127.0f;
}

float VelocityVolumeControl::scaleAndOffsetVelocity(float velocity, const VolumeConfig& config) const {
    float scaled = velocity * config.velocityScale + config.velocityOffset;
    return std::max(MIN_VELOCITY, std::min(scaled, MAX_VELOCITY));
}

float VelocityVolumeControl::applyVolumeRange(float volume, const VolumeConfig& config) const {
    return std::max(config.volumeMin, std::min(volume, config.volumeMax));
}

void VelocityVolumeControl::notifyVolumeChange(uint32_t voiceId, float oldVolume, float newVolume) {
    if (volumeChangeCallback_ && std::abs(oldVolume - newVolume) > 0.001f) {
        volumeChangeCallback_(voiceId, oldVolume, newVolume);
    }
}

float VelocityVolumeControl::interpolateTableValue(float index, const std::vector<float>& table) const {
    if (table.empty()) {
        return 0.0f;
    }
    
    if (index <= 0.0f) {
        return table[0];
    }
    
    if (index >= static_cast<float>(table.size() - 1)) {
        return table[table.size() - 1];
    }
    
    size_t lowerIndex = static_cast<size_t>(index);
    size_t upperIndex = lowerIndex + 1;
    float fraction = index - static_cast<float>(lowerIndex);
    
    return table[lowerIndex] + fraction * (table[upperIndex] - table[lowerIndex]);
}

const VelocityVolumeControl::VolumeConfig& VelocityVolumeControl::getEffectiveConfig(uint32_t engineId) const {
    auto it = engineConfigs_.find(engineId);
    return (it != engineConfigs_.end()) ? it->second : globalConfig_;
}