#include "LUFSNormalizer.h"
#include <algorithm>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

LUFSNormalizer::LUFSNormalizer() {
    sampleRate_ = 44100.0f;
    stereoInput_ = true;
    bypassed_ = false;
    initialized_ = false;
    
    targetLUFS_ = LUFS_REFERENCE;
    integrationTimeSeconds_ = 3.0f;
    maxGainReductionDB_ = 12.0f;
    maxGainBoostDB_ = 6.0f;
    gainSmoothingTimeMs_ = 50.0f;
    
    // Initialize K-weighting filters
    leftKFilter_ = {};
    rightKFilter_ = {};
    
    // Initialize integration buffer
    loudnessBuffer_.fill(-70.0f);
    bufferIndex_ = 0;
    bufferFull_ = false;
    
    // Initialize measurements
    instantaneousLoudness_ = MIN_LUFS;
    integratedLoudness_ = MIN_LUFS;
    meanSquareSum_ = 0.0f;
    integrationSamples_ = 0;
    
    // Initialize gain control
    currentGain_ = 1.0f;
    targetGain_ = 1.0f;
    gainSmoothingCoeff_ = 0.99f;
    
    // Initialize reference
    hasReference_ = false;
    referenceLUFS_ = LUFS_REFERENCE;
    
    cpuUsage_ = 0.0f;
}

bool LUFSNormalizer::initialize(float sampleRate, bool stereoInput) {
    if (sampleRate <= 0.0f) {
        return false;
    }
    
    sampleRate_ = sampleRate;
    stereoInput_ = stereoInput;
    
    calculateFilterCoefficients();
    
    // Calculate integration parameters
    integrationSamples_ = static_cast<int>(integrationTimeSeconds_ * sampleRate_);
    integrationSamples_ = std::min(integrationSamples_, INTEGRATION_BUFFER_SIZE);
    
    // Calculate gain smoothing coefficient
    float smoothingTimeSeconds = gainSmoothingTimeMs_ * 0.001f;
    gainSmoothingCoeff_ = std::exp(-1.0f / (smoothingTimeSeconds * sampleRate_));
    gainSmoothingCoeff_ = clamp(gainSmoothingCoeff_, 0.9f, 0.999f);
    
    reset();
    
    initialized_ = true;
    return true;
}

void LUFSNormalizer::shutdown() {
    if (!initialized_) {
        return;
    }
    
    reset();
    initialized_ = false;
}

float LUFSNormalizer::processSample(float input) {
    if (!initialized_ || bypassed_) {
        return input;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Process through K-weighting filter
    float leftK = processKWeighting(input, leftKFilter_);
    float rightK = stereoInput_ ? 0.0f : leftK; // Mono input uses left for both channels
    
    // Calculate instantaneous loudness
    instantaneousLoudness_ = calculateInstantaneousLoudness(leftK, rightK);
    
    // Update integrated loudness
    updateIntegratedLoudness(instantaneousLoudness_);
    
    // Update target gain based on integrated loudness
    updateTargetGain();
    
    // Smooth the gain
    currentGain_ += (targetGain_ - currentGain_) * (1.0f - gainSmoothingCoeff_);
    
    // Apply gain
    float output = input * currentGain_;
    
    // Update CPU usage
    auto endTime = std::chrono::high_resolution_clock::now();
    float processingTime = std::chrono::duration<float, std::micro>(endTime - startTime).count();
    cpuUsage_ = cpuUsage_ * 0.999f + processingTime * 0.001f;
    
    return output;
}

void LUFSNormalizer::processStereoSample(float& left, float& right) {
    if (!initialized_ || bypassed_) {
        return;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Process through K-weighting filters
    float leftK = processKWeighting(left, leftKFilter_);
    float rightK = processKWeighting(right, rightKFilter_);
    
    // Calculate instantaneous loudness
    instantaneousLoudness_ = calculateInstantaneousLoudness(leftK, rightK);
    
    // Update integrated loudness
    updateIntegratedLoudness(instantaneousLoudness_);
    
    // Update target gain based on integrated loudness
    updateTargetGain();
    
    // Smooth the gain
    currentGain_ += (targetGain_ - currentGain_) * (1.0f - gainSmoothingCoeff_);
    
    // Apply gain to both channels
    left *= currentGain_;
    right *= currentGain_;
    
    // Update CPU usage
    auto endTime = std::chrono::high_resolution_clock::now();
    float processingTime = std::chrono::duration<float, std::micro>(endTime - startTime).count();
    cpuUsage_ = cpuUsage_ * 0.999f + processingTime * 0.001f;
}

void LUFSNormalizer::processBlock(float* buffer, int numSamples, bool stereo) {
    if (!initialized_ || bypassed_) {
        return;
    }
    
    if (stereo) {
        // Process stereo interleaved
        for (int i = 0; i < numSamples; i += 2) {
            processStereoSample(buffer[i], buffer[i + 1]);
        }
    } else {
        // Process mono
        for (int i = 0; i < numSamples; i++) {
            buffer[i] = processSample(buffer[i]);
        }
    }
}

void LUFSNormalizer::processStereoBlock(float* leftChannel, float* rightChannel, int numSamples) {
    if (!initialized_ || bypassed_) {
        return;
    }
    
    for (int i = 0; i < numSamples; i++) {
        processStereoSample(leftChannel[i], rightChannel[i]);
    }
}

void LUFSNormalizer::setTargetLUFS(float targetLUFS) {
    targetLUFS_ = clamp(targetLUFS, -50.0f, -6.0f);
}

void LUFSNormalizer::setIntegrationTime(float timeSeconds) {
    integrationTimeSeconds_ = clamp(timeSeconds, 0.1f, 10.0f);
    if (initialized_) {
        integrationSamples_ = static_cast<int>(integrationTimeSeconds_ * sampleRate_);
        integrationSamples_ = std::min(integrationSamples_, INTEGRATION_BUFFER_SIZE);
    }
}

void LUFSNormalizer::setMaxGainReduction(float maxReductionDB) {
    maxGainReductionDB_ = clamp(maxReductionDB, 0.0f, 24.0f);
}

void LUFSNormalizer::setMaxGainBoost(float maxBoostDB) {
    maxGainBoostDB_ = clamp(maxBoostDB, 0.0f, 12.0f);
}

void LUFSNormalizer::setGainSmoothingTime(float timeMs) {
    gainSmoothingTimeMs_ = clamp(timeMs, 1.0f, 500.0f);
    if (initialized_) {
        float smoothingTimeSeconds = gainSmoothingTimeMs_ * 0.001f;
        gainSmoothingCoeff_ = std::exp(-1.0f / (smoothingTimeSeconds * sampleRate_));
        gainSmoothingCoeff_ = clamp(gainSmoothingCoeff_, 0.9f, 0.999f);
    }
}

void LUFSNormalizer::setBypass(bool bypass) {
    bypassed_ = bypass;
}

void LUFSNormalizer::setSampleRate(float sampleRate) {
    if (sampleRate > 0.0f && std::abs(sampleRate - sampleRate_) > 0.1f) {
        bool wasInitialized = initialized_;
        if (wasInitialized) {
            shutdown();
        }
        if (wasInitialized) {
            initialize(sampleRate, stereoInput_);
        }
    }
}

float LUFSNormalizer::getCurrentLUFS() const {
    return instantaneousLoudness_;
}

float LUFSNormalizer::getIntegratedLUFS() const {
    return integratedLoudness_;
}

void LUFSNormalizer::calibrateReference() {
    if (initialized_) {
        referenceLUFS_ = integratedLoudness_;
        hasReference_ = true;
    }
}

void LUFSNormalizer::resetCalibration() {
    hasReference_ = false;
    referenceLUFS_ = LUFS_REFERENCE;
}

void LUFSNormalizer::reset() {
    // Reset K-weighting filters
    leftKFilter_ = {};
    rightKFilter_ = {};
    
    // Reset integration
    loudnessBuffer_.fill(MIN_LUFS);
    bufferIndex_ = 0;
    bufferFull_ = false;
    
    instantaneousLoudness_ = MIN_LUFS;
    integratedLoudness_ = MIN_LUFS;
    meanSquareSum_ = 0.0f;
    integrationSamples_ = 0;
    
    // Reset gain
    currentGain_ = 1.0f;
    targetGain_ = 1.0f;
    
    cpuUsage_ = 0.0f;
}

// Private method implementations

void LUFSNormalizer::calculateFilterCoefficients() {
    // EBU R128 K-weighting filter coefficients for 48kHz
    // Scaled for current sample rate
    float fs = sampleRate_;
    float f0 = fs / 48000.0f; // Scale factor
    
    // Pre-filter (high-pass at ~38Hz)
    float preOmega = 2.0f * static_cast<float>(M_PI) * 38.13547087602444f * f0 / fs;
    float preK = std::tan(preOmega * 0.5f);
    float preNorm = 1.0f / (1.0f + preK * 1.4142135623730951f + preK * preK);
    
    preFilterCoeffs_.b0 = preNorm;
    preFilterCoeffs_.b1 = -2.0f * preNorm;
    preFilterCoeffs_.b2 = preNorm;
    preFilterCoeffs_.a1 = 2.0f * (preK * preK - 1.0f) * preNorm;
    preFilterCoeffs_.a2 = (1.0f - preK * 1.4142135623730951f + preK * preK) * preNorm;
    
    // High-frequency shelf filter (~4kHz, +4dB)
    float shelfFreq = 1681.9744509555319f * f0;
    float shelfOmega = 2.0f * static_cast<float>(M_PI) * shelfFreq / fs;
    float shelfK = std::tan(shelfOmega * 0.5f);
    float shelfQ = 0.7071067811865476f;
    float shelfGain = std::pow(10.0f, 4.0f / 20.0f); // +4dB
    
    float shelfA = std::sqrt(shelfGain);
    float shelfW = shelfOmega;
    float shelfS = 1.0f;
    float shelfAlpha = std::sin(shelfW) / 2.0f * std::sqrt((shelfA + 1.0f / shelfA) * (1.0f / shelfS - 1.0f) + 2.0f);
    float shelfBeta = std::sqrt(shelfA) / shelfQ;
    
    float shelfNorm = 1.0f / ((shelfA + 1.0f) - (shelfA - 1.0f) * std::cos(shelfW) + shelfBeta * std::sin(shelfW));
    
    shelfFilterCoeffs_.b0 = shelfA * ((shelfA + 1.0f) + (shelfA - 1.0f) * std::cos(shelfW) + shelfBeta * std::sin(shelfW)) * shelfNorm;
    shelfFilterCoeffs_.b1 = -2.0f * shelfA * ((shelfA - 1.0f) + (shelfA + 1.0f) * std::cos(shelfW)) * shelfNorm;
    shelfFilterCoeffs_.b2 = shelfA * ((shelfA + 1.0f) + (shelfA - 1.0f) * std::cos(shelfW) - shelfBeta * std::sin(shelfW)) * shelfNorm;
    shelfFilterCoeffs_.a1 = 2.0f * ((shelfA - 1.0f) - (shelfA + 1.0f) * std::cos(shelfW)) * shelfNorm;
    shelfFilterCoeffs_.a2 = ((shelfA + 1.0f) - (shelfA - 1.0f) * std::cos(shelfW) - shelfBeta * std::sin(shelfW)) * shelfNorm;
    
    // Clamp coefficients for stability
    preFilterCoeffs_.b0 = clamp(preFilterCoeffs_.b0, -10.0f, 10.0f);
    preFilterCoeffs_.b1 = clamp(preFilterCoeffs_.b1, -10.0f, 10.0f);
    preFilterCoeffs_.b2 = clamp(preFilterCoeffs_.b2, -10.0f, 10.0f);
    preFilterCoeffs_.a1 = clamp(preFilterCoeffs_.a1, -1.99f, 1.99f);
    preFilterCoeffs_.a2 = clamp(preFilterCoeffs_.a2, -0.99f, 0.99f);
    
    shelfFilterCoeffs_.b0 = clamp(shelfFilterCoeffs_.b0, -10.0f, 10.0f);
    shelfFilterCoeffs_.b1 = clamp(shelfFilterCoeffs_.b1, -10.0f, 10.0f);
    shelfFilterCoeffs_.b2 = clamp(shelfFilterCoeffs_.b2, -10.0f, 10.0f);
    shelfFilterCoeffs_.a1 = clamp(shelfFilterCoeffs_.a1, -1.99f, 1.99f);
    shelfFilterCoeffs_.a2 = clamp(shelfFilterCoeffs_.a2, -0.99f, 0.99f);
}

float LUFSNormalizer::processKWeighting(float input, KWeightingFilter& filter) const {
    // Pre-filter (high-pass)
    float preOut = preFilterCoeffs_.b0 * input + preFilterCoeffs_.b1 * filter.preX1 + preFilterCoeffs_.b2 * filter.preX2
                   - preFilterCoeffs_.a1 * filter.preY1 - preFilterCoeffs_.a2 * filter.preY2;
    
    filter.preX2 = filter.preX1;
    filter.preX1 = input;
    filter.preY2 = filter.preY1;
    filter.preY1 = preOut;
    
    // High-frequency shelf filter
    float shelfOut = shelfFilterCoeffs_.b0 * preOut + shelfFilterCoeffs_.b1 * filter.shelfX1 + shelfFilterCoeffs_.b2 * filter.shelfX2
                     - shelfFilterCoeffs_.a1 * filter.shelfY1 - shelfFilterCoeffs_.a2 * filter.shelfY2;
    
    filter.shelfX2 = filter.shelfX1;
    filter.shelfX1 = preOut;
    filter.shelfY2 = filter.shelfY1;
    filter.shelfY1 = shelfOut;
    
    return shelfOut;
}

float LUFSNormalizer::calculateInstantaneousLoudness(float leftK, float rightK) const {
    // Calculate mean square of K-weighted signals
    float leftMS = leftK * leftK;
    float rightMS = rightK * rightK;
    
    // For stereo: average both channels; for mono: use left channel only  
    float meanSquare = stereoInput_ ? (leftMS + rightMS) * 0.5f : leftMS;
    
    // Avoid log of zero
    meanSquare = std::max(meanSquare, 1e-10f);
    
    // Convert to LUFS: -0.691 + 10*log10(meanSquare)
    float lufs = -0.691f + 10.0f * std::log10(meanSquare);
    
    return clamp(lufs, MIN_LUFS, MAX_LUFS);
}

void LUFSNormalizer::updateIntegratedLoudness(float instantaneous) {
    // Store in circular buffer
    loudnessBuffer_[bufferIndex_] = instantaneous;
    bufferIndex_ = (bufferIndex_ + 1) % integrationSamples_;
    
    if (bufferIndex_ == 0 && !bufferFull_) {
        bufferFull_ = true;
    }
    
    // Calculate integrated loudness
    if (bufferFull_ || bufferIndex_ > 100) { // Need minimum samples for stable measurement
        float sum = 0.0f;
        int count = 0;
        int samples = bufferFull_ ? integrationSamples_ : bufferIndex_;
        
        for (int i = 0; i < samples; i++) {
            float lufs = loudnessBuffer_[i];
            if (lufs > GATE_THRESHOLD) { // Apply gating
                sum += std::pow(10.0f, lufs / 10.0f);
                count++;
            }
        }
        
        if (count > 0) {
            integratedLoudness_ = 10.0f * std::log10(sum / count);
            integratedLoudness_ = clamp(integratedLoudness_, MIN_LUFS, MAX_LUFS);
        }
    }
}

void LUFSNormalizer::updateTargetGain() {
    // Use reference LUFS if calibrated, otherwise use target
    float referenceLufs = hasReference_ ? referenceLUFS_ : targetLUFS_;
    
    // Calculate gain needed to reach target
    float lufsError = referenceLufs - integratedLoudness_;
    
    // Convert LUFS difference to linear gain
    float gainDB = lufsError;
    
    // Clamp to safe limits
    gainDB = clamp(gainDB, -maxGainReductionDB_, maxGainBoostDB_);
    
    // Convert to linear gain
    targetGain_ = std::pow(10.0f, gainDB / 20.0f);
    targetGain_ = clamp(targetGain_, 0.1f, 4.0f);
}

float LUFSNormalizer::lufsToLinear(float lufs) const {
    return std::pow(10.0f, (lufs + 0.691f) / 10.0f);
}

float LUFSNormalizer::linearToLUFS(float linear) const {
    linear = std::max(linear, 1e-10f);
    return -0.691f + 10.0f * std::log10(linear);
}

float LUFSNormalizer::clamp(float value, float min, float max) const {
    return std::max(min, std::min(max, value));
}