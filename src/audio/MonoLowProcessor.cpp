#include "MonoLowProcessor.h"
#include <algorithm>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_SQRT2  
#define M_SQRT2 1.4142135623730951
#endif

MonoLowProcessor::MonoLowProcessor() {
    sampleRate_ = 44100.0f;
    crossoverHz_ = DEFAULT_CROSSOVER_HZ;
    monoGain_ = 1.0f;
    bypassed_ = false;
    initialized_ = false;
    cpuUsage_ = 0.0f;
    
    // Initialize filter states
    resetBiquadState(leftLowPass1_);
    resetBiquadState(leftLowPass2_);
    resetBiquadState(leftHighPass1_);
    resetBiquadState(leftHighPass2_);
    resetBiquadState(rightLowPass1_);
    resetBiquadState(rightLowPass2_);
    resetBiquadState(rightHighPass1_);
    resetBiquadState(rightHighPass2_);
    
    // Initialize coefficients
    lowPassCoeffs_ = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    highPassCoeffs_ = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
}

bool MonoLowProcessor::initialize(float sampleRate, float crossoverHz) {
    if (sampleRate <= 0.0f) {
        return false;
    }
    
    sampleRate_ = sampleRate;
    crossoverHz_ = clamp(crossoverHz, MIN_CROSSOVER_HZ, MAX_CROSSOVER_HZ);
    
    calculateCoefficients();
    reset();
    
    initialized_ = true;
    return true;
}

void MonoLowProcessor::shutdown() {
    if (!initialized_) {
        return;
    }
    
    reset();
    initialized_ = false;
}

void MonoLowProcessor::processStereo(float& left, float& right) {
    if (!initialized_ || bypassed_) {
        return;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Process left channel
    float leftLow1 = processBiquad(left, leftLowPass1_, lowPassCoeffs_);
    float leftLow = processBiquad(leftLow1, leftLowPass2_, lowPassCoeffs_);
    
    float leftHigh1 = processBiquad(left, leftHighPass1_, highPassCoeffs_);
    float leftHigh = processBiquad(leftHigh1, leftHighPass2_, highPassCoeffs_);
    
    // Process right channel
    float rightLow1 = processBiquad(right, rightLowPass1_, lowPassCoeffs_);
    float rightLow = processBiquad(rightLow1, rightLowPass2_, lowPassCoeffs_);
    
    float rightHigh1 = processBiquad(right, rightHighPass1_, highPassCoeffs_);
    float rightHigh = processBiquad(rightHigh1, rightHighPass2_, highPassCoeffs_);
    
    // Create mono sum of low frequencies
    float monoLow = (leftLow + rightLow) * 0.5f * monoGain_;
    
    // Reconstruct output: mono low + stereo high
    left = monoLow + leftHigh;
    right = monoLow + rightHigh;
    
    // Update CPU usage
    auto endTime = std::chrono::high_resolution_clock::now();
    float processingTime = std::chrono::duration<float, std::micro>(endTime - startTime).count();
    cpuUsage_ = cpuUsage_ * 0.999f + processingTime * 0.001f;
}

void MonoLowProcessor::processBlock(float* leftChannel, float* rightChannel, int numSamples) {
    if (!initialized_ || bypassed_) {
        return;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numSamples; i++) {
        float left = leftChannel[i];
        float right = rightChannel[i];
        
        // Process left channel
        float leftLow1 = processBiquad(left, leftLowPass1_, lowPassCoeffs_);
        float leftLow = processBiquad(leftLow1, leftLowPass2_, lowPassCoeffs_);
        
        float leftHigh1 = processBiquad(left, leftHighPass1_, highPassCoeffs_);
        float leftHigh = processBiquad(leftHigh1, leftHighPass2_, highPassCoeffs_);
        
        // Process right channel
        float rightLow1 = processBiquad(right, rightLowPass1_, lowPassCoeffs_);
        float rightLow = processBiquad(rightLow1, rightLowPass2_, lowPassCoeffs_);
        
        float rightHigh1 = processBiquad(right, rightHighPass1_, highPassCoeffs_);
        float rightHigh = processBiquad(rightHigh1, rightHighPass2_, highPassCoeffs_);
        
        // Create mono sum of low frequencies
        float monoLow = (leftLow + rightLow) * 0.5f * monoGain_;
        
        // Reconstruct output: mono low + stereo high
        leftChannel[i] = monoLow + leftHigh;
        rightChannel[i] = monoLow + rightHigh;
    }
    
    // Update CPU usage
    auto endTime = std::chrono::high_resolution_clock::now();
    float processingTime = std::chrono::duration<float, std::micro>(endTime - startTime).count();
    cpuUsage_ = cpuUsage_ * 0.99f + (processingTime / numSamples) * 0.01f;
}

void MonoLowProcessor::setCrossoverFrequency(float hz) {
    float newCrossover = clamp(hz, MIN_CROSSOVER_HZ, MAX_CROSSOVER_HZ);
    
    if (std::abs(newCrossover - crossoverHz_) > 0.1f) {
        crossoverHz_ = newCrossover;
        if (initialized_) {
            calculateCoefficients();
        }
    }
}

void MonoLowProcessor::setBypass(bool bypass) {
    bypassed_ = bypass;
}

void MonoLowProcessor::setSampleRate(float sampleRate) {
    if (sampleRate > 0.0f && std::abs(sampleRate - sampleRate_) > 0.1f) {
        sampleRate_ = sampleRate;
        if (initialized_) {
            calculateCoefficients();
        }
    }
}

void MonoLowProcessor::setMonoGain(float gain) {
    monoGain_ = clamp(gain, 0.0f, 2.0f);
}

float MonoLowProcessor::getMagnitudeResponse(float frequency, bool lowBand) const {
    if (!initialized_ || frequency <= 0.0f) {
        return lowBand ? (frequency == 0.0f ? 1.0f : 0.0f) : 1.0f;
    }
    
    // For very low frequencies, low-pass should pass, high-pass should block
    if (frequency < 1.0f) {
        return lowBand ? 1.0f : 0.0f;
    }
    
    // Calculate normalized frequency
    float omega = 2.0f * static_cast<float>(M_PI) * frequency / sampleRate_;
    
    // Clamp omega to valid range
    omega = clamp(omega, 1e-6f, static_cast<float>(M_PI) - 1e-6f);
    
    float cosOmega = std::cos(omega);
    float cos2Omega = std::cos(2.0f * omega);
    float sinOmega = std::sin(omega);
    float sin2Omega = std::sin(2.0f * omega);
    
    const BiquadCoeffs& coeffs = lowBand ? lowPassCoeffs_ : highPassCoeffs_;
    
    // Calculate single biquad response
    float numeratorReal = coeffs.b0 + coeffs.b1 * cosOmega + coeffs.b2 * cos2Omega;
    float numeratorImag = -coeffs.b1 * sinOmega - coeffs.b2 * sin2Omega;
    float denominatorReal = 1.0f + coeffs.a1 * cosOmega + coeffs.a2 * cos2Omega;
    float denominatorImag = -coeffs.a1 * sinOmega - coeffs.a2 * sin2Omega;
    
    float numeratorMag = std::sqrt(numeratorReal * numeratorReal + numeratorImag * numeratorImag);
    float denominatorMag = std::sqrt(denominatorReal * denominatorReal + denominatorImag * denominatorImag);
    
    float singleStageResponse = numeratorMag / std::max(denominatorMag, 1e-10f);
    
    // 4th order Linkwitz-Riley = two cascaded 2nd order sections
    return singleStageResponse * singleStageResponse;
}

void MonoLowProcessor::reset() {
    resetBiquadState(leftLowPass1_);
    resetBiquadState(leftLowPass2_);
    resetBiquadState(leftHighPass1_);
    resetBiquadState(leftHighPass2_);
    resetBiquadState(rightLowPass1_);
    resetBiquadState(rightLowPass2_);
    resetBiquadState(rightHighPass1_);
    resetBiquadState(rightHighPass2_);
    cpuUsage_ = 0.0f;
}

void MonoLowProcessor::reset(float initialValue) {
    // Set all filter states to initial value
    leftLowPass1_.x1 = leftLowPass1_.x2 = initialValue;
    leftLowPass1_.y1 = leftLowPass1_.y2 = initialValue;
    leftLowPass2_.x1 = leftLowPass2_.x2 = initialValue;
    leftLowPass2_.y1 = leftLowPass2_.y2 = initialValue;
    
    leftHighPass1_.x1 = leftHighPass1_.x2 = initialValue;
    leftHighPass1_.y1 = leftHighPass1_.y2 = 0.0f; // High-pass outputs 0 for DC
    leftHighPass2_.x1 = leftHighPass2_.x2 = initialValue;
    leftHighPass2_.y1 = leftHighPass2_.y2 = 0.0f;
    
    rightLowPass1_.x1 = rightLowPass1_.x2 = initialValue;
    rightLowPass1_.y1 = rightLowPass1_.y2 = initialValue;
    rightLowPass2_.x1 = rightLowPass2_.x2 = initialValue;
    rightLowPass2_.y1 = rightLowPass2_.y2 = initialValue;
    
    rightHighPass1_.x1 = rightHighPass1_.x2 = initialValue;
    rightHighPass1_.y1 = rightHighPass1_.y2 = 0.0f;
    rightHighPass2_.x1 = rightHighPass2_.x2 = initialValue;
    rightHighPass2_.y1 = rightHighPass2_.y2 = 0.0f;
    
    cpuUsage_ = 0.0f;
}

// Private method implementations

void MonoLowProcessor::calculateCoefficients() {
    if (sampleRate_ <= 0.0f || crossoverHz_ <= 0.0f) {
        // Pass-through coefficients
        lowPassCoeffs_ = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        highPassCoeffs_ = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        return;
    }
    
    // Calculate Linkwitz-Riley 2nd-order sections
    // We use two cascaded 2nd-order Butterworth sections for 4th-order LR response
    float omega = 2.0f * static_cast<float>(M_PI) * crossoverHz_ / sampleRate_;
    float k = std::tan(omega * 0.5f);
    float norm = 1.0f / (1.0f + k / static_cast<float>(M_SQRT2) + k * k);
    
    // Low-pass coefficients (2nd-order Butterworth)
    lowPassCoeffs_.b0 = k * k * norm;
    lowPassCoeffs_.b1 = 2.0f * lowPassCoeffs_.b0;
    lowPassCoeffs_.b2 = lowPassCoeffs_.b0;
    lowPassCoeffs_.a1 = 2.0f * (k * k - 1.0f) * norm;
    lowPassCoeffs_.a2 = (1.0f - k / static_cast<float>(M_SQRT2) + k * k) * norm;
    
    // High-pass coefficients (2nd-order Butterworth)
    highPassCoeffs_.b0 = norm;
    highPassCoeffs_.b1 = -2.0f * highPassCoeffs_.b0;
    highPassCoeffs_.b2 = highPassCoeffs_.b0;
    highPassCoeffs_.a1 = lowPassCoeffs_.a1; // Same denominator
    highPassCoeffs_.a2 = lowPassCoeffs_.a2;
    
    // Clamp coefficients for stability
    lowPassCoeffs_.b0 = clamp(lowPassCoeffs_.b0, -10.0f, 10.0f);
    lowPassCoeffs_.b1 = clamp(lowPassCoeffs_.b1, -10.0f, 10.0f);
    lowPassCoeffs_.b2 = clamp(lowPassCoeffs_.b2, -10.0f, 10.0f);
    lowPassCoeffs_.a1 = clamp(lowPassCoeffs_.a1, -1.99f, 1.99f);
    lowPassCoeffs_.a2 = clamp(lowPassCoeffs_.a2, -0.99f, 0.99f);
    
    highPassCoeffs_.b0 = clamp(highPassCoeffs_.b0, -10.0f, 10.0f);
    highPassCoeffs_.b1 = clamp(highPassCoeffs_.b1, -10.0f, 10.0f);
    highPassCoeffs_.b2 = clamp(highPassCoeffs_.b2, -10.0f, 10.0f);
    highPassCoeffs_.a1 = clamp(highPassCoeffs_.a1, -1.99f, 1.99f);
    highPassCoeffs_.a2 = clamp(highPassCoeffs_.a2, -0.99f, 0.99f);
}

float MonoLowProcessor::processBiquad(float input, BiquadState& state, const BiquadCoeffs& coeffs) const {
    // Direct Form II biquad processing
    float output = coeffs.b0 * input + coeffs.b1 * state.x1 + coeffs.b2 * state.x2 
                   - coeffs.a1 * state.y1 - coeffs.a2 * state.y2;
    
    // Update delay elements
    state.x2 = state.x1;
    state.x1 = input;
    state.y2 = state.y1;
    state.y1 = output;
    
    return output;
}

void MonoLowProcessor::resetBiquadState(BiquadState& state) {
    state.x1 = state.x2 = 0.0f;
    state.y1 = state.y2 = 0.0f;
}

float MonoLowProcessor::clamp(float value, float min, float max) const {
    return std::max(min, std::min(max, value));
}