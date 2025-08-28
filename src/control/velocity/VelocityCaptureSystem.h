#pragma once
#include <cstdint>
#include <vector>
#include <array>
#include <functional>
#include <memory>
#include <atomic>

/**
 * VelocityCaptureSystem - Advanced velocity capture from multiple input sources
 * 
 * Provides comprehensive velocity-sensitive input processing for the EtherSynth:
 * - Hall effect sensor velocity capture with high precision timing
 * - Multi-source velocity input consolidation and processing
 * - Advanced velocity curve mapping and response shaping
 * - Real-time velocity analysis and adaptive threshold adjustment
 * - Integration with pattern sequencing and sample triggering
 * - Hardware-optimized for STM32 H7 embedded platform with ADC processing
 * 
 * Features:
 * - 16-channel hall effect sensor input processing
 * - Sub-millisecond velocity capture timing with hardware interrupts
 * - Adaptive noise filtering and calibration per channel
 * - Multi-stage velocity processing pipeline with real-time analysis
 * - Configurable velocity curves (linear, exponential, logarithmic, custom)
 * - Cross-channel velocity correlation and ghost note suppression
 * - Integration with MIDI input and external velocity sources
 * - Real-time velocity visualization and monitoring
 */
class VelocityCaptureSystem {
public:
    // Maximum number of velocity input channels
    static constexpr uint8_t MAX_VELOCITY_CHANNELS = 16;
    
    // Velocity input source types
    enum class VelocitySourceType {
        HALL_EFFECT_SENSOR,     // Primary hall effect sensor input
        MIDI_INPUT,             // MIDI velocity data
        EXTERNAL_ANALOG,        // External analog velocity input
        SOFTWARE_TRIGGER,       // Software-generated velocity
        COMPOSITE_SOURCE,       // Combination of multiple sources
        DISABLED                // Channel disabled
    };
    
    // Velocity capture configuration per channel
    struct ChannelConfig {
        VelocitySourceType sourceType;      // Input source type
        uint8_t adcChannel;                 // ADC channel for analog inputs
        float sensitivityMultiplier;        // Sensitivity adjustment (0.1-10.0)
        float noiseFloor;                   // Noise floor threshold (0.0-1.0)
        float maxVelocity;                  // Maximum velocity ceiling (0.0-1.0)
        bool enableAdaptiveThreshold;       // Enable adaptive threshold adjustment
        bool enableNoiseGate;               // Enable noise gate filtering
        uint32_t debounceTimeUs;            // Debounce time in microseconds
        
        // Advanced processing options
        bool enableVelocityCurve;           // Apply velocity curve mapping
        uint8_t velocityCurveType;          // Curve type (0=linear, 1=exp, 2=log, 3=custom)
        float curveAmount;                  // Curve intensity (0.0-2.0)
        bool enableCrossChannelSupression; // Suppress ghost notes from other channels
        float crossChannelThreshold;       // Threshold for cross-channel suppression
        
        ChannelConfig() :
            sourceType(VelocitySourceType::HALL_EFFECT_SENSOR),
            adcChannel(0),
            sensitivityMultiplier(1.0f),
            noiseFloor(0.02f),
            maxVelocity(1.0f),
            enableAdaptiveThreshold(true),
            enableNoiseGate(true),
            debounceTimeUs(1000),
            enableVelocityCurve(false),
            velocityCurveType(0),
            curveAmount(1.0f),
            enableCrossChannelSupression(false),
            crossChannelThreshold(0.1f) {}
    };
    
    // Global capture system configuration
    struct CaptureConfig {
        uint32_t sampleRateHz;              // ADC sampling rate
        uint8_t adcResolution;              // ADC resolution (12, 14, 16 bits)
        bool enableHardwareFiltering;       // Enable hardware anti-aliasing filter
        uint32_t bufferSizeFrames;          // Capture buffer size
        float globalSensitivity;            // Global sensitivity multiplier
        bool enableAutoCalibration;         // Enable automatic calibration
        uint32_t calibrationIntervalMs;     // Calibration update interval
        
        // Processing pipeline settings
        bool enableRealTimeProcessing;      // Enable real-time velocity processing
        uint8_t processingThreadPriority;   // Processing thread priority (1-99)
        uint32_t maxLatencyUs;              // Maximum acceptable latency
        bool enableLatencyCompensation;     // Compensate for processing latency
        
        CaptureConfig() :
            sampleRateHz(48000),
            adcResolution(12),
            enableHardwareFiltering(true),
            bufferSizeFrames(128),
            globalSensitivity(1.0f),
            enableAutoCalibration(true),
            calibrationIntervalMs(5000),
            enableRealTimeProcessing(true),
            processingThreadPriority(80),
            maxLatencyUs(1000),
            enableLatencyCompensation(true) {}
    };
    
    // Velocity capture event data
    struct VelocityEvent {
        uint8_t channelId;                  // Source channel (0-15)
        float velocity;                     // Captured velocity (0.0-1.0)
        uint32_t timestampUs;               // Capture timestamp in microseconds
        VelocitySourceType sourceType;      // Source that generated the event
        float rawValue;                     // Raw sensor value before processing
        float processedValue;               // Value after curve/filtering
        bool isGhostNote;                   // Whether this might be a ghost note
        uint8_t confidenceLevel;            // Confidence in velocity accuracy (0-255)
        
        VelocityEvent() :
            channelId(255),
            velocity(0.0f),
            timestampUs(0),
            sourceType(VelocitySourceType::DISABLED),
            rawValue(0.0f),
            processedValue(0.0f),
            isGhostNote(false),
            confidenceLevel(255) {}
    };
    
    // Real-time velocity analysis data
    struct VelocityAnalysis {
        float averageVelocity;              // Running average velocity
        float peakVelocity;                 // Peak velocity in analysis window
        float velocityVariance;             // Velocity variance/consistency
        uint32_t eventCount;                // Number of events in analysis window
        float dynamicRange;                 // Effective dynamic range being used
        std::array<float, MAX_VELOCITY_CHANNELS> channelActivity; // Per-channel activity
        
        // Performance metrics
        float averageLatencyUs;             // Average processing latency
        float maxLatencyUs;                 // Maximum observed latency
        uint32_t droppedEvents;             // Number of dropped events
        float cpuUsage;                     // CPU usage percentage
        
        VelocityAnalysis() :
            averageVelocity(0.0f),
            peakVelocity(0.0f),
            velocityVariance(0.0f),
            eventCount(0),
            dynamicRange(0.0f),
            averageLatencyUs(0.0f),
            maxLatencyUs(0.0f),
            droppedEvents(0),
            cpuUsage(0.0f) {
            channelActivity.fill(0.0f);
        }
    };
    
    // Calibration data per channel
    struct ChannelCalibration {
        float minRawValue;                  // Minimum observed raw value
        float maxRawValue;                  // Maximum observed raw value
        float optimalSensitivity;           // Automatically determined sensitivity
        float noiseLevel;                   // Measured noise level
        uint32_t calibrationSamples;        // Number of calibration samples
        bool isCalibrated;                  // Whether channel is calibrated
        uint32_t lastCalibrationTime;       // Last calibration timestamp
        
        ChannelCalibration() :
            minRawValue(1.0f),
            maxRawValue(0.0f),
            optimalSensitivity(1.0f),
            noiseLevel(0.01f),
            calibrationSamples(0),
            isCalibrated(false),
            lastCalibrationTime(0) {}
    };
    
    VelocityCaptureSystem();
    ~VelocityCaptureSystem();
    
    // System Configuration
    void setCaptureConfig(const CaptureConfig& config);
    const CaptureConfig& getCaptureConfig() const { return config_; }
    void setChannelConfig(uint8_t channelId, const ChannelConfig& config);
    const ChannelConfig& getChannelConfig(uint8_t channelId) const;
    
    // System Control
    bool startCapture();
    bool stopCapture();
    bool pauseCapture();
    bool resumeCapture();
    bool isCapturing() const { return isCapturing_.load(); }
    
    // Channel Management
    void enableChannel(uint8_t channelId, VelocitySourceType sourceType = VelocitySourceType::HALL_EFFECT_SENSOR);
    void disableChannel(uint8_t channelId);
    bool isChannelEnabled(uint8_t channelId) const;
    std::vector<uint8_t> getEnabledChannels() const;
    
    // Velocity Processing
    void processVelocityInput(uint8_t channelId, float rawValue, uint32_t timestampUs);
    float applyVelocityCurve(float velocity, uint8_t curveType, float curveAmount) const;
    bool detectGhostNote(uint8_t channelId, float velocity, uint32_t timestampUs) const;
    float calculateConfidenceLevel(uint8_t channelId, float velocity, float rawValue) const;
    
    // Calibration
    void startChannelCalibration(uint8_t channelId);
    void stopChannelCalibration(uint8_t channelId);
    bool isChannelCalibrating(uint8_t channelId) const;
    void resetChannelCalibration(uint8_t channelId);
    const ChannelCalibration& getChannelCalibration(uint8_t channelId) const;
    
    // Analysis and Monitoring
    VelocityAnalysis getCurrentAnalysis() const;
    float getChannelActivity(uint8_t channelId) const;
    std::vector<VelocityEvent> getRecentEvents(uint32_t maxEvents = 100) const;
    void resetAnalysis();
    
    // Hardware Integration
    void configureADC();
    void setupHardwareFiltering();
    void enableHardwareInterrupts();
    void disableHardwareInterrupts();
    bool testHardwareConnection(uint8_t channelId) const;
    
    // External Integration
    void integrateWithSequencer(class SequencerEngine* sequencer);
    void integrateWithSampler(class AutoSampleLoader* sampleLoader);
    void integrateWithMIDI(class MIDIInterface* midiInterface);
    void setExternalVelocitySource(std::function<float(uint8_t)> velocityCallback);
    
    // Callbacks
    using VelocityEventCallback = std::function<void(const VelocityEvent& event)>;
    using CalibrationCompleteCallback = std::function<void(uint8_t channelId, const ChannelCalibration& calibration)>;
    using SystemStatusCallback = std::function<void(bool isCapturing, const VelocityAnalysis& analysis)>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    
    void setVelocityEventCallback(VelocityEventCallback callback) { velocityEventCallback_ = callback; }
    void setCalibrationCompleteCallback(CalibrationCompleteCallback callback) { calibrationCompleteCallback_ = callback; }
    void setSystemStatusCallback(SystemStatusCallback callback) { systemStatusCallback_ = callback; }
    void setErrorCallback(ErrorCallback callback) { errorCallback_ = callback; }
    
    // Performance Analysis
    size_t getEstimatedMemoryUsage() const;
    float getAverageProcessingTime() const;
    uint32_t getTotalEventsProcessed() const;
    void resetPerformanceCounters();

private:
    // Configuration
    CaptureConfig config_;
    std::array<ChannelConfig, MAX_VELOCITY_CHANNELS> channelConfigs_;
    std::array<ChannelCalibration, MAX_VELOCITY_CHANNELS> channelCalibrations_;
    
    // System state
    std::atomic<bool> isCapturing_;
    std::atomic<bool> isPaused_;
    std::array<std::atomic<bool>, MAX_VELOCITY_CHANNELS> channelEnabled_;
    std::array<std::atomic<bool>, MAX_VELOCITY_CHANNELS> channelCalibrating_;
    
    // Processing data
    std::vector<VelocityEvent> eventHistory_;
    std::array<uint32_t, MAX_VELOCITY_CHANNELS> lastEventTime_;
    std::array<float, MAX_VELOCITY_CHANNELS> lastVelocity_;
    VelocityAnalysis currentAnalysis_;
    
    // Performance tracking
    uint32_t totalEventsProcessed_;
    uint32_t totalProcessingTime_;
    uint32_t processingStartTime_;
    
    // Hardware state
    bool adcConfigured_;
    bool hardwareFiltersEnabled_;
    bool hardwareInterruptsEnabled_;
    
    // Integration
    class SequencerEngine* sequencer_;
    class AutoSampleLoader* sampleLoader_;
    class MIDIInterface* midiInterface_;
    std::function<float(uint8_t)> externalVelocityCallback_;
    
    // Callbacks
    VelocityEventCallback velocityEventCallback_;
    CalibrationCompleteCallback calibrationCompleteCallback_;
    SystemStatusCallback systemStatusCallback_;
    ErrorCallback errorCallback_;
    
    // Internal processing
    void processRawVelocity(uint8_t channelId, float rawValue, uint32_t timestampUs);
    float applyChannelProcessing(uint8_t channelId, float rawValue) const;
    void updateChannelCalibration(uint8_t channelId, float rawValue);
    void updateAnalysis(const VelocityEvent& event);
    
    // Velocity curve implementations
    float applyLinearCurve(float velocity, float amount) const;
    float applyExponentialCurve(float velocity, float amount) const;
    float applyLogarithmicCurve(float velocity, float amount) const;
    float applyCustomCurve(float velocity, float amount) const;
    
    // Ghost note detection
    bool checkCrossChannelActivity(uint8_t channelId, uint32_t timestampUs) const;
    float calculateChannelCorrelation(uint8_t channel1, uint8_t channel2) const;
    
    // Hardware interface
    float readADCChannel(uint8_t adcChannel) const;
    void configureADCChannel(uint8_t adcChannel, uint8_t resolution) const;
    bool testADCChannel(uint8_t adcChannel) const;
    
    // Validation helpers
    bool validateChannelId(uint8_t channelId) const;
    bool validateVelocityValue(float velocity) const;
    void sanitizeChannelConfig(ChannelConfig& config) const;
    
    // Notification helpers
    void notifyVelocityEvent(const VelocityEvent& event);
    void notifyCalibrationComplete(uint8_t channelId);
    void notifySystemStatus();
    void notifyError(const std::string& error);
    
    // Utility methods
    uint32_t getCurrentTimeMicroseconds() const;
    float calculateMovingAverage(float newValue, float oldAverage, uint32_t sampleCount) const;
    
    // Constants
    static constexpr uint32_t MAX_EVENT_HISTORY = 1000;
    static constexpr uint32_t CALIBRATION_SAMPLES_REQUIRED = 500;
    static constexpr uint32_t ANALYSIS_UPDATE_INTERVAL_US = 10000;  // 10ms
    static constexpr float MIN_VELOCITY_THRESHOLD = 0.001f;
    static constexpr float MAX_CROSS_CHANNEL_TIME_US = 2000.0f;  // 2ms window
};