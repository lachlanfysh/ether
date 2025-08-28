#include "VelocityParameterScaling.h"
#include <algorithm>
#include <cmath>
#include <numeric>

// Static member definition
VelocityParameterScaling::ParameterScalingConfig VelocityParameterScaling::defaultConfig_;

VelocityParameterScaling::VelocityParameterScaling() {
    enabled_ = true;
    initializeCategoryDefaults();
    initializeScalingPresets();
}

// Parameter configuration
void VelocityParameterScaling::setParameterScaling(uint32_t parameterId, const ParameterScalingConfig& config) {
    parameterConfigs_[parameterId] = config;
    
    // Initialize analysis data if auto-scaling is enabled
    if (config.enableAutoScaling) {
        velocityAnalysis_[parameterId] = VelocityAnalysis();
    }
}

void VelocityParameterScaling::setParameterCategory(uint32_t parameterId, ParameterCategory category) {
    auto it = parameterConfigs_.find(parameterId);
    if (it != parameterConfigs_.end()) {
        it->second.category = category;
        // Apply category defaults to this parameter
        applyDefaultScalingForCategory(parameterId, category);
    } else {
        // Create new configuration with category defaults
        applyDefaultScalingForCategory(parameterId, category);
    }
}

void VelocityParameterScaling::setParameterVelocityScale(uint32_t parameterId, float scale) {
    auto& config = parameterConfigs_[parameterId];
    config.velocityScale = clampScale(scale);
}

void VelocityParameterScaling::setParameterPolarity(uint32_t parameterId, VelocityModulationUI::ModulationPolarity polarity) {
    auto& config = parameterConfigs_[parameterId];
    config.polarity = polarity;
}

void VelocityParameterScaling::setParameterVelocityRange(uint32_t parameterId, const VelocityRange& range) {
    auto& config = parameterConfigs_[parameterId];
    config.velocityRange = range;
}

void VelocityParameterScaling::setParameterDeadzone(uint32_t parameterId, float deadzone) {
    auto& config = parameterConfigs_[parameterId];
    config.deadzone = std::max(MIN_DEADZONE, std::min(deadzone, MAX_DEADZONE));
}

void VelocityParameterScaling::setParameterThreshold(uint32_t parameterId, float threshold) {
    auto& config = parameterConfigs_[parameterId];
    config.threshold = std::max(MIN_THRESHOLD, std::min(threshold, MAX_THRESHOLD));
}

const VelocityParameterScaling::ParameterScalingConfig& VelocityParameterScaling::getParameterScaling(uint32_t parameterId) const {
    auto it = parameterConfigs_.find(parameterId);
    return (it != parameterConfigs_.end()) ? it->second : defaultConfig_;
}

VelocityParameterScaling::ParameterCategory VelocityParameterScaling::getParameterCategory(uint32_t parameterId) const {
    auto it = parameterConfigs_.find(parameterId);
    return (it != parameterConfigs_.end()) ? it->second.category : ParameterCategory::CUSTOM;
}

float VelocityParameterScaling::getParameterVelocityScale(uint32_t parameterId) const {
    auto it = parameterConfigs_.find(parameterId);
    return (it != parameterConfigs_.end()) ? it->second.velocityScale : 1.0f;
}

bool VelocityParameterScaling::hasParameterScaling(uint32_t parameterId) const {
    return parameterConfigs_.find(parameterId) != parameterConfigs_.end();
}

// Velocity scaling calculation
VelocityParameterScaling::ScalingResult VelocityParameterScaling::calculateParameterScaling(uint32_t parameterId, float velocity, float baseValue) {
    ScalingResult result;
    result.originalVelocity = velocity;
    
    if (!enabled_) {
        result.scaledVelocity = velocity;
        result.finalValue = baseValue;
        return result;
    }
    
    auto it = parameterConfigs_.find(parameterId);
    if (it == parameterConfigs_.end()) {
        result.scaledVelocity = velocity;
        result.finalValue = baseValue;
        return result;
    }
    
    const auto& config = it->second;
    result.category = config.category;
    
    // Check deadzone
    result.inDeadzone = isInDeadzone(velocity, config.deadzone);
    if (result.inDeadzone) {
        result.scaledVelocity = 0.0f;
        result.finalValue = baseValue;  // No modulation in deadzone
        return result;
    }
    
    // Check threshold (with hysteresis)
    static std::unordered_map<uint32_t, bool> thresholdStates;
    result.thresholdPassed = passesThreshold(velocity, config.threshold, config.hysteresis, thresholdStates[parameterId]);
    if (!result.thresholdPassed) {
        result.scaledVelocity = 0.0f;
        result.finalValue = baseValue;  // No modulation below threshold
        return result;
    }
    
    // Apply velocity inversion
    float processedVel = config.invertVelocity ? (1.0f - velocity) : velocity;
    
    // Apply velocity range mapping
    processedVel = applyVelocityRange(config.velocityRange, processedVel);
    
    // Apply compression/expansion
    if (config.compressionRatio > 1.0f) {
        processedVel = applyVelocityCompression(processedVel, config.compressionRatio, config.softKnee);
        result.compressionAmount = (config.compressionRatio - 1.0f) / (MAX_COMPRESSION_RATIO - 1.0f);
    }
    
    if (config.expansionRatio < 1.0f) {
        processedVel = applyVelocityExpansion(processedVel, config.expansionRatio, config.softKnee);
        result.expansionAmount = (1.0f - config.expansionRatio) / (1.0f - MIN_EXPANSION_RATIO);
    }
    
    // Apply velocity scaling
    processedVel *= config.velocityScale;
    
    // Apply polarity and calculate final value
    switch (config.polarity) {
        case VelocityModulationUI::ModulationPolarity::POSITIVE:
            result.finalValue = baseValue + processedVel;
            break;
            
        case VelocityModulationUI::ModulationPolarity::NEGATIVE:
            result.finalValue = baseValue - processedVel;
            break;
            
        case VelocityModulationUI::ModulationPolarity::BIPOLAR:
            result.finalValue = applyBipolarScaling(processedVel, config.centerPoint, config.asymmetry);
            break;
    }
    
    // Clamp final result
    result.finalValue = std::max(0.0f, std::min(result.finalValue, 1.0f));
    result.scaledVelocity = processedVel;
    
    // Update auto-scaling analysis if enabled
    if (config.enableAutoScaling) {
        analyzeVelocityUsage(parameterId, velocity);
    }
    
    return result;
}

float VelocityParameterScaling::applyVelocityScaling(uint32_t parameterId, float velocity) {
    const auto& config = getParameterScaling(parameterId);
    
    if (!enabled_ || isInDeadzone(velocity, config.deadzone)) {
        return 0.0f;
    }
    
    float processedVel = config.invertVelocity ? (1.0f - velocity) : velocity;
    processedVel = applyVelocityRange(config.velocityRange, processedVel);
    
    if (config.compressionRatio > 1.0f) {
        processedVel = applyVelocityCompression(processedVel, config.compressionRatio, config.softKnee);
    }
    
    if (config.expansionRatio < 1.0f) {
        processedVel = applyVelocityExpansion(processedVel, config.expansionRatio, config.softKnee);
    }
    
    return processedVel * config.velocityScale;
}

float VelocityParameterScaling::applyVelocityRange(const VelocityRange& range, float velocity) {
    // Map input range to output range
    if (velocity < range.inputMin) {
        velocity = range.inputMin;
    } else if (velocity > range.inputMax) {
        velocity = range.inputMax;
    }
    
    // Normalize to 0-1 within input range
    float normalized = (velocity - range.inputMin) / (range.inputMax - range.inputMin);
    
    // Map to output range
    float mapped = range.outputMin + normalized * (range.outputMax - range.outputMin);
    
    // Clamp if requested
    if (range.clampOutput) {
        mapped = std::max(0.0f, std::min(mapped, 1.0f));
    }
    
    return mapped;
}

float VelocityParameterScaling::applyVelocityCompression(float velocity, float ratio, float softKnee) {
    return applySoftKneeCompression(velocity, ratio, softKnee);
}

float VelocityParameterScaling::applyVelocityExpansion(float velocity, float ratio, float softKnee) {
    return applySoftKneeExpansion(velocity, ratio, softKnee);
}

float VelocityParameterScaling::applyBipolarScaling(float velocity, float centerPoint, float asymmetry) {
    // Convert velocity to bipolar range around center point
    float bipolar = (velocity - 0.5f) * 2.0f;  // -1 to +1
    
    // Apply asymmetry
    if (asymmetry != 0.0f) {
        if (bipolar > 0.0f) {
            bipolar *= (1.0f + asymmetry);
        } else {
            bipolar *= (1.0f - asymmetry);
        }
    }
    
    // Scale around center point
    return centerPoint + bipolar * 0.5f;
}

// Category-based default configurations
void VelocityParameterScaling::applyDefaultScalingForCategory(uint32_t parameterId, ParameterCategory category) {
    auto defaultConfig = getDefaultConfigForCategory(category);
    defaultConfig.category = category;
    setParameterScaling(parameterId, defaultConfig);
}

VelocityParameterScaling::ParameterScalingConfig VelocityParameterScaling::getDefaultConfigForCategory(ParameterCategory category) const {
    auto it = categoryDefaults_.find(category);
    return (it != categoryDefaults_.end()) ? it->second : defaultConfig_;
}

void VelocityParameterScaling::updateCategoryDefaults(ParameterCategory category, const ParameterScalingConfig& config) {
    categoryDefaults_[category] = config;
}

// Preset management
void VelocityParameterScaling::addScalingPreset(const ScalingPreset& preset) {
    // Remove existing preset with same name
    removeScalingPreset(preset.name);
    scalingPresets_.push_back(preset);
}

void VelocityParameterScaling::removeScalingPreset(const std::string& presetName) {
    scalingPresets_.erase(
        std::remove_if(scalingPresets_.begin(), scalingPresets_.end(),
            [&presetName](const ScalingPreset& preset) {
                return preset.name == presetName;
            }),
        scalingPresets_.end()
    );
}

void VelocityParameterScaling::applyScalingPreset(uint32_t parameterId, const std::string& presetName) {
    for (const auto& preset : scalingPresets_) {
        if (preset.name == presetName) {
            setParameterScaling(parameterId, preset.config);
            break;
        }
    }
}

std::vector<VelocityParameterScaling::ScalingPreset> VelocityParameterScaling::getAvailablePresets() const {
    return scalingPresets_;
}

std::vector<VelocityParameterScaling::ScalingPreset> VelocityParameterScaling::getPresetsForCategory(ParameterCategory category) const {
    std::vector<ScalingPreset> categoryPresets;
    
    for (const auto& preset : scalingPresets_) {
        if (preset.config.category == category) {
            categoryPresets.push_back(preset);
        }
    }
    
    return categoryPresets;
}

// Batch operations
void VelocityParameterScaling::setAllParametersScale(float scale) {
    float clampedScale = clampScale(scale);
    for (auto& [parameterId, config] : parameterConfigs_) {
        config.velocityScale = clampedScale;
    }
}

void VelocityParameterScaling::setAllParametersPolarity(VelocityModulationUI::ModulationPolarity polarity) {
    for (auto& [parameterId, config] : parameterConfigs_) {
        config.polarity = polarity;
    }
}

void VelocityParameterScaling::applyCategoryScalingToAll(ParameterCategory category) {
    auto defaultConfig = getDefaultConfigForCategory(category);
    for (auto& [parameterId, config] : parameterConfigs_) {
        if (config.category == category) {
            config = defaultConfig;
            config.category = category;  // Preserve category
        }
    }
}

void VelocityParameterScaling::resetAllParametersToDefaults() {
    for (auto& [parameterId, config] : parameterConfigs_) {
        auto defaultConfig = getDefaultConfigForCategory(config.category);
        defaultConfig.category = config.category;  // Preserve category
        config = defaultConfig;
    }
}

// Auto-scaling and analysis
void VelocityParameterScaling::enableAutoScaling(uint32_t parameterId, bool enabled) {
    auto it = parameterConfigs_.find(parameterId);
    if (it != parameterConfigs_.end()) {
        it->second.enableAutoScaling = enabled;
        if (enabled) {
            velocityAnalysis_[parameterId] = VelocityAnalysis();
        } else {
            velocityAnalysis_.erase(parameterId);
        }
    }
}

void VelocityParameterScaling::resetVelocityAnalysis(uint32_t parameterId) {
    auto it = velocityAnalysis_.find(parameterId);
    if (it != velocityAnalysis_.end()) {
        it->second = VelocityAnalysis();
    }
}

void VelocityParameterScaling::analyzeVelocityUsage(uint32_t parameterId, float velocity) {
    updateVelocityAnalysis(parameterId, velocity);
    
    // Update auto-scaling if enough samples collected
    auto& analysis = velocityAnalysis_[parameterId];
    if (analysis.sampleCount_ >= 10) {  // Need at least 10 samples
        updateAutoScaling(parameterId);
    }
}

void VelocityParameterScaling::updateAutoScaling(uint32_t parameterId) {
    auto analysisIt = velocityAnalysis_.find(parameterId);
    auto configIt = parameterConfigs_.find(parameterId);
    
    if (analysisIt == velocityAnalysis_.end() || configIt == parameterConfigs_.end()) {
        return;
    }
    
    const auto& analysis = analysisIt->second;
    auto& config = configIt->second;
    
    if (!config.enableAutoScaling) {
        return;
    }
    
    // Calculate recommended scale based on velocity usage
    float recommendedScale = calculateRecommendedScale(analysis);
    
    // Apply gradual adjustment (don't jump immediately)
    float adjustment = 0.1f;
    if (recommendedScale > config.velocityScale) {
        config.velocityScale += adjustment;
    } else if (recommendedScale < config.velocityScale) {
        config.velocityScale -= adjustment;
    }
    
    config.velocityScale = clampScale(config.velocityScale);
}

// System management
void VelocityParameterScaling::reset() {
    parameterConfigs_.clear();
    velocityAnalysis_.clear();
    initializeCategoryDefaults();
    initializeScalingPresets();
}

void VelocityParameterScaling::removeParameter(uint32_t parameterId) {
    parameterConfigs_.erase(parameterId);
    velocityAnalysis_.erase(parameterId);
}

// Statistics and monitoring
size_t VelocityParameterScaling::getConfiguredParameterCount() const {
    return parameterConfigs_.size();
}

std::vector<uint32_t> VelocityParameterScaling::getParametersInCategory(ParameterCategory category) const {
    std::vector<uint32_t> parameters;
    
    for (const auto& [parameterId, config] : parameterConfigs_) {
        if (config.category == category) {
            parameters.push_back(parameterId);
        }
    }
    
    return parameters;
}

float VelocityParameterScaling::getAverageVelocityScale() const {
    if (parameterConfigs_.empty()) {
        return 1.0f;
    }
    
    float sum = 0.0f;
    for (const auto& [parameterId, config] : parameterConfigs_) {
        sum += config.velocityScale;
    }
    
    return sum / static_cast<float>(parameterConfigs_.size());
}

std::unordered_map<VelocityParameterScaling::ParameterCategory, int> VelocityParameterScaling::getCategoryCounts() const {
    std::unordered_map<ParameterCategory, int> counts;
    
    for (const auto& [parameterId, config] : parameterConfigs_) {
        counts[config.category]++;
    }
    
    return counts;
}

// Utility functions
std::string VelocityParameterScaling::categoryToString(ParameterCategory category) {
    switch (category) {
        case ParameterCategory::FILTER_CUTOFF: return "Filter Cutoff";
        case ParameterCategory::FILTER_RESONANCE: return "Filter Resonance";
        case ParameterCategory::OSCILLATOR_LEVEL: return "Oscillator Level";
        case ParameterCategory::ENVELOPE_ATTACK: return "Envelope Attack";
        case ParameterCategory::ENVELOPE_DECAY: return "Envelope Decay";
        case ParameterCategory::ENVELOPE_SUSTAIN: return "Envelope Sustain";
        case ParameterCategory::ENVELOPE_RELEASE: return "Envelope Release";
        case ParameterCategory::LFO_RATE: return "LFO Rate";
        case ParameterCategory::LFO_DEPTH: return "LFO Depth";
        case ParameterCategory::DISTORTION_DRIVE: return "Distortion Drive";
        case ParameterCategory::DELAY_TIME: return "Delay Time";
        case ParameterCategory::REVERB_SIZE: return "Reverb Size";
        case ParameterCategory::REVERB_DAMPING: return "Reverb Damping";
        case ParameterCategory::PITCH_BEND: return "Pitch Bend";
        case ParameterCategory::DETUNE: return "Detune";
        case ParameterCategory::PAN: return "Pan";
        case ParameterCategory::VOLUME: return "Volume";
        case ParameterCategory::CUSTOM: return "Custom";
    }
    return "Unknown";
}

bool VelocityParameterScaling::isValidScale(float scale) {
    return scale >= MIN_VELOCITY_SCALE && scale <= MAX_VELOCITY_SCALE;
}

float VelocityParameterScaling::clampScale(float scale) {
    return std::max(MIN_VELOCITY_SCALE, std::min(scale, MAX_VELOCITY_SCALE));
}

// Private methods
void VelocityParameterScaling::initializeCategoryDefaults() {
    ParameterScalingConfig config;
    
    // Filter Cutoff - Exponential scaling works well for frequency
    config = ParameterScalingConfig();
    config.category = ParameterCategory::FILTER_CUTOFF;
    config.velocityScale = 1.5f;  // More sensitive
    config.polarity = VelocityModulationUI::ModulationPolarity::POSITIVE;
    categoryDefaults_[ParameterCategory::FILTER_CUTOFF] = config;
    
    // Filter Resonance - More conservative scaling
    config = ParameterScalingConfig();
    config.category = ParameterCategory::FILTER_RESONANCE;
    config.velocityScale = 0.8f;  // Less sensitive to avoid harshness
    config.polarity = VelocityModulationUI::ModulationPolarity::POSITIVE;
    categoryDefaults_[ParameterCategory::FILTER_RESONANCE] = config;
    
    // Volume - Exponential for perceived loudness
    config = ParameterScalingConfig();
    config.category = ParameterCategory::VOLUME;
    config.velocityScale = 2.0f;  // High sensitivity
    config.polarity = VelocityModulationUI::ModulationPolarity::POSITIVE;
    categoryDefaults_[ParameterCategory::VOLUME] = config;
    
    // Pan - Bipolar around center
    config = ParameterScalingConfig();
    config.category = ParameterCategory::PAN;
    config.velocityScale = 1.0f;
    config.polarity = VelocityModulationUI::ModulationPolarity::BIPOLAR;
    config.centerPoint = 0.5f;  // Center pan
    categoryDefaults_[ParameterCategory::PAN] = config;
    
    // Add more category defaults...
    // (For brevity, showing key examples)
}

void VelocityParameterScaling::initializeScalingPresets() {
    // Create some useful presets
    ParameterScalingConfig config;
    
    // Subtle scaling preset
    config = ParameterScalingConfig();
    config.velocityScale = 0.5f;
    config.polarity = VelocityModulationUI::ModulationPolarity::POSITIVE;
    scalingPresets_.emplace_back("Subtle", config, "Gentle velocity response");
    
    // Aggressive scaling preset
    config = ParameterScalingConfig();
    config.velocityScale = 2.0f;
    config.polarity = VelocityModulationUI::ModulationPolarity::POSITIVE;
    scalingPresets_.emplace_back("Aggressive", config, "Strong velocity response");
    
    // Bipolar preset
    config = ParameterScalingConfig();
    config.velocityScale = 1.0f;
    config.polarity = VelocityModulationUI::ModulationPolarity::BIPOLAR;
    config.centerPoint = 0.5f;
    scalingPresets_.emplace_back("Bipolar", config, "Bidirectional modulation");
}

float VelocityParameterScaling::applySoftKneeCompression(float velocity, float ratio, float knee) {
    // Simple soft-knee compression
    float threshold = 0.7f;  // Compress above 70%
    
    if (velocity <= threshold - knee/2) {
        return velocity;  // Below knee, no compression
    } else if (velocity >= threshold + knee/2) {
        // Above knee, full compression
        float excess = velocity - threshold;
        return threshold + excess / ratio;
    } else {
        // Within knee, gradual compression
        float kneeRatio = (velocity - (threshold - knee/2)) / knee;
        float currentRatio = 1.0f + kneeRatio * (ratio - 1.0f);
        float excess = velocity - threshold;
        return threshold + excess / currentRatio;
    }
}

float VelocityParameterScaling::applySoftKneeExpansion(float velocity, float ratio, float knee) {
    // Simple soft-knee expansion (opposite of compression)
    float threshold = 0.3f;  // Expand below 30%
    
    if (velocity >= threshold + knee/2) {
        return velocity;  // Above knee, no expansion
    } else if (velocity <= threshold - knee/2) {
        // Below knee, full expansion
        float deficit = threshold - velocity;
        return threshold - deficit / ratio;
    } else {
        // Within knee, gradual expansion
        float kneeRatio = ((threshold + knee/2) - velocity) / knee;
        float currentRatio = 1.0f + kneeRatio * (1.0f/ratio - 1.0f);
        float deficit = threshold - velocity;
        return threshold - deficit / currentRatio;
    }
}

bool VelocityParameterScaling::isInDeadzone(float velocity, float deadzone) {
    return velocity <= deadzone;
}

bool VelocityParameterScaling::passesThreshold(float velocity, float threshold, float hysteresis, bool& state) {
    if (!state && velocity > threshold + hysteresis) {
        state = true;
    } else if (state && velocity < threshold - hysteresis) {
        state = false;
    }
    return state;
}

void VelocityParameterScaling::updateVelocityAnalysis(uint32_t parameterId, float velocity) {
    auto& analysis = velocityAnalysis_[parameterId];
    
    // Update min/max
    analysis.minVelocity_ = std::min(analysis.minVelocity_, velocity);
    analysis.maxVelocity_ = std::max(analysis.maxVelocity_, velocity);
    
    // Add to history
    analysis.velocityHistory_.push_back(velocity);
    if (analysis.velocityHistory_.size() > MAX_ANALYSIS_HISTORY) {
        analysis.velocityHistory_.erase(analysis.velocityHistory_.begin());
    }
    
    // Update average
    analysis.sampleCount_++;
    float sum = std::accumulate(analysis.velocityHistory_.begin(), analysis.velocityHistory_.end(), 0.0f);
    analysis.averageVelocity_ = sum / analysis.velocityHistory_.size();
}

float VelocityParameterScaling::calculateRecommendedScale(const VelocityAnalysis& analysis) {
    // Calculate recommended scale based on velocity usage patterns
    float range = analysis.maxVelocity_ - analysis.minVelocity_;
    
    if (range < 0.3f) {
        // Limited velocity range, suggest higher scaling
        return 2.0f;
    } else if (range > 0.8f) {
        // Full velocity range, suggest moderate scaling
        return 1.0f;
    } else {
        // Intermediate range
        return 1.5f;
    }
}