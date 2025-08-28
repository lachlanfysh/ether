#include "AdvancedLFO.h"
#include <algorithm>
#include <cstring>

namespace EtherSynth {

// Static member initialization
std::array<float, AdvancedLFO::SINE_TABLE_SIZE> AdvancedLFO::sineTable_;
bool AdvancedLFO::sineTableInitialized_ = false;

AdvancedLFO::AdvancedLFO() : randomSeed_(54321) {
    if (!sineTableInitialized_) {
        initializeSineTable();
    }
    
    customWavetable_.fill(0.0f);
    updatePhaseIncrement();
    updateRandomization();
}

void AdvancedLFO::setSettings(const LFOSettings& settings) {
    settings_ = settings;
    
    // Clamp values to valid ranges
    settings_.rate = std::clamp(settings_.rate, 0.01f, 100.0f);
    settings_.depth = std::clamp(settings_.depth, 0.0f, 1.0f);
    settings_.offset = std::clamp(settings_.offset, -1.0f, 1.0f);
    settings_.phase = std::clamp(settings_.phase, 0.0f, 1.0f);
    settings_.pulseWidth = std::clamp(settings_.pulseWidth, 0.1f, 0.9f);
    settings_.smooth = std::clamp(settings_.smooth, 0.0f, 1.0f);
    
    updatePhaseIncrement();
    updateRandomization();
    
    // Update smoothing coefficient
    smoothingCoeff_ = 1.0f - (settings_.smooth * 0.01f);
}

void AdvancedLFO::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updatePhaseIncrement();
}

void AdvancedLFO::setTempo(float bpm) {
    tempo_ = std::clamp(bpm, 60.0f, 200.0f);
    updatePhaseIncrement();
}

float AdvancedLFO::process() {
    if (!settings_.enabled) {
        return currentValue_;
    }
    
    float rawValue = 0.0f;
    
    // Handle different sync modes
    switch (settings_.syncMode) {
        case SyncMode::FREE_RUNNING:
        case SyncMode::TEMPO_SYNC:
        case SyncMode::KEY_SYNC: {
            // Apply frequency modulation
            float effectivePhaseInc = phaseIncrement_;
            if (settings_.fmAmount > 0.0f) {
                effectivePhaseInc *= (1.0f + fmInput_ * settings_.fmAmount);
            }
            
            // Generate waveform
            float adjustedPhase = wrap(phase_ + settings_.phase + phaseRandomOffset_);
            rawValue = generateWaveform(settings_.waveform, adjustedPhase);
            
            // Update phase
            phase_ += effectivePhaseInc * rateRandomMultiplier_;
            phase_ = wrap(phase_);
            break;
        }
        
        case SyncMode::ONE_SHOT: {
            if (phase_ < 1.0f) {
                float adjustedPhase = phase_ + settings_.phase;
                rawValue = generateWaveform(settings_.waveform, adjustedPhase);
                phase_ += phaseIncrement_;
            } else {
                rawValue = 0.0f; // Stay at 0 after completion
            }
            break;
        }
        
        case SyncMode::ENVELOPE: {
            updateEnvelope();
            rawValue = envValue_;
            break;
        }
    }
    
    // Apply amplitude modulation
    if (settings_.amAmount > 0.0f) {
        rawValue *= (1.0f + amInput_ * settings_.amAmount);
    }
    
    // Apply smoothing and final processing
    currentValue_ = applySmoothingAndModulation(rawValue);
    
    sampleCounter_++;
    return currentValue_;
}

void AdvancedLFO::processBlock(float* output, int blockSize) {
    // Optimized block processing for better performance
    for (int i = 0; i < blockSize; ++i) {
        output[i] = process();
    }
}

void AdvancedLFO::trigger() {
    phase_ = 0.0f;
    if (settings_.syncMode == SyncMode::ENVELOPE) {
        envStage_ = EnvStage::ATTACK;
        envValue_ = 0.0f;
        envTarget_ = 1.0f;
        envRate_ = 1.0f / (settings_.attack * sampleRate_);
    }
    updateRandomization();
}

void AdvancedLFO::noteOn(uint8_t velocity) {
    if (settings_.retrigger) {
        trigger();
    }
}

void AdvancedLFO::noteOff() {
    if (settings_.syncMode == SyncMode::ENVELOPE && 
        envStage_ != EnvStage::IDLE && envStage_ != EnvStage::RELEASE) {
        envStage_ = EnvStage::RELEASE;
        envTarget_ = 0.0f;
        envRate_ = 1.0f / (settings_.release * sampleRate_);
    }
}

// Private methods

void AdvancedLFO::updatePhaseIncrement() {
    if (settings_.syncMode == SyncMode::TEMPO_SYNC) {
        // Calculate phase increment based on tempo sync
        float cyclesPerSecond = (tempo_ / 60.0f) * getClockDivisionMultiplier();
        phaseIncrement_ = cyclesPerSecond / sampleRate_;
    } else {
        // Free running mode
        phaseIncrement_ = settings_.rate / sampleRate_;
    }
}

void AdvancedLFO::updateEnvelope() {
    switch (envStage_) {
        case EnvStage::ATTACK:
            envValue_ += envRate_;
            if (envValue_ >= envTarget_) {
                envValue_ = envTarget_;
                envStage_ = EnvStage::DECAY;
                envTarget_ = settings_.sustain;
                envRate_ = (1.0f - settings_.sustain) / (settings_.decay * sampleRate_);
            }
            break;
            
        case EnvStage::DECAY:
            envValue_ -= envRate_;
            if (envValue_ <= envTarget_) {
                envValue_ = envTarget_;
                envStage_ = EnvStage::SUSTAIN;
            }
            break;
            
        case EnvStage::SUSTAIN:
            // Hold sustain level
            break;
            
        case EnvStage::RELEASE:
            envValue_ -= envRate_;
            if (envValue_ <= 0.0f) {
                envValue_ = 0.0f;
                envStage_ = EnvStage::IDLE;
            }
            break;
            
        case EnvStage::IDLE:
            envValue_ = 0.0f;
            break;
    }
}

void AdvancedLFO::initializeSineTable() {
    for (size_t i = 0; i < SINE_TABLE_SIZE; ++i) {
        float phase = static_cast<float>(i) / SINE_TABLE_SIZE;
        sineTable_[i] = std::sin(2.0f * M_PI * phase);
    }
    sineTableInitialized_ = true;
}

float AdvancedLFO::generateWaveform(Waveform waveform, float phase) {
    switch (waveform) {
        case Waveform::SINE:
            return generateSine(phase);
        case Waveform::TRIANGLE:
            return generateTriangle(phase);
        case Waveform::SAWTOOTH_UP:
            return generateSawtooth(phase);
        case Waveform::SAWTOOTH_DOWN:
            return -generateSawtooth(phase);
        case Waveform::SQUARE:
            return generateSquare(phase);
        case Waveform::PULSE:
            return generateSquare(phase); // Use square with different pulse width
        case Waveform::NOISE:
            return generateNoise();
        case Waveform::SAMPLE_HOLD: {
            static float heldValue = 0.0f;
            static float lastPhase = 0.0f;
            if (phase < lastPhase) { // Wrapped around
                heldValue = generateNoise();
            }
            lastPhase = phase;
            return heldValue;
        }
        case Waveform::EXPONENTIAL_UP:
            return generateExponential(phase, true);
        case Waveform::EXPONENTIAL_DOWN:
            return generateExponential(phase, false);
        case Waveform::LOGARITHMIC:
            return generateLogarithmic(phase);
        case Waveform::CUSTOM:
            if (wavetableSize_ > 0) {
                return interpolateWavetable(customWavetable_.data(), wavetableSize_, phase);
            }
            return 0.0f;
        default:
            return 0.0f;
    }
}

inline float AdvancedLFO::generateSine(float phase) const {
    // High-quality table lookup with linear interpolation
    float tablePhase = wrap(phase) * (SINE_TABLE_SIZE - 1);
    int index = static_cast<int>(tablePhase);
    float frac = tablePhase - index;
    
    int nextIndex = (index + 1) % SINE_TABLE_SIZE;
    return sineTable_[index] * (1.0f - frac) + sineTable_[nextIndex] * frac;
}

inline float AdvancedLFO::generateTriangle(float phase) const {
    phase = wrap(phase);
    if (phase < 0.5f) {
        return 4.0f * phase - 1.0f; // Rising from -1 to 1
    } else {
        return 3.0f - 4.0f * phase; // Falling from 1 to -1
    }
}

inline float AdvancedLFO::generateSawtooth(float phase) const {
    return 2.0f * wrap(phase) - 1.0f; // -1 to 1
}

inline float AdvancedLFO::generateSquare(float phase) const {
    return (wrap(phase) < settings_.pulseWidth) ? 1.0f : -1.0f;
}

inline float AdvancedLFO::generateNoise() {
    // Generate smooth noise using linear congruential generator
    randomSeed_ = randomSeed_ * 1664525u + 1013904223u;
    return (static_cast<float>(randomSeed_) / 4294967295.0f) * 2.0f - 1.0f;
}

inline float AdvancedLFO::generateExponential(float phase, bool rising) const {
    phase = wrap(phase);
    if (rising) {
        return std::expm1(phase * 3.0f) / std::expm1(3.0f) * 2.0f - 1.0f;
    } else {
        return (1.0f - std::expm1((1.0f - phase) * 3.0f) / std::expm1(3.0f)) * 2.0f - 1.0f;
    }
}

inline float AdvancedLFO::generateLogarithmic(float phase) const {
    phase = wrap(phase);
    if (phase < 0.001f) phase = 0.001f; // Avoid log(0)
    return (std::log(phase * 10.0f + 1.0f) / std::log(11.0f)) * 2.0f - 1.0f;
}

float AdvancedLFO::applySmoothingAndModulation(float rawValue) {
    // Apply smoothing
    smoothedValue_ = smoothedValue_ * smoothingCoeff_ + rawValue * (1.0f - smoothingCoeff_);
    
    float processedValue = smoothedValue_;
    
    // Apply depth scaling
    processedValue *= settings_.depth;
    
    // Apply DC offset
    processedValue += settings_.offset;
    
    // Convert bipolar to unipolar if needed
    if (!settings_.bipolar) {
        processedValue = bipolarToUnipolar(processedValue);
    }
    
    // Apply inversion
    if (settings_.invert) {
        processedValue = -processedValue;
    }
    
    // Clamp to valid range
    if (settings_.bipolar) {
        processedValue = std::clamp(processedValue, -1.0f, 1.0f);
    } else {
        processedValue = std::clamp(processedValue, 0.0f, 1.0f);
    }
    
    return processedValue;
}

void AdvancedLFO::updateRandomization() {
    if (settings_.phaseRandom > 0.0f) {
        phaseRandomOffset_ = fastRandom() * settings_.phaseRandom;
    } else {
        phaseRandomOffset_ = 0.0f;
    }
    
    if (settings_.rateRandom > 0.0f) {
        rateRandomMultiplier_ = 1.0f + (fastRandom() - 0.5f) * settings_.rateRandom;
        rateRandomMultiplier_ = std::clamp(rateRandomMultiplier_, 0.1f, 2.0f);
    } else {
        rateRandomMultiplier_ = 1.0f;
    }
}

float AdvancedLFO::getClockDivisionMultiplier() const {
    switch (settings_.clockDiv) {
        case ClockDivision::FOUR_BARS: return 1.0f / 16.0f;
        case ClockDivision::TWO_BARS: return 1.0f / 8.0f;
        case ClockDivision::ONE_BAR: return 1.0f / 4.0f;
        case ClockDivision::HALF_NOTE: return 1.0f / 2.0f;
        case ClockDivision::QUARTER_NOTE: return 1.0f;
        case ClockDivision::EIGHTH_NOTE: return 2.0f;
        case ClockDivision::SIXTEENTH_NOTE: return 4.0f;
        case ClockDivision::THIRTY_SECOND: return 8.0f;
        case ClockDivision::DOTTED_QUARTER: return 2.0f / 3.0f;
        case ClockDivision::DOTTED_EIGHTH: return 4.0f / 3.0f;
        case ClockDivision::QUARTER_TRIPLET: return 3.0f / 2.0f;
        case ClockDivision::EIGHTH_TRIPLET: return 3.0f;
        case ClockDivision::SIXTEENTH_TRIPLET: return 6.0f;
        default: return 1.0f;
    }
}

float AdvancedLFO::fastRandom() {
    randomSeed_ = randomSeed_ * 1664525u + 1013904223u;
    return static_cast<float>(randomSeed_) / 4294967295.0f;
}

bool AdvancedLFO::isActive() const {
    if (!settings_.enabled) return false;
    
    if (settings_.syncMode == SyncMode::ONE_SHOT) {
        return phase_ < 1.0f;
    }
    
    if (settings_.syncMode == SyncMode::ENVELOPE) {
        return envStage_ != EnvStage::IDLE;
    }
    
    return true;
}

void AdvancedLFO::setFrequencyModulation(float fmValue) {
    fmInput_ = std::clamp(fmValue, -1.0f, 1.0f);
}

void AdvancedLFO::setAmplitudeModulation(float amValue) {
    amInput_ = std::clamp(amValue, -1.0f, 1.0f);
}

//=============================================================================
// Utility functions
//=============================================================================

const char* getWaveformName(AdvancedLFO::Waveform waveform) {
    switch (waveform) {
        case AdvancedLFO::Waveform::SINE: return "Sine";
        case AdvancedLFO::Waveform::TRIANGLE: return "Triangle";
        case AdvancedLFO::Waveform::SAWTOOTH_UP: return "Saw Up";
        case AdvancedLFO::Waveform::SAWTOOTH_DOWN: return "Saw Down";
        case AdvancedLFO::Waveform::SQUARE: return "Square";
        case AdvancedLFO::Waveform::PULSE: return "Pulse";
        case AdvancedLFO::Waveform::NOISE: return "Noise";
        case AdvancedLFO::Waveform::SAMPLE_HOLD: return "S&H";
        case AdvancedLFO::Waveform::EXPONENTIAL_UP: return "Exp Up";
        case AdvancedLFO::Waveform::EXPONENTIAL_DOWN: return "Exp Down";
        case AdvancedLFO::Waveform::LOGARITHMIC: return "Log";
        case AdvancedLFO::Waveform::CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

const char* getSyncModeName(AdvancedLFO::SyncMode mode) {
    switch (mode) {
        case AdvancedLFO::SyncMode::FREE_RUNNING: return "Free";
        case AdvancedLFO::SyncMode::TEMPO_SYNC: return "Sync";
        case AdvancedLFO::SyncMode::KEY_SYNC: return "Key";
        case AdvancedLFO::SyncMode::ONE_SHOT: return "1-Shot";
        case AdvancedLFO::SyncMode::ENVELOPE: return "Env";
        default: return "Unknown";
    }
}

} // namespace EtherSynth