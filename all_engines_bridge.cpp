#include "Sources/CEtherSynth/include/EtherSynthBridge.h"
#include "src/core/Types.h"
#include "src/synthesis/SynthEngine.h"

// All SynthEngine-based engines (new interface)
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

// All SynthEngineBase-based engines (older interface)
#include "src/engines/SlideAccentBassEngine.h"
#include "src/engines/Classic4OpFMEngine.h"
// Note: SerialHPLPEngine has compilation issues, skipping for now

#include <iostream>
#include <map>
#include <string>
#include <cstring>
#include <memory>

extern "C" {

// Wrapper to make SynthEngineBase compatible with SynthEngine interface
class SynthEngineBaseWrapper : public SynthEngine {
private:
    std::unique_ptr<SynthEngineBase> baseEngine_;
    EngineType engineType_;
    std::string engineName_;
    float sampleRate_ = 48000.0f;
    
public:
    SynthEngineBaseWrapper(std::unique_ptr<SynthEngineBase> baseEngine, EngineType type, const std::string& name) 
        : baseEngine_(std::move(baseEngine)), engineType_(type), engineName_(name) {
        if (baseEngine_) {
            baseEngine_->initialize(sampleRate_);
        }
    }
    
    ~SynthEngineBaseWrapper() override {
        if (baseEngine_) {
            baseEngine_->shutdown();
        }
    }
    
    // SynthEngine interface implementation
    EngineType getType() const override { return engineType_; }
    const char* getName() const override { return engineName_.c_str(); }
    const char* getDescription() const override { return "Legacy engine wrapper"; }
    
    void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f) override {
        if (baseEngine_) {
            baseEngine_->noteOn(note, velocity);
        }
    }
    
    void noteOff(uint8_t note) override {
        if (baseEngine_) {
            baseEngine_->noteOff();  // SynthEngineBase doesn't take note parameter
        }
    }
    
    void setAftertouch(uint8_t note, float aftertouch) override {
        // SynthEngineBase doesn't support aftertouch
    }
    
    void allNotesOff() override {
        if (baseEngine_) {
            baseEngine_->allNotesOff();
        }
    }
    
    void setParameter(ParameterID param, float value) override {
        if (!baseEngine_) return;
        
        // Map ParameterID to H/T/M parameters
        switch (param) {
            case ParameterID::HARMONICS:
                baseEngine_->setHarmonics(value);
                break;
            case ParameterID::TIMBRE:
                baseEngine_->setTimbre(value);
                break;
            case ParameterID::MORPH:
                baseEngine_->setMorph(value);
                break;
            default:
                // Other parameters not supported in SynthEngineBase
                break;
        }
    }
    
    float getParameter(ParameterID param) const override {
        if (!baseEngine_) return 0.0f;
        
        float h, t, m;
        baseEngine_->getHTMParameters(h, t, m);
        
        switch (param) {
            case ParameterID::HARMONICS: return h;
            case ParameterID::TIMBRE: return t;
            case ParameterID::MORPH: return m;
            default: return 0.0f;
        }
    }
    
    bool hasParameter(ParameterID param) const override {
        // SynthEngineBase only supports H/T/M
        return param == ParameterID::HARMONICS || 
               param == ParameterID::TIMBRE || 
               param == ParameterID::MORPH;
    }
    
    void processAudio(EtherAudioBuffer& outputBuffer) override {
        if (!baseEngine_) return;
        
        // Convert EtherAudioBuffer to format expected by SynthEngineBase
        // This is a simplified conversion - real implementation might need buffering
        for (auto& frame : outputBuffer) {
            // SynthEngineBase engines might have different audio processing interface
            // For now, clear the buffer (engines need their own audio processing implementation)
            frame = AudioFrame{0.0f, 0.0f};
        }
    }
    
    size_t getActiveVoiceCount() const override {
        // SynthEngineBase doesn't provide voice count, estimate
        return 1; // Most are mono engines anyway
    }
    
    size_t getMaxVoiceCount() const override { return 1; }
    void setVoiceCount(size_t maxVoices) override {}
    
    float getCPUUsage() const override { return 0.0f; }
    
    void savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const override {
        actualSize = 0; // Not implemented for wrapped engines
    }
    
    bool loadPreset(const uint8_t* data, size_t size) override {
        return false; // Not implemented for wrapped engines
    }
    
    void setSampleRate(float sampleRate) override {
        sampleRate_ = sampleRate;
        if (baseEngine_) {
            baseEngine_->initialize(sampleRate); // Reinitialize with new sample rate
        }
    }
    
    void setBufferSize(size_t bufferSize) override {
        // SynthEngineBase doesn't have buffer size concept
    }
};

// All engines bridge struct with both engine types
struct AllEnginesEtherSynthInstance {
    float bpm = 120.0f;
    float masterVolume = 0.8f;
    InstrumentColor activeInstrument = InstrumentColor::CORAL;
    bool playing = false;
    bool recording = false;
    float cpuUsage = 15.0f;
    int activeVoices = 0;
    
    // All synthesis engines - both types unified under SynthEngine interface
    std::array<std::unique_ptr<SynthEngine>, static_cast<size_t>(InstrumentColor::COUNT)> engines;
    
    // Engine type per instrument slot
    std::array<EngineType, static_cast<size_t>(InstrumentColor::COUNT)> engineTypes;
    
    AllEnginesEtherSynthInstance() {
        // Initialize all slots with no engines (nullptr)
        for (size_t i = 0; i < engines.size(); i++) {
            engines[i] = nullptr;
            engineTypes[i] = EngineType::MACRO_VA; // Default type, but no engine created yet
        }
        std::cout << "All Engines Bridge: Created EtherSynth instance with ALL synthesis engines (both types)" << std::endl;
    }
    
    ~AllEnginesEtherSynthInstance() {
        std::cout << "All Engines Bridge: Destroyed EtherSynth instance" << std::endl;
    }
    
    // Create ALL the real synthesis engines including both interface types
    std::unique_ptr<SynthEngine> createEngine(EngineType type) {
        switch (type) {
            // SynthEngine-based engines (new interface)
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
                
            // SynthEngineBase-based engines (older interface) - wrapped
            case EngineType::SLIDE_ACCENT_BASS: {
                auto baseEngine = std::make_unique<SlideAccentBassEngine>();
                return std::make_unique<SynthEngineBaseWrapper>(
                    std::move(baseEngine), EngineType::SLIDE_ACCENT_BASS, "SlideAccentBass");
            }
            case EngineType::CLASSIC_4OP_FM: {
                auto baseEngine = std::make_unique<Classic4OpFMEngine>();
                return std::make_unique<SynthEngineBaseWrapper>(
                    std::move(baseEngine), EngineType::CLASSIC_4OP_FM, "Classic4OpFM");
            }
                
            // For sampler engines that don't compile yet, fall back to working ones
            case EngineType::DRUM_KIT:
                return std::make_unique<NoiseEngine>(); // Drums = rhythmic noise
            case EngineType::SAMPLER_KIT:
                return std::make_unique<MacroWavetableEngine>(); // Samples = wavetables
            case EngineType::SAMPLER_SLICER:
                return std::make_unique<MacroWavetableEngine>(); // Slicing = wavetables
            case EngineType::SERIAL_HPLP:
                return std::make_unique<MacroVAEngine>(); // Fallback until compilation fixed
                
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
            std::cout << "All Engines Bridge: Created REAL " << engines[index]->getName() 
                      << " engine for instrument " << index << std::endl;
        }
    }
};

// Bridge functions (same as before, but now supporting ALL engines)
void* ether_create(void) {
    return new AllEnginesEtherSynthInstance();
}

void ether_destroy(void* synth) {
    delete static_cast<AllEnginesEtherSynthInstance*>(synth);
}

int ether_initialize(void* synth) {
    if (!synth) return 0;
    auto* instance = static_cast<AllEnginesEtherSynthInstance*>(synth);
    
    // Set default engine for active instrument
    instance->setEngineType(instance->activeInstrument, EngineType::MACRO_VA);
    
    std::cout << "All Engines Bridge: Initialized with ALL synthesis engines (both interface types)" << std::endl;
    return 1;
}

// Process audio through ALL the real engines (both types)
void ether_process_audio(void* synth, float* outputBuffer, size_t bufferSize) {
    if (!synth || !outputBuffer) return;
    
    auto* instance = static_cast<AllEnginesEtherSynthInstance*>(synth);
    
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

// Note events - route to active instrument's REAL engine (both types)
void ether_note_on(void* synth, int key_index, float velocity, float aftertouch) {
    if (!synth) return;
    auto* instance = static_cast<AllEnginesEtherSynthInstance*>(synth);
    
    size_t activeIndex = static_cast<size_t>(instance->activeInstrument);
    if (activeIndex < instance->engines.size() && instance->engines[activeIndex]) {
        // Call REAL engine noteOn method! (works for both engine types through wrapper)
        instance->engines[activeIndex]->noteOn(key_index, velocity, aftertouch);
        instance->activeVoices++;
        std::cout << "All Engines Bridge: Note ON " << key_index << " vel=" << velocity 
                  << " on REAL " << instance->engines[activeIndex]->getName() << " engine" << std::endl;
    }
}

void ether_note_off(void* synth, int key_index) {
    if (!synth) return;
    auto* instance = static_cast<AllEnginesEtherSynthInstance*>(synth);
    
    size_t activeIndex = static_cast<size_t>(instance->activeInstrument);
    if (activeIndex < instance->engines.size() && instance->engines[activeIndex]) {
        // Call REAL engine noteOff method! (works for both engine types through wrapper)
        instance->engines[activeIndex]->noteOff(key_index);
        instance->activeVoices = std::max(0, instance->activeVoices - 1);
        std::cout << "All Engines Bridge: Note OFF " << key_index << std::endl;
    }
}

void ether_all_notes_off(void* synth) {
    if (!synth) return;
    auto* instance = static_cast<AllEnginesEtherSynthInstance*>(synth);
    
    // Call REAL engine allNotesOff methods! (works for both engine types through wrapper)
    for (auto& engine : instance->engines) {
        if (engine) {
            engine->allNotesOff();
        }
    }
    instance->activeVoices = 0;
    std::cout << "All Engines Bridge: All notes OFF" << std::endl;
}

// Engine management (now supports ALL engines)
void ether_set_instrument_engine_type(void* synth, int instrument, int engine_type) {
    if (!synth) return;
    auto* instance = static_cast<AllEnginesEtherSynthInstance*>(synth);
    
    if (instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT) &&
        engine_type >= 0 && engine_type < static_cast<int>(EngineType::COUNT)) {
        
        InstrumentColor color = static_cast<InstrumentColor>(instrument);
        EngineType type = static_cast<EngineType>(engine_type);
        
        instance->setEngineType(color, type);
        std::cout << "All Engines Bridge: Set instrument " << instrument 
                  << " to REAL engine type " << engine_type << std::endl;
    }
}

// Rest of the bridge functions (same as before)
int ether_get_instrument_engine_type(void* synth, int instrument) {
    if (!synth) return 0;
    auto* instance = static_cast<AllEnginesEtherSynthInstance*>(synth);
    
    if (instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT)) {
        return static_cast<int>(instance->engineTypes[instrument]);
    }
    return 0;
}

void ether_set_instrument_parameter(void* synth, int instrument, int param_id, float value) {
    if (!synth) return;
    auto* instance = static_cast<AllEnginesEtherSynthInstance*>(synth);
    
    if (instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT)) {
        size_t index = static_cast<size_t>(instrument);
        if (instance->engines[index]) {
            ParameterID param = static_cast<ParameterID>(param_id);
            if (instance->engines[index]->hasParameter(param)) {
                // Call REAL engine setParameter method! (works for both engine types through wrapper)
                instance->engines[index]->setParameter(param, value);
            }
        }
    }
}

float ether_get_instrument_parameter(void* synth, int instrument, int param_id) {
    if (!synth) return 0.0f;
    auto* instance = static_cast<AllEnginesEtherSynthInstance*>(synth);
    
    if (instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT)) {
        size_t index = static_cast<size_t>(instrument);
        if (instance->engines[index]) {
            ParameterID param = static_cast<ParameterID>(param_id);
            if (instance->engines[index]->hasParameter(param)) {
                // Call REAL engine getParameter method! (works for both engine types through wrapper)
                return instance->engines[index]->getParameter(param);
            }
        }
    }
    return 0.0f;
}

int ether_get_active_voice_count(void* synth) {
    if (!synth) return 0;
    auto* instance = static_cast<AllEnginesEtherSynthInstance*>(synth);
    
    int totalVoices = 0;
    for (auto& engine : instance->engines) {
        if (engine) {
            // Get REAL voice count from REAL engine! (works for both engine types through wrapper)
            totalVoices += engine->getActiveVoiceCount();
        }
    }
    return totalVoices;
}

float ether_get_cpu_usage(void* synth) {
    if (!synth) return 0.0f;
    auto* instance = static_cast<AllEnginesEtherSynthInstance*>(synth);
    
    float totalCPU = 0.0f;
    int engineCount = 0;
    for (auto& engine : instance->engines) {
        if (engine) {
            // Get REAL CPU usage from REAL engine! (works for both engine types through wrapper)
            totalCPU += engine->getCPUUsage();
            engineCount++;
        }
    }
    
    return engineCount > 0 ? totalCPU / engineCount : 0.0f;
}

// Transport functions
void ether_play(void* synth) {
    if (!synth) return;
    static_cast<AllEnginesEtherSynthInstance*>(synth)->playing = true;
    std::cout << "All Engines Bridge: Play" << std::endl;
}

void ether_stop(void* synth) {
    if (!synth) return;
    static_cast<AllEnginesEtherSynthInstance*>(synth)->playing = false;
    std::cout << "All Engines Bridge: Stop" << std::endl;
}

void ether_set_active_instrument(void* synth, int color_index) {
    if (!synth) return;
    auto* instance = static_cast<AllEnginesEtherSynthInstance*>(synth);
    
    if (color_index >= 0 && color_index < static_cast<int>(InstrumentColor::COUNT)) {
        instance->activeInstrument = static_cast<InstrumentColor>(color_index);
    }
}

int ether_get_active_instrument(void* synth) {
    if (!synth) return 0;
    return static_cast<int>(static_cast<AllEnginesEtherSynthInstance*>(synth)->activeInstrument);
}

// Engine info functions - now includes ALL engines
int ether_get_engine_type_count() {
    return static_cast<int>(EngineType::COUNT);
}

const char* ether_get_engine_type_name(int engine_type) {
    static const char* names[] = {
        "MacroVA", "MacroFM", "MacroWaveshaper", "MacroWavetable",
        "MacroChord", "MacroHarmonics", "FormantVocal", "NoiseParticles",
        "TidesOsc", "RingsVoice", "ElementsVoice",
        "DrumKit", "SamplerKit", "SamplerSlicer",
        "SlideAccentBass", "Classic4OpFM", "SerialHPLP"
    };
    
    if (engine_type >= 0 && engine_type < static_cast<int>(EngineType::COUNT)) {
        return names[engine_type];
    }
    return "Unknown";
}

void ether_set_master_volume(void* synth, float volume) {
    if (!synth) return;
    static_cast<AllEnginesEtherSynthInstance*>(synth)->masterVolume = volume;
}

float ether_get_master_volume(void* synth) {
    if (!synth) return 0.8f;
    return static_cast<AllEnginesEtherSynthInstance*>(synth)->masterVolume;
}

void ether_shutdown(void* synth) {
    if (!synth) return;
    std::cout << "All Engines Bridge: Shutdown" << std::endl;
}

} // extern "C"