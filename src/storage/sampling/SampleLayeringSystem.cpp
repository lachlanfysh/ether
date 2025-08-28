#include "SampleLayeringSystem.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>

SampleLayeringSystem::SampleLayeringSystem() {
    config_ = LayeringConfig();
    nextLayerId_ = 0;
    nextGroupId_ = 0;
    
    rangeManager_ = nullptr;
    sampleLoader_ = nullptr;
    
    // Reserve space for efficiency
    activeLayers_.reserve(config_.maxLayers);
    mutedLayers_.reserve(config_.maxLayers);
    soloedLayers_.reserve(config_.maxLayers);
}

// Configuration
void SampleLayeringSystem::setLayeringConfig(const LayeringConfig& config) {
    if (validateGroup(LayerGroup())) {  // Basic validation using default group
        config_ = config;
        
        // Resize vectors if needed
        activeLayers_.reserve(config_.maxLayers);
        mutedLayers_.reserve(config_.maxLayers);
        soloedLayers_.reserve(config_.maxLayers);
        
        // Remove excess layers if max was reduced
        while (layers_.size() > config_.maxLayers) {
            auto it = layers_.begin();
            std::advance(it, config_.maxLayers);
            layers_.erase(it, layers_.end());
        }
        
        // Remove excess groups if max was reduced
        while (groups_.size() > config_.maxGroups) {
            auto it = groups_.begin();
            std::advance(it, config_.maxGroups);
            groups_.erase(it, groups_.end());
        }
    }
}

// Layer Management
bool SampleLayeringSystem::addLayer(const SampleLayer& layer) {
    if (!validateLayer(layer) || layers_.size() >= config_.maxLayers) {
        return false;
    }
    
    SampleLayer newLayer = layer;
    newLayer.layerId = nextLayerId_++;
    sanitizeLayerParameters(newLayer);
    
    layers_[newLayer.layerId] = newLayer;
    
    // Generate Euclidean pattern if needed
    if (newLayer.sequencingMode == LayerSequencingMode::EUCLIDEAN) {
        newLayer.stepPattern = generateEuclideanPattern(
            newLayer.euclideanSteps, newLayer.euclideanHits, newLayer.euclideanRotation);
        layers_[newLayer.layerId] = newLayer;
    }
    
    return true;
}

bool SampleLayeringSystem::removeLayer(uint8_t layerId) {
    auto it = layers_.find(layerId);
    if (it == layers_.end()) {
        return false;
    }
    
    // Remove from active, muted, and soloed lists
    activeLayers_.erase(std::remove(activeLayers_.begin(), activeLayers_.end(), layerId), activeLayers_.end());
    mutedLayers_.erase(std::remove(mutedLayers_.begin(), mutedLayers_.end(), layerId), mutedLayers_.end());
    soloedLayers_.erase(std::remove(soloedLayers_.begin(), soloedLayers_.end(), layerId), soloedLayers_.end());
    
    // Remove from any groups
    for (auto& groupPair : groups_) {
        auto& layerIds = groupPair.second.layerIds;
        layerIds.erase(std::remove(layerIds.begin(), layerIds.end(), layerId), layerIds.end());
    }
    
    layers_.erase(it);
    
    notifyLayerDeactivated(layerId);
    return true;
}

bool SampleLayeringSystem::updateLayer(uint8_t layerId, const SampleLayer& layer) {
    auto it = layers_.find(layerId);
    if (it == layers_.end() || !validateLayer(layer)) {
        return false;
    }
    
    SampleLayer updatedLayer = layer;
    updatedLayer.layerId = layerId;  // Ensure ID consistency
    sanitizeLayerParameters(updatedLayer);
    
    // Preserve current state
    updatedLayer.isActive = it->second.isActive;
    updatedLayer.currentStep = it->second.currentStep;
    updatedLayer.currentGain = it->second.currentGain;
    updatedLayer.activationTime = it->second.activationTime;
    
    layers_[layerId] = updatedLayer;
    
    // Regenerate Euclidean pattern if parameters changed
    if (updatedLayer.sequencingMode == LayerSequencingMode::EUCLIDEAN) {
        layers_[layerId].stepPattern = generateEuclideanPattern(
            updatedLayer.euclideanSteps, updatedLayer.euclideanHits, updatedLayer.euclideanRotation);
    }
    
    return true;
}

void SampleLayeringSystem::clearAllLayers() {
    layers_.clear();
    activeLayers_.clear();
    mutedLayers_.clear();
    soloedLayers_.clear();
    nextLayerId_ = 0;
    
    // Clear groups
    for (auto& groupPair : groups_) {
        groupPair.second.layerIds.clear();
    }
}

SampleLayeringSystem::SampleLayer SampleLayeringSystem::getLayer(uint8_t layerId) const {
    auto it = layers_.find(layerId);
    return (it != layers_.end()) ? it->second : SampleLayer();
}

std::vector<uint8_t> SampleLayeringSystem::getAllLayerIds() const {
    std::vector<uint8_t> layerIds;
    for (const auto& pair : layers_) {
        layerIds.push_back(pair.first);
    }
    return layerIds;
}

uint8_t SampleLayeringSystem::getLayerCount() const {
    return static_cast<uint8_t>(layers_.size());
}

// Layer Group Management
bool SampleLayeringSystem::createGroup(const LayerGroup& group) {
    if (!validateGroup(group) || groups_.size() >= config_.maxGroups) {
        return false;
    }
    
    LayerGroup newGroup = group;
    newGroup.groupId = nextGroupId_++;
    
    groups_[newGroup.groupId] = newGroup;
    return true;
}

bool SampleLayeringSystem::removeGroup(uint8_t groupId) {
    return groups_.erase(groupId) > 0;
}

bool SampleLayeringSystem::updateGroup(uint8_t groupId, const LayerGroup& group) {
    auto it = groups_.find(groupId);
    if (it == groups_.end() || !validateGroup(group)) {
        return false;
    }
    
    LayerGroup updatedGroup = group;
    updatedGroup.groupId = groupId;
    groups_[groupId] = updatedGroup;
    
    return true;
}

bool SampleLayeringSystem::addLayerToGroup(uint8_t layerId, uint8_t groupId) {
    auto layerIt = layers_.find(layerId);
    auto groupIt = groups_.find(groupId);
    
    if (layerIt == layers_.end() || groupIt == groups_.end()) {
        return false;
    }
    
    auto& layerIds = groupIt->second.layerIds;
    if (std::find(layerIds.begin(), layerIds.end(), layerId) == layerIds.end()) {
        layerIds.push_back(layerId);
    }
    
    return true;
}

bool SampleLayeringSystem::removeLayerFromGroup(uint8_t layerId, uint8_t groupId) {
    auto groupIt = groups_.find(groupId);
    if (groupIt == groups_.end()) {
        return false;
    }
    
    auto& layerIds = groupIt->second.layerIds;
    layerIds.erase(std::remove(layerIds.begin(), layerIds.end(), layerId), layerIds.end());
    
    return true;
}

std::vector<SampleLayeringSystem::LayerGroup> SampleLayeringSystem::getAllGroups() const {
    std::vector<LayerGroup> groups;
    for (const auto& pair : groups_) {
        groups.push_back(pair.second);
    }
    return groups;
}

// Layer Activation
SampleLayeringSystem::LayerActivationResult SampleLayeringSystem::activateLayers(
    float velocity, uint8_t midiNote, uint8_t currentStep) {
    
    LayerActivationResult result;
    
    // Clear previous activations
    activeLayers_.clear();
    
    // Check each layer for activation
    for (auto& layerPair : layers_) {
        SampleLayer& layer = layerPair.second;
        uint8_t layerId = layerPair.first;
        
        if (shouldActivateLayer(layer, velocity, midiNote, currentStep) && !isEffectivelyMuted(layerId)) {
            activeLayers_.push_back(layerId);
            layer.isActive = true;
            layer.activationTime = getCurrentTimeMs();
            layer.targetGain = layer.layerGain;
            
            // Add to result
            result.activatedLayers.push_back(layerId);
            result.sampleSlots.push_back(layer.sampleSlot);
            result.layerGains.push_back(layer.layerGain);
            result.layerDelays.push_back(layer.layerDelay);
            
            notifyLayerActivated(layerId, velocity);
        } else {
            layer.isActive = false;
        }
    }
    
    // Calculate gain compensation
    if (config_.enableAutoGainCompensation && !result.activatedLayers.empty()) {
        result.totalGainCompensation = calculateAutoGainCompensation(result.activatedLayers.size());
        
        // Apply compensation to layer gains
        for (float& gain : result.layerGains) {
            gain *= result.totalGainCompensation;
        }
    }
    
    // Determine effective blend mode
    if (!result.activatedLayers.empty()) {
        // Use blend mode from first active layer (could be more sophisticated)
        const SampleLayer& firstLayer = layers_[result.activatedLayers[0]];
        result.effectiveBlendMode = firstLayer.blendMode;
    }
    
    return result;
}

void SampleLayeringSystem::updateLayerStates(uint8_t currentStep) {
    updateLayerSequencing(currentStep);
    
    // Update parameter smoothing for active layers
    for (uint8_t layerId : activeLayers_) {
        auto it = layers_.find(layerId);
        if (it != layers_.end()) {
            updateParameterSmoothing(it->second);
        }
    }
}

void SampleLayeringSystem::deactivateAllLayers() {
    for (auto& layerPair : layers_) {
        if (layerPair.second.isActive) {
            layerPair.second.isActive = false;
            layerPair.second.targetGain = 0.0f;
            notifyLayerDeactivated(layerPair.first);
        }
    }
    activeLayers_.clear();
}

void SampleLayeringSystem::deactivateLayer(uint8_t layerId) {
    auto it = layers_.find(layerId);
    if (it != layers_.end() && it->second.isActive) {
        it->second.isActive = false;
        it->second.targetGain = 0.0f;
        
        activeLayers_.erase(std::remove(activeLayers_.begin(), activeLayers_.end(), layerId), 
                           activeLayers_.end());
        
        notifyLayerDeactivated(layerId);
    }
}

// Sequencing Control
void SampleLayeringSystem::updateLayerSequencing(uint8_t currentStep) {
    for (auto& layerPair : layers_) {
        SampleLayer& layer = layerPair.second;
        
        switch (layer.sequencingMode) {
            case LayerSequencingMode::SHARED_PATTERN:
                // Nothing to update - follows main pattern
                break;
                
            case LayerSequencingMode::INDEPENDENT_STEPS:
                updateIndependentSequencing(layer, currentStep);
                break;
                
            case LayerSequencingMode::OFFSET_PATTERN:
                updateOffsetSequencing(layer, currentStep);
                break;
                
            case LayerSequencingMode::POLYRHYTHM:
                updatePolyrhythmSequencing(layer, currentStep);
                break;
                
            case LayerSequencingMode::EUCLIDEAN:
                updateEuclideanSequencing(layer, currentStep);
                break;
                
            case LayerSequencingMode::PROBABILITY_STEPS:
                // Handled in shouldActivateLayer
                break;
                
            case LayerSequencingMode::CONDITIONAL_STEPS:
                // Complex logic would be implemented here
                break;
        }
    }
}

void SampleLayeringSystem::resetLayerSequencing() {
    for (auto& layerPair : layers_) {
        layerPair.second.currentStep = 0;
    }
}

bool SampleLayeringSystem::isLayerActiveAtStep(uint8_t layerId, uint8_t step) const {
    auto it = layers_.find(layerId);
    if (it == layers_.end()) {
        return false;
    }
    
    const SampleLayer& layer = it->second;
    
    switch (layer.sequencingMode) {
        case LayerSequencingMode::SHARED_PATTERN:
            return true;  // Follows main pattern
            
        case LayerSequencingMode::INDEPENDENT_STEPS:
        case LayerSequencingMode::EUCLIDEAN:
            if (step < layer.stepPattern.size()) {
                return layer.stepPattern[step];
            }
            return false;
            
        case LayerSequencingMode::OFFSET_PATTERN: {
            int16_t offsetStep = step + layer.patternOffset;
            if (offsetStep >= 0 && offsetStep < static_cast<int16_t>(layer.stepPattern.size())) {
                return layer.stepPattern[offsetStep];
            }
            return false;
        }
        
        case LayerSequencingMode::POLYRHYTHM: {
            uint8_t adjustedStep = step % layer.patternLength;
            return (adjustedStep < layer.stepPattern.size()) ? layer.stepPattern[adjustedStep] : false;
        }
        
        default:
            return true;
    }
}

void SampleLayeringSystem::setLayerStepPattern(uint8_t layerId, const std::vector<bool>& pattern) {
    auto it = layers_.find(layerId);
    if (it != layers_.end()) {
        it->second.stepPattern = pattern;
        it->second.patternLength = static_cast<uint8_t>(std::min(pattern.size(), size_t(MAX_PATTERN_LENGTH)));
        
        notifyPatternUpdated(layerId, pattern);
    }
}

std::vector<bool> SampleLayeringSystem::getLayerStepPattern(uint8_t layerId) const {
    auto it = layers_.find(layerId);
    return (it != layers_.end()) ? it->second.stepPattern : std::vector<bool>();
}

// Parameter Control
void SampleLayeringSystem::setLayerGain(uint8_t layerId, float gain) {
    auto it = layers_.find(layerId);
    if (it != layers_.end()) {
        it->second.layerGain = std::max(0.0f, std::min(gain, 4.0f));  // 0 to +12dB
        it->second.targetGain = it->second.layerGain;
        notifyLayerParameterChanged(layerId, "gain", it->second.layerGain);
    }
}

void SampleLayeringSystem::setLayerPan(uint8_t layerId, float pan) {
    auto it = layers_.find(layerId);
    if (it != layers_.end()) {
        it->second.layerPan = std::max(-1.0f, std::min(pan, 1.0f));
        notifyLayerParameterChanged(layerId, "pan", it->second.layerPan);
    }
}

void SampleLayeringSystem::setLayerBlendAmount(uint8_t layerId, float amount) {
    auto it = layers_.find(layerId);
    if (it != layers_.end()) {
        it->second.blendAmount = std::max(0.0f, std::min(amount, 1.0f));
        notifyLayerParameterChanged(layerId, "blend_amount", it->second.blendAmount);
    }
}

void SampleLayeringSystem::setGroupGain(uint8_t groupId, float gain) {
    auto it = groups_.find(groupId);
    if (it != groups_.end()) {
        it->second.groupGain = std::max(0.0f, std::min(gain, 4.0f));
    }
}

void SampleLayeringSystem::setGroupPan(uint8_t groupId, float pan) {
    auto it = groups_.find(groupId);
    if (it != groups_.end()) {
        it->second.groupPan = std::max(-1.0f, std::min(pan, 1.0f));
    }
}

// Mute/Solo Control
void SampleLayeringSystem::muteLayer(uint8_t layerId, bool mute) {
    if (mute) {
        if (std::find(mutedLayers_.begin(), mutedLayers_.end(), layerId) == mutedLayers_.end()) {
            mutedLayers_.push_back(layerId);
        }
    } else {
        mutedLayers_.erase(std::remove(mutedLayers_.begin(), mutedLayers_.end(), layerId), 
                          mutedLayers_.end());
    }
    
    updateMuteSoloStates();
}

void SampleLayeringSystem::soloLayer(uint8_t layerId, bool solo) {
    if (solo) {
        if (std::find(soloedLayers_.begin(), soloedLayers_.end(), layerId) == soloedLayers_.end()) {
            soloedLayers_.push_back(layerId);
        }
    } else {
        soloedLayers_.erase(std::remove(soloedLayers_.begin(), soloedLayers_.end(), layerId), 
                           soloedLayers_.end());
    }
    
    updateMuteSoloStates();
}

bool SampleLayeringSystem::isLayerMuted(uint8_t layerId) const {
    return std::find(mutedLayers_.begin(), mutedLayers_.end(), layerId) != mutedLayers_.end();
}

bool SampleLayeringSystem::isLayerSoloed(uint8_t layerId) const {
    return std::find(soloedLayers_.begin(), soloedLayers_.end(), layerId) != soloedLayers_.end();
}

// Euclidean Rhythm Generation
std::vector<bool> SampleLayeringSystem::generateEuclideanPattern(uint8_t steps, uint8_t hits, uint8_t rotation) const {
    std::vector<bool> pattern(steps, false);
    
    if (hits == 0 || hits > steps) {
        return pattern;
    }
    
    // Simplified Euclidean rhythm generation using basic distribution
    // For perfect Bjorklund's algorithm, this could be more complex
    // but this provides good Euclidean-like distribution
    
    if (hits == steps) {
        // All steps are hits
        for (size_t i = 0; i < steps; ++i) {
            pattern[i] = true;
        }
        return pattern;
    }
    
    // Distribute hits as evenly as possible
    float stepSize = static_cast<float>(steps) / static_cast<float>(hits);
    for (uint8_t hit = 0; hit < hits; ++hit) {
        size_t position = static_cast<size_t>(hit * stepSize) % steps;
        pattern[position] = true;
    }
    
    // Apply rotation if specified
    if (rotation > 0) {
        std::vector<bool> rotatedPattern(steps, false);
        for (size_t i = 0; i < steps; ++i) {
            size_t rotatedIndex = (i + rotation) % steps;
            rotatedPattern[rotatedIndex] = pattern[i];
        }
        pattern = rotatedPattern;
    }
    
    return pattern;
}

void SampleLayeringSystem::setLayerEuclideanPattern(uint8_t layerId, uint8_t steps, uint8_t hits, uint8_t rotation) {
    auto it = layers_.find(layerId);
    if (it != layers_.end()) {
        it->second.euclideanSteps = steps;
        it->second.euclideanHits = hits;
        it->second.euclideanRotation = rotation;
        it->second.stepPattern = generateEuclideanPattern(steps, hits, rotation);
        it->second.patternLength = steps;
        
        notifyPatternUpdated(layerId, it->second.stepPattern);
    }
}

// Internal methods
bool SampleLayeringSystem::shouldActivateLayer(const SampleLayer& layer, float velocity, uint8_t midiNote, uint8_t currentStep) const {
    switch (layer.activationMode) {
        case LayerActivationMode::ALWAYS_ACTIVE:
            return true;
            
        case LayerActivationMode::VELOCITY_GATED:
            return evaluateVelocityGating(layer, velocity);
            
        case LayerActivationMode::PITCH_GATED:
            return evaluatePitchGating(layer, midiNote);
            
        case LayerActivationMode::PROBABILITY:
            return evaluateProbabilityGating(layer);
            
        case LayerActivationMode::STEP_SEQUENCED:
            return evaluateStepSequencing(layer, currentStep);
            
        case LayerActivationMode::ENVELOPE_GATED:
        case LayerActivationMode::MODULATION_GATED:
        case LayerActivationMode::CONDITIONAL:
            // Complex logic would be implemented here
            return true;
            
        default:
            return true;
    }
}

bool SampleLayeringSystem::evaluateVelocityGating(const SampleLayer& layer, float velocity) const {
    return velocity >= layer.velocityThreshold && velocity <= layer.velocityMax;
}

bool SampleLayeringSystem::evaluatePitchGating(const SampleLayer& layer, uint8_t midiNote) const {
    return midiNote >= layer.pitchMin && midiNote <= layer.pitchMax;
}

bool SampleLayeringSystem::evaluateProbabilityGating(const SampleLayer& layer) const {
    return generateRandomFloat() <= layer.probability;
}

bool SampleLayeringSystem::evaluateStepSequencing(const SampleLayer& layer, uint8_t currentStep) const {
    return isLayerActiveAtStep(layer.layerId, currentStep);
}

// Sequencing helpers
void SampleLayeringSystem::updateIndependentSequencing(SampleLayer& layer, uint8_t currentStep) {
    layer.currentStep = currentStep % layer.patternLength;
}

void SampleLayeringSystem::updateOffsetSequencing(SampleLayer& layer, uint8_t currentStep) {
    int16_t adjustedStep = currentStep + layer.patternOffset;
    if (adjustedStep < 0) {
        adjustedStep += layer.patternLength;
    }
    layer.currentStep = adjustedStep % layer.patternLength;
}

void SampleLayeringSystem::updatePolyrhythmSequencing(SampleLayer& layer, uint8_t currentStep) {
    layer.currentStep = currentStep % layer.patternLength;
}

void SampleLayeringSystem::updateEuclideanSequencing(SampleLayer& layer, uint8_t currentStep) {
    layer.currentStep = currentStep % layer.euclideanSteps;
}

// Parameter smoothing
void SampleLayeringSystem::updateParameterSmoothing(SampleLayer& layer) {
    smoothParameter(layer.currentGain, layer.targetGain, config_.parameterSmoothingTime);
}

void SampleLayeringSystem::smoothParameter(float& current, float target, float smoothingTime) {
    float rate = 1.0f / (smoothingTime * 0.001f * 48000.0f);  // Assume 48kHz
    float diff = target - current;
    current += diff * std::min(1.0f, rate);
}

// Gain compensation
float SampleLayeringSystem::calculateAutoGainCompensation(uint8_t activeLayerCount) const {
    if (activeLayerCount <= 1) {
        return 1.0f;
    }
    
    // Simple compensation based on layer count
    return 1.0f / std::sqrt(static_cast<float>(activeLayerCount));
}

// Mute/Solo logic
bool SampleLayeringSystem::isEffectivelyMuted(uint8_t layerId) const {
    // If any layers are soloed and this layer is not soloed, it's effectively muted
    if (!soloedLayers_.empty() && !isLayerSoloed(layerId)) {
        return true;
    }
    
    // Otherwise, check if explicitly muted
    return isLayerMuted(layerId);
}

void SampleLayeringSystem::updateMuteSoloStates() {
    // This would trigger UI updates and other state changes
}

// Validation helpers
bool SampleLayeringSystem::validateLayer(const SampleLayer& layer) const {
    return layer.velocityThreshold >= 0.0f && layer.velocityThreshold <= 1.0f &&
           layer.velocityMax >= layer.velocityThreshold && layer.velocityMax <= 1.0f &&
           layer.pitchMin <= layer.pitchMax &&
           layer.probability >= 0.0f && layer.probability <= 1.0f &&
           layer.playbackRate >= MIN_PLAYBACK_RATE && layer.playbackRate <= MAX_PLAYBACK_RATE &&
           layer.layerDelay >= 0.0f && layer.layerDelay <= MAX_LAYER_DELAY_MS &&
           layer.patternLength > 0 && layer.patternLength <= MAX_PATTERN_LENGTH;
}

bool SampleLayeringSystem::validateGroup(const LayerGroup& group) const {
    return true;  // Basic validation - could be expanded
}

void SampleLayeringSystem::sanitizeLayerParameters(SampleLayer& layer) const {
    layer.velocityThreshold = std::max(0.0f, std::min(layer.velocityThreshold, 1.0f));
    layer.velocityMax = std::max(layer.velocityThreshold, std::min(layer.velocityMax, 1.0f));
    layer.pitchMin = std::min(layer.pitchMin, static_cast<uint8_t>(127));
    layer.pitchMax = std::max(layer.pitchMin, std::min(layer.pitchMax, static_cast<uint8_t>(127)));
    layer.probability = std::max(0.0f, std::min(layer.probability, 1.0f));
    layer.playbackRate = std::max(MIN_PLAYBACK_RATE, std::min(layer.playbackRate, MAX_PLAYBACK_RATE));
    layer.layerDelay = std::max(0.0f, std::min(layer.layerDelay, MAX_LAYER_DELAY_MS));
    layer.patternLength = std::max(static_cast<uint8_t>(1), std::min(layer.patternLength, static_cast<uint8_t>(MAX_PATTERN_LENGTH)));
}

// Notification helpers
void SampleLayeringSystem::notifyLayerActivated(uint8_t layerId, float velocity) {
    if (layerActivatedCallback_) {
        layerActivatedCallback_(layerId, velocity);
    }
}

void SampleLayeringSystem::notifyLayerDeactivated(uint8_t layerId) {
    if (layerDeactivatedCallback_) {
        layerDeactivatedCallback_(layerId);
    }
}

void SampleLayeringSystem::notifyLayerParameterChanged(uint8_t layerId, const std::string& parameter, float value) {
    if (layerParameterChangedCallback_) {
        layerParameterChangedCallback_(layerId, parameter, value);
    }
}

void SampleLayeringSystem::notifyPatternUpdated(uint8_t layerId, const std::vector<bool>& pattern) {
    if (patternUpdatedCallback_) {
        patternUpdatedCallback_(layerId, pattern);
    }
}

// Utility methods
uint32_t SampleLayeringSystem::getCurrentTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

float SampleLayeringSystem::generateRandomFloat() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    return dis(gen);
}

// Performance Analysis
size_t SampleLayeringSystem::getEstimatedMemoryUsage() const {
    return sizeof(SampleLayeringSystem) + 
           layers_.size() * sizeof(SampleLayer) +
           groups_.size() * sizeof(LayerGroup) +
           presets_.size() * sizeof(LayeringPreset);
}

uint8_t SampleLayeringSystem::getActiveLayerCount() const {
    return static_cast<uint8_t>(activeLayers_.size());
}

float SampleLayeringSystem::getCombinedProcessingLoad() const {
    return static_cast<float>(activeLayers_.size()) / static_cast<float>(config_.maxLayers);
}

// Integration
void SampleLayeringSystem::integrateWithVelocityPitchRangeManager(VelocityPitchRangeManager* rangeManager) {
    rangeManager_ = rangeManager;
}

void SampleLayeringSystem::integrateWithAutoSampleLoader(AutoSampleLoader* sampleLoader) {
    sampleLoader_ = sampleLoader;
}

void SampleLayeringSystem::setSampleAccessCallback(std::function<const AutoSampleLoader::SamplerSlot&(uint8_t)> callback) {
    sampleAccessCallback_ = callback;
}

// Callbacks
void SampleLayeringSystem::setLayerActivatedCallback(LayerActivatedCallback callback) {
    layerActivatedCallback_ = callback;
}

void SampleLayeringSystem::setLayerDeactivatedCallback(LayerDeactivatedCallback callback) {
    layerDeactivatedCallback_ = callback;
}

void SampleLayeringSystem::setLayerParameterChangedCallback(LayerParameterChangedCallback callback) {
    layerParameterChangedCallback_ = callback;
}

void SampleLayeringSystem::setPatternUpdatedCallback(PatternUpdatedCallback callback) {
    patternUpdatedCallback_ = callback;
}