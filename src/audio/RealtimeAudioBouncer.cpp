#include "RealtimeAudioBouncer.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <chrono>
#include <random>

RealtimeAudioBouncer::RealtimeAudioBouncer() :
    status_(BounceStatus::IDLE),
    targetDurationMs_(0),
    startTimeMs_(0),
    samplesRecorded_(0),
    targetSampleCount_(0),
    bufferSize_(DEFAULT_BUFFER_SIZE),
    peakLevel_(0.0f),
    rmsLevel_(0.0f),
    rmsAccumulator_(0.0f),
    rmsSampleCount_(0),
    outputFile_(),
    bytesWritten_(0),
    srcRatio_(1.0f),
    srcState_(0),
    limiterState_(0.0f),
    performanceUpdateCounter_(0),
    totalProcessingTime_(0) {
    
    writeIndex_.store(0);
    readIndex_.store(0);
    highpassState_[0] = 0.0f;
    highpassState_[1] = 0.0f;
    
    initializeBuffers();
}

RealtimeAudioBouncer::~RealtimeAudioBouncer() {
    if (isActive()) {
        cancelBounce();
    }
    
    outputFile_.close();
}

// Configuration
void RealtimeAudioBouncer::setBounceConfig(const BounceConfig& config) {
    if (isActive()) {
        return;  // Cannot change config while bouncing
    }
    
    if (validateConfig(config)) {
        config_ = config;
        
        // Update sample rate conversion ratio
        if (sampleRateCallback_) {
            uint32_t systemSampleRate = sampleRateCallback_();
            srcRatio_ = static_cast<float>(config_.sampleRate) / static_cast<float>(systemSampleRate);
        }
        
        // Resize buffers if needed
        if (config_.bufferSizeFrames != bufferSize_) {
            bufferSize_ = config_.bufferSizeFrames;
            initializeBuffers();
        }
    }
}

void RealtimeAudioBouncer::setProcessingParams(const ProcessingParams& params) {
    ProcessingParams sanitizedParams = params;
    sanitizeProcessingParams(sanitizedParams);
    processingParams_ = sanitizedParams;
}

// Bounce Operations
bool RealtimeAudioBouncer::startBounce(const std::string& outputPath, uint32_t durationMs) {
    if (isActive()) {
        return false;  // Already bouncing
    }
    
    if (!validateOutputPath(outputPath)) {
        updateStatus(BounceStatus::ERROR, "Invalid output path");
        return false;
    }
    
    // Check if file exists and we're not allowed to overwrite
    if (!config_.overwriteExisting) {
        FILE* existingFile = fopen(outputPath.c_str(), "r");
        if (existingFile) {
            fclose(existingFile);
            updateStatus(BounceStatus::ERROR, "File already exists");
            return false;
        }
    }
    
    updateStatus(BounceStatus::INITIALIZING, "Initializing bounce operation");
    
    // Store bounce parameters
    targetDurationMs_ = std::min(durationMs, config_.maxRecordingLengthMs);
    currentOutputPath_ = outputPath;
    startTimeMs_ = getCurrentTimeMs();
    samplesRecorded_ = 0;
    targetSampleCount_ = (static_cast<uint64_t>(targetDurationMs_) * static_cast<uint32_t>(config_.sampleRate)) / 1000;
    
    // Reset buffers and meters
    resetBuffers();
    resetLevelMeters();
    
    // Create output file
    if (!createOutputFile(outputPath)) {
        updateStatus(BounceStatus::ERROR, "Failed to create output file");
        return false;
    }
    
    updateStatus(BounceStatus::RECORDING, "Recording audio");
    return true;
}

bool RealtimeAudioBouncer::pauseBounce() {
    if (status_.load() != BounceStatus::RECORDING) {
        return false;
    }
    
    updateStatus(BounceStatus::PROCESSING, "Bounce paused");
    return true;
}

bool RealtimeAudioBouncer::resumeBounce() {
    if (status_.load() != BounceStatus::PROCESSING) {
        return false;
    }
    
    updateStatus(BounceStatus::RECORDING, "Recording resumed");
    return true;
}

void RealtimeAudioBouncer::stopBounce() {
    if (!isActive()) {
        return;
    }
    
    updateStatus(BounceStatus::FINALIZING, "Finalizing bounce");
    
    // Flush any remaining audio data
    flushBuffers();
    
    // Finalize the output file
    if (outputFile_) {
        finalizeOutputFile();
        outputFile_.close();
    }
    
    // Create result
    BounceResult result;
    result.status = BounceStatus::COMPLETED;
    result.outputFilePath = currentOutputPath_;
    result.totalSamples = samplesRecorded_;
    result.durationMs = (samplesRecorded_ * 1000) / static_cast<uint32_t>(config_.sampleRate);
    result.peakLevel = peakLevel_;
    result.rmsLevel = rmsLevel_;
    result.fileSizeBytes = bytesWritten_;
    
    updateStatus(BounceStatus::COMPLETED, "Bounce completed successfully");
    
    if (completedCallback_) {
        completedCallback_(result);
    }
}

void RealtimeAudioBouncer::cancelBounce() {
    if (!isActive()) {
        return;
    }
    
    updateStatus(BounceStatus::CANCELLED, "Bounce cancelled");
    
    // Close and delete output file
    if (outputFile_) {
        outputFile_.close();
        remove(currentOutputPath_.c_str());  // Delete incomplete file
    }
    
    BounceResult result;
    result.status = BounceStatus::CANCELLED;
    result.errorMessage = "Operation cancelled by user";
    
    if (completedCallback_) {
        completedCallback_(result);
    }
}

// State Management
RealtimeAudioBouncer::AudioMetrics RealtimeAudioBouncer::getCurrentMetrics() const {
    AudioMetrics metrics = currentMetrics_;
    metrics.currentPeakLevel = peakLevel_;
    metrics.currentRmsLevel = rmsLevel_;
    metrics.samplesProcessed = samplesRecorded_;
    return metrics;
}

float RealtimeAudioBouncer::getProgressPercentage() const {
    if (targetSampleCount_ == 0) {
        return 0.0f;
    }
    return std::min(1.0f, static_cast<float>(samplesRecorded_) / static_cast<float>(targetSampleCount_));
}

uint32_t RealtimeAudioBouncer::getElapsedTimeMs() const {
    if (!isActive()) {
        return 0;
    }
    return getCurrentTimeMs() - startTimeMs_;
}

uint32_t RealtimeAudioBouncer::getRemainingTimeMs() const {
    uint32_t elapsed = getElapsedTimeMs();
    if (elapsed >= targetDurationMs_) {
        return 0;
    }
    return targetDurationMs_ - elapsed;
}

// Audio Processing
void RealtimeAudioBouncer::processAudioBlock(const float* inputBuffer, uint32_t sampleCount) {
    if (!isRecording() || !inputBuffer) {
        return;
    }
    
    uint32_t processStartTime = getCurrentTimeMs();
    
    // Create working buffer for processing
    std::vector<float> workBuffer(sampleCount * config_.channels);
    
    if (config_.channels == 1) {
        // Mono processing
        std::copy(inputBuffer, inputBuffer + sampleCount, workBuffer.data());
    } else {
        // Stereo processing - assume interleaved input
        std::copy(inputBuffer, inputBuffer + sampleCount * 2, workBuffer.data());
    }
    
    // Apply input gain
    if (processingParams_.inputGain != 1.0f) {
        for (uint32_t i = 0; i < workBuffer.size(); ++i) {
            workBuffer[i] *= processingParams_.inputGain;
        }
    }
    
    // Apply highpass filter if enabled
    if (processingParams_.enableHighpassFilter) {
        processHighpassFilter(workBuffer.data(), workBuffer.size());
    }
    
    // Apply limiter if enabled
    if (processingParams_.enableLimiter) {
        processLimiter(workBuffer.data(), workBuffer.size());
    }
    
    // Apply output gain
    if (processingParams_.outputGain != 1.0f) {
        for (uint32_t i = 0; i < workBuffer.size(); ++i) {
            workBuffer[i] *= processingParams_.outputGain;
        }
    }
    
    // Update level meters
    updateLevelMeters(workBuffer.data(), workBuffer.size());
    
    // Convert to output format and write
    convertToOutputFormat(workBuffer.data(), workBuffer.size() / config_.channels);
    
    // Update performance metrics
    uint32_t processingTime = getCurrentTimeMs() - processStartTime;
    totalProcessingTime_ += processingTime;
    performanceUpdateCounter_++;
    
    if (performanceUpdateCounter_ >= METRICS_UPDATE_INTERVAL) {
        updateMetrics();
        performanceUpdateCounter_ = 0;
    }
    
    // Check if we've reached target duration
    if (samplesRecorded_ >= targetSampleCount_) {
        stopBounce();
    }
}

void RealtimeAudioBouncer::processInterleavedStereo(const float* leftBuffer, const float* rightBuffer, uint32_t sampleCount) {
    if (!isRecording() || !leftBuffer || !rightBuffer) {
        return;
    }
    
    // Interleave stereo channels
    std::vector<float> interleavedBuffer(sampleCount * 2);
    for (uint32_t i = 0; i < sampleCount; ++i) {
        interleavedBuffer[i * 2] = leftBuffer[i];
        interleavedBuffer[i * 2 + 1] = rightBuffer[i];
    }
    
    processAudioBlock(interleavedBuffer.data(), sampleCount);
}

void RealtimeAudioBouncer::processMono(const float* inputBuffer, uint32_t sampleCount) {
    if (config_.channels == 1) {
        processAudioBlock(inputBuffer, sampleCount);
    } else {
        // Convert mono to stereo by duplicating
        std::vector<float> stereoBuffer(sampleCount * 2);
        for (uint32_t i = 0; i < sampleCount; ++i) {
            stereoBuffer[i * 2] = inputBuffer[i];
            stereoBuffer[i * 2 + 1] = inputBuffer[i];
        }
        processAudioBlock(stereoBuffer.data(), sampleCount);
    }
}

// Format Conversion
void RealtimeAudioBouncer::convertToOutputFormat(const float* inputBuffer, uint32_t sampleCount) {
    if (!inputBuffer || sampleCount == 0) {
        return;
    }
    
    // Apply sample rate conversion if needed
    std::vector<float> srcBuffer;
    const float* processBuffer = inputBuffer;
    uint32_t processSampleCount = sampleCount;
    
    if (srcRatio_ != 1.0f) {
        uint32_t outputSamples = static_cast<uint32_t>(sampleCount * srcRatio_);
        srcBuffer.resize(outputSamples * config_.channels);
        applySampleRateConversion(inputBuffer, sampleCount * config_.channels, 
                                 srcBuffer.data(), outputSamples);
        processBuffer = srcBuffer.data();
        processSampleCount = outputSamples;
    }
    
    // Apply normalization if enabled
    std::vector<float> normalizedBuffer;
    if (config_.enableNormalization) {
        normalizedBuffer.assign(processBuffer, processBuffer + processSampleCount * config_.channels);
        applyNormalization(normalizedBuffer.data(), normalizedBuffer.size(), config_.normalizationLevel);
        processBuffer = normalizedBuffer.data();
    }
    
    // Apply dithering if enabled and reducing bit depth
    if (config_.enableDithering && config_.format != AudioFormat::WAV_32BIT_FLOAT && 
        config_.format != AudioFormat::RAW_PCM_32_FLOAT) {
        std::vector<float> ditheredBuffer(processBuffer, processBuffer + processSampleCount * config_.channels);
        applyDithering(ditheredBuffer.data(), ditheredBuffer.size());
        processBuffer = ditheredBuffer.data();
    }
    
    // Convert to target format
    auto formatData = convertFloatToFormat(processBuffer, processSampleCount * config_.channels, config_.format);
    
    // Write to file
    if (!formatData.empty()) {
        writeAudioData(formatData.data(), formatData.size());
        samplesRecorded_ += processSampleCount;
    }
    
    updateProgress();
}

std::vector<uint8_t> RealtimeAudioBouncer::convertFloatToFormat(const float* samples, uint32_t sampleCount, AudioFormat format) {
    std::vector<uint8_t> output;
    
    switch (format) {
        case AudioFormat::WAV_16BIT:
        case AudioFormat::AIFF_16BIT:
        case AudioFormat::RAW_PCM_16: {
            output.resize(sampleCount * sizeof(int16_t));
            int16_t* int16Output = reinterpret_cast<int16_t*>(output.data());
            convertToInt16(samples, int16Output, sampleCount);
            break;
        }
        
        case AudioFormat::WAV_24BIT:
        case AudioFormat::AIFF_24BIT:
        case AudioFormat::RAW_PCM_24: {
            output.resize(sampleCount * 3);  // 3 bytes per sample
            convertToInt24(samples, output.data(), sampleCount);
            break;
        }
        
        case AudioFormat::WAV_32BIT_FLOAT:
        case AudioFormat::RAW_PCM_32_FLOAT: {
            output.resize(sampleCount * sizeof(float));
            float* floatOutput = reinterpret_cast<float*>(output.data());
            convertToFloat32(samples, floatOutput, sampleCount);
            break;
        }
    }
    
    return output;
}

void RealtimeAudioBouncer::applySampleRateConversion(const float* input, uint32_t inputSamples, 
                                                    float* output, uint32_t& outputSamples) {
    // Simple linear interpolation sample rate conversion
    // For production, would use a more sophisticated algorithm like sinc interpolation
    
    if (srcRatio_ == 1.0f) {
        std::copy(input, input + std::min(inputSamples, outputSamples), output);
        return;
    }
    
    float position = 0.0f;
    uint32_t samplesGenerated = 0;
    
    while (position < inputSamples - 1 && samplesGenerated < outputSamples) {
        uint32_t index = static_cast<uint32_t>(position);
        float fraction = position - index;
        
        // Linear interpolation
        output[samplesGenerated] = input[index] * (1.0f - fraction) + input[index + 1] * fraction;
        
        position += 1.0f / srcRatio_;
        samplesGenerated++;
    }
    
    outputSamples = samplesGenerated;
}

// Audio Analysis
void RealtimeAudioBouncer::updateLevelMeters(const float* buffer, uint32_t sampleCount) {
    float currentPeak = calculatePeakLevel(buffer, sampleCount);
    float currentRms = calculateRmsLevel(buffer, sampleCount);
    
    // Update peak with decay
    peakLevel_ = std::max(currentPeak, peakLevel_ * LEVEL_METER_DECAY);
    
    // Update RMS with rolling average
    rmsAccumulator_ = rmsAccumulator_ * LEVEL_METER_DECAY + currentRms * currentRms * (1.0f - LEVEL_METER_DECAY);
    rmsLevel_ = std::sqrt(rmsAccumulator_);
}

float RealtimeAudioBouncer::calculatePeakLevel(const float* buffer, uint32_t sampleCount) {
    float peak = 0.0f;
    for (uint32_t i = 0; i < sampleCount; ++i) {
        peak = std::max(peak, std::abs(buffer[i]));
    }
    return peak;
}

float RealtimeAudioBouncer::calculateRmsLevel(const float* buffer, uint32_t sampleCount) {
    if (sampleCount == 0) return 0.0f;
    
    float sum = 0.0f;
    for (uint32_t i = 0; i < sampleCount; ++i) {
        sum += buffer[i] * buffer[i];
    }
    return std::sqrt(sum / sampleCount);
}

void RealtimeAudioBouncer::resetLevelMeters() {
    peakLevel_ = 0.0f;
    rmsLevel_ = 0.0f;
    rmsAccumulator_ = 0.0f;
    rmsSampleCount_ = 0;
}

// File I/O
bool RealtimeAudioBouncer::createOutputFile(const std::string& path) {
    if (!outputFile_.open(path, "wb")) {
        return false;
    }
    
    bytesWritten_ = 0;
    
    // Write format header if needed
    if (config_.format == AudioFormat::WAV_16BIT || 
        config_.format == AudioFormat::WAV_24BIT || 
        config_.format == AudioFormat::WAV_32BIT_FLOAT) {
        writeWavHeader(static_cast<uint32_t>(config_.sampleRate), config_.channels, 
                      getBitsPerSample(config_.format), 0);  // Data size will be updated later
    } else if (config_.format == AudioFormat::AIFF_16BIT || 
               config_.format == AudioFormat::AIFF_24BIT) {
        writeAiffHeader(static_cast<uint32_t>(config_.sampleRate), config_.channels,
                       getBitsPerSample(config_.format), 0);  // Data size will be updated later
    }
    
    return true;
}

bool RealtimeAudioBouncer::writeAudioData(const uint8_t* data, uint32_t dataSize) {
    if (!outputFile_.is_open() || !data || dataSize == 0) {
        return false;
    }
    
    size_t written = fwrite(data, 1, dataSize, outputFile_);
    if (written == dataSize) {
        bytesWritten_ += dataSize;
        return true;
    }
    
    return false;
}

bool RealtimeAudioBouncer::finalizeOutputFile() {
    if (!outputFile_.is_open()) {
        return false;
    }
    
    // Update file headers with final data size
    if (config_.format == AudioFormat::WAV_16BIT || 
        config_.format == AudioFormat::WAV_24BIT || 
        config_.format == AudioFormat::WAV_32BIT_FLOAT) {
        
        uint32_t dataSize = bytesWritten_ - 44;  // Subtract WAV header size
        
        // Update chunk sizes in WAV header
        fseek(outputFile_, 4, SEEK_SET);
        writeInt32LE(bytesWritten_ - 8);  // File size - 8
        
        fseek(outputFile_, 40, SEEK_SET);
        writeInt32LE(dataSize);  // Data chunk size
        
    } else if (config_.format == AudioFormat::AIFF_16BIT || 
               config_.format == AudioFormat::AIFF_24BIT) {
        
        uint32_t dataSize = bytesWritten_ - 54;  // Subtract AIFF header size
        
        // Update AIFF chunk sizes
        fseek(outputFile_, 4, SEEK_SET);
        writeInt32LE(bytesWritten_ - 8);  // File size - 8
        
        fseek(outputFile_, 22, SEEK_SET);
        writeInt32LE(samplesRecorded_);  // Number of sample frames
        
        fseek(outputFile_, 50, SEEK_SET);
        writeInt32LE(dataSize);  // Sound data size
    }
    
    fflush(outputFile_);
    return true;
}

void RealtimeAudioBouncer::writeWavHeader(uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample, uint32_t dataSize) {
    // RIFF header
    fwrite("RIFF", 1, 4, outputFile_);
    writeInt32LE(36 + dataSize);  // File size - 8
    fwrite("WAVE", 1, 4, outputFile_);
    
    // Format chunk
    fwrite("fmt ", 1, 4, outputFile_);
    writeInt32LE(16);  // Format chunk size
    writeInt16LE(isFloatFormat(config_.format) ? 3 : 1);  // Audio format (1=PCM, 3=IEEE float)
    writeInt16LE(channels);
    writeInt32LE(sampleRate);
    writeInt32LE(sampleRate * channels * bitsPerSample / 8);  // Byte rate
    writeInt16LE(channels * bitsPerSample / 8);  // Block align
    writeInt16LE(bitsPerSample);
    
    // Data chunk
    fwrite("data", 1, 4, outputFile_);
    writeInt32LE(dataSize);
    
    bytesWritten_ = 44;  // WAV header size
}

void RealtimeAudioBouncer::writeAiffHeader(uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample, uint32_t dataSize) {
    // FORM header
    fwrite("FORM", 1, 4, outputFile_);
    writeInt32LE(46 + dataSize);  // File size - 8
    fwrite("AIFF", 1, 4, outputFile_);
    
    // Common chunk
    fwrite("COMM", 1, 4, outputFile_);
    writeInt32LE(18);  // Common chunk size
    writeInt16LE(channels);
    writeInt32LE(0);  // Number of sample frames (will be updated)
    writeInt16LE(bitsPerSample);
    
    // IEEE 754 extended precision sample rate (simplified)
    uint16_t exponent = 0x4000 + 15;  // Bias + log2 of typical sample rates
    writeInt16LE(exponent);
    writeInt32LE(sampleRate << 16);
    writeInt32LE(0);
    
    // Sound data chunk
    fwrite("SSND", 1, 4, outputFile_);
    writeInt32LE(8 + dataSize);  // Sound data chunk size
    writeInt32LE(0);  // Offset
    writeInt32LE(0);  // Block size
    
    bytesWritten_ = 54;  // AIFF header size
}

// Buffer Management
bool RealtimeAudioBouncer::initializeBuffers() {
    bufferSize_ = std::max(MIN_BUFFER_SIZE, std::min(config_.bufferSizeFrames, MAX_BUFFER_SIZE));
    
    circularBuffer_.resize(bufferSize_ * config_.channels);
    std::fill(circularBuffer_.begin(), circularBuffer_.end(), 0.0f);
    
    writeIndex_.store(0);
    readIndex_.store(0);
    
    return true;
}

void RealtimeAudioBouncer::resetBuffers() {
    if (!circularBuffer_.empty()) {
        std::fill(circularBuffer_.begin(), circularBuffer_.end(), 0.0f);
    }
    
    writeIndex_.store(0);
    readIndex_.store(0);
}

void RealtimeAudioBouncer::flushBuffers() {
    // Process any remaining data in circular buffer
    uint32_t available = getAvailableReadSpace();
    if (available > 0) {
        std::vector<float> flushBuffer(available);
        if (readFromBuffer(flushBuffer.data(), available / config_.channels)) {
            convertToOutputFormat(flushBuffer.data(), available / config_.channels);
        }
    }
}

uint32_t RealtimeAudioBouncer::getBufferUsage() const {
    uint32_t write = writeIndex_.load();
    uint32_t read = readIndex_.load();
    
    if (write >= read) {
        return write - read;
    } else {
        return bufferSize_ - read + write;
    }
}

bool RealtimeAudioBouncer::hasBufferUnderrun() const {
    return currentMetrics_.bufferUnderruns > 0;
}

bool RealtimeAudioBouncer::hasBufferOverrun() const {
    return currentMetrics_.bufferOverruns > 0;
}

// Integration
void RealtimeAudioBouncer::integrateWithSequencer(std::function<void(float*, uint32_t)> audioCallback) {
    sequencerCallback_ = audioCallback;
}

void RealtimeAudioBouncer::integrateWithEffectsChain(std::function<void(float*, uint32_t)> effectsCallback) {
    effectsCallback_ = effectsCallback;
}

// Performance Analysis
size_t RealtimeAudioBouncer::getEstimatedMemoryUsage() const {
    return sizeof(RealtimeAudioBouncer) + 
           circularBuffer_.size() * sizeof(float) +
           srcBuffer_.size() * sizeof(float);
}

float RealtimeAudioBouncer::getAverageCpuLoad() const {
    if (performanceUpdateCounter_ == 0) {
        return 0.0f;
    }
    return static_cast<float>(totalProcessingTime_) / performanceUpdateCounter_;
}

uint32_t RealtimeAudioBouncer::getTotalSamplesProcessed() const {
    return samplesRecorded_;
}

void RealtimeAudioBouncer::resetPerformanceCounters() {
    performanceUpdateCounter_ = 0;
    totalProcessingTime_ = 0;
    currentMetrics_ = AudioMetrics();
}

// Internal methods
void RealtimeAudioBouncer::updateStatus(BounceStatus newStatus, const std::string& message) {
    status_.store(newStatus);
    
    if (statusCallback_) {
        statusCallback_(newStatus, message);
    }
}

void RealtimeAudioBouncer::updateProgress() {
    if (progressCallback_) {
        float percentage = getProgressPercentage();
        uint32_t elapsed = getElapsedTimeMs();
        uint32_t remaining = getRemainingTimeMs();
        progressCallback_(percentage, elapsed, remaining);
    }
}

void RealtimeAudioBouncer::updateMetrics() {
    currentMetrics_.samplesProcessed = samplesRecorded_;
    currentMetrics_.currentPeakLevel = peakLevel_;
    currentMetrics_.currentRmsLevel = rmsLevel_;
    
    if (performanceUpdateCounter_ > 0) {
        currentMetrics_.cpuLoad = static_cast<float>(totalProcessingTime_) / performanceUpdateCounter_;
    }
    
    if (metricsCallback_) {
        metricsCallback_(currentMetrics_);
    }
}

// Audio processing helpers
void RealtimeAudioBouncer::processLimiter(float* buffer, uint32_t sampleCount) {
    float threshold = processingParams_.limiterThreshold;
    float releaseMs = processingParams_.limiterRelease;
    float releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * 48000.0f));  // Assume 48kHz
    
    for (uint32_t i = 0; i < sampleCount; ++i) {
        float inputLevel = std::abs(buffer[i]);
        
        if (inputLevel > threshold) {
            float reduction = threshold / inputLevel;
            limiterState_ = std::min(reduction, limiterState_);
        } else {
            limiterState_ += (1.0f - limiterState_) * (1.0f - releaseCoeff);
        }
        
        buffer[i] *= limiterState_;
    }
}

void RealtimeAudioBouncer::processHighpassFilter(float* buffer, uint32_t sampleCount) {
    // Simple DC blocking filter (first-order highpass)
    float cutoff = processingParams_.highpassFrequency;
    float sampleRate = static_cast<float>(config_.sampleRate);
    float rc = 1.0f / (2.0f * M_PI * cutoff);
    float alpha = rc / (rc + 1.0f / sampleRate);
    
    for (uint32_t i = 0; i < sampleCount; i += config_.channels) {
        for (uint8_t ch = 0; ch < config_.channels; ++ch) {
            float input = buffer[i + ch];
            float output = alpha * (highpassState_[ch] + input - buffer[i + ch]);
            highpassState_[ch] = output;
            buffer[i + ch] = output;
        }
    }
}

void RealtimeAudioBouncer::applyDithering(float* buffer, uint32_t sampleCount) {
    // Simple triangular dithering
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    
    float ditherAmount = 1.0f / 32768.0f;  // For 16-bit quantization
    
    for (uint32_t i = 0; i < sampleCount; ++i) {
        float dither = (dis(gen) + dis(gen)) * 0.5f * ditherAmount;
        buffer[i] += dither;
    }
}

void RealtimeAudioBouncer::applyNormalization(float* buffer, uint32_t sampleCount, float targetLevel) {
    float currentPeak = calculatePeakLevel(buffer, sampleCount);
    
    if (currentPeak > 0.0f) {
        float gain = targetLevel / currentPeak;
        for (uint32_t i = 0; i < sampleCount; ++i) {
            buffer[i] *= gain;
        }
    }
}

// Format conversion helpers
void RealtimeAudioBouncer::convertToInt16(const float* input, int16_t* output, uint32_t sampleCount) {
    for (uint32_t i = 0; i < sampleCount; ++i) {
        float clamped = std::max(-1.0f, std::min(1.0f, input[i]));
        output[i] = static_cast<int16_t>(clamped * 32767.0f);
    }
}

void RealtimeAudioBouncer::convertToInt24(const float* input, uint8_t* output, uint32_t sampleCount) {
    for (uint32_t i = 0; i < sampleCount; ++i) {
        float clamped = std::max(-1.0f, std::min(1.0f, input[i]));
        int32_t sample24 = static_cast<int32_t>(clamped * 8388607.0f);  // 2^23 - 1
        
        output[i * 3] = sample24 & 0xFF;
        output[i * 3 + 1] = (sample24 >> 8) & 0xFF;
        output[i * 3 + 2] = (sample24 >> 16) & 0xFF;
    }
}

void RealtimeAudioBouncer::convertToFloat32(const float* input, float* output, uint32_t sampleCount) {
    std::copy(input, input + sampleCount, output);
}

// Buffer management helpers
uint32_t RealtimeAudioBouncer::getAvailableWriteSpace() const {
    uint32_t used = getBufferUsage();
    return bufferSize_ - used - 1;  // Leave one slot to distinguish full from empty
}

uint32_t RealtimeAudioBouncer::getAvailableReadSpace() const {
    return getBufferUsage();
}

bool RealtimeAudioBouncer::writeToBuffer(const float* data, uint32_t sampleCount) {
    uint32_t totalSamples = sampleCount * config_.channels;
    if (getAvailableWriteSpace() < totalSamples) {
        currentMetrics_.bufferOverruns++;
        return false;
    }
    
    uint32_t writePos = writeIndex_.load();
    
    for (uint32_t i = 0; i < totalSamples; ++i) {
        circularBuffer_[writePos] = data[i];
        writePos = (writePos + 1) % bufferSize_;
    }
    
    writeIndex_.store(writePos);
    return true;
}

bool RealtimeAudioBouncer::readFromBuffer(float* data, uint32_t sampleCount) {
    uint32_t totalSamples = sampleCount * config_.channels;
    if (getAvailableReadSpace() < totalSamples) {
        currentMetrics_.bufferUnderruns++;
        return false;
    }
    
    uint32_t readPos = readIndex_.load();
    
    for (uint32_t i = 0; i < totalSamples; ++i) {
        data[i] = circularBuffer_[readPos];
        readPos = (readPos + 1) % bufferSize_;
    }
    
    readIndex_.store(readPos);
    return true;
}

// File format helpers
void RealtimeAudioBouncer::writeInt16LE(uint16_t value) {
    uint8_t bytes[2] = {
        static_cast<uint8_t>(value & 0xFF),
        static_cast<uint8_t>((value >> 8) & 0xFF)
    };
    fwrite(bytes, 1, 2, outputFile_);
}

void RealtimeAudioBouncer::writeInt32LE(uint32_t value) {
    uint8_t bytes[4] = {
        static_cast<uint8_t>(value & 0xFF),
        static_cast<uint8_t>((value >> 8) & 0xFF),
        static_cast<uint8_t>((value >> 16) & 0xFF),
        static_cast<uint8_t>((value >> 24) & 0xFF)
    };
    fwrite(bytes, 1, 4, outputFile_);
}

void RealtimeAudioBouncer::writeFloat32LE(float value) {
    union { float f; uint32_t i; } converter;
    converter.f = value;
    writeInt32LE(converter.i);
}

uint32_t RealtimeAudioBouncer::calculateWavDataSize(uint32_t sampleCount, uint16_t channels, uint16_t bitsPerSample) {
    return sampleCount * channels * bitsPerSample / 8;
}

uint32_t RealtimeAudioBouncer::calculateAiffDataSize(uint32_t sampleCount, uint16_t channels, uint16_t bitsPerSample) {
    return sampleCount * channels * bitsPerSample / 8;
}

// Validation helpers
bool RealtimeAudioBouncer::validateConfig(const BounceConfig& config) const {
    if (config.channels == 0 || config.channels > 2) {
        return false;
    }
    
    if (config.bufferSizeFrames < MIN_BUFFER_SIZE || config.bufferSizeFrames > MAX_BUFFER_SIZE) {
        return false;
    }
    
    if (config.normalizationLevel <= 0.0f || config.normalizationLevel > 1.0f) {
        return false;
    }
    
    return true;
}

bool RealtimeAudioBouncer::validateOutputPath(const std::string& path) const {
    if (path.empty()) {
        return false;
    }
    
    // Check if path has appropriate extension
    std::string expectedExt = getFormatExtension(config_.format);
    if (path.length() < expectedExt.length()) {
        return false;
    }
    
    std::string pathExt = path.substr(path.length() - expectedExt.length());
    std::transform(pathExt.begin(), pathExt.end(), pathExt.begin(), ::tolower);
    
    return pathExt == expectedExt;
}

void RealtimeAudioBouncer::sanitizeProcessingParams(ProcessingParams& params) const {
    params.inputGain = std::max(0.0f, std::min(params.inputGain, 10.0f));
    params.outputGain = std::max(0.0f, std::min(params.outputGain, 10.0f));
    params.limiterThreshold = std::max(0.1f, std::min(params.limiterThreshold, 1.0f));
    params.limiterRelease = std::max(1.0f, std::min(params.limiterRelease, 1000.0f));
    params.highpassFrequency = std::max(5.0f, std::min(params.highpassFrequency, 200.0f));
}

// Utility methods
uint32_t RealtimeAudioBouncer::getCurrentTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

std::string RealtimeAudioBouncer::getFormatExtension(AudioFormat format) const {
    switch (format) {
        case AudioFormat::WAV_16BIT:
        case AudioFormat::WAV_24BIT:
        case AudioFormat::WAV_32BIT_FLOAT:
            return ".wav";
        case AudioFormat::AIFF_16BIT:
        case AudioFormat::AIFF_24BIT:
            return ".aiff";
        case AudioFormat::RAW_PCM_16:
        case AudioFormat::RAW_PCM_24:
        case AudioFormat::RAW_PCM_32_FLOAT:
            return ".pcm";
        default:
            return ".wav";
    }
}

uint32_t RealtimeAudioBouncer::getBitsPerSample(AudioFormat format) const {
    switch (format) {
        case AudioFormat::WAV_16BIT:
        case AudioFormat::AIFF_16BIT:
        case AudioFormat::RAW_PCM_16:
            return 16;
        case AudioFormat::WAV_24BIT:
        case AudioFormat::AIFF_24BIT:
        case AudioFormat::RAW_PCM_24:
            return 24;
        case AudioFormat::WAV_32BIT_FLOAT:
        case AudioFormat::RAW_PCM_32_FLOAT:
            return 32;
        default:
            return 16;
    }
}

bool RealtimeAudioBouncer::isFloatFormat(AudioFormat format) const {
    return format == AudioFormat::WAV_32BIT_FLOAT || format == AudioFormat::RAW_PCM_32_FLOAT;
}