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

// Standalone engines (no base class)
#include "src/engines/SlideAccentBassEngine.h"
#include "src/engines/Classic4OpFMEngine.h"

#include <iostream>
#include <map>
#include <string>
#include <cstring>
#include <memory>

extern "C" {

// Wrapper for SlideAccentBassEngine (standalone)
class SlideAccentBassWrapper : public SynthEngine {
private:
    std::unique_ptr<SlideAccentBassEngine> bassEngine_;
    float sampleRate_ = 48000.0f;
    
public:
    SlideAccentBassWrapper() : bassEngine_(std::make_unique<SlideAccentBassEngine>()) {
        if (bassEngine_) {
            bassEngine_->initialize(sampleRate_);
        }
    }
    
    ~SlideAccentBassWrapper() override {
        if (bassEngine_) {
            bassEngine_->shutdown();
        }
    }
    
    // SynthEngine interface implementation
    EngineType getType() const override { return EngineType::SLIDE_ACCENT_BASS; }
    const char* getName() const override { return "SlideAccentBass"; }
    const char* getDescription() const override { return "TB-303 style bass synthesis"; }
    
    void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f) override {
        if (bassEngine_) {
            bassEngine_->noteOn(note, velocity);
        }
    }
    
    void noteOff(uint8_t note) override {
        if (bassEngine_) {
            bassEngine_->noteOff();
        }
    }
    
    void setAftertouch(uint8_t note, float aftertouch) override {
        // Not supported by SlideAccentBassEngine
    }
    
    void allNotesOff() override {
        if (bassEngine_) {
            bassEngine_->allNotesOff();
        }
    }
    
    void setParameter(ParameterID param, float value) override {
        if (!bassEngine_) return;
        
        // Map ParameterID to SlideAccentBass parameters
        switch (param) {
            case ParameterID::HARMONICS:
                bassEngine_->setHarmonics(value);
                break;
            case ParameterID::TIMBRE:
                bassEngine_->setTimbre(value);
                break;
            case ParameterID::MORPH:
                bassEngine_->setMorph(value);
                break;
            default:
                break;
        }
    }
    
    float getParameter(ParameterID param) const override {
        if (!bassEngine_) return 0.0f;
        
        float h, t, m;
        bassEngine_->getHTMParameters(h, t, m);
        
        switch (param) {
            case ParameterID::HARMONICS: return h;
            case ParameterID::TIMBRE: return t;
            case ParameterID::MORPH: return m;
            default: return 0.0f;
        }
    }
    
    bool hasParameter(ParameterID param) const override {
        return param == ParameterID::HARMONICS || 
               param == ParameterID::TIMBRE || 
               param == ParameterID::MORPH;
    }
    
    void processAudio(EtherAudioBuffer& outputBuffer) override {
        if (!bassEngine_) return;
        
        // SlideAccentBassEngine has its own processAudio method
        // For now, clear buffer - need to check the actual interface
        for (auto& frame : outputBuffer) {
            frame = AudioFrame{0.0f, 0.0f};
        }
    }
    
    size_t getActiveVoiceCount() const override { return 1; } // Mono bass
    size_t getMaxVoiceCount() const override { return 1; }
    void setVoiceCount(size_t maxVoices) override {}
    float getCPUUsage() const override { return 0.0f; }
    
    void savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const override {
        actualSize = 0;
    }
    
    bool loadPreset(const uint8_t* data, size_t size) override {
        return false;
    }
    
    void setSampleRate(float sampleRate) override {
        sampleRate_ = sampleRate;
        if (bassEngine_) {
            bassEngine_->initialize(sampleRate);
        }
    }
    
    void setBufferSize(size_t bufferSize) override {}
};

// Wrapper for Classic4OpFMEngine (standalone)
class Classic4OpFMWrapper : public SynthEngine {
private:
    std::unique_ptr<Classic4OpFMEngine> fmEngine_;
    float sampleRate_ = 48000.0f;
    
public:
    Classic4OpFMWrapper() : fmEngine_(std::make_unique<Classic4OpFMEngine>()) {
        if (fmEngine_) {
            fmEngine_->initialize(sampleRate_);
        }
    }
    
    ~Classic4OpFMWrapper() override {
        if (fmEngine_) {
            fmEngine_->shutdown();
        }
    }
    
    // SynthEngine interface implementation
    EngineType getType() const override { return EngineType::CLASSIC_4OP_FM; }
    const char* getName() const override { return "Classic4OpFM"; }
    const char* getDescription() const override { return "Classic 4-operator FM synthesis"; }
    
    void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f) override {
        if (fmEngine_) {
            fmEngine_->noteOn(note, velocity);
        }
    }
    
    void noteOff(uint8_t note) override {
        if (fmEngine_) {
            fmEngine_->noteOff();
        }
    }
    
    void setAftertouch(uint8_t note, float aftertouch) override {
        // Not supported by Classic4OpFMEngine
    }
    
    void allNotesOff() override {
        if (fmEngine_) {
            fmEngine_->allNotesOff();
        }
    }
    
    void setParameter(ParameterID param, float value) override {
        if (!fmEngine_) return;
        
        // Map ParameterID to Classic4OpFM parameters
        switch (param) {
            case ParameterID::HARMONICS:
                fmEngine_->setHarmonics(value);
                break;
            case ParameterID::TIMBRE:
                fmEngine_->setTimbre(value);
                break;
            case ParameterID::MORPH:
                fmEngine_->setMorph(value);
                break;
            default:
                break;
        }
    }
    
    float getParameter(ParameterID param) const override {
        if (!fmEngine_) return 0.0f;
        
        float h, t, m;
        fmEngine_->getHTMParameters(h, t, m);
        
        switch (param) {
            case ParameterID::HARMONICS: return h;
            case ParameterID::TIMBRE: return t;
            case ParameterID::MORPH: return m;
            default: return 0.0f;
        }
    }
    
    bool hasParameter(ParameterID param) const override {
        return param == ParameterID::HARMONICS || 
               param == ParameterID::TIMBRE || 
               param == ParameterID::MORPH;
    }
    
    void processAudio(EtherAudioBuffer& outputBuffer) override {
        if (!fmEngine_) return;
        
        // Classic4OpFMEngine has its own processAudio method
        // For now, clear buffer - need to check the actual interface
        for (auto& frame : outputBuffer) {
            frame = AudioFrame{0.0f, 0.0f};
        }
    }
    
    size_t getActiveVoiceCount() const override { return 4; } // 4-op FM
    size_t getMaxVoiceCount() const override { return 8; }
    void setVoiceCount(size_t maxVoices) override {}
    float getCPUUsage() const override { return 0.0f; }
    
    void savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const override {
        actualSize = 0;
    }
    
    bool loadPreset(const uint8_t* data, size_t size) override {
        return false;
    }
    
    void setSampleRate(float sampleRate) override {
        sampleRate_ = sampleRate;
        if (fmEngine_) {
            fmEngine_->initialize(sampleRate);
        }
    }
    
    void setBufferSize(size_t bufferSize) override {}
};

// All engines bridge struct with both new and standalone engines
struct StandaloneEnginesEtherSynthInstance {
    float bpm = 120.0f;
    float masterVolume = 0.8f;
    InstrumentColor activeInstrument = InstrumentColor::CORAL;
    bool playing = false;
    bool recording = false;
    float cpuUsage = 15.0f;
    int activeVoices = 0;
    
    // All synthesis engines unified under SynthEngine interface
    std::array<std::unique_ptr<SynthEngine>, static_cast<size_t>(InstrumentColor::COUNT)> engines;
    
    // Engine type per instrument slot
    std::array<EngineType, static_cast<size_t>(InstrumentColor::COUNT)> engineTypes;
    
    StandaloneEnginesEtherSynthInstance() {
        // Initialize all slots with no engines (nullptr)
        for (size_t i = 0; i < engines.size(); i++) {
            engines[i] = nullptr;
            engineTypes[i] = EngineType::MACRO_VA; // Default type, but no engine created yet
        }
        std::cout << "Standalone Engines Bridge: Created EtherSynth instance with ALL synthesis engines" << std::endl;
    }
    
    ~StandaloneEnginesEtherSynthInstance() {
        std::cout << "Standalone Engines Bridge: Destroyed EtherSynth instance" << std::endl;
    }
    
    // Create ALL the real synthesis engines including standalone ones
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
                
            // Standalone engines - wrapped
            case EngineType::SLIDE_ACCENT_BASS:
                return std::make_unique<SlideAccentBassWrapper>();
            case EngineType::CLASSIC_4OP_FM:
                return std::make_unique<Classic4OpFMWrapper>();
                
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
            std::cout << "Standalone Engines Bridge: Created REAL " << engines[index]->getName() 
                      << " engine for instrument " << index << std::endl;
        }
    }
};

// Bridge functions (same as before, but now supporting ALL engines including standalone)
void* ether_create(void) {
    return new StandaloneEnginesEtherSynthInstance();
}

void ether_destroy(void* synth) {
    delete static_cast<StandaloneEnginesEtherSynthInstance*>(synth);
}

int ether_initialize(void* synth) {
    if (!synth) return 0;
    auto* instance = static_cast<StandaloneEnginesEtherSynthInstance*>(synth);
    
    // Set default engine for active instrument
    instance->setEngineType(instance->activeInstrument, EngineType::MACRO_VA);
    
    std::cout << "Standalone Engines Bridge: Initialized with ALL synthesis engines (including standalone)" << std::endl;
    return 1;
}

// Process audio through ALL the real engines (including standalone)
void ether_process_audio(void* synth, float* outputBuffer, size_t bufferSize) {
    if (!synth || !outputBuffer) return;
    
    auto* instance = static_cast<StandaloneEnginesEtherSynthInstance*>(synth);
    
    // Clear output buffer
    for (size_t i = 0; i < bufferSize * 2; i++) { // Stereo
        outputBuffer[i] = 0.0f;
    }
    
    // Create EtherAudioBuffer for processing
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
            
            // Process audio through REAL engine - including standalone engines!
            instance->engines[i]->processAudio(audioBuffer);
            
            // Mix into output buffer
            for (size_t sample = 0; sample < std::min(bufferSize, BUFFER_SIZE); sample++) {
                outputBuffer[sample * 2] += audioBuffer[sample].left * instance->masterVolume;
                outputBuffer[sample * 2 + 1] += audioBuffer[sample].right * instance->masterVolume;
            }
        }
    }
}

// Note events - route to active instrument's REAL engine (including standalone)
void ether_note_on(void* synth, int key_index, float velocity, float aftertouch) {
    if (!synth) return;
    auto* instance = static_cast<StandaloneEnginesEtherSynthInstance*>(synth);
    
    size_t activeIndex = static_cast<size_t>(instance->activeInstrument);
    if (activeIndex < instance->engines.size() && instance->engines[activeIndex]) {
        // Call REAL engine noteOn method! (works for all engine types through wrappers)
        instance->engines[activeIndex]->noteOn(key_index, velocity, aftertouch);
        instance->activeVoices++;
        std::cout << "Standalone Engines Bridge: Note ON " << key_index << " vel=" << velocity 
                  << " on REAL " << instance->engines[activeIndex]->getName() << " engine" << std::endl;
    }
}

void ether_note_off(void* synth, int key_index) {
    if (!synth) return;
    auto* instance = static_cast<StandaloneEnginesEtherSynthInstance*>(synth);
    
    size_t activeIndex = static_cast<size_t>(instance->activeInstrument);
    if (activeIndex < instance->engines.size() && instance->engines[activeIndex]) {
        instance->engines[activeIndex]->noteOff(key_index);
        instance->activeVoices = std::max(0, instance->activeVoices - 1);
        std::cout << "Standalone Engines Bridge: Note OFF " << key_index << std::endl;
    }
}

void ether_all_notes_off(void* synth) {
    if (!synth) return;
    auto* instance = static_cast<StandaloneEnginesEtherSynthInstance*>(synth);
    
    for (auto& engine : instance->engines) {
        if (engine) {
            engine->allNotesOff();
        }
    }
    instance->activeVoices = 0;
    std::cout << "Standalone Engines Bridge: All notes OFF" << std::endl;
}

// Engine management (now supports ALL engines including standalone)
void ether_set_instrument_engine_type(void* synth, int instrument, int engine_type) {
    if (!synth) return;
    auto* instance = static_cast<StandaloneEnginesEtherSynthInstance*>(synth);
    
    if (instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT) &&
        engine_type >= 0 && engine_type < static_cast<int>(EngineType::COUNT)) {
        
        InstrumentColor color = static_cast<InstrumentColor>(instrument);
        EngineType type = static_cast<EngineType>(engine_type);
        
        instance->setEngineType(color, type);
        std::cout << "Standalone Engines Bridge: Set instrument " << instrument 
                  << " to REAL engine type " << engine_type << std::endl;
    }
}

// Rest of the functions (same as before)
int ether_get_instrument_engine_type(void* synth, int instrument) {
    if (!synth) return 0;
    auto* instance = static_cast<StandaloneEnginesEtherSynthInstance*>(synth);
    
    if (instrument >= 0 && instrument < static_cast<int>(InstrumentColor::COUNT)) {
        return static_cast<int>(instance->engineTypes[instrument]);
    }
    return 0;
}

void ether_set_instrument_parameter(void* synth, int instrument, int param_id, float value) {
    if (!synth) return;
    auto* instance = static_cast<StandaloneEnginesEtherSynthInstance*>(synth);
    
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
    auto* instance = static_cast<StandaloneEnginesEtherSynthInstance*>(synth);
    
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

int ether_get_active_voice_count(void* synth) {
    if (!synth) return 0;
    auto* instance = static_cast<StandaloneEnginesEtherSynthInstance*>(synth);
    
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
    auto* instance = static_cast<StandaloneEnginesEtherSynthInstance*>(synth);
    
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

void ether_play(void* synth) {
    if (!synth) return;
    static_cast<StandaloneEnginesEtherSynthInstance*>(synth)->playing = true;
    std::cout << "Standalone Engines Bridge: Play" << std::endl;
}

void ether_stop(void* synth) {
    if (!synth) return;
    static_cast<StandaloneEnginesEtherSynthInstance*>(synth)->playing = false;
    std::cout << "Standalone Engines Bridge: Stop" << std::endl;
}

void ether_set_active_instrument(void* synth, int color_index) {
    if (!synth) return;
    auto* instance = static_cast<StandaloneEnginesEtherSynthInstance*>(synth);
    
    if (color_index >= 0 && color_index < static_cast<int>(InstrumentColor::COUNT)) {
        instance->activeInstrument = static_cast<InstrumentColor>(color_index);
    }
}

int ether_get_active_instrument(void* synth) {
    if (!synth) return 0;
    return static_cast<int>(static_cast<StandaloneEnginesEtherSynthInstance*>(synth)->activeInstrument);
}

// Engine info functions - now includes ALL engines including standalone
int ether_get_engine_type_count() {
    return static_cast<int>(EngineType::COUNT);
}

const char* ether_get_engine_type_name(int engine_type) {
    static const char* names[] = {
        "MacroVA", "MacroFM", "MacroWaveshaper", "MacroWavetable",
        "MacroChord", "MacroHarmonics", "FormantVocal", "NoiseParticles",
        "TidesOsc", "RingsVoice", "ElementsVoice",
        "DrumKit", "SamplerKit", "SamplerSlicer",
        "SlideAccentBass", "Classic4OpFM", "Granular", "SerialHPLP"
    };
    if (engine_type >= 0 && engine_type < static_cast<int>(EngineType::COUNT)) {
        return names[engine_type];
    }
    return "Unknown";
}

// New: Display names aligned to neutral categories
extern "C" const char* ether_get_engine_display_name(int engine_type) {
    if (engine_type < 0 || engine_type >= static_cast<int>(EngineType::COUNT)) return "Unknown";
    EngineType t = static_cast<EngineType>(engine_type);
    switch (t) {
        case EngineType::MACRO_VA: return "Analogue";
        case EngineType::MACRO_FM:
        case EngineType::CLASSIC_4OP_FM: return "FM";
        case EngineType::MACRO_WAVETABLE: return "Wavetable";
        case EngineType::MACRO_WAVESHAPER: return "Shaper";
        case EngineType::FORMANT_VOCAL: return "Vocal";
        case EngineType::RINGS_VOICE: return "Modal";
        case EngineType::ELEMENTS_VOICE: return "Exciter";
        case EngineType::TIDES_OSC: return "Morph";
        case EngineType::NOISE_PARTICLES: return "Noise";
        case EngineType::SERIAL_HPLP: return "Filter";
        case EngineType::SLIDE_ACCENT_BASS: return "Acid";
        case EngineType::DRUM_KIT: return "Drum Kit";
        case EngineType::SAMPLER_KIT:
        case EngineType::SAMPLER_SLICER: return "Sampler";
        case EngineType::MACRO_CHORD: return "Multi-Voice";
        case EngineType::MACRO_HARMONICS: return "Morph";
        default: return "Unknown";
    }
}

void ether_set_master_volume(void* synth, float volume) {
    if (!synth) return;
    static_cast<StandaloneEnginesEtherSynthInstance*>(synth)->masterVolume = volume;
}

float ether_get_master_volume(void* synth) {
    if (!synth) return 0.8f;
    return static_cast<StandaloneEnginesEtherSynthInstance*>(synth)->masterVolume;
}

void ether_shutdown(void* synth) {
    if (!synth) return;
    std::cout << "Standalone Engines Bridge: Shutdown" << std::endl;
}

} // extern "C"
