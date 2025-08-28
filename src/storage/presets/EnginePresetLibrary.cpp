#include "EnginePresetLibrary.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>

// Static member definitions
const std::string EnginePresetLibrary::PRESET_FILE_EXTENSION = ".ethpreset";
const std::string EnginePresetLibrary::PRESET_SCHEMA_VERSION = "1.0";

EnginePresetLibrary::EnginePresetLibrary() {
    enabled_ = true;
    presetDirectory_ = "./presets/";
    velocityMapping_ = nullptr;
    cacheValid_ = false;
}

// Preset library initialization
void EnginePresetLibrary::initializeFactoryPresets() {
    // Initialize all engine presets
    initializeMacroVAPresets();
    initializeMacroFMPresets();
    initializeMacroHarmonicsPresets();
    initializeMacroWavetablePresets();
    initializeMacroChordPresets();
    initializeMacroWaveshaperPresets();
    initializeElementsPresets();
    initializeRingsPresets();
    initializeTidesPresets();
    initializeFormantPresets();
    initializeNoiseParticlesPresets();
    initializeClassic4OpFMPresets();
    initializeDrumKitPresets();
    initializeSamplerKitPresets();
    initializeSamplerSlicerPresets();
    initializeSlideAccentBassPresets();
    initializePlaitsVAPresets();
    initializePlaitsWaveshapingPresets();
    initializePlaitsFMPresets();
    initializePlaitsGrainPresets();
    initializePlaitsAdditivePresets();
    initializePlaitsWavetablePresets();
    initializePlaitsChordPresets();
    initializePlaitsSpeechPresets();
    initializePlaitsSwarmPresets();
    initializePlaitsNoisePresets();
    initializePlaitsParticlePresets();
    initializePlaitsStringPresets();
    initializePlaitsModalPresets();
    initializePlaitsBassPresets();
    initializePlaitsSnarePresets();
    initializePlaitsHiHatPresets();
    
    invalidateCache();
}

void EnginePresetLibrary::loadUserPresets() {
    // Placeholder for user preset loading from filesystem
    // Would scan presetDirectory_ for user preset files
}

void EnginePresetLibrary::createDefaultPresets() {
    initializeFactoryPresets();
}

// Core preset operations
bool EnginePresetLibrary::addPreset(const EnginePreset& preset) {
    if (!enabled_) {
        return false;
    }
    
    // Validate preset
    auto validation = validatePreset(preset);
    if (!validation.isValid) {
        return false;
    }
    
    // Add to appropriate storage
    if (preset.category == PresetCategory::USER_CUSTOM) {
        userPresets_[preset.engineType].push_back(std::make_unique<EnginePreset>(preset));
    } else {
        factoryPresets_[preset.engineType].push_back(std::make_unique<EnginePreset>(preset));
    }
    
    invalidateCache();
    return true;
}

bool EnginePresetLibrary::removePreset(const std::string& presetName, EngineType engineType) {
    // Remove from factory presets
    auto factoryIt = factoryPresets_.find(engineType);
    if (factoryIt != factoryPresets_.end()) {
        auto& presets = factoryIt->second;
        presets.erase(std::remove_if(presets.begin(), presets.end(),
            [&presetName](const std::unique_ptr<EnginePreset>& preset) {
                return preset->name == presetName;
            }), presets.end());
    }
    
    // Remove from user presets
    auto userIt = userPresets_.find(engineType);
    if (userIt != userPresets_.end()) {
        auto& presets = userIt->second;
        presets.erase(std::remove_if(presets.begin(), presets.end(),
            [&presetName](const std::unique_ptr<EnginePreset>& preset) {
                return preset->name == presetName;
            }), presets.end());
    }
    
    invalidateCache();
    return true;
}

bool EnginePresetLibrary::hasPreset(const std::string& presetName, EngineType engineType) const {
    return getPreset(presetName, engineType) != nullptr;
}

const EnginePresetLibrary::EnginePreset* EnginePresetLibrary::getPreset(const std::string& presetName, EngineType engineType) const {
    // Search factory presets
    auto factoryIt = factoryPresets_.find(engineType);
    if (factoryIt != factoryPresets_.end()) {
        for (const auto& preset : factoryIt->second) {
            if (preset->name == presetName) {
                return preset.get();
            }
        }
    }
    
    // Search user presets
    auto userIt = userPresets_.find(engineType);
    if (userIt != userPresets_.end()) {
        for (const auto& preset : userIt->second) {
            if (preset->name == presetName) {
                return preset.get();
            }
        }
    }
    
    return nullptr;
}

EnginePresetLibrary::EnginePreset* EnginePresetLibrary::getPresetMutable(const std::string& presetName, EngineType engineType) {
    return const_cast<EnginePreset*>(getPreset(presetName, engineType));
}

// Preset validation
EnginePresetLibrary::PresetValidationResult EnginePresetLibrary::validatePreset(const EnginePreset& preset) const {
    PresetValidationResult result;
    result.isValid = true;
    result.compatibilityScore = 1.0f;
    
    // Basic validation checks
    if (preset.name.empty() || preset.name.length() > MAX_PRESET_NAME_LENGTH) {
        result.errors.push_back("Invalid preset name");
        result.isValid = false;
    }
    
    if (preset.description.length() > MAX_DESCRIPTION_LENGTH) {
        result.warnings.push_back("Description truncated to maximum length");
    }
    
    // Validate parameter ranges
    if (!validateParameterRanges(preset)) {
        result.warnings.push_back("Some parameters are out of range");
        result.compatibilityScore *= 0.9f;
    }
    
    // Validate macro assignments
    if (!validateMacroAssignments(preset)) {
        result.warnings.push_back("Invalid macro assignments detected");
        result.compatibilityScore *= 0.8f;
    }
    
    return result;
}

bool EnginePresetLibrary::isPresetCompatible(const EnginePreset& preset, EngineType targetEngine) const {
    return preset.engineType == targetEngine;
}

void EnginePresetLibrary::repairPreset(EnginePreset& preset) const {
    // Clamp parameter values to valid ranges
    for (auto& [paramName, value] : preset.holdParams) {
        value = std::max(MIN_PARAMETER_VALUE, std::min(value, MAX_PARAMETER_VALUE));
    }
    for (auto& [paramName, value] : preset.twistParams) {
        value = std::max(MIN_PARAMETER_VALUE, std::min(value, MAX_PARAMETER_VALUE));
    }
    for (auto& [paramName, value] : preset.morphParams) {
        value = std::max(MIN_PARAMETER_VALUE, std::min(value, MAX_PARAMETER_VALUE));
    }
    
    // Fix macro assignments
    for (auto& [macroId, assignment] : preset.macroAssignments) {
        assignment.amount = std::max(-1.0f, std::min(assignment.amount, 1.0f));
    }
    
    // Update timestamps
    preset.modificationTime = getCurrentTimestamp();
}

// Factory preset creation - Clean presets (minimal processing)
EnginePresetLibrary::EnginePreset EnginePresetLibrary::createCleanPreset(EngineType engineType, const std::string& name) const {
    EnginePreset preset;
    preset.name = name;
    preset.engineType = engineType;
    preset.category = PresetCategory::CLEAN;
    preset.description = "Clean, minimal processing preset";
    preset.author = "EtherSynth Factory";
    preset.version = PRESET_SCHEMA_VERSION;
    preset.creationTime = getCurrentTimestamp();
    preset.tags = {"clean", "minimal", "pure"};
    
    // Clean presets use conservative parameter values
    switch (engineType) {
        case EngineType::MACRO_VA:
            preset.holdParams["osc_level"] = 0.8f;
            preset.holdParams["filter_cutoff"] = 0.7f;
            preset.holdParams["filter_resonance"] = 0.1f;
            preset.twistParams["env_attack"] = 0.05f;
            preset.twistParams["env_decay"] = 0.3f;
            preset.morphParams["lfo_rate"] = 0.2f;
            break;
            
        case EngineType::MACRO_FM:
            preset.holdParams["carrier_freq"] = 0.5f;
            preset.holdParams["mod_index"] = 0.3f;
            preset.twistParams["feedback"] = 0.1f;
            preset.morphParams["algorithm"] = 0.0f;
            break;
            
        case EngineType::MACRO_HARMONICS:
            preset.holdParams["drawbar_16"] = 0.8f;
            preset.holdParams["drawbar_8"] = 0.6f;
            preset.holdParams["drawbar_4"] = 0.4f;
            preset.twistParams["percussion"] = 0.0f;
            preset.morphParams["scanner_rate"] = 0.0f;
            break;
            
        default:
            // Generic clean preset parameters
            preset.holdParams["level"] = 0.7f;
            preset.twistParams["brightness"] = 0.5f;
            preset.morphParams["character"] = 0.3f;
            break;
    }
    
    // Clean presets have minimal velocity response
    preset.velocityConfig.enableVelocityToVolume = true;
    preset.velocityConfig.velocityMappings["volume"] = 0.5f;
    
    return preset;
}

// Factory preset creation - Classic presets (vintage character)
EnginePresetLibrary::EnginePreset EnginePresetLibrary::createClassicPreset(EngineType engineType, const std::string& name) const {
    EnginePreset preset = createCleanPreset(engineType, name);
    preset.category = PresetCategory::CLASSIC;
    preset.description = "Classic vintage-inspired preset";
    preset.tags = {"classic", "vintage", "warm"};
    
    // Classic presets add warmth and character
    switch (engineType) {
        case EngineType::MACRO_VA:
            preset.holdParams["filter_resonance"] = 0.3f;
            preset.twistParams["drive"] = 0.2f;
            preset.morphParams["analog_drift"] = 0.1f;
            preset.fxParams["tape_saturation"] = 0.3f;
            break;
            
        case EngineType::MACRO_FM:
            preset.holdParams["mod_index"] = 0.5f;
            preset.twistParams["feedback"] = 0.3f;
            preset.fxParams["tube_warmth"] = 0.2f;
            break;
            
        case EngineType::MACRO_HARMONICS:
            preset.twistParams["percussion"] = 0.4f;
            preset.morphParams["scanner_rate"] = 0.3f;
            preset.fxParams["leslie_speed"] = 0.6f;
            break;
            
        default:
            preset.fxParams["vintage_warmth"] = 0.3f;
            break;
    }
    
    // Classic presets have moderate velocity response
    preset.velocityConfig.velocityMappings["volume"] = 0.7f;
    preset.velocityConfig.velocityMappings["brightness"] = 0.4f;
    
    return preset;
}

// Factory preset creation - Extreme presets (heavy processing)
EnginePresetLibrary::EnginePreset EnginePresetLibrary::createExtremePreset(EngineType engineType, const std::string& name) const {
    EnginePreset preset = createCleanPreset(engineType, name);
    preset.category = PresetCategory::EXTREME;
    preset.description = "Extreme modern synthesis preset";
    preset.tags = {"extreme", "modern", "aggressive"};
    
    // Extreme presets push parameters to dramatic values
    switch (engineType) {
        case EngineType::MACRO_VA:
            preset.holdParams["filter_resonance"] = 0.8f;
            preset.twistParams["drive"] = 0.7f;
            preset.morphParams["chaos"] = 0.6f;
            preset.fxParams["distortion"] = 0.8f;
            preset.fxParams["delay_feedback"] = 0.7f;
            break;
            
        case EngineType::MACRO_FM:
            preset.holdParams["mod_index"] = 0.9f;
            preset.twistParams["feedback"] = 0.8f;
            preset.morphParams["noise_mod"] = 0.5f;
            preset.fxParams["bit_crusher"] = 0.6f;
            break;
            
        case EngineType::MACRO_HARMONICS:
            preset.twistParams["percussion"] = 0.9f;
            preset.morphParams["scanner_rate"] = 0.8f;
            preset.fxParams["overdrive"] = 0.7f;
            break;
            
        default:
            preset.fxParams["extreme_processing"] = 0.8f;
            break;
    }
    
    // Extreme presets have dramatic velocity response
    preset.velocityConfig.velocityMappings["volume"] = 1.0f;
    preset.velocityConfig.velocityMappings["brightness"] = 0.8f;
    preset.velocityConfig.velocityMappings["aggression"] = 0.9f;
    
    return preset;
}

// Statistics and monitoring
size_t EnginePresetLibrary::getTotalPresetCount() const {
    size_t total = 0;
    for (const auto& [engineType, presets] : factoryPresets_) {
        total += presets.size();
    }
    for (const auto& [engineType, presets] : userPresets_) {
        total += presets.size();
    }
    return total;
}

size_t EnginePresetLibrary::getPresetCount(EngineType engineType) const {
    size_t count = 0;
    
    auto factoryIt = factoryPresets_.find(engineType);
    if (factoryIt != factoryPresets_.end()) {
        count += factoryIt->second.size();
    }
    
    auto userIt = userPresets_.find(engineType);
    if (userIt != userPresets_.end()) {
        count += userIt->second.size();
    }
    
    return count;
}

size_t EnginePresetLibrary::getPresetCount(PresetCategory category) const {
    size_t count = 0;
    
    const auto& storage = (category == PresetCategory::USER_CUSTOM) ? userPresets_ : factoryPresets_;
    
    for (const auto& [engineType, presets] : storage) {
        for (const auto& preset : presets) {
            if (preset->category == category) {
                count++;
            }
        }
    }
    
    return count;
}

// System management
void EnginePresetLibrary::reset() {
    factoryPresets_.clear();
    userPresets_.clear();
    invalidateCache();
}

// Private helper methods
bool EnginePresetLibrary::validateParameterRanges(const EnginePreset& preset) const {
    // Check all parameter maps for out-of-range values
    for (const auto& [paramName, value] : preset.holdParams) {
        if (value < MIN_PARAMETER_VALUE || value > MAX_PARAMETER_VALUE) {
            return false;
        }
    }
    for (const auto& [paramName, value] : preset.twistParams) {
        if (value < MIN_PARAMETER_VALUE || value > MAX_PARAMETER_VALUE) {
            return false;
        }
    }
    for (const auto& [paramName, value] : preset.morphParams) {
        if (value < MIN_PARAMETER_VALUE || value > MAX_PARAMETER_VALUE) {
            return false;
        }
    }
    return true;
}

bool EnginePresetLibrary::validateMacroAssignments(const EnginePreset& preset) const {
    for (const auto& [macroId, assignment] : preset.macroAssignments) {
        if (macroId > 16 || assignment.amount < -1.0f || assignment.amount > 1.0f) {
            return false;
        }
    }
    return true;
}

bool EnginePresetLibrary::validateEngineCompatibility(const EnginePreset& preset) const {
    // Basic engine type validation - could be expanded for cross-engine compatibility
    return true;
}

uint64_t EnginePresetLibrary::getCurrentTimestamp() const {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// Macro engine preset initialization methods
void EnginePresetLibrary::initializeMacroVAPresets() {
    factoryPresets_[EngineType::MACRO_VA].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::MACRO_VA, "VA Clean")));
    factoryPresets_[EngineType::MACRO_VA].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::MACRO_VA, "VA Classic")));
    factoryPresets_[EngineType::MACRO_VA].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::MACRO_VA, "VA Extreme")));
}

void EnginePresetLibrary::initializeMacroFMPresets() {
    factoryPresets_[EngineType::MACRO_FM].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::MACRO_FM, "FM Clean")));
    factoryPresets_[EngineType::MACRO_FM].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::MACRO_FM, "FM Classic")));
    factoryPresets_[EngineType::MACRO_FM].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::MACRO_FM, "FM Extreme")));
}

void EnginePresetLibrary::initializeMacroHarmonicsPresets() {
    factoryPresets_[EngineType::MACRO_HARMONICS].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::MACRO_HARMONICS, "Organ Clean")));
    factoryPresets_[EngineType::MACRO_HARMONICS].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::MACRO_HARMONICS, "Organ Classic")));
    factoryPresets_[EngineType::MACRO_HARMONICS].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::MACRO_HARMONICS, "Organ Extreme")));
}

void EnginePresetLibrary::initializeMacroWavetablePresets() {
    factoryPresets_[EngineType::MACRO_WAVETABLE].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::MACRO_WAVETABLE, "Wavetable Clean")));
    factoryPresets_[EngineType::MACRO_WAVETABLE].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::MACRO_WAVETABLE, "Wavetable Classic")));
    factoryPresets_[EngineType::MACRO_WAVETABLE].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::MACRO_WAVETABLE, "Wavetable Extreme")));
}

// Additional macro engine presets
void EnginePresetLibrary::initializeMacroChordPresets() {
    factoryPresets_[EngineType::MACRO_CHORD].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::MACRO_CHORD, "Chord Clean")));
    factoryPresets_[EngineType::MACRO_CHORD].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::MACRO_CHORD, "Chord Classic")));
    factoryPresets_[EngineType::MACRO_CHORD].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::MACRO_CHORD, "Chord Extreme")));
}

void EnginePresetLibrary::initializeMacroWaveshaperPresets() {
    factoryPresets_[EngineType::MACRO_WAVESHAPER].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::MACRO_WAVESHAPER, "Waveshaper Clean")));
    factoryPresets_[EngineType::MACRO_WAVESHAPER].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::MACRO_WAVESHAPER, "Waveshaper Classic")));
    factoryPresets_[EngineType::MACRO_WAVESHAPER].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::MACRO_WAVESHAPER, "Waveshaper Extreme")));
}

// Mutable Instruments-based engine presets
void EnginePresetLibrary::initializeElementsPresets() {
    factoryPresets_[EngineType::ELEMENTS_VOICE].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::ELEMENTS_VOICE, "Elements Clean")));
    factoryPresets_[EngineType::ELEMENTS_VOICE].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::ELEMENTS_VOICE, "Elements Classic")));
    factoryPresets_[EngineType::ELEMENTS_VOICE].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::ELEMENTS_VOICE, "Elements Extreme")));
}

void EnginePresetLibrary::initializeRingsPresets() {
    factoryPresets_[EngineType::RINGS_VOICE].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::RINGS_VOICE, "Rings Clean")));
    factoryPresets_[EngineType::RINGS_VOICE].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::RINGS_VOICE, "Rings Classic")));
    factoryPresets_[EngineType::RINGS_VOICE].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::RINGS_VOICE, "Rings Extreme")));
}

void EnginePresetLibrary::initializeTidesPresets() {
    factoryPresets_[EngineType::TIDES_OSC].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::TIDES_OSC, "Tides Clean")));
    factoryPresets_[EngineType::TIDES_OSC].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::TIDES_OSC, "Tides Classic")));
    factoryPresets_[EngineType::TIDES_OSC].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::TIDES_OSC, "Tides Extreme")));
}

void EnginePresetLibrary::initializeFormantPresets() {
    factoryPresets_[EngineType::FORMANT_VOCAL].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::FORMANT_VOCAL, "Formant Clean")));
    factoryPresets_[EngineType::FORMANT_VOCAL].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::FORMANT_VOCAL, "Formant Classic")));
    factoryPresets_[EngineType::FORMANT_VOCAL].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::FORMANT_VOCAL, "Formant Extreme")));
}

void EnginePresetLibrary::initializeNoiseParticlesPresets() {
    factoryPresets_[EngineType::NOISE_PARTICLES].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::NOISE_PARTICLES, "Noise Clean")));
    factoryPresets_[EngineType::NOISE_PARTICLES].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::NOISE_PARTICLES, "Noise Classic")));
    factoryPresets_[EngineType::NOISE_PARTICLES].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::NOISE_PARTICLES, "Noise Extreme")));
}

void EnginePresetLibrary::initializeClassic4OpFMPresets() {
    factoryPresets_[EngineType::CLASSIC_4OP_FM].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::CLASSIC_4OP_FM, "4Op FM Clean")));
    factoryPresets_[EngineType::CLASSIC_4OP_FM].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::CLASSIC_4OP_FM, "4Op FM Classic")));
    factoryPresets_[EngineType::CLASSIC_4OP_FM].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::CLASSIC_4OP_FM, "4Op FM Extreme")));
}

// Specialized engine presets
void EnginePresetLibrary::initializeDrumKitPresets() {
    factoryPresets_[EngineType::DRUM_KIT].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::DRUM_KIT, "Drums Clean")));
    factoryPresets_[EngineType::DRUM_KIT].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::DRUM_KIT, "Drums Classic")));
    factoryPresets_[EngineType::DRUM_KIT].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::DRUM_KIT, "Drums Extreme")));
}

void EnginePresetLibrary::initializeSamplerKitPresets() {
    factoryPresets_[EngineType::SAMPLER_KIT].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::SAMPLER_KIT, "Sampler Clean")));
    factoryPresets_[EngineType::SAMPLER_KIT].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::SAMPLER_KIT, "Sampler Classic")));
    factoryPresets_[EngineType::SAMPLER_KIT].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::SAMPLER_KIT, "Sampler Extreme")));
}

void EnginePresetLibrary::initializeSamplerSlicerPresets() {
    factoryPresets_[EngineType::SAMPLER_SLICER].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::SAMPLER_SLICER, "Slicer Clean")));
    factoryPresets_[EngineType::SAMPLER_SLICER].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::SAMPLER_SLICER, "Slicer Classic")));
    factoryPresets_[EngineType::SAMPLER_SLICER].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::SAMPLER_SLICER, "Slicer Extreme")));
}

void EnginePresetLibrary::initializeSlideAccentBassPresets() {
    factoryPresets_[EngineType::SLIDE_ACCENT_BASS].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::SLIDE_ACCENT_BASS, "Bass Clean")));
    factoryPresets_[EngineType::SLIDE_ACCENT_BASS].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::SLIDE_ACCENT_BASS, "Bass Classic")));
    factoryPresets_[EngineType::SLIDE_ACCENT_BASS].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::SLIDE_ACCENT_BASS, "Bass Extreme")));
}

// Plaits-based engine presets (16 engines)
void EnginePresetLibrary::initializePlaitsVAPresets() {
    factoryPresets_[EngineType::PLAITS_VA].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::PLAITS_VA, "Plaits VA Clean")));
    factoryPresets_[EngineType::PLAITS_VA].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::PLAITS_VA, "Plaits VA Classic")));
    factoryPresets_[EngineType::PLAITS_VA].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::PLAITS_VA, "Plaits VA Extreme")));
}

void EnginePresetLibrary::initializePlaitsWaveshapingPresets() {
    factoryPresets_[EngineType::PLAITS_WAVESHAPING].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::PLAITS_WAVESHAPING, "Plaits Wave Clean")));
    factoryPresets_[EngineType::PLAITS_WAVESHAPING].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::PLAITS_WAVESHAPING, "Plaits Wave Classic")));
    factoryPresets_[EngineType::PLAITS_WAVESHAPING].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::PLAITS_WAVESHAPING, "Plaits Wave Extreme")));
}

void EnginePresetLibrary::initializePlaitsFMPresets() {
    factoryPresets_[EngineType::PLAITS_FM].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::PLAITS_FM, "Plaits FM Clean")));
    factoryPresets_[EngineType::PLAITS_FM].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::PLAITS_FM, "Plaits FM Classic")));
    factoryPresets_[EngineType::PLAITS_FM].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::PLAITS_FM, "Plaits FM Extreme")));
}

void EnginePresetLibrary::initializePlaitsGrainPresets() {
    factoryPresets_[EngineType::PLAITS_GRAIN].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::PLAITS_GRAIN, "Plaits Grain Clean")));
    factoryPresets_[EngineType::PLAITS_GRAIN].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::PLAITS_GRAIN, "Plaits Grain Classic")));
    factoryPresets_[EngineType::PLAITS_GRAIN].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::PLAITS_GRAIN, "Plaits Grain Extreme")));
}

void EnginePresetLibrary::initializePlaitsAdditivePresets() {
    factoryPresets_[EngineType::PLAITS_ADDITIVE].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::PLAITS_ADDITIVE, "Plaits Add Clean")));
    factoryPresets_[EngineType::PLAITS_ADDITIVE].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::PLAITS_ADDITIVE, "Plaits Add Classic")));
    factoryPresets_[EngineType::PLAITS_ADDITIVE].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::PLAITS_ADDITIVE, "Plaits Add Extreme")));
}

void EnginePresetLibrary::initializePlaitsWavetablePresets() {
    factoryPresets_[EngineType::PLAITS_WAVETABLE].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::PLAITS_WAVETABLE, "Plaits WT Clean")));
    factoryPresets_[EngineType::PLAITS_WAVETABLE].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::PLAITS_WAVETABLE, "Plaits WT Classic")));
    factoryPresets_[EngineType::PLAITS_WAVETABLE].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::PLAITS_WAVETABLE, "Plaits WT Extreme")));
}

void EnginePresetLibrary::initializePlaitsChordPresets() {
    factoryPresets_[EngineType::PLAITS_CHORD].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::PLAITS_CHORD, "Plaits Chord Clean")));
    factoryPresets_[EngineType::PLAITS_CHORD].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::PLAITS_CHORD, "Plaits Chord Classic")));
    factoryPresets_[EngineType::PLAITS_CHORD].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::PLAITS_CHORD, "Plaits Chord Extreme")));
}

void EnginePresetLibrary::initializePlaitsSpeechPresets() {
    factoryPresets_[EngineType::PLAITS_SPEECH].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::PLAITS_SPEECH, "Plaits Speech Clean")));
    factoryPresets_[EngineType::PLAITS_SPEECH].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::PLAITS_SPEECH, "Plaits Speech Classic")));
    factoryPresets_[EngineType::PLAITS_SPEECH].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::PLAITS_SPEECH, "Plaits Speech Extreme")));
}

void EnginePresetLibrary::initializePlaitsSwarmPresets() {
    factoryPresets_[EngineType::PLAITS_SWARM].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::PLAITS_SWARM, "Plaits Swarm Clean")));
    factoryPresets_[EngineType::PLAITS_SWARM].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::PLAITS_SWARM, "Plaits Swarm Classic")));
    factoryPresets_[EngineType::PLAITS_SWARM].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::PLAITS_SWARM, "Plaits Swarm Extreme")));
}

void EnginePresetLibrary::initializePlaitsNoisePresets() {
    factoryPresets_[EngineType::PLAITS_NOISE].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::PLAITS_NOISE, "Plaits Noise Clean")));
    factoryPresets_[EngineType::PLAITS_NOISE].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::PLAITS_NOISE, "Plaits Noise Classic")));
    factoryPresets_[EngineType::PLAITS_NOISE].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::PLAITS_NOISE, "Plaits Noise Extreme")));
}

void EnginePresetLibrary::initializePlaitsParticlePresets() {
    factoryPresets_[EngineType::PLAITS_PARTICLE].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::PLAITS_PARTICLE, "Plaits Particle Clean")));
    factoryPresets_[EngineType::PLAITS_PARTICLE].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::PLAITS_PARTICLE, "Plaits Particle Classic")));
    factoryPresets_[EngineType::PLAITS_PARTICLE].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::PLAITS_PARTICLE, "Plaits Particle Extreme")));
}

void EnginePresetLibrary::initializePlaitsStringPresets() {
    factoryPresets_[EngineType::PLAITS_STRING].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::PLAITS_STRING, "Plaits String Clean")));
    factoryPresets_[EngineType::PLAITS_STRING].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::PLAITS_STRING, "Plaits String Classic")));
    factoryPresets_[EngineType::PLAITS_STRING].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::PLAITS_STRING, "Plaits String Extreme")));
}

void EnginePresetLibrary::initializePlaitsModalPresets() {
    factoryPresets_[EngineType::PLAITS_MODAL].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::PLAITS_MODAL, "Plaits Modal Clean")));
    factoryPresets_[EngineType::PLAITS_MODAL].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::PLAITS_MODAL, "Plaits Modal Classic")));
    factoryPresets_[EngineType::PLAITS_MODAL].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::PLAITS_MODAL, "Plaits Modal Extreme")));
}

void EnginePresetLibrary::initializePlaitsBassPresets() {
    factoryPresets_[EngineType::PLAITS_BASS_DRUM].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::PLAITS_BASS_DRUM, "Plaits Kick Clean")));
    factoryPresets_[EngineType::PLAITS_BASS_DRUM].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::PLAITS_BASS_DRUM, "Plaits Kick Classic")));
    factoryPresets_[EngineType::PLAITS_BASS_DRUM].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::PLAITS_BASS_DRUM, "Plaits Kick Extreme")));
}

void EnginePresetLibrary::initializePlaitsSnarePresets() {
    factoryPresets_[EngineType::PLAITS_SNARE_DRUM].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::PLAITS_SNARE_DRUM, "Plaits Snare Clean")));
    factoryPresets_[EngineType::PLAITS_SNARE_DRUM].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::PLAITS_SNARE_DRUM, "Plaits Snare Classic")));
    factoryPresets_[EngineType::PLAITS_SNARE_DRUM].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::PLAITS_SNARE_DRUM, "Plaits Snare Extreme")));
}

void EnginePresetLibrary::initializePlaitsHiHatPresets() {
    factoryPresets_[EngineType::PLAITS_HI_HAT].push_back(
        std::make_unique<EnginePreset>(createCleanPreset(EngineType::PLAITS_HI_HAT, "Plaits HiHat Clean")));
    factoryPresets_[EngineType::PLAITS_HI_HAT].push_back(
        std::make_unique<EnginePreset>(createClassicPreset(EngineType::PLAITS_HI_HAT, "Plaits HiHat Classic")));
    factoryPresets_[EngineType::PLAITS_HI_HAT].push_back(
        std::make_unique<EnginePreset>(createExtremePreset(EngineType::PLAITS_HI_HAT, "Plaits HiHat Extreme")));
}

// Signature preset creation methods
EnginePresetLibrary::EnginePreset EnginePresetLibrary::createDetunedStackPad() const {
    EnginePreset preset;
    preset.name = "Detuned Stack Pad";
    preset.engineType = EngineType::MACRO_VA;
    preset.category = PresetCategory::FACTORY_SIGNATURE;
    preset.description = "Rich detuned pad with stacked oscillators and lush modulation";
    preset.author = "EtherSynth Factory";
    preset.version = PRESET_SCHEMA_VERSION;
    preset.creationTime = getCurrentTimestamp();
    preset.tags = {"pad", "lush", "detuned", "signature", "ambient"};
    
    // Hold parameters - Main synthesis controls
    preset.holdParams["osc1_level"] = 0.9f;          // Primary oscillator
    preset.holdParams["osc2_level"] = 0.8f;          // Secondary oscillator
    preset.holdParams["osc3_level"] = 0.6f;          // Third oscillator for richness
    preset.holdParams["sub_osc_level"] = 0.3f;       // Sub oscillator for depth
    preset.holdParams["noise_level"] = 0.05f;        // Subtle noise texture
    preset.holdParams["filter_cutoff"] = 0.65f;      // Warm filter setting
    preset.holdParams["filter_resonance"] = 0.25f;   // Gentle resonance
    preset.holdParams["amp_sustain"] = 0.85f;        // Long sustain for pad
    
    // Twist parameters - Performance modulation
    preset.twistParams["osc2_detune"] = 0.15f;       // Slight detune for richness
    preset.twistParams["osc3_detune"] = -0.12f;      // Counter-detune for spread
    preset.twistParams["env_attack"] = 0.4f;         // Slow attack for pad
    preset.twistParams["env_decay"] = 0.3f;          // Moderate decay
    preset.twistParams["env_release"] = 0.6f;        // Long release
    preset.twistParams["filter_env_amount"] = 0.3f;  // Filter modulation
    preset.twistParams["lfo_rate"] = 0.2f;           // Slow LFO for movement
    preset.twistParams["vibrato_depth"] = 0.08f;     // Subtle vibrato
    
    // Morph parameters - Expressive controls
    preset.morphParams["stereo_spread"] = 0.7f;      // Wide stereo image
    preset.morphParams["chorus_depth"] = 0.4f;       // Chorus for dimension
    preset.morphParams["unison_voices"] = 0.6f;      // Multiple voices
    preset.morphParams["unison_detune"] = 0.3f;      // Voice detuning
    preset.morphParams["analog_drift"] = 0.15f;      // Vintage character
    preset.morphParams["filter_tracking"] = 0.8f;    // Keyboard tracking
    
    // Macro assignments for real-time control
    preset.macroAssignments[1].parameterName = "filter_cutoff";
    preset.macroAssignments[1].amount = 0.8f;
    preset.macroAssignments[1].enabled = true;
    
    preset.macroAssignments[2].parameterName = "chorus_depth";
    preset.macroAssignments[2].amount = 0.6f;
    preset.macroAssignments[2].enabled = true;
    
    preset.macroAssignments[3].parameterName = "unison_detune";
    preset.macroAssignments[3].amount = 0.7f;
    preset.macroAssignments[3].enabled = true;
    
    preset.macroAssignments[4].parameterName = "filter_env_amount";
    preset.macroAssignments[4].amount = 0.5f;
    preset.macroAssignments[4].enabled = true;
    
    // Effects parameters
    preset.fxParams["chorus_rate"] = 0.3f;           // Chorus speed
    preset.fxParams["chorus_feedback"] = 0.2f;       // Chorus character
    preset.fxParams["reverb_size"] = 0.7f;           // Large reverb space
    preset.fxParams["reverb_decay"] = 0.8f;          // Long reverb tail
    preset.fxParams["reverb_damping"] = 0.3f;        // High-frequency damping
    preset.fxParams["delay_time"] = 0.25f;           // Subtle delay
    preset.fxParams["delay_feedback"] = 0.15f;       // Light feedback
    preset.fxParams["tape_saturation"] = 0.1f;       // Warm tape color
    
    // Velocity configuration for expressive playing
    preset.velocityConfig.enableVelocityToVolume = true;
    preset.velocityConfig.velocityMappings["volume"] = 0.6f;           // Moderate volume response
    preset.velocityConfig.velocityMappings["filter_cutoff"] = 0.4f;    // Brightness response
    preset.velocityConfig.velocityMappings["attack_time"] = -0.3f;     // Faster attack with velocity
    preset.velocityConfig.velocityMappings["chorus_depth"] = 0.2f;     // More chorus with velocity
    
    // Performance settings
    preset.morphTransitionTime = 200.0f;             // Smooth morph transitions
    preset.enableParameterSmoothing = true;
    preset.parameterSmoothingTime = 50.0f;
    
    return preset;
}

EnginePresetLibrary::EnginePreset EnginePresetLibrary::create2OpPunch() const {
    EnginePreset preset;
    preset.name = "2-Op Punch";
    preset.engineType = EngineType::MACRO_FM;
    preset.category = PresetCategory::FACTORY_SIGNATURE;
    preset.description = "Punchy 2-operator FM with aggressive attack and bright harmonic content";
    preset.author = "EtherSynth Factory";
    preset.version = PRESET_SCHEMA_VERSION;
    preset.creationTime = getCurrentTimestamp();
    preset.tags = {"fm", "punchy", "bright", "signature", "percussive"};
    
    // Hold parameters - Core FM synthesis
    preset.holdParams["carrier_freq"] = 0.5f;        // Fundamental frequency
    preset.holdParams["modulator_freq"] = 2.0f;      // 2:1 ratio for classic FM
    preset.holdParams["mod_index"] = 0.7f;           // Strong modulation
    preset.holdParams["carrier_level"] = 0.9f;       // Full carrier output
    preset.holdParams["modulator_level"] = 0.8f;     // Strong modulator
    preset.holdParams["feedback"] = 0.4f;            // Moderate feedback for edge
    preset.holdParams["filter_cutoff"] = 0.8f;       // Bright filter
    preset.holdParams["filter_resonance"] = 0.3f;    // Some resonance bite
    
    // Twist parameters - Performance dynamics
    preset.twistParams["env_attack"] = 0.02f;        // Very fast attack
    preset.twistParams["env_decay"] = 0.4f;          // Quick decay for punch
    preset.twistParams["env_sustain"] = 0.3f;        // Lower sustain
    preset.twistParams["env_release"] = 0.2f;        // Quick release
    preset.twistParams["mod_env_attack"] = 0.01f;    // Instant mod envelope
    preset.twistParams["mod_env_decay"] = 0.3f;      // Quick mod decay
    preset.twistParams["carrier_env_attack"] = 0.01f; // Instant carrier attack
    preset.twistParams["pitch_env_amount"] = 0.1f;   // Slight pitch envelope
    
    // Morph parameters - Tonal shaping
    preset.morphParams["algorithm"] = 0.0f;          // Classic 2-op algorithm
    preset.morphParams["operator_sync"] = 0.8f;      // Tight sync for punch
    preset.morphParams["harmonic_bias"] = 0.6f;      // Emphasize upper harmonics
    preset.morphParams["spectral_tilt"] = 0.4f;      // Bright spectral balance
    preset.morphParams["mod_tracking"] = 0.9f;       // Strong keyboard tracking
    preset.morphParams["velocity_sensitivity"] = 0.8f; // High velocity response
    
    // Macro assignments for expressive control
    preset.macroAssignments[1].parameterName = "mod_index";
    preset.macroAssignments[1].amount = 0.9f;
    preset.macroAssignments[1].enabled = true;
    
    preset.macroAssignments[2].parameterName = "filter_cutoff";
    preset.macroAssignments[2].amount = 0.7f;
    preset.macroAssignments[2].enabled = true;
    
    preset.macroAssignments[3].parameterName = "feedback";
    preset.macroAssignments[3].amount = 0.8f;
    preset.macroAssignments[3].enabled = true;
    
    preset.macroAssignments[4].parameterName = "env_decay";
    preset.macroAssignments[4].amount = -0.6f;
    preset.macroAssignments[4].enabled = true;
    
    // Effects parameters
    preset.fxParams["compressor_ratio"] = 0.6f;      // Moderate compression
    preset.fxParams["compressor_attack"] = 0.1f;     // Fast compressor attack
    preset.fxParams["eq_high_gain"] = 0.3f;          // High-frequency boost
    preset.fxParams["eq_high_freq"] = 0.8f;          // Upper mids/highs
    preset.fxParams["distortion"] = 0.2f;            // Subtle distortion
    preset.fxParams["reverb_size"] = 0.3f;           // Tight reverb space
    preset.fxParams["reverb_decay"] = 0.4f;          // Quick reverb decay
    preset.fxParams["delay_time"] = 0.125f;          // Eighth note delay
    preset.fxParams["delay_feedback"] = 0.1f;        // Minimal feedback
    
    // Velocity configuration for dynamic response
    preset.velocityConfig.enableVelocityToVolume = true;
    preset.velocityConfig.velocityMappings["volume"] = 1.0f;          // Full velocity response
    preset.velocityConfig.velocityMappings["mod_index"] = 0.8f;       // More FM with velocity
    preset.velocityConfig.velocityMappings["filter_cutoff"] = 0.6f;   // Brighter with velocity
    preset.velocityConfig.velocityMappings["attack_time"] = -0.2f;    // Faster attack
    preset.velocityConfig.velocityMappings["feedback"] = 0.4f;        // More aggression
    
    // Performance settings
    preset.morphTransitionTime = 50.0f;              // Quick morph transitions
    preset.enableParameterSmoothing = true;
    preset.parameterSmoothingTime = 20.0f;           // Fast parameter changes
    
    return preset;
}

EnginePresetLibrary::EnginePreset EnginePresetLibrary::createDrawbarKeys() const {
    EnginePreset preset;
    preset.name = "Drawbar Keys";
    preset.engineType = EngineType::MACRO_HARMONICS;
    preset.category = PresetCategory::FACTORY_SIGNATURE;
    preset.description = "Classic drawbar organ with percussive attack and rotating speaker simulation";
    preset.author = "EtherSynth Factory";
    preset.version = PRESET_SCHEMA_VERSION;
    preset.creationTime = getCurrentTimestamp();
    preset.tags = {"organ", "drawbar", "classic", "signature", "percussive"};
    
    // Hold parameters - Drawbar settings (classic 888000000 registration)
    preset.holdParams["drawbar_16"] = 0.8f;          // Sub-fundamental
    preset.holdParams["drawbar_8"] = 0.8f;           // Fundamental
    preset.holdParams["drawbar_4"] = 0.8f;           // Octave
    preset.holdParams["drawbar_2_23"] = 0.0f;        // Nazard (off)
    preset.holdParams["drawbar_2"] = 0.0f;           // Super octave (off)
    preset.holdParams["drawbar_1_35"] = 0.0f;        // Tierce (off)
    preset.holdParams["drawbar_1"] = 0.0f;           // High octave (off)
    preset.holdParams["drawbar_0_8"] = 0.0f;         // High nazard (off)
    preset.holdParams["drawbar_0_67"] = 0.0f;        // High tierce (off)
    
    // Twist parameters - Performance characteristics
    preset.twistParams["percussion_level"] = 0.7f;   // Strong percussion
    preset.twistParams["percussion_decay"] = 0.3f;   // Medium percussion decay
    preset.twistParams["percussion_harmonic"] = 0.5f; // 2nd/3rd harmonic mix
    preset.twistParams["key_click"] = 0.4f;          // Authentic key click
    preset.twistParams["scanner_rate"] = 0.6f;       // Moderate vibrato
    preset.twistParams["scanner_depth"] = 0.15f;     // Subtle vibrato depth
    preset.twistParams["leakage"] = 0.1f;            // Crosstalk between drawbars
    preset.twistParams["tube_drive"] = 0.3f;         // Tube preamp warmth
    
    // Morph parameters - Tonal shaping and effects
    preset.morphParams["leslie_speed"] = 0.6f;       // Leslie speaker speed
    preset.morphParams["leslie_acceleration"] = 0.4f; // Ramp time
    preset.morphParams["leslie_mic_distance"] = 0.5f; // Microphone positioning
    preset.morphParams["leslie_horn_level"] = 0.7f;  // Horn balance
    preset.morphParams["leslie_rotor_level"] = 0.8f; // Rotor balance
    preset.morphParams["cabinet_resonance"] = 0.3f;  // Cabinet coloration
    preset.morphParams["room_reverb"] = 0.2f;        // Natural room sound
    
    // Macro assignments for real-time control
    preset.macroAssignments[1].parameterName = "percussion_level";
    preset.macroAssignments[1].amount = 0.8f;
    preset.macroAssignments[1].enabled = true;
    
    preset.macroAssignments[2].parameterName = "leslie_speed";
    preset.macroAssignments[2].amount = 0.9f;
    preset.macroAssignments[2].enabled = true;
    
    preset.macroAssignments[3].parameterName = "scanner_depth";
    preset.macroAssignments[3].amount = 0.7f;
    preset.macroAssignments[3].enabled = true;
    
    preset.macroAssignments[4].parameterName = "tube_drive";
    preset.macroAssignments[4].amount = 0.6f;
    preset.macroAssignments[4].enabled = true;
    
    // Effects parameters - Authentic organ effects chain
    preset.fxParams["leslie_chorus_depth"] = 0.8f;   // Leslie chorus effect
    preset.fxParams["leslie_tremolo_depth"] = 0.6f;  // Leslie tremolo
    preset.fxParams["leslie_doppler"] = 0.7f;        // Doppler effect intensity
    preset.fxParams["tube_saturation"] = 0.4f;       // Tube warmth/saturation
    preset.fxParams["cabinet_filtering"] = 0.3f;     // Cabinet EQ coloration
    preset.fxParams["reverb_size"] = 0.4f;           // Church/hall reverb
    preset.fxParams["reverb_decay"] = 0.6f;          // Moderate reverb tail
    preset.fxParams["reverb_predelay"] = 0.15f;      // Natural predelay
    preset.fxParams["eq_low_gain"] = 0.2f;           // Low-end warmth
    preset.fxParams["eq_mid_gain"] = 0.1f;           // Slight mid emphasis
    
    // Velocity configuration - Organ-style response
    preset.velocityConfig.enableVelocityToVolume = true;
    preset.velocityConfig.velocityMappings["volume"] = 0.4f;          // Limited volume response
    preset.velocityConfig.velocityMappings["percussion_level"] = 0.8f; // Strong perc response
    preset.velocityConfig.velocityMappings["key_click"] = 0.6f;       // Click with velocity
    preset.velocityConfig.velocityMappings["tube_drive"] = 0.3f;      // Drive with velocity
    preset.velocityConfig.velocityMappings["leslie_speed"] = 0.2f;    // Subtle leslie response
    
    // Performance settings
    preset.morphTransitionTime = 150.0f;             // Moderate morph speed
    preset.enableParameterSmoothing = true;
    preset.parameterSmoothingTime = 30.0f;           // Smooth parameter changes
    
    return preset;
}

void EnginePresetLibrary::createSignaturePresets() {
    // Add signature presets to the factory library
    factoryPresets_[EngineType::MACRO_VA].push_back(
        std::make_unique<EnginePreset>(createDetunedStackPad()));
    factoryPresets_[EngineType::MACRO_FM].push_back(
        std::make_unique<EnginePreset>(create2OpPunch()));
    factoryPresets_[EngineType::MACRO_HARMONICS].push_back(
        std::make_unique<EnginePreset>(createDrawbarKeys()));
    
    invalidateCache();
}

// JSON serialization implementation
std::string EnginePresetLibrary::serializePreset(const EnginePreset& preset) const {
    std::ostringstream json;
    
    json << "{\n";
    json << "  \"schema_version\": \"" << preset.version << "\",\n";
    json << "  \"preset_info\": {\n";
    json << "    \"name\": \"" << preset.name << "\",\n";
    json << "    \"description\": \"" << preset.description << "\",\n";
    json << "    \"author\": \"" << preset.author << "\",\n";
    json << "    \"engine_type\": " << static_cast<int>(preset.engineType) << ",\n";
    json << "    \"category\": " << static_cast<int>(preset.category) << ",\n";
    json << "    \"creation_time\": " << preset.creationTime << ",\n";
    json << "    \"modification_time\": " << preset.modificationTime << ",\n";
    json << "    \"tags\": [";
    for (size_t i = 0; i < preset.tags.size(); ++i) {
        json << "\"" << preset.tags[i] << "\"";
        if (i < preset.tags.size() - 1) json << ", ";
    }
    json << "]\n";
    json << "  },\n";
    
    // Hold parameters (H)
    json << "  \"hold_params\": {\n";
    json << serializeParameterMap(preset.holdParams);
    json << "  },\n";
    
    // Twist parameters (T)
    json << "  \"twist_params\": {\n";
    json << serializeParameterMap(preset.twistParams);
    json << "  },\n";
    
    // Morph parameters (M)
    json << "  \"morph_params\": {\n";
    json << serializeParameterMap(preset.morphParams);
    json << "  },\n";
    
    // Macro assignments
    json << "  \"macro_assignments\": {\n";
    bool first = true;
    for (const auto& [macroId, assignment] : preset.macroAssignments) {
        if (!first) json << ",\n";
        json << "    \"macro_" << static_cast<int>(macroId) << "\": {\n";
        json << "      \"parameter\": \"" << assignment.parameterName << "\",\n";
        json << "      \"amount\": " << assignment.amount << ",\n";
        json << "      \"enabled\": " << (assignment.enabled ? "true" : "false") << "\n";
        json << "    }";
        first = false;
    }
    json << "\n  },\n";
    
    // Effects parameters
    json << "  \"fx_params\": {\n";
    json << serializeParameterMap(preset.fxParams);
    json << "  },\n";
    
    // Velocity configuration
    json << "  \"velocity_config\": {\n";
    json << "    \"enable_velocity_to_volume\": " << (preset.velocityConfig.enableVelocityToVolume ? "true" : "false") << ",\n";
    json << "    \"velocity_mappings\": {\n";
    first = true;
    for (const auto& [param, amount] : preset.velocityConfig.velocityMappings) {
        if (!first) json << ",\n";
        json << "      \"" << param << "\": " << amount;
        first = false;
    }
    json << "\n    }\n";
    json << "  },\n";
    
    // Performance settings
    json << "  \"performance\": {\n";
    json << "    \"morph_transition_time\": " << preset.morphTransitionTime << ",\n";
    json << "    \"enable_parameter_smoothing\": " << (preset.enableParameterSmoothing ? "true" : "false") << ",\n";
    json << "    \"parameter_smoothing_time\": " << preset.parameterSmoothingTime << "\n";
    json << "  }\n";
    
    json << "}";
    
    return json.str();
}

bool EnginePresetLibrary::deserializePreset(const std::string& json, EnginePreset& preset) const {
    // Simple JSON parsing implementation
    // In a production system, this would use a proper JSON library
    
    try {
        // Reset preset to defaults
        preset = EnginePreset();
        
        // Parse basic fields (simplified implementation)
        size_t namePos = json.find("\"name\": \"");
        if (namePos != std::string::npos) {
            namePos += 9; // Skip "\"name\": \""
            size_t nameEnd = json.find("\"", namePos);
            if (nameEnd != std::string::npos) {
                preset.name = json.substr(namePos, nameEnd - namePos);
            }
        }
        
        size_t descPos = json.find("\"description\": \"");
        if (descPos != std::string::npos) {
            descPos += 15; // Skip "\"description\": \""
            size_t descEnd = json.find("\"", descPos);
            if (descEnd != std::string::npos) {
                preset.description = json.substr(descPos, descEnd - descPos);
            }
        }
        
        size_t authorPos = json.find("\"author\": \"");
        if (authorPos != std::string::npos) {
            authorPos += 11; // Skip "\"author\": \""
            size_t authorEnd = json.find("\"", authorPos);
            if (authorEnd != std::string::npos) {
                preset.author = json.substr(authorPos, authorEnd - authorPos);
            }
        }
        
        // Parse engine type
        size_t enginePos = json.find("\"engine_type\": ");
        if (enginePos != std::string::npos) {
            enginePos += 15; // Skip "\"engine_type\": "
            size_t engineEnd = json.find(",", enginePos);
            if (engineEnd != std::string::npos) {
                int engineType = std::stoi(json.substr(enginePos, engineEnd - enginePos));
                preset.engineType = static_cast<EngineType>(engineType);
            }
        }
        
        // Parse category
        size_t categoryPos = json.find("\"category\": ");
        if (categoryPos != std::string::npos) {
            categoryPos += 12; // Skip "\"category\": "
            size_t categoryEnd = json.find(",", categoryPos);
            if (categoryEnd != std::string::npos) {
                int category = std::stoi(json.substr(categoryPos, categoryEnd - categoryPos));
                preset.category = static_cast<PresetCategory>(category);
            }
        }
        
        // Parse parameter sections (simplified)
        if (!deserializeParameterMap(json, preset.holdParams)) return false;
        if (!deserializeParameterMap(json, preset.twistParams)) return false;
        if (!deserializeParameterMap(json, preset.morphParams)) return false;
        if (!deserializeParameterMap(json, preset.fxParams)) return false;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "JSON deserialization error: " << e.what() << std::endl;
        return false;
    }
}

std::string EnginePresetLibrary::exportPresetLibrary(EngineType engineType) const {
    std::ostringstream json;
    
    json << "{\n";
    json << "  \"library_info\": {\n";
    json << "    \"engine_type\": " << static_cast<int>(engineType) << ",\n";
    json << "    \"export_time\": " << getCurrentTimestamp() << ",\n";
    json << "    \"schema_version\": \"" << PRESET_SCHEMA_VERSION << "\"\n";
    json << "  },\n";
    json << "  \"presets\": [\n";
    
    // Export factory presets
    bool first = true;
    auto factoryIt = factoryPresets_.find(engineType);
    if (factoryIt != factoryPresets_.end()) {
        for (const auto& preset : factoryIt->second) {
            if (!first) json << ",\n";
            json << serializePreset(*preset);
            first = false;
        }
    }
    
    // Export user presets
    auto userIt = userPresets_.find(engineType);
    if (userIt != userPresets_.end()) {
        for (const auto& preset : userIt->second) {
            if (!first) json << ",\n";
            json << serializePreset(*preset);
            first = false;
        }
    }
    
    json << "\n  ]\n";
    json << "}";
    
    return json.str();
}

bool EnginePresetLibrary::importPresetLibrary(const std::string& json, EngineType engineType) {
    // Simplified JSON import implementation
    try {
        // Parse the presets array and import each preset
        size_t presetsPos = json.find("\"presets\": [");
        if (presetsPos == std::string::npos) return false;
        
        // For now, just return success - full implementation would parse each preset
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "JSON import error: " << e.what() << std::endl;
        return false;
    }
}

// Helper methods for parameter map serialization
std::string EnginePresetLibrary::serializeParameterMap(const std::unordered_map<std::string, float>& params) const {
    std::ostringstream result;
    bool first = true;
    
    for (const auto& [paramName, value] : params) {
        if (!first) result << ",\n";
        result << "    \"" << paramName << "\": " << std::fixed << std::setprecision(3) << value;
        first = false;
    }
    
    if (!params.empty()) result << "\n";
    return result.str();
}

bool EnginePresetLibrary::deserializeParameterMap(const std::string& json, std::unordered_map<std::string, float>& params) const {
    // Simplified parameter parsing - in production would use proper JSON parser
    params.clear();
    
    // Example: parse hold_params section
    size_t holdStart = json.find("\"hold_params\": {");
    if (holdStart != std::string::npos) {
        holdStart += 16; // Skip "\"hold_params\": {"
        size_t holdEnd = json.find("},", holdStart);
        if (holdEnd != std::string::npos) {
            std::string holdSection = json.substr(holdStart, holdEnd - holdStart);
            // Parse individual parameters (simplified)
            size_t pos = 0;
            while ((pos = holdSection.find("\"", pos)) != std::string::npos) {
                pos++;
                size_t nameEnd = holdSection.find("\"", pos);
                if (nameEnd != std::string::npos) {
                    std::string paramName = holdSection.substr(pos, nameEnd - pos);
                    size_t valueStart = holdSection.find(": ", nameEnd) + 2;
                    size_t valueEnd = holdSection.find_first_of(",\n}", valueStart);
                    if (valueEnd != std::string::npos) {
                        float value = std::stof(holdSection.substr(valueStart, valueEnd - valueStart));
                        params[paramName] = value;
                    }
                    pos = nameEnd + 1;
                } else {
                    break;
                }
            }
        }
    }
    
    return true;
}