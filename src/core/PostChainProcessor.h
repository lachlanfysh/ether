#pragma once

#include "CoreParameters.h"
#include <cmath>

namespace EtherSynth {

class SimpleFilter {
private:
    float z1 = 0.0f, z2 = 0.0f;
    float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f, b1 = 0.0f, b2 = 0.0f;
    
public:
    void setLowpass(float cutoff, float resonance, float sampleRate = 48000.0f) {
        float omega = 2.0f * M_PI * cutoff / sampleRate;
        float cos_omega = std::cos(omega);
        float sin_omega = std::sin(omega);
        float alpha = sin_omega / (2.0f * resonance);
        
        float norm = 1.0f / (1.0f + alpha);
        a0 = (1.0f - cos_omega) * 0.5f * norm;
        a1 = (1.0f - cos_omega) * norm;
        a2 = a0;
        b1 = -2.0f * cos_omega * norm;
        b2 = (1.0f - alpha) * norm;
    }
    
    void setHighpass(float cutoff, float sampleRate = 48000.0f) {
        float omega = 2.0f * M_PI * cutoff / sampleRate;
        float cos_omega = std::cos(omega);
        float sin_omega = std::sin(omega);
        float alpha = sin_omega / 2.0f; // Q = 0.707
        
        float norm = 1.0f / (1.0f + alpha);
        a0 = (1.0f + cos_omega) * 0.5f * norm;
        a1 = -(1.0f + cos_omega) * norm;
        a2 = a0;
        b1 = -2.0f * cos_omega * norm;
        b2 = (1.0f - alpha) * norm;
    }
    
    float process(float input) {
        float output = a0 * input + a1 * z1 + a2 * z2 - b1 * z1 - b2 * z2;
        z2 = z1;
        z1 = input;
        return output;
    }
    
    void reset() {
        z1 = z2 = 0.0f;
    }
};

class ADSREnvelope {
private:
    enum Stage { ATTACK, DECAY, SUSTAIN, RELEASE, IDLE };
    Stage stage = IDLE;
    float level = 0.0f;
    float attack_rate = 0.0f, decay_rate = 0.0f, sustain_level = 0.0f, release_rate = 0.0f;
    
public:
    void setParameters(float attack_time, float decay_time, float sustain_lvl, float release_time, float sampleRate = 48000.0f) {
        attack_rate = (attack_time > 0.001f) ? 1.0f / (attack_time * sampleRate) : 1000.0f;
        decay_rate = (decay_time > 0.001f) ? 1.0f / (decay_time * sampleRate) : 1000.0f;
        sustain_level = std::clamp(sustain_lvl, 0.0f, 1.0f);
        release_rate = (release_time > 0.001f) ? 1.0f / (release_time * sampleRate) : 1000.0f;
    }
    
    void noteOn() {
        stage = ATTACK;
    }
    
    void noteOff() {
        stage = RELEASE;
    }
    
    float process() {
        switch (stage) {
            case ATTACK:
                level += attack_rate;
                if (level >= 1.0f) {
                    level = 1.0f;
                    stage = DECAY;
                }
                break;
                
            case DECAY:
                level -= decay_rate * (level - sustain_level);
                if (level <= sustain_level + 0.001f) {
                    level = sustain_level;
                    stage = SUSTAIN;
                }
                break;
                
            case SUSTAIN:
                level = sustain_level;
                break;
                
            case RELEASE:
                level -= release_rate * level;
                if (level <= 0.001f) {
                    level = 0.0f;
                    stage = IDLE;
                }
                break;
                
            case IDLE:
                level = 0.0f;
                break;
        }
        
        return level;
    }
    
    bool isActive() const { return stage != IDLE; }
    void reset() { stage = IDLE; level = 0.0f; }
};

class PostChainProcessor {
private:
    SimpleFilter hpf, lpf;
    ADSREnvelope envelope;
    bool envelope_active = false;
    float sampleRate = 48000.0f;
    
    // Soft clipping using tanh approximation
    float softClip(float x, float amount) {
        if (amount <= 0.0f) return x;
        
        float drive = 1.0f + amount * 3.0f;
        x *= drive;
        
        // Fast tanh approximation
        if (x > 3.0f) return 0.995f / drive;
        if (x < -3.0f) return -0.995f / drive;
        
        float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2) / drive;
    }
    
public:
    void setSampleRate(float sr) {
        sampleRate = sr;
    }
    
    void updateParameters(const CoreParams& params, bool engine_has_native_filter = false) {
        // Update HPF (always applied)
        float hpf_cutoff = ParameterUtils::getScaledValue(PARAM_HPF, params[PARAM_HPF]);
        hpf.setHighpass(hpf_cutoff, sampleRate);
        
        // Update LPF - use engine native filter if available, otherwise fallback
        float lpf_cutoff, lpf_q;
        if (engine_has_native_filter) {
            // Use TIMBRE as fallback cutoff mapping
            lpf_cutoff = ParameterUtils::expScale(params[PARAM_TIMBRE], 200.0f, 20000.0f);
            lpf_q = 0.707f; // Default Q
        } else {
            // Use dedicated filter parameters
            lpf_cutoff = ParameterUtils::getScaledValue(PARAM_FILTER_CUTOFF, params[PARAM_FILTER_CUTOFF]);
            lpf_q = ParameterUtils::getScaledValue(PARAM_FILTER_RESONANCE, params[PARAM_FILTER_RESONANCE]);
        }
        lpf.setLowpass(lpf_cutoff, lpf_q, sampleRate);
        
        // Update envelope
        float attack = ParameterUtils::getScaledValue(PARAM_ATTACK, params[PARAM_ATTACK]);
        float decay = ParameterUtils::getScaledValue(PARAM_DECAY, params[PARAM_DECAY]);
        float sustain = params[PARAM_SUSTAIN];
        float release = ParameterUtils::getScaledValue(PARAM_RELEASE, params[PARAM_RELEASE]);
        envelope.setParameters(attack, decay, sustain, release, sampleRate);
    }
    
    void noteOn() {
        envelope.noteOn();
        envelope_active = true;
    }
    
    void noteOff() {
        envelope.noteOff();
    }
    
    struct StereoSample {
        float left, right;
    };
    
    StereoSample process(float mono_input, const CoreParams& params, bool apply_envelope = true) {
        float signal = mono_input;
        
        // 1. Pre-gain boost (HARMONICS fallback)
        float harmonics_gain = 1.0f + params[PARAM_HARMONICS] * 2.0f; // +0→+6dB
        signal *= harmonics_gain;
        
        // 2. Amplitude scaling
        signal *= params[PARAM_AMPLITUDE];
        
        // 3. HPF (always active)
        signal = hpf.process(signal);
        
        // 4. LPF+Resonance
        signal = lpf.process(signal);
        
        // 5. Soft clipping
        signal = softClip(signal, params[PARAM_CLIP]);
        
        // 6. ADSR envelope (if active)
        if (apply_envelope && envelope_active) {
            float env_level = envelope.process();
            signal *= env_level;
            
            if (!envelope.isActive()) {
                envelope_active = false;
            }
        }
        
        // 7. Volume scaling
        float volume = ParameterUtils::volumeScale(params[PARAM_VOLUME]);
        signal *= volume;
        
        // 8. Pan and stereo output
        float left_gain, right_gain;
        ParameterUtils::panLaw(params[PARAM_PAN], left_gain, right_gain);
        
        return {signal * left_gain, signal * right_gain};
    }
    
    bool isEnvelopeActive() const {
        return envelope_active;
    }
    
    void reset() {
        hpf.reset();
        lpf.reset();
        envelope.reset();
        envelope_active = false;
    }
};

} // namespace EtherSynth