#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <chrono>

/**
 * VelocityCapture - Real-time velocity capture during step programming
 * 
 * Captures velocity input from multiple sources during sequencer step programming:
 * - SmartKnob turn velocity (fast/slow turns → high/low velocity)
 * - Touch pressure on TouchGFX display (pressure → velocity)
 * - External MIDI input velocity (direct MIDI velocity values)
 * - Audio input level analysis (envelope following for audio-to-velocity)
 * - Manual velocity adjustment with visual feedback
 * 
 * Features:
 * - Multi-source velocity input with configurable priority
 * - Real-time velocity preview and feedback
 * - Velocity curve mapping and scaling
 * - Historical velocity averaging for consistent input
 * - Integration with step programming workflow
 */
class VelocityCapture {
public:
    // Velocity input sources
    enum class VelocitySource {
        HALL_EFFECT_KEYS,   // Hall effect velocity-sensitive keys
        SMARTKNOB_TURN,     // SmartKnob rotation velocity
        TOUCH_PRESSURE,     // TouchGFX pressure sensitivity
        MIDI_INPUT,         // External MIDI velocity
        AUDIO_INPUT,        // Audio envelope follower
        MANUAL_ADJUST,      // Manual velocity knob/slider
        STEP_REPEAT,        // Velocity from step repetition timing
        NONE                // No active source
    };
    
    // Velocity capture configuration
    struct CaptureConfig {
        VelocitySource primarySource;      // Primary velocity source
        VelocitySource secondarySource;    // Fallback source
        float sensitivityScale;            // Global sensitivity (0.1-2.0)
        float velocityCurve;               // Velocity curve (0.5=exponential, 1.0=linear, 2.0=logarithmic)
        uint8_t minVelocity;               // Minimum captured velocity (1-127)
        uint8_t maxVelocity;               // Maximum captured velocity (1-127)
        float smoothingAmount;             // Velocity smoothing (0.0-1.0)
        int historyLength;                 // History averaging length (1-16)
        bool enablePreview;                // Enable velocity preview feedback
        
        // Default configuration
        CaptureConfig() :
            primarySource(VelocitySource::HALL_EFFECT_KEYS),
            secondarySource(VelocitySource::SMARTKNOB_TURN),
            sensitivityScale(1.0f),
            velocityCurve(1.0f),
            minVelocity(10),
            maxVelocity(127),
            smoothingAmount(0.2f),
            historyLength(4),
            enablePreview(true) {}
    };
    
    // Velocity capture event
    struct VelocityCaptureEvent {
        uint8_t velocity;                   // Captured velocity (1-127)
        VelocitySource source;              // Source of velocity
        float rawValue;                     // Raw input value (0.0-1.0)
        float scaledValue;                  // Scaled/curved value (0.0-1.0)
        std::chrono::steady_clock::time_point timestamp;
        
        VelocityCaptureEvent() :
            velocity(100),
            source(VelocitySource::NONE),
            rawValue(0.0f),
            scaledValue(0.0f),
            timestamp(std::chrono::steady_clock::now()) {}
    };
    
    // Velocity preview callback type
    using VelocityPreviewCallback = std::function<void(uint8_t velocity, VelocitySource source)>;
    
    VelocityCapture();
    ~VelocityCapture() = default;
    
    // Configuration
    void setConfig(const CaptureConfig& config);
    const CaptureConfig& getConfig() const { return config_; }
    void setPrimarySource(VelocitySource source);
    void setSecondarySource(VelocitySource source);
    void setSensitivity(float scale);
    void setVelocityCurve(float curve);
    void setVelocityRange(uint8_t minVel, uint8_t maxVel);
    
    // Velocity input from various sources
    void updateHallEffectVelocity(float keyVelocity);           // Hall effect key strike velocity (0.0-1.0)
    void updateSmartKnobVelocity(float rotationSpeed);          // Rotation speed in radians/second
    void updateTouchPressure(float pressure, bool touching);    // Pressure 0.0-1.0, touch state
    void updateMidiVelocity(uint8_t velocity);                  // Direct MIDI velocity input
    void updateAudioLevel(float level);                         // Audio input level (0.0-1.0)
    void updateManualVelocity(float normalizedVelocity);        // Manual control (0.0-1.0)
    void updateStepRepeatTiming(float repeatRate);              // Step repeat rate for velocity
    
    // Velocity capture and programming
    uint8_t captureVelocity();                                  // Capture current velocity
    uint8_t captureVelocityFromSource(VelocitySource source);   // Capture from specific source
    void startVelocityCapture();                                // Begin capture session
    void stopVelocityCapture();                                 // End capture session
    bool isCapturing() const { return isCapturing_; }
    
    // Velocity preview and feedback
    void setPreviewCallback(VelocityPreviewCallback callback);
    void enablePreview(bool enabled);
    void triggerPreview(uint8_t velocity, VelocitySource source);
    
    // Velocity history and analysis
    uint8_t getLastVelocity() const;
    uint8_t getAverageVelocity(int samples = 0) const;          // 0 = use config history length
    VelocitySource getLastVelocitySource() const;
    const std::vector<VelocityCaptureEvent>& getVelocityHistory() const;
    void clearVelocityHistory();
    
    // Real-time velocity monitoring
    uint8_t getCurrentVelocity() const;                         // Current velocity from active source
    VelocitySource getActiveSource() const;                     // Currently active source
    float getSourceValue(VelocitySource source) const;         // Raw value from specific source
    bool isSourceActive(VelocitySource source) const;          // Check if source has recent activity
    
    // Velocity curve utilities
    static float applyCurve(float input, float curve);         // Apply velocity curve to input
    static float linearToExponential(float input, float strength = 2.0f);
    static float linearToLogarithmic(float input, float strength = 0.5f);
    static uint8_t scaleToVelocityRange(float normalized, uint8_t minVel, uint8_t maxVel);
    
    // Reset and calibration
    void reset();
    void calibrateSources();                                    // Calibrate all input sources
    void calibrateSource(VelocitySource source);               // Calibrate specific source
    
private:
    CaptureConfig config_;
    bool isCapturing_;
    VelocityPreviewCallback previewCallback_;
    
    // Current source values
    float hallEffectVelocity_;
    float smartKnobVelocity_;
    float touchPressure_;
    uint8_t midiVelocity_;
    float audioLevel_;
    float manualVelocity_;
    float stepRepeatRate_;
    
    // Source state tracking
    std::chrono::steady_clock::time_point lastHallEffectTime_;
    std::chrono::steady_clock::time_point lastSmartKnobTime_;
    std::chrono::steady_clock::time_point lastTouchTime_;
    std::chrono::steady_clock::time_point lastMidiTime_;
    std::chrono::steady_clock::time_point lastAudioTime_;
    std::chrono::steady_clock::time_point lastManualTime_;
    bool touchActive_;
    
    // Velocity history
    std::vector<VelocityCaptureEvent> velocityHistory_;
    uint8_t lastCapturedVelocity_;
    VelocitySource lastCapturedSource_;
    
    // Smoothing and filtering
    float smoothedVelocity_;
    
    // Internal processing
    uint8_t processVelocityInput(float rawValue, VelocitySource source);
    float getSourceRawValue(VelocitySource source) const;
    bool isSourceRecentlyActive(VelocitySource source, 
                               std::chrono::milliseconds timeoutMs = std::chrono::milliseconds(500)) const;
    VelocitySource selectActiveSource() const;
    void addToHistory(const VelocityCaptureEvent& event);
    void updateSmoothingFilter(float targetVelocity);
    
    // Utility functions
    float clamp(float value, float min, float max) const;
    uint8_t clampVelocity(uint8_t velocity) const;
    
    // Constants
    static constexpr float DEFAULT_SENSITIVITY = 1.0f;
    static constexpr float MIN_SENSITIVITY = 0.1f;
    static constexpr float MAX_SENSITIVITY = 2.0f;
    static constexpr uint8_t MIN_VELOCITY_VALUE = 1;
    static constexpr uint8_t MAX_VELOCITY_VALUE = 127;
    static constexpr int MAX_HISTORY_LENGTH = 32;
    static constexpr float DEFAULT_SMOOTHING = 0.95f;
};