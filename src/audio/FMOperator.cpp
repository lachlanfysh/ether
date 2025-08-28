#include "FMOperator.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

FMOperator::FMOperator() {
    waveform_ = Waveform::SINE;
    frequency_ = 440.0f;
    level_ = 1.0f;
    enabled_ = true;
    sampleRate_ = 44100.0f;
    phase_ = 0.0f;
    phaseIncrement_ = 0.0f;
    initialized_ = false;
}

FMOperator::~FMOperator() {
    shutdown();
}

bool FMOperator::initialize(float sampleRate) {
    if (initialized_) {
        return true;
    }
    
    sampleRate_ = sampleRate;
    updatePhaseIncrement();
    
    initialized_ = true;
    return true;
}

void FMOperator::shutdown() {
    if (!initialized_) {
        return;
    }
    
    phase_ = 0.0f;
    phaseIncrement_ = 0.0f;
    initialized_ = false;
}

void FMOperator::setFrequency(float frequency) {
    frequency_ = frequency;
    if (initialized_) {
        updatePhaseIncrement();
    }
}

void FMOperator::setLevel(float level) {
    level_ = std::max(0.0f, level);
}

void FMOperator::setPhase(float phase) {
    phase_ = normalizePhase(phase);
}

float FMOperator::processSample(float modulation) {
    if (!initialized_ || !enabled_ || level_ <= 0.001f) {
        return 0.0f;
    }
    
    // Apply phase modulation
    float modulatedPhase = phase_ + modulation;
    modulatedPhase = normalizePhase(modulatedPhase);
    
    // Generate waveform
    float output = generateWaveform(modulatedPhase);
    
    // Apply level
    output *= level_;
    
    // Update phase for next sample
    phase_ += phaseIncrement_;
    phase_ = normalizePhase(phase_);
    
    return output;
}

void FMOperator::processBlock(float* output, const float* modulation, int numSamples) {
    if (!initialized_ || !enabled_ || level_ <= 0.001f) {
        for (int i = 0; i < numSamples; i++) {
            output[i] = 0.0f;
        }
        return;
    }
    
    for (int i = 0; i < numSamples; i++) {
        float mod = (modulation != nullptr) ? modulation[i] : 0.0f;
        output[i] = processSample(mod);
    }
}

float FMOperator::generateWaveform(float phase) const {
    switch (waveform_) {
        case Waveform::SINE:
            return generateSine(phase);
        case Waveform::SAW_APPROX:
            return generateSawApprox(phase);
        case Waveform::SQUARE_APPROX:
            return generateSquareApprox(phase);
        case Waveform::TRIANGLE_APPROX:
            return generateTriangleApprox(phase);
        case Waveform::HALF_SINE:
            return generateHalfSine(phase);
        case Waveform::FULL_SINE:
            return generateFullSine(phase);
        case Waveform::QUARTER_SINE:
            return generateQuarterSine(phase);
        case Waveform::ALT_SINE:
            return generateAltSine(phase);
        default:
            return generateSine(phase);
    }
}

float FMOperator::generateSawApprox(float phase) const {
    // Sine-based sawtooth approximation using Fourier series
    float fundamental = std::sin(phase);
    float harmonic2 = std::sin(phase * 2.0f) * 0.5f;
    float harmonic3 = std::sin(phase * 3.0f) * 0.333f;
    float harmonic4 = std::sin(phase * 4.0f) * 0.25f;
    
    return (fundamental + harmonic2 + harmonic3 + harmonic4) * 0.637f; // Normalize
}

float FMOperator::generateSquareApprox(float phase) const {
    // Sine-based square wave approximation using odd harmonics
    float fundamental = std::sin(phase);
    float harmonic3 = std::sin(phase * 3.0f) * 0.333f;
    float harmonic5 = std::sin(phase * 5.0f) * 0.2f;
    float harmonic7 = std::sin(phase * 7.0f) * 0.143f;
    
    return (fundamental + harmonic3 + harmonic5 + harmonic7) * 0.785f; // Normalize
}

float FMOperator::generateTriangleApprox(float phase) const {
    // Sine-based triangle wave approximation
    float fundamental = std::sin(phase);
    float harmonic3 = -std::sin(phase * 3.0f) * 0.111f; // Negative for triangle
    float harmonic5 = std::sin(phase * 5.0f) * 0.04f;
    float harmonic7 = -std::sin(phase * 7.0f) * 0.0204f;
    
    return (fundamental + harmonic3 + harmonic5 + harmonic7) * 0.81f; // Normalize
}

float FMOperator::generateHalfSine(float phase) const {
    // Half-wave rectified sine
    float sine = std::sin(phase);
    return (sine > 0.0f) ? sine : 0.0f;
}

float FMOperator::generateFullSine(float phase) const {
    // Full-wave rectified sine
    return std::abs(std::sin(phase));
}

float FMOperator::generateQuarterSine(float phase) const {
    // Quarter sine wave (sine for first quarter, zero elsewhere)
    if (phase < PI * 0.5f) {
        return std::sin(phase);
    } else {
        return 0.0f;
    }
}

float FMOperator::generateAltSine(float phase) const {
    // Alternating sine polarity
    float sine = std::sin(phase);
    int cycle = static_cast<int>(phase * INV_TWO_PI);
    return (cycle % 2 == 0) ? sine : -sine;
}

void FMOperator::updatePhaseIncrement() {
    if (sampleRate_ > 0.0f) {
        phaseIncrement_ = TWO_PI * frequency_ / sampleRate_;
    } else {
        phaseIncrement_ = 0.0f;
    }
}