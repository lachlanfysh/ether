#include "DSPUtils.h"
#include <cmath>
#include <algorithm>

namespace DSP {

// Pre-computed BLEP table (simplified version)
const float BLEP::blepTable[TABLE_SIZE] = {
    // This would be pre-computed offline for efficiency
    // For now, we'll compute on-demand in getBLEP()
    0.0f // placeholder
};

float BLEP::getBLEP(float phase, float freqRatio) {
    // Simplified BLEP implementation
    // In production, this would use the pre-computed table
    if (phase < 0.0f || phase >= 1.0f) return 0.0f;
    
    // Basic polynomial BLEP kernel
    if (phase < 0.5f) {
        float t = 2.0f * phase;
        return t * t * (3.0f - 2.0f * t) - 1.0f;
    } else {
        float t = 2.0f * (phase - 0.5f);
        return 1.0f - t * t * (3.0f - 2.0f * t);
    }
}

float BLEP::getBLAMP(float phase, float freqRatio) {
    // Integrated BLEP (BLAMP) for ramp discontinuities
    if (phase < 0.0f || phase >= 1.0f) return 0.0f;
    
    if (phase < 0.5f) {
        float t = 2.0f * phase;
        return t * t * t * (t - 2.0f) + t;
    } else {
        float t = 2.0f * (phase - 0.5f);
        return t * (t * t * (2.0f - t) + 1.0f) - 1.0f;
    }
}

// SVF Implementation
void SVF::updateCoeffs() {
    float freq = std::clamp(cutoff_, 10.0f, sampleRate_ * 0.45f);
    g_ = std::tan(PI * freq / sampleRate_);
    k_ = 2.0f - 2.0f * resonance_;
    
    float denom = 1.0f + g_ * (g_ + k_);
    a1_ = g_ / denom;
    a2_ = g_ * g_ / denom;  
    a3_ = g_ * k_ / denom;
}

float SVF::process(float input) {
    float v3 = input - ic2eq_;
    float v1 = a1_ * ic1eq_ + a2_ * v3;
    float v2 = ic2eq_ + a2_ * ic1eq_ + a3_ * v3;
    
    ic1eq_ = 2.0f * v1 - ic1eq_;
    ic2eq_ = 2.0f * v2 - ic2eq_;
    
    switch (mode_) {
        case LP: return v2;
        case HP: return input - k_ * v1 - v2;  
        case BP: return v1;
        case NOTCH: return input - k_ * v1;
        default: return v2;
    }
}

void SVF::processBlock(const float* input, float* output, int count) {
    for (int i = 0; i < count; ++i) {
        output[i] = process(input[i]);
    }
}

// ADSR Implementation  
void ADSR::setADSR(float attack_ms, float decay_ms, float sustain, float release_ms) {
    attack_ms_ = std::max(0.1f, attack_ms);
    decay_ms_ = std::max(1.0f, decay_ms);
    sustain_ = std::clamp(sustain, 0.0f, 1.0f);
    release_ms_ = std::max(1.0f, release_ms);
    updateRates();
}

void ADSR::updateRates() {
    attackRate_ = msToRate(attack_ms_, sampleRate_);
    decayRate_ = msToRate(decay_ms_, sampleRate_);
    releaseRate_ = msToRate(release_ms_, sampleRate_);
}

float ADSR::msToRate(float time_ms, float sampleRate) {
    // Convert milliseconds to exponential rate
    float samples = time_ms * sampleRate * 0.001f;
    return 1.0f - std::exp(-1.0f / samples);
}

float ADSR::process() {
    switch (stage_) {
        case ATTACK:
            output_ += (1.0f - output_) * attackRate_;
            if (output_ >= 0.999f) {
                output_ = 1.0f;
                stage_ = DECAY;
            }
            break;
            
        case DECAY:
            output_ += (sustainLevel_ - output_) * decayRate_;
            if (std::abs(output_ - sustainLevel_) < 0.001f) {
                stage_ = SUSTAIN;
            }
            break;
            
        case SUSTAIN:
            output_ = sustainLevel_;
            break;
            
        case RELEASE:
            output_ += (0.0f - output_) * releaseRate_;
            if (output_ <= 0.001f) {
                output_ = 0.0f;
                stage_ = IDLE;
            }
            break;
            
        case IDLE:
        default:
            output_ = 0.0f;
            break;
    }
    
    return output_;
}

void ADSR::processBlock(float* output, int count) {
    for (int i = 0; i < count; ++i) {
        output[i] = process();
    }
}

// Oscillator implementations
namespace Oscillator {
    float blSawtooth(float& phase, float increment) {
        float output = 2.0f * phase - 1.0f;
        
        // Add BLEP correction at discontinuity
        if (phase + increment >= 1.0f) {
            float overshoot = (phase + increment) - 1.0f;
            output += BLEP::getBLEP(overshoot / increment);
        }
        
        phase += increment;
        if (phase >= 1.0f) phase -= 1.0f;
        
        return output;
    }
    
    float blSquare(float& phase, float increment, float pulseWidth) {
        float output = (phase < pulseWidth) ? 1.0f : -1.0f;
        
        // BLEP correction at rising edge (0 -> 1)
        if (phase < pulseWidth && (phase + increment) >= pulseWidth) {
            float overshoot = (phase + increment) - pulseWidth;
            output -= 2.0f * BLEP::getBLEP(overshoot / increment);
        }
        
        // BLEP correction at falling edge (1 -> 0) 
        if (phase + increment >= 1.0f) {
            float overshoot = (phase + increment) - 1.0f;
            output += 2.0f * BLEP::getBLEP(overshoot / increment);
        }
        
        phase += increment;
        if (phase >= 1.0f) phase -= 1.0f;
        
        return output;
    }
}

// Random implementation
float Random::normal(float mean, float stddev) {
    if (hasSpare_) {
        hasSpare_ = false;
        return spare_ * stddev + mean;
    }
    
    hasSpare_ = true;
    
    // Box-Muller transform
    float u = uniform();
    float v = uniform();
    float mag = stddev * std::sqrt(-2.0f * std::log(u));
    spare_ = mag * std::cos(TWO_PI * v);
    return mag * std::sin(TWO_PI * v) + mean;
}

} // namespace DSP