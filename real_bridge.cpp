#include "Sources/CEtherSynth/include/EtherSynthBridge.h"
#include "src/core/Types.h"
#include "src/synthesis/SynthEngine.h"

// Real synthesis engine includes
#include "src/engines/MacroVAEngine.h"
#include "src/engines/MacroFMEngine.h"
#include "src/engines/MacroWaveshaperEngine.h"
#include "src/engines/MacroWavetableEngine.h"
#include "src/engines/MacroChordEngine.h"
#include "src/engines/MacroHarmonicsEngine.h"
#include "src/engines/FormantEngine.h"
#include "src/engines/NoiseEngine.h"
#include "src/engines/TidesOscEngine.h"
#include "src/engines/RingsVoiceEngine.h"
#include "src/engines/ElementsVoiceEngine.h"

#include <iostream>
#include <map>
#include <string>
#include <cstring>
#include <memory>

extern "C" {

// Real bridge struct with actual synthesis engines
struct RealEtherSynthInstance {
    float bpm = 120.0f;
    float masterVolume = 0.8f;
    InstrumentColor activeInstrument = InstrumentColor::CORAL;
    bool playing = false;
    bool recording = false;
    float cpuUsage = 15.0f;
    int activeVoices = 0;
    
    // Real synthesis engines - one per instrument slot
    std::array<std::unique_ptr<SynthEngine>, static_cast<size_t>(InstrumentColor::COUNT)> engines;
    
    // Engine type per instrument slot
    std::array<EngineType, static_cast<size_t>(InstrumentColor::COUNT)> engineTypes;
    
    RealEtherSynthInstance() {
        // Initialize all slots with no engines (nullptr)
        for (size_t i = 0; i < engines.size(); i++) {
            engines[i] = nullptr;
            engineTypes[i] = EngineType::MACRO_VA; // Default type, but no engine created yet
        }
        std::cout << "Real Bridge: Created EtherSynth instance with real synthesis engines" << std::endl;
    }
    
    ~RealEtherSynthInstance() {
        std::cout << "Real Bridge: Destroyed EtherSynth instance" << std::endl;
    }
    
    // Create a real synthesis engine of the specified type
    std::unique_ptr<SynthEngine> createEngine(EngineType type) {
        switch (type) {
            case EngineType::MACRO_VA:
                return std::make_unique<MacroVAEngine>();
            case EngineType::MACRO_FM:
                return std::make_unique<MacroFMEngine>();
            case EngineType::MACRO_WAVESHAPER:
                return std::make_unique<MacroWaveshaperEngine>();
            case EngineType::MACRO_WAVETABLE:
                return std::make_unique<MacroWavetableEngine>();
            case EngineType::MACRO_CHORD:
                return std::make_unique<MacroChordEngine>();
            case EngineType::MACRO_HARMONICS:
                return std::make_unique<MacroHarmonicsEngine>();
            case EngineType::FORMANT_VOCAL:
                return std::make_unique<FormantEngine>();
            case EngineType::NOISE_PARTICLES:
                return std::make_unique<NoiseEngine>();
            case EngineType::TIDES_OSC:
                return std::make_unique<TidesOscEngine>();
            case EngineType::RINGS_VOICE:
                return std::make_unique<RingsVoiceEngine>();
            case EngineType::ELEMENTS_VOICE:
                return std::make_unique<ElementsVoiceEngine>();
            
            // For sampler engines, fall back to simpler engines for now
            case EngineType::DRUM_KIT:
            case EngineType::SAMPLER_KIT:
            case EngineType::SAMPLER_SLICER:
                return std::make_unique<NoiseEngine>(); // Fallback
                
            default:
                return std::make_unique<MacroVAEngine>(); // Safe fallback
        }
    }
    
    void setEngineType(InstrumentColor instrument, EngineType type) {
        size_t index = static_cast<size_t>(instrument);
        if (index >= engines.size()) return;
        
        engineTypes[index] = type;
        engines[index] = createEngine(type);
        
        if (engines[index]) {
            engines[index]->setSampleRate(SAMPLE_RATE);
            engines[index]->setBufferSize(BUFFER_SIZE);
            std::cout << "Real Bridge: Created " << engines[index]->getName() 
                      << " engine for instrument " << index << std::endl;
        }
    }
};

// Bridge functions
void* ether_create(void) {
    return new RealEtherSynthInstance();
}

void ether_destroy(void* synth) {
    delete static_cast<RealEtherSynthInstance*>(synth);
}

int ether_initialize(void* synth) {
    if (!synth) return 0;
    auto* instance = static_cast<RealEtherSynthInstance*>(synth);
    
    // Set default engine for active instrument
    instance->setEngineType(instance->activeInstrument, EngineType::MACRO_VA);
    
    std::cout << "Real Bridge: Initialized with real synthesis engines" << std::endl;
    return 1;
}

// New function: Process audio through real engines
void ether_process_audio(void* synth, float* outputBuffer, size_t bufferSize) {
    if (!synth || !outputBuffer) return;
    
    auto* instance = static_cast<RealEtherSynthInstance*>(synth);
    
    // Clear output buffer
    for (size_t i = 0; i < bufferSize * 2; i++) { // Stereo
        outputBuffer[i] = 0.0f;
    }
    
    // Create EtherAudioBuffer for processing
    EtherAudioBuffer audioBuffer{};
    for (auto& frame : audioBuffer) {
        frame = AudioFrame{0.0f, 0.0f};
    }
    
    // Process all active engines
    for (size_t i = 0; i < instance->engines.size(); i++) {
        if (instance->engines[i]) {
            // Clear buffer for this engine
            for (auto& frame : audioBuffer) {
                frame = AudioFrame{0.0f, 0.0f};
            }
            
            // Process audio through real engine
            instance->engines[i]->processAudio(audioBuffer);
            
            // Mix into output buffer
            for (size_t sample = 0; sample < std::min(bufferSize, BUFFER_SIZE); sample++) {
                outputBuffer[sample * 2] += audioBuffer[sample].left * instance->masterVolume;
                outputBuffer[sample * 2 + 1] += audioBuffer[sample].right * instance->masterVolume;
            }
        }
    }
}

// Note events - route to active instrument's engine
void ether_note_on(void* synth, int key_index, float velocity, float aftertouch) {
    if (!synth) return;
    auto* instance = static_cast<RealEtherSynthInstance*>(synth);
    
    size_t activeIndex = static_cast<size_t>(instance->activeInstrument);
    if (activeIndex < instance->engines.size() && instance->engines[activeIndex]) {
        instance->engines[activeIndex]->noteOn(key_index, velocity, aftertouch);
        instance->activeVoices++;
        std::cout << "Real Bridge: Note ON " << key_index << " vel=" << velocity 
                  << " on " << instance->engines[activeIndex]->getName() << std::endl;
    }
}

void ether_note_off(void* synth, int key_index) {
    if (!synth) return;
    auto* instance = static_cast<RealEtherSynthInstance*>(synth);
    
    size_t activeIndex = static_cast<size_t>(instance->activeInstrument);
    if (activeIndex < instance->engines.size() && instance->engines[activeIndex]) {
        instance->engines[activeIndex]->noteOff(key_index);
        instance->activeVoices = std::max(0, instance->activeVoices - 1);
        std::cout << "Real Bridge: Note OFF " << key_index << std::endl;
    }
}

void ether_all_notes_off(void* synth) {
    if (!synth) return;
    auto* instance = static_cast<RealEtherSynthInstance*>(synth);
    
    for (auto& engine : instance->engines) {
        if (engine) {
            engine->allNotesOff();
        }
    }
    instance->activeVoices = 0;
    std::cout << "Real Bridge: All notes OFF" << std::endl;
}

// Engine management
void ether_set_instrument_engine_type(void* synth, int instrument, int engine_type) {
    if (!synth) return;
    auto* instance = static_cast<RealEtherSynthInstance*>(synth);
    
    if (instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT) &&
        engine_type >= 0 && engine_type < static_cast<int>(EngineType::COUNT)) {
        
        InstrumentColor color = static_cast<InstrumentColor>(instrument);
        EngineType type = static_cast<EngineType>(engine_type);
        
        instance->setEngineType(color, type);
        std::cout << "Real Bridge: Set instrument " << instrument 
                  << " to engine type " << engine_type << std::endl;
    }
}

int ether_get_instrument_engine_type(void* synth, int instrument) {
    if (!synth) return 0;
    auto* instance = static_cast<RealEtherSynthInstance*>(synth);
    
    if (instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT)) {
        return static_cast<int>(instance->engineTypes[instrument]);
    }
    return 0;
}

// Parameter control - route to active instrument's engine
void ether_set_instrument_parameter(void* synth, int instrument, int param_id, float value) {
    if (!synth) return;
    auto* instance = static_cast<RealEtherSynthInstance*>(synth);
    
    if (instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT)) {
        size_t index = static_cast<size_t>(instrument);
        if (instance->engines[index]) {
            ParameterID param = static_cast<ParameterID>(param_id);
            if (instance->engines[index]->hasParameter(param)) {
                instance->engines[index]->setParameter(param, value);
            }
        }
    }
}

float ether_get_instrument_parameter(void* synth, int instrument, int param_id) {
    if (!synth) return 0.0f;
    auto* instance = static_cast<RealEtherSynthInstance*>(synth);
    
    if (instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT)) {
        size_t index = static_cast<size_t>(instrument);
        if (instance->engines[index]) {
            ParameterID param = static_cast<ParameterID>(param_id);
            if (instance->engines[index]->hasParameter(param)) {
                return instance->engines[index]->getParameter(param);
            }
        }
    }
    return 0.0f;
}

// Transport and basic state (same as enhanced_bridge)
void ether_play(void* synth) {
    if (!synth) return;
    static_cast<RealEtherSynthInstance*>(synth)->playing = true;
    std::cout << "Real Bridge: Play" << std::endl;
}

void ether_stop(void* synth) {
    if (!synth) return;
    static_cast<RealEtherSynthInstance*>(synth)->playing = false;
    std::cout << "Real Bridge: Stop" << std::endl;
}

void ether_set_active_instrument(void* synth, int color_index) {
    if (!synth) return;
    auto* instance = static_cast<RealEtherSynthInstance*>(synth);
    
    if (color_index >= 0 && color_index < static_cast<int>(InstrumentColor::COUNT)) {
        instance->activeInstrument = static_cast<InstrumentColor>(color_index);
    }
}

int ether_get_active_instrument(void* synth) {
    if (!synth) return 0;
    return static_cast<int>(static_cast<RealEtherSynthInstance*>(synth)->activeInstrument);
}

int ether_get_active_voice_count(void* synth) {
    if (!synth) return 0;
    auto* instance = static_cast<RealEtherSynthInstance*>(synth);
    
    int totalVoices = 0;
    for (auto& engine : instance->engines) {
        if (engine) {
            totalVoices += engine->getActiveVoiceCount();
        }
    }
    return totalVoices;
}

float ether_get_cpu_usage(void* synth) {
    if (!synth) return 0.0f;
    auto* instance = static_cast<RealEtherSynthInstance*>(synth);
    
    float totalCPU = 0.0f;
    int engineCount = 0;
    for (auto& engine : instance->engines) {
        if (engine) {
            totalCPU += engine->getCPUUsage();
            engineCount++;
        }
    }
    
    return engineCount > 0 ? totalCPU / engineCount : 0.0f;
}

// Engine info functions (same as enhanced_bridge)
int ether_get_engine_type_count() {
    return static_cast<int>(EngineType::COUNT);
}

const char* ether_get_engine_type_name(int engine_type) {
    static const char* names[] = {
        "MacroVA", "MacroFM", "MacroWaveshaper", "MacroWavetable",
        "MacroChord", "MacroHarmonics", "FormantVocal", "NoiseParticles",
        "TidesOsc", "RingsVoice", "ElementsVoice", 
        "DrumKit", "SamplerKit", "SamplerSlicer"
    };
    
    if (engine_type >= 0 && engine_type < static_cast<int>(EngineType::COUNT)) {
        return names[engine_type];
    }
    return "Unknown";
}

// Master volume control
void ether_set_master_volume(void* synth, float volume) {
    if (!synth) return;
    static_cast<RealEtherSynthInstance*>(synth)->masterVolume = volume;
}

float ether_get_master_volume(void* synth) {
    if (!synth) return 0.8f;
    return static_cast<RealEtherSynthInstance*>(synth)->masterVolume;
}

void ether_shutdown(void* synth) {
    if (!synth) return;
    std::cout << "Real Bridge: Shutdown" << std::endl;
}

} // extern "C"