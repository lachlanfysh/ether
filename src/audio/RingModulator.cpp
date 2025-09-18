#include "RingModulator.h"
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

RingModulator::RingModulator() {
    updatePhaseIncrement();
}

void RingModulator::setModulationFrequency(float frequency) {
    modFreq_ = frequency;
    updatePhaseIncrement();
}

void RingModulator::setModulationDepth(float depth) {
    modDepth_ = std::max(0.0f, std::min(depth, 1.0f));
}

void RingModulator::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updatePhaseIncrement();
}

void RingModulator::updatePhaseIncrement() {
    phaseIncrement_ = 2.0f * M_PI * modFreq_ / sampleRate_;
}

float RingModulator::process(float input) {
    float modulator = std::sin(phase_);
    phase_ += phaseIncrement_;
    
    if (phase_ >= 2.0f * M_PI) {
        phase_ -= 2.0f * M_PI;
    }
    
    return input * (1.0f - modDepth_ + modDepth_ * modulator);
}

float RingModulator::process(float input, float externalModulator) {
    return input * (1.0f - modDepth_ + modDepth_ * externalModulator);
}

void RingModulator::reset() {
    phase_ = 0.0f;
}
