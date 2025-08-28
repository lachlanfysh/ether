#include "VelocityCaptureSystem.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <sstream>

VelocityCaptureSystem::VelocityCaptureSystem() {
    config_ = CaptureConfig();
    
    // Initialize channel configurations
    for (uint8_t i = 0; i < MAX_VELOCITY_CHANNELS; ++i) {
        channelConfigs_[i] = ChannelConfig();
        channelCalibrations_[i] = ChannelCalibration();
        channelEnabled_[i].store(false);
        channelCalibrating_[i].store(false);
        lastEventTime_[i] = 0;
        lastVelocity_[i] = 0.0f;
    }
    
    // Initialize system state
    isCapturing_.store(false);
    isPaused_.store(false);
    
    // Initialize hardware state
    adcConfigured_ = false;
    hardwareFiltersEnabled_ = false;
    hardwareInterruptsEnabled_ = false;
    
    // Initialize integration pointers
    sequencer_ = nullptr;
    sampleLoader_ = nullptr;
    midiInterface_ = nullptr;
    
    // Initialize performance tracking
    totalEventsProcessed_ = 0;
    totalProcessingTime_ = 0;
    processingStartTime_ = 0;
    
    // Reserve event history
    eventHistory_.reserve(MAX_EVENT_HISTORY);
    
    // Initialize analysis data
    currentAnalysis_ = VelocityAnalysis();
}

VelocityCaptureSystem::~VelocityCaptureSystem() {
    stopCapture();
    disableHardwareInterrupts();
}

// System Configuration
void VelocityCaptureSystem::setCaptureConfig(const CaptureConfig& config) {
    config_ = config;
    
    // Validate and sanitize configuration
    config_.sampleRateHz = std::clamp(config_.sampleRateHz, 8000u, 192000u);
    config_.adcResolution = std::clamp(config_.adcResolution, uint8_t(8), uint8_t(16));
    config_.bufferSizeFrames = std::clamp(config_.bufferSizeFrames, 64u, 8192u);
    config_.globalSensitivity = std::clamp(config_.globalSensitivity, 0.1f, 10.0f);
    config_.processingThreadPriority = std::clamp(config_.processingThreadPriority, uint8_t(1), uint8_t(99));
    config_.maxLatencyUs = std::clamp(config_.maxLatencyUs, 100u, 10000u);
    
    // Reconfigure hardware if needed
    if (adcConfigured_) {
        configureADC();
    }
}

void VelocityCaptureSystem::setChannelConfig(uint8_t channelId, const ChannelConfig& config) {
    if (!validateChannelId(channelId)) return;
    
    ChannelConfig sanitizedConfig = config;
    sanitizeChannelConfig(sanitizedConfig);
    channelConfigs_[channelId] = sanitizedConfig;
}

const VelocityCaptureSystem::ChannelConfig& VelocityCaptureSystem::getChannelConfig(uint8_t channelId) const {
    if (!validateChannelId(channelId)) {
        static ChannelConfig defaultConfig;
        return defaultConfig;
    }
    return channelConfigs_[channelId];
}

// System Control
bool VelocityCaptureSystem::startCapture() {
    if (isCapturing_.load()) {
        return true; // Already capturing
    }
    
    // Configure hardware
    if (!adcConfigured_) {
        configureADC();
    }
    
    setupHardwareFiltering();
    enableHardwareInterrupts();
    
    // Reset analysis and performance counters
    resetAnalysis();
    resetPerformanceCounters();
    
    // Start processing
    processingStartTime_ = getCurrentTimeMicroseconds();
    isCapturing_.store(true);
    isPaused_.store(false);
    
    notifySystemStatus();
    return true;
}

bool VelocityCaptureSystem::stopCapture() {
    if (!isCapturing_.load()) {
        return true; // Already stopped
    }
    
    isCapturing_.store(false);
    isPaused_.store(false);
    
    disableHardwareInterrupts();
    
    notifySystemStatus();
    return true;
}

bool VelocityCaptureSystem::pauseCapture() {
    if (!isCapturing_.load()) {
        return false;
    }
    
    isPaused_.store(true);
    notifySystemStatus();
    return true;
}

bool VelocityCaptureSystem::resumeCapture() {
    if (!isCapturing_.load()) {
        return false;
    }
    
    isPaused_.store(false);
    notifySystemStatus();
    return true;
}

// Channel Management
void VelocityCaptureSystem::enableChannel(uint8_t channelId, VelocitySourceType sourceType) {
    if (!validateChannelId(channelId)) return;
    
    channelConfigs_[channelId].sourceType = sourceType;
    channelEnabled_[channelId].store(true);
}

void VelocityCaptureSystem::disableChannel(uint8_t channelId) {
    if (!validateChannelId(channelId)) return;
    
    channelEnabled_[channelId].store(false);
    channelConfigs_[channelId].sourceType = VelocitySourceType::DISABLED;
}

bool VelocityCaptureSystem::isChannelEnabled(uint8_t channelId) const {
    if (!validateChannelId(channelId)) return false;
    return channelEnabled_[channelId].load();
}

std::vector<uint8_t> VelocityCaptureSystem::getEnabledChannels() const {
    std::vector<uint8_t> enabled;
    for (uint8_t i = 0; i < MAX_VELOCITY_CHANNELS; ++i) {
        if (channelEnabled_[i].load()) {
            enabled.push_back(i);
        }
    }
    return enabled;
}

// Velocity Processing
void VelocityCaptureSystem::processVelocityInput(uint8_t channelId, float rawValue, uint32_t timestampUs) {
    if (!validateChannelId(channelId)) return;
    if (!isCapturing_.load() || isPaused_.load()) return;
    if (!channelEnabled_[channelId].load()) return;
    
    uint32_t processingStart = getCurrentTimeMicroseconds();
    
    processRawVelocity(channelId, rawValue, timestampUs);
    
    // Update performance metrics
    uint32_t processingTime = getCurrentTimeMicroseconds() - processingStart;
    totalProcessingTime_ += processingTime;
    totalEventsProcessed_++;
    
    // Update current analysis
    if (totalEventsProcessed_ % (config_.sampleRateHz / 100) == 0) { // Update every 10ms worth of samples
        updateAnalysis(VelocityEvent()); // Trigger analysis update
    }
}

float VelocityCaptureSystem::applyVelocityCurve(float velocity, uint8_t curveType, float curveAmount) const {
    if (!validateVelocityValue(velocity)) return 0.0f;
    
    switch (curveType) {
        case 0: // Linear
            return applyLinearCurve(velocity, curveAmount);
        case 1: // Exponential
            return applyExponentialCurve(velocity, curveAmount);
        case 2: // Logarithmic
            return applyLogarithmicCurve(velocity, curveAmount);
        case 3: // Custom
            return applyCustomCurve(velocity, curveAmount);
        default:
            return velocity;
    }
}

bool VelocityCaptureSystem::detectGhostNote(uint8_t channelId, float velocity, uint32_t timestampUs) const {
    if (!validateChannelId(channelId)) return false;
    
    const ChannelConfig& config = channelConfigs_[channelId];
    if (!config.enableCrossChannelSupression) return false;
    
    // Check for cross-channel activity that might indicate a ghost note
    (void)velocity; // May be used in future for velocity-based ghost detection
    return checkCrossChannelActivity(channelId, timestampUs);
}

float VelocityCaptureSystem::calculateConfidenceLevel(uint8_t channelId, float velocity, float rawValue) const {
    if (!validateChannelId(channelId)) return 0.0f;
    
    const ChannelConfig& config = channelConfigs_[channelId];
    const ChannelCalibration& calibration = channelCalibrations_[channelId];
    
    float confidence = 1.0f;
    
    // Reduce confidence for values near noise floor
    if (rawValue < config.noiseFloor * 2.0f) {
        confidence *= 0.5f;
    }
    
    // Reduce confidence if channel isn't calibrated
    if (!calibration.isCalibrated) {
        confidence *= 0.7f;
    }
    
    // Reduce confidence for extreme velocity values
    if (velocity > 0.95f || velocity < 0.05f) {
        confidence *= 0.8f;
    }
    
    return std::clamp(confidence, 0.0f, 1.0f) * 255.0f;
}

// Calibration
void VelocityCaptureSystem::startChannelCalibration(uint8_t channelId) {
    if (!validateChannelId(channelId)) return;
    
    channelCalibrating_[channelId].store(true);
    
    // Reset calibration data
    channelCalibrations_[channelId] = ChannelCalibration();
    channelCalibrations_[channelId].lastCalibrationTime = getCurrentTimeMicroseconds();
}

void VelocityCaptureSystem::stopChannelCalibration(uint8_t channelId) {
    if (!validateChannelId(channelId)) return;
    
    channelCalibrating_[channelId].store(false);
    
    ChannelCalibration& calibration = channelCalibrations_[channelId];
    if (calibration.calibrationSamples >= CALIBRATION_SAMPLES_REQUIRED) {
        calibration.isCalibrated = true;
        
        // Calculate optimal sensitivity
        float range = calibration.maxRawValue - calibration.minRawValue;
        if (range > 0.0f) {
            calibration.optimalSensitivity = 1.0f / range;
        }
        
        notifyCalibrationComplete(channelId);
    }
}

bool VelocityCaptureSystem::isChannelCalibrating(uint8_t channelId) const {
    if (!validateChannelId(channelId)) return false;
    return channelCalibrating_[channelId].load();
}

void VelocityCaptureSystem::resetChannelCalibration(uint8_t channelId) {
    if (!validateChannelId(channelId)) return;
    
    channelCalibrations_[channelId] = ChannelCalibration();
    channelCalibrating_[channelId].store(false);
}

const VelocityCaptureSystem::ChannelCalibration& VelocityCaptureSystem::getChannelCalibration(uint8_t channelId) const {
    if (!validateChannelId(channelId)) {
        static ChannelCalibration defaultCalibration;
        return defaultCalibration;
    }
    return channelCalibrations_[channelId];
}

// Analysis and Monitoring
VelocityCaptureSystem::VelocityAnalysis VelocityCaptureSystem::getCurrentAnalysis() const {
    return currentAnalysis_;
}

float VelocityCaptureSystem::getChannelActivity(uint8_t channelId) const {
    if (!validateChannelId(channelId)) return 0.0f;
    return currentAnalysis_.channelActivity[channelId];
}

std::vector<VelocityCaptureSystem::VelocityEvent> VelocityCaptureSystem::getRecentEvents(uint32_t maxEvents) const {
    std::vector<VelocityEvent> recent;
    
    uint32_t startIndex = 0;
    if (eventHistory_.size() > maxEvents) {
        startIndex = eventHistory_.size() - maxEvents;
    }
    
    for (uint32_t i = startIndex; i < eventHistory_.size(); ++i) {
        recent.push_back(eventHistory_[i]);
    }
    
    return recent;
}

void VelocityCaptureSystem::resetAnalysis() {
    currentAnalysis_ = VelocityAnalysis();
}

// Hardware Integration
void VelocityCaptureSystem::configureADC() {
    // Configure ADC for multi-channel velocity capture
    for (uint8_t i = 0; i < MAX_VELOCITY_CHANNELS; ++i) {
        if (channelEnabled_[i].load()) {
            const ChannelConfig& config = channelConfigs_[i];
            if (config.sourceType == VelocitySourceType::HALL_EFFECT_SENSOR ||
                config.sourceType == VelocitySourceType::EXTERNAL_ANALOG) {
                configureADCChannel(config.adcChannel, config_.adcResolution);
            }
        }
    }
    
    adcConfigured_ = true;
}

void VelocityCaptureSystem::setupHardwareFiltering() {
    if (config_.enableHardwareFiltering && !hardwareFiltersEnabled_) {
        // Configure hardware anti-aliasing filters for velocity inputs
        hardwareFiltersEnabled_ = true;
    }
}

void VelocityCaptureSystem::enableHardwareInterrupts() {
    if (!hardwareInterruptsEnabled_) {
        // Enable ADC conversion complete interrupts
        hardwareInterruptsEnabled_ = true;
    }
}

void VelocityCaptureSystem::disableHardwareInterrupts() {
    if (hardwareInterruptsEnabled_) {
        // Disable ADC interrupts
        hardwareInterruptsEnabled_ = false;
    }
}

bool VelocityCaptureSystem::testHardwareConnection(uint8_t channelId) const {
    if (!validateChannelId(channelId)) return false;
    
    const ChannelConfig& config = channelConfigs_[channelId];
    return testADCChannel(config.adcChannel);
}

// External Integration
void VelocityCaptureSystem::integrateWithSequencer(SequencerEngine* sequencer) {
    sequencer_ = sequencer;
}

void VelocityCaptureSystem::integrateWithSampler(AutoSampleLoader* sampleLoader) {
    sampleLoader_ = sampleLoader;
}

void VelocityCaptureSystem::integrateWithMIDI(class MIDIInterface* midiInterface) {
    midiInterface_ = midiInterface;
}

void VelocityCaptureSystem::setExternalVelocitySource(std::function<float(uint8_t)> velocityCallback) {
    externalVelocityCallback_ = velocityCallback;
}

// Performance Analysis
size_t VelocityCaptureSystem::getEstimatedMemoryUsage() const {
    size_t baseSize = sizeof(*this);
    size_t eventHistorySize = eventHistory_.capacity() * sizeof(VelocityEvent);
    return baseSize + eventHistorySize;
}

float VelocityCaptureSystem::getAverageProcessingTime() const {
    if (totalEventsProcessed_ == 0) return 0.0f;
    return static_cast<float>(totalProcessingTime_) / totalEventsProcessed_;
}

uint32_t VelocityCaptureSystem::getTotalEventsProcessed() const {
    return totalEventsProcessed_;
}

void VelocityCaptureSystem::resetPerformanceCounters() {
    totalEventsProcessed_ = 0;
    totalProcessingTime_ = 0;
    processingStartTime_ = getCurrentTimeMicroseconds();
}

// Internal processing methods
void VelocityCaptureSystem::processRawVelocity(uint8_t channelId, float rawValue, uint32_t timestampUs) {
    const ChannelConfig& config = channelConfigs_[channelId];
    
    // Update calibration if active
    if (channelCalibrating_[channelId].load()) {
        updateChannelCalibration(channelId, rawValue);
    }
    
    // Apply channel-specific processing
    float processedValue = applyChannelProcessing(channelId, rawValue);
    
    // Apply velocity curve if enabled
    if (config.enableVelocityCurve) {
        processedValue = applyVelocityCurve(processedValue, config.velocityCurveType, config.curveAmount);
    }
    
    // Create velocity event
    VelocityEvent event;
    event.channelId = channelId;
    event.velocity = processedValue;
    event.timestampUs = timestampUs;
    event.sourceType = config.sourceType;
    event.rawValue = rawValue;
    event.processedValue = processedValue;
    event.isGhostNote = detectGhostNote(channelId, processedValue, timestampUs);
    event.confidenceLevel = static_cast<uint8_t>(calculateConfidenceLevel(channelId, processedValue, rawValue));
    
    // Update tracking
    lastEventTime_[channelId] = timestampUs;
    lastVelocity_[channelId] = processedValue;
    
    // Store in history
    if (eventHistory_.size() >= MAX_EVENT_HISTORY) {
        eventHistory_.erase(eventHistory_.begin());
    }
    eventHistory_.push_back(event);
    
    // Update analysis
    updateAnalysis(event);
    
    // Notify callback
    notifyVelocityEvent(event);
}

float VelocityCaptureSystem::applyChannelProcessing(uint8_t channelId, float rawValue) const {
    const ChannelConfig& config = channelConfigs_[channelId];
    const ChannelCalibration& calibration = channelCalibrations_[channelId];
    
    float processed = rawValue;
    
    // Apply noise gate
    if (config.enableNoiseGate && processed < config.noiseFloor) {
        return 0.0f;
    }
    
    // Apply sensitivity
    processed *= config.sensitivityMultiplier * config_.globalSensitivity;
    
    // Apply calibration if available
    if (calibration.isCalibrated) {
        processed *= calibration.optimalSensitivity;
    }
    
    // Apply adaptive threshold
    if (config.enableAdaptiveThreshold) {
        float adaptiveThreshold = calibration.noiseLevel * 1.5f;
        if (processed < adaptiveThreshold) {
            processed = 0.0f;
        }
    }
    
    // Clamp to valid range
    processed = std::clamp(processed, 0.0f, config.maxVelocity);
    
    return processed;
}

void VelocityCaptureSystem::updateChannelCalibration(uint8_t channelId, float rawValue) {
    ChannelCalibration& calibration = channelCalibrations_[channelId];
    
    // Update min/max values
    calibration.minRawValue = std::min(calibration.minRawValue, rawValue);
    calibration.maxRawValue = std::max(calibration.maxRawValue, rawValue);
    
    // Update noise level estimation
    if (rawValue < calibration.noiseLevel * 2.0f) {
        calibration.noiseLevel = calculateMovingAverage(rawValue, calibration.noiseLevel, calibration.calibrationSamples);
    }
    
    calibration.calibrationSamples++;
}

void VelocityCaptureSystem::updateAnalysis(const VelocityEvent& event) {
    if (event.channelId < MAX_VELOCITY_CHANNELS) {
        // Update channel activity
        currentAnalysis_.channelActivity[event.channelId] = 
            calculateMovingAverage(event.velocity, currentAnalysis_.channelActivity[event.channelId], 100);
        
        // Update global metrics
        currentAnalysis_.averageVelocity = 
            calculateMovingAverage(event.velocity, currentAnalysis_.averageVelocity, currentAnalysis_.eventCount + 1);
        
        currentAnalysis_.peakVelocity = std::max(currentAnalysis_.peakVelocity, event.velocity);
        currentAnalysis_.eventCount++;
    }
    
    // Update performance metrics
    if (totalEventsProcessed_ > 0) {
        currentAnalysis_.averageLatencyUs = getAverageProcessingTime();
        currentAnalysis_.cpuUsage = (static_cast<float>(totalProcessingTime_) / 
                                   (getCurrentTimeMicroseconds() - processingStartTime_)) * 100.0f;
    }
}

// Velocity curve implementations
float VelocityCaptureSystem::applyLinearCurve(float velocity, float amount) const {
    return velocity * amount;
}

float VelocityCaptureSystem::applyExponentialCurve(float velocity, float amount) const {
    return std::pow(velocity, amount);
}

float VelocityCaptureSystem::applyLogarithmicCurve(float velocity, float amount) const {
    if (velocity <= 0.0f) return 0.0f;
    return std::log(1.0f + velocity * (std::exp(amount) - 1.0f)) / amount;
}

float VelocityCaptureSystem::applyCustomCurve(float velocity, float amount) const {
    // Custom S-curve implementation
    float x = velocity * 2.0f - 1.0f; // Map to -1 to 1
    float curved = x / (1.0f + std::abs(x) * amount);
    return (curved + 1.0f) * 0.5f; // Map back to 0 to 1
}

// Ghost note detection
bool VelocityCaptureSystem::checkCrossChannelActivity(uint8_t channelId, uint32_t timestampUs) const {
    const ChannelConfig& config = channelConfigs_[channelId];
    
    for (uint8_t i = 0; i < MAX_VELOCITY_CHANNELS; ++i) {
        if (i != channelId && channelEnabled_[i].load()) {
            uint32_t timeDiff = (timestampUs > lastEventTime_[i]) ? 
                               (timestampUs - lastEventTime_[i]) : 
                               (lastEventTime_[i] - timestampUs);
            
            if (timeDiff < MAX_CROSS_CHANNEL_TIME_US && 
                lastVelocity_[i] > config.crossChannelThreshold) {
                return true; // Likely ghost note
            }
        }
    }
    
    return false;
}

float VelocityCaptureSystem::calculateChannelCorrelation(uint8_t channel1, uint8_t channel2) const {
    // Simple correlation based on timing and velocity similarity
    if (!validateChannelId(channel1) || !validateChannelId(channel2)) return 0.0f;
    if (channel1 == channel2) return 1.0f;
    
    uint32_t timeDiff = (lastEventTime_[channel1] > lastEventTime_[channel2]) ?
                       (lastEventTime_[channel1] - lastEventTime_[channel2]) :
                       (lastEventTime_[channel2] - lastEventTime_[channel1]);
    
    float velocityDiff = std::abs(lastVelocity_[channel1] - lastVelocity_[channel2]);
    
    float timeCorrelation = 1.0f - std::min(1.0f, timeDiff / MAX_CROSS_CHANNEL_TIME_US);
    float velocityCorrelation = 1.0f - velocityDiff;
    
    return (timeCorrelation + velocityCorrelation) * 0.5f;
}

// Hardware interface
float VelocityCaptureSystem::readADCChannel(uint8_t adcChannel) const {
    // Platform-specific ADC read implementation
    // Return normalized value 0.0 to 1.0
    (void)adcChannel; // Suppress unused parameter warning
    return 0.0f; // Placeholder
}

void VelocityCaptureSystem::configureADCChannel(uint8_t adcChannel, uint8_t resolution) const {
    // Platform-specific ADC configuration
    (void)adcChannel;
    (void)resolution;
}

bool VelocityCaptureSystem::testADCChannel(uint8_t adcChannel) const {
    // Test ADC channel functionality
    (void)adcChannel;
    return true; // Placeholder
}

// Validation helpers
bool VelocityCaptureSystem::validateChannelId(uint8_t channelId) const {
    return channelId < MAX_VELOCITY_CHANNELS;
}

bool VelocityCaptureSystem::validateVelocityValue(float velocity) const {
    return velocity >= 0.0f && velocity <= 1.0f && !std::isnan(velocity) && !std::isinf(velocity);
}

void VelocityCaptureSystem::sanitizeChannelConfig(ChannelConfig& config) const {
    config.sensitivityMultiplier = std::clamp(config.sensitivityMultiplier, 0.1f, 10.0f);
    config.noiseFloor = std::clamp(config.noiseFloor, 0.0f, 1.0f);
    config.maxVelocity = std::clamp(config.maxVelocity, 0.0f, 1.0f);
    config.debounceTimeUs = std::clamp(config.debounceTimeUs, 100u, 10000u);
    config.curveAmount = std::clamp(config.curveAmount, 0.1f, 5.0f);
    config.crossChannelThreshold = std::clamp(config.crossChannelThreshold, 0.0f, 1.0f);
    
    if (config.velocityCurveType > 3) {
        config.velocityCurveType = 0; // Default to linear
    }
}

// Notification helpers
void VelocityCaptureSystem::notifyVelocityEvent(const VelocityEvent& event) {
    if (velocityEventCallback_) {
        velocityEventCallback_(event);
    }
}

void VelocityCaptureSystem::notifyCalibrationComplete(uint8_t channelId) {
    if (calibrationCompleteCallback_) {
        calibrationCompleteCallback_(channelId, channelCalibrations_[channelId]);
    }
}

void VelocityCaptureSystem::notifySystemStatus() {
    if (systemStatusCallback_) {
        systemStatusCallback_(isCapturing_.load(), currentAnalysis_);
    }
}

void VelocityCaptureSystem::notifyError(const std::string& error) {
    if (errorCallback_) {
        errorCallback_(error);
    }
}

// Utility methods
uint32_t VelocityCaptureSystem::getCurrentTimeMicroseconds() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(duration).count());
}

float VelocityCaptureSystem::calculateMovingAverage(float newValue, float oldAverage, uint32_t sampleCount) const {
    if (sampleCount <= 1) return newValue;
    float alpha = 1.0f / sampleCount;
    return oldAverage * (1.0f - alpha) + newValue * alpha;
}