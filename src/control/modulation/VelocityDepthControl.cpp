#include "VelocityDepthControl.h"
#include "VelocityLatchSystem.h"
#include "VelocityParameterScaling.h"
#include <algorithm>
#include <cmath>

// Static member definitions
VelocityDepthControl::ParameterDepthConfig VelocityDepthControl::defaultParameterConfig_;
VelocityDepthControl::GlobalDepthConfig VelocityDepthControl::defaultGlobalConfig_;
std::unordered_map<uint32_t, VelocityDepthControl::SafetyLimits> VelocityDepthControl::parameterSafetyLimits_;

VelocityDepthControl::VelocityDepthControl() {
    enabled_ = true;
    globalConfig_ = GlobalDepthConfig();
    latchSystem_ = nullptr;
    scalingSystem_ = nullptr;
    initializeDepthPresets();
}

// Global depth control
void VelocityDepthControl::setMasterDepth(float depth) {
    float clampedDepth = std::max(MIN_DEPTH, std::min(depth, globalConfig_.maxGlobalDepth));
    float oldDepth = globalConfig_.masterDepth;
    globalConfig_.masterDepth = clampedDepth;
    
    // Update all linked parameters
    if (globalConfig_.enableMasterDepthControl) {
        for (auto& [parameterId, config] : parameterConfigs_) {
            if (config.linkToMasterDepth) {
                float newParameterDepth = clampedDepth * config.masterDepthScale;
                setParameterBaseDepth(parameterId, newParameterDepth);
            }
        }
    }
    
    // Notify depth change for master
    if (depthChangeCallback_) {
        depthChangeCallback_(0, oldDepth, clampedDepth);  // Use ID 0 for master
    }
}

void VelocityDepthControl::setGlobalConfig(const GlobalDepthConfig& config) {
    globalConfig_ = config;
    
    // Clamp values to safe ranges
    globalConfig_.masterDepth = std::max(MIN_DEPTH, std::min(globalConfig_.masterDepth, MAX_DEPTH));
    globalConfig_.maxGlobalDepth = std::max(MIN_DEPTH, std::min(globalConfig_.maxGlobalDepth, MAX_DEPTH));
    globalConfig_.emergencyDepthLimit = std::max(MIN_DEPTH, std::min(globalConfig_.emergencyDepthLimit, MAX_DEPTH));
    globalConfig_.depthTransitionTime = std::max(MIN_SMOOTHING_TIME, std::min(globalConfig_.depthTransitionTime, MAX_SMOOTHING_TIME));
}

// Per-parameter depth configuration
void VelocityDepthControl::setParameterDepthConfig(uint32_t parameterId, const ParameterDepthConfig& config) {
    ParameterDepthConfig clampedConfig = config;
    
    // Clamp depth values
    clampedConfig.baseDepth = std::max(MIN_DEPTH, std::min(clampedConfig.baseDepth, MAX_DEPTH));
    clampedConfig.maxAllowedDepth = std::max(MIN_DEPTH, std::min(clampedConfig.maxAllowedDepth, MAX_DEPTH));
    clampedConfig.minAllowedDepth = std::max(MIN_DEPTH, std::min(clampedConfig.minAllowedDepth, MAX_DEPTH));
    clampedConfig.depthSmoothingTime = std::max(MIN_SMOOTHING_TIME, std::min(clampedConfig.depthSmoothingTime, MAX_SMOOTHING_TIME));
    clampedConfig.masterDepthScale = std::max(0.0f, std::min(clampedConfig.masterDepthScale, 2.0f));
    
    // Ensure min <= max
    if (clampedConfig.minAllowedDepth > clampedConfig.maxAllowedDepth) {
        std::swap(clampedConfig.minAllowedDepth, clampedConfig.maxAllowedDepth);
    }
    
    parameterConfigs_[parameterId] = clampedConfig;
    
    // Initialize smoothing state
    currentSmoothDepths_[parameterId] = clampedConfig.baseDepth;
    targetDepths_[parameterId] = clampedConfig.baseDepth;
    realTimeDepthMod_[parameterId] = 0.0f;
}

void VelocityDepthControl::setParameterBaseDepth(uint32_t parameterId, float depth) {
    auto& config = parameterConfigs_[parameterId];
    float clampedDepth = std::max(config.minAllowedDepth, std::min(depth, config.maxAllowedDepth));
    
    float oldDepth = config.baseDepth;
    config.baseDepth = clampedDepth;
    targetDepths_[parameterId] = clampedDepth;
    
    notifyDepthChange(parameterId, oldDepth, clampedDepth);
}

void VelocityDepthControl::setParameterMaxDepth(uint32_t parameterId, float maxDepth) {
    auto& config = parameterConfigs_[parameterId];
    config.maxAllowedDepth = std::max(MIN_DEPTH, std::min(maxDepth, MAX_DEPTH));
    
    // Ensure current depth doesn't exceed new max
    if (config.baseDepth > config.maxAllowedDepth) {
        setParameterBaseDepth(parameterId, config.maxAllowedDepth);
    }
}

void VelocityDepthControl::setParameterDepthMode(uint32_t parameterId, DepthMode mode) {
    parameterConfigs_[parameterId].depthMode = mode;
}

void VelocityDepthControl::setParameterSafetyLevel(uint32_t parameterId, SafetyLevel level) {
    parameterConfigs_[parameterId].safetyLevel = level;
}

const VelocityDepthControl::ParameterDepthConfig& VelocityDepthControl::getParameterDepthConfig(uint32_t parameterId) const {
    auto it = parameterConfigs_.find(parameterId);
    return (it != parameterConfigs_.end()) ? it->second : defaultParameterConfig_;
}

float VelocityDepthControl::getParameterBaseDepth(uint32_t parameterId) const {
    auto it = parameterConfigs_.find(parameterId);
    return (it != parameterConfigs_.end()) ? it->second.baseDepth : DEFAULT_DEPTH;
}

bool VelocityDepthControl::hasParameterDepthConfig(uint32_t parameterId) const {
    return parameterConfigs_.find(parameterId) != parameterConfigs_.end();
}

// Depth calculation and application
VelocityDepthControl::DepthResult VelocityDepthControl::calculateEffectiveDepth(uint32_t parameterId, float requestedDepth) {
    DepthResult result;
    result.requestedDepth = requestedDepth;
    
    if (!enabled_) {
        result.actualDepth = 0.0f;
        result.effectiveDepth = 0.0f;
        return result;
    }
    
    const auto& config = getParameterDepthConfig(parameterId);
    float workingDepth = requestedDepth;
    
    // Apply master depth scaling if linked
    if (config.linkToMasterDepth && globalConfig_.enableMasterDepthControl) {
        workingDepth = config.baseDepth * globalConfig_.masterDepth * config.masterDepthScale;
    }
    
    // Add real-time depth modulation
    float rtMod = getRealTimeDepthModulation(parameterId);
    workingDepth += rtMod;
    
    // Apply safety limiting
    float safeDepth = applySafetyLimiting(parameterId, workingDepth, config.safetyLevel);
    result.wasLimited = (std::abs(safeDepth - workingDepth) > 0.001f);
    result.limitingAmount = result.wasLimited ? std::abs(safeDepth - workingDepth) / workingDepth : 0.0f;
    result.appliedSafetyLevel = config.safetyLevel;
    
    // Apply parameter-specific limits
    result.actualDepth = std::max(config.minAllowedDepth, std::min(safeDepth, config.maxAllowedDepth));
    
    // Apply depth mode-specific processing
    switch (config.depthMode) {
        case DepthMode::ABSOLUTE:
            result.effectiveDepth = result.actualDepth;
            break;
            
        case DepthMode::RELATIVE:
            // Scale by base parameter value (assuming 0.5 as reference)
            result.effectiveDepth = result.actualDepth * 0.5f;
            break;
            
        case DepthMode::SCALED:
            // Scale by parameter's natural modulation range
            result.effectiveDepth = result.actualDepth * 0.8f;  // Most parameters work well at 80% of full range
            break;
            
        case DepthMode::LIMITED:
            // Apply conservative scaling for safety
            result.effectiveDepth = result.actualDepth * 0.6f;
            break;
            
        case DepthMode::DYNAMIC:
            // Dynamic scaling based on current system state (simplified)
            float dynamicScale = std::min(1.0f, 2.0f / (1.0f + getAverageDepth()));
            result.effectiveDepth = result.actualDepth * dynamicScale;
            break;
    }
    
    // Apply smoothing if enabled
    auto smoothIt = currentSmoothDepths_.find(parameterId);
    if (smoothIt != currentSmoothDepths_.end() && config.enableDepthModulation) {
        float smoothTime = config.depthSmoothingTime / 1000.0f;  // Convert ms to seconds
        float deltaTime = 1.0f / 48000.0f;  // Assume 48kHz sample rate for smoothing
        
        float smoothedDepth = applyDepthSmoothing(smoothIt->second, result.effectiveDepth, smoothTime, deltaTime);
        result.wasSmoothened = (std::abs(smoothedDepth - result.effectiveDepth) > 0.001f);
        result.effectiveDepth = smoothedDepth;
        
        currentSmoothDepths_[parameterId] = smoothedDepth;
    }
    
    return result;
}

float VelocityDepthControl::applyDepthToModulation(uint32_t parameterId, float baseModulation, float velocity) {
    auto depthResult = calculateEffectiveDepth(parameterId, getParameterBaseDepth(parameterId));
    return baseModulation * depthResult.effectiveDepth;
}

float VelocityDepthControl::getEffectiveParameterDepth(uint32_t parameterId) const {
    auto it = parameterConfigs_.find(parameterId);
    if (it == parameterConfigs_.end()) {
        return DEFAULT_DEPTH;
    }
    
    float depth = it->second.baseDepth;
    
    // Apply master depth if linked
    if (it->second.linkToMasterDepth && globalConfig_.enableMasterDepthControl) {
        depth *= globalConfig_.masterDepth * it->second.masterDepthScale;
    }
    
    // Add real-time modulation
    depth += getRealTimeDepthModulation(parameterId);
    
    return std::max(it->second.minAllowedDepth, std::min(depth, it->second.maxAllowedDepth));
}

// Safety and limiting
float VelocityDepthControl::applySafetyLimiting(uint32_t parameterId, float depth, SafetyLevel level) {
    if (level == SafetyLevel::NONE) {
        return depth;
    }
    
    float maxSafeDepth = getMaxSafeDepth(parameterId, level);
    return std::min(depth, maxSafeDepth);
}

bool VelocityDepthControl::isDepthSafe(uint32_t parameterId, float depth) const {
    const auto& config = getParameterDepthConfig(parameterId);
    float maxSafe = getMaxSafeDepth(parameterId, config.safetyLevel);
    return depth <= maxSafe;
}

float VelocityDepthControl::getMaxSafeDepth(uint32_t parameterId, SafetyLevel level) const {
    switch (level) {
        case SafetyLevel::NONE:
            return MAX_DEPTH;
        case SafetyLevel::CONSERVATIVE:
            return 0.8f;  // 80% - should limit depths above this
        case SafetyLevel::MODERATE:
            return 1.2f;  // 120%
        case SafetyLevel::AGGRESSIVE:
            return 1.8f;  // 180%
        case SafetyLevel::CUSTOM:
            // Would look up custom limits per parameter
            return 1.5f;  // Default custom limit
    }
    return 1.0f;
}

void VelocityDepthControl::emergencyDepthLimit(float maxDepth) {
    float emergencyLimit = std::max(MIN_DEPTH, std::min(maxDepth, EMERGENCY_LIMIT));
    
    for (auto& [parameterId, config] : parameterConfigs_) {
        if (config.baseDepth > emergencyLimit) {
            config.baseDepth = emergencyLimit;
            currentSmoothDepths_[parameterId] = emergencyLimit;
            targetDepths_[parameterId] = emergencyLimit;
        }
    }
    
    if (globalConfig_.masterDepth > emergencyLimit) {
        globalConfig_.masterDepth = emergencyLimit;
    }
}

// Real-time depth modulation
void VelocityDepthControl::updateDepthSmoothing(float deltaTime) {
    for (auto& [parameterId, config] : parameterConfigs_) {
        if (config.enableDepthModulation) {
            updateDepthSmoothingForParameter(parameterId, deltaTime);
        }
    }
}

void VelocityDepthControl::setRealTimeDepthModulation(uint32_t parameterId, float depthModulation) {
    realTimeDepthMod_[parameterId] = std::max(-1.0f, std::min(depthModulation, 1.0f));
}

float VelocityDepthControl::getRealTimeDepthModulation(uint32_t parameterId) const {
    auto it = realTimeDepthMod_.find(parameterId);
    return (it != realTimeDepthMod_.end()) ? it->second : 0.0f;
}

// Batch operations
void VelocityDepthControl::setAllParametersDepth(float depth) {
    float clampedDepth = std::max(MIN_DEPTH, std::min(depth, MAX_DEPTH));
    
    for (auto& [parameterId, config] : parameterConfigs_) {
        float oldDepth = config.baseDepth;
        config.baseDepth = std::max(config.minAllowedDepth, std::min(clampedDepth, config.maxAllowedDepth));
        targetDepths_[parameterId] = config.baseDepth;
        notifyDepthChange(parameterId, oldDepth, config.baseDepth);
    }
}

void VelocityDepthControl::setAllParametersSafetyLevel(SafetyLevel level) {
    for (auto& [parameterId, config] : parameterConfigs_) {
        config.safetyLevel = level;
    }
}

void VelocityDepthControl::linkAllParametersToMaster(bool linked) {
    for (auto& [parameterId, config] : parameterConfigs_) {
        config.linkToMasterDepth = linked;
    }
}

void VelocityDepthControl::resetAllParametersToDefaults() {
    for (auto& [parameterId, config] : parameterConfigs_) {
        float oldDepth = config.baseDepth;
        config = defaultParameterConfig_;
        currentSmoothDepths_[parameterId] = config.baseDepth;
        targetDepths_[parameterId] = config.baseDepth;
        realTimeDepthMod_[parameterId] = 0.0f;
        notifyDepthChange(parameterId, oldDepth, config.baseDepth);
    }
}

// System management
void VelocityDepthControl::reset() {
    parameterConfigs_.clear();
    currentSmoothDepths_.clear();
    targetDepths_.clear();
    realTimeDepthMod_.clear();
    globalConfig_ = GlobalDepthConfig();
}

void VelocityDepthControl::removeParameter(uint32_t parameterId) {
    parameterConfigs_.erase(parameterId);
    currentSmoothDepths_.erase(parameterId);
    targetDepths_.erase(parameterId);
    realTimeDepthMod_.erase(parameterId);
}

// Statistics and monitoring
size_t VelocityDepthControl::getConfiguredParameterCount() const {
    return parameterConfigs_.size();
}

float VelocityDepthControl::getAverageDepth() const {
    if (parameterConfigs_.empty()) {
        return DEFAULT_DEPTH;
    }
    
    float sum = 0.0f;
    for (const auto& [parameterId, config] : parameterConfigs_) {
        sum += config.baseDepth;
    }
    
    return sum / static_cast<float>(parameterConfigs_.size());
}

size_t VelocityDepthControl::getParametersOverDepth(float depthThreshold) const {
    size_t count = 0;
    for (const auto& [parameterId, config] : parameterConfigs_) {
        if (config.baseDepth > depthThreshold) {
            count++;
        }
    }
    return count;
}

std::vector<uint32_t> VelocityDepthControl::getParametersWithExcessiveDepth(float threshold) const {
    std::vector<uint32_t> excessiveParameters;
    
    for (const auto& [parameterId, config] : parameterConfigs_) {
        if (config.baseDepth > threshold) {
            excessiveParameters.push_back(parameterId);
        }
    }
    
    return excessiveParameters;
}

float VelocityDepthControl::getSystemDepthLoad() const {
    // Estimate processing load based on active depth modulation
    float load = 0.0f;
    
    for (const auto& [parameterId, config] : parameterConfigs_) {
        if (config.enableDepthModulation) {
            load += config.baseDepth * 0.01f;  // ~1% CPU per unit of depth
        }
    }
    
    return load;
}

// Integration with other systems
void VelocityDepthControl::integrateWithVelocityLatch(VelocityLatchSystem* latchSystem) {
    latchSystem_ = latchSystem;
}

void VelocityDepthControl::integrateWithParameterScaling(VelocityParameterScaling* scalingSystem) {
    scalingSystem_ = scalingSystem;
}

void VelocityDepthControl::integrateWithVelocityUI(std::shared_ptr<VelocityModulationUI::VelocityModulationPanel> panel) {
    uiPanel_ = panel;
}

void VelocityDepthControl::setDepthChangeCallback(DepthChangeCallback callback) {
    depthChangeCallback_ = callback;
}

// Private methods
void VelocityDepthControl::initializeDepthPresets() {
    // Subtle depth preset
    DepthPreset subtlePreset("Subtle", "Conservative depth settings for gentle modulation");
    subtlePreset.globalConfig.masterDepth = 0.7f;
    subtlePreset.globalConfig.globalSafetyLevel = SafetyLevel::CONSERVATIVE;
    depthPresets_.push_back(subtlePreset);
    
    // Standard depth preset
    DepthPreset standardPreset("Standard", "Balanced depth settings for general use");
    standardPreset.globalConfig.masterDepth = 1.0f;
    standardPreset.globalConfig.globalSafetyLevel = SafetyLevel::MODERATE;
    depthPresets_.push_back(standardPreset);
    
    // Extreme depth preset
    DepthPreset extremePreset("Extreme", "High depth settings for dramatic expression");
    extremePreset.globalConfig.masterDepth = 1.5f;
    extremePreset.globalConfig.globalSafetyLevel = SafetyLevel::AGGRESSIVE;
    depthPresets_.push_back(extremePreset);
}

void VelocityDepthControl::updateDepthSmoothingForParameter(uint32_t parameterId, float deltaTime) {
    auto currentIt = currentSmoothDepths_.find(parameterId);
    auto targetIt = targetDepths_.find(parameterId);
    
    if (currentIt == currentSmoothDepths_.end() || targetIt == targetDepths_.end()) {
        return;
    }
    
    const auto& config = getParameterDepthConfig(parameterId);
    float smoothTime = config.depthSmoothingTime / 1000.0f;  // Convert to seconds
    
    currentIt->second = applyDepthSmoothing(currentIt->second, targetIt->second, smoothTime, deltaTime);
}

void VelocityDepthControl::notifyDepthChange(uint32_t parameterId, float oldDepth, float newDepth) {
    if (depthChangeCallback_) {
        depthChangeCallback_(parameterId, oldDepth, newDepth);
    }
    
    // Update V-icon if UI is integrated
    if (uiPanel_) {
        auto* vIcon = uiPanel_->getVIcon(parameterId);
        if (vIcon) {
            vIcon->setModulationDepth(newDepth);
        }
    }
}

float VelocityDepthControl::applyDepthSmoothing(float current, float target, float smoothingTime, float deltaTime) {
    if (smoothingTime <= 0.0f) {
        return target;
    }
    
    float alpha = 1.0f - std::exp(-deltaTime / smoothingTime);
    return current + alpha * (target - current);
}