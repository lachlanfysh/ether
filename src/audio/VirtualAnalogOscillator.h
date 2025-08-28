#pragma once
#include <cmath>

/**
 * VirtualAnalogOscillator - Virtual analog oscillator with multiple waveforms
 */
class VirtualAnalogOscillator {
public:
    enum Waveform {
        SAWTOOTH,
        SQUARE,
        TRIANGLE,
        SINE,
        NOISE
    };
    
    VirtualAnalogOscillator() = default;
    ~VirtualAnalogOscillator() = default;
    
    bool initialize(float sampleRate) {
        sampleRate_ = sampleRate;
        return true;
    }
    
    void shutdown() {}
    
    void setWaveform(Waveform waveform) { waveform_ = waveform; }
    void setFrequency(float frequency) { frequency_ = frequency; }
    void setLevel(float level) { level_ = level; }
    void setPulseWidth(float width) { pulseWidth_ = width; }
    void resetPhase() { phase_ = 0.0f; }
    
    float processSample() {
        phase_ += frequency_ / sampleRate_;
        while (phase_ >= 1.0f) phase_ -= 1.0f;
        
        float output = 0.0f;
        switch (waveform_) {
            case SINE:
                output = std::sin(phase_ * 2.0f * PI_CONST);
                break;
            case SAWTOOTH:
                output = (phase_ * 2.0f) - 1.0f;
                break;
            case SQUARE:
                output = (phase_ < pulseWidth_) ? 1.0f : -1.0f;
                break;
            case TRIANGLE:
                output = (phase_ < 0.5f) ? (phase_ * 4.0f - 1.0f) : (3.0f - phase_ * 4.0f);
                break;
            case NOISE:
                output = ((rand() / (float)RAND_MAX) * 2.0f) - 1.0f;
                break;
        }
        
        return output * level_;
    }
    
private:
    static constexpr float PI_CONST = 3.14159265359f;
    float sampleRate_ = 44100.0f;
    Waveform waveform_ = SAWTOOTH;
    float frequency_ = 440.0f;
    float level_ = 1.0f;
    float pulseWidth_ = 0.5f;
    float phase_ = 0.0f;
};