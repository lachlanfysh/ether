#include "DCBlocker.h"
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

DCBlocker::DCBlocker() {
    sampleRate_ = 44100.0f;
    cutoffHz_ = DEFAULT_CUTOFF_HZ;
    initialized_ = false;
    
    x1_ = 0.0f;
    y1_ = 0.0f;
    
    a1_ = 0.0f;
    b0_ = 0.0f;
    b1_ = 0.0f;
}

bool DCBlocker::initialize(float sampleRate, float cutoffHz) {
    if (sampleRate <= 0.0f) {
        return false;
    }
    
    sampleRate_ = sampleRate;
    cutoffHz_ = clamp(cutoffHz, MIN_CUTOFF_HZ, MAX_CUTOFF_HZ);
    
    calculateCoefficients();
    reset();
    
    initialized_ = true;
    return true;
}

void DCBlocker::shutdown() {
    if (!initialized_) {
        return;
    }
    
    reset();
    initialized_ = false;
}

float DCBlocker::processSample(float input) {
    if (!initialized_) {
        return input;
    }
    
    // High-pass filter difference equation:
    // y[n] = bFormantEngine*x[n] + b1*x[n-1] - a1*y[n-1]
    float output = b0_ * input + b1_ * x1_ - a1_ * y1_;
    
    // Update delay elements
    x1_ = input;
    y1_ = output;
    
    return output;
}

void DCBlocker::processBlock(float* output, const float* input, int numSamples) {
    if (!initialized_) {
        // Pass through if not initialized
        for (int i = 0; i < numSamples; i++) {
            output[i] = input[i];
        }
        return;
    }
    
    for (int i = 0; i < numSamples; i++) {
        output[i] = processSample(input[i]);
    }
}

void DCBlocker::processBlock(float* buffer, int numSamples) {
    if (!initialized_) {
        return;
    }
    
    for (int i = 0; i < numSamples; i++) {
        buffer[i] = processSample(buffer[i]);
    }
}

void DCBlocker::setCutoffFrequency(float hz) {
    float newCutoff = clamp(hz, MIN_CUTOFF_HZ, MAX_CUTOFF_HZ);
    
    if (std::abs(newCutoff - cutoffHz_) > 0.1f) {
        cutoffHz_ = newCutoff;
        if (initialized_) {
            calculateCoefficients();
        }
    }
}

void DCBlocker::setSampleRate(float sampleRate) {
    if (sampleRate > 0.0f && std::abs(sampleRate - sampleRate_) > 0.1f) {
        sampleRate_ = sampleRate;
        if (initialized_) {
            calculateCoefficients();
        }
    }
}

void DCBlocker::reset() {
    x1_ = 0.0f;
    y1_ = 0.0f;
}

void DCBlocker::reset(float initialValue) {
    x1_ = initialValue;
    y1_ = initialValue;
}

void DCBlocker::processMultiple(DCBlocker* blockers, float** buffers, int numChannels, int numSamples) {
    for (int ch = 0; ch < numChannels; ch++) {
        if (blockers[ch].isInitialized()) {
            blockers[ch].processBlock(buffers[ch], numSamples);
        }
    }
}

// Private method implementations

void DCBlocker::calculateCoefficients() {
    if (sampleRate_ <= 0.0f || cutoffHz_ <= 0.0f) {
        b0_ = 1.0f;
        b1_ = 0.0f;
        a1_ = 0.0f;
        return;
    }
    
    // Calculate normalized frequency
    float omega = 2.0f * static_cast<float>(M_PI) * cutoffHz_ / sampleRate_;
    
    // Calculate filter coefficients using bilinear transform
    float cosOmega = std::cos(omega);
    float sinOmega = std::sin(omega);
    float alpha = sinOmega / (2.0f * 0.7071f); // Q = 0.7071 (Butterworth response)
    
    float norm = 1.0f + alpha;
    
    // High-pass filter coefficients
    b0_ = (1.0f + cosOmega) / (2.0f * norm);
    b1_ = -(1.0f + cosOmega) / (2.0f * norm);
    a1_ = (1.0f - alpha) / norm;
    
    // Clamp coefficients to stable range
    b0_ = clamp(b0_, -2.0f, 2.0f);
    b1_ = clamp(b1_, -2.0f, 2.0f);
    a1_ = clamp(a1_, -0.99f, 0.99f);
}

float DCBlocker::clamp(float value, float min, float max) const {
    return std::max(min, std::min(max, value));
}