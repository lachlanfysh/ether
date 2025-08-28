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
#include <random>
#include <optional>
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
    SUB_BASS,
    WARM_PAD,
    BRIGHT_LEAD,
    STRING_ENSEMBLE,
    GRANULAR,
    PLAITS,
    COUNT
};

// Plaits synthesis models
enum class PlaitsModel {
    VIRTUAL_ANALOG,     // Classic waveforms with waveshaping
    WAVESHAPING,        // Complex harmonic distortion
    FM_SYNTHESIS,       // 2-operator FM
    GRANULAR_CLOUD,     // Granular synthesis
    ADDITIVE,           // Harmonic synthesis
    WAVETABLE,          // Wavetable synthesis
    PHYSICAL_STRING,    // Physical modeling - string
    SPEECH_FORMANT,     // Speech/formant synthesis
    COUNT
};

// Drum types for drum machine
enum class DrumType {
    KICK,
    SNARE,
    HIHAT_CLOSED,
    HIHAT_OPEN,
    CLAP,
    CRASH,
    TOM_HIGH,
    TOM_LOW,
    COUNT
};

// Step sequencer constants
constexpr size_t MAX_DRUM_STEPS = 16;
constexpr size_t MAX_DRUM_TRACKS = 8;

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

// Sub Bass synthesis engine
class SubBassEngine : public ISynthEngine {
private:
    Oscillator osc_;
    ADSREnvelope envelope_;
    float volume_ = 0.8f;
    
public:
    SubBassEngine() {
        osc_.setWaveform(Oscillator::Waveform::SINE);
        envelope_.setAttack(0.01f);
        envelope_.setDecay(0.1f);
        envelope_.setSustain(0.9f);
        envelope_.setRelease(0.8f);
    }
    
    void noteOn(uint8_t note, uint8_t velocity) override {
        float freq = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
        osc_.setFrequency(freq);
        envelope_.noteOn();
    }
    
    void noteOff(uint8_t note) override {
        envelope_.noteOff();
    }
    
    void setParameter(ParameterID param, float value) override {
        switch (param) {
            case ParameterID::VOLUME:
                volume_ = value;
                break;
            case ParameterID::ATTACK:
                envelope_.setAttack(0.001f + value * 0.5f);
                break;
            case ParameterID::DECAY:
                envelope_.setDecay(0.01f + value * 1.0f);
                break;
            case ParameterID::SUSTAIN:
                envelope_.setSustain(value);
                break;
            case ParameterID::RELEASE:
                envelope_.setRelease(0.1f + value * 2.0f);
                break;
            default:
                break;
        }
    }
    
    float getParameter(ParameterID param) const override {
        switch (param) {
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
            
            float osc_out = osc_.process();
            float envelope_out = envelope_.process();
            
            output[i] = osc_out * envelope_out * volume_;
        }
    }
    
    const char* getName() const override {
        return "Sub Bass";
    }
};

// Warm Pad synthesis engine  
class WarmPadEngine : public ISynthEngine {
private:
    Oscillator osc1_, osc2_, osc3_;
    LowPassFilter filter_;
    ADSREnvelope envelope_;
    
    float volume_ = 0.6f;
    float detune_amount_ = 0.02f;
    
public:
    WarmPadEngine() {
        osc1_.setWaveform(Oscillator::Waveform::SAW);
        osc2_.setWaveform(Oscillator::Waveform::SAW);
        osc3_.setWaveform(Oscillator::Waveform::TRIANGLE);
        
        envelope_.setAttack(0.8f);
        envelope_.setDecay(0.3f);
        envelope_.setSustain(0.8f);
        envelope_.setRelease(1.5f);
        
        filter_.setCutoff(800.0f);
        filter_.setResonance(0.3f);
    }
    
    void noteOn(uint8_t note, uint8_t velocity) override {
        float freq = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
        osc1_.setFrequency(freq);
        osc2_.setFrequency(freq * (1.0f + detune_amount_));
        osc3_.setFrequency(freq * (1.0f - detune_amount_));
        envelope_.noteOn();
    }
    
    void noteOff(uint8_t note) override {
        envelope_.noteOff();
    }
    
    void setParameter(ParameterID param, float value) override {
        switch (param) {
            case ParameterID::HARMONICS:
                detune_amount_ = value * 0.05f;
                break;
            case ParameterID::TIMBRE:
                filter_.setCutoff(400.0f + value * 2000.0f);
                break;
            case ParameterID::VOLUME:
                volume_ = value;
                break;
            case ParameterID::ATTACK:
                envelope_.setAttack(0.1f + value * 2.0f);
                break;
            case ParameterID::DECAY:
                envelope_.setDecay(0.1f + value * 1.0f);
                break;
            case ParameterID::SUSTAIN:
                envelope_.setSustain(value);
                break;
            case ParameterID::RELEASE:
                envelope_.setRelease(0.5f + value * 3.0f);
                break;
            default:
                break;
        }
    }
    
    float getParameter(ParameterID param) const override {
        switch (param) {
            case ParameterID::HARMONICS: return detune_amount_ / 0.05f;
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
            float osc3_out = osc3_.process();
            float mixed = (osc1_out + osc2_out + osc3_out) / 3.0f;
            
            float filtered = filter_.process(mixed);
            float envelope_out = envelope_.process();
            
            output[i] = filtered * envelope_out * volume_;
        }
    }
    
    const char* getName() const override {
        return "Warm Pad";
    }
};

// Bright Lead synthesis engine
class BrightLeadEngine : public ISynthEngine {
private:
    Oscillator osc1_, osc2_;
    LowPassFilter filter_;
    ADSREnvelope envelope_;
    
    float osc_mix_ = 0.7f;
    float filter_cutoff_ = 2000.0f;
    float filter_resonance_ = 2.0f;
    float volume_ = 0.7f;
    
public:
    BrightLeadEngine() {
        osc1_.setWaveform(Oscillator::Waveform::SAW);
        osc2_.setWaveform(Oscillator::Waveform::SQUARE);
        envelope_.setAttack(0.01f);
        envelope_.setDecay(0.2f);
        envelope_.setSustain(0.6f);
        envelope_.setRelease(0.3f);
        filter_.setCutoff(filter_cutoff_);
        filter_.setResonance(filter_resonance_);
    }
    
    void noteOn(uint8_t note, uint8_t velocity) override {
        float freq = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
        osc1_.setFrequency(freq);
        osc2_.setFrequency(freq * 1.01f); // Slight detune
        envelope_.noteOn();
    }
    
    void noteOff(uint8_t note) override {
        envelope_.noteOff();
    }
    
    void setParameter(ParameterID param, float value) override {
        switch (param) {
            case ParameterID::HARMONICS:
                osc_mix_ = value;
                break;
            case ParameterID::TIMBRE:
                filter_cutoff_ = 500.0f + value * 4000.0f;
                filter_.setCutoff(filter_cutoff_);
                break;
            case ParameterID::MORPH:
                filter_resonance_ = 0.5f + value * 4.0f;
                filter_.setResonance(filter_resonance_);
                break;
            case ParameterID::VOLUME:
                volume_ = value;
                break;
            case ParameterID::ATTACK:
                envelope_.setAttack(0.001f + value * 0.1f);
                break;
            case ParameterID::DECAY:
                envelope_.setDecay(0.01f + value * 1.0f);
                break;
            case ParameterID::SUSTAIN:
                envelope_.setSustain(value);
                break;
            case ParameterID::RELEASE:
                envelope_.setRelease(0.01f + value * 1.0f);
                break;
            default:
                break;
        }
    }
    
    float getParameter(ParameterID param) const override {
        switch (param) {
            case ParameterID::HARMONICS: return osc_mix_;
            case ParameterID::TIMBRE: return (filter_cutoff_ - 500.0f) / 4000.0f;
            case ParameterID::MORPH: return (filter_resonance_ - 0.5f) / 4.0f;
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
        return "Bright Lead";
    }
};

// String Ensemble synthesis engine
class StringEnsembleEngine : public ISynthEngine {
private:
    Oscillator osc1_, osc2_, osc3_, osc4_;
    LowPassFilter filter_;
    ADSREnvelope envelope_;
    
    float volume_ = 0.5f;
    
public:
    StringEnsembleEngine() {
        osc1_.setWaveform(Oscillator::Waveform::SAW);
        osc2_.setWaveform(Oscillator::Waveform::SAW);
        osc3_.setWaveform(Oscillator::Waveform::SAW);
        osc4_.setWaveform(Oscillator::Waveform::SAW);
        
        envelope_.setAttack(0.5f);
        envelope_.setDecay(0.3f);
        envelope_.setSustain(0.9f);
        envelope_.setRelease(1.0f);
        
        filter_.setCutoff(1200.0f);
        filter_.setResonance(0.2f);
    }
    
    void noteOn(uint8_t note, uint8_t velocity) override {
        float freq = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
        osc1_.setFrequency(freq);
        osc2_.setFrequency(freq * 1.01f);
        osc3_.setFrequency(freq * 0.99f);
        osc4_.setFrequency(freq * 1.005f);
        envelope_.noteOn();
    }
    
    void noteOff(uint8_t note) override {
        envelope_.noteOff();
    }
    
    void setParameter(ParameterID param, float value) override {
        switch (param) {
            case ParameterID::TIMBRE:
                filter_.setCutoff(600.0f + value * 2000.0f);
                break;
            case ParameterID::VOLUME:
                volume_ = value;
                break;
            case ParameterID::ATTACK:
                envelope_.setAttack(0.1f + value * 2.0f);
                break;
            case ParameterID::DECAY:
                envelope_.setDecay(0.1f + value * 1.0f);
                break;
            case ParameterID::SUSTAIN:
                envelope_.setSustain(value);
                break;
            case ParameterID::RELEASE:
                envelope_.setRelease(0.5f + value * 3.0f);
                break;
            default:
                break;
        }
    }
    
    float getParameter(ParameterID param) const override {
        switch (param) {
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
            float osc3_out = osc3_.process();
            float osc4_out = osc4_.process();
            float mixed = (osc1_out + osc2_out + osc3_out + osc4_out) / 4.0f;
            
            float filtered = filter_.process(mixed);
            float envelope_out = envelope_.process();
            
            output[i] = filtered * envelope_out * volume_;
        }
    }
    
    const char* getName() const override {
        return "String Ensemble";
    }
};

// Granular synthesis engine
class GranularEngine : public ISynthEngine {
private:
    struct Grain {
        float position = 0.0f;
        float increment = 1.0f;
        float amplitude = 0.0f;
        uint32_t length = 0;
        uint32_t age = 0;
        bool active = false;
    };
    
    static constexpr size_t MAX_GRAINS = 8;
    std::array<Grain, MAX_GRAINS> grains_;
    std::vector<float> sample_buffer_;
    
    float grain_size_ = 0.1f;  // seconds
    float grain_density_ = 10.0f;  // grains per second
    float position_ = 0.5f;  // 0-1 through sample
    float pitch_ = 1.0f;
    float volume_ = 0.5f;
    
    float grain_accumulator_ = 0.0f;
    
public:
    GranularEngine() {
        // Create a simple waveform as our "sample"
        sample_buffer_.resize(static_cast<size_t>(SAMPLE_RATE * 2));  // 2 second buffer
        for (size_t i = 0; i < sample_buffer_.size(); i++) {
            float t = static_cast<float>(i) / SAMPLE_RATE;
            sample_buffer_[i] = std::sin(TWO_PI * 440.0f * t) * 
                               std::sin(TWO_PI * 2.0f * t);  // AM modulated sine
        }
    }
    
    void noteOn(uint8_t note, uint8_t velocity) override {
        pitch_ = std::pow(2.0f, (note - 60) / 12.0f);  // Relative to C4
    }
    
    void noteOff(uint8_t note) override {
        // Grains continue naturally
    }
    
    void setParameter(ParameterID param, float value) override {
        switch (param) {
            case ParameterID::HARMONICS:
                grain_size_ = 0.01f + value * 0.5f;  // 10ms to 500ms
                break;
            case ParameterID::TIMBRE:
                position_ = value;  // Position in sample
                break;
            case ParameterID::MORPH:
                grain_density_ = 1.0f + value * 50.0f;  // 1 to 50 grains/sec
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
            case ParameterID::HARMONICS: return (grain_size_ - 0.01f) / 0.49f;
            case ParameterID::TIMBRE: return position_;
            case ParameterID::MORPH: return (grain_density_ - 1.0f) / 49.0f;
            case ParameterID::VOLUME: return volume_;
            default: return 0.0f;
        }
    }
    
    void process(float* output, size_t frames) override {
        for (size_t i = 0; i < frames; i++) {
            // Trigger new grains based on density
            grain_accumulator_ += grain_density_ / SAMPLE_RATE;
            while (grain_accumulator_ >= 1.0f) {
                triggerGrain();
                grain_accumulator_ -= 1.0f;
            }
            
            // Process all active grains
            float sample = 0.0f;
            for (auto& grain : grains_) {
                if (grain.active) {
                    sample += processGrain(grain);
                }
            }
            
            output[i] = sample * volume_ * 0.5f;  // Scale down for multiple grains
        }
    }
    
    const char* getName() const override {
        return "Granular";
    }
    
private:
    void triggerGrain() {
        // Find inactive grain
        for (auto& grain : grains_) {
            if (!grain.active) {
                grain.active = true;
                grain.position = position_ * static_cast<float>(sample_buffer_.size());
                grain.position = std::clamp(grain.position, 0.0f, 
                                           static_cast<float>(sample_buffer_.size() - 1));
                grain.increment = pitch_;
                grain.length = static_cast<uint32_t>(grain_size_ * SAMPLE_RATE);
                grain.age = 0;
                grain.amplitude = 1.0f;
                break;
            }
        }
    }
    
    float processGrain(Grain& grain) {
        if (grain.age >= grain.length) {
            grain.active = false;
            return 0.0f;
        }
        
        // Hann window envelope
        float window_pos = static_cast<float>(grain.age) / static_cast<float>(grain.length);
        float envelope = 0.5f * (1.0f - std::cos(TWO_PI * window_pos));
        
        // Linear interpolation for sample playback
        size_t index = static_cast<size_t>(grain.position);
        float frac = grain.position - static_cast<float>(index);
        
        if (index >= sample_buffer_.size() - 1) {
            grain.active = false;
            return 0.0f;
        }
        
        float sample = sample_buffer_[index] * (1.0f - frac) + 
                      sample_buffer_[index + 1] * frac;
        
        grain.position += grain.increment;
        grain.age++;
        
        return sample * envelope * grain.amplitude;
    }
};

// Advanced Plaits-style macro oscillator
class PlaitsEngine : public ISynthEngine {
private:
    PlaitsModel current_model_ = PlaitsModel::VIRTUAL_ANALOG;
    
    // Core oscillators and components
    Oscillator main_osc_, aux_osc_, lfo_;
    ADSREnvelope envelope_;
    LowPassFilter filter_;
    
    // Plaits parameters (mapped from harmonics, timbre, morph)
    float harmonics_ = 0.5f;  // Model-specific parameter 1
    float timbre_ = 0.5f;     // Model-specific parameter 2  
    float morph_ = 0.5f;      // Model-specific parameter 3
    float volume_ = 0.5f;
    
    // Internal state
    bool note_active_ = false;
    float base_frequency_ = 440.0f;
    uint8_t current_note_ = 69;
    
    // Wavetable data
    std::vector<std::vector<float>> wavetables_;
    float wavetable_position_ = 0.0f;
    
    // Granular synthesis state
    struct PlaitsGrain {
        float position = 0.0f;
        float speed = 1.0f;
        float amplitude = 0.0f;
        uint32_t age = 0;
        uint32_t duration = 0;
        bool active = false;
    };
    static constexpr size_t MAX_PLAITS_GRAINS = 16;
    std::array<PlaitsGrain, MAX_PLAITS_GRAINS> grains_;
    float grain_trigger_phase_ = 0.0f;
    
    // Physical modeling state
    std::vector<float> string_delay_line_;
    size_t string_delay_length_ = 0;
    float string_feedback_ = 0.98f;
    float string_damping_ = 0.999f;
    
    // Speech/formant synthesis
    struct Formant {
        float frequency = 800.0f;
        float bandwidth = 50.0f;
        float amplitude = 1.0f;
        LowPassFilter filter;
    };
    std::array<Formant, 3> formants_;
    
    // Random number generation for various models
    std::mt19937 rng_{std::random_device{}()};
    std::uniform_real_distribution<float> noise_dist_{-1.0f, 1.0f};
    
public:
    PlaitsEngine() {
        // Initialize oscillators
        main_osc_.setWaveform(Oscillator::Waveform::SAW);
        aux_osc_.setWaveform(Oscillator::Waveform::SINE);
        lfo_.setFrequency(0.5f);
        
        // Initialize envelope
        envelope_.setAttack(0.01f);
        envelope_.setDecay(0.1f);
        envelope_.setSustain(0.7f);
        envelope_.setRelease(0.3f);
        
        // Initialize filter
        filter_.setCutoff(1000.0f);
        filter_.setResonance(1.0f);
        
        // Initialize wavetables
        initializeWavetables();
        
        // Initialize string delay line
        string_delay_line_.resize(static_cast<size_t>(SAMPLE_RATE * 0.1f), 0.0f); // 100ms max
        
        // Initialize formants (default vowel 'A')
        formants_[0].frequency = 730.0f;  // F1
        formants_[1].frequency = 1090.0f; // F2  
        formants_[2].frequency = 2440.0f; // F3
        for (auto& formant : formants_) {
            formant.filter.setCutoff(formant.frequency);
            formant.filter.setResonance(5.0f);
        }
    }
    
    void noteOn(uint8_t note, uint8_t velocity) override {
        current_note_ = note;
        base_frequency_ = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
        main_osc_.setFrequency(base_frequency_);
        envelope_.noteOn();
        note_active_ = true;
        
        // Model-specific initialization
        updateModelParameters();
        
        // Reset physical model state
        if (current_model_ == PlaitsModel::PHYSICAL_STRING) {
            initializeStringModel();
        }
    }
    
    void noteOff(uint8_t note) override {
        envelope_.noteOff();
        note_active_ = false;
    }
    
    void setParameter(ParameterID param, float value) override {
        switch (param) {
            case ParameterID::HARMONICS:
                harmonics_ = value;
                updateModelParameters();
                break;
            case ParameterID::TIMBRE:
                timbre_ = value;
                updateModelParameters();
                break;
            case ParameterID::MORPH:
                morph_ = value;
                updateModelParameters();
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
            case ParameterID::HARMONICS: return harmonics_;
            case ParameterID::TIMBRE: return timbre_;
            case ParameterID::MORPH: return morph_;
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
            
            float sample = processCurrentModel();
            float envelope_out = envelope_.process();
            
            output[i] = sample * envelope_out * volume_;
        }
    }
    
    const char* getName() const override {
        return "Plaits";
    }
    
    // Plaits-specific methods
    void setModel(PlaitsModel model) {
        current_model_ = model;
        updateModelParameters();
    }
    
    PlaitsModel getModel() const { return current_model_; }
    
    const char* getModelName() const {
        switch (current_model_) {
            case PlaitsModel::VIRTUAL_ANALOG: return "VirtAnalog";
            case PlaitsModel::WAVESHAPING: return "Waveshape";
            case PlaitsModel::FM_SYNTHESIS: return "FM";
            case PlaitsModel::GRANULAR_CLOUD: return "Granular";
            case PlaitsModel::ADDITIVE: return "Additive";
            case PlaitsModel::WAVETABLE: return "Wavetable";
            case PlaitsModel::PHYSICAL_STRING: return "PhysString";
            case PlaitsModel::SPEECH_FORMANT: return "Speech";
            default: return "Unknown";
        }
    }
    
private:
    float processCurrentModel() {
        switch (current_model_) {
            case PlaitsModel::VIRTUAL_ANALOG:
                return processVirtualAnalog();
            case PlaitsModel::WAVESHAPING:
                return processWaveshaping();
            case PlaitsModel::FM_SYNTHESIS:
                return processFM();
            case PlaitsModel::GRANULAR_CLOUD:
                return processGranular();
            case PlaitsModel::ADDITIVE:
                return processAdditive();
            case PlaitsModel::WAVETABLE:
                return processWavetable();
            case PlaitsModel::PHYSICAL_STRING:
                return processPhysicalString();
            case PlaitsModel::SPEECH_FORMANT:
                return processSpeechFormant();
            default:
                return processVirtualAnalog();
        }
    }
    
    float processVirtualAnalog() {
        // Virtual analog: harmonics controls waveshape, timbre controls filter, morph controls waveform blend
        float osc_out = main_osc_.process();
        
        // Waveshaping based on harmonics
        float shape_amount = harmonics_ * 3.0f;
        osc_out = std::tanh(osc_out * (1.0f + shape_amount));
        
        // Filter modulation based on timbre
        float cutoff = 200.0f + timbre_ * 8000.0f;
        filter_.setCutoff(cutoff);
        
        return filter_.process(osc_out);
    }
    
    float processWaveshaping() {
        // Complex waveshaping: harmonics controls distortion, timbre controls asymmetry, morph controls fold
        float osc_out = main_osc_.process();
        
        // Primary distortion
        float drive = 1.0f + harmonics_ * 10.0f;
        osc_out *= drive;
        
        // Asymmetric shaping based on timbre
        float asymmetry = (timbre_ - 0.5f) * 2.0f;
        if (osc_out > 0.0f) {
            osc_out = std::pow(osc_out, 1.0f + asymmetry * 0.5f);
        } else {
            osc_out = -std::pow(-osc_out, 1.0f - asymmetry * 0.5f);
        }
        
        // Wave folding based on morph
        float fold_amount = morph_ * 4.0f;
        osc_out = std::sin(osc_out * fold_amount) / (1.0f + fold_amount * 0.1f);
        
        return std::tanh(osc_out);
    }
    
    float processFM() {
        // 2-operator FM: harmonics controls ratio, timbre controls index, morph controls feedback
        float fm_ratio = 0.5f + harmonics_ * 8.0f;
        float fm_index = timbre_ * 12.0f;
        float feedback = morph_ * 0.5f;
        
        aux_osc_.setFrequency(base_frequency_ * fm_ratio);
        
        static float feedback_sample = 0.0f;
        float modulator = aux_osc_.process() + feedback_sample * feedback;
        feedback_sample = modulator;
        
        float carrier_freq_mod = base_frequency_ + modulator * fm_index * base_frequency_;
        main_osc_.setFrequency(carrier_freq_mod);
        
        return main_osc_.process();
    }
    
    float processGranular() {
        // Granular synthesis: harmonics controls grain size, timbre controls density, morph controls pitch spread
        float grain_size = 0.01f + harmonics_ * 0.2f; // 10ms to 200ms
        float grain_density = 5.0f + timbre_ * 45.0f; // 5 to 50 grains/sec
        float pitch_spread = morph_ * 2.0f; // ±2 semitones
        
        // Trigger new grains
        grain_trigger_phase_ += grain_density / SAMPLE_RATE;
        if (grain_trigger_phase_ >= 1.0f) {
            grain_trigger_phase_ -= 1.0f;
            triggerPlaitsGrain(grain_size, pitch_spread);
        }
        
        // Process active grains
        float output = 0.0f;
        for (auto& grain : grains_) {
            if (grain.active) {
                output += processPlaitsGrain(grain);
            }
        }
        
        return output * 0.3f; // Scale down for multiple grains
    }
    
    float processAdditive() {
        // Additive synthesis: harmonics controls harmonic content, timbre controls spectral tilt, morph controls odd/even balance
        float fundamental = std::sin(TWO_PI * base_frequency_ * wavetable_position_ / SAMPLE_RATE);
        wavetable_position_ += 1.0f;
        if (wavetable_position_ >= SAMPLE_RATE) wavetable_position_ -= SAMPLE_RATE;
        
        float output = fundamental;
        
        // Add harmonics
        int max_harmonics = static_cast<int>(2.0f + harmonics_ * 30.0f);
        for (int h = 2; h <= max_harmonics; h++) {
            float harmonic_freq = base_frequency_ * h;
            if (harmonic_freq > SAMPLE_RATE * 0.45f) break;
            
            // Spectral tilt
            float amplitude = 1.0f / (h * (1.0f + timbre_ * 5.0f));
            
            // Odd/even balance
            if (h % 2 == 0) { // Even harmonics
                amplitude *= (1.0f - morph_);
            } else { // Odd harmonics  
                amplitude *= morph_;
            }
            
            float harmonic = std::sin(TWO_PI * harmonic_freq * wavetable_position_ / SAMPLE_RATE);
            output += harmonic * amplitude;
        }
        
        return output * 0.3f;
    }
    
    float processWavetable() {
        // Wavetable synthesis: harmonics selects wavetable, timbre controls position, morph controls interpolation
        if (wavetables_.empty()) return 0.0f;
        
        size_t table_index = static_cast<size_t>(harmonics_ * (wavetables_.size() - 1));
        float position = timbre_ * (wavetables_[table_index].size() - 1);
        
        size_t pos_int = static_cast<size_t>(position);
        float pos_frac = position - pos_int;
        
        if (pos_int >= wavetables_[table_index].size() - 1) {
            return wavetables_[table_index].back();
        }
        
        // Linear interpolation
        float sample1 = wavetables_[table_index][pos_int];
        float sample2 = wavetables_[table_index][pos_int + 1];
        
        return sample1 * (1.0f - pos_frac) + sample2 * pos_frac;
    }
    
    float processPhysicalString() {
        // Karplus-Strong string synthesis: harmonics controls stiffness, timbre controls damping, morph controls excitation
        if (string_delay_line_.empty()) return 0.0f;
        
        // Excitation (noise burst on note trigger)
        float excitation = 0.0f;
        if (note_active_ && envelope_.process() > 0.95f) { // During attack
            excitation = noise_dist_(rng_) * morph_;
        }
        
        // Delay line feedback with filtering
        float delayed = string_delay_line_[0];
        
        // All-pass filter for stiffness (harmonics)
        float stiffness = harmonics_;
        delayed = delayed * (1.0f - stiffness) + string_delay_line_[1] * stiffness;
        
        // Damping (timbre)
        delayed *= string_damping_ * (1.0f - timbre_ * 0.1f);
        
        // Feed back into delay line
        for (size_t i = 0; i < string_delay_line_.size() - 1; i++) {
            string_delay_line_[i] = string_delay_line_[i + 1];
        }
        string_delay_line_.back() = delayed + excitation;
        
        return delayed;
    }
    
    float processSpeechFormant() {
        // Speech/formant synthesis: harmonics controls vowel, timbre controls consonant, morph controls nasality
        float excitation = noise_dist_(rng_) * 0.1f; // Noise excitation
        
        // Update formant frequencies based on harmonics (vowel selection)
        updateFormantFrequencies();
        
        // Process through formant filters
        float output = 0.0f;
        for (auto& formant : formants_) {
            float filtered = formant.filter.process(excitation);
            output += filtered * formant.amplitude;
        }
        
        // Apply consonant effects (timbre)
        if (timbre_ > 0.5f) {
            output = std::tanh(output * (1.0f + (timbre_ - 0.5f) * 4.0f));
        }
        
        return output * 0.3f;
    }
    
    void updateModelParameters() {
        // Update model-specific parameters when harmonics/timbre/morph change
        switch (current_model_) {
            case PlaitsModel::PHYSICAL_STRING:
                if (note_active_) {
                    initializeStringModel();
                }
                break;
            case PlaitsModel::SPEECH_FORMANT:
                updateFormantFrequencies();
                break;
            default:
                break;
        }
    }
    
    void initializeWavetables() {
        // Create basic wavetables
        wavetables_.resize(8);
        
        for (size_t table = 0; table < wavetables_.size(); table++) {
            wavetables_[table].resize(512);
            
            for (size_t i = 0; i < wavetables_[table].size(); i++) {
                float phase = TWO_PI * i / wavetables_[table].size();
                
                switch (table) {
                    case 0: // Sine
                        wavetables_[table][i] = std::sin(phase);
                        break;
                    case 1: // Triangle
                        wavetables_[table][i] = (phase < M_PI) ? 
                            (2.0f * phase / M_PI - 1.0f) : (1.0f - 2.0f * (phase - M_PI) / M_PI);
                        break;
                    case 2: // Sawtooth
                        wavetables_[table][i] = 2.0f * phase / TWO_PI - 1.0f;
                        break;
                    case 3: // Square
                        wavetables_[table][i] = (phase < M_PI) ? 1.0f : -1.0f;
                        break;
                    case 4: // Complex 1 (multiple harmonics)
                        wavetables_[table][i] = std::sin(phase) + 0.3f * std::sin(3.0f * phase) + 0.1f * std::sin(5.0f * phase);
                        break;
                    case 5: // Complex 2 (even harmonics)
                        wavetables_[table][i] = std::sin(phase) + 0.5f * std::sin(2.0f * phase) + 0.2f * std::sin(4.0f * phase);
                        break;
                    case 6: // Noise-like
                        wavetables_[table][i] = noise_dist_(rng_);
                        break;
                    case 7: // Formant-like
                        wavetables_[table][i] = std::sin(phase) * std::exp(-3.0f * phase / TWO_PI);
                        break;
                }
            }
        }
    }
    
    void initializeStringModel() {
        // Initialize delay line length based on frequency
        string_delay_length_ = static_cast<size_t>(SAMPLE_RATE / base_frequency_);
        string_delay_length_ = std::min(string_delay_length_, string_delay_line_.size());
        
        // Clear delay line and add initial excitation
        std::fill(string_delay_line_.begin(), string_delay_line_.end(), 0.0f);
        for (size_t i = 0; i < string_delay_length_; i++) {
            string_delay_line_[i] = noise_dist_(rng_) * 0.5f;
        }
    }
    
    void triggerPlaitsGrain(float grain_size, float pitch_spread) {
        // Find inactive grain
        for (auto& grain : grains_) {
            if (!grain.active) {
                grain.active = true;
                grain.position = 0.0f;
                
                // Random pitch variation
                float pitch_variation = (noise_dist_(rng_) * pitch_spread);
                grain.speed = std::pow(2.0f, pitch_variation / 12.0f);
                
                grain.duration = static_cast<uint32_t>(grain_size * SAMPLE_RATE);
                grain.age = 0;
                grain.amplitude = 0.5f + noise_dist_(rng_) * 0.2f;
                break;
            }
        }
    }
    
    float processPlaitsGrain(PlaitsGrain& grain) {
        if (grain.age >= grain.duration) {
            grain.active = false;
            return 0.0f;
        }
        
        // Hann window envelope
        float window_pos = static_cast<float>(grain.age) / static_cast<float>(grain.duration);
        float envelope = 0.5f * (1.0f - std::cos(TWO_PI * window_pos));
        
        // Simple oscillator for grain content
        float phase = grain.position * TWO_PI;
        float sample = std::sin(phase);
        
        grain.position += base_frequency_ * grain.speed / SAMPLE_RATE;
        if (grain.position >= 1.0f) grain.position -= 1.0f;
        
        grain.age++;
        
        return sample * envelope * grain.amplitude;
    }
    
    void updateFormantFrequencies() {
        // Vowel interpolation based on harmonics parameter
        float vowel_pos = harmonics_;
        
        // Define formant frequencies for different vowels
        struct VowelFormants {
            float f1, f2, f3;
        };
        
        const VowelFormants vowels[5] = {
            {730, 1090, 2440},  // A
            {270, 2290, 3010},  // I  
            {300, 870, 2240},   // U
            {530, 1840, 2480},  // E
            {570, 840, 2410}    // O
        };
        
        // Interpolate between vowels
        int vowel_index = static_cast<int>(vowel_pos * 4.0f);
        float frac = vowel_pos * 4.0f - vowel_index;
        
        if (vowel_index >= 4) {
            vowel_index = 4;
            frac = 0.0f;
        }
        
        const VowelFormants& v1 = vowels[vowel_index];
        const VowelFormants& v2 = vowels[std::min(vowel_index + 1, 4)];
        
        formants_[0].frequency = v1.f1 * (1.0f - frac) + v2.f1 * frac;
        formants_[1].frequency = v1.f2 * (1.0f - frac) + v2.f2 * frac;
        formants_[2].frequency = v1.f3 * (1.0f - frac) + v2.f3 * frac;
        
        // Update filter cutoffs
        for (auto& formant : formants_) {
            formant.filter.setCutoff(formant.frequency);
        }
    }
};

// Euclidean rhythm generator
class EuclideanRhythm {
private:
    size_t steps_ = 16;
    size_t hits_ = 4;
    size_t rotation_ = 0;
    std::vector<bool> pattern_;
    size_t current_step_ = 0;
    bool active_ = false;
    
    void generatePattern() {
        pattern_.clear();
        pattern_.resize(steps_, false);
        
        if (hits_ == 0 || hits_ > steps_) return;
        
        // Euclidean algorithm for rhythm generation
        std::vector<int> bucket(steps_, 0);
        for (size_t i = 0; i < steps_; i++) {
            bucket[i] = (i * hits_) / steps_;
        }
        
        for (size_t i = 0; i < steps_; i++) {
            if (i == 0 || bucket[i] != bucket[i-1]) {
                pattern_[i] = true;
            }
        }
        
        // Apply rotation
        if (rotation_ > 0) {
            std::vector<bool> rotated(steps_);
            for (size_t i = 0; i < steps_; i++) {
                rotated[(i + rotation_) % steps_] = pattern_[i];
            }
            pattern_ = rotated;
        }
    }
    
public:
    EuclideanRhythm() {
        generatePattern();
    }
    
    void setPattern(size_t hits, size_t rotation) {
        hits_ = std::clamp(hits, size_t(0), steps_);
        rotation_ = rotation % steps_;
        generatePattern();
    }
    
    void setSteps(size_t steps) {
        steps_ = std::clamp(steps, size_t(1), size_t(32));
        generatePattern();
    }
    
    bool step() {
        if (!active_) return false;
        
        bool hit = pattern_[current_step_];
        current_step_ = (current_step_ + 1) % steps_;
        return hit;
    }
    
    void reset() {
        current_step_ = 0;
    }
    
    void setActive(bool active) {
        active_ = active;
        if (active) reset();
    }
    
    bool isActive() const { return active_; }
    size_t getHits() const { return hits_; }
    size_t getSteps() const { return steps_; }
    size_t getRotation() const { return rotation_; }
    size_t getCurrentStep() const { return current_step_; }
    
    std::string getPatternString() const {
        std::string pattern;
        for (size_t i = 0; i < steps_; i++) {
            if (i == current_step_ && active_) {
                pattern += pattern_[i] ? "◉" : "◯";
            } else {
                pattern += pattern_[i] ? "●" : "○";
            }
        }
        return pattern;
    }
};

// Arpeggiator with multiple patterns and timing control
class Arpeggiator {
public:
    enum class Pattern {
        UP,
        DOWN,
        UP_DOWN,
        DOWN_UP,
        RANDOM,
        AS_PLAYED,
        COUNT
    };
    
    enum class Speed {
        WHOLE = 4,      // 1/1 notes
        HALF = 2,       // 1/2 notes  
        QUARTER = 1,    // 1/4 notes
        EIGHTH = 0,     // 1/8 notes (default)
        SIXTEENTH = -1, // 1/16 notes
        TRIPLET = -2,   // 1/8 triplets
        COUNT = 6
    };
    
private:
    bool active_ = false;
    Pattern pattern_ = Pattern::UP;
    Speed speed_ = Speed::EIGHTH;
    std::vector<uint8_t> held_notes_;
    std::vector<uint8_t> arp_sequence_;
    size_t current_step_ = 0;
    float bpm_ = 120.0f;
    
    std::chrono::steady_clock::time_point last_step_time_;
    bool direction_up_ = true;  // For UP_DOWN patterns
    
    // Random number generation
    std::mt19937 rng_{std::random_device{}()};
    
    void generateSequence() {
        arp_sequence_.clear();
        if (held_notes_.empty()) return;
        
        std::vector<uint8_t> sorted_notes = held_notes_;
        std::sort(sorted_notes.begin(), sorted_notes.end());
        
        switch (pattern_) {
            case Pattern::UP:
                arp_sequence_ = sorted_notes;
                break;
                
            case Pattern::DOWN:
                arp_sequence_ = sorted_notes;
                std::reverse(arp_sequence_.begin(), arp_sequence_.end());
                break;
                
            case Pattern::UP_DOWN:
                arp_sequence_ = sorted_notes;
                // Add descending part (excluding first and last to avoid repetition)
                for (int i = static_cast<int>(sorted_notes.size()) - 2; i > 0; i--) {
                    arp_sequence_.push_back(sorted_notes[i]);
                }
                break;
                
            case Pattern::DOWN_UP:
                arp_sequence_ = sorted_notes;
                std::reverse(arp_sequence_.begin(), arp_sequence_.end());
                // Add ascending part (excluding first and last)
                for (size_t i = sorted_notes.size() - 2; i > 0; i--) {
                    arp_sequence_.push_back(sorted_notes[i]);
                }
                break;
                
            case Pattern::RANDOM:
                arp_sequence_ = sorted_notes;
                std::shuffle(arp_sequence_.begin(), arp_sequence_.end(), rng_);
                break;
                
            case Pattern::AS_PLAYED:
                arp_sequence_ = held_notes_;  // Keep original order
                break;
                
            case Pattern::COUNT:
                break;
        }
        
        current_step_ = 0;
    }
    
    float getStepDurationMs() const {
        float base_duration = (60.0f / bpm_) * 1000.0f; // Quarter note in ms
        
        switch (speed_) {
            case Speed::WHOLE: return base_duration * 4.0f;
            case Speed::HALF: return base_duration * 2.0f;
            case Speed::QUARTER: return base_duration;
            case Speed::EIGHTH: return base_duration * 0.5f;
            case Speed::SIXTEENTH: return base_duration * 0.25f;
            case Speed::TRIPLET: return base_duration * 0.333f;
            default: return base_duration * 0.5f;
        }
    }
    
public:
    Arpeggiator() {
        last_step_time_ = std::chrono::steady_clock::now();
    }
    
    void setActive(bool active) {
        active_ = active;
        if (active) {
            last_step_time_ = std::chrono::steady_clock::now();
            current_step_ = 0;
        }
    }
    
    bool isActive() const { return active_; }
    
    void noteOn(uint8_t note) {
        // Add note if not already held
        auto it = std::find(held_notes_.begin(), held_notes_.end(), note);
        if (it == held_notes_.end()) {
            held_notes_.push_back(note);
            generateSequence();
        }
    }
    
    void noteOff(uint8_t note) {
        auto it = std::find(held_notes_.begin(), held_notes_.end(), note);
        if (it != held_notes_.end()) {
            held_notes_.erase(it);
            generateSequence();
        }
    }
    
    void allNotesOff() {
        held_notes_.clear();
        arp_sequence_.clear();
        current_step_ = 0;
    }
    
    // Returns the next note to play, or 0 if no step should happen
    uint8_t step() {
        if (!active_ || arp_sequence_.empty()) return 0;
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_step_time_);
        
        if (elapsed.count() >= getStepDurationMs()) {
            uint8_t note = arp_sequence_[current_step_];
            current_step_ = (current_step_ + 1) % arp_sequence_.size();
            last_step_time_ = now;
            return note;
        }
        
        return 0;  // No step yet
    }
    
    void setPattern(Pattern pattern) {
        pattern_ = pattern;
        generateSequence();
    }
    
    void setSpeed(Speed speed) {
        speed_ = speed;
    }
    
    void setBPM(float bpm) {
        bpm_ = std::clamp(bpm, 60.0f, 200.0f);
    }
    
    Pattern getPattern() const { return pattern_; }
    Speed getSpeed() const { return speed_; }
    float getBPM() const { return bpm_; }
    size_t getHeldNotesCount() const { return held_notes_.size(); }
    size_t getCurrentStep() const { return current_step_; }
    size_t getSequenceLength() const { return arp_sequence_.size(); }
    
    const char* getPatternName() const {
        switch (pattern_) {
            case Pattern::UP: return "Up";
            case Pattern::DOWN: return "Down"; 
            case Pattern::UP_DOWN: return "Up-Down";
            case Pattern::DOWN_UP: return "Down-Up";
            case Pattern::RANDOM: return "Random";
            case Pattern::AS_PLAYED: return "As Played";
            default: return "Unknown";
        }
    }
    
    const char* getSpeedName() const {
        switch (speed_) {
            case Speed::WHOLE: return "1/1";
            case Speed::HALF: return "1/2";
            case Speed::QUARTER: return "1/4";
            case Speed::EIGHTH: return "1/8";
            case Speed::SIXTEENTH: return "1/16";
            case Speed::TRIPLET: return "1/8T";
            default: return "1/8";
        }
    }
    
    std::string getSequenceVisualization() const {
        if (arp_sequence_.empty()) return "[]";
        
        std::string viz = "[";
        for (size_t i = 0; i < arp_sequence_.size(); i++) {
            if (i == current_step_) viz += ">";
            viz += std::to_string(arp_sequence_[i] % 12);  // Show note within octave
            if (i == current_step_) viz += "<";
            if (i < arp_sequence_.size() - 1) viz += " ";
        }
        viz += "]";
        return viz;
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

// Drum synthesis engine
class DrumSynth {
private:
    struct DrumVoice {
        DrumType type;
        float phase = 0.0f;
        float envelope = 0.0f;
        float decay_rate = 0.0001f;
        bool active = false;
        
        // Specific parameters per drum type
        float pitch = 1.0f;
        float noise_level = 0.0f;
        float filter_freq = 1000.0f;
        float fm_amount = 0.0f;
    };
    
    static constexpr size_t MAX_DRUM_VOICES = 16;
    std::array<DrumVoice, MAX_DRUM_VOICES> drum_voices_;
    
    // Noise generator
    std::mt19937 rng_{std::random_device{}()};
    std::uniform_real_distribution<float> noise_dist_{-1.0f, 1.0f};
    
    // Simple filters for each voice
    std::array<LowPassFilter, MAX_DRUM_VOICES> filters_;
    
public:
    DrumSynth() {
        // Initialize all drum voices as inactive
        for (auto& voice : drum_voices_) {
            voice.active = false;
        }
        
        // Initialize filters
        for (auto& filter : filters_) {
            filter.setCutoff(1000.0f);
            filter.setResonance(1.0f);
        }
    }
    
    void triggerDrum(DrumType type, float velocity = 1.0f) {
        // Find free voice or steal oldest
        DrumVoice* free_voice = nullptr;
        for (auto& voice : drum_voices_) {
            if (!voice.active) {
                free_voice = &voice;
                break;
            }
        }
        
        if (!free_voice) {
            free_voice = &drum_voices_[0]; // Steal first voice
        }
        
        // Configure voice based on drum type
        free_voice->type = type;
        free_voice->phase = 0.0f;
        free_voice->envelope = velocity;
        free_voice->active = true;
        
        configureDrumVoice(*free_voice, type);
    }
    
    void process(float* output, size_t frames) {
        for (size_t i = 0; i < frames; i++) {
            float drum_mix = 0.0f;
            
            for (size_t v = 0; v < drum_voices_.size(); v++) {
                DrumVoice& voice = drum_voices_[v];
                if (voice.active) {
                    float sample = processDrumVoice(voice);
                    sample = filters_[v].process(sample);
                    drum_mix += sample * voice.envelope;
                    
                    // Update envelope
                    voice.envelope *= voice.decay_rate;
                    if (voice.envelope < 0.001f) {
                        voice.active = false;
                    }
                }
            }
            
            output[i] += drum_mix * 0.5f; // Scale down drums
        }
    }
    
    const char* getDrumName(DrumType type) const {
        switch (type) {
            case DrumType::KICK: return "Kick";
            case DrumType::SNARE: return "Snare";
            case DrumType::HIHAT_CLOSED: return "HH Closed";
            case DrumType::HIHAT_OPEN: return "HH Open";
            case DrumType::CLAP: return "Clap";
            case DrumType::CRASH: return "Crash";
            case DrumType::TOM_HIGH: return "Tom Hi";
            case DrumType::TOM_LOW: return "Tom Lo";
            default: return "Unknown";
        }
    }
    
private:
    void configureDrumVoice(DrumVoice& voice, DrumType type) {
        switch (type) {
            case DrumType::KICK:
                voice.pitch = 1.0f;
                voice.noise_level = 0.1f;
                voice.filter_freq = 150.0f;
                voice.fm_amount = 8.0f;
                voice.decay_rate = 0.9992f; // Long decay
                break;
                
            case DrumType::SNARE:
                voice.pitch = 3.0f;
                voice.noise_level = 0.8f;
                voice.filter_freq = 800.0f;
                voice.fm_amount = 2.0f;
                voice.decay_rate = 0.9985f; // Medium decay
                break;
                
            case DrumType::HIHAT_CLOSED:
                voice.pitch = 10.0f;
                voice.noise_level = 1.0f;
                voice.filter_freq = 8000.0f;
                voice.fm_amount = 0.0f;
                voice.decay_rate = 0.9970f; // Fast decay
                break;
                
            case DrumType::HIHAT_OPEN:
                voice.pitch = 10.0f;
                voice.noise_level = 1.0f;
                voice.filter_freq = 6000.0f;
                voice.fm_amount = 0.0f;
                voice.decay_rate = 0.9990f; // Slower decay
                break;
                
            case DrumType::CLAP:
                voice.pitch = 5.0f;
                voice.noise_level = 0.9f;
                voice.filter_freq = 1200.0f;
                voice.fm_amount = 1.0f;
                voice.decay_rate = 0.9980f; // Medium-fast decay
                break;
                
            case DrumType::CRASH:
                voice.pitch = 8.0f;
                voice.noise_level = 0.7f;
                voice.filter_freq = 4000.0f;
                voice.fm_amount = 0.5f;
                voice.decay_rate = 0.9998f; // Very long decay
                break;
                
            case DrumType::TOM_HIGH:
                voice.pitch = 2.5f;
                voice.noise_level = 0.2f;
                voice.filter_freq = 600.0f;
                voice.fm_amount = 3.0f;
                voice.decay_rate = 0.9988f; // Medium decay
                break;
                
            case DrumType::TOM_LOW:
                voice.pitch = 1.5f;
                voice.noise_level = 0.2f;
                voice.filter_freq = 400.0f;
                voice.fm_amount = 3.0f;
                voice.decay_rate = 0.9990f; // Medium-long decay
                break;
                
            case DrumType::COUNT:
                break;
        }
        
        // Update filter cutoff
        size_t voice_index = &voice - &drum_voices_[0];
        filters_[voice_index].setCutoff(voice.filter_freq);
    }
    
    float processDrumVoice(DrumVoice& voice) {
        float sample = 0.0f;
        
        // Base frequency (varies by drum type)
        float base_freq = 80.0f * voice.pitch;
        
        // Oscillator component
        float osc_sample = std::sin(TWO_PI * base_freq * voice.phase / SAMPLE_RATE);
        
        // FM modulation for some drums
        if (voice.fm_amount > 0.0f) {
            float mod_freq = base_freq * 2.0f;
            float modulator = std::sin(TWO_PI * mod_freq * voice.phase / SAMPLE_RATE);
            osc_sample = std::sin(TWO_PI * base_freq * voice.phase / SAMPLE_RATE + 
                                modulator * voice.fm_amount * voice.envelope);
        }
        
        // Noise component
        float noise_sample = noise_dist_(rng_);
        
        // Mix oscillator and noise based on drum type
        sample = osc_sample * (1.0f - voice.noise_level) + noise_sample * voice.noise_level;
        
        // Special processing for specific drums
        switch (voice.type) {
            case DrumType::KICK:
                // Pitch sweep for kick
                sample *= (1.0f + voice.envelope * 2.0f);
                break;
                
            case DrumType::CLAP:
                // Multiple hits for clap
                if (voice.envelope > 0.7f || (voice.envelope > 0.4f && voice.envelope < 0.5f) || 
                    (voice.envelope > 0.2f && voice.envelope < 0.25f)) {
                    sample *= 1.5f;
                }
                break;
                
            default:
                break;
        }
        
        voice.phase += 1.0f;
        if (voice.phase >= SAMPLE_RATE) voice.phase -= SAMPLE_RATE;
        
        return sample;
    }
};

// Step sequencer for drum machine
class StepSequencer {
private:
    struct DrumTrack {
        DrumType drum_type;
        std::array<bool, MAX_DRUM_STEPS> steps;
        bool muted = false;
        float velocity = 1.0f;
        
        DrumTrack(DrumType type) : drum_type(type) {
            steps.fill(false);
        }
    };
    
    std::array<DrumTrack, MAX_DRUM_TRACKS> tracks_;
    size_t current_step_ = 0;
    bool playing_ = false;
    float bpm_ = 120.0f;
    
    std::chrono::steady_clock::time_point last_step_time_;
    size_t selected_track_ = 0;
    
public:
    StepSequencer() : tracks_({
        DrumTrack(DrumType::KICK),
        DrumTrack(DrumType::SNARE),
        DrumTrack(DrumType::HIHAT_CLOSED),
        DrumTrack(DrumType::HIHAT_OPEN),
        DrumTrack(DrumType::CLAP),
        DrumTrack(DrumType::CRASH),
        DrumTrack(DrumType::TOM_HIGH),
        DrumTrack(DrumType::TOM_LOW)
    }) {
        last_step_time_ = std::chrono::steady_clock::now();
        
        // Set up a basic pattern
        tracks_[0].steps[0] = true;  // Kick on 1
        tracks_[0].steps[4] = true;  // Kick on 5
        tracks_[0].steps[8] = true;  // Kick on 9
        tracks_[0].steps[12] = true; // Kick on 13
        
        tracks_[1].steps[4] = true;  // Snare on 5
        tracks_[1].steps[12] = true; // Snare on 13
        
        tracks_[2].steps[2] = true;  // Hi-hat pattern
        tracks_[2].steps[6] = true;
        tracks_[2].steps[10] = true;
        tracks_[2].steps[14] = true;
    }
    
    void setPlaying(bool playing) {
        playing_ = playing;
        if (playing) {
            last_step_time_ = std::chrono::steady_clock::now();
        }
    }
    
    bool isPlaying() const { return playing_; }
    
    void setBPM(float bpm) {
        bpm_ = std::clamp(bpm, 60.0f, 200.0f);
    }
    
    float getBPM() const { return bpm_; }
    
    // Returns list of drums to trigger on this step
    std::vector<std::pair<DrumType, float>> step(DrumSynth& drum_synth) {
        std::vector<std::pair<DrumType, float>> triggers;
        
        if (!playing_) return triggers;
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_step_time_);
        
        // Calculate step duration (16th notes)
        float step_duration_ms = (60.0f / bpm_) * 1000.0f / 4.0f;
        
        if (elapsed.count() >= step_duration_ms) {
            // Process current step
            for (const auto& track : tracks_) {
                if (!track.muted && track.steps[current_step_]) {
                    triggers.emplace_back(track.drum_type, track.velocity);
                }
            }
            
            // Advance step
            current_step_ = (current_step_ + 1) % MAX_DRUM_STEPS;
            last_step_time_ = now;
        }
        
        return triggers;
    }
    
    void toggleStep(size_t track, size_t step) {
        if (track < tracks_.size() && step < MAX_DRUM_STEPS) {
            tracks_[track].steps[step] = !tracks_[track].steps[step];
        }
    }
    
    bool getStep(size_t track, size_t step) const {
        if (track < tracks_.size() && step < MAX_DRUM_STEPS) {
            return tracks_[track].steps[step];
        }
        return false;
    }
    
    void selectTrack(size_t track) {
        if (track < tracks_.size()) {
            selected_track_ = track;
        }
    }
    
    size_t getSelectedTrack() const { return selected_track_; }
    size_t getCurrentStep() const { return current_step_; }
    
    void toggleMute(size_t track) {
        if (track < tracks_.size()) {
            tracks_[track].muted = !tracks_[track].muted;
        }
    }
    
    bool isMuted(size_t track) const {
        if (track < tracks_.size()) {
            return tracks_[track].muted;
        }
        return false;
    }
    
    DrumType getTrackDrumType(size_t track) const {
        if (track < tracks_.size()) {
            return tracks_[track].drum_type;
        }
        return DrumType::KICK;
    }
    
    void clear() {
        for (auto& track : tracks_) {
            track.steps.fill(false);
        }
    }
    
    void clearTrack(size_t track) {
        if (track < tracks_.size()) {
            tracks_[track].steps.fill(false);
        }
    }
    
    std::string getPatternVisualization() const {
        std::string pattern;
        for (size_t step = 0; step < MAX_DRUM_STEPS; step++) {
            if (step == current_step_ && playing_) {
                pattern += "▶";
            } else {
                pattern += "•";
            }
        }
        return pattern;
    }
};

// Advanced reverb engine inspired by Mutable Instruments
class LushReverb {
public:
    enum class Algorithm {
        HALL,           // Large hall reverb
        ROOM,           // Intimate room reverb  
        PLATE,          // Vintage plate reverb
        SPRING,         // Spring tank reverb
        SHIMMER,        // Shimmer with pitch shifting
        CLOUDS,         // Clouds-style granular reverb
        CHORUS,         // Chorus-reverb hybrid
        DELAY,          // Multi-tap delay reverb
        COUNT
    };
    
private:
    Algorithm current_algorithm_ = Algorithm::HALL;
    
    // Core parameters
    float size_ = 0.8f;          // Room size / decay time
    float damping_ = 0.6f;       // High frequency damping
    float diffusion_ = 0.7f;     // Early reflection density
    float mix_ = 0.3f;           // Dry/wet mix
    float feedback_ = 0.85f;     // Overall feedback amount
    float modulation_ = 0.1f;    // Modulation depth
    
    // Delay line lengths (in samples) - carefully tuned prime numbers
    static constexpr size_t DELAY_LENGTHS[12] = {
        1051, 1153, 1277, 1361, 1439, 1523, 1607, 1693, 1787, 1871, 1949, 2039
    };
    
    // Multiple delay lines for rich reverb texture
    std::array<std::vector<float>, 12> delay_lines_;
    std::array<size_t, 12> delay_indices_;
    
    // All-pass filters for diffusion
    struct AllPassFilter {
        std::vector<float> buffer;
        size_t index = 0;
        float feedback = 0.7f;
        
        AllPassFilter(size_t length) : buffer(length, 0.0f) {}
        
        float process(float input) {
            float delayed = buffer[index];
            float output = -input + delayed;
            buffer[index] = input + delayed * feedback;
            index = (index + 1) % buffer.size();
            return output;
        }
    };
    
    std::array<AllPassFilter, 4> allpass_filters_;
    
    // Low-pass filters for damping
    std::array<LowPassFilter, 6> damping_filters_;
    
    // Modulation LFOs
    std::array<float, 4> lfo_phases_;
    std::array<float, 4> lfo_frequencies_;
    
    // Pitch shifter for shimmer effect
    struct PitchShifter {
        std::vector<float> buffer;
        size_t write_index = 0;
        float read_index = 0.0f;
        float shift_ratio = 1.0f;
        
        PitchShifter() : buffer(8192, 0.0f) {}
        
        float process(float input) {
            buffer[write_index] = input;
            
            // Linear interpolation for fractional delay
            size_t read_int = static_cast<size_t>(read_index);
            float read_frac = read_index - read_int;
            
            size_t read_int2 = (read_int + 1) % buffer.size();
            float output = buffer[read_int] * (1.0f - read_frac) + buffer[read_int2] * read_frac;
            
            write_index = (write_index + 1) % buffer.size();
            read_index += shift_ratio;
            if (read_index >= buffer.size()) read_index -= buffer.size();
            
            return output;
        }
        
        void setShift(float semitones) {
            shift_ratio = std::pow(2.0f, semitones / 12.0f);
        }
    };
    
    PitchShifter pitch_shifter_up_, pitch_shifter_down_;
    
public:
    LushReverb() : allpass_filters_({
        AllPassFilter(347), AllPassFilter(443), AllPassFilter(521), AllPassFilter(631)
    }) {
        // Initialize delay lines
        for (size_t i = 0; i < delay_lines_.size(); i++) {
            delay_lines_[i].resize(DELAY_LENGTHS[i], 0.0f);
            delay_indices_[i] = 0;
        }
        
        // Initialize damping filters
        for (auto& filter : damping_filters_) {
            filter.setCutoff(5000.0f);
            filter.setResonance(0.7f);
        }
        
        // Initialize LFO phases and frequencies
        lfo_frequencies_[0] = 0.13f;
        lfo_frequencies_[1] = 0.17f;
        lfo_frequencies_[2] = 0.23f;
        lfo_frequencies_[3] = 0.29f;
        
        for (auto& phase : lfo_phases_) {
            phase = 0.0f;
        }
        
        // Initialize pitch shifters
        pitch_shifter_up_.setShift(12.0f);   // +1 octave
        pitch_shifter_down_.setShift(-12.0f); // -1 octave
    }
    
    void setAlgorithm(Algorithm algorithm) {
        current_algorithm_ = algorithm;
        updateAlgorithmParameters();
    }
    
    Algorithm getAlgorithm() const { return current_algorithm_; }
    Algorithm getCurrentAlgorithm() const { return current_algorithm_; }
    
    void setSize(float size) {
        size_ = std::clamp(size, 0.0f, 1.0f);
        updateAlgorithmParameters();
    }
    
    void setDamping(float damping) {
        damping_ = std::clamp(damping, 0.0f, 1.0f);
        // Update damping filter cutoffs
        float cutoff = 1000.0f + (1.0f - damping_) * 8000.0f;
        for (auto& filter : damping_filters_) {
            filter.setCutoff(cutoff);
        }
    }
    
    void setDiffusion(float diffusion) {
        diffusion_ = std::clamp(diffusion, 0.0f, 1.0f);
        // Update allpass feedback
        float feedback = 0.5f + diffusion_ * 0.3f;
        for (auto& ap : allpass_filters_) {
            ap.feedback = feedback;
        }
    }
    
    void setMix(float mix) {
        mix_ = std::clamp(mix, 0.0f, 1.0f);
    }
    
    void setFeedback(float feedback) {
        feedback_ = std::clamp(feedback, 0.0f, 0.95f);
    }
    
    void setModulation(float modulation) {
        modulation_ = std::clamp(modulation, 0.0f, 1.0f);
    }
    
    float getSize() const { return size_; }
    float getDamping() const { return damping_; }
    float getDiffusion() const { return diffusion_; }
    float getMix() const { return mix_; }
    float getFeedback() const { return feedback_; }
    float getModulation() const { return modulation_; }
    
    const char* getAlgorithmName() const {
        switch (current_algorithm_) {
            case Algorithm::HALL: return "Hall";
            case Algorithm::ROOM: return "Room";
            case Algorithm::PLATE: return "Plate";
            case Algorithm::SPRING: return "Spring";
            case Algorithm::SHIMMER: return "Shimmer";
            case Algorithm::CLOUDS: return "Clouds";
            case Algorithm::CHORUS: return "Chorus";
            case Algorithm::DELAY: return "Delay";
            default: return "Unknown";
        }
    }
    
    float process(float input) {
        float wet_signal = 0.0f;
        
        switch (current_algorithm_) {
            case Algorithm::HALL:
                wet_signal = processHall(input);
                break;
            case Algorithm::ROOM:
                wet_signal = processRoom(input);
                break;
            case Algorithm::PLATE:
                wet_signal = processPlate(input);
                break;
            case Algorithm::SPRING:
                wet_signal = processSpring(input);
                break;
            case Algorithm::SHIMMER:
                wet_signal = processShimmer(input);
                break;
            case Algorithm::CLOUDS:
                wet_signal = processClouds(input);
                break;
            case Algorithm::CHORUS:
                wet_signal = processChorus(input);
                break;
            case Algorithm::DELAY:
                wet_signal = processDelay(input);
                break;
            default:
                wet_signal = processHall(input);
                break;
        }
        
        // Mix dry and wet signals
        return input * (1.0f - mix_) + wet_signal * mix_;
    }
    
private:
    void updateAlgorithmParameters() {
        // Adjust parameters based on current algorithm
        switch (current_algorithm_) {
            case Algorithm::HALL:
                feedback_ = 0.85f + size_ * 0.1f;
                break;
            case Algorithm::ROOM:
                feedback_ = 0.7f + size_ * 0.15f;
                break;
            case Algorithm::PLATE:
                feedback_ = 0.8f + size_ * 0.1f;
                break;
            case Algorithm::SPRING:
                feedback_ = 0.6f + size_ * 0.2f;
                break;
            case Algorithm::SHIMMER:
                feedback_ = 0.9f + size_ * 0.05f;
                break;
            case Algorithm::CLOUDS:
                feedback_ = 0.75f + size_ * 0.15f;
                break;
            case Algorithm::CHORUS:
                feedback_ = 0.5f + size_ * 0.3f;
                break;
            case Algorithm::DELAY:
                feedback_ = 0.4f + size_ * 0.4f;
                break;
        }
    }
    
    float processHall(float input) {
        // Large hall with long decay and rich diffusion
        float signal = input;
        
        // Early reflections through allpass chain
        for (auto& ap : allpass_filters_) {
            signal = ap.process(signal);
        }
        
        // Late reverb through delay network
        float late_reverb = 0.0f;
        for (size_t i = 0; i < 8; i++) {
            float& delayed = delay_lines_[i][delay_indices_[i]];
            late_reverb += delayed;
            
            // Feedback with damping
            float feedback_signal = signal + late_reverb * feedback_ * 0.125f;
            feedback_signal = damping_filters_[i % 6].process(feedback_signal);
            delay_lines_[i][delay_indices_[i]] = feedback_signal;
            
            delay_indices_[i] = (delay_indices_[i] + 1) % DELAY_LENGTHS[i];
        }
        
        return late_reverb * 0.125f;
    }
    
    float processRoom(float input) {
        // Intimate room with shorter decay and warmer tone
        float signal = input;
        
        // Less diffusion for more intimate sound
        for (size_t i = 0; i < 2; i++) {
            signal = allpass_filters_[i].process(signal);
        }
        
        float late_reverb = 0.0f;
        for (size_t i = 0; i < 6; i++) {
            float& delayed = delay_lines_[i][delay_indices_[i]];
            late_reverb += delayed;
            
            // More damping for warmer sound
            float feedback_signal = signal + late_reverb * feedback_ * 0.15f;
            feedback_signal = damping_filters_[i].process(feedback_signal);
            delay_lines_[i][delay_indices_[i]] = feedback_signal;
            
            delay_indices_[i] = (delay_indices_[i] + 1) % DELAY_LENGTHS[i];
        }
        
        return late_reverb * 0.16f;
    }
    
    float processPlate(float input) {
        // Vintage plate reverb with characteristic brightness
        float signal = input;
        
        // Heavy diffusion for plate character
        for (auto& ap : allpass_filters_) {
            signal = ap.process(signal);
        }
        
        // Less damping for brighter sound
        float late_reverb = 0.0f;
        for (size_t i = 0; i < 6; i++) {
            float& delayed = delay_lines_[i][delay_indices_[i]];
            late_reverb += delayed;
            
            float feedback_signal = signal + late_reverb * feedback_ * 0.14f;
            // Minimal damping
            if (i % 2 == 0) {
                feedback_signal = damping_filters_[i / 2].process(feedback_signal);
            }
            delay_lines_[i][delay_indices_[i]] = feedback_signal;
            
            delay_indices_[i] = (delay_indices_[i] + 1) % DELAY_LENGTHS[i];
        }
        
        return late_reverb * 0.15f;
    }
    
    float processSpring(float input) {
        // Spring tank reverb with characteristic "boing"
        float signal = input;
        
        // Minimal diffusion for spring character
        signal = allpass_filters_[0].process(signal);
        
        // Resonant delays
        float spring_signal = 0.0f;
        for (size_t i = 0; i < 3; i++) {
            float& delayed = delay_lines_[i][delay_indices_[i]];
            spring_signal += delayed;
            
            // High feedback for metallic resonance
            float feedback_signal = signal + spring_signal * feedback_ * 0.3f;
            // Add some saturation
            feedback_signal = std::tanh(feedback_signal * 2.0f) * 0.5f;
            delay_lines_[i][delay_indices_[i]] = feedback_signal;
            
            delay_indices_[i] = (delay_indices_[i] + 1) % DELAY_LENGTHS[i];
        }
        
        return spring_signal * 0.3f;
    }
    
    float processShimmer(float input) {
        // Shimmer reverb with pitch-shifted feedback
        float signal = input;
        
        // Diffusion
        for (size_t i = 0; i < 3; i++) {
            signal = allpass_filters_[i].process(signal);
        }
        
        float late_reverb = 0.0f;
        for (size_t i = 0; i < 6; i++) {
            float& delayed = delay_lines_[i][delay_indices_[i]];
            late_reverb += delayed;
            
            float feedback_signal = signal + late_reverb * feedback_ * 0.12f;
            
            // Add pitch-shifted components
            if (i == 2) {
                feedback_signal += pitch_shifter_up_.process(late_reverb) * 0.3f;
            } else if (i == 4) {
                feedback_signal += pitch_shifter_down_.process(late_reverb) * 0.2f;
            }
            
            feedback_signal = damping_filters_[i % 6].process(feedback_signal);
            delay_lines_[i][delay_indices_[i]] = feedback_signal;
            
            delay_indices_[i] = (delay_indices_[i] + 1) % DELAY_LENGTHS[i];
        }
        
        return late_reverb * 0.13f;
    }
    
    float processClouds(float input) {
        // Clouds-style granular reverb (simplified version)
        float signal = input;
        
        // Light diffusion
        signal = allpass_filters_[0].process(signal);
        
        // Traditional reverb with some modulation
        updateLFOs();
        
        float late_reverb = 0.0f;
        for (size_t i = 0; i < 4; i++) {
            float& delayed = delay_lines_[i][delay_indices_[i]];
            late_reverb += delayed;
            
            float feedback_signal = signal + late_reverb * feedback_ * 0.2f;
            
            // Add modulation
            float lfo_mod = std::sin(lfo_phases_[i % 4]) * modulation_ * 0.1f;
            feedback_signal += lfo_mod;
            
            feedback_signal = damping_filters_[i].process(feedback_signal);
            delay_lines_[i][delay_indices_[i]] = feedback_signal;
            
            delay_indices_[i] = (delay_indices_[i] + 1) % DELAY_LENGTHS[i];
        }
        
        return late_reverb * 0.2f;
    }
    
    float processChorus(float input) {
        // Chorus-reverb hybrid
        float signal = input;
        
        // Modulated delays for chorus effect
        updateLFOs();
        
        float chorus_signal = 0.0f;
        for (size_t i = 0; i < 4; i++) {
            // Modulated delay time
            float lfo_value = std::sin(lfo_phases_[i]) * modulation_ * 10.0f;
            float mod_delay = DELAY_LENGTHS[i] + lfo_value;
            
            // Simple linear interpolation for modulated delay
            size_t delay_int = static_cast<size_t>(mod_delay) % DELAY_LENGTHS[i];
            float& delayed = delay_lines_[i][delay_int];
            
            chorus_signal += delayed;
            
            float feedback_signal = signal + chorus_signal * feedback_ * 0.15f;
            delay_lines_[i][delay_indices_[i]] = feedback_signal;
            
            delay_indices_[i] = (delay_indices_[i] + 1) % DELAY_LENGTHS[i];
        }
        
        return chorus_signal * 0.25f;
    }
    
    float processDelay(float input) {
        // Multi-tap delay reverb
        float signal = input;
        
        float delay_signal = 0.0f;
        for (size_t i = 0; i < 4; i++) {
            float& delayed = delay_lines_[i][delay_indices_[i]];
            delay_signal += delayed * (1.0f - i * 0.15f); // Decreasing amplitude
            
            float feedback_signal = signal;
            if (i > 0) {
                feedback_signal += delay_signal * feedback_ * 0.3f;
            }
            
            delay_lines_[i][delay_indices_[i]] = feedback_signal;
            delay_indices_[i] = (delay_indices_[i] + 1) % DELAY_LENGTHS[i];
        }
        
        return delay_signal * 0.4f;
    }
    
    void updateLFOs() {
        for (size_t i = 0; i < lfo_phases_.size(); i++) {
            lfo_phases_[i] += TWO_PI * lfo_frequencies_[i] / SAMPLE_RATE;
            if (lfo_phases_[i] >= TWO_PI) {
                lfo_phases_[i] -= TWO_PI;
            }
        }
    }
};

// Multi-mode filter with various filter types
class MultiModeFilter {
public:
    enum class FilterType {
        LOW_PASS,
        HIGH_PASS,
        BAND_PASS,
        NOTCH,
        COMB,
        FORMANT,
        COUNT
    };
    
private:
    FilterType filter_type_ = FilterType::LOW_PASS;
    float cutoff_ = 1000.0f;
    float resonance_ = 1.0f;
    float gain_ = 1.0f;
    
    // State variables for different filter implementations
    float x1_ = 0.0f, x2_ = 0.0f;
    float y1_ = 0.0f, y2_ = 0.0f;
    float a0_, a1_, a2_, b1_, b2_;
    
    // Comb filter delay line
    std::vector<float> comb_buffer_;
    size_t comb_index_ = 0;
    float comb_feedback_ = 0.5f;
    
    // Formant filter state
    struct FormantStage {
        float x1 = 0.0f, x2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;
        float a0, a1, a2, b1, b2;
    };
    std::array<FormantStage, 3> formant_stages_;
    
public:
    MultiModeFilter() {
        comb_buffer_.resize(4800, 0.0f); // Max 100ms at 48kHz
        updateCoefficients();
    }
    
    void setFilterType(FilterType type) {
        filter_type_ = type;
        updateCoefficients();
    }
    
    void setCutoff(float cutoff) {
        cutoff_ = std::clamp(cutoff, 20.0f, 20000.0f);
        updateCoefficients();
    }
    
    void setResonance(float resonance) {
        resonance_ = std::clamp(resonance, 0.1f, 20.0f);
        updateCoefficients();
    }
    
    void setGain(float gain) {
        gain_ = std::clamp(gain, 0.1f, 4.0f);
    }
    
    FilterType getFilterType() const { return filter_type_; }
    float getCutoff() const { return cutoff_; }
    float getResonance() const { return resonance_; }
    float getGain() const { return gain_; }
    
    const char* getFilterTypeName() const {
        switch (filter_type_) {
            case FilterType::LOW_PASS: return "LowPass";
            case FilterType::HIGH_PASS: return "HighPass";
            case FilterType::BAND_PASS: return "BandPass";
            case FilterType::NOTCH: return "Notch";
            case FilterType::COMB: return "Comb";
            case FilterType::FORMANT: return "Formant";
            default: return "Unknown";
        }
    }
    
    float process(float input) {
        switch (filter_type_) {
            case FilterType::LOW_PASS:
                return processLowPass(input);
            case FilterType::HIGH_PASS:
                return processHighPass(input);
            case FilterType::BAND_PASS:
                return processBandPass(input);
            case FilterType::NOTCH:
                return processNotch(input);
            case FilterType::COMB:
                return processComb(input);
            case FilterType::FORMANT:
                return processFormant(input);
            default:
                return processLowPass(input);
        }
    }
    
private:
    void updateCoefficients() {
        float omega = TWO_PI * cutoff_ / SAMPLE_RATE;
        float sin_omega = std::sin(omega);
        float cos_omega = std::cos(omega);
        float alpha = sin_omega / (2.0f * resonance_);
        
        switch (filter_type_) {
            case FilterType::LOW_PASS:
                {
                    float b0 = 1.0f + alpha;
                    a0_ = (1.0f - cos_omega) / 2.0f / b0;
                    a1_ = (1.0f - cos_omega) / b0;
                    a2_ = (1.0f - cos_omega) / 2.0f / b0;
                    b1_ = -2.0f * cos_omega / b0;
                    b2_ = (1.0f - alpha) / b0;
                }
                break;
                
            case FilterType::HIGH_PASS:
                {
                    float b0 = 1.0f + alpha;
                    a0_ = (1.0f + cos_omega) / 2.0f / b0;
                    a1_ = -(1.0f + cos_omega) / b0;
                    a2_ = (1.0f + cos_omega) / 2.0f / b0;
                    b1_ = -2.0f * cos_omega / b0;
                    b2_ = (1.0f - alpha) / b0;
                }
                break;
                
            case FilterType::BAND_PASS:
                {
                    float b0 = 1.0f + alpha;
                    a0_ = alpha / b0;
                    a1_ = 0.0f;
                    a2_ = -alpha / b0;
                    b1_ = -2.0f * cos_omega / b0;
                    b2_ = (1.0f - alpha) / b0;
                }
                break;
                
            case FilterType::NOTCH:
                {
                    float b0 = 1.0f + alpha;
                    a0_ = 1.0f / b0;
                    a1_ = -2.0f * cos_omega / b0;
                    a2_ = 1.0f / b0;
                    b1_ = -2.0f * cos_omega / b0;
                    b2_ = (1.0f - alpha) / b0;
                }
                break;
                
            case FilterType::COMB:
                {
                    // Update comb delay length based on cutoff
                    float delay_samples = SAMPLE_RATE / cutoff_;
                    size_t delay_length = static_cast<size_t>(std::clamp(delay_samples, 1.0f, static_cast<float>(comb_buffer_.size() - 1)));
                    if (delay_length < comb_buffer_.size()) {
                        // Resize buffer if needed
                        std::fill(comb_buffer_.begin() + delay_length, comb_buffer_.end(), 0.0f);
                    }
                    comb_feedback_ = 1.0f - 1.0f / resonance_;
                }
                break;
                
            case FilterType::FORMANT:
                updateFormantCoefficients();
                break;
                
            case FilterType::COUNT:
                break;
        }
    }
    
    void updateFormantCoefficients() {
        // Three formant frequencies based on cutoff
        float f1 = cutoff_ * 0.8f;
        float f2 = cutoff_ * 1.5f;
        float f3 = cutoff_ * 2.8f;
        
        for (size_t i = 0; i < 3; i++) {
            float freq = (i == 0) ? f1 : (i == 1) ? f2 : f3;
            float omega = TWO_PI * freq / SAMPLE_RATE;
            float sin_omega = std::sin(omega);
            float cos_omega = std::cos(omega);
            float alpha = sin_omega / (2.0f * resonance_);
            
            float b0 = 1.0f + alpha;
            formant_stages_[i].a0 = alpha / b0;
            formant_stages_[i].a1 = 0.0f;
            formant_stages_[i].a2 = -alpha / b0;
            formant_stages_[i].b1 = -2.0f * cos_omega / b0;
            formant_stages_[i].b2 = (1.0f - alpha) / b0;
        }
    }
    
    float processLowPass(float input) {
        float output = a0_ * input + a1_ * x1_ + a2_ * x2_ - b1_ * y1_ - b2_ * y2_;
        
        x2_ = x1_;
        x1_ = input;
        y2_ = y1_;
        y1_ = output;
        
        return output * gain_;
    }
    
    float processHighPass(float input) {
        return processLowPass(input); // Uses same biquad structure
    }
    
    float processBandPass(float input) {
        return processLowPass(input); // Uses same biquad structure
    }
    
    float processNotch(float input) {
        return processLowPass(input); // Uses same biquad structure
    }
    
    float processComb(float input) {
        float delay_samples = SAMPLE_RATE / cutoff_;
        size_t delay_length = static_cast<size_t>(std::clamp(delay_samples, 1.0f, static_cast<float>(comb_buffer_.size() - 1)));
        
        float delayed = comb_buffer_[(comb_index_ + comb_buffer_.size() - delay_length) % comb_buffer_.size()];
        float output = input + delayed * comb_feedback_;
        
        comb_buffer_[comb_index_] = output;
        comb_index_ = (comb_index_ + 1) % comb_buffer_.size();
        
        return output * gain_;
    }
    
    float processFormant(float input) {
        float output = input;
        
        // Process through three formant stages
        for (auto& stage : formant_stages_) {
            float stage_output = stage.a0 * output + stage.a1 * stage.x1 + stage.a2 * stage.x2 
                               - stage.b1 * stage.y1 - stage.b2 * stage.y2;
            
            stage.x2 = stage.x1;
            stage.x1 = output;
            stage.y2 = stage.y1;
            stage.y1 = stage_output;
            
            output = stage_output;
        }
        
        return output * gain_;
    }
};

// Preset system for saving and loading complete synthesizer states
class PresetManager {
public:
    struct Preset {
        std::string name;
        EngineType engine_type;
        PlaitsModel plaits_model;
        
        // Synthesis parameters
        std::array<float, static_cast<size_t>(ParameterID::COUNT)> synth_params;
        
        // Reverb settings
        LushReverb::Algorithm reverb_algorithm;
        float reverb_size;
        float reverb_damping;
        float reverb_diffusion;
        float reverb_mix;
        float reverb_send;
        bool reverb_enabled;
        
        // Filter settings
        MultiModeFilter::FilterType filter_type;
        float filter_cutoff;
        float filter_resonance;
        float filter_gain;
        
        // Drum patterns (simplified)
        std::array<std::array<bool, MAX_DRUM_STEPS>, MAX_DRUM_TRACKS> drum_patterns;
        float drum_bpm;
        
        // Mode states
        bool chord_mode;
        bool bicep_mode;
        float bicep_intensity;
        bool drum_mode;
        
        Preset() {
            name = "Init";
            engine_type = EngineType::SUBTRACTIVE;
            plaits_model = PlaitsModel::VIRTUAL_ANALOG;
            
            synth_params.fill(0.5f);
            synth_params[static_cast<size_t>(ParameterID::ATTACK)] = 0.1f;
            synth_params[static_cast<size_t>(ParameterID::DECAY)] = 0.3f;
            synth_params[static_cast<size_t>(ParameterID::SUSTAIN)] = 0.7f;
            synth_params[static_cast<size_t>(ParameterID::RELEASE)] = 0.4f;
            
            reverb_algorithm = LushReverb::Algorithm::HALL;
            reverb_size = 0.8f;
            reverb_damping = 0.6f;
            reverb_diffusion = 0.7f;
            reverb_mix = 0.3f;
            reverb_send = 0.3f;
            reverb_enabled = true;
            
            filter_type = MultiModeFilter::FilterType::LOW_PASS;
            filter_cutoff = 1000.0f;
            filter_resonance = 1.0f;
            filter_gain = 1.0f;
            
            for (auto& track : drum_patterns) {
                track.fill(false);
            }
            drum_bpm = 120.0f;
            
            chord_mode = false;
            bicep_mode = false;
            bicep_intensity = 1.0f;
            drum_mode = false;
        }
    };
    
private:
    static constexpr size_t MAX_PRESETS = 32;
    std::array<Preset, MAX_PRESETS> presets_;
    size_t current_preset_ = 0;
    bool preset_modified_ = false;
    
public:
    PresetManager() {
        // Initialize with default presets
        initializeDefaultPresets();
    }
    
    void savePreset(size_t slot, const Preset& preset) {
        if (slot < MAX_PRESETS) {
            presets_[slot] = preset;
            preset_modified_ = false;
        }
    }
    
    const Preset& loadPreset(size_t slot) {
        if (slot < MAX_PRESETS) {
            current_preset_ = slot;
            preset_modified_ = false;
            return presets_[slot];
        }
        return presets_[0];
    }
    
    void setCurrentPreset(size_t slot) {
        if (slot < MAX_PRESETS) {
            current_preset_ = slot;
        }
    }
    
    size_t getCurrentPresetIndex() const { return current_preset_; }
    
    const Preset& getCurrentPresetData() const {
        return presets_[current_preset_];
    }
    
    void markModified() { preset_modified_ = true; }
    bool isModified() const { return preset_modified_; }
    
    const char* getPresetName(size_t slot) const {
        if (slot < MAX_PRESETS) {
            return presets_[slot].name.c_str();
        }
        return "Empty";
    }
    
    void nextPreset() {
        current_preset_ = (current_preset_ + 1) % MAX_PRESETS;
        preset_modified_ = false;
    }
    
    void cyclePreset(int direction) {
        if (direction > 0) {
            current_preset_ = (current_preset_ + 1) % MAX_PRESETS;
        } else {
            current_preset_ = (current_preset_ + MAX_PRESETS - 1) % MAX_PRESETS;
        }
        preset_modified_ = false;
    }
    
    size_t getCurrentSlot() const {
        return current_preset_;
    }
    
    std::optional<Preset> getCurrentPreset() const {
        return std::optional<Preset>(presets_[current_preset_]);
    }
    
    void savePreset(const Preset& preset) {
        savePreset(current_preset_, preset);
    }
    
    void previousPreset() {
        current_preset_ = (current_preset_ + MAX_PRESETS - 1) % MAX_PRESETS;
        preset_modified_ = false;
    }
    
private:
    void initializeDefaultPresets() {
        // Preset 0: Basic Lead
        presets_[0].name = "Basic Lead";
        presets_[0].engine_type = EngineType::SUBTRACTIVE;
        presets_[0].synth_params[static_cast<size_t>(ParameterID::HARMONICS)] = 0.3f;
        presets_[0].synth_params[static_cast<size_t>(ParameterID::TIMBRE)] = 0.6f;
        presets_[0].synth_params[static_cast<size_t>(ParameterID::ATTACK)] = 0.05f;
        presets_[0].synth_params[static_cast<size_t>(ParameterID::RELEASE)] = 0.3f;
        
        // Preset 1: Warm Pad
        presets_[1].name = "Warm Pad";
        presets_[1].engine_type = EngineType::WARM_PAD;
        presets_[1].synth_params[static_cast<size_t>(ParameterID::ATTACK)] = 0.8f;
        presets_[1].synth_params[static_cast<size_t>(ParameterID::RELEASE)] = 1.0f;
        presets_[1].reverb_size = 0.9f;
        presets_[1].reverb_mix = 0.4f;
        
        // Preset 2: FM Bell
        presets_[2].name = "FM Bell";
        presets_[2].engine_type = EngineType::FM;
        presets_[2].synth_params[static_cast<size_t>(ParameterID::HARMONICS)] = 0.7f;
        presets_[2].synth_params[static_cast<size_t>(ParameterID::TIMBRE)] = 0.4f;
        presets_[2].synth_params[static_cast<size_t>(ParameterID::DECAY)] = 0.6f;
        
        // Preset 3: Plaits Clouds
        presets_[3].name = "Plaits Clouds";
        presets_[3].engine_type = EngineType::PLAITS;
        presets_[3].plaits_model = PlaitsModel::GRANULAR_CLOUD;
        presets_[3].reverb_algorithm = LushReverb::Algorithm::CLOUDS;
        presets_[3].reverb_mix = 0.5f;
        
        // Preset 4: Drum Kit
        presets_[4].name = "Drum Kit";
        presets_[4].drum_mode = true;
        presets_[4].drum_bpm = 120.0f;
        presets_[4].reverb_algorithm = LushReverb::Algorithm::ROOM;
        presets_[4].reverb_send = 0.2f;
        
        // Fill remaining presets with variations
        for (size_t i = 5; i < MAX_PRESETS; i++) {
            presets_[i].name = "User " + std::to_string(i);
        }
    }
};

// Modulation matrix for routing modulators to parameters
class ModulationMatrix {
public:
    enum class ModSource {
        LFO1,
        LFO2,
        LFO3,
        LFO4,
        ENV1,
        ENV2,
        RANDOM,
        AUDIO_LEVEL,
        COUNT
    };
    
    enum class ModDestination {
        FILTER_CUTOFF,
        FILTER_RESONANCE,
        OSC_PITCH,
        OSC_TIMBRE,
        OSC_MORPH,
        REVERB_SIZE,
        REVERB_MIX,
        VOLUME,
        COUNT
    };
    
    struct ModConnection {
        ModSource source;
        ModDestination destination;
        float amount;        // -1.0 to +1.0
        bool enabled;
        
        ModConnection() : source(ModSource::LFO1), destination(ModDestination::FILTER_CUTOFF), 
                         amount(0.0f), enabled(false) {}
    };
    
private:
    static constexpr size_t MAX_CONNECTIONS = 16;
    std::array<ModConnection, MAX_CONNECTIONS> connections_;
    
    // Modulation sources
    struct LFO {
        float frequency = 1.0f;
        float phase = 0.0f;
        float depth = 1.0f;
        enum class Shape { SINE, TRIANGLE, SAW, SQUARE, RANDOM } shape = Shape::SINE;
        float value = 0.0f;
        
        void process() {
            switch (shape) {
                case Shape::SINE:
                    value = std::sin(phase) * depth;
                    break;
                case Shape::TRIANGLE:
                    value = (2.0f * std::abs(2.0f * (phase / TWO_PI - std::floor(phase / TWO_PI + 0.5f))) - 1.0f) * depth;
                    break;
                case Shape::SAW:
                    value = (2.0f * (phase / TWO_PI - std::floor(phase / TWO_PI + 0.5f))) * depth;
                    break;
                case Shape::SQUARE:
                    value = (phase < M_PI ? 1.0f : -1.0f) * depth;
                    break;
                case Shape::RANDOM:
                    // Sample and hold random
                    static float last_random = 0.0f;
                    static float last_phase = 0.0f;
                    if (phase < last_phase) { // Wrapped around
                        last_random = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
                    }
                    last_phase = phase;
                    value = last_random * depth;
                    break;
            }
            
            phase += TWO_PI * frequency / SAMPLE_RATE;
            if (phase >= TWO_PI) phase -= TWO_PI;
        }
        
        const char* getShapeName() const {
            switch (shape) {
                case Shape::SINE: return "Sine";
                case Shape::TRIANGLE: return "Tri";
                case Shape::SAW: return "Saw";
                case Shape::SQUARE: return "Square";
                case Shape::RANDOM: return "Random";
                default: return "Unknown";
            }
        }
    };
    
    std::array<LFO, 4> lfos_;
    
    struct Envelope {
        float attack = 0.1f;
        float decay = 0.3f;
        float sustain = 0.7f;
        float release = 0.4f;
        
        enum class Stage { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE } stage = Stage::IDLE;
        float level = 0.0f;
        float value = 0.0f;
        
        void noteOn() {
            stage = Stage::ATTACK;
        }
        
        void noteOff() {
            if (stage != Stage::IDLE) {
                stage = Stage::RELEASE;
            }
        }
        
        void process() {
            float attack_rate = 1.0f / (attack * SAMPLE_RATE);
            float decay_rate = 1.0f / (decay * SAMPLE_RATE);
            float release_rate = 1.0f / (release * SAMPLE_RATE);
            
            switch (stage) {
                case Stage::IDLE:
                    level = 0.0f;
                    break;
                    
                case Stage::ATTACK:
                    level += attack_rate;
                    if (level >= 1.0f) {
                        level = 1.0f;
                        stage = Stage::DECAY;
                    }
                    break;
                    
                case Stage::DECAY:
                    level -= decay_rate;
                    if (level <= sustain) {
                        level = sustain;
                        stage = Stage::SUSTAIN;
                    }
                    break;
                    
                case Stage::SUSTAIN:
                    level = sustain;
                    break;
                    
                case Stage::RELEASE:
                    level -= release_rate;
                    if (level <= 0.0f) {
                        level = 0.0f;
                        stage = Stage::IDLE;
                    }
                    break;
            }
            
            value = level;
        }
        
        bool isActive() const {
            return stage != Stage::IDLE;
        }
    };
    
    std::array<Envelope, 2> envelopes_;
    
    // Audio level follower
    float audio_level_ = 0.0f;
    float audio_level_smooth_ = 0.0f;
    
    // Random source
    float random_value_ = 0.0f;
    float random_rate_ = 1.0f; // Hz
    float random_phase_ = 0.0f;
    
    // Modulation outputs
    std::array<float, static_cast<size_t>(ModDestination::COUNT)> mod_outputs_;
    
public:
    ModulationMatrix() {
        // Initialize LFOs with different frequencies
        lfos_[0].frequency = 0.5f;
        lfos_[1].frequency = 1.2f;
        lfos_[2].frequency = 3.7f;
        lfos_[3].frequency = 0.1f;
        
        // Initialize some default connections
        connections_[0].source = ModSource::LFO1;
        connections_[0].destination = ModDestination::FILTER_CUTOFF;
        connections_[0].amount = 0.3f;
        connections_[0].enabled = true;
        
        connections_[1].source = ModSource::ENV1;
        connections_[1].destination = ModDestination::FILTER_CUTOFF;
        connections_[1].amount = 0.6f;
        connections_[1].enabled = true;
        
        connections_[2].source = ModSource::LFO2;
        connections_[2].destination = ModDestination::OSC_PITCH;
        connections_[2].amount = 0.1f;
        connections_[2].enabled = false;
        
        mod_outputs_.fill(0.0f);
    }
    
    void process(float audio_input) {
        // Update all modulation sources
        for (auto& lfo : lfos_) {
            lfo.process();
        }
        
        for (auto& env : envelopes_) {
            env.process();
        }
        
        // Update audio level follower
        float abs_input = std::abs(audio_input);
        audio_level_smooth_ = audio_level_smooth_ * 0.999f + abs_input * 0.001f;
        audio_level_ = audio_level_smooth_;
        
        // Update random source
        random_phase_ += TWO_PI * random_rate_ / SAMPLE_RATE;
        if (random_phase_ >= TWO_PI) {
            random_phase_ -= TWO_PI;
            random_value_ = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        }
        
        // Clear modulation outputs
        mod_outputs_.fill(0.0f);
        
        // Process all connections
        for (const auto& connection : connections_) {
            if (!connection.enabled) continue;
            
            float mod_value = getModSourceValue(connection.source);
            mod_value *= connection.amount;
            
            size_t dest_index = static_cast<size_t>(connection.destination);
            mod_outputs_[dest_index] += mod_value;
        }
    }
    
    float getModulation(ModDestination destination) const {
        size_t index = static_cast<size_t>(destination);
        return std::clamp(mod_outputs_[index], -1.0f, 1.0f);
    }
    
    void noteOn() {
        for (auto& env : envelopes_) {
            env.noteOn();
        }
    }
    
    void noteOff() {
        for (auto& env : envelopes_) {
            env.noteOff();
        }
    }
    
    // LFO control
    void setLFOFrequency(size_t lfo_index, float frequency) {
        if (lfo_index < lfos_.size()) {
            lfos_[lfo_index].frequency = std::clamp(frequency, 0.01f, 50.0f);
        }
    }
    
    void setLFOShape(size_t lfo_index, LFO::Shape shape) {
        if (lfo_index < lfos_.size()) {
            lfos_[lfo_index].shape = shape;
        }
    }
    
    void setLFODepth(size_t lfo_index, float depth) {
        if (lfo_index < lfos_.size()) {
            lfos_[lfo_index].depth = std::clamp(depth, 0.0f, 1.0f);
        }
    }
    
    // Connection management
    void setConnection(size_t index, ModSource source, ModDestination destination, float amount, bool enabled) {
        if (index < connections_.size()) {
            connections_[index].source = source;
            connections_[index].destination = destination;
            connections_[index].amount = std::clamp(amount, -1.0f, 1.0f);
            connections_[index].enabled = enabled;
        }
    }
    
    const ModConnection& getConnection(size_t index) const {
        if (index < connections_.size()) {
            return connections_[index];
        }
        static ModConnection empty;
        return empty;
    }
    
    void toggleConnection(size_t index) {
        if (index < connections_.size()) {
            connections_[index].enabled = !connections_[index].enabled;
        }
    }
    
    // Getters for UI
    float getLFOFrequency(size_t lfo_index) const {
        return (lfo_index < lfos_.size()) ? lfos_[lfo_index].frequency : 0.0f;
    }
    
    const char* getLFOShapeName(size_t lfo_index) const {
        return (lfo_index < lfos_.size()) ? lfos_[lfo_index].getShapeName() : "Unknown";
    }
    
    float getLFODepth(size_t lfo_index) const {
        return (lfo_index < lfos_.size()) ? lfos_[lfo_index].depth : 0.0f;
    }
    
    const char* getModSourceName(ModSource source) const {
        switch (source) {
            case ModSource::LFO1: return "LFO1";
            case ModSource::LFO2: return "LFO2";
            case ModSource::LFO3: return "LFO3";
            case ModSource::LFO4: return "LFO4";
            case ModSource::ENV1: return "Env1";
            case ModSource::ENV2: return "Env2";
            case ModSource::RANDOM: return "Random";
            case ModSource::AUDIO_LEVEL: return "Audio";
            default: return "Unknown";
        }
    }
    
    const char* getModDestinationName(ModDestination destination) const {
        switch (destination) {
            case ModDestination::FILTER_CUTOFF: return "Filter Freq";
            case ModDestination::FILTER_RESONANCE: return "Filter Res";
            case ModDestination::OSC_PITCH: return "Osc Pitch";
            case ModDestination::OSC_TIMBRE: return "Osc Timbre";
            case ModDestination::OSC_MORPH: return "Osc Morph";
            case ModDestination::REVERB_SIZE: return "Reverb Size";
            case ModDestination::REVERB_MIX: return "Reverb Mix";
            case ModDestination::VOLUME: return "Volume";
            default: return "Unknown";
        }
    }
    
private:
    float getModSourceValue(ModSource source) const {
        switch (source) {
            case ModSource::LFO1: return lfos_[0].value;
            case ModSource::LFO2: return lfos_[1].value;
            case ModSource::LFO3: return lfos_[2].value;
            case ModSource::LFO4: return lfos_[3].value;
            case ModSource::ENV1: return envelopes_[0].value;
            case ModSource::ENV2: return envelopes_[1].value;
            case ModSource::RANDOM: return random_value_;
            case ModSource::AUDIO_LEVEL: return audio_level_;
            default: return 0.0f;
        }
    }
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
                case EngineType::SUB_BASS:
                    voice.engine = std::make_unique<SubBassEngine>();
                    break;
                case EngineType::WARM_PAD:
                    voice.engine = std::make_unique<WarmPadEngine>();
                    break;
                case EngineType::BRIGHT_LEAD:
                    voice.engine = std::make_unique<BrightLeadEngine>();
                    break;
                case EngineType::STRING_ENSEMBLE:
                    voice.engine = std::make_unique<StringEnsembleEngine>();
                    break;
                case EngineType::GRANULAR:
                    voice.engine = std::make_unique<GranularEngine>();
                    break;
                case EngineType::PLAITS:
                    voice.engine = std::make_unique<PlaitsEngine>();
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
    
    // Play note with multiple engines (bicep mode)
    void noteOnMultiEngine(uint8_t note, uint8_t velocity, const std::vector<EngineType>& engines) {
        for (EngineType engine_type : engines) {
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
            
            // Create specific engine for this voice
            if (free_voice) {
                switch (engine_type) {
                    case EngineType::SUBTRACTIVE:
                        free_voice->engine = std::make_unique<SubtractiveEngine>();
                        break;
                    case EngineType::FM:
                        free_voice->engine = std::make_unique<FMEngine>();
                        break;
                    case EngineType::SUB_BASS:
                        free_voice->engine = std::make_unique<SubBassEngine>();
                        break;
                    case EngineType::WARM_PAD:
                        free_voice->engine = std::make_unique<WarmPadEngine>();
                        break;
                    case EngineType::BRIGHT_LEAD:
                        free_voice->engine = std::make_unique<BrightLeadEngine>();
                        break;
                    case EngineType::STRING_ENSEMBLE:
                        free_voice->engine = std::make_unique<StringEnsembleEngine>();
                        break;
                    case EngineType::GRANULAR:
                        free_voice->engine = std::make_unique<GranularEngine>();
                        break;
                    case EngineType::PLAITS:
                        free_voice->engine = std::make_unique<PlaitsEngine>();
                        break;
                    default:
                        free_voice->engine = std::make_unique<SubtractiveEngine>();
                        break;
                }
                
                free_voice->note = note;
                free_voice->active = true;
                free_voice->start_time = voice_counter_++;
                free_voice->engine->noteOn(note, velocity);
            }
        }
    }
    
    // Plaits-specific methods
    void setPlaitsModel(PlaitsModel model) {
        for (auto& voice : voices_) {
            if (voice.engine) {
                PlaitsEngine* plaits_engine = dynamic_cast<PlaitsEngine*>(voice.engine.get());
                if (plaits_engine) {
                    plaits_engine->setModel(model);
                }
            }
        }
    }
    
    const char* getCurrentPlaitsModelName() const {
        for (const auto& voice : voices_) {
            if (voice.engine) {
                const PlaitsEngine* plaits_engine = dynamic_cast<const PlaitsEngine*>(voice.engine.get());
                if (plaits_engine) {
                    return plaits_engine->getModelName();
                }
            }
        }
        return "VirtAnalog"; // Default
    }
};

// Main synthesizer class
class TerminalSynth {
private:
    VoiceManager voice_manager_;
    ChordGenerator chord_generator_;
    EuclideanRhythm euclidean_rhythm_;
    Arpeggiator arpeggiator_;
    DrumSynth drum_synth_;
    StepSequencer step_sequencer_;
    LushReverb reverb_;
    MultiModeFilter global_filter_;
    ModulationMatrix mod_matrix_;
    PresetManager preset_manager_;
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
    
    // Bicep mode - Multi-instrumental layering mode
    bool bicep_mode_ = false;
    float bicep_intensity_ = 1.0f;
    std::vector<EngineType> bicep_layers_;
    size_t bicep_current_layer_ = 0;
    
    // Arpeggiator state
    std::chrono::steady_clock::time_point last_arp_check_;
    
    // Drum machine mode
    bool drum_mode_ = false;
    bool drum_recording_ = false;
    
    // Reverb state
    bool reverb_enabled_ = true;
    float reverb_send_ = 0.3f;
    
    // Filter state
    bool filter_enabled_ = true;
    
    // Modulation state
    size_t current_mod_connection_ = 0;
    bool mod_edit_mode_ = false;
    
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
        
        // Initialize arpeggiator timing
        last_arp_check_ = std::chrono::steady_clock::now();
        
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
        
        // Process arpeggiator timing
        if (arpeggiator_.isActive()) {
            uint8_t arp_note = arpeggiator_.step();
            if (arp_note > 0) {
                // Trigger arpeggiator note
                uint8_t velocity = 100;
                if (bicep_mode_ && !bicep_layers_.empty()) {
                    velocity = static_cast<uint8_t>(std::clamp(100.0f * bicep_intensity_, 80.0f, 127.0f));
                    voice_manager_.noteOnMultiEngine(arp_note, velocity, bicep_layers_);
                } else {
                    if (bicep_mode_) {
                        velocity = static_cast<uint8_t>(std::clamp(100.0f * bicep_intensity_, 80.0f, 127.0f));
                    }
                    voice_manager_.noteOn(arp_note, velocity);
                }
                
                // Schedule note off for arpeggiator notes
                std::thread([this, arp_note]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    voice_manager_.noteOff(arp_note);
                }).detach();
            }
        }
        
        // Process step sequencer and drum machine
        auto drum_triggers = step_sequencer_.step(drum_synth_);
        for (const auto& trigger : drum_triggers) {
            drum_synth_.triggerDrum(trigger.first, trigger.second);
        }
        
        // Process synthesizer voices into temporary buffer
        std::vector<float> temp_buffer(frames, 0.0f);
        voice_manager_.process(temp_buffer.data(), frames);
        
        // Process drum machine (additive)
        drum_synth_.process(temp_buffer.data(), frames);
        
        // Apply modulation matrix and filter processing
        for (size_t i = 0; i < frames; i++) {
            // Process modulation matrix once per frame (pass current audio sample for audio level detection)
            float audio_level = (i < frames) ? std::abs(temp_buffer[i]) : 0.0f;
            mod_matrix_.process(audio_level);
            
            // Apply modulation to filter cutoff
            float filter_mod = mod_matrix_.getModulation(ModulationMatrix::ModDestination::FILTER_CUTOFF);
            float base_cutoff = global_filter_.getCutoff();
            float modulated_cutoff = std::clamp(base_cutoff + filter_mod * 5000.0f, 20.0f, 20000.0f);
            global_filter_.setCutoff(modulated_cutoff);
            
            // Apply modulation to filter resonance
            float resonance_mod = mod_matrix_.getModulation(ModulationMatrix::ModDestination::FILTER_RESONANCE);
            float base_resonance = global_filter_.getResonance();
            float modulated_resonance = std::clamp(base_resonance + resonance_mod * 5.0f, 0.1f, 10.0f);
            global_filter_.setResonance(modulated_resonance);
            
            // Apply global filter
            temp_buffer[i] = global_filter_.process(temp_buffer[i]);
        }
        
        // Apply reverb to final mix
        for (size_t i = 0; i < frames; i++) {
            if (reverb_enabled_) {
                // Send to reverb and mix with dry signal
                float reverb_input = temp_buffer[i] * reverb_send_;
                float reverb_output = reverb_.process(reverb_input);
                output[i] = temp_buffer[i] * (1.0f - reverb_send_) + reverb_output;
            } else {
                output[i] = temp_buffer[i];
            }
        }
        
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
            case ']': selectEngine(EngineType::SUB_BASS); break;
            case '\\': selectEngine(EngineType::WARM_PAD); break;
            case '-': selectEngine(EngineType::BRIGHT_LEAD); break;
            case '=': selectEngine(EngineType::STRING_ENSEMBLE); break;
            case '`': selectEngine(EngineType::GRANULAR); break;
            
            // Plaits model cycling (only when Plaits is active)
            case 'M': cyclePlaitsModel(1); break;
            case 'N': cyclePlaitsModel(-1); break;
            
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
            
            // Bicep mode controls
            case 'B': toggleBicepMode(); break;
            case 'i': adjustBicepIntensity(-0.1f); break;
            case 'I': adjustBicepIntensity(0.1f); break;
            case '{': cycleBicepLayer(-1); break;
            case '}': cycleBicepLayer(1); break;
            case '+': addBicepLayer(); break;
            case '_': removeBicepLayer(); break;
            
            // Euclidean rhythm controls
            case 'E': toggleEuclideanRhythm(); break;
            case ',': adjustEuclideanHits(-1); break;
            case '.': adjustEuclideanHits(1); break;
            case '<': adjustEuclideanRotation(-1); break;
            case '>': adjustEuclideanRotation(1); break;
            
            // Arpeggiator controls
            case '/': toggleArpeggiator(); break;
            case '?': cycleArpPattern(1); break;
            case 'S': cycleArpSpeed(1); break;
            case 'T': adjustArpBPM(5.0f); break;
            case 'R': adjustArpBPM(-5.0f); break;
            
            // Drum machine controls
            case 'D': toggleDrumMode(); break;
            case 'X': toggleStepSequencer(); break;
            case 'C': clearDrumPattern(); break;
            case 'V': adjustDrumBPM(5.0f); break;
            case 'G': adjustDrumBPM(-5.0f); break;
            case 'H': cycleDrumTrack(1); break;
            case 'J': cycleDrumTrack(-1); break;
            
            // Reverb controls
            case '#': toggleReverb(); break;
            case '^': cycleReverbAlgorithm(1); break;
            case '&': adjustReverbSize(0.05f); break;
            case '!': adjustReverbSize(-0.05f); break;
            case '@': adjustReverbMix(0.05f); break;
            case '$': adjustReverbMix(-0.05f); break;
            
            // Filter controls (temporarily disabled due to key conflicts)
            // case 'U': cycleFilterType(1); break;
            // case 'Y': cycleFilterType(-1); break;
            // case 'h': adjustFilterCutoff(-0.05f); break;
            // case 'H': adjustFilterCutoff(0.05f); break;
            // case 'j': adjustFilterResonance(-0.05f); break;
            // case 'J': adjustFilterResonance(0.05f); break;
            
            // Preset controls (temporarily disabled due to key conflicts)
            // case '_': cyclePreset(-1); break;
            // case ')': cyclePreset(1); break;
            // case '~': saveCurrentPreset(); break;
            case 'Z': 
                if (drum_mode_) {
                    handleStepInput(ch);
                } else {
                    handleKey(ch, 60); // C
                }
                break;
            
            
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
        if (drum_mode_) {
            // In drum mode, trigger drums directly
            handleDrumKey(key_char);
        } else if (arpeggiator_.isActive()) {
            // In arpeggiator mode, add notes to the arpeggiator
            arpeggiator_.noteOn(note);
        } else if (chord_mode_) {
            // In chord mode, any key triggers a chord based on that root note
            chord_generator_.setRootNote(note);
            playChord();
        } else {
            // Normal single note mode with auto note-off
            uint8_t velocity = 100;
            
            if (bicep_mode_ && !bicep_layers_.empty()) {
                // Multi-instrumental bicep mode - play with all layers
                velocity = static_cast<uint8_t>(std::clamp(100.0f * bicep_intensity_, 80.0f, 127.0f));
                voice_manager_.noteOnMultiEngine(note, velocity, bicep_layers_);
            } else {
                // Single engine mode
                if (bicep_mode_) {
                    // Apply intensity even in single engine mode
                    velocity = static_cast<uint8_t>(std::clamp(100.0f * bicep_intensity_, 80.0f, 127.0f));
                }
                voice_manager_.noteOn(note, velocity);
            }
            
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
        arpeggiator_.allNotesOff();
    }
    
    void toggleBicepMode() {
        bicep_mode_ = !bicep_mode_;
        if (bicep_mode_) {
            // Initialize with default multi-engine layers
            bicep_layers_ = {EngineType::SUB_BASS, EngineType::BRIGHT_LEAD, EngineType::WARM_PAD};
            bicep_intensity_ = 1.5f;
            bicep_current_layer_ = 0;
        } else {
            bicep_layers_.clear();
            bicep_intensity_ = 1.0f;
        }
    }
    
    void adjustBicepIntensity(float delta) {
        bicep_intensity_ = std::clamp(bicep_intensity_ + delta, 0.1f, 3.0f);
    }
    
    void cycleBicepLayer(int direction) {
        if (!bicep_mode_ || bicep_layers_.empty()) return;
        
        bicep_current_layer_ = (bicep_current_layer_ + direction + bicep_layers_.size()) % bicep_layers_.size();
    }
    
    void addBicepLayer() {
        if (!bicep_mode_) return;
        
        // Add the current engine to bicep layers if not already present
        bool already_present = false;
        for (EngineType engine : bicep_layers_) {
            if (engine == current_engine_type_) {
                already_present = true;
                break;
            }
        }
        
        if (!already_present && bicep_layers_.size() < 5) {
            bicep_layers_.push_back(current_engine_type_);
        }
    }
    
    void removeBicepLayer() {
        if (!bicep_mode_ || bicep_layers_.empty()) return;
        
        if (bicep_current_layer_ < bicep_layers_.size()) {
            bicep_layers_.erase(bicep_layers_.begin() + bicep_current_layer_);
            if (bicep_current_layer_ >= bicep_layers_.size() && !bicep_layers_.empty()) {
                bicep_current_layer_ = bicep_layers_.size() - 1;
            }
        }
    }
    
    void toggleEuclideanRhythm() {
        euclidean_rhythm_.setActive(!euclidean_rhythm_.isActive());
    }
    
    void adjustEuclideanHits(int delta) {
        size_t current_hits = euclidean_rhythm_.getHits();
        size_t new_hits = std::clamp(static_cast<int>(current_hits) + delta, 0, 16);
        euclidean_rhythm_.setPattern(new_hits, euclidean_rhythm_.getRotation());
    }
    
    void adjustEuclideanRotation(int delta) {
        size_t current_rotation = euclidean_rhythm_.getRotation();
        size_t steps = euclidean_rhythm_.getSteps();
        size_t new_rotation = (current_rotation + delta + steps) % steps;
        euclidean_rhythm_.setPattern(euclidean_rhythm_.getHits(), new_rotation);
    }
    
    void toggleArpeggiator() {
        arpeggiator_.setActive(!arpeggiator_.isActive());
        if (!arpeggiator_.isActive()) {
            arpeggiator_.allNotesOff();
        }
    }
    
    void cycleArpPattern(int direction) {
        int current = static_cast<int>(arpeggiator_.getPattern());
        int count = static_cast<int>(Arpeggiator::Pattern::COUNT);
        current = (current + direction + count) % count;
        arpeggiator_.setPattern(static_cast<Arpeggiator::Pattern>(current));
    }
    
    void cycleArpSpeed(int direction) {
        int current = static_cast<int>(arpeggiator_.getSpeed());
        int count = static_cast<int>(Arpeggiator::Speed::COUNT);
        current = (current + direction + count) % count;
        arpeggiator_.setSpeed(static_cast<Arpeggiator::Speed>(current));
    }
    
    void adjustArpBPM(float delta) {
        float current_bpm = arpeggiator_.getBPM();
        arpeggiator_.setBPM(current_bpm + delta);
    }
    
    void cyclePlaitsModel(int direction) {
        if (current_engine_type_ != EngineType::PLAITS) return;
        
        // Update the current Plaits model for future voices
        static PlaitsModel current_plaits_model = PlaitsModel::VIRTUAL_ANALOG;
        int current_model = static_cast<int>(current_plaits_model);
        int model_count = static_cast<int>(PlaitsModel::COUNT);
        current_model = (current_model + direction + model_count) % model_count;
        current_plaits_model = static_cast<PlaitsModel>(current_model);
        
        // Reinitialize voices with new model
        voice_manager_.initializeVoices(current_engine_type_);
        
        // Set the model for all new voices
        voice_manager_.setPlaitsModel(current_plaits_model);
    }
    
    void toggleDrumMode() {
        drum_mode_ = !drum_mode_;
        if (drum_mode_) {
            // Mute synth voices when in drum mode
            allNotesOff();
        }
    }
    
    void toggleStepSequencer() {
        step_sequencer_.setPlaying(!step_sequencer_.isPlaying());
    }
    
    void clearDrumPattern() {
        step_sequencer_.clear();
    }
    
    void adjustDrumBPM(float delta) {
        float current_bpm = step_sequencer_.getBPM();
        step_sequencer_.setBPM(current_bpm + delta);
    }
    
    void cycleDrumTrack(int direction) {
        size_t current_track = step_sequencer_.getSelectedTrack();
        size_t new_track = (current_track + direction + MAX_DRUM_TRACKS) % MAX_DRUM_TRACKS;
        step_sequencer_.selectTrack(new_track);
    }
    
    void handleDrumKey(char key) {
        // Map piano keys to drum sounds for immediate triggering
        static const std::map<char, DrumType> key_to_drum = {
            {'z', DrumType::KICK},
            {'x', DrumType::SNARE},
            {'c', DrumType::HIHAT_CLOSED},
            {'v', DrumType::HIHAT_OPEN},
            {'b', DrumType::CLAP},
            {'n', DrumType::CRASH},
            {'m', DrumType::TOM_HIGH},
            {',', DrumType::TOM_LOW}
        };
        
        auto it = key_to_drum.find(key);
        if (it != key_to_drum.end()) {
            drum_synth_.triggerDrum(it->second, 1.0f);
        }
    }
    
    void handleStepInput(char ch) {
        if (!drum_mode_) return;
        
        // In drum mode, use a simple approach - just toggle current step
        if (ch == 'Z') {
            // Toggle current step for selected track
            size_t track = step_sequencer_.getSelectedTrack();
            size_t step = step_sequencer_.getCurrentStep();
            step_sequencer_.toggleStep(track, step);
        }
    }
    
    void toggleReverb() {
        reverb_enabled_ = !reverb_enabled_;
    }
    
    void cycleReverbAlgorithm(int direction) {
        int current = static_cast<int>(reverb_.getAlgorithm());
        int count = static_cast<int>(LushReverb::Algorithm::COUNT);
        current = (current + direction + count) % count;
        reverb_.setAlgorithm(static_cast<LushReverb::Algorithm>(current));
    }
    
    void adjustReverbSize(float delta) {
        float current_size = reverb_.getSize();
        reverb_.setSize(current_size + delta);
    }
    
    void adjustReverbMix(float delta) {
        float current_mix = reverb_.getMix();
        reverb_.setMix(current_mix + delta);
    }
    
    void adjustReverbSend(float delta) {
        reverb_send_ = std::clamp(reverb_send_ + delta, 0.0f, 1.0f);
    }
    
    // Filter control functions
    void cycleFilterType(int direction) {
        int current_type = static_cast<int>(global_filter_.getFilterType());
        int max_types = static_cast<int>(MultiModeFilter::FilterType::COUNT);
        current_type = (current_type + direction + max_types) % max_types;
        global_filter_.setFilterType(static_cast<MultiModeFilter::FilterType>(current_type));
    }
    
    void adjustFilterCutoff(float delta) {
        float current = global_filter_.getCutoff();
        global_filter_.setCutoff(std::clamp(current + delta * 2000.0f, 20.0f, 20000.0f));
    }
    
    void adjustFilterResonance(float delta) {
        float current = global_filter_.getResonance();
        global_filter_.setResonance(std::clamp(current + delta * 5.0f, 0.1f, 10.0f));
    }
    
    // Preset control functions
    void cyclePreset(int direction) {
        preset_manager_.cyclePreset(direction);
        loadCurrentPreset();
    }
    
    void saveCurrentPreset() {
        PresetManager::Preset preset;
        preset.name = "User Preset " + std::to_string(preset_manager_.getCurrentSlot());
        preset.engine_type = current_engine_type_;
        preset.plaits_model = static_cast<PlaitsModel>(0); // Default
        
        // Save all current parameters
        for (size_t i = 0; i < parameters_.size(); i++) {
            preset.synth_params[i] = parameters_[i];
        }
        
        // Save filter settings
        preset.filter_type = global_filter_.getFilterType();
        preset.filter_cutoff = global_filter_.getCutoff();
        preset.filter_resonance = global_filter_.getResonance();
        
        // Save reverb settings
        preset.reverb_algorithm = reverb_.getCurrentAlgorithm();
        preset.reverb_size = reverb_.getSize();
        preset.reverb_damping = reverb_.getDamping();
        preset.reverb_diffusion = reverb_.getDiffusion();
        preset.reverb_mix = reverb_.getMix();
        preset.reverb_send = reverb_send_;
        preset.reverb_enabled = reverb_enabled_;
        
        preset_manager_.savePreset(preset);
    }
    
    void loadCurrentPreset() {
        auto preset = preset_manager_.getCurrentPreset();
        if (preset.has_value()) {
            const auto& p = preset.value();
            
            // Load engine
            selectEngine(p.engine_type);
            
            // Load parameters
            for (size_t i = 0; i < parameters_.size() && i < p.synth_params.size(); i++) {
                parameters_[i] = p.synth_params[i];
            }
            updateAllParameters();
            
            // Load filter settings
            global_filter_.setFilterType(p.filter_type);
            global_filter_.setCutoff(p.filter_cutoff);
            global_filter_.setResonance(p.filter_resonance);
            
            // Load reverb settings
            reverb_.setAlgorithm(p.reverb_algorithm);
            reverb_.setSize(p.reverb_size);
            reverb_.setDamping(p.reverb_damping);
            reverb_.setDiffusion(p.reverb_diffusion);
            reverb_.setMix(p.reverb_mix);
            reverb_send_ = p.reverb_send;
            reverb_enabled_ = p.reverb_enabled;
        }
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
        return getEngineName(current_engine_type_);
    }
    
    const char* getEngineName(EngineType engine_type) const {
        switch (engine_type) {
            case EngineType::SUBTRACTIVE: return "Subtractive";
            case EngineType::FM: return "FM";
            case EngineType::SUB_BASS: return "Sub Bass";
            case EngineType::WARM_PAD: return "Warm Pad";
            case EngineType::BRIGHT_LEAD: return "Bright Lead";
            case EngineType::STRING_ENSEMBLE: return "String Ensemble";
            case EngineType::GRANULAR: return "Granular";
            case EngineType::PLAITS: return "Plaits";
            default: return "Unknown";
        }
    }
    
    void printInterface() {
        // Clear screen and move cursor to top
        std::cout << "\033[2J\033[H";
        
        std::cout << "=== ether Terminal Synthesizer (Polyphonic) ===" << std::endl << std::endl;
        
        // Current engine and voice info
        std::cout << "Engine: " << getEngineName();
        if (current_engine_type_ == EngineType::PLAITS) {
            std::cout << " | Model: " << voice_manager_.getCurrentPlaitsModelName();
        }
        std::cout << std::endl;
        std::cout << "Active Voices: " << voice_manager_.getActiveVoiceCount() << "/32" << std::endl;
        
        // Mode status
        if (chord_mode_) {
            std::cout << "🎵 CHORD MODE: " << chord_generator_.getChordTypeName() 
                      << " (" << chord_generator_.getVoicingName() << ")";
        } else {
            std::cout << "Single Note Mode";
        }
        
        if (bicep_mode_) {
            std::cout << " | 💪 BICEP MODE (Intensity: " << std::fixed << std::setprecision(1) 
                      << bicep_intensity_ << ")";
            if (!bicep_layers_.empty()) {
                std::cout << std::endl << "   Layers: ";
                for (size_t i = 0; i < bicep_layers_.size(); i++) {
                    if (i == bicep_current_layer_) std::cout << "[";
                    std::cout << getEngineName(bicep_layers_[i]);
                    if (i == bicep_current_layer_) std::cout << "]";
                    if (i < bicep_layers_.size() - 1) std::cout << " + ";
                }
            }
        }
        std::cout << std::endl;
        
        // Filter status
        std::cout << "🎛 FILTER: " << global_filter_.getFilterTypeName()
                  << " | Cutoff: " << std::fixed << std::setprecision(0) << global_filter_.getCutoff() << "Hz"
                  << " | Q: " << std::setprecision(1) << global_filter_.getResonance() << std::endl;
        
        // Preset status
        std::cout << "💾 PRESET: Slot " << preset_manager_.getCurrentSlot() + 1 << "/32";
        auto current_preset = preset_manager_.getCurrentPreset();
        if (current_preset.has_value()) {
            std::cout << " - " << current_preset.value().name;
        } else {
            std::cout << " - [Empty]";
        }
        std::cout << std::endl;
        
        // Euclidean rhythm status
        if (euclidean_rhythm_.isActive()) {
            std::cout << "🥁 EUCLIDEAN RHYTHM: " << euclidean_rhythm_.getHits() 
                      << "/" << euclidean_rhythm_.getSteps() 
                      << " (Rot: " << euclidean_rhythm_.getRotation() << ")" << std::endl;
            std::cout << "   Pattern: " << euclidean_rhythm_.getPatternString() << std::endl;
        }
        
        // Arpeggiator status
        if (arpeggiator_.isActive()) {
            std::cout << "🎹 ARPEGGIATOR: " << arpeggiator_.getPatternName() 
                      << " | " << arpeggiator_.getSpeedName() 
                      << " | " << std::fixed << std::setprecision(0) << arpeggiator_.getBPM() << " BPM" << std::endl;
            std::cout << "   Held Notes: " << arpeggiator_.getHeldNotesCount()
                      << " | Sequence: " << arpeggiator_.getSequenceVisualization() << std::endl;
        }
        
        // Drum machine status
        if (drum_mode_) {
            std::cout << "🥁 DRUM MODE: " << (step_sequencer_.isPlaying() ? "PLAYING" : "STOPPED")
                      << " | " << std::fixed << std::setprecision(0) << step_sequencer_.getBPM() << " BPM" << std::endl;
            
            // Show current track
            size_t track = step_sequencer_.getSelectedTrack();
            DrumType drum_type = step_sequencer_.getTrackDrumType(track);
            std::cout << "   Track " << (track + 1) << ": " << drum_synth_.getDrumName(drum_type)
                      << (step_sequencer_.isMuted(track) ? " (MUTED)" : "") << std::endl;
            
            // Show pattern visualization
            std::cout << "   Steps: " << step_sequencer_.getPatternVisualization() << std::endl;
            
            // Show current track's pattern
            std::cout << "   Track: ";
            for (size_t step = 0; step < MAX_DRUM_STEPS; step++) {
                if (step == step_sequencer_.getCurrentStep() && step_sequencer_.isPlaying()) {
                    std::cout << (step_sequencer_.getStep(track, step) ? "▶" : "▷");
                } else {
                    std::cout << (step_sequencer_.getStep(track, step) ? "●" : "○");
                }
            }
            std::cout << std::endl;
        }
        
        // Reverb status
        if (reverb_enabled_) {
            std::cout << "🎵 REVERB: " << reverb_.getAlgorithmName()
                      << " | Size: " << std::fixed << std::setprecision(2) << reverb_.getSize()
                      << " | Mix: " << reverb_.getMix()
                      << " | Send: " << reverb_send_ << std::endl;
        } else {
            std::cout << "⏸  Reverb: OFF" << std::endl;
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
        std::cout << "  ENGINES: 1: Subtractive   [: FM         ]: Sub Bass" << std::endl;
        std::cout << "           \\: Warm Pad      -: Bright Lead =: Strings" << std::endl;
        std::cout << "           `: Granular      4: Plaits" << std::endl;
        if (current_engine_type_ == EngineType::PLAITS) {
            std::cout << "  PLAITS:  M/N: Cycle synthesis models" << std::endl;
        }
        std::cout << "  SYNTH:   a/A: Harmonics    f/F: Timbre" << std::endl;
        std::cout << "           p/P: Morph        o/O: Volume" << std::endl;
        std::cout << "  ADSR:    k/K: Attack       l/L: Decay" << std::endl;
        std::cout << "           ;/:: Sustain      '/\": Release" << std::endl;
        std::cout << "  CHORDS:  0: Toggle chord mode" << std::endl;
        std::cout << "           9/(: Chord type   8/*: Voicing" << std::endl;
        std::cout << "  BICEP:   B: Toggle bicep mode   i/I: Intensity" << std::endl;
        std::cout << "           {/}: Navigate layers  +: Add current engine" << std::endl;
        std::cout << "           _: Remove layer" << std::endl;
        std::cout << "  FILTER:  </> : Filter type      {}:Cutoff  (): Resonance" << std::endl;
        std::cout << "  PRESETS: L/;: Browse presets    :: Save current preset" << std::endl;
        std::cout << "  RHYTHM:  E: Toggle euclidean   ,/.: Hits   </>/: Rotation" << std::endl;
        std::cout << "  ARPEGG:  /: Toggle arp   ?: Pattern   S: Speed" << std::endl;
        std::cout << "           T/R: BPM +/-" << std::endl;
        std::cout << "  DRUMS:   D: Toggle drum mode   X: Play/Stop sequencer" << std::endl;
        std::cout << "           V/G: BPM +/-   H/J: Select track   C: Clear" << std::endl;
        if (drum_mode_) {
            std::cout << "  STEPS:   Z: Toggle current step" << std::endl;
            std::cout << "  TRIGGER: ZXCVBNM,: Kick/Snare/HH/HH/Clap/Crash/Tom/Tom" << std::endl;
        }
        std::cout << "  REVERB:  #: Toggle   ^: Algorithm   &/!: Size +/-" << std::endl;
        std::cout << "           @/$: Mix +/-" << std::endl;
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