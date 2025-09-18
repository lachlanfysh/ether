#include "StateVariableFilter.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

StateVariableFilter::StateVariableFilter() {
    updateCoefficients();
}

bool StateVariableFilter::initialize(float sampleRate) {
    setSampleRate(sampleRate);
    reset();
    return true;
}

void StateVariableFilter::setCutoffFrequency(float frequency) {
    cutoffFreq_ = std::max(10.0f, std::min(frequency, sampleRate_ * 0.45f));
    updateCoefficients();
}

void StateVariableFilter::setResonance(float resonance) {
    resonance_ = std::max(0.1f, std::min(resonance, 30.0f));
    updateCoefficients();
}

void StateVariableFilter::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updateCoefficients();
}

void StateVariableFilter::updateCoefficients() {
    f_ = 2.0f * std::sin(M_PI * cutoffFreq_ / sampleRate_);
    q_ = 1.0f / resonance_;
}

float StateVariableFilter::processLowpass(float input) {
    float hp = input - lp_ - q_ * bp_;
    bp_ += f_ * hp;
    lp_ += f_ * bp_;
    return lp_;
}

float StateVariableFilter::processHighpass(float input) {
    float hp = input - lp_ - q_ * bp_;
    bp_ += f_ * hp;
    lp_ += f_ * bp_;
    return hp;
}

float StateVariableFilter::processBandpass(float input) {
    float hp = input - lp_ - q_ * bp_;
    bp_ += f_ * hp;
    lp_ += f_ * bp_;
    return bp_;
}

StateVariableFilter::FilterOutputs StateVariableFilter::processAll(float input) {
    float hp = input - lp_ - q_ * bp_;
    bp_ += f_ * hp;
    lp_ += f_ * bp_;
    
    return {lp_, hp, bp_};
}

void StateVariableFilter::reset() {
    bp_ = 0.0f;
    lp_ = 0.0f;
}
