#include "EngineParameterLayouts.h"
#include <cassert>

std::unordered_map<EngineType, EngineParameterLayout> EngineParameterMappings::layouts_;
bool EngineParameterMappings::initialized_ = false;

const EngineParameterLayout& EngineParameterMappings::getLayout(EngineType engineType) {
    if (!initialized_) {
        initializeLayouts();
    }
    
    auto it = layouts_.find(engineType);
    assert(it != layouts_.end() && "Engine type not found in parameter layouts");
    return it->second;
}

ParameterID EngineParameterMappings::getParameterAt(EngineType engineType, int keyIndex) {
    assert(keyIndex >= 0 && keyIndex < 16 && "Key index out of range");
    return getLayout(engineType).parameters[keyIndex];
}

const char* EngineParameterMappings::getParameterName(EngineType engineType, int keyIndex) {
    assert(keyIndex >= 0 && keyIndex < 16 && "Key index out of range");
    return getLayout(engineType).displayNames[keyIndex];
}

std::pair<float, float> EngineParameterMappings::getParameterRange(EngineType engineType, int keyIndex) {
    assert(keyIndex >= 0 && keyIndex < 16 && "Key index out of range");
    return getLayout(engineType).ranges[keyIndex];
}

EngineParameterLayout::ParameterGroup EngineParameterMappings::getParameterGroup(EngineType engineType, int keyIndex) {
    assert(keyIndex >= 0 && keyIndex < 16 && "Key index out of range");
    return getLayout(engineType).groups[keyIndex];
}

void EngineParameterMappings::initializeLayouts() {
    if (initialized_) return;
    
    layouts_[EngineType::MACRO_VA] = createMacroVALayout();
    layouts_[EngineType::MACRO_FM] = createMacroFMLayout();
    layouts_[EngineType::MACRO_WAVETABLE] = createMacroWTLayout();
    layouts_[EngineType::MACRO_WAVESHAPER] = createMacroWSLayout();
    layouts_[EngineType::MACRO_CHORD] = createMacroChordLayout();
    layouts_[EngineType::MACRO_HARMONICS] = createMacroHarmonicsLayout();
    layouts_[EngineType::FORMANT_VOCAL] = createFormantVocalLayout();
    layouts_[EngineType::NOISE_PARTICLES] = createNoiseParticlesLayout();
    layouts_[EngineType::TIDES_OSC] = createTidesOscLayout();
    layouts_[EngineType::RINGS_VOICE] = createRingsVoiceLayout();
    layouts_[EngineType::ELEMENTS_VOICE] = createElementsVoiceLayout();
    layouts_[EngineType::DRUM_KIT] = createDrumKitLayout();
    layouts_[EngineType::SAMPLER_KIT] = createSamplerKitLayout();
    layouts_[EngineType::SAMPLER_SLICER] = createSamplerSlicerLayout();
    
    initialized_ = true;
}

// MACRO VA: Analog-style virtual analog synthesis
EngineParameterLayout EngineParameterMappings::createMacroVALayout() {
    EngineParameterLayout layout;
    
    // OSC Group (Keys 1-4): Core synthesis
    layout.parameters[0] = ParameterID::OSC_MIX;      // Key 1: Osc Mix
    layout.parameters[1] = ParameterID::TIMBRE;       // Key 2: Timbre  
    layout.parameters[2] = ParameterID::DETUNE;       // Key 3: Detune
    layout.parameters[3] = ParameterID::SUB_LEVEL;    // Key 4: Sub Level
    
    // FILTER Group (Keys 5-8): Filter & tone shaping
    layout.parameters[4] = ParameterID::FILTER_CUTOFF;    // Key 5: Cutoff
    layout.parameters[5] = ParameterID::FILTER_RESONANCE; // Key 6: Resonance  
    layout.parameters[6] = ParameterID::FILTER_TYPE;      // Key 7: Filter Type
    layout.parameters[7] = ParameterID::SUB_ANCHOR;       // Key 8: Sub Anchor
    
    // ENV Group (Keys 9-12): Envelope & dynamics
    layout.parameters[8] = ParameterID::ATTACK;       // Key 9: Attack
    layout.parameters[9] = ParameterID::DECAY;        // Key 10: Decay
    layout.parameters[10] = ParameterID::SUSTAIN;     // Key 11: Sustain
    layout.parameters[11] = ParameterID::RELEASE;     // Key 12: Release
    
    // FX Group (Keys 13-16): Effects & mix
    layout.parameters[12] = ParameterID::LFO_RATE;    // Key 13: LFO Rate
    layout.parameters[13] = ParameterID::LFO_DEPTH;   // Key 14: LFO Depth
    layout.parameters[14] = ParameterID::VOLUME;      // Key 15: Volume
    layout.parameters[15] = ParameterID::PAN;         // Key 16: Pan
    
    // Display names
    const char* names[] = {
        "OSC MIX", "TIMBRE", "DETUNE", "SUB LEV",
        "CUTOFF", "RESO", "TYPE", "SUB ANC", 
        "ATTACK", "DECAY", "SUSTAIN", "RELEASE",
        "LFO RT", "LFO DEP", "VOLUME", "PAN"
    };
    for (int i = 0; i < 16; i++) {
        layout.displayNames[i] = names[i];
    }
    
    // Parameter ranges
    std::pair<float, float> ranges[] = {
        {0.0f, 1.0f}, {0.0f, 1.0f}, {-1.0f, 1.0f}, {0.0f, 1.0f},  // OSC
        {20.0f, 20000.0f}, {0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f}, // FILTER
        {0.001f, 10.0f}, {0.001f, 10.0f}, {0.0f, 1.0f}, {0.001f, 10.0f}, // ENV
        {0.1f, 20.0f}, {0.0f, 1.0f}, {0.0f, 1.0f}, {-1.0f, 1.0f}  // FX
    };
    for (int i = 0; i < 16; i++) {
        layout.ranges[i] = ranges[i];
    }
    
    // Units
    const char* units[] = {
        "%", "%", "ct", "%",        // OSC
        "Hz", "%", "", "%",         // FILTER  
        "s", "s", "%", "s",         // ENV
        "Hz", "%", "%", ""          // FX
    };
    for (int i = 0; i < 16; i++) {
        layout.units[i] = units[i];
    }
    
    // Parameter groups
    for (int i = 0; i < 4; i++) layout.groups[i] = EngineParameterLayout::ParameterGroup::OSC;
    for (int i = 4; i < 8; i++) layout.groups[i] = EngineParameterLayout::ParameterGroup::FILTER;
    for (int i = 8; i < 12; i++) layout.groups[i] = EngineParameterLayout::ParameterGroup::ENV;
    for (int i = 12; i < 16; i++) layout.groups[i] = EngineParameterLayout::ParameterGroup::FX;
    
    return layout;
}

// MACRO FM: Frequency modulation synthesis
EngineParameterLayout EngineParameterMappings::createMacroFMLayout() {
    EngineParameterLayout layout;
    
    // OSC Group (Keys 1-4): FM synthesis core
    layout.parameters[0] = ParameterID::HARMONICS;    // Key 1: Algorithm
    layout.parameters[1] = ParameterID::TIMBRE;       // Key 2: Ratio
    layout.parameters[2] = ParameterID::MORPH;        // Key 3: Index
    layout.parameters[3] = ParameterID::OSC_MIX;      // Key 4: Feedback
    
    // FILTER Group (Keys 5-8): Post-processing
    layout.parameters[4] = ParameterID::FILTER_CUTOFF;
    layout.parameters[5] = ParameterID::FILTER_RESONANCE;
    layout.parameters[6] = ParameterID::FILTER_TYPE;
    layout.parameters[7] = ParameterID::DETUNE;       // Key 8: Fine Tune
    
    // ENV Group (Keys 9-12): Envelope & dynamics
    layout.parameters[8] = ParameterID::ATTACK;
    layout.parameters[9] = ParameterID::DECAY;
    layout.parameters[10] = ParameterID::SUSTAIN;
    layout.parameters[11] = ParameterID::RELEASE;
    
    // FX Group (Keys 13-16): Effects & mix
    layout.parameters[12] = ParameterID::LFO_RATE;
    layout.parameters[13] = ParameterID::LFO_DEPTH;
    layout.parameters[14] = ParameterID::VOLUME;
    layout.parameters[15] = ParameterID::PAN;
    
    // Display names for FM
    const char* names[] = {
        "ALGO", "RATIO", "INDEX", "FEEDBK",
        "CUTOFF", "RESO", "TYPE", "TUNE",
        "ATTACK", "DECAY", "SUSTAIN", "RELEASE", 
        "LFO RT", "LFO DEP", "VOLUME", "PAN"
    };
    for (int i = 0; i < 16; i++) {
        layout.displayNames[i] = names[i];
    }
    
    // FM-specific ranges
    std::pair<float, float> ranges[] = {
        {1.0f, 32.0f}, {0.25f, 16.0f}, {0.0f, 8.0f}, {0.0f, 1.0f},    // OSC/FM
        {20.0f, 20000.0f}, {0.0f, 1.0f}, {0.0f, 1.0f}, {-100.0f, 100.0f}, // FILTER
        {0.001f, 10.0f}, {0.001f, 10.0f}, {0.0f, 1.0f}, {0.001f, 10.0f},   // ENV
        {0.1f, 20.0f}, {0.0f, 1.0f}, {0.0f, 1.0f}, {-1.0f, 1.0f}          // FX
    };
    for (int i = 0; i < 16; i++) {
        layout.ranges[i] = ranges[i];
    }
    
    // Units for FM
    const char* units[] = {
        "", "", "", "%",            // FM
        "Hz", "%", "", "ct",        // FILTER
        "s", "s", "%", "s",         // ENV
        "Hz", "%", "%", ""          // FX
    };
    for (int i = 0; i < 16; i++) {
        layout.units[i] = units[i];
    }
    
    // Parameter groups (same structure)
    for (int i = 0; i < 4; i++) layout.groups[i] = EngineParameterLayout::ParameterGroup::OSC;
    for (int i = 4; i < 8; i++) layout.groups[i] = EngineParameterLayout::ParameterGroup::FILTER;
    for (int i = 8; i < 12; i++) layout.groups[i] = EngineParameterLayout::ParameterGroup::ENV;
    for (int i = 12; i < 16; i++) layout.groups[i] = EngineParameterLayout::ParameterGroup::FX;
    
    return layout;
}

// Helper function for other engines - creates basic template
static EngineParameterLayout createBasicLayout(
    const char* osc1, const char* osc2, const char* osc3, const char* osc4) {
    
    EngineParameterLayout layout;
    
    // Standard parameter assignment
    layout.parameters[0] = ParameterID::HARMONICS;
    layout.parameters[1] = ParameterID::TIMBRE;
    layout.parameters[2] = ParameterID::MORPH;
    layout.parameters[3] = ParameterID::OSC_MIX;
    layout.parameters[4] = ParameterID::FILTER_CUTOFF;
    layout.parameters[5] = ParameterID::FILTER_RESONANCE;
    layout.parameters[6] = ParameterID::FILTER_TYPE;
    layout.parameters[7] = ParameterID::DETUNE;
    layout.parameters[8] = ParameterID::ATTACK;
    layout.parameters[9] = ParameterID::DECAY;
    layout.parameters[10] = ParameterID::SUSTAIN;
    layout.parameters[11] = ParameterID::RELEASE;
    layout.parameters[12] = ParameterID::LFO_RATE;
    layout.parameters[13] = ParameterID::LFO_DEPTH;
    layout.parameters[14] = ParameterID::VOLUME;
    layout.parameters[15] = ParameterID::PAN;
    
    // Engine-specific display names
    layout.displayNames[0] = osc1;
    layout.displayNames[1] = osc2;
    layout.displayNames[2] = osc3;
    layout.displayNames[3] = osc4;
    layout.displayNames[4] = "CUTOFF";
    layout.displayNames[5] = "RESO";
    layout.displayNames[6] = "TYPE";
    layout.displayNames[7] = "DETUNE";
    layout.displayNames[8] = "ATTACK";
    layout.displayNames[9] = "DECAY";
    layout.displayNames[10] = "SUSTAIN";
    layout.displayNames[11] = "RELEASE";
    layout.displayNames[12] = "LFO RT";
    layout.displayNames[13] = "LFO DEP";
    layout.displayNames[14] = "VOLUME";
    layout.displayNames[15] = "PAN";
    
    // Standard ranges
    std::pair<float, float> ranges[] = {
        {0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f},        // OSC
        {20.0f, 20000.0f}, {0.0f, 1.0f}, {0.0f, 1.0f}, {-100.0f, 100.0f}, // FILTER
        {0.001f, 10.0f}, {0.001f, 10.0f}, {0.0f, 1.0f}, {0.001f, 10.0f},   // ENV
        {0.1f, 20.0f}, {0.0f, 1.0f}, {0.0f, 1.0f}, {-1.0f, 1.0f}          // FX
    };
    for (int i = 0; i < 16; i++) {
        layout.ranges[i] = ranges[i];
    }
    
    // Standard units
    const char* units[] = {
        "%", "%", "%", "%",         // OSC
        "Hz", "%", "", "ct",        // FILTER
        "s", "s", "%", "s",         // ENV
        "Hz", "%", "%", ""          // FX
    };
    for (int i = 0; i < 16; i++) {
        layout.units[i] = units[i];
    }
    
    // Standard groups
    for (int i = 0; i < 4; i++) layout.groups[i] = EngineParameterLayout::ParameterGroup::OSC;
    for (int i = 4; i < 8; i++) layout.groups[i] = EngineParameterLayout::ParameterGroup::FILTER;
    for (int i = 8; i < 12; i++) layout.groups[i] = EngineParameterLayout::ParameterGroup::ENV;
    for (int i = 12; i < 16; i++) layout.groups[i] = EngineParameterLayout::ParameterGroup::FX;
    
    return layout;
}

// MACRO WAVETABLE: Wavetable synthesis
EngineParameterLayout EngineParameterMappings::createMacroWTLayout() {
    return createBasicLayout("WAVE", "UNISON", "SPREAD", "SYNC");
}

// MACRO WAVESHAPER: Waveshaping synthesis  
EngineParameterLayout EngineParameterMappings::createMacroWSLayout() {
    return createBasicLayout("SYMMETRY", "DRIVE", "FOLD", "BIAS");
}

// MACRO CHORD: Chord synthesis
EngineParameterLayout EngineParameterMappings::createMacroChordLayout() {
    return createBasicLayout("CHORD", "SPREAD", "VOICE", "DETUNE");
}

// MACRO HARMONICS: Harmonic synthesis
EngineParameterLayout EngineParameterMappings::createMacroHarmonicsLayout() {
    return createBasicLayout("PARTIALS", "EVEN/ODD", "SKEW", "WARP");
}

// FORMANT VOCAL: Vocal synthesis
EngineParameterLayout EngineParameterMappings::createFormantVocalLayout() {
    return createBasicLayout("VOWEL", "FORMANT", "BREATH", "NOISE");
}

// NOISE PARTICLES: Granular noise
EngineParameterLayout EngineParameterMappings::createNoiseParticlesLayout() {
    return createBasicLayout("COLOR", "DENSITY", "HP", "LP");
}

// TIDES OSC: Tidal oscillator
EngineParameterLayout EngineParameterMappings::createTidesOscLayout() {
    return createBasicLayout("SLOPE", "SMOOTH", "SHAPE", "RATE");
}

// RINGS VOICE: Rings resonator
EngineParameterLayout EngineParameterMappings::createRingsVoiceLayout() {
    return createBasicLayout("EXCITER", "DECAY", "DAMP", "BRIGHT");
}

// ELEMENTS VOICE: Elements modal
EngineParameterLayout EngineParameterMappings::createElementsVoiceLayout() {
    return createBasicLayout("EXCITER", "MATERIAL", "SPACE", "BRIGHT");
}

// DRUM KIT: Drum machine
EngineParameterLayout EngineParameterMappings::createDrumKitLayout() {
    return createBasicLayout("ACCENT", "HUMANIZE", "SEED", "VARIATION");
}

// SAMPLER KIT: Sample playback
EngineParameterLayout EngineParameterMappings::createSamplerKitLayout() {
    return createBasicLayout("START", "LOOP", "PITCH", "FILTER");
}

// SAMPLER SLICER: Beat slicing
EngineParameterLayout EngineParameterMappings::createSamplerSlicerLayout() {
    return createBasicLayout("SLICE", "PITCH", "START", "FILTER");
}

// Utility function implementations
namespace EngineParameterUtils {
    
float scaleKnobToParameter(EngineType engineType, int keyIndex, float knobValue) {
    if (!isValidKeyIndex(keyIndex)) return 0.0f;
    
    auto range = EngineParameterMappings::getParameterRange(engineType, keyIndex);
    return range.first + knobValue * (range.second - range.first);
}

float scaleParameterToKnob(EngineType engineType, int keyIndex, float paramValue) {
    if (!isValidKeyIndex(keyIndex)) return 0.0f;
    
    auto range = EngineParameterMappings::getParameterRange(engineType, keyIndex);
    if (range.second <= range.first) return 0.0f;
    
    return (paramValue - range.first) / (range.second - range.first);
}

uint32_t getGroupColor(EngineParameterLayout::ParameterGroup group) {
    switch (group) {
        case EngineParameterLayout::ParameterGroup::OSC:    return 0xD1AE9E; // Coral
        case EngineParameterLayout::ParameterGroup::FILTER: return 0xA6C0BA; // Teal
        case EngineParameterLayout::ParameterGroup::ENV:    return 0xE3C8BC; // Peach
        case EngineParameterLayout::ParameterGroup::FX:     return 0xBDCFC2; // Sage
        default: return 0x8A8A8A; // Slate
    }
}

bool isValidKeyIndex(int keyIndex) {
    return keyIndex >= 0 && keyIndex < 16;
}

} // namespace EngineParameterUtils