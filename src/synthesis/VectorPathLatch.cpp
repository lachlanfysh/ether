#include "VectorPathLatch.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <unordered_map>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef STM32H7
#include "stm32h7xx_hal.h"
#else
#include <chrono>
#endif

VectorPathLatch::VectorPathLatch() {
    // Initialize default configuration
    config_.playbackMode = PlaybackMode::FORWARD;
    config_.syncMode = SyncMode::BEAT_SYNC;
    config_.beatDivision = BeatDivision::QUARTER_NOTE;
    config_.baseRate = 1.0f;
    config_.swingAmount = 0.0f;
    config_.grooveShift = 0.0f;
    config_.loopStart = 0.0f;
    config_.loopEnd = 1.0f;
    config_.crossfadeTime = 0.05f;
    config_.enableRateModulation = false;
    config_.rateModDepth = 0.5f;
    config_.rateModFreq = 1.0f;
    config_.quantizeStart = true;
    config_.quantizeStop = false;
    config_.smoothing = 0.1f;
    config_.randomSeed = 12345;
    
    // Initialize tempo info
    tempoInfo_.bpm = 120.0f;
    tempoInfo_.beatsPerBar = 4;
    tempoInfo_.beatDivision = 4;
    tempoInfo_.currentBeat = 0.0f;
    tempoInfo_.isPlaying = false;
    
    // Initialize recording state
    recording_.mode = RecordMode::OFF;
    recording_.armed = false;
    recording_.recording = false;
    recording_.punchInTime = 0.0f;
    recording_.punchOutTime = 1.0f;
    recording_.recordingStart = 0.0f;
    recording_.lastRecordTime = 0.0f;
    recording_.minWaypointDistance = 0.01f;
    
    // Initialize playback state
    latched_ = false;
    paused_ = false;
    currentPosition_ = 0.0f;
    lastPosition_ = 0.0f;
    playbackDirection_ = 1.0f;
    effectiveRate_ = 1.0f;
    latchStartTime_ = 0;
    
    // Initialize modulation state
    rateModPhase_ = 0.0f;
    rateModValue_ = 0.0f;
    
    // Initialize random state
    randomState_ = config_.randomSeed;
    randomIndex_ = 0;
    generateRandomSequence();
    
    // Initialize crossfade state
    crossfadePosition_ = 0.0f;
    inCrossfade_ = false;
}

VectorPathLatch::~VectorPathLatch() {
    shutdown();
}

bool VectorPathLatch::initialize(VectorPath* vectorPath) {
    if (initialized_) {
        return true;
    }
    
    vectorPath_ = vectorPath;
    
    if (!vectorPath_) {
        return false;
    }
    
    initialized_ = true;
    return true;
}

void VectorPathLatch::shutdown() {
    if (!initialized_) {
        return;
    }
    
    stopLatch();
    vectorPath_ = nullptr;
    initialized_ = false;
}

void VectorPathLatch::setLatchConfig(const LatchConfig& config) {
    config_ = config;
    
    // Update random seed if it changed
    if (randomState_ != static_cast<uint32_t>(config_.randomSeed)) {
        randomState_ = config_.randomSeed;
        generateRandomSequence();
    }
    
    // Clamp values to valid ranges
    config_.swingAmount = std::clamp(config_.swingAmount, 0.0f, 1.0f);
    config_.grooveShift = std::clamp(config_.grooveShift, -0.5f, 0.5f);
    config_.loopStart = std::clamp(config_.loopStart, 0.0f, 1.0f);
    config_.loopEnd = std::clamp(config_.loopEnd, 0.0f, 1.0f);
    config_.crossfadeTime = std::clamp(config_.crossfadeTime, CROSSFADE_MIN_TIME, CROSSFADE_MAX_TIME);
    config_.rateModDepth = std::clamp(config_.rateModDepth, 0.0f, 2.0f);
    config_.rateModFreq = std::clamp(config_.rateModFreq, 0.1f, RATE_MOD_MAX_FREQ);
    config_.smoothing = std::clamp(config_.smoothing, 0.0f, 1.0f);
    
    // Ensure loop start is before loop end
    if (config_.loopStart >= config_.loopEnd) {
        config_.loopEnd = std::min(config_.loopStart + 0.1f, 1.0f);
    }
}

void VectorPathLatch::setTempoInfo(const TempoInfo& tempo) {
    tempoInfo_ = tempo;
    
    // Clamp values to reasonable ranges
    tempoInfo_.bpm = std::clamp(tempoInfo_.bpm, 20.0f, 300.0f);
    tempoInfo_.beatsPerBar = std::clamp(tempoInfo_.beatsPerBar, 1, 16);
    tempoInfo_.beatDivision = std::clamp(tempoInfo_.beatDivision, 1, 32);
}

void VectorPathLatch::startLatch() {
    if (!initialized_ || !vectorPath_) {
        return;
    }
    
    if (config_.quantizeStart && tempoInfo_.isPlaying) {
        // Wait for next quantized beat
        waitingForQuantizedStart_ = true;
        nextQuantizedStart_ = calculateNextQuantizedBeat(tempoInfo_.currentBeat, config_.beatDivision);
    } else {
        // Start immediately
        latched_ = true;
        paused_ = false;
        latchStartTime_ = getTimeMs();
        
        // Reset position based on playback mode
        if (config_.playbackMode == PlaybackMode::REVERSE) {
            currentPosition_ = config_.loopEnd;
            playbackDirection_ = -1.0f;
        } else {
            currentPosition_ = config_.loopStart;
            playbackDirection_ = 1.0f;
        }
        
        lastPosition_ = currentPosition_;
        
        if (playbackCallback_) {
            playbackCallback_(currentPosition_, false);
        }
    }
}

void VectorPathLatch::stopLatch() {
    if (config_.quantizeStop && tempoInfo_.isPlaying && latched_) {
        // Wait for next quantized beat
        waitingForQuantizedStop_ = true;
    } else {
        // Stop immediately
        latched_ = false;
        paused_ = false;
        waitingForQuantizedStart_ = false;
        waitingForQuantizedStop_ = false;
        inCrossfade_ = false;
        
        if (playbackCallback_) {
            playbackCallback_(currentPosition_, false);
        }
    }
}

void VectorPathLatch::pauseLatch() {
    if (latched_) {
        paused_ = !paused_;
    }
}

void VectorPathLatch::syncToTransport(float beat, bool playing) {
    tempoInfo_.currentBeat = beat;
    tempoInfo_.isPlaying = playing;
    
    // Handle quantized start/stop
    if (waitingForQuantizedStart_ && shouldStartPlaybook(beat)) {
        waitingForQuantizedStart_ = false;
        startLatch();
    }
    
    if (waitingForQuantizedStop_ && shouldStopPlaybook(beat)) {
        waitingForQuantizedStop_ = false;
        stopLatch();
    }
    
    if (beatSyncCallback_) {
        beatSyncCallback_(beat, config_.beatDivision);
    }
}

void VectorPathLatch::setLoopPoints(float start, float end) {
    config_.loopStart = std::clamp(start, 0.0f, 1.0f);
    config_.loopEnd = std::clamp(end, 0.0f, 1.0f);
    
    // Ensure start is before end
    if (config_.loopStart >= config_.loopEnd) {
        config_.loopEnd = std::min(config_.loopStart + 0.1f, 1.0f);
    }
}

void VectorPathLatch::getLoopPoints(float& start, float& end) const {
    start = config_.loopStart;
    end = config_.loopEnd;
}

void VectorPathLatch::nudgeLoopStart(float delta) {
    float newStart = config_.loopStart + delta;
    setLoopPoints(newStart, config_.loopEnd);
}

void VectorPathLatch::nudgeLoopEnd(float delta) {
    float newEnd = config_.loopEnd + delta;
    setLoopPoints(config_.loopStart, newEnd);
}

void VectorPathLatch::resetLoopPoints() {
    setLoopPoints(0.0f, 1.0f);
}

void VectorPathLatch::setPlaybackRate(float rate) {
    config_.baseRate = std::clamp(rate, 0.1f, 10.0f);
}

void VectorPathLatch::setRateModulation(bool enabled, float depth, float frequency) {
    config_.enableRateModulation = enabled;
    config_.rateModDepth = std::clamp(depth, 0.0f, 2.0f);
    config_.rateModFreq = std::clamp(frequency, 0.1f, RATE_MOD_MAX_FREQ);
}

void VectorPathLatch::setPlaybackMode(PlaybackMode mode) {
    config_.playbackMode = mode;
    
    // Reset state for new mode
    if (mode == PlaybackMode::RANDOM) {
        generateRandomSequence();
        randomIndex_ = 0;
    } else if (mode == PlaybackMode::PENDULUM) {
        rateModPhase_ = 0.0f;
    }
}

void VectorPathLatch::setSyncMode(SyncMode mode) {
    config_.syncMode = mode;
}

void VectorPathLatch::setBeatDivision(BeatDivision division) {
    config_.beatDivision = division;
}

void VectorPathLatch::setRecordMode(RecordMode mode) {
    recording_.mode = mode;
    
    if (mode == RecordMode::OFF) {
        stopRecording();
    }
}

void VectorPathLatch::armRecording(bool armed) {
    recording_.armed = armed;
    
    if (recordingCallback_) {
        recordingCallback_(recording_.mode, recording_.recording);
    }
}

void VectorPathLatch::startRecording() {
    if (!recording_.armed || recording_.mode == RecordMode::OFF) {
        return;
    }
    
    recording_.recording = true;
    recording_.recordingStart = getTimeMs() * 0.001f;
    recording_.lastRecordTime = 0.0f;
    
    if (recording_.mode == RecordMode::REPLACE) {
        clearRecordBuffer();
    }
    
    if (recordingCallback_) {
        recordingCallback_(recording_.mode, true);
    }
}

void VectorPathLatch::stopRecording() {
    if (!recording_.recording) {
        return;
    }
    
    recording_.recording = false;
    
    if (recordingCallback_) {
        recordingCallback_(recording_.mode, false);
    }
}

void VectorPathLatch::commitRecording() {
    if (recording_.recordBuffer.empty() || !vectorPath_) {
        return;
    }
    
    if (recording_.mode == RecordMode::REPLACE) {
        vectorPath_->clearWaypoints();
    }
    
    // Add recorded waypoints to path
    for (const auto& waypoint : recording_.recordBuffer) {
        vectorPath_->addWaypoint(waypoint);
    }
    
    clearRecordBuffer();
}

void VectorPathLatch::discardRecording() {
    clearRecordBuffer();
}

void VectorPathLatch::setPunchPoints(float punchIn, float punchOut) {
    recording_.punchInTime = std::clamp(punchIn, 0.0f, 1.0f);
    recording_.punchOutTime = std::clamp(punchOut, 0.0f, 1.0f);
    
    // Ensure punch-in is before punch-out
    if (recording_.punchInTime >= recording_.punchOutTime) {
        recording_.punchOutTime = std::min(recording_.punchInTime + 0.1f, 1.0f);
    }
}

void VectorPathLatch::clearRecordBuffer() {
    recording_.recordBuffer.clear();
}

void VectorPathLatch::setPosition(float position) {
    currentPosition_ = std::clamp(position, 0.0f, 1.0f);
    
    if (vectorPath_) {
        VectorPath::Position pathPos = vectorPath_->interpolatePosition(currentPosition_);
        vectorPath_->setPosition(pathPos);
    }
}

void VectorPathLatch::jumpToPosition(float position, bool quantized) {
    float targetPosition = std::clamp(position, 0.0f, 1.0f);
    
    if (quantized && config_.syncMode != SyncMode::FREE_RUNNING) {
        // Quantize to beat boundaries
        float beatDivisionValue = getBeatDivisionValue(config_.beatDivision);
        float quantizedBeat = std::round(tempoInfo_.currentBeat / beatDivisionValue) * beatDivisionValue;
        // For now, just use the target position (beat quantization would need more context)
    }
    
    setPosition(targetPosition);
}

void VectorPathLatch::update(float deltaTimeMs) {
    if (!initialized_) {
        return;
    }
    
    updateBeatSync(deltaTimeMs);
    updateRateModulation(deltaTimeMs);
    updateRecording(deltaTimeMs);
    
    if (latched_ && !paused_) {
        updatePlaybackPosition(deltaTimeMs);
    }
    
    updateCrossfade(deltaTimeMs);
}

// Private method implementations

void VectorPathLatch::updatePlaybackPosition(float deltaTimeMs) {
    if (!vectorPath_) {
        return;
    }
    
    float deltaTime = deltaTimeMs * 0.001f; // Convert to seconds
    
    // Calculate effective rate
    effectiveRate_ = config_.baseRate;
    
    if (config_.enableRateModulation) {
        effectiveRate_ *= (1.0f + rateModValue_ * config_.rateModDepth);
    }
    
    // Calculate position delta based on sync mode
    float deltaPosition = 0.0f;
    
    switch (config_.syncMode) {
        case SyncMode::FREE_RUNNING:
            deltaPosition = deltaTime * effectiveRate_;
            break;
            
        case SyncMode::BEAT_SYNC:
            {
                float beatDivisionTime = calculateBeatDivisionTime(config_.beatDivision);
                if (beatDivisionTime > 0.0f) {
                    float loopDuration = config_.loopEnd - config_.loopStart;
                    deltaPosition = (deltaTime / beatDivisionTime) * loopDuration * effectiveRate_;
                }
            }
            break;
            
        case SyncMode::BAR_SYNC:
            {
                float barTime = (60.0f / tempoInfo_.bpm) * tempoInfo_.beatsPerBar;
                if (barTime > 0.0f) {
                    float loopDuration = config_.loopEnd - config_.loopStart;
                    deltaPosition = (deltaTime / barTime) * loopDuration * effectiveRate_;
                }
            }
            break;
            
        case SyncMode::PATTERN_SYNC:
            // Would need pattern length information
            deltaPosition = deltaTime * effectiveRate_;
            break;
    }
    
    // Apply swing timing
    if (config_.swingAmount > 0.0f) {
        deltaPosition = applySwing(currentPosition_, config_.swingAmount) * deltaPosition;
    }
    
    // Update based on playback mode
    switch (config_.playbackMode) {
        case PlaybackMode::FORWARD:
            updateForwardPlayback(deltaPosition);
            break;
            
        case PlaybackMode::REVERSE:
            updateReversePlayback(deltaPosition);
            break;
            
        case PlaybackMode::PING_PONG:
            updatePingPongPlayback(deltaPosition);
            break;
            
        case PlaybackMode::RANDOM:
            updateRandomPlayback(deltaPosition);
            break;
            
        case PlaybackMode::PENDULUM:
            updatePendulumPlayback(deltaTimeMs);
            break;
            
        case PlaybackMode::FREEZE:
            updateFreezeMode();
            break;
    }
    
    // Apply smoothing
    if (config_.smoothing > 0.0f) {
        currentPosition_ = lerp(lastPosition_, currentPosition_, 1.0f - config_.smoothing);
    }
    
    // Update VectorPath position
    VectorPath::Position pathPos = vectorPath_->interpolatePosition(currentPosition_);
    vectorPath_->setPosition(pathPos);
    
    lastPosition_ = currentPosition_;
}

void VectorPathLatch::updateBeatSync(float deltaTimeMs) {
    if (config_.syncMode == SyncMode::FREE_RUNNING) {
        return;
    }
    
    // Update current beat based on delta time
    if (tempoInfo_.isPlaying) {
        float deltaTime = deltaTimeMs * 0.001f;
        float beatsPerSecond = tempoInfo_.bpm / 60.0f;
        tempoInfo_.currentBeat += deltaTime * beatsPerSecond;
    }
}

void VectorPathLatch::updateRateModulation(float deltaTimeMs) {
    if (!config_.enableRateModulation) {
        rateModValue_ = 0.0f;
        return;
    }
    
    float deltaTime = deltaTimeMs * 0.001f;
    rateModPhase_ += deltaTime * config_.rateModFreq * 2.0f * M_PI;
    
    // Keep phase in range
    while (rateModPhase_ > 2.0f * M_PI) {
        rateModPhase_ -= 2.0f * M_PI;
    }
    
    rateModValue_ = std::sin(rateModPhase_);
}

void VectorPathLatch::updateRecording(float deltaTimeMs) {
    if (!recording_.recording || !vectorPath_) {
        return;
    }
    
    float currentTime = getTimeMs() * 0.001f;
    float recordTime = currentTime - recording_.recordingStart;
    
    // Check if we're in punch range for punch-in mode
    if (recording_.mode == RecordMode::PUNCH_IN) {
        if (currentPosition_ < recording_.punchInTime || currentPosition_ > recording_.punchOutTime) {
            return;
        }
    }
    
    // Check minimum time interval
    if (recordTime - recording_.lastRecordTime < MIN_RECORD_INTERVAL) {
        return;
    }
    
    // Capture current position if it's different enough
    VectorPath::Position currentPos = vectorPath_->getPosition();
    if (shouldCaptureWaypoint(currentPos)) {
        captureWaypoint(currentPos, recordTime);
        recording_.lastRecordTime = recordTime;
    }
}

void VectorPathLatch::updateCrossfade(float deltaTimeMs) {
    if (!inCrossfade_) {
        return;
    }
    
    float deltaTime = deltaTimeMs * 0.001f;
    crossfadePosition_ += deltaTime / config_.crossfadeTime;
    
    if (crossfadePosition_ >= 1.0f) {
        // Crossfade complete
        inCrossfade_ = false;
        crossfadePosition_ = 0.0f;
    } else {
        // Interpolate between start and end positions
        float t = smoothStep(0.0f, 1.0f, crossfadePosition_);
        VectorPath::Position currentPos;
        currentPos.x = lerp(crossfadeStartPos_.x, crossfadeEndPos_.x, t);
        currentPos.y = lerp(crossfadeStartPos_.y, crossfadeEndPos_.y, t);
        
        if (vectorPath_) {
            vectorPath_->setPosition(currentPos);
        }
    }
}

float VectorPathLatch::calculateBeatDivisionTime(BeatDivision division) const {
    float beatTime = 60.0f / tempoInfo_.bpm; // Time for one quarter note
    float divisionValue = getBeatDivisionValue(division);
    return beatTime * divisionValue;
}

float VectorPathLatch::calculateNextQuantizedBeat(float currentBeat, BeatDivision division) const {
    float divisionValue = getBeatDivisionValue(division);
    return std::ceil(currentBeat / divisionValue) * divisionValue;
}

bool VectorPathLatch::shouldStartPlaybook(float currentBeat) const {
    return currentBeat >= nextQuantizedStart_;
}

bool VectorPathLatch::shouldStopPlaybook(float currentBeat) const {
    float divisionValue = getBeatDivisionValue(config_.beatDivision);
    float nextQuantizedStop = std::ceil(currentBeat / divisionValue) * divisionValue;
    return currentBeat >= nextQuantizedStop;
}

void VectorPathLatch::updateForwardPlayback(float deltaPosition) {
    currentPosition_ += deltaPosition;
    
    bool looped = false;
    if (checkLoopBoundary(currentPosition_, looped)) {
        if (looped && playbackCallback_) {
            playbackCallback_(currentPosition_, true);
        }
    }
}

void VectorPathLatch::updateReversePlayback(float deltaPosition) {
    currentPosition_ -= deltaPosition;
    
    bool looped = false;
    if (checkLoopBoundary(currentPosition_, looped)) {
        if (looped && playbackCallback_) {
            playbackCallback_(currentPosition_, true);
        }
    }
}

void VectorPathLatch::updatePingPongPlayback(float deltaPosition) {
    currentPosition_ += deltaPosition * playbackDirection_;
    
    bool looped = false;
    if (checkLoopBoundary(currentPosition_, looped)) {
        if (looped) {
            playbackDirection_ *= -1.0f; // Reverse direction
            if (playbackCallback_) {
                playbackCallback_(currentPosition_, true);
            }
        }
    }
}

void VectorPathLatch::updateRandomPlayback(float deltaPosition) {
    // Move to next random waypoint when delta accumulates enough
    static float randomAccumulator = 0.0f;
    randomAccumulator += deltaPosition;
    
    if (randomAccumulator >= 0.1f) { // Threshold for random jumps
        randomAccumulator = 0.0f;
        
        if (vectorPath_ && vectorPath_->getWaypointCount() > 0) {
            int waypointIndex = randomSequence_[randomIndex_];
            randomIndex_ = (randomIndex_ + 1) % 32;
            
            if (waypointIndex < vectorPath_->getWaypointCount()) {
                const VectorPath::Waypoint& waypoint = vectorPath_->getWaypoint(waypointIndex);
                currentPosition_ = static_cast<float>(waypointIndex) / (vectorPath_->getWaypointCount() - 1);
                
                if (playbackCallback_) {
                    playbackCallback_(currentPosition_, false);
                }
            }
        }
    }
}

void VectorPathLatch::updatePendulumPlayback(float deltaTimeMs) {
    // Sine wave motion between loop points
    float deltaTime = deltaTimeMs * 0.001f;
    rateModPhase_ += deltaTime * effectiveRate_ * 2.0f * M_PI;
    
    while (rateModPhase_ > 2.0f * M_PI) {
        rateModPhase_ -= 2.0f * M_PI;
    }
    
    float sineValue = std::sin(rateModPhase_);
    float normalizedSine = (sineValue + 1.0f) * 0.5f; // 0 to 1
    
    currentPosition_ = lerp(config_.loopStart, config_.loopEnd, normalizedSine);
}

void VectorPathLatch::updateFreezeMode() {
    // Position doesn't change in freeze mode
}

bool VectorPathLatch::checkLoopBoundary(float& position, bool& looped) {
    looped = false;
    
    if (position > config_.loopEnd) {
        if (config_.crossfadeTime > 0.0f) {
            handleLoopCrossfade(position);
        }
        
        position = config_.loopStart + (position - config_.loopEnd);
        looped = true;
    } else if (position < config_.loopStart) {
        if (config_.crossfadeTime > 0.0f) {
            handleLoopCrossfade(position);
        }
        
        position = config_.loopEnd - (config_.loopStart - position);
        looped = true;
    }
    
    return true;
}

void VectorPathLatch::handleLoopCrossfade(float position) {
    if (!vectorPath_ || config_.crossfadeTime <= 0.0f) {
        return;
    }
    
    VectorPath::Position startPos = vectorPath_->interpolatePosition(config_.loopEnd);
    VectorPath::Position endPos = vectorPath_->interpolatePosition(config_.loopStart);
    
    startCrossfade(startPos, endPos);
}

void VectorPathLatch::startCrossfade(const VectorPath::Position& startPos, const VectorPath::Position& endPos) {
    crossfadeStartPos_ = startPos;
    crossfadeEndPos_ = endPos;
    crossfadePosition_ = 0.0f;
    inCrossfade_ = true;
}

void VectorPathLatch::captureWaypoint(const VectorPath::Position& position, float timestamp) {
    if (recording_.recordBuffer.size() >= MAX_RECORDED_WAYPOINTS) {
        return;
    }
    
    VectorPath::Waypoint waypoint(position.x, position.y, 0.5f);
    waypoint.timeMs = static_cast<uint32_t>(timestamp * 1000.0f);
    
    recording_.recordBuffer.push_back(waypoint);
}

bool VectorPathLatch::shouldCaptureWaypoint(const VectorPath::Position& position) const {
    if (recording_.recordBuffer.empty()) {
        return true;
    }
    
    const VectorPath::Waypoint& lastWaypoint = recording_.recordBuffer.back();
    float distance = std::sqrt(
        std::pow(position.x - lastWaypoint.x, 2) +
        std::pow(position.y - lastWaypoint.y, 2)
    );
    
    return distance >= recording_.minWaypointDistance;
}

void VectorPathLatch::generateRandomSequence() {
    for (int i = 0; i < 32; i++) {
        randomSequence_[i] = nextRandom() % 16; // Assume max 16 waypoints for random selection
    }
}

uint32_t VectorPathLatch::nextRandom() {
    randomState_ = randomState_ * 1664525 + 1013904223; // Linear congruential generator
    return randomState_;
}

float VectorPathLatch::applySwing(float position, float swingAmount) const {
    // Apply swing timing - delay every other beat
    float beatPosition = moduloWrap(position * 2.0f, 0.0f, 2.0f);
    
    if (beatPosition >= 1.0f) {
        return 1.0f + swingAmount * 0.1f; // Delay second beat
    }
    
    return 1.0f;
}

float VectorPathLatch::calculateGrooveOffset(float position) const {
    // Apply groove shift
    return config_.grooveShift * 0.1f;
}

float VectorPathLatch::moduloWrap(float value, float min, float max) const {
    float range = max - min;
    while (value >= max) value -= range;
    while (value < min) value += range;
    return value;
}

float VectorPathLatch::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}

float VectorPathLatch::smoothStep(float edge0, float edge1, float x) const {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

uint32_t VectorPathLatch::getTimeMs() const {
#ifdef STM32H7
    return HAL_GetTick();
#else
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
#endif
}

float VectorPathLatch::getBeatDivisionValue(BeatDivision division) {
    switch (division) {
        case BeatDivision::WHOLE_NOTE: return 4.0f;
        case BeatDivision::HALF_NOTE: return 2.0f;
        case BeatDivision::QUARTER_NOTE: return 1.0f;
        case BeatDivision::EIGHTH_NOTE: return 0.5f;
        case BeatDivision::SIXTEENTH_NOTE: return 0.25f;
        case BeatDivision::THIRTY_SECOND: return 0.125f;
        case BeatDivision::DOTTED_QUARTER: return 1.5f;
        case BeatDivision::DOTTED_EIGHTH: return 0.75f;
        case BeatDivision::TRIPLET_QUARTER: return 1.0f / 3.0f;
        case BeatDivision::TRIPLET_EIGHTH: return 1.0f / 6.0f;
        default: return 1.0f;
    }
}

std::string VectorPathLatch::getBeatDivisionName(BeatDivision division) {
    switch (division) {
        case BeatDivision::WHOLE_NOTE: return "1/1";
        case BeatDivision::HALF_NOTE: return "1/2";
        case BeatDivision::QUARTER_NOTE: return "1/4";
        case BeatDivision::EIGHTH_NOTE: return "1/8";
        case BeatDivision::SIXTEENTH_NOTE: return "1/16";
        case BeatDivision::THIRTY_SECOND: return "1/32";
        case BeatDivision::DOTTED_QUARTER: return "1/4.";
        case BeatDivision::DOTTED_EIGHTH: return "1/8.";
        case BeatDivision::TRIPLET_QUARTER: return "1/4T";
        case BeatDivision::TRIPLET_EIGHTH: return "1/8T";
        default: return "1/4";
    }
}

// Preset management stubs (would need persistent storage implementation)
void VectorPathLatch::savePreset(const std::string& name) {
    // Implementation would save current config to persistent storage
}

bool VectorPathLatch::loadPreset(const std::string& name) {
    // Implementation would load config from persistent storage
    return false;
}

void VectorPathLatch::deletePreset(const std::string& name) {
    // Implementation would delete preset from persistent storage
}

std::vector<std::string> VectorPathLatch::getPresetNames() const {
    // Implementation would return list of saved presets
    return {};
}