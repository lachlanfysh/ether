// ether Terminal Synthesizer Prototype - Complete Clean Version
// Compile: g++ -std=c++17 -framework CoreAudio -framework AudioUnit -o ether_synth ether_synth.cpp
// Run: ./ether_synth

#include <iostream>
#include <vector>
#include <array>
#include <memory>
#include <cmath>
#include <thread>
#include <atomic>
#include <chrono>
#include <map>
#include <algorithm>
#include <iomanip>
#include <termios.h>
#include <unistd.h>
#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudio.h>

// Constants
constexpr float SAMPLE_RATE = 48000.0f;
constexpr size_t BUFFER_SIZE = 512;
constexpr float TWO_PI = 2.0f * M_PI;
constexpr size_t MAX_VOICES = 32;

// Parameter system
enum class ParameterID {
    HARMONICS,
    TIMBRE, 
    MORPH,
    FILTER_CUTOFF,
    FILTER_RESONANCE,
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE,
    VOLUME,
    COUNT
};

// Synthesis engine types
enum class EngineType {
    SUBTRACTIVE,
    FM,
    ADDITIVE,
    NOISE,
    COUNT
};

// Basic ADSR envelope
class ADSREnvelope {
private:
    enum class Stage { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };
    
    Stage stage_ = Stage::IDLE;
    float current_level_ = 0.0f;
    float attack_rate_ = 0.001f;
    float decay_rate_ = 0.0005f;
    float sustain_level_ = 0.7f;
    float release_rate_ = 0.0002f;
    
public:
    void setAttack(float attack_time) {
        attack_rate_ = 1.0f / (attack_time * SAMPLE_RATE);
    }
    
    void setDecay(float decay_time) {
        decay_rate_ = 1.0f / (decay_time * SAMPLE_RATE);
    }
    
    void setSustain(float sustain_level) {
        sustain_level_ = sustain_level;
    }
    
    void setRelease(float release_time) {
        release_rate_ = 1.0f / (release_time * SAMPLE_RATE);
    }
    
    void noteOn() {
        stage_ = Stage::ATTACK;
    }
    
    void noteOff() {
        if (stage_ != Stage::IDLE) {
            stage_ = Stage::RELEASE;
        }
    }
    
    float process() {
        switch (stage_) {
            case Stage::IDLE:
                return 0.0f;
                
            case Stage::ATTACK:
                current_level_ += attack_rate_;
                if (current_level_ >= 1.0f) {
                    current_level_ = 1.0f;
                    stage_ = Stage::DECAY;
                }
                break;
                
            case Stage::DECAY:
                current_level_ -= decay_rate_;
                if (current_level_ <= sustain_level_) {
                    current_level_ = sustain_level_;
                    stage_ = Stage::SUSTAIN;
                }
                break;
                
            case Stage::SUSTAIN:
                current_level_ = sustain_level_;
                break;
                
            case Stage::RELEASE:
                current_level_ -= release_rate_;
                if (current_level_ <= 0.0f) {
                    current_level_ = 0.0f;
                    stage_ = Stage::IDLE;
                }
                break;
        }
        
        return current_level_;
    }
    
    bool isActive() const {
        return stage_ != Stage::IDLE;
    }
};

// Simple oscillator
class Oscillator {
public:
    enum class Waveform { SINE, SAW, SQUARE, TRIANGLE };
    
private:
    float frequency_ = 440.0f;
    float phase_ = 0.0f;
    float phase_increment_ = 0.0f;
    Waveform waveform_ = Waveform::SAW;
    
public:
    void setFrequency(float freq) {
        frequency_ = freq;
        phase_increment_ = TWO_PI * frequency_ / SAMPLE_RATE;
    }
    
    void setWaveform(Waveform waveform) {
        waveform_ = waveform;
    }
    
    float process() {
        float output = 0.0f;
        
        switch (waveform_) {
            case Waveform::SINE:
                output = std::sin(phase_);
                break;
                
            case Waveform::SAW:
                output = (2.0f * phase_ / TWO_PI) - 1.0f;
                break;
                
            case Waveform::SQUARE:
                output = (phase_ < M_PI) ? 1.0f : -1.0f;
                break;
                
            case Waveform::TRIANGLE:
                if (phase_ < M_PI) {
                    output = (2.0f * phase_ / M_PI) - 1.0f;
                } else {
                    output = 1.0f - (2.0f * (phase_ - M_PI) / M_PI);
                }
                break;
        }
        
        phase_ += phase_increment_;
        if (phase_ >= TWO_PI) {
            phase_ -= TWO_PI;
        }
        
        return output;
    }
};

// Simple low-pass filter
class LowPassFilter {
private:
    float cutoff_ = 1000.0f;
    float resonance_ = 0.1f;
    float x1_ = 0.0f, x2_ = 0.0f;
    float y1_ = 0.0f, y2_ = 0.0f;
    float a0_, a1_, a2_, b1_, b2_;
    
    void updateCoefficients() {
        float omega = TWO_PI * cutoff_ / SAMPLE_RATE;
        float sin_omega = std::sin(omega);
        float cos_omega = std::cos(omega);
        float alpha = sin_omega / (2.0f * (1.0f / resonance_));
        
        float b0 = 1.0f + alpha;
        a0_ = (1.0f - cos_omega) / 2.0f / b0;
        a1_ = (1.0f - cos_omega) / b0;
        a2_ = (1.0f - cos_omega) / 2.0f / b0;
        b1_ = -2.0f * cos_omega / b0;
        b2_ = (1.0f - alpha) / b0;
    }
    
public:
    void setCutoff(float cutoff) {
        cutoff_ = std::clamp(cutoff, 20.0f, 20000.0f);
        updateCoefficients();
    }
    
    void setResonance(float resonance) {
        resonance_ = std::clamp(resonance, 0.1f, 10.0f);
        updateCoefficients();
    }
    
    float process(float input) {
        float output = a0_ * input + a1_ * x1_ + a2_ * x2_ - b1_ * y1_ - b2_ * y2_;
        
        x2_ = x1_;
        x1_ = input;
        y2_ = y1_;
        y1_ = output;
        
        return output;
    }
};

// Base synthesis engine interface
class ISynthEngine {
public:
    virtual ~ISynthEngine() = default;
    virtual void noteOn(uint8_t note, uint8_t velocity) = 0;
    virtual void noteOff(uint8_t note) = 0;
    virtual void setParameter(ParameterID param, float value) = 0;
    virtual float getParameter(ParameterID param) const = 0;
    virtual void process(float* output, size_t frames) = 0;
    virtual const char* getName() const = 0;
};

// Subtractive synthesis engine
class SubtractiveEngine : public ISynthEngine {
private:
    Oscillator osc1_, osc2_;
    LowPassFilter filter_;
    ADSREnvelope envelope_;
    
    float osc_mix_ = 0.5f;
    float filter_cutoff_ = 1000.0f;
    float filter_resonance_ = 1.0f;
    float volume_ = 0.5f;
    
    bool note_active_ = false;
    float current_note_freq_ = 440.0f;
    
public:
    SubtractiveEngine() {
        osc1_.setWaveform(Oscillator::Waveform::SAW);
        osc2_.setWaveform(Oscillator::Waveform::SAW);
        envelope_.setAttack(0.01f);
        envelope_.setDecay(0.1f);
        envelope_.setSustain(0.7f);
        envelope_.setRelease(0.2f);
        filter_.setCutoff(filter_cutoff_);
        filter_.setResonance(filter_resonance_);
    }
    
    void noteOn(uint8_t note, uint8_t velocity) override {
        current_note_freq_ = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
        osc1_.setFrequency(current_note_freq_);
        osc2_.setFrequency(current_note_freq_ * 1.005f); // Slight detune
        envelope_.noteOn();
        note_active_ = true;
    }
    
    void noteOff(uint8_t note) override {
        envelope_.noteOff();
        note_active_ = false;
    }
    
    void setParameter(ParameterID param, float value) override {
        switch (param) {
            case ParameterID::HARMONICS:
                osc_mix_ = value;
                break;
            case ParameterID::TIMBRE:
                filter_cutoff_ = 200.0f + value * 4800.0f;
                filter_.setCutoff(filter_cutoff_);
                break;
            case ParameterID::MORPH:
                filter_resonance_ = 0.5f + value * 4.5f;
                filter_.setResonance(filter_resonance_);
                break;
            case ParameterID::ATTACK:
                envelope_.setAttack(0.001f + value * 2.0f);
                break;
            case ParameterID::DECAY:
                envelope_.setDecay(0.01f + value * 2.0f);
                break;
            case ParameterID::SUSTAIN:
                envelope_.setSustain(value);
                break;
            case ParameterID::RELEASE:
                envelope_.setRelease(0.01f + value * 3.0f);
                break;
            case ParameterID::VOLUME:
                volume_ = value;
                break;
            default:
                break;
        }
    }
    
    float getParameter(ParameterID param) const override {
        switch (param) {
            case ParameterID::HARMONICS: return osc_mix_;
            case ParameterID::TIMBRE: return (filter_cutoff_ - 200.0f) / 4800.0f;
            case ParameterID::MORPH: return (filter_resonance_ - 0.5f) / 4.5f;
            case ParameterID::VOLUME: return volume_;
            default: return 0.0f;
        }
    }
    
    void process(float* output, size_t frames) override {
        for (size_t i = 0; i < frames; i++) {
            if (!envelope_.isActive()) {
                output[i] = 0.0f;
                continue;
            }
            
            float osc1_out = osc1_.process();
            float osc2_out = osc2_.process();
            float mixed = osc1_out * (1.0f - osc_mix_) + osc2_out * osc_mix_;
            
            float filtered = filter_.process(mixed);
            float envelope_out = envelope_.process();
            
            output[i] = filtered * envelope_out * volume_;
        }
    }
    
    const char* getName() const override {
        return "Subtractive";
    }
};

// FM synthesis engine
class FMEngine : public ISynthEngine {
private:
    Oscillator carrier_, modulator_;
    ADSREnvelope envelope_;
    
    float fm_ratio_ = 1.0f;
    float fm_index_ = 1.0f;
    float volume_ = 0.5f;
    
    bool note_active_ = false;
    float base_frequency_ = 440.0f;
    
public:
    FMEngine() {
        carrier_.setWaveform(Oscillator::Waveform::SINE);
        modulator_.setWaveform(Oscillator::Waveform::SINE);
        envelope_.setAttack(0.01f);
        envelope_.setDecay(0.1f);
        envelope_.setSustain(0.7f);
        envelope_.setRelease(0.2f);
    }
    
    void noteOn(uint8_t note, uint8_t velocity) override {
        base_frequency_ = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
        carrier_.setFrequency(base_frequency_);
        modulator_.setFrequency(base_frequency_ * fm_ratio_);
        envelope_.noteOn();
        note_active_ = true;
    }
    
    void noteOff(uint8_t note) override {
        envelope_.noteOff();
        note_active_ = false;
    }
    
    void setParameter(ParameterID param, float value) override {
        switch (param) {
            case ParameterID::HARMONICS:
                fm_ratio_ = 0.5f + value * 7.5f; // 0.5 to 8.0
                modulator_.setFrequency(base_frequency_ * fm_ratio_);
                break;
            case ParameterID::TIMBRE:
                fm_index_ = value * 10.0f; // 0 to 10
                break;
            case ParameterID::ATTACK:
                envelope_.setAttack(0.001f + value * 2.0f);
                break;
            case ParameterID::DECAY:
                envelope_.setDecay(0.01f + value * 2.0f);
                break;
            case ParameterID::SUSTAIN:
                envelope_.setSustain(value);
                break;
            case ParameterID::RELEASE:
                envelope_.setRelease(0.01f + value * 3.0f);
                break;
            case ParameterID::VOLUME:
                volume_ = value;
                break;
            default:
                break;
        }
    }
    
    float getParameter(ParameterID param) const override {
        switch (param) {
            case ParameterID::HARMONICS: return (fm_ratio_ - 0.5f) / 7.5f;
            case ParameterID::TIMBRE: return fm_index_ / 10.0f;
            case ParameterID::VOLUME: return volume_;
            default: return 0.0f;
        }
    }
    
    void process(float* output, size_t frames) override {
        for (size_t i = 0; i < frames; i++) {
            if (!envelope_.isActive()) {
                output[i] = 0.0f;
                continue;
            }
            
            float modulator_out = modulator_.process() * fm_index_;
            // Simulate FM by frequency modulation (simplified)
            float fm_output = std::sin(carrier_.process() + modulator_out);
            float envelope_out = envelope_.process();
            
            output[i] = fm_output * envelope_out * volume_;
        }
    }
    
    const char* getName() const override {
        return "FM";
    }
};

// Chord generator (Orchid-style)
class ChordGenerator {
public:
    enum class ChordType {
        MAJOR,
        MINOR,
        SEVENTH,
        MAJOR_SEVENTH,
        MINOR_SEVENTH,
        DIMINISHED,
        AUGMENTED,
        SUS2,
        SUS4,
        COUNT
    };
    
    enum class Voicing {
        ROOT_POSITION,
        FIRST_INVERSION,
        SECOND_INVERSION,
        WIDE_SPREAD,
        CLOSE_VOICING,
        COUNT
    };
    
private:
    ChordType current_chord_type_ = ChordType::MAJOR;
    Voicing current_voicing_ = Voicing::ROOT_POSITION;
    uint8_t root_note_ = 60; // C4
    
    // Chord interval patterns (semitones from root)
    static const std::map<ChordType, std::vector<int>> chord_intervals_;
    
public:
    void setChordType(ChordType type) { current_chord_type_ = type; }
    void setVoicing(Voicing voicing) { current_voicing_ = voicing; }
    void setRootNote(uint8_t note) { root_note_ = note; }
    
    ChordType getChordType() const { return current_chord_type_; }
    Voicing getVoicing() const { return current_voicing_; }
    
    std::vector<uint8_t> generateChord() const {
        auto it = chord_intervals_.find(current_chord_type_);
        if (it == chord_intervals_.end()) {
            return {root_note_}; // Fallback to root note
        }
        
        std::vector<uint8_t> chord_notes;
        for (int interval : it->second) {
            chord_notes.push_back(root_note_ + interval);
        }
        
        // Apply voicing
        applyVoicing(chord_notes);
        
        return chord_notes;
    }
    
    const char* getChordTypeName() const {
        switch (current_chord_type_) {
            case ChordType::MAJOR: return "Major";
            case ChordType::MINOR: return "Minor";
            case ChordType::SEVENTH: return "7th";
            case ChordType::MAJOR_SEVENTH: return "Maj7";
            case ChordType::MINOR_SEVENTH: return "Min7";
            case ChordType::DIMINISHED: return "Dim";
            case ChordType::AUGMENTED: return "Aug";
            case ChordType::SUS2: return "Sus2";
            case ChordType::SUS4: return "Sus4";
            default: return "Unknown";
        }
    }
    
    const char* getVoicingName() const {
        switch (current_voicing_) {
            case Voicing::ROOT_POSITION: return "Root";
            case Voicing::FIRST_INVERSION: return "1st Inv";
            case Voicing::SECOND_INVERSION: return "2nd Inv"; 
            case Voicing::WIDE_SPREAD: return "Wide";
            case Voicing::CLOSE_VOICING: return "Close";
            default: return "Unknown";
        }
    }
    
private:
    void applyVoicing(std::vector<uint8_t>& notes) const {
        if (notes.empty()) return;
        
        switch (current_voicing_) {
            case Voicing::ROOT_POSITION:
                // No change - already in root position
                break;
                
            case Voicing::FIRST_INVERSION:
                if (notes.size() >= 3) {
                    notes[0] += 12; // Move root up an octave
                    std::rotate(notes.begin(), notes.begin() + 1, notes.end());
                }
                break;
                
            case Voicing::SECOND_INVERSION:
                if (notes.size() >= 3) {
                    notes[0] += 12; // Move root up
                    notes[1] += 12; // Move third up
                    std::rotate(notes.begin(), notes.begin() + 2, notes.end());
                }
                break;
                
            case Voicing::WIDE_SPREAD:
                // Spread notes across multiple octaves
                for (size_t i = 1; i < notes.size(); i += 2) {
                    notes[i] += 12;
                }
                break;
                
            case Voicing::CLOSE_VOICING:
                // Keep all notes within one octave (already done by default)
                break;
                
            case Voicing::COUNT:
                // Should never happen
                break;
        }
        
        // Ensure all notes are in valid MIDI range
        for (auto& note : notes) {
            while (note > 127) note -= 12;
            while (note < 0) note += 12;
        }
    }
};

// Define chord intervals
const std::map<ChordGenerator::ChordType, std::vector<int>> ChordGenerator::chord_intervals_ = {
    {ChordType::MAJOR, {0, 4, 7}},              // C E G
    {ChordType::MINOR, {0, 3, 7}},              // C Eb G
    {ChordType::SEVENTH, {0, 4, 7, 10}},        // C E G Bb
    {ChordType::MAJOR_SEVENTH, {0, 4, 7, 11}},  // C E G B
    {ChordType::MINOR_SEVENTH, {0, 3, 7, 10}},  // C Eb G Bb
    {ChordType::DIMINISHED, {0, 3, 6}},         // C Eb Gb
    {ChordType::AUGMENTED, {0, 4, 8}},          // C E G#
    {ChordType::SUS2, {0, 2, 7}},               // C D G
    {ChordType::SUS4, {0, 5, 7}}                // C F G
};

// Voice manager for polyphony
class VoiceManager {
private:
    struct Voice {
        std::unique_ptr<ISynthEngine> engine;
        uint8_t note = 0;
        bool active = false;
        uint32_t start_time = 0;
        
        Voice() = default;
        Voice(const Voice&) = delete;
        Voice& operator=(const Voice&) = delete;
        
        Voice(Voice&& other) noexcept 
            : engine(std::move(other.engine))
            , note(other.note)
            , active(other.active)
            , start_time(other.start_time) {}
        
        Voice& operator=(Voice&& other) noexcept {
            if (this != &other) {
                engine = std::move(other.engine);
                note = other.note;
                active = other.active;
                start_time = other.start_time;
            }
            return *this;
        }
    };
    
    std::array<Voice, MAX_VOICES> voices_;
    uint32_t voice_counter_ = 0;
    
public:
    void initializeVoices(EngineType engine_type) {
        for (auto& voice : voices_) {
            switch (engine_type) {
                case EngineType::SUBTRACTIVE:
                    voice.engine = std::make_unique<SubtractiveEngine>();
                    break;
                case EngineType::FM:
                    voice.engine = std::make_unique<FMEngine>();
                    break;
                default:
                    voice.engine = std::make_unique<SubtractiveEngine>();
                    break;
            }
            voice.active = false;
        }
    }
    
    void noteOn(uint8_t note, uint8_t velocity) {
        // Find free voice
        Voice* free_voice = nullptr;
        for (auto& voice : voices_) {
            if (!voice.active) {
                free_voice = &voice;
                break;
            }
        }
        
        // If no free voice, steal oldest
        if (!free_voice) {
            uint32_t oldest_time = UINT32_MAX;
            for (auto& voice : voices_) {
                if (voice.start_time < oldest_time) {
                    oldest_time = voice.start_time;
                    free_voice = &voice;
                }
            }
            if (free_voice && free_voice->engine) {
                free_voice->engine->noteOff(free_voice->note);
            }
        }
        
        // Assign new note
        if (free_voice && free_voice->engine) {
            free_voice->note = note;
            free_voice->active = true;
            free_voice->start_time = voice_counter_++;
            free_voice->engine->noteOn(note, velocity);
        }
    }
    
    void noteOff(uint8_t note) {
        for (auto& voice : voices_) {
            if (voice.active && voice.note == note && voice.engine) {
                voice.engine->noteOff(note);
            }
        }
    }
    
    void allNotesOff() {
        for (auto& voice : voices_) {
            if (voice.active && voice.engine) {
                voice.engine->noteOff(voice.note);
            }
        }
    }
    
    void setParameter(ParameterID param, float value) {
        for (auto& voice : voices_) {
            if (voice.engine) {
                voice.engine->setParameter(param, value);
            }
        }
    }
    
    void process(float* output, size_t frames) {
        // Clear output buffer
        std::fill(output, output + frames, 0.0f);
        
        // Count active voices
        size_t active_count = 0;
        for (const auto& voice : voices_) {
            if (voice.active) active_count++;
        }
        
        // Dynamic volume scaling
        float voice_scale = 0.8f / std::max(1.0f, std::sqrt(static_cast<float>(active_count)));
        
        // Mix all active voices
        std::vector<float> voice_buffer(frames);
        for (auto& voice : voices_) {
            if (voice.active && voice.engine) {
                std::fill(voice_buffer.begin(), voice_buffer.end(), 0.0f);
                voice.engine->process(voice_buffer.data(), frames);
                
                // Mix into output
                for (size_t i = 0; i < frames; i++) {
                    output[i] += voice_buffer[i] * voice_scale;
                }
                
                // Check if voice should be deactivated
                bool still_active = false;
                for (size_t i = 0; i < frames; i++) {
                    if (std::abs(voice_buffer[i]) > 0.001f) {
                        still_active = true;
                        break;
                    }
                }
                if (!still_active) {
                    voice.active = false;
                }
            }
        }
    }
    
    size_t getActiveVoiceCount() const {
        size_t count = 0;
        for (const auto& voice : voices_) {
            if (voice.active) count++;
        }
        return count;
    }
};

// Main synthesizer class
class TerminalSynth {
private:
    VoiceManager voice_manager_;
    ChordGenerator chord_generator_;
    EngineType current_engine_type_ = EngineType::SUBTRACTIVE;
    
    // Parameter values (0.0 - 1.0)
    std::array<float, static_cast<size_t>(ParameterID::COUNT)> parameters_;
    
    // CoreAudio components
    AudioUnit audio_unit_;
    bool audio_initialized_ = false;
    
    // Terminal control
    struct termios old_termios_;
    std::atomic<bool> running_{true};
    
    // Chord mode
    bool chord_mode_ = false;
    std::vector<uint8_t> currently_held_chord_;
    
public:
    TerminalSynth() {
        // Initialize voice manager with default engine
        voice_manager_.initializeVoices(current_engine_type_);
        
        // Initialize parameters
        std::fill(parameters_.begin(), parameters_.end(), 0.5f);
        
        // Set some reasonable defaults for ADSR
        parameters_[static_cast<size_t>(ParameterID::ATTACK)] = 0.1f;   // Quick attack
        parameters_[static_cast<size_t>(ParameterID::DECAY)] = 0.3f;    // Medium decay
        parameters_[static_cast<size_t>(ParameterID::SUSTAIN)] = 0.7f;  // High sustain
        parameters_[static_cast<size_t>(ParameterID::RELEASE)] = 0.4f;  // Medium release
        
        updateAllParameters();
        
        setupTerminal();
        initializeAudio();
    }
    
    ~TerminalSynth() {
        running_ = false;
        if (audio_initialized_) {
            AudioUnitUninitialize(audio_unit_);
            AudioComponentInstanceDispose(audio_unit_);
        }
        restoreTerminal();
    }
    
    void run() {
        printInterface();
        
        char ch;
        while (running_ && read(STDIN_FILENO, &ch, 1) == 1) {
            handleInput(ch);
        }
    }

private:
    void setupTerminal() {
        tcgetattr(STDIN_FILENO, &old_termios_);
        struct termios new_termios = old_termios_;
        new_termios.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    }
    
    void restoreTerminal() {
        tcsetattr(STDIN_FILENO, TCSANOW, &old_termios_);
    }
    
    static OSStatus audioCallback(void* inRefCon,
                                 AudioUnitRenderActionFlags* ioActionFlags,
                                 const AudioTimeStamp* inTimeStamp,
                                 UInt32 inBusNumber,
                                 UInt32 inNumberFrames,
                                 AudioBufferList* ioData) {
        TerminalSynth* synth = static_cast<TerminalSynth*>(inRefCon);
        return synth->renderAudio(ioData, inNumberFrames);
    }
    
    OSStatus renderAudio(AudioBufferList* ioData, UInt32 frames) {
        float* output = static_cast<float*>(ioData->mBuffers[0].mData);
        voice_manager_.process(output, frames);
        return noErr;
    }
    
    void initializeAudio() {
        AudioComponentDescription desc = {};
        desc.componentType = kAudioUnitType_Output;
        desc.componentSubType = kAudioUnitSubType_DefaultOutput;
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;
        
        AudioComponent component = AudioComponentFindNext(nullptr, &desc);
        if (!component) return;
        
        if (AudioComponentInstanceNew(component, &audio_unit_) != noErr) return;
        
        // Set format
        AudioStreamBasicDescription format = {};
        format.mSampleRate = SAMPLE_RATE;
        format.mFormatID = kAudioFormatLinearPCM;
        format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
        format.mChannelsPerFrame = 1;
        format.mBitsPerChannel = 32;
        format.mBytesPerFrame = 4;
        format.mBytesPerPacket = 4;
        format.mFramesPerPacket = 1;
        
        AudioUnitSetProperty(audio_unit_, kAudioUnitProperty_StreamFormat,
                           kAudioUnitScope_Input, 0, &format, sizeof(format));
        
        // Set callback
        AURenderCallbackStruct callback = {};
        callback.inputProc = audioCallback;
        callback.inputProcRefCon = this;
        
        AudioUnitSetProperty(audio_unit_, kAudioUnitProperty_SetRenderCallback,
                           kAudioUnitScope_Input, 0, &callback, sizeof(callback));
        
        if (AudioUnitInitialize(audio_unit_) == noErr) {
            AudioOutputUnitStart(audio_unit_);
            audio_initialized_ = true;
        }
    }
    
    void handleInput(char ch) {
        switch (ch) {
            // Piano keys (bottom row)
            case 'z': handleKey(ch, 60); break; // C
            case 's': handleKey(ch, 61); break; // C#
            case 'x': handleKey(ch, 62); break; // D
            case 'd': handleKey(ch, 63); break; // D#
            case 'c': handleKey(ch, 64); break; // E
            case 'v': handleKey(ch, 65); break; // F
            case 'g': handleKey(ch, 66); break; // F#
            case 'b': handleKey(ch, 67); break; // G
            case 'h': handleKey(ch, 68); break; // G#
            case 'n': handleKey(ch, 69); break; // A
            case 'j': handleKey(ch, 70); break; // A#
            case 'm': handleKey(ch, 71); break; // B
            
            // Upper octave
            case 'q': handleKey(ch, 72); break; // C
            case '2': handleKey(ch, 73); break; // C#
            case 'w': handleKey(ch, 74); break; // D
            case '3': handleKey(ch, 75); break; // D#
            case 'e': handleKey(ch, 76); break; // E
            case 'r': handleKey(ch, 77); break; // F
            case '5': handleKey(ch, 78); break; // F#
            case 't': handleKey(ch, 79); break; // G
            case '6': handleKey(ch, 80); break; // G#
            case 'y': handleKey(ch, 81); break; // A
            case '7': handleKey(ch, 82); break; // A#
            case 'u': handleKey(ch, 83); break; // B
            
            // Engine selection
            case '1': selectEngine(EngineType::SUBTRACTIVE); break;
            case '[': selectEngine(EngineType::FM); break;
            case ']': selectEngine(EngineType::ADDITIVE); break;
            case '\\': selectEngine(EngineType::NOISE); break;
            
            // Parameter control
            case 'a': adjustParameter(ParameterID::HARMONICS, -0.05f); break;
            case 'A': adjustParameter(ParameterID::HARMONICS, 0.05f); break;
            case 'f': adjustParameter(ParameterID::TIMBRE, -0.05f); break;
            case 'F': adjustParameter(ParameterID::TIMBRE, 0.05f); break;
            case 'p': adjustParameter(ParameterID::MORPH, -0.05f); break;
            case 'P': adjustParameter(ParameterID::MORPH, 0.05f); break;
            case 'o': adjustParameter(ParameterID::VOLUME, -0.05f); break;
            case 'O': adjustParameter(ParameterID::VOLUME, 0.05f); break;
            
            // ADSR controls
            case 'k': adjustParameter(ParameterID::ATTACK, -0.05f); break;
            case 'K': adjustParameter(ParameterID::ATTACK, 0.05f); break;
            case 'l': adjustParameter(ParameterID::DECAY, -0.05f); break;
            case 'L': adjustParameter(ParameterID::DECAY, 0.05f); break;
            case ';': adjustParameter(ParameterID::SUSTAIN, -0.05f); break;
            case ':': adjustParameter(ParameterID::SUSTAIN, 0.05f); break;
            case '\'': adjustParameter(ParameterID::RELEASE, -0.05f); break;
            case '"': adjustParameter(ParameterID::RELEASE, 0.05f); break;
            
            // Chord controls
            case '0': toggleChordMode(); break;
            case '9': cycleChordType(-1); break;
            case '(': cycleChordType(1); break;
            case '8': cycleVoicing(-1); break;
            case '*': cycleVoicing(1); break;
            
            // Special keys
            case ' ': allNotesOff(); break;
            case 27: // ESC
                running_ = false;
                break;
            
            default:
                break;
        }
        
        if (ch != 27) { // Don't refresh on ESC
            printInterface();
        }
    }
    
    void handleKey(char key_char, uint8_t note) {
        if (chord_mode_) {
            // In chord mode, any key triggers a chord based on that root note
            chord_generator_.setRootNote(note);
            playChord();
        } else {
            // Normal single note mode with auto note-off
            voice_manager_.noteOn(note, 100);
            
            // Schedule auto note-off in a separate thread (terminal workaround)
            std::thread([this, note]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                voice_manager_.noteOff(note);
            }).detach();
        }
    }
    
    void toggleChordMode() {
        chord_mode_ = !chord_mode_;
        if (!chord_mode_) {
            allNotesOff();
        }
    }
    
    void cycleChordType(int direction) {
        int current = static_cast<int>(chord_generator_.getChordType());
        int count = static_cast<int>(ChordGenerator::ChordType::COUNT);
        current = (current + direction + count) % count;
        chord_generator_.setChordType(static_cast<ChordGenerator::ChordType>(current));
    }
    
    void cycleVoicing(int direction) {
        int current = static_cast<int>(chord_generator_.getVoicing());
        int count = static_cast<int>(ChordGenerator::Voicing::COUNT);
        current = (current + direction + count) % count;
        chord_generator_.setVoicing(static_cast<ChordGenerator::Voicing>(current));
    }
    
    void playChord() {
        // Stop current chord
        for (uint8_t note : currently_held_chord_) {
            voice_manager_.noteOff(note);
        }
        
        // Generate and play new chord
        currently_held_chord_ = chord_generator_.generateChord();
        for (uint8_t note : currently_held_chord_) {
            voice_manager_.noteOn(note, 100);
        }
        
        // Auto release chord after delay
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            for (uint8_t note : currently_held_chord_) {
                voice_manager_.noteOff(note);
            }
        }).detach();
    }
    
    void allNotesOff() {
        voice_manager_.allNotesOff();
        currently_held_chord_.clear();
    }
    
    void selectEngine(EngineType engine_type) {
        allNotesOff();
        current_engine_type_ = engine_type;
        voice_manager_.initializeVoices(engine_type);
        updateAllParameters();
    }
    
    void adjustParameter(ParameterID param, float delta) {
        size_t param_index = static_cast<size_t>(param);
        parameters_[param_index] = std::clamp(parameters_[param_index] + delta, 0.0f, 1.0f);
        voice_manager_.setParameter(param, parameters_[param_index]);
    }
    
    void updateAllParameters() {
        for (size_t i = 0; i < parameters_.size(); i++) {
            voice_manager_.setParameter(static_cast<ParameterID>(i), parameters_[i]);
        }
    }
    
    const char* getEngineName() const {
        switch (current_engine_type_) {
            case EngineType::SUBTRACTIVE: return "Subtractive";
            case EngineType::FM: return "FM";
            case EngineType::ADDITIVE: return "Additive";
            case EngineType::NOISE: return "Noise";
            default: return "Unknown";
        }
    }
    
    void printInterface() {
        // Clear screen and move cursor to top
        std::cout << "\033[2J\033[H";
        
        std::cout << "=== ether Terminal Synthesizer (Polyphonic) ===" << std::endl << std::endl;
        
        // Current engine and voice info
        std::cout << "Engine: " << getEngineName() << std::endl;
        std::cout << "Active Voices: " << voice_manager_.getActiveVoiceCount() << "/32" << std::endl;
        
        // Chord mode status
        if (chord_mode_) {
            std::cout << "🎵 CHORD MODE: " << chord_generator_.getChordTypeName() 
                      << " (" << chord_generator_.getVoicingName() << ")" << std::endl;
        } else {
            std::cout << "Single Note Mode" << std::endl;
        }
        std::cout << std::endl;
        
        // Parameters
        std::cout << "Synthesis Parameters:" << std::endl;
        std::cout << "  Harmonics: " << std::fixed << std::setprecision(2) 
                  << parameters_[static_cast<size_t>(ParameterID::HARMONICS)] 
                  << " (a/A)" << std::endl;
        std::cout << "  Timbre:    " << parameters_[static_cast<size_t>(ParameterID::TIMBRE)] 
                  << " (f/F)" << std::endl;
        std::cout << "  Morph:     " << parameters_[static_cast<size_t>(ParameterID::MORPH)] 
                  << " (p/P)" << std::endl;
        std::cout << "  Volume:    " << parameters_[static_cast<size_t>(ParameterID::VOLUME)] 
                  << " (o/O)" << std::endl << std::endl;
        
        std::cout << "ADSR Envelope:" << std::endl;
        std::cout << "  Attack:    " << parameters_[static_cast<size_t>(ParameterID::ATTACK)] 
                  << " (k/K)" << std::endl;
        std::cout << "  Decay:     " << parameters_[static_cast<size_t>(ParameterID::DECAY)] 
                  << " (l/L)" << std::endl;
        std::cout << "  Sustain:   " << parameters_[static_cast<size_t>(ParameterID::SUSTAIN)] 
                  << " (;/:)" << std::endl;
        std::cout << "  Release:   " << parameters_[static_cast<size_t>(ParameterID::RELEASE)] 
                  << " ('/" << '"' << ")" << std::endl << std::endl;
        
        // Keyboard layout
        std::cout << "Piano Keys:" << std::endl;
        std::cout << "  Upper: Q2W3ER5T6Y7U" << std::endl;
        std::cout << "  Lower: ZSXDCVGBHNJM" << std::endl << std::endl;
        
        // Controls
        std::cout << "Controls:" << std::endl;
        std::cout << "  ENGINES: 1,[,],\\: Select engine (Sub/FM/Add/Noise)" << std::endl;
        std::cout << "  SYNTH:   a/A: Harmonics    f/F: Timbre" << std::endl;
        std::cout << "           p/P: Morph        o/O: Volume" << std::endl;
        std::cout << "  ADSR:    k/K: Attack       l/L: Decay" << std::endl;
        std::cout << "           ;/:: Sustain      '/\": Release" << std::endl;
        std::cout << "  CHORDS:  0: Toggle chord mode" << std::endl;
        std::cout << "           9/(: Chord type   8/*: Voicing" << std::endl;
        std::cout << "  PLAY:    Space: All off   ESC: Quit" << std::endl;
        
        std::cout.flush();
    }
};

int main() {
    try {
        TerminalSynth synth;
        synth.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}