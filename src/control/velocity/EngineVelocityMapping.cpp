#include "EngineVelocityMapping.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iostream>

// Static member definition
EngineVelocityMapping::EngineVelocityConfig EngineVelocityMapping::defaultConfig_;

EngineVelocityMapping::EngineVelocityMapping() {
    enabled_ = true;
    sampleRate_ = DEFAULT_SAMPLE_RATE;
    velocityModulation_ = nullptr;
    depthControl_ = nullptr;
    volumeControl_ = nullptr;
    
    initializeDefaultPresets();
}

// Engine configuration management
void EngineVelocityMapping::setEngineConfig(uint32_t engineId, const EngineVelocityConfig& config) {
    engineConfigs_[engineId] = config;
    
    // Initialize voice states for this engine if needed
    if (engineVoices_.find(engineId) == engineVoices_.end()) {
        engineVoices_[engineId] = std::unordered_map<uint32_t, EngineVoiceState>();
    }
}

const EngineVelocityMapping::EngineVelocityConfig& EngineVelocityMapping::getEngineConfig(uint32_t engineId) const {
    auto it = engineConfigs_.find(engineId);
    return (it != engineConfigs_.end()) ? it->second : defaultConfig_;
}

bool EngineVelocityMapping::hasEngineConfig(uint32_t engineId) const {
    return engineConfigs_.find(engineId) != engineConfigs_.end();
}

void EngineVelocityMapping::removeEngineConfig(uint32_t engineId) {
    engineConfigs_.erase(engineId);
    engineVoices_.erase(engineId);
}

// Velocity mapping updates
std::vector<EngineVelocityMapping::ParameterUpdateResult> EngineVelocityMapping::updateEngineParameters(uint32_t engineId, uint32_t voiceId, uint8_t velocity) {
    std::vector<ParameterUpdateResult> results;
    
    if (!enabled_) {
        return results;
    }
    
    const auto& config = getEngineConfig(engineId);
    
    // Update each mapped parameter
    for (const auto& mapping : config.mappings) {
        if (!mapping.enabled) {
            continue;
        }
        
        ParameterUpdateResult result;
        result.target = mapping.target;
        result.originalValue = mapping.baseValue;
        
        // Calculate velocity-modulated parameter value
        result.modulatedValue = mapVelocityToParameter(mapping, velocity);
        result.velocityComponent = result.modulatedValue - result.originalValue;
        result.wasUpdated = (std::abs(result.velocityComponent) > 0.001f);
        
        results.push_back(result);
        
        // Notify callback if parameter was significantly updated
        if (result.wasUpdated) {
            notifyParameterUpdate(engineId, voiceId, result.target, result.modulatedValue);
        }
    }
    
    // Update voice state
    auto engineIt = engineVoices_.find(engineId);
    if (engineIt != engineVoices_.end()) {
        auto voiceIt = engineIt->second.find(voiceId);
        if (voiceIt != engineIt->second.end()) {
            voiceIt->second.currentVelocity = velocity;
            voiceIt->second.lastUpdateTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            
            // Store updated parameter values
            for (const auto& result : results) {
                voiceIt->second.lastParameterValues[result.target] = result.modulatedValue;
            }
        }
    }
    
    return results;
}

EngineVelocityMapping::ParameterUpdateResult EngineVelocityMapping::updateSingleParameter(uint32_t engineId, VelocityTarget target, float baseValue, uint8_t velocity) {
    ParameterUpdateResult result;
    result.target = target;
    result.originalValue = baseValue;
    
    if (!enabled_) {
        result.modulatedValue = baseValue;
        return result;
    }
    
    const auto& config = getEngineConfig(engineId);
    
    // Find the mapping for this target
    for (const auto& mapping : config.mappings) {
        if (mapping.target == target && mapping.enabled) {
            result.modulatedValue = mapVelocityToParameter(mapping, velocity);
            result.velocityComponent = result.modulatedValue - result.originalValue;
            result.wasUpdated = (std::abs(result.velocityComponent) > 0.001f);
            return result;
        }
    }
    
    // No mapping found, return unchanged value
    result.modulatedValue = baseValue;
    return result;
}

void EngineVelocityMapping::updateAllEngineVoices(uint32_t engineId, float deltaTime) {
    (void)deltaTime; // Used for future smoothing implementation
    auto engineIt = engineVoices_.find(engineId);
    if (engineIt == engineVoices_.end()) {
        return;
    }
    
    // Update all voices for this engine with smoothing if enabled
    for (auto& [voiceId, voiceState] : engineIt->second) {
        auto results = updateEngineParameters(engineId, voiceId, voiceState.currentVelocity);
        
        // Apply engine-specific parameter updates
        const auto& config = getEngineConfig(engineId);
        switch (config.engineType) {
            case EngineType::MACRO_VA:
                applyMacroVAParameters(engineId, voiceId, results);
                break;
            case EngineType::MACRO_FM:
                applyMacroFMParameters(engineId, voiceId, results);
                break;
            case EngineType::MACRO_HARMONICS:
                applyMacroHarmonicsParameters(engineId, voiceId, results);
                break;
            case EngineType::MACRO_WAVETABLE:
                applyMacroWavetableParameters(engineId, voiceId, results);
                break;
            // Additional engine types - use generic parameter application for now
            case EngineType::MACRO_CHORD:
            case EngineType::MACRO_WAVESHAPER:
            case EngineType::ELEMENTS_VOICE:
            case EngineType::RINGS_VOICE:
            case EngineType::TIDES_OSC:
            case EngineType::FORMANT_VOCAL:
            case EngineType::NOISE_PARTICLES:
            case EngineType::CLASSIC_4OP_FM:
            case EngineType::DRUM_KIT:
            case EngineType::SAMPLER_KIT:
            case EngineType::SAMPLER_SLICER:
            case EngineType::SLIDE_ACCENT_BASS:
            case EngineType::PLAITS_VA:
            case EngineType::PLAITS_WAVESHAPING:
            case EngineType::PLAITS_FM:
            case EngineType::PLAITS_GRAIN:
            case EngineType::PLAITS_ADDITIVE:
            case EngineType::PLAITS_WAVETABLE:
            case EngineType::PLAITS_CHORD:
            case EngineType::PLAITS_SPEECH:
            case EngineType::PLAITS_SWARM:
            case EngineType::PLAITS_NOISE:
            case EngineType::PLAITS_PARTICLE:
            case EngineType::PLAITS_STRING:
            case EngineType::PLAITS_MODAL:
            case EngineType::PLAITS_BASS_DRUM:
            case EngineType::PLAITS_SNARE_DRUM:
            case EngineType::PLAITS_HI_HAT:
                // Generic parameter application - specific implementations can be added later
                break;
        }
    }
}

// Engine-specific parameter application (stubs for now)
void EngineVelocityMapping::applyMacroVAParameters(uint32_t engineId, uint32_t voiceId, const std::vector<ParameterUpdateResult>& updates) {
    (void)engineId; (void)voiceId;
    
    // Apply VA-specific velocity mappings
    for (const auto& update : updates) {
        switch (update.target) {
            case VelocityTarget::ENV_ATTACK:
                // Apply velocity to envelope attack time
                break;
            case VelocityTarget::ENV_DECAY:
                // Apply velocity to envelope decay time
                break;
            case VelocityTarget::ENV_SUSTAIN:
                // Apply velocity to envelope sustain level
                break;
            case VelocityTarget::VA_OSC_DETUNE:
                // Apply velocity to oscillator detune
                break;
            case VelocityTarget::FILTER_CUTOFF:
                // Apply velocity to filter cutoff
                break;
            default:
                break;
        }
    }
}

void EngineVelocityMapping::applyMacroFMParameters(uint32_t engineId, uint32_t voiceId, const std::vector<ParameterUpdateResult>& updates) {
    (void)engineId; (void)voiceId;
    
    // Apply FM-specific velocity mappings
    for (const auto& update : updates) {
        switch (update.target) {
            case VelocityTarget::FM_MOD_INDEX:
                // Apply velocity to FM modulation index
                break;
            case VelocityTarget::FM_CARRIER_LEVEL:
                // Apply velocity to carrier level
                break;
            case VelocityTarget::FM_MODULATOR_LEVEL:
                // Apply velocity to modulator level
                break;
            case VelocityTarget::FM_FEEDBACK:
                // Apply velocity to FM feedback
                break;
            default:
                break;
        }
    }
}

void EngineVelocityMapping::applyMacroHarmonicsParameters(uint32_t engineId, uint32_t voiceId, const std::vector<ParameterUpdateResult>& updates) {
    (void)engineId; (void)voiceId;
    
    // Apply Harmonics-specific velocity mappings
    for (const auto& update : updates) {
        switch (update.target) {
            case VelocityTarget::HARM_DRAWBAR_LEVELS:
                // Apply velocity to drawbar levels
                break;
            case VelocityTarget::HARM_PERCUSSION_LEVEL:
                // Apply velocity to percussion level
                break;
            case VelocityTarget::HARM_SCANNER_RATE:
                // Apply velocity to scanner rate
                break;
            case VelocityTarget::HARM_KEY_CLICK:
                // Apply velocity to key click intensity
                break;
            default:
                break;
        }
    }
}

void EngineVelocityMapping::applyMacroWavetableParameters(uint32_t engineId, uint32_t voiceId, const std::vector<ParameterUpdateResult>& updates) {
    (void)engineId; (void)voiceId;
    
    // Apply Wavetable-specific velocity mappings
    for (const auto& update : updates) {
        switch (update.target) {
            case VelocityTarget::WT_POSITION:
                // Apply velocity to wavetable position
                break;
            case VelocityTarget::WT_SCAN_RATE:
                // Apply velocity to scan rate
                break;
            case VelocityTarget::WT_MORPH_AMOUNT:
                // Apply velocity to morph amount
                break;
            case VelocityTarget::WT_GRAIN_SIZE:
                // Apply velocity to grain size
                break;
            default:
                break;
        }
    }
}

// Preset management
void EngineVelocityMapping::loadEnginePreset(uint32_t engineId, const std::string& presetName) {
    // Find the preset by name
    for (const auto& [engineType, presets] : enginePresets_) {
        for (const auto& preset : presets) {
            if (preset.configName == presetName) {
                setEngineConfig(engineId, preset);
                return;
            }
        }
    }
}

void EngineVelocityMapping::saveEnginePreset(uint32_t engineId, const std::string& presetName, const std::string& description) {
    if (!hasEngineConfig(engineId)) {
        return;
    }
    
    auto config = getEngineConfig(engineId);
    config.configName = presetName;
    config.description = description;
    
    // Add to presets for this engine type
    enginePresets_[config.engineType].push_back(config);
}

std::vector<std::string> EngineVelocityMapping::getAvailablePresets(EngineType engineType) const {
    std::vector<std::string> presetNames;
    
    auto it = enginePresets_.find(engineType);
    if (it != enginePresets_.end()) {
        for (const auto& preset : it->second) {
            presetNames.push_back(preset.configName);
        }
    }
    
    return presetNames;
}

void EngineVelocityMapping::createDefaultPresets() {
    initializeDefaultPresets();
}

// Integration with other velocity systems
void EngineVelocityMapping::integrateWithVelocityModulation(RelativeVelocityModulation* modSystem) {
    velocityModulation_ = modSystem;
}

void EngineVelocityMapping::integrateWithDepthControl(VelocityDepthControl* depthControl) {
    depthControl_ = depthControl;
}

void EngineVelocityMapping::integrateWithVolumeControl(VelocityVolumeControl* volumeControl) {
    volumeControl_ = volumeControl;
}

// Voice management
void EngineVelocityMapping::addEngineVoice(uint32_t engineId, uint32_t voiceId, uint8_t velocity) {
    auto& engineVoices = engineVoices_[engineId];
    
    EngineVoiceState voiceState;
    voiceState.voiceId = voiceId;
    voiceState.engineId = engineId;
    voiceState.currentVelocity = velocity;
    voiceState.lastUpdateTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    engineVoices[voiceId] = voiceState;
    
    // Perform initial parameter update
    updateEngineParameters(engineId, voiceId, velocity);
}

void EngineVelocityMapping::updateEngineVoiceVelocity(uint32_t engineId, uint32_t voiceId, uint8_t newVelocity) {
    auto engineIt = engineVoices_.find(engineId);
    if (engineIt == engineVoices_.end()) {
        return;
    }
    
    auto voiceIt = engineIt->second.find(voiceId);
    if (voiceIt == engineIt->second.end()) {
        return;
    }
    
    voiceIt->second.currentVelocity = newVelocity;
    
    // Update all parameters with new velocity
    updateEngineParameters(engineId, voiceId, newVelocity);
}

void EngineVelocityMapping::removeEngineVoice(uint32_t engineId, uint32_t voiceId) {
    auto engineIt = engineVoices_.find(engineId);
    if (engineIt != engineVoices_.end()) {
        engineIt->second.erase(voiceId);
    }
}

void EngineVelocityMapping::clearAllEngineVoices(uint32_t engineId) {
    auto engineIt = engineVoices_.find(engineId);
    if (engineIt != engineVoices_.end()) {
        engineIt->second.clear();
    }
}

// System management
void EngineVelocityMapping::reset() {
    engineConfigs_.clear();
    engineVoices_.clear();
    enginePresets_.clear();
    initializeDefaultPresets();
}

// Performance monitoring
size_t EngineVelocityMapping::getActiveEngineCount() const {
    return engineConfigs_.size();
}

size_t EngineVelocityMapping::getActiveVoiceCount(uint32_t engineId) const {
    auto engineIt = engineVoices_.find(engineId);
    return (engineIt != engineVoices_.end()) ? engineIt->second.size() : 0;
}

size_t EngineVelocityMapping::getTotalActiveVoices() const {
    size_t total = 0;
    for (const auto& [engineId, voices] : engineVoices_) {
        total += voices.size();
    }
    return total;
}

float EngineVelocityMapping::getAverageProcessingTime() const {
    // Placeholder for processing time calculation
    return getTotalActiveVoices() * 0.001f; // Estimate ~1μs per voice
}

// Parameter mapping utilities
float EngineVelocityMapping::mapVelocityToParameter(const VelocityMapping& mapping, uint8_t velocity) const {
    // Normalize velocity
    float normalizedVel = normalizeVelocity(velocity);
    
    // Apply global scaling and offset
    normalizedVel = normalizedVel * 1.0f + 0.0f; // Would use engine config values
    
    // Apply inversion if enabled
    if (mapping.invertVelocity) {
        normalizedVel = 1.0f - normalizedVel;
    }
    
    // Apply velocity curve
    float curvedVel = applyCurveToVelocity(normalizedVel, mapping);
    
    // Scale to parameter range
    float paramValue = mapping.baseValue + curvedVel * mapping.velocityAmount;
    
    // Apply parameter-specific scaling
    paramValue = scaleParameterValue(paramValue, mapping);
    
    return std::max(mapping.minValue, std::min(paramValue, mapping.maxValue));
}

EngineVelocityMapping::VelocityTarget EngineVelocityMapping::getParameterTarget(const std::string& parameterName) const {
    // Map parameter name strings to targets
    if (parameterName == "volume") return VelocityTarget::VOLUME;
    if (parameterName == "filter_cutoff") return VelocityTarget::FILTER_CUTOFF;
    if (parameterName == "filter_resonance") return VelocityTarget::FILTER_RESONANCE;
    if (parameterName == "env_attack") return VelocityTarget::ENV_ATTACK;
    if (parameterName == "fm_mod_index") return VelocityTarget::FM_MOD_INDEX;
    if (parameterName == "harm_percussion") return VelocityTarget::HARM_PERCUSSION_LEVEL;
    if (parameterName == "wt_position") return VelocityTarget::WT_POSITION;
    
    return VelocityTarget::VOLUME; // Default fallback
}

std::string EngineVelocityMapping::getTargetName(VelocityTarget target) const {
    switch (target) {
        case VelocityTarget::VOLUME: return "Volume";
        case VelocityTarget::FILTER_CUTOFF: return "Filter Cutoff";
        case VelocityTarget::FILTER_RESONANCE: return "Filter Resonance";
        case VelocityTarget::ENV_ATTACK: return "Envelope Attack";
        case VelocityTarget::ENV_DECAY: return "Envelope Decay";
        case VelocityTarget::ENV_SUSTAIN: return "Envelope Sustain";
        case VelocityTarget::FM_MOD_INDEX: return "FM Modulation Index";
        case VelocityTarget::FM_CARRIER_LEVEL: return "FM Carrier Level";
        case VelocityTarget::HARM_DRAWBAR_LEVELS: return "Drawbar Levels";
        case VelocityTarget::HARM_PERCUSSION_LEVEL: return "Percussion Level";
        case VelocityTarget::WT_POSITION: return "Wavetable Position";
        case VelocityTarget::WT_SCAN_RATE: return "Wavetable Scan Rate";
        default: return "Unknown";
    }
}

std::vector<EngineVelocityMapping::VelocityTarget> EngineVelocityMapping::getEngineTargets(EngineType engineType) const {
    std::vector<VelocityTarget> targets;
    
    // Universal targets
    targets.push_back(VelocityTarget::VOLUME);
    targets.push_back(VelocityTarget::FILTER_CUTOFF);
    targets.push_back(VelocityTarget::FILTER_RESONANCE);
    targets.push_back(VelocityTarget::ENV_ATTACK);
    targets.push_back(VelocityTarget::ENV_DECAY);
    targets.push_back(VelocityTarget::ENV_SUSTAIN);
    targets.push_back(VelocityTarget::ENV_RELEASE);
    
    // Engine-specific targets
    switch (engineType) {
        case EngineType::MACRO_VA:
            targets.push_back(VelocityTarget::VA_OSC_DETUNE);
            targets.push_back(VelocityTarget::VA_OSC_PWM);
            targets.push_back(VelocityTarget::VA_NOISE_LEVEL);
            targets.push_back(VelocityTarget::VA_SUB_LEVEL);
            break;
            
        case EngineType::MACRO_FM:
            targets.push_back(VelocityTarget::FM_MOD_INDEX);
            targets.push_back(VelocityTarget::FM_CARRIER_LEVEL);
            targets.push_back(VelocityTarget::FM_MODULATOR_LEVEL);
            targets.push_back(VelocityTarget::FM_FEEDBACK);
            targets.push_back(VelocityTarget::FM_OPERATOR_RATIO);
            break;
            
        case EngineType::MACRO_HARMONICS:
            targets.push_back(VelocityTarget::HARM_DRAWBAR_LEVELS);
            targets.push_back(VelocityTarget::HARM_PERCUSSION_LEVEL);
            targets.push_back(VelocityTarget::HARM_SCANNER_RATE);
            targets.push_back(VelocityTarget::HARM_KEY_CLICK);
            break;
            
        case EngineType::MACRO_WAVETABLE:
            targets.push_back(VelocityTarget::WT_POSITION);
            targets.push_back(VelocityTarget::WT_SCAN_RATE);
            targets.push_back(VelocityTarget::WT_MORPH_AMOUNT);
            targets.push_back(VelocityTarget::WT_GRAIN_SIZE);
            targets.push_back(VelocityTarget::WT_SPECTRAL_TILT);
            break;
            
        case EngineType::MACRO_CHORD:
            targets.push_back(VelocityTarget::CHORD_VOICING);
            targets.push_back(VelocityTarget::CHORD_SPREAD);
            targets.push_back(VelocityTarget::CHORD_STRUM_RATE);
            targets.push_back(VelocityTarget::CHORD_HARMONIC_CONTENT);
            break;
            
        case EngineType::MACRO_WAVESHAPER:
            targets.push_back(VelocityTarget::WS_DRIVE_AMOUNT);
            targets.push_back(VelocityTarget::WS_CURVE_TYPE);
            targets.push_back(VelocityTarget::WS_FOLD_AMOUNT);
            break;
            
        case EngineType::ELEMENTS_VOICE:
            targets.push_back(VelocityTarget::ELEM_BOW_PRESSURE);
            targets.push_back(VelocityTarget::ELEM_STRIKE_META);
            targets.push_back(VelocityTarget::ELEM_BRIGHTNESS);
            targets.push_back(VelocityTarget::ELEM_DAMPING);
            break;
            
        case EngineType::RINGS_VOICE:
            targets.push_back(VelocityTarget::RINGS_STRUCTURE);
            targets.push_back(VelocityTarget::RINGS_BRIGHTNESS);
            targets.push_back(VelocityTarget::RINGS_DAMPING);
            targets.push_back(VelocityTarget::RINGS_POSITION);
            break;
            
        case EngineType::TIDES_OSC:
            targets.push_back(VelocityTarget::TIDES_SLOPE);
            targets.push_back(VelocityTarget::TIDES_SMOOTH);
            targets.push_back(VelocityTarget::TIDES_SHIFT);
            break;
            
        case EngineType::FORMANT_VOCAL:
            targets.push_back(VelocityTarget::FORMANT_VOWEL);
            targets.push_back(VelocityTarget::FORMANT_CLOSURE);
            targets.push_back(VelocityTarget::FORMANT_BREATH);
            break;
            
        case EngineType::NOISE_PARTICLES:
            targets.push_back(VelocityTarget::NOISE_COLOR);
            targets.push_back(VelocityTarget::NOISE_DENSITY);
            targets.push_back(VelocityTarget::NOISE_TEXTURE);
            break;
            
        case EngineType::CLASSIC_4OP_FM:
            targets.push_back(VelocityTarget::FM_MOD_INDEX);
            targets.push_back(VelocityTarget::FM_FEEDBACK);
            targets.push_back(VelocityTarget::FM_ALGORITHM);
            targets.push_back(VelocityTarget::FM_OPERATOR_RATIO);
            break;
            
        case EngineType::DRUM_KIT:
            targets.push_back(VelocityTarget::DRUM_PITCH);
            targets.push_back(VelocityTarget::DRUM_DECAY);
            targets.push_back(VelocityTarget::DRUM_SNAP);
            targets.push_back(VelocityTarget::DRUM_DRIVE);
            break;
            
        case EngineType::SAMPLER_KIT:
        case EngineType::SAMPLER_SLICER:
            targets.push_back(VelocityTarget::SAMPLE_START);
            targets.push_back(VelocityTarget::SAMPLE_PITCH);
            targets.push_back(VelocityTarget::SAMPLE_FILTER);
            break;
            
        case EngineType::SLIDE_ACCENT_BASS:
            targets.push_back(VelocityTarget::BASS_SLIDE_TIME);
            targets.push_back(VelocityTarget::BASS_ACCENT_LEVEL);
            targets.push_back(VelocityTarget::BASS_SUB_HARMONIC);
            targets.push_back(VelocityTarget::BASS_DISTORTION);
            break;
            
        // Plaits engines use similar parameter sets
        case EngineType::PLAITS_VA:
            targets.push_back(VelocityTarget::VA_OSC_DETUNE);
            targets.push_back(VelocityTarget::VA_NOISE_LEVEL);
            break;
            
        case EngineType::PLAITS_FM:
            targets.push_back(VelocityTarget::FM_MOD_INDEX);
            targets.push_back(VelocityTarget::FM_FEEDBACK);
            break;
            
        case EngineType::PLAITS_GRAIN:
        case EngineType::PLAITS_WAVETABLE:
            targets.push_back(VelocityTarget::WT_POSITION);
            targets.push_back(VelocityTarget::WT_GRAIN_SIZE);
            break;
            
        case EngineType::PLAITS_BASS_DRUM:
        case EngineType::PLAITS_SNARE_DRUM:
        case EngineType::PLAITS_HI_HAT:
            targets.push_back(VelocityTarget::DRUM_PITCH);
            targets.push_back(VelocityTarget::DRUM_DECAY);
            targets.push_back(VelocityTarget::DRUM_SNAP);
            break;
            
        case EngineType::PLAITS_SPEECH:
            targets.push_back(VelocityTarget::FORMANT_VOWEL);
            targets.push_back(VelocityTarget::FORMANT_CLOSURE);
            break;
            
        case EngineType::PLAITS_NOISE:
        case EngineType::PLAITS_PARTICLE:
        case EngineType::PLAITS_SWARM:
            targets.push_back(VelocityTarget::NOISE_COLOR);
            targets.push_back(VelocityTarget::NOISE_DENSITY);
            break;
            
        case EngineType::PLAITS_STRING:
        case EngineType::PLAITS_MODAL:
            targets.push_back(VelocityTarget::RINGS_BRIGHTNESS);
            targets.push_back(VelocityTarget::RINGS_DAMPING);
            break;
            
        case EngineType::PLAITS_ADDITIVE:
        case EngineType::PLAITS_CHORD:
        case EngineType::PLAITS_WAVESHAPING:
            // Use universal targets for these
            break;
    }
    
    return targets;
}

void EngineVelocityMapping::setParameterUpdateCallback(ParameterUpdateCallback callback) {
    parameterUpdateCallback_ = callback;
}

// Private methods
void EngineVelocityMapping::initializeDefaultPresets() {
    enginePresets_[EngineType::MACRO_VA].push_back(createMacroVADefault());
    enginePresets_[EngineType::MACRO_FM].push_back(createMacroFMDefault());
    enginePresets_[EngineType::MACRO_HARMONICS].push_back(createMacroHarmonicsDefault());
    enginePresets_[EngineType::MACRO_WAVETABLE].push_back(createMacroWavetableDefault());
    
    // Set the first VA preset as the global default
    if (!enginePresets_[EngineType::MACRO_VA].empty()) {
        defaultConfig_ = enginePresets_[EngineType::MACRO_VA][0];
    }
}

EngineVelocityMapping::EngineVelocityConfig EngineVelocityMapping::createMacroVADefault() {
    EngineVelocityConfig config;
    config.engineType = EngineType::MACRO_VA;
    config.configName = "VA Classic";
    config.description = "Classic virtual analog velocity response";
    
    // Volume mapping
    VelocityMapping volumeMapping;
    volumeMapping.target = VelocityTarget::VOLUME;
    volumeMapping.enabled = true;
    volumeMapping.baseValue = 0.0f;
    volumeMapping.velocityAmount = 1.0f;
    volumeMapping.curveType = RelativeVelocityModulation::CurveType::EXPONENTIAL;
    volumeMapping.curveAmount = 1.5f;
    config.mappings.push_back(volumeMapping);
    
    // Filter cutoff mapping
    VelocityMapping filterMapping;
    filterMapping.target = VelocityTarget::FILTER_CUTOFF;
    filterMapping.enabled = true;
    filterMapping.baseValue = 0.5f;
    filterMapping.velocityAmount = 0.4f;
    filterMapping.curveType = RelativeVelocityModulation::CurveType::LINEAR;
    config.mappings.push_back(filterMapping);
    
    // Envelope attack mapping (faster attack with higher velocity)
    VelocityMapping envAttackMapping;
    envAttackMapping.target = VelocityTarget::ENV_ATTACK;
    envAttackMapping.enabled = true;
    envAttackMapping.baseValue = 0.3f;
    envAttackMapping.velocityAmount = -0.25f; // Negative for faster attack
    envAttackMapping.invertVelocity = true;
    envAttackMapping.curveType = RelativeVelocityModulation::CurveType::LOGARITHMIC;
    config.mappings.push_back(envAttackMapping);
    
    return config;
}

EngineVelocityMapping::EngineVelocityConfig EngineVelocityMapping::createMacroFMDefault() {
    EngineVelocityConfig config;
    config.engineType = EngineType::MACRO_FM;
    config.configName = "FM Expressive";
    config.description = "Expressive FM synthesis velocity response";
    
    // Volume mapping
    VelocityMapping volumeMapping;
    volumeMapping.target = VelocityTarget::VOLUME;
    volumeMapping.enabled = true;
    volumeMapping.velocityAmount = 1.0f;
    volumeMapping.curveType = RelativeVelocityModulation::CurveType::S_CURVE;
    volumeMapping.curveAmount = 2.0f;
    config.mappings.push_back(volumeMapping);
    
    // FM modulation index (key to FM expression)
    VelocityMapping modIndexMapping;
    modIndexMapping.target = VelocityTarget::FM_MOD_INDEX;
    modIndexMapping.enabled = true;
    modIndexMapping.baseValue = 0.3f;
    modIndexMapping.velocityAmount = 0.6f;
    modIndexMapping.curveType = RelativeVelocityModulation::CurveType::EXPONENTIAL;
    modIndexMapping.curveAmount = 2.0f;
    config.mappings.push_back(modIndexMapping);
    
    // Carrier level
    VelocityMapping carrierMapping;
    carrierMapping.target = VelocityTarget::FM_CARRIER_LEVEL;
    carrierMapping.enabled = true;
    carrierMapping.baseValue = 0.6f;
    carrierMapping.velocityAmount = 0.3f;
    config.mappings.push_back(carrierMapping);
    
    return config;
}

EngineVelocityMapping::EngineVelocityConfig EngineVelocityMapping::createMacroHarmonicsDefault() {
    EngineVelocityConfig config;
    config.engineType = EngineType::MACRO_HARMONICS;
    config.configName = "Organ Traditional";
    config.description = "Traditional organ-style velocity response";
    
    // Volume mapping (organs traditionally have less velocity response)
    VelocityMapping volumeMapping;
    volumeMapping.target = VelocityTarget::VOLUME;
    volumeMapping.enabled = true;
    volumeMapping.baseValue = 0.2f;
    volumeMapping.velocityAmount = 0.6f;
    volumeMapping.curveType = RelativeVelocityModulation::CurveType::LINEAR;
    config.mappings.push_back(volumeMapping);
    
    // Percussion level
    VelocityMapping percMapping;
    percMapping.target = VelocityTarget::HARM_PERCUSSION_LEVEL;
    percMapping.enabled = true;
    percMapping.baseValue = 0.1f;
    percMapping.velocityAmount = 0.8f;
    percMapping.curveType = RelativeVelocityModulation::CurveType::EXPONENTIAL;
    percMapping.curveAmount = 1.8f;
    config.mappings.push_back(percMapping);
    
    // Key click intensity
    VelocityMapping clickMapping;
    clickMapping.target = VelocityTarget::HARM_KEY_CLICK;
    clickMapping.enabled = true;
    clickMapping.baseValue = 0.05f;
    clickMapping.velocityAmount = 0.3f;
    clickMapping.curveType = RelativeVelocityModulation::CurveType::POWER_CURVE;
    clickMapping.curveAmount = 2.5f;
    config.mappings.push_back(clickMapping);
    
    return config;
}

EngineVelocityMapping::EngineVelocityConfig EngineVelocityMapping::createMacroWavetableDefault() {
    EngineVelocityConfig config;
    config.engineType = EngineType::MACRO_WAVETABLE;
    config.configName = "Wavetable Dynamic";
    config.description = "Dynamic wavetable velocity response";
    
    // Volume mapping
    VelocityMapping volumeMapping;
    volumeMapping.target = VelocityTarget::VOLUME;
    volumeMapping.enabled = true;
    volumeMapping.velocityAmount = 1.0f;
    volumeMapping.curveType = RelativeVelocityModulation::CurveType::LINEAR;
    config.mappings.push_back(volumeMapping);
    
    // Wavetable position
    VelocityMapping positionMapping;
    positionMapping.target = VelocityTarget::WT_POSITION;
    positionMapping.enabled = true;
    positionMapping.baseValue = 0.2f;
    positionMapping.velocityAmount = 0.6f;
    positionMapping.curveType = RelativeVelocityModulation::CurveType::S_CURVE;
    positionMapping.curveAmount = 1.5f;
    config.mappings.push_back(positionMapping);
    
    // Filter cutoff
    VelocityMapping filterMapping;
    filterMapping.target = VelocityTarget::FILTER_CUTOFF;
    filterMapping.enabled = true;
    filterMapping.baseValue = 0.4f;
    filterMapping.velocityAmount = 0.5f;
    filterMapping.curveType = RelativeVelocityModulation::CurveType::EXPONENTIAL;
    filterMapping.curveAmount = 1.3f;
    config.mappings.push_back(filterMapping);
    
    // Morph amount
    VelocityMapping morphMapping;
    morphMapping.target = VelocityTarget::WT_MORPH_AMOUNT;
    morphMapping.enabled = true;
    morphMapping.baseValue = 0.1f;
    morphMapping.velocityAmount = 0.4f;
    morphMapping.curveType = RelativeVelocityModulation::CurveType::LOGARITHMIC;
    morphMapping.curveAmount = 2.0f;
    config.mappings.push_back(morphMapping);
    
    return config;
}

float EngineVelocityMapping::normalizeVelocity(uint8_t velocity) const {
    return static_cast<float>(velocity) / 127.0f;
}

float EngineVelocityMapping::applyCurveToVelocity(float velocity, const VelocityMapping& mapping) const {
    // Use the same curve logic as RelativeVelocityModulation
    switch (mapping.curveType) {
        case RelativeVelocityModulation::CurveType::LINEAR:
            return velocity;
        case RelativeVelocityModulation::CurveType::EXPONENTIAL:
            return std::pow(velocity, 1.0f / mapping.curveAmount);
        case RelativeVelocityModulation::CurveType::LOGARITHMIC:
            return std::pow(velocity, mapping.curveAmount);
        case RelativeVelocityModulation::CurveType::S_CURVE: {
            float x = velocity * 2.0f - 1.0f;
            float curved = std::tanh(x * mapping.curveAmount) / std::tanh(mapping.curveAmount);
            return (curved + 1.0f) * 0.5f;
        }
        case RelativeVelocityModulation::CurveType::POWER_CURVE:
            return std::pow(velocity, mapping.curveAmount);
        case RelativeVelocityModulation::CurveType::STEPPED: {
            int steps = static_cast<int>(mapping.curveAmount);
            steps = std::max(2, std::min(steps, 16));
            float stepSize = 1.0f / static_cast<float>(steps - 1);
            int stepIndex = static_cast<int>(velocity * (steps - 1) + 0.5f);
            return stepIndex * stepSize;
        }
        default:
            return velocity;
    }
}

float EngineVelocityMapping::scaleParameterValue(float value, const VelocityMapping& mapping) const {
    (void)mapping; // Used for future parameter-specific scaling implementation
    // Apply parameter-specific scaling and clamping
    return std::max(MIN_PARAMETER_VALUE, std::min(value, MAX_PARAMETER_VALUE));
}

void EngineVelocityMapping::notifyParameterUpdate(uint32_t engineId, uint32_t voiceId, VelocityTarget target, float value) {
    if (parameterUpdateCallback_) {
        parameterUpdateCallback_(engineId, voiceId, target, value);
    }
}