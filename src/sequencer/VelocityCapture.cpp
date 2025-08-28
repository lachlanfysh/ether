#include "VelocityCapture.h"
#include <algorithm>
#include <cmath>
#include <numeric>

VelocityCapture::VelocityCapture() {
    config_ = CaptureConfig();
    isCapturing_ = false;
    previewCallback_ = nullptr;
    
    hallEffectVelocity_ = 0.0f;
    smartKnobVelocity_ = 0.0f;
    touchPressure_ = 0.0f;
    midiVelocity_ = 100;  // Default MIDI velocity
    audioLevel_ = 0.0f;
    manualVelocity_ = 0.7f;  // Default manual velocity (~90)
    stepRepeatRate_ = 0.0f;
    
    touchActive_ = false;
    lastCapturedVelocity_ = 100;
    lastCapturedSource_ = VelocitySource::NONE;
    smoothedVelocity_ = 0.7f;
    
    auto now = std::chrono::steady_clock::now();
    lastHallEffectTime_ = now;
    lastSmartKnobTime_ = now;
    lastTouchTime_ = now;
    lastMidiTime_ = now;
    lastAudioTime_ = now;
    lastManualTime_ = now;
}

// Configuration
void VelocityCapture::setConfig(const CaptureConfig& config) {
    config_ = config;
    
    // Validate configuration
    config_.sensitivityScale = clamp(config_.sensitivityScale, MIN_SENSITIVITY, MAX_SENSITIVITY);
    config_.velocityCurve = clamp(config_.velocityCurve, 0.1f, 4.0f);
    config_.minVelocity = clampVelocity(config_.minVelocity);
    config_.maxVelocity = clampVelocity(config_.maxVelocity);
    config_.smoothingAmount = clamp(config_.smoothingAmount, 0.0f, 1.0f);
    config_.historyLength = std::max(1, std::min(config_.historyLength, MAX_HISTORY_LENGTH));
    
    // Ensure min <= max
    if (config_.minVelocity > config_.maxVelocity) {
        std::swap(config_.minVelocity, config_.maxVelocity);
    }
}

void VelocityCapture::setPrimarySource(VelocitySource source) {
    config_.primarySource = source;
}

void VelocityCapture::setSecondarySource(VelocitySource source) {
    config_.secondarySource = source;
}

void VelocityCapture::setSensitivity(float scale) {
    config_.sensitivityScale = clamp(scale, MIN_SENSITIVITY, MAX_SENSITIVITY);
}

void VelocityCapture::setVelocityCurve(float curve) {
    config_.velocityCurve = clamp(curve, 0.1f, 4.0f);
}

void VelocityCapture::setVelocityRange(uint8_t minVel, uint8_t maxVel) {
    config_.minVelocity = clampVelocity(minVel);
    config_.maxVelocity = clampVelocity(maxVel);
    
    if (config_.minVelocity > config_.maxVelocity) {
        std::swap(config_.minVelocity, config_.maxVelocity);
    }
}

// Velocity input from various sources
void VelocityCapture::updateHallEffectVelocity(float keyVelocity) {
    // Hall effect sensors provide direct velocity measurement from key strike speed
    hallEffectVelocity_ = clamp(keyVelocity, 0.0f, 1.0f) * config_.sensitivityScale;
    lastHallEffectTime_ = std::chrono::steady_clock::now();
    
    if (config_.enablePreview && previewCallback_) {
        uint8_t previewVel = processVelocityInput(hallEffectVelocity_, VelocitySource::HALL_EFFECT_KEYS);
        triggerPreview(previewVel, VelocitySource::HALL_EFFECT_KEYS);
    }
}

void VelocityCapture::updateSmartKnobVelocity(float rotationSpeed) {
    // Convert rotation speed to velocity (0-10 rad/s maps to 0-1)
    float normalizedSpeed = clamp(std::abs(rotationSpeed) / 10.0f, 0.0f, 1.0f);
    smartKnobVelocity_ = normalizedSpeed * config_.sensitivityScale;
    lastSmartKnobTime_ = std::chrono::steady_clock::now();
    
    if (config_.enablePreview && previewCallback_) {
        uint8_t previewVel = processVelocityInput(smartKnobVelocity_, VelocitySource::SMARTKNOB_TURN);
        triggerPreview(previewVel, VelocitySource::SMARTKNOB_TURN);
    }
}

void VelocityCapture::updateTouchPressure(float pressure, bool touching) {
    touchPressure_ = clamp(pressure, 0.0f, 1.0f) * config_.sensitivityScale;
    touchActive_ = touching;
    
    if (touching) {
        lastTouchTime_ = std::chrono::steady_clock::now();
        
        if (config_.enablePreview && previewCallback_) {
            uint8_t previewVel = processVelocityInput(touchPressure_, VelocitySource::TOUCH_PRESSURE);
            triggerPreview(previewVel, VelocitySource::TOUCH_PRESSURE);
        }
    }
}

void VelocityCapture::updateMidiVelocity(uint8_t velocity) {
    midiVelocity_ = clampVelocity(velocity);
    lastMidiTime_ = std::chrono::steady_clock::now();
    
    if (config_.enablePreview && previewCallback_) {
        triggerPreview(midiVelocity_, VelocitySource::MIDI_INPUT);
    }
}

void VelocityCapture::updateAudioLevel(float level) {
    audioLevel_ = clamp(level, 0.0f, 1.0f) * config_.sensitivityScale;
    lastAudioTime_ = std::chrono::steady_clock::now();
    
    if (config_.enablePreview && previewCallback_) {
        uint8_t previewVel = processVelocityInput(audioLevel_, VelocitySource::AUDIO_INPUT);
        triggerPreview(previewVel, VelocitySource::AUDIO_INPUT);
    }
}

void VelocityCapture::updateManualVelocity(float normalizedVelocity) {
    manualVelocity_ = clamp(normalizedVelocity, 0.0f, 1.0f);
    lastManualTime_ = std::chrono::steady_clock::now();
    
    if (config_.enablePreview && previewCallback_) {
        uint8_t previewVel = processVelocityInput(manualVelocity_, VelocitySource::MANUAL_ADJUST);
        triggerPreview(previewVel, VelocitySource::MANUAL_ADJUST);
    }
}

void VelocityCapture::updateStepRepeatTiming(float repeatRate) {
    // Convert repeat rate (Hz) to velocity (faster = higher velocity)
    float normalizedRate = clamp(repeatRate / 20.0f, 0.0f, 1.0f); // 0-20Hz range
    stepRepeatRate_ = normalizedRate * config_.sensitivityScale;
    
    if (config_.enablePreview && previewCallback_) {
        uint8_t previewVel = processVelocityInput(stepRepeatRate_, VelocitySource::STEP_REPEAT);
        triggerPreview(previewVel, VelocitySource::STEP_REPEAT);
    }
}

// Velocity capture and programming
uint8_t VelocityCapture::captureVelocity() {
    VelocitySource activeSource = selectActiveSource();
    return captureVelocityFromSource(activeSource);
}

uint8_t VelocityCapture::captureVelocityFromSource(VelocitySource source) {
    uint8_t velocity;
    
    if (source == VelocitySource::MIDI_INPUT) {
        // MIDI input is already in velocity range
        velocity = midiVelocity_;
    } else {
        // Process other sources through curve and scaling
        float rawValue = getSourceRawValue(source);
        velocity = processVelocityInput(rawValue, source);
    }
    
    // Add to history
    VelocityCaptureEvent event;
    event.velocity = velocity;
    event.source = source;
    event.rawValue = getSourceRawValue(source);
    event.scaledValue = static_cast<float>(velocity) / 127.0f;
    event.timestamp = std::chrono::steady_clock::now();
    
    addToHistory(event);
    
    lastCapturedVelocity_ = velocity;
    lastCapturedSource_ = source;
    
    // Update smoothing filter
    updateSmoothingFilter(static_cast<float>(velocity) / 127.0f);
    
    return velocity;
}

void VelocityCapture::startVelocityCapture() {
    isCapturing_ = true;
}

void VelocityCapture::stopVelocityCapture() {
    isCapturing_ = false;
}

// Velocity preview and feedback
void VelocityCapture::setPreviewCallback(VelocityPreviewCallback callback) {
    previewCallback_ = callback;
}

void VelocityCapture::enablePreview(bool enabled) {
    config_.enablePreview = enabled;
}

void VelocityCapture::triggerPreview(uint8_t velocity, VelocitySource source) {
    if (previewCallback_) {
        previewCallback_(velocity, source);
    }
}

// Velocity history and analysis
uint8_t VelocityCapture::getLastVelocity() const {
    return lastCapturedVelocity_;
}

uint8_t VelocityCapture::getAverageVelocity(int samples) const {
    if (velocityHistory_.empty()) {
        return lastCapturedVelocity_;
    }
    
    int sampleCount = (samples == 0) ? config_.historyLength : samples;
    sampleCount = std::min(sampleCount, static_cast<int>(velocityHistory_.size()));
    
    if (sampleCount <= 0) {
        return lastCapturedVelocity_;
    }
    
    // Calculate average from most recent samples
    int sum = 0;
    auto it = velocityHistory_.rbegin();
    for (int i = 0; i < sampleCount && it != velocityHistory_.rend(); ++i, ++it) {
        sum += it->velocity;
    }
    
    return static_cast<uint8_t>(sum / sampleCount);
}

VelocityCapture::VelocitySource VelocityCapture::getLastVelocitySource() const {
    return lastCapturedSource_;
}

const std::vector<VelocityCapture::VelocityCaptureEvent>& VelocityCapture::getVelocityHistory() const {
    return velocityHistory_;
}

void VelocityCapture::clearVelocityHistory() {
    velocityHistory_.clear();
}

// Real-time velocity monitoring
uint8_t VelocityCapture::getCurrentVelocity() const {
    VelocitySource activeSource = selectActiveSource();
    
    if (activeSource == VelocitySource::MIDI_INPUT) {
        return midiVelocity_;
    } else if (activeSource == VelocitySource::NONE) {
        return lastCapturedVelocity_;
    } else {
        float rawValue = getSourceRawValue(activeSource);
        // Cast away const for temporary processing
        return const_cast<VelocityCapture*>(this)->processVelocityInput(rawValue, activeSource);
    }
}

VelocityCapture::VelocitySource VelocityCapture::getActiveSource() const {
    return selectActiveSource();
}

float VelocityCapture::getSourceValue(VelocitySource source) const {
    return getSourceRawValue(source);
}

bool VelocityCapture::isSourceActive(VelocitySource source) const {
    return isSourceRecentlyActive(source);
}

// Static velocity curve utilities
float VelocityCapture::applyCurve(float input, float curve) {
    if (curve == 1.0f) {
        return input; // Linear
    } else if (curve < 1.0f) {
        // Exponential (more sensitivity at low end)
        return std::pow(input, curve);
    } else {
        // Logarithmic (more sensitivity at high end)
        return 1.0f - std::pow(1.0f - input, curve);
    }
}

float VelocityCapture::linearToExponential(float input, float strength) {
    return std::pow(input, strength);
}

float VelocityCapture::linearToLogarithmic(float input, float strength) {
    return 1.0f - std::pow(1.0f - input, strength);
}

uint8_t VelocityCapture::scaleToVelocityRange(float normalized, uint8_t minVel, uint8_t maxVel) {
    float scaled = minVel + normalized * (maxVel - minVel);
    return static_cast<uint8_t>(std::round(std::max(1.0f, std::min(127.0f, scaled))));
}

// Reset and calibration
void VelocityCapture::reset() {
    isCapturing_ = false;
    clearVelocityHistory();
    lastCapturedVelocity_ = 100;
    lastCapturedSource_ = VelocitySource::NONE;
    smoothedVelocity_ = 0.7f;
    
    // Reset source values
    hallEffectVelocity_ = 0.0f;
    smartKnobVelocity_ = 0.0f;
    touchPressure_ = 0.0f;
    audioLevel_ = 0.0f;
    stepRepeatRate_ = 0.0f;
    touchActive_ = false;
}

void VelocityCapture::calibrateSources() {
    // Reset all source calibration
    auto now = std::chrono::steady_clock::now();
    lastHallEffectTime_ = now - std::chrono::seconds(1);  // Mark as inactive
    lastSmartKnobTime_ = now - std::chrono::seconds(1);   // Mark as inactive
    lastTouchTime_ = now - std::chrono::seconds(1);
    lastAudioTime_ = now - std::chrono::seconds(1);
    
    // Keep MIDI and manual sources active
    lastMidiTime_ = now;
    lastManualTime_ = now;
}

void VelocityCapture::calibrateSource(VelocitySource source) {
    auto inactiveTime = std::chrono::steady_clock::now() - std::chrono::seconds(1);
    
    switch (source) {
        case VelocitySource::HALL_EFFECT_KEYS:
            lastHallEffectTime_ = inactiveTime;
            hallEffectVelocity_ = 0.0f;
            break;
        case VelocitySource::SMARTKNOB_TURN:
            lastSmartKnobTime_ = inactiveTime;
            smartKnobVelocity_ = 0.0f;
            break;
        case VelocitySource::TOUCH_PRESSURE:
            lastTouchTime_ = inactiveTime;
            touchPressure_ = 0.0f;
            touchActive_ = false;
            break;
        case VelocitySource::AUDIO_INPUT:
            lastAudioTime_ = inactiveTime;
            audioLevel_ = 0.0f;
            break;
        case VelocitySource::STEP_REPEAT:
            stepRepeatRate_ = 0.0f;
            break;
        default:
            break;
    }
}

// Private methods
uint8_t VelocityCapture::processVelocityInput(float rawValue, VelocitySource source) {
    // Apply velocity curve
    float curved = applyCurve(rawValue, config_.velocityCurve);
    
    // Apply smoothing if enabled
    if (config_.smoothingAmount > 0.0f) {
        updateSmoothingFilter(curved);
        curved = smoothedVelocity_;
    }
    
    // Scale to velocity range
    return scaleToVelocityRange(curved, config_.minVelocity, config_.maxVelocity);
}

float VelocityCapture::getSourceRawValue(VelocitySource source) const {
    switch (source) {
        case VelocitySource::HALL_EFFECT_KEYS:
            return hallEffectVelocity_;
        case VelocitySource::SMARTKNOB_TURN:
            return smartKnobVelocity_;
        case VelocitySource::TOUCH_PRESSURE:
            return touchActive_ ? touchPressure_ : 0.0f;
        case VelocitySource::MIDI_INPUT:
            return static_cast<float>(midiVelocity_) / 127.0f;
        case VelocitySource::AUDIO_INPUT:
            return audioLevel_;
        case VelocitySource::MANUAL_ADJUST:
            return manualVelocity_;
        case VelocitySource::STEP_REPEAT:
            return stepRepeatRate_;
        default:
            return 0.0f;
    }
}

bool VelocityCapture::isSourceRecentlyActive(VelocitySource source, std::chrono::milliseconds timeoutMs) const {
    auto now = std::chrono::steady_clock::now();
    
    switch (source) {
        case VelocitySource::HALL_EFFECT_KEYS:
            return (now - lastHallEffectTime_) < timeoutMs && hallEffectVelocity_ > 0.01f;
        case VelocitySource::SMARTKNOB_TURN:
            return (now - lastSmartKnobTime_) < timeoutMs && smartKnobVelocity_ > 0.01f;
        case VelocitySource::TOUCH_PRESSURE:
            return touchActive_ && (now - lastTouchTime_) < timeoutMs;
        case VelocitySource::MIDI_INPUT:
            return (now - lastMidiTime_) < timeoutMs;
        case VelocitySource::AUDIO_INPUT:
            return (now - lastAudioTime_) < timeoutMs && audioLevel_ > 0.01f;
        case VelocitySource::MANUAL_ADJUST:
            return (now - lastManualTime_) < timeoutMs;
        case VelocitySource::STEP_REPEAT:
            return stepRepeatRate_ > 0.01f;
        default:
            return false;
    }
}

VelocityCapture::VelocitySource VelocityCapture::selectActiveSource() const {
    // Check primary source first
    if (isSourceRecentlyActive(config_.primarySource)) {
        return config_.primarySource;
    }
    
    // Check secondary source
    if (isSourceRecentlyActive(config_.secondarySource)) {
        return config_.secondarySource;
    }
    
    // Check all other sources in priority order
    std::vector<VelocitySource> priorityOrder = {
        VelocitySource::HALL_EFFECT_KEYS,  // Highest priority (physical keys)
        VelocitySource::MIDI_INPUT,        // External input
        VelocitySource::TOUCH_PRESSURE,    // Touch interaction
        VelocitySource::SMARTKNOB_TURN,    // Knob interaction
        VelocitySource::AUDIO_INPUT,       // Audio following
        VelocitySource::STEP_REPEAT,       // Repeat timing
        VelocitySource::MANUAL_ADJUST      // Manual control (always available)
    };
    
    for (VelocitySource source : priorityOrder) {
        if (isSourceRecentlyActive(source)) {
            return source;
        }
    }
    
    return VelocitySource::MANUAL_ADJUST; // Fallback to manual
}

void VelocityCapture::addToHistory(const VelocityCaptureEvent& event) {
    velocityHistory_.push_back(event);
    
    // Limit history length
    if (velocityHistory_.size() > static_cast<size_t>(config_.historyLength)) {
        velocityHistory_.erase(velocityHistory_.begin());
    }
}

void VelocityCapture::updateSmoothingFilter(float targetVelocity) {
    float alpha = 1.0f - config_.smoothingAmount;
    smoothedVelocity_ = alpha * targetVelocity + config_.smoothingAmount * smoothedVelocity_;
}

// Utility functions
float VelocityCapture::clamp(float value, float min, float max) const {
    return std::max(min, std::min(max, value));
}

uint8_t VelocityCapture::clampVelocity(uint8_t velocity) const {
    return std::max(MIN_VELOCITY_VALUE, std::min(velocity, MAX_VELOCITY_VALUE));
}