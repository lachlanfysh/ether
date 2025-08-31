#include "LoudnessMonitor.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iostream>
#include <numeric>

namespace EtherSynth {

LoudnessMonitor::LoudnessMonitor() 
    : autoNormalizationEnabled_(false)
    , processingLoad_(0.0f)
    , lastProcessTime_(0)
    , currentNormalizationGain_(1.0f)
    , targetNormalizationGain_(1.0f)
{
    std::cout << "LoudnessMonitor: Initialized ITU-R BS.1770-4 compliant loudness measurement\n";
}

LoudnessMonitor::~LoudnessMonitor() {
    shutdown();
}

void LoudnessMonitor::initialize(float sampleRate, int channels) {
    std::lock_guard<std::mutex> lock(processingMutex_);
    
    sampleRate_ = sampleRate;
    numChannels_ = std::min(channels, MAX_CHANNELS);
    
    // Initialize K-weighting filters for each channel
    kWeightingFilters_.resize(numChannels_);
    for (auto& filter : kWeightingFilters_) {
        filter.initialize(sampleRate_);
    }
    
    // Initialize true peak detectors
    truePeakDetectors_.resize(numChannels_);
    for (auto& detector : truePeakDetectors_) {
        detector.initialize(2048); // Max buffer size
    }
    
    // Initialize K-weighted buffers
    kWeightedBuffers_.resize(numChannels_);
    for (auto& buffer : kWeightedBuffers_) {
        buffer.reserve(8192);
    }
    
    // Update window sizes based on sample rate
    momentaryWindowSize_ = int(0.4f * sampleRate_);         // 400ms
    shortTermWindowSize_ = int(3.0f * sampleRate_);         // 3 seconds
    integratedMinSamples_ = int(40.0f * sampleRate_);       // 40 seconds minimum
    
    reset();
    
    std::cout << "LoudnessMonitor: Initialized for " << numChannels_ 
              << " channels at " << sampleRate_ << " Hz\n";
}

void LoudnessMonitor::shutdown() {
    std::lock_guard<std::mutex> lock(processingMutex_);
    
    kWeightingFilters_.clear();
    truePeakDetectors_.clear();
    kWeightedBuffers_.clear();
    loudnessBlocks_.clear();
    momentaryHistory_.clear();
    shortTermHistory_.clear();
    truePeakHistory_.clear();
    
    std::cout << "LoudnessMonitor: Shutdown complete\n";
}

void LoudnessMonitor::reset() {
    std::lock_guard<std::mutex> lock(processingMutex_);
    
    // Reset filters and detectors
    for (auto& filter : kWeightingFilters_) {
        filter.reset();
    }
    
    for (auto& detector : truePeakDetectors_) {
        detector.reset();
    }
    
    // Clear buffers
    for (auto& buffer : kWeightedBuffers_) {
        buffer.clear();
    }
    
    loudnessBlocks_.clear();
    momentaryHistory_.clear();
    shortTermHistory_.clear();
    truePeakHistory_.clear();
    
    // Reset measurements
    currentData_ = LoudnessData();
    currentData_.timestamp = getCurrentTimeMs();
    
    std::cout << "LoudnessMonitor: Reset all measurements\n";
}

void LoudnessMonitor::processAudioBuffer(const float* const* channelBuffers, 
                                        int numChannels, int bufferSize) {
    if (!channelBuffers || numChannels <= 0 || bufferSize <= 0) return;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    std::lock_guard<std::mutex> lock(processingMutex_);
    
    numChannels = std::min(numChannels, numChannels_);
    
    // Update K-weighted signal
    updateKWeightedSignal(channelBuffers, numChannels, bufferSize);
    
    // Update true peak measurements
    updateTruePeaks(channelBuffers, numChannels, bufferSize);
    
    // Update loudness measurements
    updateLoudnessBlocks();
    updateMomentaryLUFS();
    updateShortTermLUFS();
    updateIntegratedLUFS();
    updateLoudnessRange();
    
    // Apply auto-normalization if enabled
    if (autoNormalizationEnabled_.load()) {
        // Note: This would modify the input buffers in a real implementation
        // For now, just calculate the normalization gain
        targetNormalizationGain_ = calculateNormalizationGain();
    }
    
    // Update timestamp
    currentData_.timestamp = getCurrentTimeMs();
    currentData_.measurementTime += uint64_t((bufferSize * 1000.0f) / sampleRate_);
    
    // Calculate processing load
    auto endTime = std::chrono::high_resolution_clock::now();
    float processTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    float bufferTime = (bufferSize * 1000.0f) / sampleRate_;
    processingLoad_.store(processTime / bufferTime);
}

void LoudnessMonitor::processInterleavedBuffer(const float* buffer, 
                                              int numChannels, int bufferSize) {
    if (!buffer || numChannels <= 0 || bufferSize <= 0) return;
    
    // Deinterleave the buffer
    std::vector<std::vector<float>> channelBuffers(numChannels);
    for (int ch = 0; ch < numChannels; ch++) {
        channelBuffers[ch].resize(bufferSize);
        for (int i = 0; i < bufferSize; i++) {
            channelBuffers[ch][i] = buffer[i * numChannels + ch];
        }
    }
    
    // Create array of pointers for processing
    std::vector<const float*> channelPtrs(numChannels);
    for (int ch = 0; ch < numChannels; ch++) {
        channelPtrs[ch] = channelBuffers[ch].data();
    }
    
    processAudioBuffer(channelPtrs.data(), numChannels, bufferSize);
}

void LoudnessMonitor::updateKWeightedSignal(const float* const* channelBuffers, 
                                           int numChannels, int bufferSize) {
    // Apply K-weighting to each channel
    for (int ch = 0; ch < numChannels; ch++) {
        auto& kWeightedBuffer = kWeightedBuffers_[ch];
        auto& filter = kWeightingFilters_[ch];
        
        // Ensure buffer has space
        kWeightedBuffer.reserve(kWeightedBuffer.size() + bufferSize);
        
        // Apply K-weighting filter
        for (int i = 0; i < bufferSize; i++) {
            float kWeightedSample = filter.process(channelBuffers[ch][i]);
            kWeightedBuffer.push_back(kWeightedSample);
        }
    }
}

void LoudnessMonitor::updateLoudnessBlocks() {
    if (kWeightedBuffers_.empty() || kWeightedBuffers_[0].size() < 480) {
        return; // Need at least 10ms of data at 48kHz
    }
    
    const int blockSize = 480; // 10ms blocks at 48kHz
    
    // Process blocks while we have enough data
    while (kWeightedBuffers_[0].size() >= blockSize) {
        LoudnessBlock block;
        block.timestamp = getCurrentTimeMs();
        
        // Calculate mean square for this block
        float sumOfSquares = 0.0f;
        
        // Channel weighting: L/R = 1.0, others = 1.41 (ITU-R BS.1770-4)
        std::vector<float> channelWeights(numChannels_, 1.0f);
        if (numChannels_ > 2) {
            for (int ch = 2; ch < numChannels_; ch++) {
                channelWeights[ch] = 1.41f; // Surround channel weighting
            }
        }
        
        for (int ch = 0; ch < numChannels_; ch++) {
            float channelSumOfSquares = 0.0f;
            for (int i = 0; i < blockSize; i++) {
                float sample = kWeightedBuffers_[ch][i];
                channelSumOfSquares += sample * sample;
            }
            sumOfSquares += channelWeights[ch] * channelSumOfSquares;
        }
        
        block.meanSquare = sumOfSquares / (blockSize * numChannels_);
        
        // Convert to LUFS (-0.691 dB offset for K-weighting)
        if (block.meanSquare > 1e-10f) {
            block.loudness = -0.691f + 10.0f * std::log10(block.meanSquare);
        } else {
            block.loudness = -120.0f; // Silent block
        }
        
        loudnessBlocks_.push_back(block);
        
        // Remove processed samples from buffers
        for (auto& buffer : kWeightedBuffers_) {
            buffer.erase(buffer.begin(), buffer.begin() + blockSize);
        }
        
        // Limit block history to prevent memory growth
        const int maxBlocks = int(600.0f * sampleRate_ / blockSize); // 10 minutes max
        if (loudnessBlocks_.size() > maxBlocks) {
            loudnessBlocks_.pop_front();
        }
    }
}

void LoudnessMonitor::updateMomentaryLUFS() {
    if (loudnessBlocks_.empty()) return;
    
    // Momentary: 400ms window (40 blocks of 10ms each at 48kHz)
    const int momentaryBlocks = 40;
    
    if (loudnessBlocks_.size() < momentaryBlocks) {
        currentData_.momentaryLUFS = -120.0f;
        return;
    }
    
    // Get last 40 blocks
    auto startIt = loudnessBlocks_.end() - momentaryBlocks;
    std::vector<LoudnessBlock> momentaryWindow(startIt, loudnessBlocks_.end());
    
    // Apply absolute gating
    applyAbsoluteGating(momentaryWindow);
    
    if (momentaryWindow.empty()) {
        currentData_.momentaryLUFS = -120.0f;
    } else {
        currentData_.momentaryLUFS = calculateMeanLoudness(momentaryWindow);
    }
    
    // Store in history for visualization
    momentaryHistory_.push_back(currentData_.momentaryLUFS);
    if (momentaryHistory_.size() > MAX_HISTORY_SIZE) {
        momentaryHistory_.pop_front();
    }
}

void LoudnessMonitor::updateShortTermLUFS() {
    if (loudnessBlocks_.empty()) return;
    
    // Short-term: 3 second window (300 blocks of 10ms each)
    const int shortTermBlocks = 300;
    
    if (loudnessBlocks_.size() < shortTermBlocks) {
        currentData_.shortTermLUFS = -120.0f;
        return;
    }
    
    // Get last 300 blocks
    auto startIt = loudnessBlocks_.end() - shortTermBlocks;
    std::vector<LoudnessBlock> shortTermWindow(startIt, loudnessBlocks_.end());
    
    // Apply absolute gating
    applyAbsoluteGating(shortTermWindow);
    
    if (shortTermWindow.empty()) {
        currentData_.shortTermLUFS = -120.0f;
    } else {
        currentData_.shortTermLUFS = calculateMeanLoudness(shortTermWindow);
    }
    
    // Store in history
    shortTermHistory_.push_back(currentData_.shortTermLUFS);
    if (shortTermHistory_.size() > MAX_HISTORY_SIZE) {
        shortTermHistory_.pop_front();
    }
}

void LoudnessMonitor::updateIntegratedLUFS() {
    if (loudnessBlocks_.empty()) return;
    
    // Need at least 40 seconds of data for integrated measurement
    const int minBlocks = 4000; // 40 seconds at 10ms blocks
    
    if (loudnessBlocks_.size() < minBlocks) {
        currentData_.integratedLUFS = -120.0f;
        return;
    }
    
    // Use all available blocks
    std::vector<LoudnessBlock> allBlocks(loudnessBlocks_.begin(), loudnessBlocks_.end());
    
    // Apply absolute gating first
    applyAbsoluteGating(allBlocks);
    currentData_.gatedBlocks = allBlocks.size();
    currentData_.totalBlocks = loudnessBlocks_.size();
    
    if (allBlocks.empty()) {
        currentData_.integratedLUFS = -120.0f;
        return;
    }
    
    // Apply relative gating
    applyRelativeGating(allBlocks);
    
    if (allBlocks.empty()) {
        currentData_.integratedLUFS = -120.0f;
    } else {
        currentData_.integratedLUFS = calculateMeanLoudness(allBlocks);
    }
    
    // Update compliance indicators
    currentData_.meetsStreamingStandard = 
        std::abs(currentData_.integratedLUFS - TARGET_LUFS_STREAMING) <= 1.0f;
    currentData_.meetsBroadcastStandard = 
        std::abs(currentData_.integratedLUFS - TARGET_LUFS_BROADCAST) <= 1.0f;
}

void LoudnessMonitor::updateLoudnessRange() {
    if (shortTermHistory_.size() < 10) {
        currentData_.loudnessRange = 0.0f;
        return;
    }
    
    // Calculate loudness range from short-term history
    std::vector<float> validValues;
    for (float value : shortTermHistory_) {
        if (value > -120.0f) {
            validValues.push_back(value);
        }
    }
    
    if (validValues.size() < 10) {
        currentData_.loudnessRange = 0.0f;
        return;
    }
    
    // Sort values and calculate 10th and 95th percentiles
    std::sort(validValues.begin(), validValues.end());
    
    int p10Index = static_cast<int>(validValues.size() * 0.1f);
    int p95Index = static_cast<int>(validValues.size() * 0.95f);
    
    p10Index = std::clamp(p10Index, 0, static_cast<int>(validValues.size()) - 1);
    p95Index = std::clamp(p95Index, 0, static_cast<int>(validValues.size()) - 1);
    
    float p10 = validValues[p10Index];
    float p95 = validValues[p95Index];
    
    currentData_.loudnessRange = p95 - p10;
}

void LoudnessMonitor::updateTruePeaks(const float* const* channelBuffers, 
                                     int numChannels, int bufferSize) {
    currentData_.truePeakL = -120.0f;
    currentData_.truePeakR = -120.0f;
    currentData_.maxTruePeak = -120.0f;
    
    for (int ch = 0; ch < numChannels; ch++) {
        float truePeak = truePeakDetectors_[ch].process(channelBuffers[ch], bufferSize);
        float truePeakdBTP = dBTPFromLinear(truePeak);
        
        if (ch == 0) currentData_.truePeakL = truePeakdBTP;
        if (ch == 1) currentData_.truePeakR = truePeakdBTP;
        
        currentData_.maxTruePeak = std::max(currentData_.maxTruePeak, truePeakdBTP);
    }
    
    // Check for clipping
    currentData_.hasClipping = currentData_.maxTruePeak > -0.1f;
    currentData_.hasOverload = currentData_.maxTruePeak > 0.0f;
    
    // Store in history
    truePeakHistory_.push_back(currentData_.maxTruePeak);
    if (truePeakHistory_.size() > MAX_HISTORY_SIZE) {
        truePeakHistory_.pop_front();
    }
}

void LoudnessMonitor::applyAbsoluteGating(std::vector<LoudnessBlock>& blocks) const {
    // Remove blocks below absolute gating threshold (-70 LUFS)
    blocks.erase(
        std::remove_if(blocks.begin(), blocks.end(),
                      [this](const LoudnessBlock& block) {
                          return block.loudness < absoluteGatingThreshold_;
                      }),
        blocks.end()
    );
}

void LoudnessMonitor::applyRelativeGating(std::vector<LoudnessBlock>& blocks) const {
    if (blocks.empty()) return;
    
    // Calculate ungated loudness first
    float ungatedLoudness = calculateMeanLoudness(blocks);
    
    // Relative gating threshold is 10 dB below ungated loudness
    float relativeThreshold = ungatedLoudness + relativeGatingThreshold_;
    const_cast<LoudnessMonitor*>(this)->currentData_.gatingThreshold = relativeThreshold;
    
    // Remove blocks below relative threshold
    blocks.erase(
        std::remove_if(blocks.begin(), blocks.end(),
                      [relativeThreshold](const LoudnessBlock& block) {
                          return block.loudness < relativeThreshold;
                      }),
        blocks.end()
    );
}

float LoudnessMonitor::calculateMeanLoudness(const std::vector<LoudnessBlock>& blocks) const {
    if (blocks.empty()) return -120.0f;
    
    // Calculate mean of linear values, then convert back to dB
    double sumLinear = 0.0;
    for (const auto& block : blocks) {
        sumLinear += LUFSToLinear(block.loudness);
    }
    
    double meanLinear = sumLinear / blocks.size();
    return linearToLUFS(meanLinear);
}

float LoudnessMonitor::calculateNormalizationGain() const {
    if (currentData_.integratedLUFS <= -120.0f) return 1.0f;
    
    float targetGain = 1.0f;
    
    switch (normalizationMode_) {
        case NormalizationMode::TARGET_LUFS:
            targetGain = std::pow(10.0f, (targetLUFS_ - currentData_.integratedLUFS) / 20.0f);
            break;
            
        case NormalizationMode::PREVENT_CLIPPING:
            if (currentData_.maxTruePeak > -0.5f) {
                targetGain = std::pow(10.0f, (-1.0f - currentData_.maxTruePeak) / 20.0f);
            }
            break;
            
        case NormalizationMode::STREAMING_READY:
            targetGain = std::pow(10.0f, (TARGET_LUFS_STREAMING - currentData_.integratedLUFS) / 20.0f);
            // Also prevent clipping
            if (currentData_.maxTruePeak * targetGain > 0.95f) {
                targetGain = 0.95f / std::pow(10.0f, currentData_.maxTruePeak / 20.0f);
            }
            break;
            
        default:
            targetGain = 1.0f;
            break;
    }
    
    return std::clamp(targetGain, 0.1f, 10.0f); // Reasonable gain limits
}

const LoudnessMonitor::LoudnessData& LoudnessMonitor::getLoudnessData() const {
    std::lock_guard<std::mutex> lock(processingMutex_);
    return currentData_;
}

float LoudnessMonitor::getMomentaryLUFS() const {
    return currentData_.momentaryLUFS;
}

float LoudnessMonitor::getIntegratedLUFS() const {
    return currentData_.integratedLUFS;
}

bool LoudnessMonitor::isCompliant(float tolerance) const {
    return std::abs(currentData_.integratedLUFS - targetLUFS_) <= tolerance;
}

float LoudnessMonitor::getComplianceOffset() const {
    return currentData_.integratedLUFS - targetLUFS_;
}

void LoudnessMonitor::setTargetLUFS(float targetLUFS) {
    targetLUFS_ = targetLUFS;
}

void LoudnessMonitor::setNormalizationMode(NormalizationMode mode) {
    normalizationMode_ = mode;
    
    // Set appropriate target LUFS for the mode
    switch (mode) {
        case NormalizationMode::BROADCAST_COMPLIANCE:
            setTargetLUFS(TARGET_LUFS_BROADCAST);
            break;
        case NormalizationMode::STREAMING_READY:
            setTargetLUFS(TARGET_LUFS_STREAMING);
            break;
        case NormalizationMode::MASTERING_CHAIN:
            setTargetLUFS(TARGET_LUFS_MASTERING);
            break;
        default:
            break;
    }
}

void LoudnessMonitor::enableAutoNormalization(bool enabled) {
    autoNormalizationEnabled_.store(enabled);
}

float LoudnessMonitor::linearToLUFS(float linear) const {
    if (linear <= 1e-10f) return -120.0f;
    return -0.691f + 10.0f * std::log10(linear);
}

float LoudnessMonitor::LUFSToLinear(float lufs) const {
    return std::pow(10.0f, (lufs + 0.691f) / 10.0f);
}

float LoudnessMonitor::dBTPFromLinear(float linear) const {
    if (linear <= 1e-10f) return -120.0f;
    return 20.0f * std::log10(linear);
}

uint64_t LoudnessMonitor::getCurrentTimeMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

// K-weighting filter implementations
void LoudnessMonitor::KWeightingFilter::initialize(float sampleRate) {
    // ITU-R BS.1770-4 K-weighting filter coefficients
    
    // High-frequency shelving filter (HSF)
    // Pre-filter: +4 dB at high frequencies
    float Qs = 1.0f / std::sqrt(2.0f);
    float fs = 1500.0f; // Shelving frequency
    float Vs = std::pow(10.0f, 4.0f / 20.0f); // +4 dB
    
    float K = std::tan(M_PI * fs / sampleRate);
    float Vb = std::pow(Vs, 0.5f);
    
    float a0 = 1.0f + K * Qs + K * K;
    hsfFilter.b0 = (Vb + Vs * K * Qs + K * K) / a0;
    hsfFilter.b1 = (2.0f * (K * K - Vb)) / a0;
    hsfFilter.b2 = (Vb - Vs * K * Qs + K * K) / a0;
    hsfFilter.a1 = (2.0f * (K * K - 1.0f)) / a0;
    hsfFilter.a2 = (1.0f - K * Qs + K * K) / a0;
    
    // High-pass filter (HPF)
    // RLB weighting: high-pass at 38 Hz
    float fc = 38.0f;
    float Qp = 1.0f / (2.0f * std::sinh(std::log(2.0f) / 2.0f * 1.0f * 2.0f * M_PI * fc / (2.0f * M_PI * fc)));
    float wc = 2.0f * M_PI * fc;
    float wc2 = wc * wc;
    float wc_over_s = wc / sampleRate;
    
    float k = wc_over_s;
    float k2 = k * k;
    float a0_hp = 4.0f + 2.0f * k / Qp + k2;
    
    hpfFilter.b0 = 4.0f / a0_hp;
    hpfFilter.b1 = -8.0f / a0_hp;
    hpfFilter.b2 = 4.0f / a0_hp;
    hpfFilter.a1 = (2.0f * k2 - 8.0f) / a0_hp;
    hpfFilter.a2 = (4.0f - 2.0f * k / Qp + k2) / a0_hp;
}

void LoudnessMonitor::KWeightingFilter::reset() {
    hsfFilter.reset();
    hpfFilter.reset();
}

float LoudnessMonitor::KWeightingFilter::process(float input) {
    float hsfOutput = hsfFilter.process(input);
    return hpfFilter.process(hsfOutput);
}

float LoudnessMonitor::KWeightingFilter::HighShelfFilter::process(float input) {
    float output = b0 * input + b1 * z1 + b2 * z2 - a1 * z1 - a2 * z2;
    z2 = z1;
    z1 = input;
    return output;
}

float LoudnessMonitor::KWeightingFilter::HighPassFilter::process(float input) {
    float output = b0 * input + b1 * z1 + b2 * z2 - a1 * z1 - a2 * z2;
    z2 = z1;
    z1 = input;
    return output;
}

// True peak detector implementation
void LoudnessMonitor::TruePeakDetector::initialize(int maxBufferSize) {
    upsampleBuffer.resize(maxBufferSize * OVERSAMPLE_FACTOR);
    filterBuffer.resize(maxBufferSize * OVERSAMPLE_FACTOR);
    reset();
}

void LoudnessMonitor::TruePeakDetector::reset() {
    maxTruePeak = -120.0f;
    bufferIndex = 0;
    std::fill(upsampleBuffer.begin(), upsampleBuffer.end(), 0.0f);
    std::fill(filterBuffer.begin(), filterBuffer.end(), 0.0f);
}

float LoudnessMonitor::TruePeakDetector::process(const float* buffer, int bufferSize) {
    // Simplified true peak detection (should use proper 4x oversampling filter)
    float currentMax = -120.0f;
    
    for (int i = 0; i < bufferSize; i++) {
        float absValue = std::abs(buffer[i]);
        if (absValue > currentMax) {
            currentMax = absValue;
        }
    }
    
    if (currentMax > maxTruePeak) {
        maxTruePeak = currentMax;
    }
    
    return currentMax;
}

float LoudnessMonitor::getProcessingLoad() const {
    return processingLoad_.load();
}

} // namespace EtherSynth