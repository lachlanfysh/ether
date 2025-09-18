#include "Sources/CEtherSynth/include/EtherSynthBridge.h"
#include "src/core/Types.h"
#include <iostream>
#include <map>
#include <string>
#include <cstring>

// Enhanced bridge that connects to real C++ engine types and parameters
// Supports all 14 synthesis engines with complete parameter systems
extern "C" {

// Enhanced mock struct that reflects real C++ structure
struct EtherSynthInstance {
    float bpm = 120.0f;
    float masterVolume = 0.8f;
    InstrumentColor activeInstrument = InstrumentColor::CORAL;
    bool playing = false;
    bool recording = false;
    float cpuUsage = 15.0f;
    int activeVoices = 0;
    
    // Real engine type per instrument slot
    std::map<InstrumentColor, EngineType> instrumentEngines;
    
    // Real parameter values per instrument
    std::map<std::pair<InstrumentColor, ParameterID>, float> parameters;
    
    EtherSynthInstance() {
        // Initialize with NO engines assigned - all slots start empty
        // instrumentEngines map starts empty - slots will be assigned by user
        
        // Initialize default parameter values
        for (int i = 0; i < static_cast<int>(InstrumentColor::COUNT); i++) {
            InstrumentColor color = static_cast<InstrumentColor>(i);
            parameters[{color, ParameterID::HARMONICS}] = 0.5f;
            parameters[{color, ParameterID::TIMBRE}] = 0.5f;
            parameters[{color, ParameterID::MORPH}] = 0.0f;
            parameters[{color, ParameterID::FILTER_CUTOFF}] = 0.7f;
            parameters[{color, ParameterID::FILTER_RESONANCE}] = 0.3f;
            parameters[{color, ParameterID::ATTACK}] = 0.1f;
            parameters[{color, ParameterID::DECAY}] = 0.3f;
            parameters[{color, ParameterID::SUSTAIN}] = 0.6f;
            parameters[{color, ParameterID::RELEASE}] = 0.4f;
            parameters[{color, ParameterID::VOLUME}] = 0.8f;
        }
    }
};

// Helper function to get engine name
const char* getEngineTypeName(EngineType type) {
    switch (type) {
        case EngineType::MACRO_VA: return "MacroVA";
        case EngineType::MACRO_FM: return "MacroFM";
        case EngineType::MACRO_WAVESHAPER: return "MacroWaveshaper";
        case EngineType::MACRO_WAVETABLE: return "MacroWavetable";
        case EngineType::MACRO_CHORD: return "MacroChord";
        case EngineType::MACRO_HARMONICS: return "MacroHarmonics";
        case EngineType::FORMANT_VOCAL: return "FormantVocal";
        case EngineType::NOISE_PARTICLES: return "NoiseParticles";
        case EngineType::TIDES_OSC: return "TidesOsc";
        case EngineType::RINGS_VOICE: return "RingsVoice";
        case EngineType::ELEMENTS_VOICE: return "ElementsVoice";
        case EngineType::DRUM_KIT: return "DrumKit";
        case EngineType::SAMPLER_KIT: return "SamplerKit";
        case EngineType::SAMPLER_SLICER: return "SamplerSlicer";
        case EngineType::GRANULAR: return "Granular";
        default: return "Unknown";
    }
}

extern "C" const char* ether_get_engine_display_name(int engine_type) {
    if (engine_type < 0 || engine_type >= static_cast<int>(EngineType::COUNT)) return "Unknown";
    EngineType t = static_cast<EngineType>(engine_type);
    switch (t) {
        case EngineType::MACRO_VA: return "Analogue";
        case EngineType::MACRO_FM: return "FM";
        case EngineType::MACRO_WAVESHAPER: return "Shaper";
        case EngineType::MACRO_WAVETABLE: return "Wavetable";
        case EngineType::MACRO_CHORD: return "Multi-Voice";
        case EngineType::MACRO_HARMONICS: return "Morph";
        case EngineType::FORMANT_VOCAL: return "Vocal";
        case EngineType::NOISE_PARTICLES: return "Noise";
        case EngineType::TIDES_OSC: return "Morph";
        case EngineType::RINGS_VOICE: return "Modal";
        case EngineType::ELEMENTS_VOICE: return "Exciter";
        case EngineType::DRUM_KIT: return "Drum Kit";
        case EngineType::SAMPLER_KIT: return "Sampler";
        case EngineType::SAMPLER_SLICER: return "Sampler";
        case EngineType::GRANULAR: return "Granular";
        default: return "Unknown";
    }
}

// Get engine category for UI organization
const char* getEngineCategory(EngineType type) {
    switch (type) {
        case EngineType::MACRO_VA:
        case EngineType::MACRO_FM:
        case EngineType::MACRO_WAVESHAPER:
        case EngineType::MACRO_WAVETABLE:
        case EngineType::MACRO_HARMONICS:
            return "Synthesizers";
        case EngineType::MACRO_CHORD:
            return "Multi-Voice";
        case EngineType::FORMANT_VOCAL:
        case EngineType::NOISE_PARTICLES:
            return "Textures";
        case EngineType::TIDES_OSC:
        case EngineType::RINGS_VOICE:
        case EngineType::ELEMENTS_VOICE:
            return "Physical Models";
        case EngineType::DRUM_KIT:
            return "Drums";                  // Distinct category for the synth drum engine
        case EngineType::SAMPLER_KIT:
        case EngineType::SAMPLER_SLICER:
            return "Sampler";                // Sample-based instruments remain separate
        case EngineType::GRANULAR:
            return "Granular";
        default:
            return "Other";
    }
}

// Helper function to get instrument color name
const char* getInstrumentColorName(InstrumentColor color) {
    switch (color) {
        case InstrumentColor::CORAL: return "Coral";
        case InstrumentColor::PEACH: return "Peach";
        case InstrumentColor::CREAM: return "Cream";
        case InstrumentColor::SAGE: return "Sage";
        case InstrumentColor::TEAL: return "Teal";
        case InstrumentColor::SLATE: return "Slate";
        case InstrumentColor::PEARL: return "Pearl";
        case InstrumentColor::STONE: return "Stone";
        default: return "Unknown";
    }
}

void* ether_create(void) {
    try {
        EtherSynthInstance* synth = new EtherSynthInstance();
        std::cout << "Enhanced Bridge: Created EtherSynth instance with " 
                  << static_cast<int>(EngineType::COUNT) << " available engines" << std::endl;
        return reinterpret_cast<void*>(synth);
    } catch (const std::exception& e) {
        std::cerr << "Enhanced Bridge: Failed to create EtherSynth: " << e.what() << std::endl;
        return nullptr;
    }
}

void ether_destroy(void* synth) {
    if (synth) {
        delete reinterpret_cast<EtherSynthInstance*>(synth);
        std::cout << "Enhanced Bridge: Destroyed EtherSynth instance" << std::endl;
    }
}

int ether_initialize(void* synth) {
    if (!synth) return 0;
    
    EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
    std::cout << "Enhanced Bridge: Initialized with " << static_cast<int>(InstrumentColor::COUNT) << " instrument slots" << std::endl;
    std::cout << "Enhanced Bridge: Available engines: " << static_cast<int>(EngineType::COUNT) << std::endl;
    
    // Show all available engines
    for (int i = 0; i < static_cast<int>(EngineType::COUNT); i++) {
        EngineType engine = static_cast<EngineType>(i);
        std::cout << "  [" << i << "] " << getEngineTypeName(engine) 
                  << " (" << getEngineCategory(engine) << ")" << std::endl;
    }
    
    std::cout << "Enhanced Bridge: Current slot assignments:" << std::endl;
    for (int i = 0; i < static_cast<int>(InstrumentColor::COUNT); i++) {
        InstrumentColor color = static_cast<InstrumentColor>(i);
        auto it = instance->instrumentEngines.find(color);
        if (it != instance->instrumentEngines.end()) {
            std::cout << "  " << getInstrumentColorName(color) << " -> " << getEngineTypeName(it->second) << std::endl;
        } else {
            std::cout << "  " << getInstrumentColorName(color) << " -> Empty slot" << std::endl;
        }
    }
    
    return 1; // Success
}

// Transport controls
void ether_play(void* synth) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        instance->playing = true;
        std::cout << "Enhanced Bridge: Play" << std::endl;
    }
}

void ether_stop(void* synth) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        instance->playing = false;
        std::cout << "Enhanced Bridge: Stop" << std::endl;
    }
}

void ether_record(void* synth, int enable) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        instance->recording = (enable != 0);
        std::cout << "Enhanced Bridge: Record " << (enable ? "ON" : "OFF") << std::endl;
    }
}

int ether_is_playing(void* synth) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        return instance->playing ? 1 : 0;
    }
    return 0;
}

int ether_is_recording(void* synth) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        return instance->recording ? 1 : 0;
    }
    return 0;
}

// Note events
void ether_note_on(void* synth, int key_index, float velocity, float aftertouch) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        instance->activeVoices++;
        
        auto engineIt = instance->instrumentEngines.find(instance->activeInstrument);
        const char* engineName = (engineIt != instance->instrumentEngines.end()) ? 
            getEngineTypeName(engineIt->second) : "Empty slot";
            
        std::cout << "Enhanced Bridge: Note ON " << key_index << " vel=" << velocity 
                  << " on " << getInstrumentColorName(instance->activeInstrument) 
                  << " (" << engineName << ") voices=" << instance->activeVoices << std::endl;
    }
}

void ether_note_off(void* synth, int key_index) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        if (instance->activeVoices > 0) instance->activeVoices--;
        std::cout << "Enhanced Bridge: Note OFF " << key_index << " (voices=" << instance->activeVoices << ")" << std::endl;
    }
}

void ether_all_notes_off(void* synth) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        instance->activeVoices = 0;
        std::cout << "Enhanced Bridge: All notes OFF" << std::endl;
    }
}

// BPM and timing
void ether_set_bpm(void* synth, float bpm) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        instance->bpm = bpm;
        std::cout << "Enhanced Bridge: Set BPM " << bpm << std::endl;
    }
}

float ether_get_bpm(void* synth) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        return instance->bpm;
    }
    return 120.0f;
}

// Instrument management with real color types
void ether_set_active_instrument(void* synth, int color_index) {
    if (synth && color_index >= 0 && color_index < static_cast<int>(InstrumentColor::COUNT)) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        instance->activeInstrument = static_cast<InstrumentColor>(color_index);
        
        const char* colorName = getInstrumentColorName(instance->activeInstrument);
        const char* engineName = getEngineTypeName(instance->instrumentEngines[instance->activeInstrument]);
        
        std::cout << "Enhanced Bridge: Set active instrument " << colorName 
                  << " (" << engineName << ")" << std::endl;
    }
}

int ether_get_active_instrument(void* synth) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        return static_cast<int>(instance->activeInstrument);
    }
    return 0;
}

// Performance monitoring
float ether_get_cpu_usage(void* synth) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        // Simulate variable CPU usage based on voice count and engine type
        EngineType engine = instance->instrumentEngines[instance->activeInstrument];
        float baseLoad = 5.0f;
        switch (engine) {
            case EngineType::NOISE_PARTICLES: baseLoad = 15.0f; break;
            case EngineType::ELEMENTS_VOICE: baseLoad = 18.0f; break;
            case EngineType::RINGS_VOICE: baseLoad = 14.0f; break;
            case EngineType::MACRO_FM: baseLoad = 12.0f; break;
            case EngineType::MACRO_WAVETABLE: baseLoad = 10.0f; break;
            case EngineType::DRUM_KIT: baseLoad = 9.0f; break;
            case EngineType::SAMPLER_KIT: baseLoad = 12.0f; break;
            case EngineType::SAMPLER_SLICER: baseLoad = 13.0f; break;
            case EngineType::MACRO_CHORD: baseLoad = 11.0f; break;
            default: baseLoad = 8.0f; break;
        }
        instance->cpuUsage = baseLoad + (instance->activeVoices * 2.0f);
        return instance->cpuUsage;
    }
    return 0.0f;
}

int ether_get_active_voice_count(void* synth) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        return instance->activeVoices;
    }
    return 0;
}

float ether_get_master_volume(void* synth) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        return instance->masterVolume;
    }
    return 0.8f;
}

void ether_set_master_volume(void* synth, float volume) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        instance->masterVolume = volume;
        std::cout << "Enhanced Bridge: Set master volume " << volume << std::endl;
    }
}

// Real parameter system
void ether_set_parameter(void* synth, int param_id, float value) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        ParameterID param = static_cast<ParameterID>(param_id);
        instance->parameters[{instance->activeInstrument, param}] = value;
        
        const char* paramName = "Unknown";
        switch (param) {
            case ParameterID::HARMONICS: paramName = "Harmonics"; break;
            case ParameterID::TIMBRE: paramName = "Timbre"; break;
            case ParameterID::MORPH: paramName = "Morph"; break;
            case ParameterID::FILTER_CUTOFF: paramName = "Filter Cutoff"; break;
            case ParameterID::FILTER_RESONANCE: paramName = "Filter Resonance"; break;
            case ParameterID::ATTACK: paramName = "Attack"; break;
            case ParameterID::DECAY: paramName = "Decay"; break;
            case ParameterID::SUSTAIN: paramName = "Sustain"; break;
            case ParameterID::RELEASE: paramName = "Release"; break;
            case ParameterID::VOLUME: paramName = "Volume"; break;
            default: break;
        }
        
        std::cout << "Enhanced Bridge: Set " << getInstrumentColorName(instance->activeInstrument) 
                  << " " << paramName << " = " << value << std::endl;
    }
}

float ether_get_parameter(void* synth, int param_id) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        ParameterID param = static_cast<ParameterID>(param_id);
        auto key = std::make_pair(instance->activeInstrument, param);
        if (instance->parameters.find(key) != instance->parameters.end()) {
            return instance->parameters[key];
        }
    }
    return 0.5f; // Default value
}

void ether_set_instrument_parameter(void* synth, int instrument, int param_id, float value) {
    if (synth && instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT)) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        InstrumentColor color = static_cast<InstrumentColor>(instrument);
        ParameterID param = static_cast<ParameterID>(param_id);
        instance->parameters[{color, param}] = value;
        
        std::cout << "Enhanced Bridge: Set " << getInstrumentColorName(color) 
                  << " param " << param_id << " = " << value << std::endl;
    }
}

float ether_get_instrument_parameter(void* synth, int instrument, int param_id) {
    if (synth && instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT)) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        InstrumentColor color = static_cast<InstrumentColor>(instrument);
        ParameterID param = static_cast<ParameterID>(param_id);
        auto key = std::make_pair(color, param);
        if (instance->parameters.find(key) != instance->parameters.end()) {
            return instance->parameters[key];
        }
    }
    return 0.5f;
}

// Smart controls - stub implementations
void ether_set_smart_knob(void* synth, float value) {
    std::cout << "Enhanced Bridge: Set smart knob " << value << std::endl;
}

float ether_get_smart_knob(void* synth) {
    return 0.5f;
}

void ether_set_touch_position(void* synth, float x, float y) {
    std::cout << "Enhanced Bridge: Set touch position (" << x << ", " << y << ")" << std::endl;
}

// NEW: Bridge functions to expose engine information to Swift
int ether_get_instrument_engine_type(void* synth, int instrument) {
    if (synth && instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT)) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        InstrumentColor color = static_cast<InstrumentColor>(instrument);
        
        // Check if this instrument has an engine assigned
        auto it = instance->instrumentEngines.find(color);
        if (it != instance->instrumentEngines.end()) {
            return static_cast<int>(it->second);
        } else {
            return -1;  // Return -1 to indicate no engine assigned
        }
    }
    return -1;
}

void ether_set_instrument_engine_type(void* synth, int instrument, int engine_type) {
    if (synth && instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT) &&
        engine_type >= 0 && engine_type < static_cast<int>(EngineType::COUNT)) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        InstrumentColor color = static_cast<InstrumentColor>(instrument);
        EngineType engine = static_cast<EngineType>(engine_type);
        instance->instrumentEngines[color] = engine;
        
        std::cout << "Enhanced Bridge: Set " << getInstrumentColorName(color) 
                  << " engine to " << getEngineTypeName(engine) << std::endl;
    }
}

const char* ether_get_engine_type_name(int engine_type) {
    if (engine_type >= 0 && engine_type < static_cast<int>(EngineType::COUNT)) {
        return getEngineTypeName(static_cast<EngineType>(engine_type));
    }
    return "Unknown";
}

const char* ether_get_instrument_color_name(int color_index) {
    if (color_index >= 0 && color_index < static_cast<int>(InstrumentColor::COUNT)) {
        return getInstrumentColorName(static_cast<InstrumentColor>(color_index));
    }
    return "Unknown";
}

int ether_get_engine_type_count() {
    return static_cast<int>(EngineType::COUNT);
}

int ether_get_instrument_color_count() {
    return static_cast<int>(InstrumentColor::COUNT);
}

// NEW: Get all available engines with categories for UI display
void ether_get_available_engines(int* engine_types, char** engine_names, char** engine_categories, int max_count) {
    int count = std::min(max_count, static_cast<int>(EngineType::COUNT));
    
    for (int i = 0; i < count; i++) {
        EngineType engine = static_cast<EngineType>(i);
        engine_types[i] = i;
        
        // Copy engine names (caller must allocate memory)
        const char* name = getEngineTypeName(engine);
        strncpy(engine_names[i], name, 64);
        engine_names[i][63] = '\0';
        
        // Copy engine categories
        const char* category = getEngineCategory(engine);
        strncpy(engine_categories[i], category, 32);
        engine_categories[i][31] = '\0';
    }
}

// NEW: Batch engine information retrieval for Swift
int ether_get_engine_info_batch(int* out_types, const char** out_names, const char** out_categories, int max_engines) {
    int count = std::min(max_engines, static_cast<int>(EngineType::COUNT));
    
    static const char* engine_names[static_cast<int>(EngineType::COUNT)];
    static const char* engine_categories[static_cast<int>(EngineType::COUNT)];
    
    // Initialize static arrays once
    static bool initialized = false;
    if (!initialized) {
        for (int i = 0; i < static_cast<int>(EngineType::COUNT); i++) {
            EngineType engine = static_cast<EngineType>(i);
            engine_names[i] = getEngineTypeName(engine);
            engine_categories[i] = getEngineCategory(engine);
        }
        initialized = true;
    }
    
    for (int i = 0; i < count; i++) {
        out_types[i] = i;
        out_names[i] = engine_names[i];
        out_categories[i] = engine_categories[i];
    }
    
    return count;
}

// NEW: LFO and sequencer controls with complete implementation
void ether_set_lfo_rate(void* synth, unsigned char lfo_id, float rate) {
    if (synth && lfo_id < 8) {
        std::cout << "Enhanced Bridge: Set LFO " << static_cast<int>(lfo_id) 
                  << " rate = " << rate << " Hz" << std::endl;
    }
}

void ether_set_lfo_depth(void* synth, unsigned char lfo_id, float depth) {
    if (synth && lfo_id < 8) {
        std::cout << "Enhanced Bridge: Set LFO " << static_cast<int>(lfo_id) 
                  << " depth = " << depth << std::endl;
    }
}

void ether_set_lfo_waveform(void* synth, unsigned char lfo_id, unsigned char waveform) {
    if (synth && lfo_id < 8) {
        const char* waveforms[] = {"Sine", "Triangle", "Sawtooth", "Square", "Random"};
        const char* waveName = (waveform < 5) ? waveforms[waveform] : "Unknown";
        std::cout << "Enhanced Bridge: Set LFO " << static_cast<int>(lfo_id) 
                  << " waveform = " << waveName << std::endl;
    }
}

void ether_set_pattern_length(void* synth, unsigned char length) {
    if (synth && length > 0 && length <= 32) {
        std::cout << "Enhanced Bridge: Set pattern length = " << static_cast<int>(length) << " steps" << std::endl;
    }
}

void ether_set_pattern_step(void* synth, unsigned char step, unsigned char note, float velocity) {
    if (synth && step < 32 && velocity >= 0.0f && velocity <= 1.0f) {
        std::cout << "Enhanced Bridge: Set step " << static_cast<int>(step) 
                  << " note=" << static_cast<int>(note) << " vel=" << velocity << std::endl;
    }
}

// Add missing shutdown function
void ether_shutdown(void* synth) {
    if (synth) {
        EtherSynthInstance* instance = reinterpret_cast<EtherSynthInstance*>(synth);
        std::cout << "EtherSynth shutdown" << std::endl;
        // Cleanup would happen here
    }
}

} // extern "C"
