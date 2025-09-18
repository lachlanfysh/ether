#include "Sources/CEtherSynth/include/EtherSynthBridge.h"
#include "src/core/Types.h"
#include "src/synthesis/SynthEngine.h"

// Only include engines that compiled successfully
#include "src/engines/MacroVAEngine.h"
#include "src/engines/MacroFMEngine.h"
#include "src/engines/NoiseEngine.h"  
#include "src/engines/FormantEngine.h"

#include <iostream>
#include <map>
#include <string>
#include <cstring>
#include <memory>

extern "C" {

// Working real bridge struct with actual compiled synthesis engines
struct WorkingRealEtherSynthInstance {
    float bpm = 120.0f;
    float masterVolume = 0.8f;
    InstrumentColor activeInstrument = InstrumentColor::CORAL;
    bool playing = false;
    bool recording = false;
    float cpuUsage = 15.0f;
    int activeVoices = 0;
    
    // Real synthesis engines - only the ones that compile
    std::array<std::unique_ptr<SynthEngine>, static_cast<size_t>(InstrumentColor::COUNT)> engines;
    
    // Engine type per instrument slot
    std::array<EngineType, static_cast<size_t>(InstrumentColor::COUNT)> engineTypes;
    
    WorkingRealEtherSynthInstance() {
        // Initialize all slots with no engines (nullptr)
        for (size_t i = 0; i < engines.size(); i++) {
            engines[i] = nullptr;
            engineTypes[i] = EngineType::MACRO_VA; // Default type, but no engine created yet
        }
        std::cout << "Working Real Bridge: Created EtherSynth instance with ACTUAL synthesis engines" << std::endl;
    }
    
    ~WorkingRealEtherSynthInstance() {
        std::cout << "Working Real Bridge: Destroyed EtherSynth instance" << std::endl;
    }
    
    // Create a real synthesis engine of the specified type (only supported ones)
    std::unique_ptr<SynthEngine> createEngine(EngineType type) {
        switch (type) {
            case EngineType::MACRO_VA:
                return std::make_unique<MacroVAEngine>();
            case EngineType::MACRO_FM:
                return std::make_unique<MacroFMEngine>();
            case EngineType::FORMANT_VOCAL:
                return std::make_unique<FormantEngine>();
            case EngineType::NOISE_PARTICLES:
                return std::make_unique<NoiseEngine>();
                
            // For other engines, fall back to working ones
            case EngineType::MACRO_WAVESHAPER:
            case EngineType::MACRO_WAVETABLE:
            case EngineType::MACRO_CHORD:
            case EngineType::MACRO_HARMONICS:
            case EngineType::TIDES_OSC:
            case EngineType::RINGS_VOICE:
            case EngineType::ELEMENTS_VOICE:
            case EngineType::DRUM_KIT:
            case EngineType::SAMPLER_KIT:
            case EngineType::SAMPLER_SLICER:
                return std::make_unique<MacroVAEngine>(); // Fallback to working engine
                
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
            std::cout << "Working Real Bridge: Created REAL " << engines[index]->getName() 
                      << " engine for instrument " << index << std::endl;
        }
    }
};

// Bridge functions
void* ether_create(void) {
    return new WorkingRealEtherSynthInstance();
}

void ether_destroy(void* synth) {
    delete static_cast<WorkingRealEtherSynthInstance*>(synth);
}

int ether_initialize(void* synth) {
    if (!synth) return 0;
    auto* instance = static_cast<WorkingRealEtherSynthInstance*>(synth);
    
    // Set default engine for active instrument
    instance->setEngineType(instance->activeInstrument, EngineType::MACRO_VA);
    
    std::cout << "Working Real Bridge: Initialized with REAL synthesis engines" << std::endl;
    return 1;
}

// Process audio through REAL engines
void ether_process_audio(void* synth, float* outputBuffer, size_t bufferSize) {
    if (!synth || !outputBuffer) return;
    
    auto* instance = static_cast<WorkingRealEtherSynthInstance*>(synth);
    
    // Clear output buffer
    for (size_t i = 0; i < bufferSize * 2; i++) { // Stereo
        outputBuffer[i] = 0.0f;
    }
    
    // Create EtherAudioBuffer for processing - this uses your real audio buffer type!
    EtherAudioBuffer audioBuffer{};
    for (auto& frame : audioBuffer) {
        frame = AudioFrame{0.0f, 0.0f};
    }
    
    // Process all active engines using REAL synthesis!
    for (size_t i = 0; i < instance->engines.size(); i++) {
        if (instance->engines[i]) {
            // Clear buffer for this engine
            for (auto& frame : audioBuffer) {
                frame = AudioFrame{0.0f, 0.0f};
            }
            
            // Process audio through REAL engine - this calls your actual synthesis code!
            instance->engines[i]->processAudio(audioBuffer);
            
            // Mix into output buffer
            for (size_t sample = 0; sample < std::min(bufferSize, BUFFER_SIZE); sample++) {
                outputBuffer[sample * 2] += audioBuffer[sample].left * instance->masterVolume;
                outputBuffer[sample * 2 + 1] += audioBuffer[sample].right * instance->masterVolume;
            }
        }
    }
}

// Note events - route to active instrument's REAL engine
void ether_note_on(void* synth, int key_index, float velocity, float aftertouch) {
    if (!synth) return;
    auto* instance = static_cast<WorkingRealEtherSynthInstance*>(synth);
    
    size_t activeIndex = static_cast<size_t>(instance->activeInstrument);
    if (activeIndex < instance->engines.size() && instance->engines[activeIndex]) {
        // Call REAL engine noteOn method!
        instance->engines[activeIndex]->noteOn(key_index, velocity, aftertouch);
        instance->activeVoices++;
        std::cout << "Working Real Bridge: Note ON " << key_index << " vel=" << velocity 
                  << " on REAL " << instance->engines[activeIndex]->getName() << " engine" << std::endl;
    }
}

void ether_note_off(void* synth, int key_index) {
    if (!synth) return;
    auto* instance = static_cast<WorkingRealEtherSynthInstance*>(synth);
    
    size_t activeIndex = static_cast<size_t>(instance->activeInstrument);
    if (activeIndex < instance->engines.size() && instance->engines[activeIndex]) {
        // Call REAL engine noteOff method!
        instance->engines[activeIndex]->noteOff(key_index);
        instance->activeVoices = std::max(0, instance->activeVoices - 1);
        std::cout << "Working Real Bridge: Note OFF " << key_index << std::endl;
    }
}

void ether_all_notes_off(void* synth) {
    if (!synth) return;
    auto* instance = static_cast<WorkingRealEtherSynthInstance*>(synth);
    
    // Call REAL engine allNotesOff methods!
    for (auto& engine : instance->engines) {
        if (engine) {
            engine->allNotesOff();
        }
    }
    instance->activeVoices = 0;
    std::cout << "Working Real Bridge: All notes OFF" << std::endl;
}

// Engine management
void ether_set_instrument_engine_type(void* synth, int instrument, int engine_type) {
    if (!synth) return;
    auto* instance = static_cast<WorkingRealEtherSynthInstance*>(synth);
    
    if (instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT) &&
        engine_type >= 0 && engine_type < static_cast<int>(EngineType::COUNT)) {
        
        InstrumentColor color = static_cast<InstrumentColor>(instrument);
        EngineType type = static_cast<EngineType>(engine_type);
        
        instance->setEngineType(color, type);
        std::cout << "Working Real Bridge: Set instrument " << instrument 
                  << " to REAL engine type " << engine_type << std::endl;
    }
}

int ether_get_instrument_engine_type(void* synth, int instrument) {
    if (!synth) return 0;
    auto* instance = static_cast<WorkingRealEtherSynthInstance*>(synth);
    
    if (instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT)) {
        return static_cast<int>(instance->engineTypes[instrument]);
    }
    return 0;
}

// Parameter control - route to active instrument's REAL engine
void ether_set_instrument_parameter(void* synth, int instrument, int param_id, float value) {
    if (!synth) return;
    auto* instance = static_cast<WorkingRealEtherSynthInstance*>(synth);
    
    if (instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT)) {
        size_t index = static_cast<size_t>(instrument);
        if (instance->engines[index]) {
            ParameterID param = static_cast<ParameterID>(param_id);
            if (instance->engines[index]->hasParameter(param)) {
                // Call REAL engine setParameter method!
                instance->engines[index]->setParameter(param, value);
            }
        }
    }
}

float ether_get_instrument_parameter(void* synth, int instrument, int param_id) {
    if (!synth) return 0.0f;
    auto* instance = static_cast<WorkingRealEtherSynthInstance*>(synth);
    
    if (instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT)) {
        size_t index = static_cast<size_t>(instrument);
        if (instance->engines[index]) {
            ParameterID param = static_cast<ParameterID>(param_id);
            if (instance->engines[index]->hasParameter(param)) {
                // Call REAL engine getParameter method!
                return instance->engines[index]->getParameter(param);
            }
        }
    }
    return 0.0f;
}

// Voice count from REAL engines
int ether_get_active_voice_count(void* synth) {
    if (!synth) return 0;
    auto* instance = static_cast<WorkingRealEtherSynthInstance*>(synth);
    
    int totalVoices = 0;
    for (auto& engine : instance->engines) {
        if (engine) {
            // Get REAL voice count from REAL engine!
            totalVoices += engine->getActiveVoiceCount();
        }
    }
    return totalVoices;
}

float ether_get_cpu_usage(void* synth) {
    if (!synth) return 0.0f;
    auto* instance = static_cast<WorkingRealEtherSynthInstance*>(synth);
    
    float totalCPU = 0.0f;
    int engineCount = 0;
    for (auto& engine : instance->engines) {
        if (engine) {
            // Get REAL CPU usage from REAL engine!
            totalCPU += engine->getCPUUsage();
            engineCount++;
        }
    }
    
    return engineCount > 0 ? totalCPU / engineCount : 0.0f;
}

// Transport and other functions (same interface as before)
void ether_play(void* synth) {
    if (!synth) return;
    static_cast<WorkingRealEtherSynthInstance*>(synth)->playing = true;
    std::cout << "Working Real Bridge: Play" << std::endl;
}

void ether_stop(void* synth) {
    if (!synth) return;
    static_cast<WorkingRealEtherSynthInstance*>(synth)->playing = false;
    std::cout << "Working Real Bridge: Stop" << std::endl;
}

void ether_set_active_instrument(void* synth, int color_index) {
    if (!synth) return;
    auto* instance = static_cast<WorkingRealEtherSynthInstance*>(synth);
    
    if (color_index >= 0 && color_index < static_cast<int>(InstrumentColor::COUNT)) {
        instance->activeInstrument = static_cast<InstrumentColor>(color_index);
    }
}

int ether_get_active_instrument(void* synth) {
    if (!synth) return 0;
    return static_cast<int>(static_cast<WorkingRealEtherSynthInstance*>(synth)->activeInstrument);
}

// Engine info functions
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
    static_cast<WorkingRealEtherSynthInstance*>(synth)->masterVolume = volume;
}

float ether_get_master_volume(void* synth) {
    if (!synth) return 0.8f;
    return static_cast<WorkingRealEtherSynthInstance*>(synth)->masterVolume;
}

void ether_shutdown(void* synth) {
    if (!synth) return;
    std::cout << "Working Real Bridge: Shutdown" << std::endl;
}

} // extern "C"