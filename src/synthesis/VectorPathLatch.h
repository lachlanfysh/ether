#pragma once
#include "VectorPath.h"
#include <functional>
#include <array>

/**
 * VectorPathLatch - Advanced latch mode system for Vector Path
 * 
 * Features:
 * - Beat-synchronized playback with configurable divisions
 * - Swing timing and groove templates
 * - Variable rate control with tempo sync
 * - Loop point editing and crossfade looping
 * - Recording mode for real-time path capture
 * - Multiple playback modes (forward, reverse, ping-pong, random)
 * - Quantization to musical subdivisions
 * - Dynamic rate modulation and freeze controls
 */
class VectorPathLatch {
public:
    enum class PlaybackMode {
        FORWARD,        // Normal forward playback
        REVERSE,        // Reverse playback
        PING_PONG,      // Bounce between start/end
        RANDOM,         // Random waypoint selection
        PENDULUM,       // Sine-wave position modulation
        FREEZE          // Hold current position
    };
    
    enum class SyncMode {
        FREE_RUNNING,   // No sync, pure rate-based
        BEAT_SYNC,      // Sync to beat divisions
        BAR_SYNC,       // Sync to bar boundaries
        PATTERN_SYNC    // Sync to pattern length
    };
    
    enum class BeatDivision {
        WHOLE_NOTE,     // 1/1 
        HALF_NOTE,      // 1/2
        QUARTER_NOTE,   // 1/4
        EIGHTH_NOTE,    // 1/8
        SIXTEENTH_NOTE, // 1/16
        THIRTY_SECOND,  // 1/32
        DOTTED_QUARTER, // 1/4.
        DOTTED_EIGHTH,  // 1/8.
        TRIPLET_QUARTER,// 1/4T
        TRIPLET_EIGHTH  // 1/8T
    };
    
    enum class RecordMode {
        OFF,            // Not recording
        OVERDUB,        // Add to existing path
        REPLACE,        // Replace existing path
        PUNCH_IN        // Record between markers
    };
    
    struct LatchConfig {
        // Playback settings
        PlaybackMode playbackMode = PlaybackMode::FORWARD;
        SyncMode syncMode = SyncMode::BEAT_SYNC;
        BeatDivision beatDivision = BeatDivision::QUARTER_NOTE;
        
        // Timing
        float baseRate = 1.0f;          // Base playback rate multiplier
        float swingAmount = 0.0f;       // Swing amount (0-1)
        float grooveShift = 0.0f;       // Timing shift (-0.5 to 0.5)
        
        // Loop points
        float loopStart = 0.0f;         // Loop start (0-1)
        float loopEnd = 1.0f;           // Loop end (0-1)
        float crossfadeTime = 0.05f;    // Loop crossfade time
        
        // Modulation
        bool enableRateModulation = false;
        float rateModDepth = 0.5f;      // Rate modulation depth
        float rateModFreq = 1.0f;       // Rate modulation frequency
        
        // Advanced
        bool quantizeStart = true;      // Quantize playback start
        bool quantizeStop = false;      // Quantize playback stop
        float smoothing = 0.1f;         // Position smoothing
        int randomSeed = 12345;         // Seed for random mode
    };
    
    struct TempoInfo {
        float bpm = 120.0f;             // Beats per minute
        int beatsPerBar = 4;            // Time signature numerator
        int beatDivision = 4;           // Time signature denominator
        float currentBeat = 0.0f;       // Current beat position
        bool isPlaying = false;         // Transport state
    };
    
    struct RecordingState {
        RecordMode mode = RecordMode::OFF;
        bool armed = false;             // Ready to record
        bool recording = false;         // Currently recording
        float punchInTime = 0.0f;       // Punch-in point
        float punchOutTime = 1.0f;      // Punch-out point
        float recordingStart = 0.0f;    // Recording start time
        std::vector<VectorPath::Waypoint> recordBuffer; // Recorded waypoints
        float lastRecordTime = 0.0f;    // Last waypoint time
        float minWaypointDistance = 0.01f; // Minimum distance between waypoints
    };
    
    using PlaybackEventCallback = std::function<void(float position, bool looped)>;
    using RecordingEventCallback = std::function<void(RecordMode mode, bool recording)>;
    using BeatSyncCallback = std::function<void(float beat, BeatDivision division)>;
    
    VectorPathLatch();
    ~VectorPathLatch();
    
    // Initialization
    bool initialize(VectorPath* vectorPath);
    void shutdown();
    
    // Configuration
    void setLatchConfig(const LatchConfig& config);
    const LatchConfig& getLatchConfig() const { return config_; }
    
    void setTempoInfo(const TempoInfo& tempo);
    const TempoInfo& getTempoInfo() const { return tempoInfo_; }
    
    // Playback control
    void startLatch();
    void stopLatch();
    void pauseLatch();
    bool isLatched() const { return latched_; }
    bool isPaused() const { return paused_; }
    
    // Transport sync
    void syncToTransport(float beat, bool playing);
    void setQuantizedStart(bool quantize) { config_.quantizeStart = quantize; }
    void setQuantizedStop(bool quantize) { config_.quantizeStop = quantize; }
    
    // Loop control
    void setLoopPoints(float start, float end);
    void getLoopPoints(float& start, float& end) const;
    void nudgeLoopStart(float delta);
    void nudgeLoopEnd(float delta);
    void resetLoopPoints();
    
    // Rate control
    void setPlaybackRate(float rate);
    float getPlaybackRate() const { return effectiveRate_; }
    void setRateModulation(bool enabled, float depth, float frequency);
    
    // Playback modes
    void setPlaybackMode(PlaybackMode mode);
    PlaybackMode getPlaybackMode() const { return config_.playbackMode; }
    
    void setSyncMode(SyncMode mode);
    SyncMode getSyncMode() const { return config_.syncMode; }
    
    void setBeatDivision(BeatDivision division);
    BeatDivision getBeatDivision() const { return config_.beatDivision; }
    
    // Recording
    void setRecordMode(RecordMode mode);
    void armRecording(bool armed);
    void startRecording();
    void stopRecording();
    void commitRecording();
    void discardRecording();
    bool isRecording() const { return recording_.recording; }
    
    void setPunchPoints(float punchIn, float punchOut);
    void clearRecordBuffer();
    
    // Position control
    void setPosition(float position);
    float getPosition() const { return currentPosition_; }
    void jumpToPosition(float position, bool quantized = false);
    
    // Callbacks
    void setPlaybackEventCallback(PlaybackEventCallback callback) { playbackCallback_ = callback; }
    void setRecordingEventCallback(RecordingEventCallback callback) { recordingCallback_ = callback; }
    void setBeatSyncCallback(BeatSyncCallback callback) { beatSyncCallback_ = callback; }
    
    // Update (call from audio thread)
    void update(float deltaTimeMs);
    
    // Preset management
    void savePreset(const std::string& name);
    bool loadPreset(const std::string& name);
    void deletePreset(const std::string& name);
    std::vector<std::string> getPresetNames() const;
    
private:
    // Core state
    VectorPath* vectorPath_ = nullptr;
    bool initialized_ = false;
    
    // Playback state
    bool latched_ = false;
    bool paused_ = false;
    float currentPosition_ = 0.0f;
    float lastPosition_ = 0.0f;
    float playbackDirection_ = 1.0f;    // 1.0 = forward, -1.0 = reverse
    float effectiveRate_ = 1.0f;
    uint32_t latchStartTime_ = 0;
    
    // Configuration
    LatchConfig config_;
    TempoInfo tempoInfo_;
    RecordingState recording_;
    
    // Beat sync state
    float lastBeat_ = 0.0f;
    float nextQuantizedStart_ = 0.0f;
    bool waitingForQuantizedStart_ = false;
    bool waitingForQuantizedStop_ = false;
    
    // Rate modulation
    float rateModPhase_ = 0.0f;
    float rateModValue_ = 0.0f;
    
    // Random playback state
    std::array<int, 32> randomSequence_;
    int randomIndex_ = 0;
    uint32_t randomState_ = 12345;
    
    // Crossfade state
    float crossfadePosition_ = 0.0f;
    bool inCrossfade_ = false;
    VectorPath::Position crossfadeStartPos_;
    VectorPath::Position crossfadeEndPos_;
    
    // Callbacks
    PlaybackEventCallback playbackCallback_;
    RecordingEventCallback recordingCallback_;
    BeatSyncCallback beatSyncCallback_;
    
    // Private methods
    void updatePlaybackPosition(float deltaTimeMs);
    void updateBeatSync(float deltaTimeMs);
    void updateRateModulation(float deltaTimeMs);
    void updateRecording(float deltaTimeMs);
    void updateCrossfade(float deltaTimeMs);
    
    // Beat sync calculations
    float calculateBeatDivisionTime(BeatDivision division) const;
    float calculateNextQuantizedBeat(float currentBeat, BeatDivision division) const;
    bool shouldStartPlayback(float currentBeat) const;
    bool shouldStopPlayback(float currentBeat) const;
    
    // Playback mode handlers
    void updateForwardPlayback(float deltaPosition);
    void updateReversePlayback(float deltaPosition);
    void updatePingPongPlayback(float deltaPosition);
    void updateRandomPlayback(float deltaPosition);
    void updatePendulumPlayback(float deltaTimeMs);
    void updateFreezeMode();
    
    // Loop handling
    bool checkLoopBoundary(float& position, bool& looped);
    void handleLoopCrossfade(float position);
    void startCrossfade(const VectorPath::Position& startPos, const VectorPath::Position& endPos);
    
    // Recording helpers
    void captureWaypoint(const VectorPath::Position& position, float timestamp);
    bool shouldCaptureWaypoint(const VectorPath::Position& position) const;
    void processRecordedWaypoints();
    void quantizeRecordedWaypoints();
    
    // Random sequence generation
    void generateRandomSequence();
    uint32_t nextRandom();
    
    // Swing timing
    float applySwing(float position, float swingAmount) const;
    float calculateGrooveOffset(float position) const;
    
    // Utility functions
    float moduloWrap(float value, float min, float max) const;
    float lerp(float a, float b, float t) const;
    float smoothStep(float edge0, float edge1, float x) const;
    uint32_t getTimeMs() const;
    
    // Beat division helpers
    static float getBeatDivisionValue(BeatDivision division);
    static std::string getBeatDivisionName(BeatDivision division);
    
    // Constants
    static constexpr float CROSSFADE_MIN_TIME = 0.001f;   // 1ms minimum crossfade
    static constexpr float CROSSFADE_MAX_TIME = 0.5f;     // 500ms maximum crossfade
    static constexpr float RATE_MOD_MAX_FREQ = 20.0f;     // Maximum rate modulation frequency
    static constexpr float MIN_RECORD_INTERVAL = 0.01f;   // Minimum time between recorded waypoints
    static constexpr int MAX_RECORDED_WAYPOINTS = 1024;   // Maximum waypoints in record buffer
};