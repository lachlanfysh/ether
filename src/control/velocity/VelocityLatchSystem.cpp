#include "VelocityLatchSystem.h"
#include <algorithm>
#include <cmath>
#include <chrono>

VelocityLatchSystem::VelocityLatchSystem() {
    systemConfig_ = LatchSystemConfig();
    
    // Initialize channel configurations
    for (uint8_t i = 0; i < MAX_LATCH_CHANNELS; ++i) {
        channelConfigs_[i] = ChannelLatchConfig();
        channelEnvelopes_[i] = VelocityEnvelope();
        channelEnabled_[i].store(false);
        channelStates_[i] = ChannelLatchState();
        hardwarePins_[i] = 255; // Invalid pin
    }
    
    // Initialize system state
    isActive_.store(false);
    isPaused_.store(false);
    
    // Initialize hardware state
    hardwareInterruptsEnabled_ = false;
    
    // Initialize integration pointers
    velocityCaptureSystem_ = nullptr;
    sequencer_ = nullptr;
    midiInterface_ = nullptr;
    
    // Initialize performance tracking
    currentMetrics_ = LatchMetrics();
    lastUpdateTime_ = 0;
    processingStartTime_ = 0;
    totalProcessingTime_ = 0;
    
    // Initialize automation
    automationRecordingEnabled_ = false;
    recordedEvents_.reserve(1000);
}

VelocityLatchSystem::~VelocityLatchSystem() {
    stopLatchSystem();
    disableHardwareInterrupts();
}

// System Configuration
void VelocityLatchSystem::setSystemConfig(const LatchSystemConfig& config) {
    systemConfig_ = config;
    sanitizeSystemConfig(systemConfig_);
}

void VelocityLatchSystem::setChannelConfig(uint8_t channelId, const ChannelLatchConfig& config) {
    if (!validateChannelId(channelId)) return;
    
    channelConfigs_[channelId] = config;
    sanitizeChannelConfig(channelConfigs_[channelId]);
}

const VelocityLatchSystem::ChannelLatchConfig& VelocityLatchSystem::getChannelConfig(uint8_t channelId) const {
    if (!validateChannelId(channelId)) {
        static ChannelLatchConfig defaultConfig;
        return defaultConfig;
    }
    return channelConfigs_[channelId];
}

// System Control
bool VelocityLatchSystem::startLatchSystem() {
    if (isActive_.load()) {
        return true; // Already active
    }
    
    // Initialize performance tracking
    processingStartTime_ = getCurrentTimeUs();
    lastUpdateTime_ = processingStartTime_;
    currentMetrics_ = LatchMetrics();
    
    // Configure hardware if needed
    if (systemConfig_.enableHardwareControl) {
        configureHardwareButtons();
        enableHardwareInterrupts();
    }
    
    isActive_.store(true);
    isPaused_.store(false);
    
    notifySystemStatus();
    return true;
}

bool VelocityLatchSystem::stopLatchSystem() {
    if (!isActive_.load()) {
        return true; // Already stopped
    }
    
    // Release all active latches
    releaseAllLatches();
    
    // Disable hardware
    disableHardwareInterrupts();
    
    isActive_.store(false);
    isPaused_.store(false);
    
    notifySystemStatus();
    return true;
}

bool VelocityLatchSystem::pauseLatchSystem() {
    if (!isActive_.load()) return false;
    
    isPaused_.store(true);
    notifySystemStatus();
    return true;
}

bool VelocityLatchSystem::resumeLatchSystem() {
    if (!isActive_.load()) return false;
    
    isPaused_.store(false);
    notifySystemStatus();
    return true;
}

// Channel Control
void VelocityLatchSystem::enableChannel(uint8_t channelId, LatchMode mode) {
    if (!validateChannelId(channelId)) return;
    
    channelConfigs_[channelId].mode = mode;
    channelEnabled_[channelId].store(true);
    
    // Reset channel state
    channelStates_[channelId] = ChannelLatchState();
}

void VelocityLatchSystem::disableChannel(uint8_t channelId) {
    if (!validateChannelId(channelId)) return;
    
    // Release any active latch first
    if (channelStates_[channelId].isLatched) {
        releaseLatch(channelId);
    }
    
    channelEnabled_[channelId].store(false);
    channelConfigs_[channelId].mode = LatchMode::OFF;
}

bool VelocityLatchSystem::isChannelEnabled(uint8_t channelId) const {
    if (!validateChannelId(channelId)) return false;
    return channelEnabled_[channelId].load() && channelConfigs_[channelId].mode != LatchMode::OFF;
}

std::vector<uint8_t> VelocityLatchSystem::getActiveChannels() const {
    std::vector<uint8_t> active;
    for (uint8_t i = 0; i < MAX_LATCH_CHANNELS; ++i) {
        if (isChannelEnabled(i) && (channelStates_[i].isLatched || channelStates_[i].isTriggered)) {
            active.push_back(i);
        }
    }
    return active;
}

// Latch Operations
void VelocityLatchSystem::triggerLatch(uint8_t channelId, float velocity, uint32_t timestampUs) {
    if (!validateChannelId(channelId) || !isChannelEnabled(channelId)) return;
    if (!isActive_.load() || isPaused_.load()) return;
    if (!validateVelocity(velocity)) return;
    
    uint32_t currentTime = (timestampUs == 0) ? getCurrentTimeUs() : timestampUs;
    uint32_t processingStart = getCurrentTimeUs();
    
    const ChannelLatchConfig& config = channelConfigs_[channelId];
    ChannelLatchState& state = channelStates_[channelId];
    
    // Check velocity threshold
    if (velocity < config.velocityThreshold) return;
    
    // Handle debouncing
    if (currentTime - state.lastTriggerTime < msToUs(config.debounceTimeMs)) return;
    
    // Check if retriggering is allowed
    if (state.isLatched && !config.enableRetrigger) return;
    if (state.retriggerCount >= config.maxRetriggerCount) return;
    
    // Process based on latch mode
    bool shouldLatch = shouldTriggerLatch(channelId, velocity);
    
    switch (config.mode) {
        case LatchMode::MOMENTARY:
            state.isLatched = true;
            state.isTriggered = true;
            break;
            
        case LatchMode::TOGGLE:
            state.isLatched = !state.isLatched;
            state.isTriggered = true;
            break;
            
        case LatchMode::TIMED_HOLD:
            state.isLatched = true;
            state.isTriggered = true;
            // Will auto-release after holdTimeMs
            break;
            
        case LatchMode::SUSTAIN_PEDAL:
            state.isLatched = true;
            state.isTriggered = true;
            break;
            
        case LatchMode::VELOCITY_THRESHOLD:
            if (shouldLatch) {
                state.isLatched = true;
                state.isTriggered = true;
            }
            break;
            
        case LatchMode::PATTERN_SYNC:
            // Sync to pattern timing
            state.isLatched = true;
            state.isTriggered = true;
            break;
            
        default:
            return; // OFF mode or unknown
    }
    
    if (state.isLatched || state.isTriggered) {
        // Update state
        state.originalVelocity = velocity;
        state.currentVelocity = config.maintainOriginalVelocity ? velocity : config.sustainLevel;
        state.targetVelocity = state.currentVelocity;
        state.latchStartTime = usToMs(currentTime);
        state.lastTriggerTime = currentTime;
        state.envelopePhase = 0.0f;
        state.isAttacking = config.enableVelocityEnvelope;
        state.isReleasing = false;
        
        // Handle retriggering
        if (state.isLatched && config.enableRetrigger) {
            state.retriggerCount++;
        }
        
        // Handle group processing
        if (config.latchGroup > 0) {
            processGroupTrigger(config.latchGroup, channelId, velocity);
        }
        
        // Record automation if enabled
        if (automationRecordingEnabled_) {
            recordLatchEvent(channelId, true, velocity, usToMs(currentTime));
        }
        
        // Update metrics
        currentMetrics_.totalLatchEvents++;
        currentMetrics_.channelLatchCounts[channelId]++;
        
        // Notify callback
        notifyLatchTrigger(channelId, velocity, usToMs(currentTime));
    }
    
    // Update performance metrics
    uint32_t processingTime = getCurrentTimeUs() - processingStart;
    totalProcessingTime_ += processingTime;
    currentMetrics_.maxLatencyUs = std::max(currentMetrics_.maxLatencyUs, processingTime);
}

void VelocityLatchSystem::releaseLatch(uint8_t channelId) {
    if (!validateChannelId(channelId)) return;
    if (!isActive_.load()) return;
    
    ChannelLatchState& state = channelStates_[channelId];
    const ChannelLatchConfig& config = channelConfigs_[channelId];
    
    if (!state.isLatched && !state.isTriggered) return;
    
    uint32_t currentTime = getCurrentTimeUs();
    uint32_t latchDuration = usToMs(currentTime) - state.latchStartTime;
    
    // Handle different release modes
    switch (config.releaseMode) {
        case ReleaseMode::INSTANT:
            state.isLatched = false;
            state.isTriggered = false;
            state.isReleasing = false;
            state.currentVelocity = 0.0f;
            state.targetVelocity = 0.0f;
            break;
            
        case ReleaseMode::LINEAR:
        case ReleaseMode::EXPONENTIAL:
        case ReleaseMode::LOGARITHMIC:
        case ReleaseMode::CUSTOM_ENVELOPE:
            state.isReleasing = true;
            state.isAttacking = false;
            state.targetVelocity = config.releaseVelocity;
            state.envelopePhase = 0.0f;
            // Will transition over releaseTimeMs
            break;
            
        case ReleaseMode::PATTERN_QUANTIZED:
            // Handle pattern quantized release
            state.isReleasing = true;
            state.targetVelocity = config.releaseVelocity;
            break;
    }
    
    // Handle group processing
    if (config.latchGroup > 0) {
        processGroupRelease(config.latchGroup, channelId);
    }
    
    // Record automation if enabled
    if (automationRecordingEnabled_) {
        recordLatchEvent(channelId, false, state.currentVelocity, usToMs(currentTime));
    }
    
    // Update metrics
    currentMetrics_.totalReleaseEvents++;
    currentMetrics_.channelActiveTimes[channelId] += latchDuration;
    currentMetrics_.longestLatchTimeMs = std::max(currentMetrics_.longestLatchTimeMs, latchDuration);
    
    // Reset retrigger count
    state.retriggerCount = 0;
    
    // Notify callback
    notifyLatchRelease(channelId, latchDuration);
}

void VelocityLatchSystem::toggleLatch(uint8_t channelId, float velocity) {
    if (!validateChannelId(channelId)) return;
    
    const ChannelLatchState& state = channelStates_[channelId];
    
    if (state.isLatched || state.isTriggered) {
        releaseLatch(channelId);
    } else {
        triggerLatch(channelId, velocity);
    }
}

void VelocityLatchSystem::releaseAllLatches() {
    for (uint8_t i = 0; i < MAX_LATCH_CHANNELS; ++i) {
        if (channelStates_[i].isLatched || channelStates_[i].isTriggered) {
            releaseLatch(i);
        }
    }
}

void VelocityLatchSystem::emergencyStop() {
    // Immediate stop - bypass normal release processing
    for (uint8_t i = 0; i < MAX_LATCH_CHANNELS; ++i) {
        ChannelLatchState& state = channelStates_[i];
        state.isLatched = false;
        state.isTriggered = false;
        state.isReleasing = false;
        state.isAttacking = false;
        state.currentVelocity = 0.0f;
        state.targetVelocity = 0.0f;
        state.retriggerCount = 0;
    }
    
    currentMetrics_.activeLatchCount = 0;
    notifySystemStatus();
}

// Velocity Processing
float VelocityLatchSystem::processVelocity(uint8_t channelId, float inputVelocity, uint32_t timestampUs) {
    if (!validateChannelId(channelId) || !isChannelEnabled(channelId)) {
        return inputVelocity; // Pass through if disabled
    }
    
    if (!isActive_.load() || isPaused_.load()) {
        return inputVelocity; // Pass through if not active
    }
    
    uint32_t currentTime = (timestampUs == 0) ? getCurrentTimeUs() : timestampUs;
    
    // Update latch state
    updateChannelLatch(channelId, currentTime);
    
    const ChannelLatchState& state = channelStates_[channelId];
    const ChannelLatchConfig& config = channelConfigs_[channelId];
    (void)config; // May be used in future velocity processing enhancements
    
    // Return current velocity if latched, otherwise pass through
    float outputVelocity = (state.isLatched || state.isTriggered) ? 
                          state.currentVelocity : inputVelocity;
    
    // Apply global multiplier
    outputVelocity *= systemConfig_.globalVelocityMultiplier;
    
    // Clamp to valid range
    outputVelocity = std::clamp(outputVelocity, 0.0f, 1.0f);
    
    return outputVelocity;
}

void VelocityLatchSystem::updateLatchStates(uint32_t currentTimeUs) {
    if (!isActive_.load() || isPaused_.load()) return;
    
    for (uint8_t i = 0; i < MAX_LATCH_CHANNELS; ++i) {
        if (isChannelEnabled(i)) {
            updateChannelLatch(i, currentTimeUs);
        }
    }
    
    // Update group states
    updateGroupStates(currentTimeUs);
    
    // Update performance metrics
    updatePerformanceMetrics();
    
    lastUpdateTime_ = currentTimeUs;
}

float VelocityLatchSystem::calculateEnvelopeOutput(uint8_t channelId, float phase, float velocity) const {
    if (!validateChannelId(channelId)) return velocity;
    
    const ChannelLatchConfig& config = channelConfigs_[channelId];
    const VelocityEnvelope& envelope = channelEnvelopes_[channelId];
    const ChannelLatchState& state = channelStates_[channelId];
    
    if (!config.enableVelocityEnvelope) return velocity;
    
    if (state.isAttacking) {
        // Attack phase
        float attackValue = interpolateEnvelope(envelope.attackCurve, phase);
        return velocity * attackValue;
    } else if (state.isReleasing) {
        // Release phase  
        float releaseValue = interpolateEnvelope(envelope.releaseCurve, phase);
        return state.originalVelocity * releaseValue;
    } else {
        // Sustain phase
        return velocity * envelope.sustainLevel;
    }
}

float VelocityLatchSystem::applyCrossfade(uint8_t channelId, float fromVelocity, float toVelocity, float phase) const {
    if (!validateChannelId(channelId)) return toVelocity;
    
    const ChannelLatchConfig& config = channelConfigs_[channelId];
    if (!config.enableCrossfade) return toVelocity;
    
    return crossfade(fromVelocity, toVelocity, phase);
}

// State Management
const VelocityLatchSystem::ChannelLatchState& VelocityLatchSystem::getChannelState(uint8_t channelId) const {
    if (!validateChannelId(channelId)) {
        static ChannelLatchState defaultState;
        return defaultState;
    }
    return channelStates_[channelId];
}

bool VelocityLatchSystem::isChannelLatched(uint8_t channelId) const {
    if (!validateChannelId(channelId)) return false;
    return channelStates_[channelId].isLatched;
}

bool VelocityLatchSystem::isChannelTriggered(uint8_t channelId) const {
    if (!validateChannelId(channelId)) return false;
    return channelStates_[channelId].isTriggered;
}

float VelocityLatchSystem::getCurrentVelocity(uint8_t channelId) const {
    if (!validateChannelId(channelId)) return 0.0f;
    return channelStates_[channelId].currentVelocity;
}

uint32_t VelocityLatchSystem::getLatchDuration(uint8_t channelId) const {
    if (!validateChannelId(channelId)) return 0;
    
    const ChannelLatchState& state = channelStates_[channelId];
    if (!state.isLatched && !state.isTriggered) return 0;
    
    return usToMs(getCurrentTimeUs()) - state.latchStartTime;
}

// Group Management
void VelocityLatchSystem::setChannelGroup(uint8_t channelId, uint8_t groupId) {
    if (!validateChannelId(channelId) || !validateGroupId(groupId)) return;
    
    channelConfigs_[channelId].latchGroup = groupId;
    channelStates_[channelId].currentGroup = groupId;
}

void VelocityLatchSystem::triggerGroup(uint8_t groupId, float velocity) {
    if (!validateGroupId(groupId)) return;
    
    for (uint8_t i = 0; i < MAX_LATCH_CHANNELS; ++i) {
        if (channelConfigs_[i].latchGroup == groupId && isChannelEnabled(i)) {
            triggerLatch(i, velocity);
        }
    }
}

void VelocityLatchSystem::releaseGroup(uint8_t groupId) {
    if (!validateGroupId(groupId)) return;
    
    for (uint8_t i = 0; i < MAX_LATCH_CHANNELS; ++i) {
        if (channelConfigs_[i].latchGroup == groupId && 
            (channelStates_[i].isLatched || channelStates_[i].isTriggered)) {
            releaseLatch(i);
        }
    }
}

std::vector<uint8_t> VelocityLatchSystem::getGroupChannels(uint8_t groupId) const {
    std::vector<uint8_t> channels;
    
    for (uint8_t i = 0; i < MAX_LATCH_CHANNELS; ++i) {
        if (channelConfigs_[i].latchGroup == groupId && isChannelEnabled(i)) {
            channels.push_back(i);
        }
    }
    
    return channels;
}

uint8_t VelocityLatchSystem::getActiveGroupCount() const {
    std::array<bool, MAX_GROUPS + 1> activeGroups = {};
    uint8_t count = 0;
    
    for (uint8_t i = 0; i < MAX_LATCH_CHANNELS; ++i) {
        uint8_t groupId = channelConfigs_[i].latchGroup;
        if (groupId > 0 && groupId <= MAX_GROUPS && 
            (channelStates_[i].isLatched || channelStates_[i].isTriggered)) {
            if (!activeGroups[groupId]) {
                activeGroups[groupId] = true;
                count++;
            }
        }
    }
    
    return count;
}

// Envelope Management
void VelocityLatchSystem::setChannelEnvelope(uint8_t channelId, const VelocityEnvelope& envelope) {
    if (!validateChannelId(channelId)) return;
    channelEnvelopes_[channelId] = envelope;
}

const VelocityLatchSystem::VelocityEnvelope& VelocityLatchSystem::getChannelEnvelope(uint8_t channelId) const {
    if (!validateChannelId(channelId)) {
        static VelocityEnvelope defaultEnvelope;
        return defaultEnvelope;
    }
    return channelEnvelopes_[channelId];
}

void VelocityLatchSystem::generateEnvelope(uint8_t channelId, ReleaseMode mode, uint32_t durationMs) {
    if (!validateChannelId(channelId)) return;
    
    VelocityEnvelope& envelope = channelEnvelopes_[channelId];
    
    switch (mode) {
        case ReleaseMode::LINEAR:
            generateLinearEnvelope(envelope, durationMs);
            break;
        case ReleaseMode::EXPONENTIAL:
            generateExponentialEnvelope(envelope, durationMs, 2.0f);
            break;
        case ReleaseMode::LOGARITHMIC:
            generateLogarithmicEnvelope(envelope, durationMs, 0.5f);
            break;
        default:
            generateLinearEnvelope(envelope, durationMs);
            break;
    }
}

void VelocityLatchSystem::resetChannelEnvelope(uint8_t channelId) {
    if (!validateChannelId(channelId)) return;
    channelEnvelopes_[channelId] = VelocityEnvelope();
}

// Timing and Sync
void VelocityLatchSystem::setTempo(float bpm) {
    systemConfig_.tempoBPM = std::clamp(bpm, MIN_TEMPO_BPM, MAX_TEMPO_BPM);
}

void VelocityLatchSystem::syncToPatternPosition(uint32_t patternPositionMs) {
    // Sync latch timing to pattern position
    (void)patternPositionMs; // Implementation depends on pattern system
}

uint32_t VelocityLatchSystem::quantizeToPattern(uint32_t timeMs, uint32_t quantizeValue) const {
    if (quantizeValue == 0) return timeMs;
    
    // Quantize to nearest beat subdivision
    uint32_t beatMs = beatsToMs(1.0f);
    uint32_t quantizeMs = beatMs / quantizeValue;
    
    return (timeMs / quantizeMs) * quantizeMs;
}

// Performance Analysis
VelocityLatchSystem::LatchMetrics VelocityLatchSystem::getCurrentMetrics() const {
    return currentMetrics_;
}

float VelocityLatchSystem::getChannelActivity(uint8_t channelId) const {
    if (!validateChannelId(channelId)) return 0.0f;
    
    const ChannelLatchState& state = channelStates_[channelId];
    return (state.isLatched || state.isTriggered) ? state.currentVelocity : 0.0f;
}

size_t VelocityLatchSystem::getEstimatedMemoryUsage() const {
    size_t baseSize = sizeof(*this);
    size_t automationSize = recordedEvents_.capacity() * sizeof(LatchAutomationEvent);
    return baseSize + automationSize;
}

void VelocityLatchSystem::resetPerformanceCounters() {
    currentMetrics_ = LatchMetrics();
    processingStartTime_ = getCurrentTimeUs();
    totalProcessingTime_ = 0;
}

// Hardware Integration
void VelocityLatchSystem::setHardwareTrigger(uint8_t channelId, uint8_t hardwarePin) {
    if (!validateChannelId(channelId)) return;
    hardwarePins_[channelId] = hardwarePin;
}

void VelocityLatchSystem::configureHardwareButtons() {
    // Configure hardware button pins for latch triggering
    // Implementation depends on hardware platform
}

void VelocityLatchSystem::enableHardwareInterrupts() {
    hardwareInterruptsEnabled_ = true;
    // Enable GPIO interrupts for hardware buttons
}

void VelocityLatchSystem::disableHardwareInterrupts() {
    hardwareInterruptsEnabled_ = false;
    // Disable GPIO interrupts
}

bool VelocityLatchSystem::testHardwareTrigger(uint8_t channelId) const {
    if (!validateChannelId(channelId)) return false;
    // Test hardware trigger functionality
    (void)channelId;
    return true; // Placeholder
}

// External Integration
void VelocityLatchSystem::integrateWithVelocityCapture(VelocityCaptureSystem* captureSystem) {
    velocityCaptureSystem_ = captureSystem;
}

void VelocityLatchSystem::integrateWithSequencer(class SequencerEngine* sequencer) {
    sequencer_ = sequencer;
}

void VelocityLatchSystem::integrateWithMIDI(class MIDIInterface* midiInterface) {
    midiInterface_ = midiInterface;
}

void VelocityLatchSystem::setExternalTriggerCallback(std::function<void(uint8_t, float)> callback) {
    externalTriggerCallback_ = callback;
}

// Automation and Recording
void VelocityLatchSystem::enableAutomationRecording(bool enable) {
    automationRecordingEnabled_ = enable;
    if (!enable) {
        clearAutomationRecording();
    }
}

void VelocityLatchSystem::recordLatchEvent(uint8_t channelId, bool isLatch, float velocity, uint32_t timestamp) {
    if (!automationRecordingEnabled_) return;
    
    recordedEvents_.emplace_back(channelId, isLatch, velocity, timestamp);
    
    // Limit recording buffer size
    if (recordedEvents_.size() > 10000) {
        recordedEvents_.erase(recordedEvents_.begin(), recordedEvents_.begin() + 1000);
    }
}

std::vector<LatchAutomationEvent> VelocityLatchSystem::getRecordedAutomation() const {
    return recordedEvents_;
}

void VelocityLatchSystem::clearAutomationRecording() {
    recordedEvents_.clear();
}

// Internal processing methods
void VelocityLatchSystem::updateChannelLatch(uint8_t channelId, uint32_t currentTimeUs) {
    if (!validateChannelId(channelId)) return;
    
    const ChannelLatchConfig& config = channelConfigs_[channelId];
    ChannelLatchState& state = channelStates_[channelId];
    
    // Check for timed release
    if (shouldReleaseLatch(channelId, currentTimeUs)) {
        processLatchTransition(channelId, false, currentTimeUs);
        return;
    }
    
    // Update velocity envelope
    if (config.enableVelocityEnvelope) {
        updateVelocityEnvelope(channelId, currentTimeUs);
    }
    
    // Update estimated release time
    if (state.isReleasing && config.releaseTimeMs > 0) {
        uint32_t elapsedMs = usToMs(currentTimeUs - state.lastTriggerTime);
        if (elapsedMs >= config.releaseTimeMs) {
            state.isReleasing = false;
            state.isLatched = false;
            state.isTriggered = false;
            state.currentVelocity = 0.0f;
        }
    }
}

bool VelocityLatchSystem::shouldTriggerLatch(uint8_t channelId, float velocity) const {
    if (!validateChannelId(channelId)) return false;
    
    const ChannelLatchConfig& config = channelConfigs_[channelId];
    
    // Check velocity threshold
    if (velocity < config.velocityThreshold) return false;
    
    // Mode-specific trigger logic
    switch (config.mode) {
        case LatchMode::VELOCITY_THRESHOLD:
            return velocity >= config.velocityThreshold;
        default:
            return true;
    }
}

bool VelocityLatchSystem::shouldReleaseLatch(uint8_t channelId, uint32_t currentTimeUs) const {
    if (!validateChannelId(channelId)) return false;
    
    const ChannelLatchConfig& config = channelConfigs_[channelId];
    const ChannelLatchState& state = channelStates_[channelId];
    
    if (!state.isLatched && !state.isTriggered) return false;
    
    // Check timed hold mode
    if (config.mode == LatchMode::TIMED_HOLD) {
        uint32_t elapsedMs = usToMs(currentTimeUs) - state.latchStartTime;
        return elapsedMs >= config.holdTimeMs;
    }
    
    // Check maximum latch time safety limit
    uint32_t elapsedMs = usToMs(currentTimeUs) - state.latchStartTime;
    if (elapsedMs >= systemConfig_.maxLatchTimeMs) {
        return true;
    }
    
    return false;
}

void VelocityLatchSystem::processLatchTransition(uint8_t channelId, bool newLatchState, uint32_t currentTimeUs) {
    (void)channelId;
    (void)newLatchState;
    (void)currentTimeUs;
    // Implementation for smooth latch state transitions
}

void VelocityLatchSystem::updateVelocityEnvelope(uint8_t channelId, uint32_t currentTimeUs) {
    if (!validateChannelId(channelId)) return;
    
    const ChannelLatchConfig& config = channelConfigs_[channelId];
    ChannelLatchState& state = channelStates_[channelId];
    const VelocityEnvelope& envelope = channelEnvelopes_[channelId];
    (void)config; // May be used in future envelope processing
    
    uint32_t elapsedMs = usToMs(currentTimeUs - state.lastTriggerTime);
    
    if (state.isAttacking) {
        // Attack phase
        if (elapsedMs >= config.attackTimeMs) {
            state.isAttacking = false;
            state.envelopePhase = 1.0f;
            state.currentVelocity = state.originalVelocity * envelope.sustainLevel;
        } else {
            state.envelopePhase = static_cast<float>(elapsedMs) / config.attackTimeMs;
            state.currentVelocity = calculateEnvelopeOutput(channelId, state.envelopePhase, state.originalVelocity);
        }
    } else if (state.isReleasing) {
        // Release phase
        if (elapsedMs >= config.releaseTimeMs) {
            state.isReleasing = false;
            state.isLatched = false;
            state.isTriggered = false;
            state.currentVelocity = 0.0f;
        } else {
            state.envelopePhase = static_cast<float>(elapsedMs) / config.releaseTimeMs;
            state.currentVelocity = calculateEnvelopeOutput(channelId, state.envelopePhase, state.originalVelocity);
        }
    }
}

// Group processing methods
void VelocityLatchSystem::updateGroupStates(uint32_t currentTimeUs) {
    (void)currentTimeUs;
    // Update group-based latch behaviors
}

void VelocityLatchSystem::processGroupTrigger(uint8_t groupId, uint8_t triggerChannelId, float velocity) {
    if (!validateGroupId(groupId) || !validateChannelId(triggerChannelId)) return;
    
    // Handle group triggering behaviors
    for (uint8_t i = 0; i < MAX_LATCH_CHANNELS; ++i) {
        if (i == triggerChannelId) continue;
        
        const ChannelLatchConfig& config = channelConfigs_[i];
        if (config.latchGroup == groupId && isChannelEnabled(i)) {
            if (config.muteOnGroupTrigger) {
                releaseLatch(i);
            } else if (config.inheritGroupVelocity) {
                triggerLatch(i, velocity);
            }
        }
    }
}

void VelocityLatchSystem::processGroupRelease(uint8_t groupId, uint8_t releaseChannelId) {
    (void)groupId;
    (void)releaseChannelId;
    // Handle group release behaviors
}

// Envelope generation methods
void VelocityLatchSystem::generateLinearEnvelope(VelocityEnvelope& envelope, uint32_t durationMs) const {
    envelope.releaseDurationMs = durationMs;
    envelope.releaseCurve.clear();
    envelope.releaseCurve.push_back(1.0f);
    envelope.releaseCurve.push_back(0.0f);
}

void VelocityLatchSystem::generateExponentialEnvelope(VelocityEnvelope& envelope, uint32_t durationMs, float curve) const {
    envelope.releaseDurationMs = durationMs;
    envelope.releaseCurve.clear();
    
    const uint32_t numPoints = 32;
    for (uint32_t i = 0; i <= numPoints; ++i) {
        float t = static_cast<float>(i) / numPoints;
        float value = std::pow(1.0f - t, curve);
        envelope.releaseCurve.push_back(value);
    }
}

void VelocityLatchSystem::generateLogarithmicEnvelope(VelocityEnvelope& envelope, uint32_t durationMs, float curve) const {
    envelope.releaseDurationMs = durationMs;
    envelope.releaseCurve.clear();
    
    const uint32_t numPoints = 32;
    for (uint32_t i = 0; i <= numPoints; ++i) {
        float t = static_cast<float>(i) / numPoints;
        float value = 1.0f - std::log(1.0f + t * (std::exp(curve) - 1.0f)) / curve;
        envelope.releaseCurve.push_back(std::max(0.0f, value));
    }
}

void VelocityLatchSystem::generateCustomEnvelope(VelocityEnvelope& envelope, const std::vector<float>& points, uint32_t durationMs) const {
    envelope.releaseDurationMs = durationMs;
    envelope.releaseCurve = points;
}

// Timing utilities
uint32_t VelocityLatchSystem::getCurrentTimeUs() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(duration).count());
}

float VelocityLatchSystem::calculateTempoMultiplier() const {
    return systemConfig_.tempoBPM / 120.0f; // Normalized to 120 BPM
}

uint32_t VelocityLatchSystem::beatsToMs(float beats) const {
    if (systemConfig_.tempoBPM <= 0.0f) return 0;
    return static_cast<uint32_t>((beats * 60000.0f) / systemConfig_.tempoBPM);
}

// Validation helpers
bool VelocityLatchSystem::validateChannelId(uint8_t channelId) const {
    return channelId < MAX_LATCH_CHANNELS;
}

bool VelocityLatchSystem::validateGroupId(uint8_t groupId) const {
    return groupId <= MAX_GROUPS;
}

bool VelocityLatchSystem::validateVelocity(float velocity) const {
    return velocity >= 0.0f && velocity <= 1.0f && !std::isnan(velocity) && !std::isinf(velocity);
}

void VelocityLatchSystem::sanitizeChannelConfig(ChannelLatchConfig& config) const {
    config.holdTimeMs = std::clamp(config.holdTimeMs, MIN_LATCH_TIME_MS, systemConfig_.maxLatchTimeMs);
    config.releaseTimeMs = std::clamp(config.releaseTimeMs, 1u, 10000u);
    config.attackTimeMs = std::clamp(config.attackTimeMs, 1u, 1000u);
    config.debounceTimeMs = std::clamp(config.debounceTimeMs, 1u, 100u);
    config.velocityThreshold = std::clamp(config.velocityThreshold, 0.0f, 1.0f);
    config.sustainLevel = std::clamp(config.sustainLevel, 0.0f, 1.0f);
    config.releaseVelocity = std::clamp(config.releaseVelocity, 0.0f, 1.0f);
    config.velocityCurveAmount = std::clamp(config.velocityCurveAmount, 0.1f, 5.0f);
    config.maxRetriggerCount = std::clamp(config.maxRetriggerCount, uint8_t(1), uint8_t(10));
    config.latchGroup = std::clamp(config.latchGroup, uint8_t(0), uint8_t(MAX_GROUPS));
}

void VelocityLatchSystem::sanitizeSystemConfig(LatchSystemConfig& config) const {
    config.globalVelocityMultiplier = std::clamp(config.globalVelocityMultiplier, 0.1f, 5.0f);
    config.maxLatchTimeMs = std::clamp(config.maxLatchTimeMs, 1000u, 300000u);
    config.tempoBPM = std::clamp(config.tempoBPM, MIN_TEMPO_BPM, MAX_TEMPO_BPM);
    config.updateIntervalUs = std::clamp(config.updateIntervalUs, 100u, 10000u);
    config.processingPriority = std::clamp(config.processingPriority, uint8_t(1), uint8_t(99));
}

// Notification helpers
void VelocityLatchSystem::notifyLatchTrigger(uint8_t channelId, float velocity, uint32_t timestamp) {
    if (latchTriggerCallback_) {
        latchTriggerCallback_(channelId, velocity, timestamp);
    }
}

void VelocityLatchSystem::notifyLatchRelease(uint8_t channelId, uint32_t duration) {
    if (latchReleaseCallback_) {
        latchReleaseCallback_(channelId, duration);
    }
}

void VelocityLatchSystem::notifyVelocityUpdate(uint8_t channelId, float velocity) {
    if (velocityUpdateCallback_) {
        velocityUpdateCallback_(channelId, velocity);
    }
}

void VelocityLatchSystem::notifySystemStatus() {
    if (systemStatusCallback_) {
        systemStatusCallback_(isActive_.load(), currentMetrics_);
    }
}

void VelocityLatchSystem::notifyError(const std::string& error) {
    if (errorCallback_) {
        errorCallback_(error);
    }
}

// Utility methods
float VelocityLatchSystem::interpolateEnvelope(const std::vector<float>& curve, float phase) const {
    if (curve.empty()) return 1.0f;
    if (curve.size() == 1) return curve[0];
    
    phase = std::clamp(phase, 0.0f, 1.0f);
    float scaledPhase = phase * (curve.size() - 1);
    uint32_t index = static_cast<uint32_t>(scaledPhase);
    float fraction = scaledPhase - index;
    
    if (index >= curve.size() - 1) return curve.back();
    
    return curve[index] * (1.0f - fraction) + curve[index + 1] * fraction;
}

float VelocityLatchSystem::applyCurve(float input, uint8_t curveType, float amount) const {
    switch (curveType) {
        case 0: // Linear
            return input * amount;
        case 1: // Exponential
            return std::pow(input, amount);
        case 2: // Logarithmic
            if (input <= 0.0f) return 0.0f;
            return std::log(1.0f + input * (std::exp(amount) - 1.0f)) / amount;
        case 3: // S-curve
            {
                float x = input * 2.0f - 1.0f;
                float curved = x / (1.0f + std::abs(x) * amount);
                return (curved + 1.0f) * 0.5f;
            }
        default:
            return input;
    }
}

float VelocityLatchSystem::crossfade(float a, float b, float phase) const {
    phase = std::clamp(phase, 0.0f, 1.0f);
    return a * (1.0f - phase) + b * phase;
}

void VelocityLatchSystem::updatePerformanceMetrics() {
    // Update active latch count
    uint32_t activeCount = 0;
    for (uint8_t i = 0; i < MAX_LATCH_CHANNELS; ++i) {
        if (channelStates_[i].isLatched || channelStates_[i].isTriggered) {
            activeCount++;
        }
    }
    currentMetrics_.activeLatchCount = activeCount;
    
    // Update CPU usage
    uint32_t currentTime = getCurrentTimeUs();
    uint32_t totalTime = currentTime - processingStartTime_;
    if (totalTime > 0) {
        currentMetrics_.cpuUsage = (static_cast<float>(totalProcessingTime_) / totalTime) * 100.0f;
    }
    
    // Update average latency
    if (currentMetrics_.totalLatchEvents > 0) {
        currentMetrics_.averageLatencyUs = totalProcessingTime_ / currentMetrics_.totalLatchEvents;
    }
}