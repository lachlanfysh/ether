#include "SubsonicFilter.h"
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.4142135623730951
#endif

SubsonicFilter::SubsonicFilter() {
    sampleRate_ = 44100.0f;
    cutoffHz_ = 24.0f;
    filterType_ = FilterType::BUTTERWORTH;
    qFactor_ = 0.7071f;
    dcBlockerEnabled_ = true;
    initialized_ = false;
    
    // Initialize filter state
    x_.fill(0.0f);
    y_.fill(0.0f);
    
    // Initialize coefficients
    b0_ = b1_ = b2_ = 0.0f;
    a1_ = a2_ = 0.0f;
    
    // Initialize DC blocker
    dcX1_ = dcY1_ = 0.0f;
    dcA1_ = dcB0_ = dcB1_ = 0.0f;
}

bool SubsonicFilter::initialize(float sampleRate, float cutoffHz, FilterType type) {
    if (sampleRate <= 0.0f) {
        return false;
    }
    
    sampleRate_ = sampleRate;
    cutoffHz_ = clamp(cutoffHz, MIN_CUTOFF_HZ, MAX_CUTOFF_HZ);
    filterType_ = type;
    
    updateQFromFilterType();
    calculateCoefficients();
    calculateDCBlockerCoefficients();
    reset();
    
    initialized_ = true;
    return true;
}

void SubsonicFilter::shutdown() {
    if (!initialized_) {
        return;
    }
    
    reset();
    initialized_ = false;
}

float SubsonicFilter::processSample(float input) {
    if (!initialized_) {
        return input;
    }
    
    float processedInput = input;
    
    // Optional DC blocker pre-stage
    if (dcBlockerEnabled_) {
        processedInput = dcB0_ * input + dcB1_ * dcX1_ - dcA1_ * dcY1_;
        dcX1_ = input;
        dcY1_ = processedInput;
    }
    
    // Biquad filter processing
    // y[n] = bFormantEngine*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
    float output = b0_ * processedInput + b1_ * x_[0] + b2_ * x_[1] - a1_ * y_[0] - a2_ * y_[1];
    
    // Update delay elements
    x_[1] = x_[0];
    x_[0] = processedInput;
    y_[1] = y_[0];
    y_[0] = output;
    
    return output;
}

void SubsonicFilter::processBlock(float* output, const float* input, int numSamples) {
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

void SubsonicFilter::processBlock(float* buffer, int numSamples) {
    if (!initialized_) {
        return;
    }
    
    for (int i = 0; i < numSamples; i++) {
        buffer[i] = processSample(buffer[i]);
    }
}

void SubsonicFilter::setCutoffFrequency(float hz) {
    float newCutoff = clamp(hz, MIN_CUTOFF_HZ, MAX_CUTOFF_HZ);
    
    if (std::abs(newCutoff - cutoffHz_) > 0.1f) {
        cutoffHz_ = newCutoff;
        if (initialized_) {
            calculateCoefficients();
            calculateDCBlockerCoefficients();
        }
    }
}

void SubsonicFilter::setFilterType(FilterType type) {
    if (type != filterType_) {
        filterType_ = type;
        updateQFromFilterType();
        if (initialized_) {
            calculateCoefficients();
        }
    }
}

void SubsonicFilter::setQFactor(float q) {
    if (filterType_ == FilterType::CUSTOM) {
        float newQ = clamp(q, MIN_Q, MAX_Q);
        if (std::abs(newQ - qFactor_) > 0.01f) {
            qFactor_ = newQ;
            if (initialized_) {
                calculateCoefficients();
            }
        }
    }
}

void SubsonicFilter::setSampleRate(float sampleRate) {
    if (sampleRate > 0.0f && std::abs(sampleRate - sampleRate_) > 0.1f) {
        sampleRate_ = sampleRate;
        if (initialized_) {
            calculateCoefficients();
            calculateDCBlockerCoefficients();
        }
    }
}

void SubsonicFilter::enableDCBlocker(bool enable) {
    if (enable != dcBlockerEnabled_) {
        dcBlockerEnabled_ = enable;
        if (initialized_ && enable) {
            calculateDCBlockerCoefficients();
        }
    }
}

float SubsonicFilter::getMagnitudeResponse(float frequency) const {
    if (!initialized_ || frequency <= 0.0f) {
        return 1.0f;
    }
    
    // Calculate normalized frequency
    float omega = 2.0f * static_cast<float>(M_PI) * frequency / sampleRate_;
    float cosOmega = std::cos(omega);
    float sinOmega = std::sin(omega);
    
    // Calculate magnitude response
    float numeratorReal = b0_ + b1_ * cosOmega + b2_ * std::cos(2.0f * omega);
    float numeratorImag = -b1_ * sinOmega - b2_ * std::sin(2.0f * omega);
    float denominatorReal = 1.0f + a1_ * cosOmega + a2_ * std::cos(2.0f * omega);
    float denominatorImag = -a1_ * sinOmega - a2_ * std::sin(2.0f * omega);
    
    float numeratorMag = std::sqrt(numeratorReal * numeratorReal + numeratorImag * numeratorImag);
    float denominatorMag = std::sqrt(denominatorReal * denominatorReal + denominatorImag * denominatorImag);
    
    return numeratorMag / std::max(denominatorMag, 1e-10f);
}

void SubsonicFilter::reset() {
    x_.fill(0.0f);
    y_.fill(0.0f);
    dcX1_ = dcY1_ = 0.0f;
}

void SubsonicFilter::reset(float initialValue) {
    x_.fill(initialValue);
    y_.fill(initialValue);
    dcX1_ = dcY1_ = initialValue;
}

void SubsonicFilter::processMultiple(SubsonicFilter* filters, float** buffers, int numChannels, int numSamples) {
    for (int ch = 0; ch < numChannels; ch++) {
        if (filters[ch].isInitialized()) {
            filters[ch].processBlock(buffers[ch], numSamples);
        }
    }
}

// Private method implementations

void SubsonicFilter::calculateCoefficients() {
    if (sampleRate_ <= 0.0f || cutoffHz_ <= 0.0f) {
        // Pass-through coefficients
        b0_ = 1.0f;
        b1_ = b2_ = 0.0f;
        a1_ = a2_ = 0.0f;
        return;
    }
    
    // Pre-warp frequency for bilinear transform
    float omega = 2.0f * static_cast<float>(M_PI) * cutoffHz_ / sampleRate_;
    float tanHalfOmega = std::tan(omega * 0.5f);
    
    // Calculate analog filter coefficients (s-domain)
    float omega0 = 2.0f * static_cast<float>(M_PI) * cutoffHz_;
    float alpha = omega0 / (2.0f * qFactor_);
    
    // Bilinear transform to z-domain
    float k = 2.0f * sampleRate_;
    float k2 = k * k;
    float omega02 = omega0 * omega0;
    
    float norm = k2 + alpha * k + omega02;
    
    // High-pass biquad coefficients
    b0_ = k2 / norm;
    b1_ = -2.0f * k2 / norm;
    b2_ = k2 / norm;
    a1_ = (2.0f * omega02 - 2.0f * k2) / norm;
    a2_ = (k2 - alpha * k + omega02) / norm;
    
    // Clamp coefficients for stability
    b0_ = clamp(b0_, -10.0f, 10.0f);
    b1_ = clamp(b1_, -10.0f, 10.0f);
    b2_ = clamp(b2_, -10.0f, 10.0f);
    a1_ = clamp(a1_, -1.99f, 1.99f);
    a2_ = clamp(a2_, -0.99f, 0.99f);
}

void SubsonicFilter::calculateDCBlockerCoefficients() {
    if (!dcBlockerEnabled_ || sampleRate_ <= 0.0f) {
        dcB0_ = 1.0f;
        dcB1_ = 0.0f;
        dcA1_ = 0.0f;
        return;
    }
    
    // DC blocker at 5Hz (below our main filter)
    float dcCutoff = 5.0f;
    float omega = 2.0f * static_cast<float>(M_PI) * dcCutoff / sampleRate_;
    float cosOmega = std::cos(omega);
    float sinOmega = std::sin(omega);
    float alpha = sinOmega / (2.0f * 0.7071f);
    
    float norm = 1.0f + alpha;
    
    dcB0_ = (1.0f + cosOmega) / (2.0f * norm);
    dcB1_ = -(1.0f + cosOmega) / (2.0f * norm);
    dcA1_ = (1.0f - alpha) / norm;
    
    // Clamp coefficients
    dcB0_ = clamp(dcB0_, -2.0f, 2.0f);
    dcB1_ = clamp(dcB1_, -2.0f, 2.0f);
    dcA1_ = clamp(dcA1_, -0.99f, 0.99f);
}

void SubsonicFilter::updateQFromFilterType() {
    switch (filterType_) {
        case FilterType::BUTTERWORTH:
            qFactor_ = 0.7071f; // 1/sqrt(2)
            break;
        case FilterType::LINKWITZ_RILEY:
            qFactor_ = 0.5f;
            break;
        case FilterType::CRITICAL:
            qFactor_ = 0.5f;
            break;
        case FilterType::CUSTOM:
            // Keep current Q factor
            break;
    }
}

float SubsonicFilter::clamp(float value, float min, float max) const {
    return std::max(min, std::min(max, value));
}