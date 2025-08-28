#pragma once
#include <cmath>
#include <algorithm>
#include "../core/Types.h"

namespace DSP {

// Mathematical constants
constexpr float PI = M_PI;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = PI * 0.5f;
constexpr float INV_PI = 1.0f / PI;

/**
 * BLEP (Band-Limited Step) utilities for anti-aliased discontinuities
 */
class BLEP {
public:
    static constexpr int OVERSAMPLING = 16;
    static constexpr int TABLE_SIZE = 2048;
    static constexpr int KERNEL_SIZE = 64;
    
    // Pre-computed BLEP table for efficiency
    static const float blepTable[TABLE_SIZE];
    
    // Get BLEP correction value for given phase
    static float getBLEP(float phase, float freqRatio = 1.0f);
    
    // Integrated BLEP (BLAMP) for ramp discontinuities  
    static float getBLAMP(float phase, float freqRatio = 1.0f);
};

/**
 * State Variable Filter (SVF) for channel strip and synthesis
 * Supports LP, HP, BP, Notch modes with resonance compensation
 */
class SVF {
public:
    enum Mode { LP, HP, BP, NOTCH };
    
    SVF() : mode_(LP), cutoff_(1000.0f), resonance_(0.1f), sampleRate_(48000.0f) {
        reset();
    }
    
    void setSampleRate(float sr) { sampleRate_ = sr; updateCoeffs(); }
    void setCutoff(float freq) { cutoff_ = freq; updateCoeffs(); }
    void setResonance(float res) { resonance_ = std::clamp(res, 0.0f, 0.95f); updateCoeffs(); }
    void setMode(Mode mode) { mode_ = mode; }
    
    float process(float input);
    void processBlock(const float* input, float* output, int count);
    
    void reset() {
        z1_ = z2_ = 0.0f;
        ic1eq_ = ic2eq_ = 0.0f;
    }
    
private:
    Mode mode_;
    float cutoff_, resonance_, sampleRate_;
    float g_, k_, a1_, a2_, a3_;
    float z1_, z2_;        // delay elements
    float ic1eq_, ic2eq_;  // integrated outputs
    
    void updateCoeffs();
};

/**
 * ADSR Envelope Generator with exponential curves
 */
class ADSR {
public:
    enum Stage { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };
    
    ADSR() : stage_(IDLE), output_(0.0f), sampleRate_(48000.0f) {
        setADSR(5.0f, 200.0f, 0.7f, 300.0f); // default 5ms/200ms/0.7/300ms
    }
    
    void setSampleRate(float sr) { sampleRate_ = sr; updateRates(); }
    void setADSR(float attack_ms, float decay_ms, float sustain, float release_ms);
    
    void noteOn() { stage_ = ATTACK; }
    void noteOff() { if (stage_ != IDLE) stage_ = RELEASE; }
    void reset() { stage_ = IDLE; output_ = 0.0f; }
    
    float process();
    void processBlock(float* output, int count);
    
    bool isActive() const { return stage_ != IDLE; }
    Stage getStage() const { return stage_; }
    float getOutput() const { return output_; }
    
private:
    Stage stage_;
    float output_;
    float sampleRate_;
    
    // Envelope parameters
    float attackRate_, decayRate_, sustainLevel_, releaseRate_;
    float attack_ms_, decay_ms_, sustain_, release_ms_;
    
    void updateRates();
    static float msToRate(float time_ms, float sampleRate);
};

/**
 * Smooth parameter interpolation for click-free parameter changes
 */
class SmoothParam {
public:
    SmoothParam(float initial = 0.0f, float time_ms = 5.0f) 
        : target_(initial), current_(initial), sampleRate_(48000.0f) {
        setSmoothing(time_ms);
    }
    
    void setSampleRate(float sr) { sampleRate_ = sr; updateCoeff(); }
    void setSmoothing(float time_ms) { smoothTime_ms_ = time_ms; updateCoeff(); }
    void setTarget(float target) { target_ = target; }
    void setImmediate(float value) { target_ = current_ = value; }
    
    float process() {
        current_ += (target_ - current_) * coeff_;
        return current_;
    }
    
    float getValue() const { return current_; }
    bool isStable() const { return std::abs(target_ - current_) < 1e-6f; }
    
private:
    float target_, current_;
    float sampleRate_, smoothTime_ms_, coeff_;
    
    void updateCoeff() {
        // Exponential smoothing coefficient
        float samples = smoothTime_ms_ * sampleRate_ * 0.001f;
        coeff_ = 1.0f - std::exp(-1.0f / samples);
    }
};

/**
 * Interpolation utilities
 */
namespace Interp {
    // Linear interpolation
    inline float linear(float a, float b, float t) {
        return a + t * (b - a);
    }
    
    // Cubic interpolation (4-point)
    inline float cubic(float a, float b, float c, float d, float t) {
        float t2 = t * t;
        float a0 = d - c - a + b;
        float a1 = a - b - a0;
        float a2 = c - a;
        float a3 = b;
        return a0 * t * t2 + a1 * t2 + a2 * t + a3;
    }
    
    // Table lookup with linear interpolation
    inline float table(const float* table, int size, float phase) {
        phase = std::fmod(phase, 1.0f);
        if (phase < 0.0f) phase += 1.0f;
        
        float fidx = phase * size;
        int idx = static_cast<int>(fidx);
        float frac = fidx - idx;
        
        int nextIdx = (idx + 1) % size;
        return linear(table[idx], table[nextIdx], frac);
    }
}

/**
 * Oscillator utilities
 */
namespace Oscillator {
    // Convert MIDI note to frequency
    inline float noteToFreq(float note) {
        return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
    }
    
    // Generate phase increment for given frequency
    inline float freqToIncrement(float freq, float sampleRate) {
        return freq / sampleRate;
    }
    
    // Wrap phase to [0, 1] range
    inline float wrapPhase(float phase) {
        return phase - std::floor(phase);
    }
    
    // Anti-aliased sawtooth using BLEP
    float blSawtooth(float& phase, float increment);
    
    // Anti-aliased square using BLEP
    float blSquare(float& phase, float increment, float pulseWidth = 0.5f);
    
    // Simple sine oscillator
    inline float sine(float phase) {
        return std::sin(phase * TWO_PI);
    }
}

/**
 * Audio utilities
 */
namespace Audio {
    // Decibel conversions
    inline float dbToLinear(float db) {
        return std::pow(10.0f, db / 20.0f);
    }
    
    inline float linearToDb(float linear) {
        return 20.0f * std::log10(std::max(linear, 1e-10f));
    }
    
    // Soft clipping/saturation
    inline float softClip(float x) {
        if (x > 1.0f) return 2.0f / 3.0f;
        if (x < -1.0f) return -2.0f / 3.0f;
        return x - (x * x * x) / 3.0f;
    }
    
    // Tanh saturation
    inline float tanhSat(float x, float drive = 1.0f) {
        return std::tanh(x * drive) / std::tanh(drive);
    }
    
    // DC blocking filter
    class DCBlocker {
        float x1_ = 0.0f, y1_ = 0.0f;
        static constexpr float R = 0.995f;
    public:
        float process(float input) {
            float output = input - x1_ + R * y1_;
            x1_ = input;
            y1_ = output;
            return output;
        }
    };
    
    // Peak follower for level monitoring
    class PeakFollower {
        float envelope_ = 0.0f;
        float attackCoeff_ = 0.99f;
        float releaseCoeff_ = 0.9999f;
        
    public:
        PeakFollower() = default;
        
        void setSampleRate(float sampleRate) {
            setAttackTime(1.0f / sampleRate);  // 1 sample attack
            setReleaseTime(100.0f / sampleRate); // 100ms release  
        }
        
        void setAttackTime(float timeMs) {
            float samples = timeMs * 48000.0f * 0.001f; // Assume 48kHz base
            attackCoeff_ = std::exp(-1.0f / samples);
        }
        
        void setReleaseTime(float timeMs) {
            float samples = timeMs * 48000.0f * 0.001f;
            releaseCoeff_ = std::exp(-1.0f / samples);
        }
        
        float process(float input) {
            float absInput = std::abs(input);
            if (absInput > envelope_) {
                envelope_ = attackCoeff_ * envelope_ + (1.0f - attackCoeff_) * absInput;
            } else {
                envelope_ = releaseCoeff_ * envelope_ + (1.0f - releaseCoeff_) * absInput;
            }
            return envelope_;
        }
        
        void reset() {
            envelope_ = 0.0f;
        }
        
        float getValue() const { return envelope_; }
    };
}

/**
 * Random number generation (deterministic for reproducibility)
 */
class Random {
public:
    explicit Random(uint32_t seed = 0x12345678) : state_(seed) {}
    
    void setSeed(uint32_t seed) { state_ = seed; }
    
    // Xorshift32 for speed
    uint32_t next() {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 17;
        state_ ^= state_ << 5;
        return state_;
    }
    
    // Uniform float in [0, 1)
    float uniform() {
        return next() * (1.0f / 4294967296.0f);
    }
    
    // Uniform float in [min, max)
    float uniform(float min, float max) {
        return min + uniform() * (max - min);
    }
    
    // Normal distribution (Box-Muller)
    float normal(float mean = 0.0f, float stddev = 1.0f);
    
private:
    uint32_t state_;
    bool hasSpare_ = false;
    float spare_;
};

} // namespace DSP