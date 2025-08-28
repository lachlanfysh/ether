#include "OversamplingProcessor.h"
#include <cmath>
#include <algorithm>
#include <chrono>

OversamplingProcessor::OversamplingProcessor() {
    oversampleFactor_ = Factor::X2;
    sampleRate_ = 44100.0f;
    enabled_ = true;
    initialized_ = false;
    filterLength_ = MAX_FILTER_LENGTH;
    latencySamples_ = filterLength_ / 2;
    cpuUsage_ = 0.0f;
}

OversamplingProcessor::~OversamplingProcessor() {
    shutdown();
}

bool OversamplingProcessor::initialize(float sampleRate, Factor oversampleFactor) {
    if (initialized_) {
        shutdown();
    }
    
    sampleRate_ = sampleRate;
    oversampleFactor_ = oversampleFactor;
    
    // Design half-band filter
    designHalfBandFilter();
    
    // Initialize delay lines
    initializeDelayLines();
    
    // Initialize processing buffers
    int maxBufferSize = 1024 * static_cast<int>(oversampleFactor_);
    oversampledBuffer_.resize(maxBufferSize);
    processedBuffer_.resize(maxBufferSize);
    
    initialized_ = true;
    return true;
}

void OversamplingProcessor::shutdown() {
    if (!initialized_) {
        return;
    }
    
    upsampleCoeffs_.clear();
    downsampleCoeffs_.clear();
    upsampleDelay_.clear();
    downsampleDelay_.clear();
    oversampledBuffer_.clear();
    processedBuffer_.clear();
    
    initialized_ = false;
}

void OversamplingProcessor::setOversampleFactor(Factor factor) {
    if (factor != oversampleFactor_) {
        oversampleFactor_ = factor;
        
        if (initialized_) {
            // Reinitialize with new factor
            float currentSampleRate = sampleRate_;
            shutdown();
            initialize(currentSampleRate, factor);
        }
    }
}

float OversamplingProcessor::getLatencyMs() const {
    return (latencySamples_ / sampleRate_) * 1000.0f;
}

void OversamplingProcessor::designHalfBandFilter() {
    // Use pre-computed optimized half-band filter coefficients
    upsampleCoeffs_.assign(HALFBAND_COEFFS.begin(), HALFBAND_COEFFS.end());
    downsampleCoeffs_ = upsampleCoeffs_; // Same coefficients for up and down
    
    filterLength_ = static_cast<int>(upsampleCoeffs_.size());
    latencySamples_ = filterLength_ / 2;
}

void OversamplingProcessor::initializeDelayLines() {
    upsampleDelay_.assign(filterLength_, 0.0f);
    downsampleDelay_.assign(filterLength_, 0.0f);
}

void OversamplingProcessor::upsample2x(const float* input, float* output, int numInputSamples) {
    for (int i = 0; i < numInputSamples; i++) {
        // Insert sample into delay line
        for (int j = filterLength_ - 1; j > 0; j--) {
            upsampleDelay_[j] = upsampleDelay_[j - 1];
        }
        upsampleDelay_[0] = input[i];
        
        // Generate two output samples per input sample
        // First sample (direct path)
        float sum1 = 0.0f;
        for (int j = 0; j < filterLength_; j += 2) {
            sum1 += upsampleDelay_[j] * upsampleCoeffs_[j];
        }
        output[i * 2] = sum1 * 2.0f; // Compensate for 2x interpolation
        
        // Second sample (interpolated)
        float sum2 = 0.0f;
        for (int j = 1; j < filterLength_; j += 2) {
            sum2 += upsampleDelay_[j] * upsampleCoeffs_[j];
        }
        output[i * 2 + 1] = sum2 * 2.0f;
    }
}

void OversamplingProcessor::downsample2x(const float* input, float* output, int numInputSamples) {
    int numOutputSamples = numInputSamples / 2;
    
    for (int i = 0; i < numOutputSamples; i++) {
        // Process every second sample with anti-aliasing filter
        float sample = input[i * 2];
        
        // Insert into delay line
        for (int j = filterLength_ - 1; j > 0; j--) {
            downsampleDelay_[j] = downsampleDelay_[j - 1];
        }
        downsampleDelay_[0] = sample;
        
        // Apply filter
        float sum = 0.0f;
        for (int j = 0; j < filterLength_; j++) {
            sum += downsampleDelay_[j] * downsampleCoeffs_[j];
        }
        
        output[i] = sum;
    }
}

void OversamplingProcessor::upsample4x(const float* input, float* output, int numInputSamples) {
    // 4x oversampling implemented as two stages of 2x
    std::vector<float> intermediate(numInputSamples * 2);
    
    // First stage: 1x to 2x
    upsample2x(input, intermediate.data(), numInputSamples);
    
    // Second stage: 2x to 4x
    upsample2x(intermediate.data(), output, numInputSamples * 2);
}

void OversamplingProcessor::downsample4x(const float* input, float* output, int numInputSamples) {
    // 4x downsampling implemented as two stages of 2x
    int intermediateSize = numInputSamples / 2;
    std::vector<float> intermediate(intermediateSize);
    
    // First stage: 4x to 2x
    downsample2x(input, intermediate.data(), numInputSamples);
    
    // Second stage: 2x to 1x
    downsample2x(intermediate.data(), output, intermediateSize);
}

float OversamplingProcessor::processUpsampleFilter(float input) {
    // Insert into delay line
    for (int j = filterLength_ - 1; j > 0; j--) {
        upsampleDelay_[j] = upsampleDelay_[j - 1];
    }
    upsampleDelay_[0] = input;
    
    // Apply filter
    float sum = 0.0f;
    for (int j = 0; j < filterLength_; j++) {
        sum += upsampleDelay_[j] * upsampleCoeffs_[j];
    }
    
    return sum;
}

float OversamplingProcessor::processDownsampleFilter(float input) {
    // Insert into delay line
    for (int j = filterLength_ - 1; j > 0; j--) {
        downsampleDelay_[j] = downsampleDelay_[j - 1];
    }
    downsampleDelay_[0] = input;
    
    // Apply filter
    float sum = 0.0f;
    for (int j = 0; j < filterLength_; j++) {
        sum += downsampleDelay_[j] * downsampleCoeffs_[j];
    }
    
    return sum;
}

void OversamplingProcessor::clearDelayLines() {
    std::fill(upsampleDelay_.begin(), upsampleDelay_.end(), 0.0f);
    std::fill(downsampleDelay_.begin(), downsampleDelay_.end(), 0.0f);
}

float OversamplingProcessor::calculateCPUUsage(int numSamples, float processingTimeMs) {
    if (sampleRate_ <= 0.0f || numSamples <= 0) {
        return 0.0f;
    }
    
    float blockTimeMs = (numSamples / sampleRate_) * 1000.0f;
    float usage = (processingTimeMs / blockTimeMs) * 100.0f;
    
    // Smooth CPU usage measurement
    cpuUsage_ = cpuUsage_ * CPU_USAGE_SMOOTH + usage * (1.0f - CPU_USAGE_SMOOTH);
    
    return cpuUsage_;
}