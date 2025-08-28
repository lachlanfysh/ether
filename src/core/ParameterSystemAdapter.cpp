#include "ParameterSystemAdapter.h"
#include <algorithm>

// Global adapter instance
ParameterSystemAdapter g_parameterAdapter;

// Initialize static members
std::unordered_map<uint32_t, ParameterID> ParameterSystemAdapter::legacyToUnifiedMap_;
std::unordered_map<ParameterID, uint32_t> ParameterSystemAdapter::unifiedToLegacyMap_;

ParameterSystemAdapter::ParameterSystemAdapter() :
    unifiedSystem_(&g_parameterSystem),
    depthControl_(std::make_unique<VelocityDepthControl>()),
    initialized_(false) {
}

ParameterSystemAdapter::~ParameterSystemAdapter() {
    shutdown();
}

bool ParameterSystemAdapter::initialize(float sampleRate) {
    if (initialized_) {
        setError("Adapter already initialized");
        return false;
    }
    
    // Initialize the unified parameter system if not already done
    if (!unifiedSystem_->isInitialized()) {
        if (!unifiedSystem_->initialize(sampleRate)) {
            setError("Failed to initialize unified parameter system");
            return false;
        }
    }
    
    // Initialize legacy parameter mapping
    initializeLegacyMapping();
    
    // Setup depth control integration
    setupDepthControlIntegration();
    
    // Generate migration recommendations
    generateMigrationRecommendations();
    
    initialized_ = true;
    return true;
}

void ParameterSystemAdapter::shutdown() {
    if (!initialized_) return;
    
    initialized_ = false;
    parameterChangeCallbacks_.clear();
    velocityScaleChangeCallbacks_.clear();
    migrationRecommendations_.clear();
}

void ParameterSystemAdapter::setParameterVelocityScale(uint32_t parameterId, float scale) {
    if (!initialized_) return;
    
    ParameterID unifiedId = legacyParameterToParameterID(parameterId);
    if (unifiedId == ParameterID::COUNT) {
        setError("Invalid legacy parameter ID: " + std::to_string(parameterId));
        return;
    }
    
    // Update the parameter configuration in the unified system
    auto config = unifiedSystem_->getParameterConfig(unifiedId);
    config.velocityScale = scale;
    config.enableVelocityScaling = (scale > 0.0f);
    
    if (!unifiedSystem_->setParameterConfig(unifiedId, config)) {
        setError("Failed to update velocity scale for parameter " + std::to_string(parameterId));
        return;
    }
    
    // Notify callbacks
    for (auto& callback : velocityScaleChangeCallbacks_) {
        callback(parameterId, 0.0f, scale); // oldValue = 0.0f since we don't track previous
    }
}

float ParameterSystemAdapter::getParameterVelocityScale(uint32_t parameterId) const {
    if (!initialized_) return 0.0f;
    
    ParameterID unifiedId = legacyParameterToParameterID(parameterId);
    if (unifiedId == ParameterID::COUNT) return 0.0f;
    
    const auto& config = unifiedSystem_->getParameterConfig(unifiedId);
    return config.velocityScale;
}

void ParameterSystemAdapter::setParameterPolarity(uint32_t parameterId, VelocityModulationUI::ModulationPolarity polarity) {
    if (!initialized_) return;
    
    ParameterID unifiedId = legacyParameterToParameterID(parameterId);
    if (unifiedId == ParameterID::COUNT) {
        setError("Invalid legacy parameter ID: " + std::to_string(parameterId));
        return;
    }
    
    // For now, we'll map polarity to the parameter configuration
    // This could be extended to support more sophisticated polarity handling
    auto config = unifiedSystem_->getParameterConfig(unifiedId);
    config.isBipolar = (polarity == VelocityModulationUI::ModulationPolarity::BIPOLAR);
    
    if (!unifiedSystem_->setParameterConfig(unifiedId, config)) {
        setError("Failed to update polarity for parameter " + std::to_string(parameterId));
    }
}

VelocityModulationUI::ModulationPolarity ParameterSystemAdapter::getParameterPolarity(uint32_t parameterId) const {
    if (!initialized_) return VelocityModulationUI::ModulationPolarity::POSITIVE;
    
    ParameterID unifiedId = legacyParameterToParameterID(parameterId);
    if (unifiedId == ParameterID::COUNT) return VelocityModulationUI::ModulationPolarity::POSITIVE;
    
    const auto& config = unifiedSystem_->getParameterConfig(unifiedId);
    return config.isBipolar ? VelocityModulationUI::ModulationPolarity::BIPOLAR 
                            : VelocityModulationUI::ModulationPolarity::POSITIVE;
}

float ParameterSystemAdapter::applyVelocityModulation(uint32_t parameterId, float baseValue, float velocity) {
    if (!initialized_) return baseValue;
    
    ParameterID unifiedId = legacyParameterToParameterID(parameterId);
    if (unifiedId == ParameterID::COUNT) return baseValue;
    
    return unifiedSystem_->calculateVelocityModulation(unifiedId, velocity);
}

void ParameterSystemAdapter::setMasterVelocityDepth(float depth) {
    if (depthControl_) {
        depthControl_->setMasterDepth(depth);
    }
}

float ParameterSystemAdapter::getMasterVelocityDepth() const {
    return depthControl_ ? depthControl_->getMasterDepth() : 1.0f;
}

void ParameterSystemAdapter::setParameterVelocityDepth(uint32_t parameterId, float depth) {
    if (depthControl_) {
        depthControl_->setParameterBaseDepth(parameterId, depth);
    }
}

float ParameterSystemAdapter::getParameterVelocityDepth(uint32_t parameterId) const {
    return depthControl_ ? depthControl_->getParameterBaseDepth(parameterId) : 1.0f;
}

ParameterID ParameterSystemAdapter::legacyParameterToParameterID(uint32_t legacyId) {
    auto it = legacyToUnifiedMap_.find(legacyId);
    return (it != legacyToUnifiedMap_.end()) ? it->second : ParameterID::COUNT;
}

uint32_t ParameterSystemAdapter::parameterIDToLegacyParameter(ParameterID paramId) {
    auto it = unifiedToLegacyMap_.find(paramId);
    return (it != unifiedToLegacyMap_.end()) ? it->second : UINT32_MAX;
}

void ParameterSystemAdapter::setParameter(uint32_t parameterId, float value) {
    if (!initialized_) return;
    
    ParameterID unifiedId = legacyParameterToParameterID(parameterId);
    if (unifiedId == ParameterID::COUNT) {
        setError("Invalid legacy parameter ID: " + std::to_string(parameterId));
        return;
    }
    
    auto result = unifiedSystem_->setParameterValue(unifiedId, value);
    if (result != UnifiedParameterSystem::UpdateResult::SUCCESS && 
        result != UnifiedParameterSystem::UpdateResult::SMOOTHING_ACTIVE) {
        setError("Failed to set parameter " + std::to_string(parameterId) + ": " + std::to_string(static_cast<int>(result)));
    }
}

void ParameterSystemAdapter::setParameter(uint32_t parameterId, size_t instrumentIndex, float value) {
    if (!initialized_) return;
    
    ParameterID unifiedId = legacyParameterToParameterID(parameterId);
    if (unifiedId == ParameterID::COUNT) {
        setError("Invalid legacy parameter ID: " + std::to_string(parameterId));
        return;
    }
    
    auto result = unifiedSystem_->setParameterValue(unifiedId, instrumentIndex, value);
    if (result != UnifiedParameterSystem::UpdateResult::SUCCESS && 
        result != UnifiedParameterSystem::UpdateResult::SMOOTHING_ACTIVE) {
        setError("Failed to set instrument parameter " + std::to_string(parameterId) + ": " + std::to_string(static_cast<int>(result)));
    }
}

float ParameterSystemAdapter::getParameter(uint32_t parameterId) const {
    if (!initialized_) return 0.0f;
    
    ParameterID unifiedId = legacyParameterToParameterID(parameterId);
    if (unifiedId == ParameterID::COUNT) return 0.0f;
    
    return unifiedSystem_->getParameterValue(unifiedId);
}

float ParameterSystemAdapter::getParameter(uint32_t parameterId, size_t instrumentIndex) const {
    if (!initialized_) return 0.0f;
    
    ParameterID unifiedId = legacyParameterToParameterID(parameterId);
    if (unifiedId == ParameterID::COUNT) return 0.0f;
    
    return unifiedSystem_->getParameterValue(unifiedId, instrumentIndex);
}

void ParameterSystemAdapter::setParameterWithVelocity(uint32_t parameterId, float baseValue, float velocity) {
    if (!initialized_) return;
    
    ParameterID unifiedId = legacyParameterToParameterID(parameterId);
    if (unifiedId == ParameterID::COUNT) {
        setError("Invalid legacy parameter ID: " + std::to_string(parameterId));
        return;
    }
    
    auto result = unifiedSystem_->setParameterWithVelocity(unifiedId, baseValue, velocity);
    if (result != UnifiedParameterSystem::UpdateResult::SUCCESS && 
        result != UnifiedParameterSystem::UpdateResult::SMOOTHING_ACTIVE) {
        setError("Failed to set parameter with velocity " + std::to_string(parameterId) + ": " + std::to_string(static_cast<int>(result)));
    }
}

void ParameterSystemAdapter::setInstrumentParameterWithVelocity(uint32_t parameterId, size_t instrumentIndex, 
                                                               float baseValue, float velocity) {
    if (!initialized_) return;
    
    // For instrument-specific velocity modulation, we need to calculate the modulation
    // and then set the parameter directly since the unified system doesn't have
    // per-instrument velocity modulation yet
    float modulatedValue = baseValue;
    if (depthControl_) {
        float depth = depthControl_->getParameterBaseDepth(parameterId);
        modulatedValue = depthControl_->applyDepthToModulation(parameterId, baseValue, velocity);
    }
    
    setParameter(parameterId, instrumentIndex, modulatedValue);
}

bool ParameterSystemAdapter::saveLegacyPreset(LegacyPresetData& preset) const {
    if (!initialized_) return false;
    
    // Convert unified preset to legacy format
    UnifiedParameterSystem::PresetData unifiedPreset;
    if (!unifiedSystem_->savePreset(unifiedPreset)) {
        return false;
    }
    
    return convertUnifiedPreset(unifiedPreset, preset);
}

bool ParameterSystemAdapter::loadLegacyPreset(const LegacyPresetData& preset) {
    if (!initialized_) return false;
    
    // Convert legacy preset to unified format
    UnifiedParameterSystem::PresetData unifiedPreset;
    if (!convertLegacyPreset(preset, unifiedPreset)) {
        return false;
    }
    
    return unifiedSystem_->loadPreset(unifiedPreset);
}

bool ParameterSystemAdapter::convertLegacyPreset(const LegacyPresetData& legacy, UnifiedParameterSystem::PresetData& unified) const {
    unified.globalParameters.clear();
    unified.presetName = legacy.presetName;
    unified.version = 1;
    
    // Convert global parameters
    for (const auto& [legacyId, value] : legacy.globalParameters) {
        ParameterID unifiedId = legacyParameterToParameterID(legacyId);
        if (unifiedId != ParameterID::COUNT) {
            unified.globalParameters[unifiedId] = value;
        }
    }
    
    // Convert instrument parameters
    for (size_t i = 0; i < MAX_INSTRUMENTS; ++i) {
        unified.instrumentParameters[i].clear();
        for (const auto& [legacyId, value] : legacy.instrumentParameters[i]) {
            ParameterID unifiedId = legacyParameterToParameterID(legacyId);
            if (unifiedId != ParameterID::COUNT) {
                unified.instrumentParameters[i][unifiedId] = value;
            }
        }
    }
    
    return true;
}

bool ParameterSystemAdapter::convertUnifiedPreset(const UnifiedParameterSystem::PresetData& unified, LegacyPresetData& legacy) const {
    legacy.globalParameters.clear();
    legacy.presetName = unified.presetName;
    
    // Convert global parameters
    for (const auto& [unifiedId, value] : unified.globalParameters) {
        uint32_t legacyId = parameterIDToLegacyParameter(unifiedId);
        if (legacyId != UINT32_MAX) {
            legacy.globalParameters[legacyId] = value;
        }
    }
    
    // Convert instrument parameters
    for (size_t i = 0; i < MAX_INSTRUMENTS; ++i) {
        legacy.instrumentParameters[i].clear();
        for (const auto& [unifiedId, value] : unified.instrumentParameters[i]) {
            uint32_t legacyId = parameterIDToLegacyParameter(unifiedId);
            if (legacyId != UINT32_MAX) {
                legacy.instrumentParameters[i][legacyId] = value;
            }
        }
    }
    
    return true;
}

void ParameterSystemAdapter::processAudioBlock() {
    if (initialized_ && unifiedSystem_) {
        unifiedSystem_->processAudioBlock();
    }
    
    if (depthControl_) {
        // Update depth control smoothing
        depthControl_->updateDepthSmoothing(1.0f / 48000.0f * BUFFER_SIZE); // Approximate delta time
    }
}

ParameterSystemAdapter::MigrationStats ParameterSystemAdapter::getMigrationStats() const {
    updateMigrationStats();
    return migrationStats_;
}

void ParameterSystemAdapter::initializeLegacyMapping() {
    // Clear existing mappings
    legacyToUnifiedMap_.clear();
    unifiedToLegacyMap_.clear();
    
    // Create mapping from legacy parameter IDs to ParameterID enum
    // This assumes legacy IDs correspond to the enum values
    for (size_t i = 0; i < static_cast<size_t>(ParameterID::COUNT); ++i) {
        ParameterID paramId = static_cast<ParameterID>(i);
        uint32_t legacyId = static_cast<uint32_t>(i);
        
        legacyToUnifiedMap_[legacyId] = paramId;
        unifiedToLegacyMap_[paramId] = legacyId;
    }
}

void ParameterSystemAdapter::setupDepthControlIntegration() {
    if (!depthControl_) return;
    
    // Integrate depth control with the velocity parameter scaling in the unified system
    // This ensures that depth changes are reflected in the unified system
    depthControl_->setDepthChangeCallback([this](uint32_t parameterId, float oldDepth, float newDepth) {
        // Update the unified system's velocity scaling configuration
        ParameterID unifiedId = legacyParameterToParameterID(parameterId);
        if (unifiedId != ParameterID::COUNT) {
            auto config = unifiedSystem_->getParameterConfig(unifiedId);
            config.velocityScale = newDepth;
            unifiedSystem_->setParameterConfig(unifiedId, config);
        }
    });
}

void ParameterSystemAdapter::updateMigrationStats() const {
    migrationStats_.totalParametersFound = legacyToUnifiedMap_.size();
    migrationStats_.parametersMigrated = unifiedSystem_->getParameterCount();
    migrationStats_.parametersWithVelocityScaling = 0;
    migrationStats_.parametersWithDepthControl = 0;
    migrationStats_.legacyAPICallsRemaining = 0; // This would need instrumentation to track
    
    // Count parameters with velocity scaling
    auto registeredParams = unifiedSystem_->getRegisteredParameters();
    for (auto paramId : registeredParams) {
        const auto& config = unifiedSystem_->getParameterConfig(paramId);
        if (config.enableVelocityScaling) {
            migrationStats_.parametersWithVelocityScaling++;
        }
        if (depthControl_ && depthControl_->hasParameterDepthConfig(parameterIDToLegacyParameter(paramId))) {
            migrationStats_.parametersWithDepthControl++;
        }
    }
    
    // Calculate completeness
    if (migrationStats_.totalParametersFound > 0) {
        migrationStats_.migrationCompleteness = 
            static_cast<float>(migrationStats_.parametersMigrated) / 
            static_cast<float>(migrationStats_.totalParametersFound);
    } else {
        migrationStats_.migrationCompleteness = 1.0f;
    }
}

void ParameterSystemAdapter::generateMigrationRecommendations() {
    migrationRecommendations_.clear();
    
    // Add recommendations for common migration patterns
    migrationRecommendations_.push_back({
        "Direct parameter access",
        "float value = someParameter; // Direct global variable access",
        "float value = g_parameterSystem.getParameterValue(ParameterID::SOME_PARAM);",
        1 // Critical
    });
    
    migrationRecommendations_.push_back({
        "Parameter updates",
        "someParameter = newValue; // Direct assignment",
        "g_parameterSystem.setParameterValue(ParameterID::SOME_PARAM, newValue);",
        1 // Critical
    });
    
    migrationRecommendations_.push_back({
        "Velocity modulation",
        "VelocityParameterScaling::calculateParameterScaling(...)",
        "g_parameterSystem.setParameterWithVelocity(...) or calculateVelocityModulation(...)",
        2 // Important
    });
    
    migrationRecommendations_.push_back({
        "Preset handling",
        "Custom preset loading/saving code",
        "Use UnifiedParameterSystem::PresetData and savePreset/loadPreset methods",
        2 // Important
    });
    
    migrationRecommendations_.push_back({
        "Parameter smoothing",
        "Manual parameter interpolation",
        "Configure smoothing in ParameterConfig and let system handle it",
        3 // Nice to have
    });
}

void ParameterSystemAdapter::setError(const std::string& error) const {
    lastError_ = error;
}

bool ParameterSystemAdapter::isValidLegacyParameterID(uint32_t parameterId) const {
    return legacyToUnifiedMap_.find(parameterId) != legacyToUnifiedMap_.end();
}