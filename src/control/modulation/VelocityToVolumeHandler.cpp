#include "VelocityToVolumeHandler.h"
#include "VelocityLatchSystem.h"
#include <algorithm>
#include <cmath>

// Static member definition
VelocityToVolumeHandler::VoiceVolumeOverride VelocityToVolumeHandler::defaultOverride_;

VelocityToVolumeHandler::VelocityToVolumeHandler() {
    globalConfig_ = VelocityVolumeConfig();
    latchSystem_ = nullptr;
    
    // Initialize default custom curve (linear)
    customCurvePoints_.resize(9);
    for (size_t i = 0; i < customCurvePoints_.size(); ++i) {
        customCurvePoints_[i] = static_cast<float>(i) / (customCurvePoints_.size() - 1);
    }
}

// Global configuration
void VelocityToVolumeHandler::setGlobalConfig(const VelocityVolumeConfig& config) {
    VelocityVolumeConfig clampedConfig = config;
    
    // Clamp all values to safe ranges
    clampedConfig.scale = std::max(MIN_SCALE, std::min(clampedConfig.scale, MAX_SCALE));
    clampedConfig.offset = std::max(MIN_OFFSET, std::min(clampedConfig.offset, MAX_OFFSET));
    clampedConfig.minVolume = std::max(0.0f, std::min(clampedConfig.minVolume, 1.0f));
    clampedConfig.maxVolume = std::max(0.0f, std::min(clampedConfig.maxVolume, 1.0f));
    clampedConfig.compensationAmount = std::max(0.0f, std::min(clampedConfig.compensationAmount, 1.0f));
    
    // Ensure min <= max
    if (clampedConfig.minVolume > clampedConfig.maxVolume) {
        std::swap(clampedConfig.minVolume, clampedConfig.maxVolume);
    }
    
    globalConfig_ = clampedConfig;
}

void VelocityToVolumeHandler::setEnabled(bool enabled) {
    globalConfig_.enabled = enabled;
}

void VelocityToVolumeHandler::setVelocityCurve(VelocityCurve curve) {
    globalConfig_.curve = curve;
}

void VelocityToVolumeHandler::setVelocityScale(float scale) {
    globalConfig_.scale = std::max(MIN_SCALE, std::min(scale, MAX_SCALE));
}

void VelocityToVolumeHandler::setVolumeRange(float minVolume, float maxVolume) {
    globalConfig_.minVolume = std::max(0.0f, std::min(minVolume, 1.0f));
    globalConfig_.maxVolume = std::max(0.0f, std::min(maxVolume, 1.0f));
    
    if (globalConfig_.minVolume > globalConfig_.maxVolume) {
        std::swap(globalConfig_.minVolume, globalConfig_.maxVolume);
    }
}

// Per-voice overrides
void VelocityToVolumeHandler::setVoiceOverride(uint32_t voiceId, const VoiceVolumeOverride& override) {
    VoiceVolumeOverride clampedOverride = override;
    clampedOverride.scaleOverride = std::max(MIN_SCALE, std::min(clampedOverride.scaleOverride, MAX_SCALE));
    
    voiceOverrides_[voiceId] = clampedOverride;
}

void VelocityToVolumeHandler::removeVoiceOverride(uint32_t voiceId) {
    voiceOverrides_.erase(voiceId);
}

bool VelocityToVolumeHandler::hasVoiceOverride(uint32_t voiceId) const {
    return voiceOverrides_.find(voiceId) != voiceOverrides_.end();
}

const VelocityToVolumeHandler::VoiceVolumeOverride& VelocityToVolumeHandler::getVoiceOverride(uint32_t voiceId) const {
    auto it = voiceOverrides_.find(voiceId);
    return (it != voiceOverrides_.end()) ? it->second : defaultOverride_;
}

// Volume calculation
float VelocityToVolumeHandler::calculateVolumeFromVelocity(float velocity, uint32_t voiceId) {
    // Clamp input velocity
    velocity = std::max(0.0f, std::min(velocity, 1.0f));
    
    // Check if velocity→volume is enabled
    bool enabled = globalConfig_.enabled;
    VelocityCurve curve = globalConfig_.curve;
    float scale = globalConfig_.scale;
    
    // Apply voice override if present
    if (voiceId != UINT32_MAX && hasVoiceOverride(voiceId)) {
        const auto& override = getVoiceOverride(voiceId);
        if (override.hasOverride) {
            enabled = override.enabledOverride;
            curve = override.curveOverride;
            scale = override.scaleOverride;
        }
    }
    
    if (!enabled) {
        // Return compensated volume when velocity→volume is disabled
        if (globalConfig_.compensateWhenDisabled) {
            return getCompensatedVolume(1.0f);
        } else {
            return 1.0f;  // Full volume
        }
    }
    
    // Apply velocity curve
    float curvedVelocity = applyVelocityCurve(velocity, curve);
    
    // Apply scaling and offset
    float volume = curvedVelocity * scale + globalConfig_.offset;
    
    // Apply volume range limits
    volume = globalConfig_.minVolume + volume * (globalConfig_.maxVolume - globalConfig_.minVolume);
    
    // Final clamping
    volume = clampVolume(volume);
    
    // Notify callback if registered
    notifyVolumeChange(voiceId, velocity, volume);
    
    return volume;
}

float VelocityToVolumeHandler::applyVelocityCurve(float velocity, VelocityCurve curve) {
    switch (curve) {
        case VelocityCurve::LINEAR:
            return applyLinearCurve(velocity);
        case VelocityCurve::EXPONENTIAL:
            return applyExponentialCurve(velocity);
        case VelocityCurve::LOGARITHMIC:
            return applyLogarithmicCurve(velocity);
        case VelocityCurve::S_CURVE:
            return applySCurve(velocity);
        case VelocityCurve::CUSTOM:
            return applyCustomCurve(velocity);
    }
    return velocity;  // Fallback to linear
}

float VelocityToVolumeHandler::getCompensatedVolume(float baseVolume) {
    return std::min(1.0f, baseVolume + globalConfig_.compensationAmount);
}

// Integration with velocity modulation system
void VelocityToVolumeHandler::integrateWithVelocitySystem(VelocityLatchSystem* latchSystem) {
    latchSystem_ = latchSystem;
}

void VelocityToVolumeHandler::updateVolumeModulation(uint32_t voiceId, float velocity, float& volumeOutput) {
    volumeOutput = calculateVolumeFromVelocity(velocity, voiceId);
}

// Curve customization
void VelocityToVolumeHandler::setCustomCurvePoints(const std::vector<float>& curvePoints) {
    if (curvePoints.size() <= MAX_CUSTOM_CURVE_POINTS) {
        customCurvePoints_ = curvePoints;
        
        // Ensure curve points are sorted and clamped
        for (auto& point : customCurvePoints_) {
            point = std::max(0.0f, std::min(point, 1.0f));
        }
    }
}

// System management
void VelocityToVolumeHandler::reset() {
    globalConfig_ = VelocityVolumeConfig();
    voiceOverrides_.clear();
    
    // Reset custom curve to linear
    customCurvePoints_.resize(9);
    for (size_t i = 0; i < customCurvePoints_.size(); ++i) {
        customCurvePoints_[i] = static_cast<float>(i) / (customCurvePoints_.size() - 1);
    }
}

void VelocityToVolumeHandler::clearAllVoiceOverrides() {
    voiceOverrides_.clear();
}

// Statistics and monitoring
float VelocityToVolumeHandler::getAverageVolumeScale() const {
    if (voiceOverrides_.empty()) {
        return globalConfig_.scale;
    }
    
    float sum = globalConfig_.scale;
    size_t count = 1;
    
    for (const auto& [voiceId, override] : voiceOverrides_) {
        if (override.hasOverride) {
            sum += override.scaleOverride;
            count++;
        }
    }
    
    return sum / static_cast<float>(count);
}

bool VelocityToVolumeHandler::isVelocityToVolumeActive() const {
    return globalConfig_.enabled;
}

void VelocityToVolumeHandler::setVolumeChangeCallback(VolumeChangeCallback callback) {
    volumeChangeCallback_ = callback;
}

// Internal curve calculation methods
float VelocityToVolumeHandler::applyLinearCurve(float velocity) {
    return velocity;  // Direct linear mapping
}

float VelocityToVolumeHandler::applyExponentialCurve(float velocity) {
    // Exponential curve for perceived loudness (square law)
    return velocity * velocity;
}

float VelocityToVolumeHandler::applyLogarithmicCurve(float velocity) {
    // Logarithmic curve for gentle response
    if (velocity <= 0.0f) return 0.0f;
    return std::log(1.0f + velocity * 9.0f) / std::log(10.0f);  // log10(1 + 9x)
}

float VelocityToVolumeHandler::applySCurve(float velocity) {
    // S-curve using tanh for balanced response
    float scaled = (velocity - 0.5f) * 6.0f;  // Scale to -3 to +3
    return 0.5f + 0.5f * std::tanh(scaled);
}

float VelocityToVolumeHandler::applyCustomCurve(float velocity) {
    return interpolateCustomCurve(velocity);
}

// Helper methods
float VelocityToVolumeHandler::clampVolume(float volume) {
    return std::max(0.0f, std::min(volume, 1.0f));
}

float VelocityToVolumeHandler::interpolateCustomCurve(float velocity) {
    if (customCurvePoints_.empty()) {
        return velocity;  // Fallback to linear
    }
    
    if (customCurvePoints_.size() == 1) {
        return customCurvePoints_[0];
    }
    
    // Find interpolation points
    float scaledVel = velocity * (customCurvePoints_.size() - 1);
    size_t index = static_cast<size_t>(scaledVel);
    
    if (index >= customCurvePoints_.size() - 1) {
        return customCurvePoints_.back();
    }
    
    float fraction = scaledVel - index;
    return customCurvePoints_[index] + fraction * (customCurvePoints_[index + 1] - customCurvePoints_[index]);
}

void VelocityToVolumeHandler::notifyVolumeChange(uint32_t voiceId, float velocity, float volume) {
    if (volumeChangeCallback_) {
        volumeChangeCallback_(voiceId, velocity, volume);
    }
}