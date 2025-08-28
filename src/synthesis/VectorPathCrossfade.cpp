#include "VectorPathCrossfade.h"
#include <cmath>
#include <algorithm>
#include <string>
#include <unordered_map>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef STM32H7
#include "stm32h7xx_hal.h"
#else
#include <chrono>
#endif

VectorPathCrossfade::VectorPathCrossfade() {
    // Initialize default configuration
    config_.mode = CrossfadeMode::EQUAL_POWER;
    config_.transitionTime = 0.05f;
    config_.preserveVoices = true;
    config_.snapToCorners = false;
    config_.cornerSnapThreshold = 0.05f;
    config_.morphCurve = 0.5f;
    config_.usePerceptualBlending = true;
    config_.enableInterpolation = true;
    config_.interpolationQuality = 2;
    config_.realTimeUpdate = true;
    
    // Initialize state
    lastPosition_ = VectorPath::Position(0.5f, 0.5f);
    lastBlend_.weights.fill(0.25f);
    engineLevels_.fill(0.0f);
    totalEngineLevel_ = 1.0f;
    totalVoiceCount_ = 0;
    anyTransitionActive_ = false;
    lastUpdateTime_ = 0;
    
    // Initialize preset morph
    presetMorph_.active = false;
    presetMorph_.progress = 0.0f;
    presetMorph_.duration = 1.0f;
    presetMorph_.startTime = 0;
    
    // Initialize corner engines
    initializeCornerEngines();
}

VectorPathCrossfade::~VectorPathCrossfade() {
    shutdown();
}

bool VectorPathCrossfade::initialize(VectorPath* vectorPath) {
    if (initialized_) {
        return true;
    }
    
    vectorPath_ = vectorPath;
    
    if (!vectorPath_) {
        return false;
    }
    
    // Set up VectorPath callbacks
    vectorPath_->setPositionChangeCallback([this](const VectorPath::Position& pos, const VectorPath::CornerBlend& blend) {
        updateCrossfade(pos, blend);
    });
    
    initialized_ = true;
    return true;
}

void VectorPathCrossfade::shutdown() {
    if (!initialized_) {
        return;
    }
    
    // Cancel all transitions
    cancelAllTransitions();
    
    // Deactivate all engines
    for (auto& corner : cornerEngines_) {
        deactivateEngine(static_cast<VectorPath::Corner>(&corner - cornerEngines_.data()));
    }
    
    vectorPath_ = nullptr;
    initialized_ = false;
}

void VectorPathCrossfade::initializeCornerEngines() {
    // Initialize default engine assignments
    cornerEngines_[0].engineType = EngineType::MACRO_VA;        // Corner A
    cornerEngines_[1].engineType = EngineType::MACRO_FM;        // Corner B
    cornerEngines_[2].engineType = EngineType::MACRO_WAVESHAPER;// Corner C
    cornerEngines_[3].engineType = EngineType::MACRO_WAVETABLE;// Corner D
    
    for (int i = 0; i < 4; i++) {
        CornerEngine& corner = cornerEngines_[i];
        corner.active = false;
        corner.level = 1.0f;
        corner.crossfadeAmount = 0.0f;
        
        // Initialize H/T/M parameters to center
        corner.harmonics = 0.5f;
        corner.timbre = 0.5f;
        corner.morph = 0.5f;
        
        // Initialize extended parameters
        corner.engineParams.fill(0.5f);
        
        // Voice management
        corner.voiceCount = 0;
        corner.maxVoices = 16;
        corner.voiceSteal = true;
        
        // Initialize transition state
        activeTransitions_[i].active = false;
        activeTransitions_[i].progress = 0.0f;
        activeTransitions_[i].corner = static_cast<VectorPath::Corner>(i);
    }
}

void VectorPathCrossfade::setCornerEngine(VectorPath::Corner corner, EngineType engineType) {
    int cornerIndex = static_cast<int>(corner);
    if (cornerIndex < 0 || cornerIndex >= 4) {
        return;
    }
    
    CornerEngine& cornerEngine = cornerEngines_[cornerIndex];
    
    if (cornerEngine.engineType == engineType) {
        return; // No change needed
    }
    
    // Start transition to new engine
    transitionCornerEngine(corner, engineType, config_.transitionTime);
}

VectorPathCrossfade::EngineType VectorPathCrossfade::getCornerEngine(VectorPath::Corner corner) const {
    int cornerIndex = static_cast<int>(corner);
    if (cornerIndex < 0 || cornerIndex >= 4) {
        return EngineType::MACRO_VA;
    }
    
    return cornerEngines_[cornerIndex].engineType;
}

void VectorPathCrossfade::setCornerEngineParams(VectorPath::Corner corner, float harmonics, float timbre, float morph) {
    int cornerIndex = static_cast<int>(corner);
    if (cornerIndex < 0 || cornerIndex >= 4) {
        return;
    }
    
    CornerEngine& cornerEngine = cornerEngines_[cornerIndex];
    cornerEngine.harmonics = std::clamp(harmonics, 0.0f, 1.0f);
    cornerEngine.timbre = std::clamp(timbre, 0.0f, 1.0f);
    cornerEngine.morph = std::clamp(morph, 0.0f, 1.0f);
    
    // Update engine if active
    if (cornerEngine.engine && cornerEngine.active) {
        cornerEngine.engine->setHTMParameters(cornerEngine.harmonics, cornerEngine.timbre, cornerEngine.morph);
    }
}

void VectorPathCrossfade::getCornerEngineParams(VectorPath::Corner corner, float& harmonics, float& timbre, float& morph) const {
    int cornerIndex = static_cast<int>(corner);
    if (cornerIndex < 0 || cornerIndex >= 4) {
        harmonics = timbre = morph = 0.5f;
        return;
    }
    
    const CornerEngine& cornerEngine = cornerEngines_[cornerIndex];
    harmonics = cornerEngine.harmonics;
    timbre = cornerEngine.timbre;
    morph = cornerEngine.morph;
}

std::shared_ptr<VectorPathCrossfade::SynthEngineBase> VectorPathCrossfade::getEngine(VectorPath::Corner corner) const {
    int cornerIndex = static_cast<int>(corner);
    if (cornerIndex < 0 || cornerIndex >= 4) {
        return nullptr;
    }
    
    return cornerEngines_[cornerIndex].engine;
}

void VectorPathCrossfade::setEngine(VectorPath::Corner corner, std::shared_ptr<SynthEngineBase> engine) {
    int cornerIndex = static_cast<int>(corner);
    if (cornerIndex < 0 || cornerIndex >= 4) {
        return;
    }
    
    CornerEngine& cornerEngine = cornerEngines_[cornerIndex];
    
    // Deactivate old engine
    if (cornerEngine.engine) {
        cornerEngine.engine->deactivate();
    }
    
    // Set new engine
    cornerEngine.engine = engine;
    
    if (engine) {
        // Update engine type to match
        cornerEngine.engineType = engine->getEngineType();
        
        // Apply current parameters
        engine->setHTMParameters(cornerEngine.harmonics, cornerEngine.timbre, cornerEngine.morph);
        
        // Activate if corner is active
        if (cornerEngine.active) {
            engine->activate();
        }
    }
}

void VectorPathCrossfade::setCrossfadeConfig(const CrossfadeConfig& config) {
    config_ = config;
    
    // Clamp values to valid ranges
    config_.transitionTime = std::clamp(config_.transitionTime, MIN_CROSSFADE_TIME, MAX_CROSSFADE_TIME);
    config_.cornerSnapThreshold = std::clamp(config_.cornerSnapThreshold, 0.01f, 0.2f);
    config_.morphCurve = std::clamp(config_.morphCurve, 0.0f, 1.0f);
    config_.interpolationQuality = std::clamp(config_.interpolationQuality, 1, 3);
}

void VectorPathCrossfade::transitionCornerEngine(VectorPath::Corner corner, EngineType newEngine, float transitionTime) {
    int cornerIndex = static_cast<int>(corner);
    if (cornerIndex < 0 || cornerIndex >= 4) {
        return;
    }
    
    CornerEngine& cornerEngine = cornerEngines_[cornerIndex];
    EngineTransition& transition = activeTransitions_[cornerIndex];
    
    if (cornerEngine.engineType == newEngine) {
        return; // No transition needed
    }
    
    // Set up transition
    transition.corner = corner;
    transition.fromEngine = cornerEngine.engineType;
    transition.toEngine = newEngine;
    transition.progress = 0.0f;
    transition.startTime = getTimeMs() * 0.001f;
    transition.active = true;
    
    // Use provided time or default from config
    float duration = (transitionTime > 0.0f) ? transitionTime : config_.transitionTime;
    startEngineTransition(corner, newEngine, duration);
    
    anyTransitionActive_ = true;
    
    // Notify callback
    if (engineChangeCallback_) {
        engineChangeCallback_(corner, cornerEngine.engineType, newEngine);
    }
}

bool VectorPathCrossfade::isTransitionActive(VectorPath::Corner corner) const {
    int cornerIndex = static_cast<int>(corner);
    if (cornerIndex < 0 || cornerIndex >= 4) {
        return false;
    }
    
    return activeTransitions_[cornerIndex].active;
}

float VectorPathCrossfade::getTransitionProgress(VectorPath::Corner corner) const {
    int cornerIndex = static_cast<int>(corner);
    if (cornerIndex < 0 || cornerIndex >= 4) {
        return 0.0f;
    }
    
    return activeTransitions_[cornerIndex].progress;
}

void VectorPathCrossfade::cancelTransition(VectorPath::Corner corner) {
    int cornerIndex = static_cast<int>(corner);
    if (cornerIndex < 0 || cornerIndex >= 4) {
        return;
    }
    
    activeTransitions_[cornerIndex].active = false;
    activeTransitions_[cornerIndex].progress = 0.0f;
    
    // Check if any transitions are still active
    anyTransitionActive_ = false;
    for (const auto& transition : activeTransitions_) {
        if (transition.active) {
            anyTransitionActive_ = true;
            break;
        }
    }
}

void VectorPathCrossfade::cancelAllTransitions() {
    for (int i = 0; i < 4; i++) {
        activeTransitions_[i].active = false;
        activeTransitions_[i].progress = 0.0f;
    }
    anyTransitionActive_ = false;
}

void VectorPathCrossfade::morphToPreset(const std::array<EngineType, 4>& targetEngines,
                                       const std::array<std::array<float, 3>, 4>& targetParams,
                                       float morphTime) {
    // Set up preset morph
    presetMorph_.active = true;
    presetMorph_.progress = 0.0f;
    presetMorph_.duration = std::max(morphTime, 0.1f);
    presetMorph_.startTime = getTimeMs();
    
    // Store current state as start point
    for (int i = 0; i < 4; i++) {
        presetMorph_.startEngines[i] = cornerEngines_[i].engineType;
        presetMorph_.startParams[i][0] = cornerEngines_[i].harmonics;
        presetMorph_.startParams[i][1] = cornerEngines_[i].timbre;
        presetMorph_.startParams[i][2] = cornerEngines_[i].morph;
    }
    
    // Store target state
    presetMorph_.targetEngines = targetEngines;
    presetMorph_.targetParams = targetParams;
}

void VectorPathCrossfade::setMaxVoices(VectorPath::Corner corner, int maxVoices) {
    int cornerIndex = static_cast<int>(corner);
    if (cornerIndex < 0 || cornerIndex >= 4) {
        return;
    }
    
    cornerEngines_[cornerIndex].maxVoices = std::clamp(maxVoices, 1, 64);
    
    // Reallocate voices if necessary
    reallocateVoices();
}

int VectorPathCrossfade::getActiveVoices(VectorPath::Corner corner) const {
    int cornerIndex = static_cast<int>(corner);
    if (cornerIndex < 0 || cornerIndex >= 4) {
        return 0;
    }
    
    return cornerEngines_[cornerIndex].voiceCount;
}

int VectorPathCrossfade::getTotalActiveVoices() const {
    return totalVoiceCount_;
}

void VectorPathCrossfade::setVoiceStealingEnabled(VectorPath::Corner corner, bool enabled) {
    int cornerIndex = static_cast<int>(corner);
    if (cornerIndex < 0 || cornerIndex >= 4) {
        return;
    }
    
    cornerEngines_[cornerIndex].voiceSteal = enabled;
}

void VectorPathCrossfade::updateCrossfade(const VectorPath::Position& position, const VectorPath::CornerBlend& blend) {
    lastPosition_ = position;
    lastBlend_ = blend;
    
    // Update engine blending
    updateEngineBlending();
    
    // Handle corner snapping
    if (config_.snapToCorners) {
        VectorPath::Corner dominantCorner;
        if (shouldSnapToCorner(blend, dominantCorner)) {
            VectorPath::CornerBlend snappedBlend = blend;
            applyCornerSnapping(snappedBlend, dominantCorner);
            lastBlend_ = snappedBlend;
        }
    }
    
    // Update voice allocation based on blend
    updateVoiceAllocation();
}

void VectorPathCrossfade::processAudio(float* outputL, float* outputR, int numSamples) {
    if (!initialized_) {
        // Clear output if not initialized
        std::fill(outputL, outputL + numSamples, 0.0f);
        std::fill(outputR, outputR + numSamples, 0.0f);
        return;
    }
    
    // Clear output buffers
    std::fill(outputL, outputL + numSamples, 0.0f);
    std::fill(outputR, outputR + numSamples, 0.0f);
    
    // Process each active engine
    for (int i = 0; i < 4; i++) {
        CornerEngine& corner = cornerEngines_[i];
        
        if (corner.active && corner.engine && engineLevels_[i] > 0.001f) {
            processEngineAudio(static_cast<VectorPath::Corner>(i), outputL, outputR, numSamples, engineLevels_[i]);
        }
    }
    
    // Apply global normalization
    applyGlobalNormalization(outputL, outputR, numSamples);
}

void VectorPathCrossfade::processParameters(float deltaTimeMs) {
    if (!initialized_) {
        return;
    }
    
    // Update transitions
    if (anyTransitionActive_) {
        updateEngineTransitions(deltaTimeMs);
    }
    
    // Update preset morph
    if (presetMorph_.active) {
        updatePresetMorph(deltaTimeMs);
    }
    
    // Update engine parameters if real-time update is enabled
    if (config_.realTimeUpdate) {
        for (int i = 0; i < 4; i++) {
            CornerEngine& corner = cornerEngines_[i];
            if (corner.active && corner.engine) {
                corner.engine->processParameters(deltaTimeMs);
            }
        }
    }
    
    lastUpdateTime_ = getTimeMs();
}

float VectorPathCrossfade::getCornerActivity(VectorPath::Corner corner) const {
    int cornerIndex = static_cast<int>(corner);
    if (cornerIndex < 0 || cornerIndex >= 4) {
        return 0.0f;
    }
    
    return engineLevels_[cornerIndex];
}

std::array<float, 4> VectorPathCrossfade::getEngineBlendWeights() const {
    return engineLevels_;
}

VectorPathCrossfade::EngineType VectorPathCrossfade::getDominantEngine() const {
    int dominantIndex = 0;
    float maxLevel = engineLevels_[0];
    
    for (int i = 1; i < 4; i++) {
        if (engineLevels_[i] > maxLevel) {
            maxLevel = engineLevels_[i];
            dominantIndex = i;
        }
    }
    
    return cornerEngines_[dominantIndex].engineType;
}

float VectorPathCrossfade::getCrossfadeComplexity() const {
    // Calculate complexity based on how many engines are active
    int activeEngines = 0;
    float totalLevel = 0.0f;
    float entropy = 0.0f;
    
    for (float level : engineLevels_) {
        if (level > 0.001f) {
            activeEngines++;
            totalLevel += level;
        }
    }
    
    if (activeEngines <= 1) {
        return 0.0f; // Simple case
    }
    
    // Calculate entropy as measure of complexity
    for (float level : engineLevels_) {
        if (level > 0.001f && totalLevel > 0.0f) {
            float p = level / totalLevel;
            entropy -= p * std::log2(p);
        }
    }
    
    // Normalize entropy to 0-1 range
    float maxEntropy = std::log2(4.0f); // Max entropy for 4 engines
    return entropy / maxEntropy;
}

// Private method implementations

void VectorPathCrossfade::updateEngineTransitions(float deltaTimeMs) {
    float deltaTime = deltaTimeMs * 0.001f;
    bool anyActive = false;
    
    for (int i = 0; i < 4; i++) {
        EngineTransition& transition = activeTransitions_[i];
        
        if (transition.active) {
            updateEngineTransition(transition, deltaTimeMs);
            anyActive = true;
            
            if (!transition.active) {
                // Transition completed
                completeEngineTransition(static_cast<VectorPath::Corner>(i));
            }
        }
    }
    
    anyTransitionActive_ = anyActive;
}

void VectorPathCrossfade::updatePresetMorph(float deltaTimeMs) {
    uint32_t currentTime = getTimeMs();
    float elapsed = (currentTime - presetMorph_.startTime) * 0.001f;
    
    presetMorph_.progress = elapsed / presetMorph_.duration;
    
    if (presetMorph_.progress >= 1.0f) {
        presetMorph_.progress = 1.0f;
        presetMorph_.active = false;
        
        // Apply final target state
        for (int i = 0; i < 4; i++) {
            VectorPath::Corner corner = static_cast<VectorPath::Corner>(i);
            setCornerEngine(corner, presetMorph_.targetEngines[i]);
            setCornerEngineParams(corner, 
                                presetMorph_.targetParams[i][0],
                                presetMorph_.targetParams[i][1], 
                                presetMorph_.targetParams[i][2]);
        }
    } else {
        // Interpolate between start and target
        float t = smoothStep(0.0f, 1.0f, presetMorph_.progress);
        
        for (int i = 0; i < 4; i++) {
            VectorPath::Corner corner = static_cast<VectorPath::Corner>(i);
            
            // Interpolate parameters
            float h = lerp(presetMorph_.startParams[i][0], presetMorph_.targetParams[i][0], t);
            float ti = lerp(presetMorph_.startParams[i][1], presetMorph_.targetParams[i][1], t);
            float m = lerp(presetMorph_.startParams[i][2], presetMorph_.targetParams[i][2], t);
            
            setCornerEngineParams(corner, h, ti, m);
            
            // Handle engine changes at midpoint
            if (presetMorph_.progress >= 0.5f && 
                presetMorph_.startEngines[i] != presetMorph_.targetEngines[i]) {
                setCornerEngine(corner, presetMorph_.targetEngines[i]);
            }
        }
    }
}

void VectorPathCrossfade::updateEngineBlending() {
    // Calculate engine blend levels based on current position and mode
    std::array<float, 4> rawWeights;
    for (int i = 0; i < 4; i++) {
        rawWeights[i] = lastBlend_.weights[i];
    }
    
    // Apply crossfade mode
    switch (config_.mode) {
        case CrossfadeMode::INSTANT:
            {
                // Find dominant corner and set it to 1.0, others to 0.0
                int dominantIndex = 0;
                for (int i = 1; i < 4; i++) {
                    if (rawWeights[i] > rawWeights[dominantIndex]) {
                        dominantIndex = i;
                    }
                }
                engineLevels_.fill(0.0f);
                engineLevels_[dominantIndex] = 1.0f;
            }
            break;
            
        case CrossfadeMode::LINEAR:
            engineLevels_ = rawWeights;
            break;
            
        case CrossfadeMode::EQUAL_POWER:
            engineLevels_ = calculateEqualPowerBlend(lastBlend_);
            break;
            
        case CrossfadeMode::S_CURVE:
            for (int i = 0; i < 4; i++) {
                engineLevels_[i] = smoothStep(0.0f, 1.0f, rawWeights[i]);
            }
            break;
            
        case CrossfadeMode::EXPONENTIAL:
            for (int i = 0; i < 4; i++) {
                engineLevels_[i] = exponentialCurve(rawWeights[i], config_.morphCurve);
            }
            break;
    }
    
    // Apply perceptual blending if enabled
    if (config_.usePerceptualBlending) {
        engineLevels_ = calculatePerceptualBlend(lastBlend_);
    }
    
    // Normalize levels
    totalEngineLevel_ = 0.0f;
    for (float level : engineLevels_) {
        totalEngineLevel_ += level;
    }
    
    if (totalEngineLevel_ > 0.0f) {
        for (float& level : engineLevels_) {
            level /= totalEngineLevel_;
        }
        totalEngineLevel_ = 1.0f;
    }
    
    // Activate/deactivate engines based on levels
    for (int i = 0; i < 4; i++) {
        VectorPath::Corner corner = static_cast<VectorPath::Corner>(i);
        
        if (engineLevels_[i] > 0.001f && !cornerEngines_[i].active) {
            activateEngine(corner);
        } else if (engineLevels_[i] <= 0.001f && cornerEngines_[i].active) {
            deactivateEngine(corner);
        }
    }
}

void VectorPathCrossfade::updateVoiceAllocation() {
    // Update voice allocation based on engine levels
    int totalVoicesNeeded = 0;
    
    // Calculate voice requirements
    for (int i = 0; i < 4; i++) {
        if (engineLevels_[i] > 0.001f) {
            float voiceRatio = engineLevels_[i];
            int voicesForEngine = static_cast<int>(voiceRatio * globalMaxVoices_);
            voicesForEngine = std::min(voicesForEngine, cornerEngines_[i].maxVoices);
            totalVoicesNeeded += voicesForEngine;
        }
    }
    
    // Reallocate if needed
    if (totalVoicesNeeded != totalVoiceCount_) {
        reallocateVoices();
    }
}

std::array<float, 4> VectorPathCrossfade::calculateEqualPowerBlend(const VectorPath::CornerBlend& blend) const {
    std::array<float, 4> result;
    
    for (int i = 0; i < 4; i++) {
        // Equal-power blending: sqrt(weight)
        result[i] = std::sqrt(std::max(0.0f, blend.weights[i]));
    }
    
    return result;
}

std::array<float, 4> VectorPathCrossfade::calculatePerceptualBlend(const VectorPath::CornerBlend& blend) const {
    std::array<float, 4> result;
    
    for (int i = 0; i < 4; i++) {
        float weight = blend.weights[i];
        
        // Apply perceptual curve (similar to loudness curve)
        if (weight < 0.1f) {
            result[i] = weight * weight * 10.0f; // Steeper curve for low levels
        } else {
            result[i] = 0.1f + (weight - 0.1f) * 0.9f / 0.9f; // Linear for higher levels
        }
    }
    
    return result;
}

float VectorPathCrossfade::applyCrossfadeCurve(float t, CrossfadeMode mode) const {
    switch (mode) {
        case CrossfadeMode::LINEAR:
            return t;
            
        case CrossfadeMode::EQUAL_POWER:
            return std::sqrt(t);
            
        case CrossfadeMode::S_CURVE:
            return smoothStep(0.0f, 1.0f, t);
            
        case CrossfadeMode::EXPONENTIAL:
            return exponentialCurve(t, config_.morphCurve);
            
        default:
            return t;
    }
}

void VectorPathCrossfade::activateEngine(VectorPath::Corner corner) {
    int cornerIndex = static_cast<int>(corner);
    CornerEngine& cornerEngine = cornerEngines_[cornerIndex];
    
    if (cornerEngine.active) {
        return;
    }
    
    // Create engine if it doesn't exist
    if (!cornerEngine.engine) {
        cornerEngine.engine = createEngine(cornerEngine.engineType);
    }
    
    if (cornerEngine.engine) {
        // Apply current parameters
        cornerEngine.engine->setHTMParameters(cornerEngine.harmonics, cornerEngine.timbre, cornerEngine.morph);
        
        // Activate engine
        cornerEngine.engine->activate();
        cornerEngine.active = true;
    }
}

void VectorPathCrossfade::deactivateEngine(VectorPath::Corner corner) {
    int cornerIndex = static_cast<int>(corner);
    CornerEngine& cornerEngine = cornerEngines_[cornerIndex];
    
    if (!cornerEngine.active) {
        return;
    }
    
    if (cornerEngine.engine) {
        cornerEngine.engine->deactivate();
    }
    
    cornerEngine.active = false;
    cornerEngine.voiceCount = 0;
}

void VectorPathCrossfade::reallocateVoices() {
    totalVoiceCount_ = 0;
    
    for (int i = 0; i < 4; i++) {
        if (engineLevels_[i] > 0.001f) {
            float voiceRatio = engineLevels_[i];
            int voicesForEngine = static_cast<int>(voiceRatio * globalMaxVoices_);
            voicesForEngine = std::min(voicesForEngine, cornerEngines_[i].maxVoices);
            
            cornerEngines_[i].voiceCount = voicesForEngine;
            totalVoiceCount_ += voicesForEngine;
        } else {
            cornerEngines_[i].voiceCount = 0;
        }
    }
}

void VectorPathCrossfade::startEngineTransition(VectorPath::Corner corner, EngineType newEngine, float duration) {
    int cornerIndex = static_cast<int>(corner);
    EngineTransition& transition = activeTransitions_[cornerIndex];
    
    transition.corner = corner;
    transition.fromEngine = cornerEngines_[cornerIndex].engineType;
    transition.toEngine = newEngine;
    transition.progress = 0.0f;
    transition.startTime = getTimeMs() * 0.001f;
    transition.active = true;
    
    // Prepare new engine
    CornerEngine& cornerEngine = cornerEngines_[cornerIndex];
    std::shared_ptr<SynthEngineBase> newEngineInstance = createEngine(newEngine);
    
    if (newEngineInstance) {
        // Set up new engine with current parameters
        newEngineInstance->setHTMParameters(cornerEngine.harmonics, cornerEngine.timbre, cornerEngine.morph);
        
        if (cornerEngine.active) {
            newEngineInstance->activate();
        }
    }
    
    // Store old engine for transition
    // (In a real implementation, you'd manage voice transitions here)
}

void VectorPathCrossfade::updateEngineTransition(EngineTransition& transition, float deltaTimeMs) {
    float deltaTime = deltaTimeMs * 0.001f;
    float currentTime = getTimeMs() * 0.001f;
    float elapsed = currentTime - transition.startTime;
    
    transition.progress = elapsed / config_.transitionTime;
    
    if (transition.progress >= 1.0f) {
        transition.progress = 1.0f;
        transition.active = false;
    }
    
    // Apply crossfade curve to transition
    float effectiveProgress = applyCrossfadeCurve(transition.progress, config_.mode);
    
    // Update crossfade amounts (would be used for voice level control)
    int cornerIndex = static_cast<int>(transition.corner);
    cornerEngines_[cornerIndex].crossfadeAmount = effectiveProgress;
}

void VectorPathCrossfade::completeEngineTransition(VectorPath::Corner corner) {
    int cornerIndex = static_cast<int>(corner);
    CornerEngine& cornerEngine = cornerEngines_[cornerIndex];
    EngineTransition& transition = activeTransitions_[cornerIndex];
    
    // Replace old engine with new engine
    cornerEngine.engineType = transition.toEngine;
    cornerEngine.engine = createEngine(transition.toEngine);
    
    if (cornerEngine.engine) {
        cornerEngine.engine->setHTMParameters(cornerEngine.harmonics, cornerEngine.timbre, cornerEngine.morph);
        
        if (cornerEngine.active) {
            cornerEngine.engine->activate();
        }
    }
    
    cornerEngine.crossfadeAmount = 0.0f;
    
    // Notify completion
    if (crossfadeCompleteCallback_) {
        crossfadeCompleteCallback_(cornerEngines_);
    }
}

bool VectorPathCrossfade::shouldSnapToCorner(const VectorPath::CornerBlend& blend, VectorPath::Corner& dominantCorner) const {
    float maxWeight = 0.0f;
    int dominantIndex = 0;
    
    for (int i = 0; i < 4; i++) {
        if (blend.weights[i] > maxWeight) {
            maxWeight = blend.weights[i];
            dominantIndex = i;
        }
    }
    
    dominantCorner = static_cast<VectorPath::Corner>(dominantIndex);
    
    // Check if dominant corner is close enough to snap
    return maxWeight > (1.0f - config_.cornerSnapThreshold);
}

void VectorPathCrossfade::applyCornerSnapping(VectorPath::CornerBlend& blend, VectorPath::Corner dominantCorner) const {
    int dominantIndex = static_cast<int>(dominantCorner);
    
    // Set dominant corner to 1.0, others to 0.0
    for (int i = 0; i < 4; i++) {
        blend.weights[i] = (i == dominantIndex) ? 1.0f : 0.0f;
    }
}

void VectorPathCrossfade::processEngineAudio(VectorPath::Corner corner, float* outputL, float* outputR, int numSamples, float level) {
    int cornerIndex = static_cast<int>(corner);
    CornerEngine& cornerEngine = cornerEngines_[cornerIndex];
    
    if (!cornerEngine.engine || !cornerEngine.active) {
        return;
    }
    
    // Create temporary buffers for engine output
    std::vector<float> tempL(numSamples, 0.0f);
    std::vector<float> tempR(numSamples, 0.0f);
    
    // Process engine audio
    cornerEngine.engine->processAudio(tempL.data(), tempR.data(), numSamples);
    
    // Mix into output with level scaling
    for (int i = 0; i < numSamples; i++) {
        outputL[i] += tempL[i] * level;
        outputR[i] += tempR[i] * level;
    }
}

void VectorPathCrossfade::applyGlobalNormalization(float* outputL, float* outputR, int numSamples) {
    // Apply gentle limiting to prevent clipping from engine mixing
    const float maxLevel = 0.95f;
    
    for (int i = 0; i < numSamples; i++) {
        outputL[i] = std::clamp(outputL[i], -maxLevel, maxLevel);
        outputR[i] = std::clamp(outputR[i], -maxLevel, maxLevel);
    }
}

// Engine factory implementation (placeholder)
std::shared_ptr<VectorPathCrossfade::SynthEngineBase> VectorPathCrossfade::createEngine(EngineType type) {
    // In a real implementation, this would create the appropriate engine instance
    // For now, return nullptr as we don't have concrete engine implementations
    return nullptr;
}

void VectorPathCrossfade::destroyEngine(std::shared_ptr<SynthEngineBase> engine) {
    if (engine) {
        engine->deactivate();
    }
    // Smart pointer will handle cleanup
}

// Utility functions
uint32_t VectorPathCrossfade::getTimeMs() const {
#ifdef STM32H7
    return HAL_GetTick();
#else
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
#endif
}

float VectorPathCrossfade::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}

float VectorPathCrossfade::smoothStep(float edge0, float edge1, float x) const {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float VectorPathCrossfade::exponentialCurve(float t, float shape) const {
    if (shape < 0.5f) {
        // Exponential ease-in
        float factor = 2.0f * shape;
        return std::pow(t, 1.0f + factor * 4.0f);
    } else {
        // Exponential ease-out
        float factor = 2.0f * (shape - 0.5f);
        return 1.0f - std::pow(1.0f - t, 1.0f + factor * 4.0f);
    }
}

float VectorPathCrossfade::equalPowerGain(float position) const {
    return std::sqrt(std::max(0.0f, std::min(1.0f, position)));
}

// Static utility methods
std::string VectorPathCrossfade::getEngineTypeName(EngineType type) {
    static const std::unordered_map<EngineType, std::string> engineNames = {
        {EngineType::MACRO_VA, "MacroVA"},
        {EngineType::MACRO_FM, "MacroFM"},
        {EngineType::MACRO_WAVESHAPER, "MacroWaveshaper"},
        {EngineType::MACRO_WAVETABLE, "MacroWavetable"},
        {EngineType::MACRO_CHORD, "MacroChord"},
        {EngineType::MACRO_HARMONICS, "MacroHarmonics"},
        {EngineType::FORMANT, "Formant"},
        {EngineType::NOISE, "Noise"},
        {EngineType::TIDES_OSC, "TidesOsc"},
        {EngineType::RINGS_VOICE, "RingsVoice"},
        {EngineType::ELEMENTS_VOICE, "ElementsVoice"},
        {EngineType::SLIDE_ACCENT_BASS, "Slide+Accent Bass"},
        {EngineType::SERIAL_HP_LP, "Serial HP→LP"},
        {EngineType::CLASSIC_4OP_FM, "Classic 4-Op FM"}
    };
    
    auto it = engineNames.find(type);
    return (it != engineNames.end()) ? it->second : "Unknown";
}

std::array<std::string, 3> VectorPathCrossfade::getEngineParameterNames(EngineType type) {
    // Return H/T/M parameter names specific to each engine
    switch (type) {
        case EngineType::MACRO_VA:
            return {"LPF+AutoQ", "Saw↔Pulse+PWM", "Sub/Noise+Tilt"};
        case EngineType::MACRO_FM:
            return {"FM Index+Tilt", "Ratio+Wave", "Feedback+Env"};
        case EngineType::MACRO_WAVESHAPER:
            return {"Drive+Asym", "Gain+Bank+EQ", "LPF+Sat"};
        case EngineType::MACRO_WAVETABLE:
            return {"Position", "Formant+Tilt", "Vector Path"};
        default:
            return {"Harmonics", "Timbre", "Morph"};
    }
}

bool VectorPathCrossfade::isEngineCompatible(EngineType engine1, EngineType engine2) {
    // Define compatibility rules for smooth transitions
    // For now, all engines are considered compatible
    return true;
}

// Preset management stubs
void VectorPathCrossfade::saveCurrentAsPreset(const std::string& name) {
    // Implementation would save current state to persistent storage
}

bool VectorPathCrossfade::loadPreset(const std::string& name) {
    // Implementation would load preset from persistent storage
    return false;
}