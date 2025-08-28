#include "ParameterSystem.h"
#include <chrono>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

// Global parameter system instance
UnifiedParameterSystem g_parameterSystem;

UnifiedParameterSystem::UnifiedParameterSystem() :
    velocityScaling_(std::make_unique<VelocityParameterScaling>()) {
    // Initialize atomic arrays to default values
    for (size_t i = 0; i < static_cast<size_t>(ParameterID::COUNT); ++i) {
        globalParameters_[i].store(0.0f, std::memory_order_relaxed);
        for (size_t j = 0; j < MAX_INSTRUMENTS; ++j) {
            instrumentParameters_[j][i].store(0.0f, std::memory_order_relaxed);
        }
    }
}

UnifiedParameterSystem::~UnifiedParameterSystem() {
    shutdown();
}

bool UnifiedParameterSystem::initialize(float sampleRate) {
    if (initialized_.load()) {
        updateLastError("Parameter system already initialized");
        return false;
    }
    
    sampleRate_ = sampleRate;
    
    // Initialize default parameter configurations
    initializeDefaultParameters();
    
    // Setup velocity scaling integration
    setupVelocityScaling();
    
    // Initialize smoothers for all registered parameters
    for (const auto& [paramId, config] : parameterConfigs_) {
        if (config.enableSmoothing) {
            // Create global smoother
            auto globalSmoother = std::make_unique<AdvancedParameterSmoother>();
            AdvancedParameterSmoother::Config smoothConfig;
            smoothConfig.smoothType = config.smoothType;
            smoothConfig.curveType = config.curveType;
            smoothConfig.audibleTimeMs = config.smoothTimeMs;
            globalSmoother->initialize(sampleRate_, smoothConfig);
            globalSmoother->setValue(config.defaultValue);
            globalSmoothers_[paramId] = std::move(globalSmoother);
            
            // Create per-instrument smoothers
            for (size_t i = 0; i < MAX_INSTRUMENTS; ++i) {
                auto instrumentSmoother = std::make_unique<AdvancedParameterSmoother>();
                instrumentSmoother->initialize(sampleRate_, smoothConfig);
                instrumentSmoother->setValue(config.defaultValue);
                instrumentSmoothers_[i][paramId] = std::move(instrumentSmoother);
            }
        }
        
        // Initialize parameter values
        parameterValues_[paramId] = ParameterValue(config.defaultValue);
        globalParameters_[static_cast<size_t>(paramId)].store(config.defaultValue, std::memory_order_relaxed);
        
        for (size_t i = 0; i < MAX_INSTRUMENTS; ++i) {
            instrumentParameters_[i][static_cast<size_t>(paramId)].store(config.defaultValue, std::memory_order_relaxed);
        }
    }
    
    initialized_.store(true);
    return true;
}

void UnifiedParameterSystem::shutdown() {
    if (!initialized_.load()) return;
    
    initialized_.store(false);
    
    // Clear smoothers
    globalSmoothers_.clear();
    for (auto& instrumentSmoothers : instrumentSmoothers_) {
        instrumentSmoothers.clear();
    }
    
    // Clear configurations
    std::lock_guard<std::mutex> lock(configMutex_);
    parameterConfigs_.clear();
    parameterValues_.clear();
    automationData_.clear();
}

void UnifiedParameterSystem::setSampleRate(float sampleRate) {
    if (sampleRate <= 0.0f) {
        updateLastError("Invalid sample rate: " + std::to_string(sampleRate));
        return;
    }
    
    sampleRate_ = sampleRate;
    
    // Update all smoothers with new sample rate
    for (auto& [paramId, smoother] : globalSmoothers_) {
        smoother->setSampleRate(sampleRate);
    }
    
    for (auto& instrumentSmoothers : instrumentSmoothers_) {
        for (auto& [paramId, smoother] : instrumentSmoothers) {
            smoother->setSampleRate(sampleRate);
        }
    }
}

bool UnifiedParameterSystem::registerParameter(const ParameterConfig& config) {
    if (!isValidParameterID(config.id)) {
        updateLastError("Invalid parameter ID");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(configMutex_);
    parameterConfigs_[config.id] = config;
    
    // Initialize parameter value
    parameterValues_[config.id] = ParameterValue(config.defaultValue);
    globalParameters_[static_cast<size_t>(config.id)].store(config.defaultValue, std::memory_order_relaxed);
    
    for (size_t i = 0; i < MAX_INSTRUMENTS; ++i) {
        instrumentParameters_[i][static_cast<size_t>(config.id)].store(config.defaultValue, std::memory_order_relaxed);
    }
    
    // Setup velocity scaling for this parameter
    if (config.enableVelocityScaling && velocityScaling_) {
        VelocityParameterScaling::ParameterScalingConfig velocityConfig;
        velocityConfig.category = config.velocityCategory;
        velocityConfig.velocityScale = config.velocityScale;
        velocityScaling_->setParameterScaling(static_cast<uint32_t>(config.id), velocityConfig);
    }
    
    return true;
}

bool UnifiedParameterSystem::registerParameter(ParameterID id, const std::string& name, 
                                              float minValue, float maxValue, float defaultValue) {
    ParameterConfig config;
    config.id = id;
    config.name = name;
    config.displayName = name;
    config.minValue = minValue;
    config.maxValue = maxValue;
    config.defaultValue = defaultValue;
    
    return registerParameter(config);
}

float UnifiedParameterSystem::getParameterValue(ParameterID id) const {
    if (!isValidParameterID(id)) return 0.0f;
    return globalParameters_[static_cast<size_t>(id)].load(std::memory_order_relaxed);
}

float UnifiedParameterSystem::getParameterValue(ParameterID id, size_t instrumentIndex) const {
    if (!isValidParameterID(id) || !isValidInstrumentIndex(instrumentIndex)) return 0.0f;
    return instrumentParameters_[instrumentIndex][static_cast<size_t>(id)].load(std::memory_order_relaxed);
}

UnifiedParameterSystem::UpdateResult UnifiedParameterSystem::setParameterValue(ParameterID id, float value) {
    if (!initialized_.load()) return UpdateResult::SYSTEM_LOCKED;
    if (!isValidParameterID(id)) return UpdateResult::INVALID_PARAMETER;
    
    std::lock_guard<std::mutex> lock(configMutex_);
    auto configIt = parameterConfigs_.find(id);
    if (configIt == parameterConfigs_.end()) return UpdateResult::INVALID_PARAMETER;
    
    const auto& config = configIt->second;
    
    // Validate value
    if (!validateParameterValue(id, value)) {
        return UpdateResult::VALUE_OUT_OF_RANGE;
    }
    
    // Apply custom validation if provided
    if (config.onValidateValue && !config.onValidateValue(id, value)) {
        return UpdateResult::VALIDATION_FAILED;
    }
    
    // Process the value (clamp, quantize)
    float processedValue = processParameterValue(id, value);
    
    // Update parameter storage
    float oldValue = globalParameters_[static_cast<size_t>(id)].load(std::memory_order_relaxed);
    
    if (config.enableSmoothing) {
        // Use smoother
        auto smoother = getSmoother(id, 0);
        if (smoother) {
            smoother->setTarget(processedValue);
            // Don't update atomic value yet - let processAudioBlock handle it
            return UpdateResult::SMOOTHING_ACTIVE;
        }
    }
    
    // Direct update (no smoothing)
    globalParameters_[static_cast<size_t>(id)].store(processedValue, std::memory_order_relaxed);
    parameterValues_[id].value = processedValue;
    parameterValues_[id].rawValue = value;
    parameterValues_[id].targetValue = processedValue;
    parameterValues_[id].hasBeenSet = true;
    parameterValues_[id].lastUpdateTime = std::chrono::steady_clock::now().time_since_epoch().count();
    
    // Call value changed callback
    if (config.onValueChanged) {
        config.onValueChanged(id, oldValue, processedValue);
    }
    
    // Record automation if enabled
    if (isParameterAutomationEnabled(id)) {
        recordParameterChange(id, processedValue, parameterValues_[id].lastUpdateTime);
    }
    
    return UpdateResult::SUCCESS;
}

UnifiedParameterSystem::UpdateResult UnifiedParameterSystem::setParameterValue(ParameterID id, size_t instrumentIndex, float value) {
    if (!initialized_.load()) return UpdateResult::SYSTEM_LOCKED;
    if (!isValidParameterID(id) || !isValidInstrumentIndex(instrumentIndex)) {
        return UpdateResult::INVALID_PARAMETER;
    }
    
    std::lock_guard<std::mutex> lock(configMutex_);
    auto configIt = parameterConfigs_.find(id);
    if (configIt == parameterConfigs_.end()) return UpdateResult::INVALID_PARAMETER;
    
    const auto& config = configIt->second;
    
    // Validate value
    if (!validateParameterValue(id, value)) {
        return UpdateResult::VALUE_OUT_OF_RANGE;
    }
    
    // Process the value
    float processedValue = processParameterValue(id, value);
    
    // Update instrument parameter
    float oldValue = instrumentParameters_[instrumentIndex][static_cast<size_t>(id)].load(std::memory_order_relaxed);
    
    if (config.enableSmoothing) {
        // Use instrument-specific smoother
        auto smoother = getSmoother(id, instrumentIndex);
        if (smoother) {
            smoother->setTarget(processedValue);
            return UpdateResult::SMOOTHING_ACTIVE;
        }
    }
    
    // Direct update
    instrumentParameters_[instrumentIndex][static_cast<size_t>(id)].store(processedValue, std::memory_order_relaxed);
    
    // Call value changed callback
    if (config.onValueChanged) {
        config.onValueChanged(id, oldValue, processedValue);
    }
    
    return UpdateResult::SUCCESS;
}

UnifiedParameterSystem::UpdateResult UnifiedParameterSystem::setParameterValueImmediate(ParameterID id, float value) {
    if (!initialized_.load()) return UpdateResult::SYSTEM_LOCKED;
    if (!isValidParameterID(id)) return UpdateResult::INVALID_PARAMETER;
    
    std::lock_guard<std::mutex> lock(configMutex_);
    auto configIt = parameterConfigs_.find(id);
    if (configIt == parameterConfigs_.end()) return UpdateResult::INVALID_PARAMETER;
    
    const auto& config = configIt->second;
    
    // Validate value
    if (!validateParameterValue(id, value)) {
        return UpdateResult::VALUE_OUT_OF_RANGE;
    }
    
    // Process the value (clamp, quantize)
    float processedValue = processParameterValue(id, value);
    
    // Direct update (no smoothing)
    float oldValue = globalParameters_[static_cast<size_t>(id)].load(std::memory_order_relaxed);
    globalParameters_[static_cast<size_t>(id)].store(processedValue, std::memory_order_relaxed);
    parameterValues_[id].value = processedValue;
    parameterValues_[id].rawValue = value;
    parameterValues_[id].targetValue = processedValue;
    parameterValues_[id].hasBeenSet = true;
    parameterValues_[id].lastUpdateTime = std::chrono::steady_clock::now().time_since_epoch().count();
    
    // Update smoother to match immediate value
    auto smoother = getSmoother(id, 0);
    if (smoother) {
        smoother->setValue(processedValue);
    }
    
    // Call value changed callback
    if (config.onValueChanged) {
        config.onValueChanged(id, oldValue, processedValue);
    }
    
    return UpdateResult::SUCCESS;
}

UnifiedParameterSystem::UpdateResult UnifiedParameterSystem::setParameterWithVelocity(ParameterID id, float baseValue, float velocity) {
    if (!velocityScaling_) return setParameterValue(id, baseValue);
    
    std::lock_guard<std::mutex> lock(configMutex_);
    auto configIt = parameterConfigs_.find(id);
    if (configIt == parameterConfigs_.end()) {
        return setParameterValue(id, baseValue);
    }
    
    const auto& config = configIt->second;
    if (!config.enableVelocityScaling) {
        return setParameterValue(id, baseValue);
    }
    
    // Calculate velocity modulation
    auto scalingResult = velocityScaling_->calculateParameterScaling(
        static_cast<uint32_t>(id), velocity, baseValue);
    
    return setParameterValue(id, scalingResult.finalValue);
}

void UnifiedParameterSystem::processAudioBlock() {
    if (!initialized_.load()) return;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Update all active smoothers
    updateSmoothers();
    
    // Update CPU usage statistics
    auto endTime = std::chrono::high_resolution_clock::now();
    auto durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
    processingTimeNs_.store(durationNs.count(), std::memory_order_relaxed);
    updateCount_.fetch_add(1, std::memory_order_relaxed);
}

void UnifiedParameterSystem::updateSmoothers() {
    // Update global smoothers
    for (auto& [paramId, smoother] : globalSmoothers_) {
        if (smoother->isSmoothing()) {
            float smoothedValue = smoother->process();
            globalParameters_[static_cast<size_t>(paramId)].store(smoothedValue, std::memory_order_relaxed);
        }
    }
    
    // Update instrument smoothers
    for (size_t i = 0; i < MAX_INSTRUMENTS; ++i) {
        for (auto& [paramId, smoother] : instrumentSmoothers_[i]) {
            if (smoother->isSmoothing()) {
                float smoothedValue = smoother->process();
                instrumentParameters_[i][static_cast<size_t>(paramId)].store(smoothedValue, std::memory_order_relaxed);
            }
        }
    }
}

bool UnifiedParameterSystem::savePreset(PresetData& preset) const {
    if (!initialized_.load()) return false;
    
    std::lock_guard<std::mutex> lock(configMutex_);
    
    // Save global parameters
    preset.globalParameters.clear();
    for (const auto& [paramId, config] : parameterConfigs_) {
        if (config.isGlobalParameter) {
            float value = globalParameters_[static_cast<size_t>(paramId)].load(std::memory_order_relaxed);
            preset.globalParameters[paramId] = value;
        }
    }
    
    // Save instrument parameters
    for (size_t i = 0; i < MAX_INSTRUMENTS; ++i) {
        preset.instrumentParameters[i].clear();
        for (const auto& [paramId, config] : parameterConfigs_) {
            if (!config.isGlobalParameter) {
                float value = instrumentParameters_[i][static_cast<size_t>(paramId)].load(std::memory_order_relaxed);
                preset.instrumentParameters[i][paramId] = value;
            }
        }
    }
    
    preset.version = 1;
    return true;
}

bool UnifiedParameterSystem::loadPreset(const PresetData& preset) {
    if (!initialized_.load()) return false;
    if (!validatePreset(preset)) return false;
    
    // Load global parameters
    for (const auto& [paramId, value] : preset.globalParameters) {
        setParameterValueImmediate(paramId, value);
    }
    
    // Load instrument parameters
    for (size_t i = 0; i < MAX_INSTRUMENTS; ++i) {
        for (const auto& [paramId, value] : preset.instrumentParameters[i]) {
            setParameterValue(paramId, i, value);
        }
    }
    
    return true;
}

std::unordered_map<ParameterID, UnifiedParameterSystem::ParameterConfig> 
UnifiedParameterSystem::createDefaultConfigurations() {
    std::unordered_map<ParameterID, ParameterConfig> configs;
    
    // Helper lambda to create configs
    auto createConfig = [](ParameterID id, const std::string& name, const std::string& unit,
                          float min, float max, float defaultVal, bool isLog = false, bool isBipolar = false) {
        ParameterConfig config;
        config.id = id;
        config.name = name;
        config.displayName = name;
        config.unit = unit;
        config.minValue = min;
        config.maxValue = max;
        config.defaultValue = defaultVal;
        config.isLogarithmic = isLog;
        config.isBipolar = isBipolar;
        return config;
    };
    
    // Synthesis parameters
    configs[ParameterID::HARMONICS] = createConfig(ParameterID::HARMONICS, "Harmonics", "", 0.0f, 1.0f, 0.5f);
    configs[ParameterID::TIMBRE] = createConfig(ParameterID::TIMBRE, "Timbre", "", 0.0f, 1.0f, 0.5f);
    configs[ParameterID::MORPH] = createConfig(ParameterID::MORPH, "Morph", "", 0.0f, 1.0f, 0.5f);
    configs[ParameterID::OSC_MIX] = createConfig(ParameterID::OSC_MIX, "Osc Mix", "", 0.0f, 1.0f, 0.5f);
    configs[ParameterID::DETUNE] = createConfig(ParameterID::DETUNE, "Detune", "cents", -50.0f, 50.0f, 0.0f, false, true);
    
    // Filter parameters
    configs[ParameterID::FILTER_CUTOFF] = createConfig(ParameterID::FILTER_CUTOFF, "Cutoff", "Hz", 20.0f, 20000.0f, 1000.0f, true);
    configs[ParameterID::FILTER_RESONANCE] = createConfig(ParameterID::FILTER_RESONANCE, "Resonance", "", 0.0f, 1.0f, 0.3f);
    configs[ParameterID::FILTER_TYPE] = createConfig(ParameterID::FILTER_TYPE, "Filter Type", "", 0.0f, 3.0f, 0.0f);
    
    // Envelope parameters
    configs[ParameterID::ATTACK] = createConfig(ParameterID::ATTACK, "Attack", "ms", 1.0f, 5000.0f, 10.0f, true);
    configs[ParameterID::DECAY] = createConfig(ParameterID::DECAY, "Decay", "ms", 1.0f, 5000.0f, 300.0f, true);
    configs[ParameterID::SUSTAIN] = createConfig(ParameterID::SUSTAIN, "Sustain", "", 0.0f, 1.0f, 0.7f);
    configs[ParameterID::RELEASE] = createConfig(ParameterID::RELEASE, "Release", "ms", 1.0f, 5000.0f, 500.0f, true);
    
    // LFO parameters
    configs[ParameterID::LFO_RATE] = createConfig(ParameterID::LFO_RATE, "LFO Rate", "Hz", 0.1f, 20.0f, 1.0f, true);
    configs[ParameterID::LFO_DEPTH] = createConfig(ParameterID::LFO_DEPTH, "LFO Depth", "", 0.0f, 1.0f, 0.5f);
    configs[ParameterID::LFO_SHAPE] = createConfig(ParameterID::LFO_SHAPE, "LFO Shape", "", 0.0f, 3.0f, 0.0f);
    
    // Effects parameters
    configs[ParameterID::REVERB_SIZE] = createConfig(ParameterID::REVERB_SIZE, "Reverb Size", "", 0.0f, 1.0f, 0.5f);
    configs[ParameterID::REVERB_DAMPING] = createConfig(ParameterID::REVERB_DAMPING, "Reverb Damping", "", 0.0f, 1.0f, 0.5f);
    configs[ParameterID::REVERB_MIX] = createConfig(ParameterID::REVERB_MIX, "Reverb Mix", "", 0.0f, 1.0f, 0.3f);
    configs[ParameterID::DELAY_TIME] = createConfig(ParameterID::DELAY_TIME, "Delay Time", "ms", 1.0f, 2000.0f, 250.0f);
    configs[ParameterID::DELAY_FEEDBACK] = createConfig(ParameterID::DELAY_FEEDBACK, "Delay Feedback", "", 0.0f, 0.95f, 0.3f);
    
    // Mix parameters
    configs[ParameterID::VOLUME] = createConfig(ParameterID::VOLUME, "Volume", "dB", -60.0f, 6.0f, 0.0f);
    configs[ParameterID::PAN] = createConfig(ParameterID::PAN, "Pan", "", -1.0f, 1.0f, 0.0f, false, true);
    
    // Set velocity categories for appropriate parameters
    configs[ParameterID::FILTER_CUTOFF].velocityCategory = VelocityParameterScaling::ParameterCategory::FILTER_CUTOFF;
    configs[ParameterID::FILTER_RESONANCE].velocityCategory = VelocityParameterScaling::ParameterCategory::FILTER_RESONANCE;
    configs[ParameterID::ATTACK].velocityCategory = VelocityParameterScaling::ParameterCategory::ENVELOPE_ATTACK;
    configs[ParameterID::DECAY].velocityCategory = VelocityParameterScaling::ParameterCategory::ENVELOPE_DECAY;
    configs[ParameterID::SUSTAIN].velocityCategory = VelocityParameterScaling::ParameterCategory::ENVELOPE_SUSTAIN;
    configs[ParameterID::RELEASE].velocityCategory = VelocityParameterScaling::ParameterCategory::ENVELOPE_RELEASE;
    configs[ParameterID::VOLUME].velocityCategory = VelocityParameterScaling::ParameterCategory::VOLUME;
    
    return configs;
}

void UnifiedParameterSystem::initializeDefaultParameters() {
    auto defaultConfigs = createDefaultConfigurations();
    
    std::lock_guard<std::mutex> lock(configMutex_);
    parameterConfigs_ = std::move(defaultConfigs);
}

void UnifiedParameterSystem::setupVelocityScaling() {
    if (!velocityScaling_) return;
    
    // Configure velocity scaling for each parameter category
    for (const auto& [paramId, config] : parameterConfigs_) {
        if (config.enableVelocityScaling) {
            velocityScaling_->applyDefaultScalingForCategory(
                static_cast<uint32_t>(paramId), config.velocityCategory);
        }
    }
}

float UnifiedParameterSystem::processParameterValue(ParameterID id, float rawValue, float velocity) const {
    auto configIt = parameterConfigs_.find(id);
    if (configIt == parameterConfigs_.end()) return rawValue;
    
    const auto& config = configIt->second;
    
    // Clamp to valid range
    float processedValue = clampParameterValue(id, rawValue);
    
    // Apply quantization if needed
    if (config.stepSize > 0.0f) {
        processedValue = quantizeParameterValue(id, processedValue);
    }
    
    // Apply velocity scaling if enabled and velocity is provided
    if (config.enableVelocityScaling && velocity > 0.0f && velocityScaling_) {
        auto scalingResult = velocityScaling_->calculateParameterScaling(
            static_cast<uint32_t>(id), velocity, processedValue);
        processedValue = scalingResult.finalValue;
    }
    
    return processedValue;
}

bool UnifiedParameterSystem::validateParameterValue(ParameterID id, float value) const {
    auto configIt = parameterConfigs_.find(id);
    if (configIt == parameterConfigs_.end()) return false;
    
    const auto& config = configIt->second;
    return value >= config.minValue && value <= config.maxValue;
}

float UnifiedParameterSystem::clampParameterValue(ParameterID id, float value) const {
    auto configIt = parameterConfigs_.find(id);
    if (configIt == parameterConfigs_.end()) return value;
    
    const auto& config = configIt->second;
    return std::clamp(value, config.minValue, config.maxValue);
}

float UnifiedParameterSystem::quantizeParameterValue(ParameterID id, float value) const {
    auto configIt = parameterConfigs_.find(id);
    if (configIt == parameterConfigs_.end()) return value;
    
    const auto& config = configIt->second;
    if (config.stepSize <= 0.0f) return value;
    
    float range = config.maxValue - config.minValue;
    float normalizedValue = (value - config.minValue) / range;
    float steps = range / config.stepSize;
    float quantizedNormalized = std::round(normalizedValue * steps) / steps;
    
    return config.minValue + quantizedNormalized * range;
}

AdvancedParameterSmoother* UnifiedParameterSystem::getSmoother(ParameterID id, size_t instrumentIndex) {
    if (instrumentIndex == 0) {
        auto it = globalSmoothers_.find(id);
        return (it != globalSmoothers_.end()) ? it->second.get() : nullptr;
    } else if (instrumentIndex < MAX_INSTRUMENTS) {
        auto it = instrumentSmoothers_[instrumentIndex].find(id);
        return (it != instrumentSmoothers_[instrumentIndex].end()) ? it->second.get() : nullptr;
    }
    return nullptr;
}

const AdvancedParameterSmoother* UnifiedParameterSystem::getSmoother(ParameterID id, size_t instrumentIndex) const {
    return const_cast<UnifiedParameterSystem*>(this)->getSmoother(id, instrumentIndex);
}

bool UnifiedParameterSystem::isValidParameterID(ParameterID id) const {
    return static_cast<size_t>(id) < static_cast<size_t>(ParameterID::COUNT);
}

bool UnifiedParameterSystem::isValidInstrumentIndex(size_t index) const {
    return index < MAX_INSTRUMENTS;
}

void UnifiedParameterSystem::updateLastError(const std::string& error) const {
    lastError_ = error;
}

void UnifiedParameterSystem::recordParameterChange(ParameterID id, float value, uint64_t timestamp) {
    auto& automation = automationData_[id];
    if (!automation.isEnabled || !automation.isRecording) return;
    
    automation.recordedValues.emplace_back(timestamp, value);
    
    // Limit history size
    if (automation.recordedValues.size() > MAX_AUTOMATION_HISTORY) {
        automation.recordedValues.erase(automation.recordedValues.begin());
    }
}

bool UnifiedParameterSystem::validatePreset(const PresetData& preset) const {
    // Basic validation - check if parameters are valid
    for (const auto& [paramId, value] : preset.globalParameters) {
        if (!isValidParameterID(paramId)) {
            return false;
        }
        if (!validateParameterValue(paramId, value)) {
            return false;
        }
    }
    
    // Validate instrument parameters
    for (size_t i = 0; i < MAX_INSTRUMENTS; ++i) {
        for (const auto& [paramId, value] : preset.instrumentParameters[i]) {
            if (!isValidParameterID(paramId)) {
                return false;
            }
            if (!validateParameterValue(paramId, value)) {
                return false;
            }
        }
    }
    
    return true;
}

size_t UnifiedParameterSystem::getParameterCount() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return parameterConfigs_.size();
}

bool UnifiedParameterSystem::isParameterAutomationEnabled(ParameterID id) const {
    auto it = automationData_.find(id);
    return (it != automationData_.end()) ? it->second.isEnabled : false;
}