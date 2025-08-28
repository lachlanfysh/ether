#pragma once
#include <cstdint>
#include <vector>
#include <array>
#include <functional>
#include <memory>
#include <atomic>
#include <chrono>

/**
 * VelocityLatchSystem - Advanced velocity latching with hold and release modes
 * 
 * Provides comprehensive velocity latching capabilities for the EtherSynth:
 * - Multi-mode velocity latching (momentary, toggle, timed hold)
 * - Velocity sustain and release envelope processing
 * - Per-channel latch state management and timing control
 * - Integration with velocity capture for seamless input processing
 * - Hardware-optimized for STM32 H7 embedded platform with real-time operation
 * 
 * Features:
 * - 16-channel independent velocity latching with configurable modes
 * - Momentary, toggle, and timed hold latch behaviors
 * - Velocity envelope generation for smooth attack/release transitions
 * - Configurable hold times and release curves per channel
 * - Cross-channel velocity grouping and gang latching
 * - Integration with pattern sequencing and sample triggering
 * - Real-time safe operations for live performance use
 * - Memory-efficient state management for embedded systems
 */
class VelocityLatchSystem {
public:
    // Maximum number of latch channels
    static constexpr uint8_t MAX_LATCH_CHANNELS = 16;
    
    // Velocity latch modes
    enum class LatchMode {
        OFF,                    // No latching - velocity passes through
        MOMENTARY,              // Latch while input is active
        TOGGLE,                 // Toggle latch state on each trigger
        TIMED_HOLD,            // Hold for specified duration then release
        SUSTAIN_PEDAL,         // Sustain pedal style latching
        VELOCITY_THRESHOLD,    // Latch only above velocity threshold
        PATTERN_SYNC          // Sync latch timing to pattern beats
    };
    
    // Velocity release modes
    enum class ReleaseMode {
        INSTANT,               // Immediate release to zero
        LINEAR,                // Linear fade to zero
        EXPONENTIAL,           // Exponential decay curve
        LOGARITHMIC,           // Logarithmic release curve
        CUSTOM_ENVELOPE,       // User-defined release envelope
        PATTERN_QUANTIZED     // Release quantized to pattern timing
    };
    
    // Latch trigger source
    enum class TriggerSource {
        VELOCITY_INPUT,        // Direct velocity input trigger
        HARDWARE_BUTTON,       // Physical button/switch
        MIDI_NOTE,             // MIDI note on/off
        PATTERN_STEP,          // Pattern step trigger
        EXTERNAL_GATE,         // External gate signal
        SOFTWARE_TRIGGER      // Software-generated trigger
    };
    
    // Per-channel latch configuration
    struct ChannelLatchConfig {
        LatchMode mode;                      // Primary latch mode
        ReleaseMode releaseMode;             // Release behavior
        TriggerSource triggerSource;         // What triggers the latch
        
        // Timing parameters
        uint32_t holdTimeMs;                 // Hold duration for timed mode
        uint32_t releaseTimeMs;              // Release fade time
        uint32_t attackTimeMs;               // Attack fade time
        uint32_t debounceTimeMs;             // Trigger debounce time
        
        // Velocity parameters
        float velocityThreshold;             // Minimum velocity to trigger latch
        float sustainLevel;                  // Level to sustain at (0.0-1.0)
        float releaseVelocity;               // Velocity during release phase
        bool maintainOriginalVelocity;       // Keep original trigger velocity
        
        // Advanced options
        bool enableVelocityEnvelope;         // Apply velocity envelope shaping
        bool enableCrossfade;                // Crossfade between latch states
        uint8_t velocityEnvelopeType;        // Envelope curve type (0-3)
        float velocityCurveAmount;           // Envelope curve intensity
        bool enableRetrigger;                // Allow re-triggering while latched
        uint8_t maxRetriggerCount;           // Maximum retriggers before reset
        
        // Group settings
        uint8_t latchGroup;                  // Latch group ID (0=independent)
        bool muteOnGroupTrigger;             // Mute when other group members trigger
        bool inheritGroupVelocity;           // Use group leader velocity
        
        ChannelLatchConfig() :
            mode(LatchMode::OFF),
            releaseMode(ReleaseMode::LINEAR),
            triggerSource(TriggerSource::VELOCITY_INPUT),
            holdTimeMs(1000),
            releaseTimeMs(500),
            attackTimeMs(50),
            debounceTimeMs(20),
            velocityThreshold(0.1f),
            sustainLevel(1.0f),
            releaseVelocity(0.0f),
            maintainOriginalVelocity(true),
            enableVelocityEnvelope(false),
            enableCrossfade(true),
            velocityEnvelopeType(0),
            velocityCurveAmount(1.0f),
            enableRetrigger(false),
            maxRetriggerCount(3),
            latchGroup(0),
            muteOnGroupTrigger(false),
            inheritGroupVelocity(false) {}
    };
    
    // Global latch system configuration
    struct LatchSystemConfig {
        bool enableGlobalLatch;              // Master latch enable/disable
        float globalVelocityMultiplier;      // Global velocity scaling
        uint32_t maxLatchTimeMs;             // Safety limit for latch duration
        bool enableTempoSync;                // Sync latch timing to tempo
        float tempoBPM;                      // Tempo for sync calculations
        
        // Performance settings
        uint32_t updateIntervalUs;           // Latch state update interval
        bool enableRealTimeProcessing;       // Real-time processing mode
        uint8_t processingPriority;          // Thread priority (1-99)
        bool enableLowLatencyMode;           // Optimize for lowest latency
        
        // Integration settings
        bool enablePatternIntegration;       // Integrate with pattern system
        bool enableMIDIControl;              // Accept MIDI latch control
        bool enableHardwareControl;          // Hardware button/switch control
        bool enableAutomationRecording;      // Record latch automation
        
        LatchSystemConfig() :
            enableGlobalLatch(true),
            globalVelocityMultiplier(1.0f),
            maxLatchTimeMs(60000),
            enableTempoSync(false),
            tempoBPM(120.0f),
            updateIntervalUs(1000),
            enableRealTimeProcessing(true),
            processingPriority(85),
            enableLowLatencyMode(true),
            enablePatternIntegration(true),
            enableMIDIControl(true),
            enableHardwareControl(true),
            enableAutomationRecording(false) {}
    };
    
    // Current latch state for a channel
    struct ChannelLatchState {
        bool isLatched;                      // Current latch state
        bool isTriggered;                    // Currently being triggered
        bool isReleasing;                    // In release phase
        bool isAttacking;                    // In attack phase
        
        float currentVelocity;               // Current output velocity
        float targetVelocity;                // Target velocity for transitions
        float originalVelocity;              // Original trigger velocity
        float envelopePhase;                 // Current envelope phase (0.0-1.0)
        
        uint32_t latchStartTime;             // When latch was triggered
        uint32_t latchDuration;              // How long latched (ms)
        uint32_t lastTriggerTime;            // Last trigger timestamp
        uint32_t estimatedReleaseTime;       // Estimated time until full release
        
        uint8_t retriggerCount;              // Number of retriggers
        uint8_t currentGroup;                // Active latch group
        bool isGroupLeader;                  // Is this channel the group leader
        
        ChannelLatchState() :
            isLatched(false),
            isTriggered(false),
            isReleasing(false),
            isAttacking(false),
            currentVelocity(0.0f),
            targetVelocity(0.0f),
            originalVelocity(0.0f),
            envelopePhase(0.0f),
            latchStartTime(0),
            latchDuration(0),
            lastTriggerTime(0),
            estimatedReleaseTime(0),
            retriggerCount(0),
            currentGroup(0),
            isGroupLeader(false) {}
    };
    
    // Latch system performance metrics
    struct LatchMetrics {
        uint32_t totalLatchEvents;           // Total latch triggers processed
        uint32_t totalReleaseEvents;         // Total release events processed
        uint32_t averageLatencyUs;           // Average processing latency
        uint32_t maxLatencyUs;               // Maximum observed latency
        float cpuUsage;                      // CPU usage percentage
        
        std::array<uint32_t, MAX_LATCH_CHANNELS> channelLatchCounts;  // Per-channel counts
        std::array<uint32_t, MAX_LATCH_CHANNELS> channelActiveTimes;  // Time spent latched
        uint32_t longestLatchTimeMs;         // Longest latch duration observed
        uint32_t activeLatchCount;           // Currently active latches
        
        LatchMetrics() :
            totalLatchEvents(0),
            totalReleaseEvents(0),
            averageLatencyUs(0),
            maxLatencyUs(0),
            cpuUsage(0.0f),
            longestLatchTimeMs(0),
            activeLatchCount(0) {
            channelLatchCounts.fill(0);
            channelActiveTimes.fill(0);
        }
    };
    
    // Velocity envelope data
    struct VelocityEnvelope {
        std::vector<float> attackCurve;      // Attack phase curve points
        std::vector<float> releaseCurve;     // Release phase curve points
        uint32_t attackDurationMs;           // Total attack time
        uint32_t releaseDurationMs;          // Total release time
        float sustainLevel;                  // Sustain level (0.0-1.0)
        
        VelocityEnvelope() :
            attackDurationMs(50),
            releaseDurationMs(500),
            sustainLevel(1.0f) {
            // Generate default linear curves
            generateLinearCurves();
        }
        
        void generateLinearCurves() {
            attackCurve = {0.0f, 1.0f};
            releaseCurve = {1.0f, 0.0f};
        }
    };
    
    VelocityLatchSystem();
    ~VelocityLatchSystem();
    
    // System Configuration
    void setSystemConfig(const LatchSystemConfig& config);
    const LatchSystemConfig& getSystemConfig() const { return systemConfig_; }
    void setChannelConfig(uint8_t channelId, const ChannelLatchConfig& config);
    const ChannelLatchConfig& getChannelConfig(uint8_t channelId) const;
    
    // System Control
    bool startLatchSystem();
    bool stopLatchSystem();
    bool pauseLatchSystem();
    bool resumeLatchSystem();
    bool isSystemActive() const { return isActive_.load(); }
    
    // Channel Control
    void enableChannel(uint8_t channelId, LatchMode mode = LatchMode::MOMENTARY);
    void disableChannel(uint8_t channelId);
    bool isChannelEnabled(uint8_t channelId) const;
    std::vector<uint8_t> getActiveChannels() const;
    
    // Latch Operations
    void triggerLatch(uint8_t channelId, float velocity, uint32_t timestampUs = 0);
    void releaseLatch(uint8_t channelId);
    void toggleLatch(uint8_t channelId, float velocity = 0.5f);
    void releaseAllLatches();
    void emergencyStop(); // Immediate stop all latches
    
    // Velocity Processing
    float processVelocity(uint8_t channelId, float inputVelocity, uint32_t timestampUs = 0);
    void updateLatchStates(uint32_t currentTimeUs);
    float calculateEnvelopeOutput(uint8_t channelId, float phase, float velocity) const;
    float applyCrossfade(uint8_t channelId, float fromVelocity, float toVelocity, float phase) const;
    
    // State Management
    const ChannelLatchState& getChannelState(uint8_t channelId) const;
    bool isChannelLatched(uint8_t channelId) const;
    bool isChannelTriggered(uint8_t channelId) const;
    float getCurrentVelocity(uint8_t channelId) const;
    uint32_t getLatchDuration(uint8_t channelId) const;
    
    // Group Management
    void setChannelGroup(uint8_t channelId, uint8_t groupId);
    void triggerGroup(uint8_t groupId, float velocity);
    void releaseGroup(uint8_t groupId);
    std::vector<uint8_t> getGroupChannels(uint8_t groupId) const;
    uint8_t getActiveGroupCount() const;
    
    // Envelope Management
    void setChannelEnvelope(uint8_t channelId, const VelocityEnvelope& envelope);
    const VelocityEnvelope& getChannelEnvelope(uint8_t channelId) const;
    void generateEnvelope(uint8_t channelId, ReleaseMode mode, uint32_t durationMs);
    void resetChannelEnvelope(uint8_t channelId);
    
    // Timing and Sync
    void setTempo(float bpm);
    float getTempo() const { return systemConfig_.tempoBPM; }
    void syncToPatternPosition(uint32_t patternPositionMs);
    uint32_t quantizeToPattern(uint32_t timeMs, uint32_t quantizeValue) const;
    
    // Performance Analysis
    LatchMetrics getCurrentMetrics() const;
    float getChannelActivity(uint8_t channelId) const;
    size_t getEstimatedMemoryUsage() const;
    void resetPerformanceCounters();
    
    // Hardware Integration
    void setHardwareTrigger(uint8_t channelId, uint8_t hardwarePin);
    void configureHardwareButtons();
    void enableHardwareInterrupts();
    void disableHardwareInterrupts();
    bool testHardwareTrigger(uint8_t channelId) const;
    
    // External Integration
    void integrateWithVelocityCapture(class VelocityCaptureSystem* captureSystem);
    void integrateWithSequencer(class SequencerEngine* sequencer);
    void integrateWithMIDI(class MIDIInterface* midiInterface);
    void setExternalTriggerCallback(std::function<void(uint8_t, float)> callback);
    
    // Callbacks
    using LatchTriggerCallback = std::function<void(uint8_t channelId, float velocity, uint32_t timestamp)>;
    using LatchReleaseCallback = std::function<void(uint8_t channelId, uint32_t duration)>;
    using VelocityUpdateCallback = std::function<void(uint8_t channelId, float velocity)>;
    using SystemStatusCallback = std::function<void(bool isActive, const LatchMetrics& metrics)>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    
    void setLatchTriggerCallback(LatchTriggerCallback callback) { latchTriggerCallback_ = callback; }
    void setLatchReleaseCallback(LatchReleaseCallback callback) { latchReleaseCallback_ = callback; }
    void setVelocityUpdateCallback(VelocityUpdateCallback callback) { velocityUpdateCallback_ = callback; }
    void setSystemStatusCallback(SystemStatusCallback callback) { systemStatusCallback_ = callback; }
    void setErrorCallback(ErrorCallback callback) { errorCallback_ = callback; }
    
    // Automation and Recording
    void enableAutomationRecording(bool enable);
    void recordLatchEvent(uint8_t channelId, bool isLatch, float velocity, uint32_t timestamp);
    std::vector<struct LatchAutomationEvent> getRecordedAutomation() const;
    void clearAutomationRecording();

private:
    // Configuration
    LatchSystemConfig systemConfig_;
    std::array<ChannelLatchConfig, MAX_LATCH_CHANNELS> channelConfigs_;
    std::array<VelocityEnvelope, MAX_LATCH_CHANNELS> channelEnvelopes_;
    
    // System state
    std::atomic<bool> isActive_;
    std::atomic<bool> isPaused_;
    std::array<std::atomic<bool>, MAX_LATCH_CHANNELS> channelEnabled_;
    std::array<ChannelLatchState, MAX_LATCH_CHANNELS> channelStates_;
    
    // Performance tracking
    LatchMetrics currentMetrics_;
    uint32_t lastUpdateTime_;
    uint32_t processingStartTime_;
    uint32_t totalProcessingTime_;
    
    // Hardware state
    std::array<uint8_t, MAX_LATCH_CHANNELS> hardwarePins_;
    bool hardwareInterruptsEnabled_;
    
    // Integration
    class VelocityCaptureSystem* velocityCaptureSystem_;
    class SequencerEngine* sequencer_;
    class MIDIInterface* midiInterface_;
    std::function<void(uint8_t, float)> externalTriggerCallback_;
    
    // Callbacks
    LatchTriggerCallback latchTriggerCallback_;
    LatchReleaseCallback latchReleaseCallback_;
    VelocityUpdateCallback velocityUpdateCallback_;
    SystemStatusCallback systemStatusCallback_;
    ErrorCallback errorCallback_;
    
    // Automation recording
    bool automationRecordingEnabled_;
    std::vector<struct LatchAutomationEvent> recordedEvents_;
    
    // Internal processing methods
    void updateChannelLatch(uint8_t channelId, uint32_t currentTimeUs);
    bool shouldTriggerLatch(uint8_t channelId, float velocity) const;
    bool shouldReleaseLatch(uint8_t channelId, uint32_t currentTimeUs) const;
    void processLatchTransition(uint8_t channelId, bool newLatchState, uint32_t currentTimeUs);
    void updateVelocityEnvelope(uint8_t channelId, uint32_t currentTimeUs);
    
    // Group processing
    void updateGroupStates(uint32_t currentTimeUs);
    void processGroupTrigger(uint8_t groupId, uint8_t triggerChannelId, float velocity);
    void processGroupRelease(uint8_t groupId, uint8_t releaseChannelId);
    
    // Envelope generation
    void generateLinearEnvelope(VelocityEnvelope& envelope, uint32_t durationMs) const;
    void generateExponentialEnvelope(VelocityEnvelope& envelope, uint32_t durationMs, float curve) const;
    void generateLogarithmicEnvelope(VelocityEnvelope& envelope, uint32_t durationMs, float curve) const;
    void generateCustomEnvelope(VelocityEnvelope& envelope, const std::vector<float>& points, uint32_t durationMs) const;
    
    // Timing utilities
    uint32_t getCurrentTimeUs() const;
    uint32_t msToUs(uint32_t ms) const { return ms * 1000; }
    uint32_t usToMs(uint32_t us) const { return us / 1000; }
    float calculateTempoMultiplier() const;
    uint32_t beatsToMs(float beats) const;
    
    // Validation helpers
    bool validateChannelId(uint8_t channelId) const;
    bool validateGroupId(uint8_t groupId) const;
    bool validateVelocity(float velocity) const;
    void sanitizeChannelConfig(ChannelLatchConfig& config) const;
    void sanitizeSystemConfig(LatchSystemConfig& config) const;
    
    // Notification helpers
    void notifyLatchTrigger(uint8_t channelId, float velocity, uint32_t timestamp);
    void notifyLatchRelease(uint8_t channelId, uint32_t duration);
    void notifyVelocityUpdate(uint8_t channelId, float velocity);
    void notifySystemStatus();
    void notifyError(const std::string& error);
    
    // Utility methods
    float interpolateEnvelope(const std::vector<float>& curve, float phase) const;
    float applyCurve(float input, uint8_t curveType, float amount) const;
    float crossfade(float a, float b, float phase) const;
    void updatePerformanceMetrics();
    
    // Constants
    static constexpr uint32_t MAX_GROUPS = 8;
    static constexpr uint32_t MAX_ENVELOPE_POINTS = 64;
    static constexpr uint32_t MIN_LATCH_TIME_MS = 1;
    static constexpr uint32_t DEFAULT_UPDATE_INTERVAL_US = 1000;  // 1ms
    static constexpr float MIN_VELOCITY_THRESHOLD = 0.001f;
    static constexpr float MAX_TEMPO_BPM = 300.0f;
    static constexpr float MIN_TEMPO_BPM = 30.0f;
};

// Automation event structure
struct LatchAutomationEvent {
    uint8_t channelId;
    bool isLatchEvent;       // true for latch, false for release
    float velocity;
    uint32_t timestamp;
    
    LatchAutomationEvent() : channelId(0), isLatchEvent(false), velocity(0.0f), timestamp(0) {}
    LatchAutomationEvent(uint8_t ch, bool latch, float vel, uint32_t time) :
        channelId(ch), isLatchEvent(latch), velocity(vel), timestamp(time) {}
};