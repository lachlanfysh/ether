#pragma once
#include <stdint.h>
#include <array>
#include <functional>

/**
 * SmartKnob - High-resolution magnetic encoder with haptic feedback
 * 
 * Features:
 * - 14-bit magnetic encoder (16384 positions per revolution)
 * - Variable detent spacing and strength
 * - Haptic feedback with configurable patterns
 * - Gesture detection (detent dwell, double-flick, fine mode)
 * - Real-time parameter mapping with acceleration curves
 * - STM32 H7 optimized with DMA and interrupts
 */
class SmartKnob {
public:
    enum class DetentMode {
        NONE,           // Smooth continuous rotation
        LIGHT,          // Light detents (24 per revolution)
        MEDIUM,         // Medium detents (12 per revolution)  
        HEAVY,          // Heavy detents (6 per revolution)
        CUSTOM          // User-defined detent spacing
    };
    
    enum class HapticPattern {
        NONE,           // No haptic feedback
        TICK,           // Light tick on detent
        BUMP,           // Medium bump on detent
        THUD,           // Heavy thud on detent
        RAMP_UP,        // Increasing resistance
        RAMP_DOWN,      // Decreasing resistance
        SPRING,         // Spring return to center
        FRICTION        // Velocity-dependent friction
    };
    
    enum class GestureType {
        NONE,
        DETENT_DWELL,   // Hold at detent for 500ms
        DOUBLE_FLICK,   // Quick back-forth motion
        FINE_MODE,      // Slow precise adjustment
        COARSE_MODE,    // Fast parameter changes
        CENTER_PUSH     // Push knob to center
    };
    
    struct HapticConfig {
        HapticPattern pattern = HapticPattern::TICK;
        float strength = 0.5f;          // 0.0 to 1.0 haptic strength
        float frequency = 50.0f;        // Hz for oscillating patterns
        float decay = 0.1f;             // Decay time in seconds
        bool velocityScaling = true;    // Scale with rotation velocity
    };
    
    struct DetentConfig {
        DetentMode mode = DetentMode::MEDIUM;
        uint16_t customSpacing = 683;   // 16384/24 = 683 for 24 detents
        float snapStrength = 0.7f;      // Magnetic snap strength
        float deadZone = 0.1f;          // Dead zone around detent center
        bool asymmetric = false;        // Different resistance in each direction
    };
    
    struct GestureConfig {
        bool detentDwellEnabled = true;
        uint32_t dwellTimeMs = 500;
        bool doubleFlickEnabled = true;
        uint32_t flickThresholdMs = 150;
        bool fineModeEnabled = true;
        float fineModeThreshold = 0.1f; // Velocity threshold for fine mode
        float coarseModeThreshold = 2.0f; // Velocity threshold for coarse mode
    };
    
    using RotationCallback = std::function<void(int32_t delta, float velocity, bool inDetent)>;
    using GestureCallback = std::function<void(GestureType gesture, float parameter)>;
    using HapticCallback = std::function<void(HapticPattern pattern, float strength)>;
    
    SmartKnob();
    ~SmartKnob();
    
    // Initialization and configuration
    bool initialize();
    void shutdown();
    
    void setDetentConfig(const DetentConfig& config);
    void setHapticConfig(const HapticConfig& config);
    void setGestureConfig(const GestureConfig& config);
    
    // Callback registration
    void setRotationCallback(RotationCallback callback) { rotationCallback_ = callback; }
    void setGestureCallback(GestureCallback callback) { gestureCallback_ = callback; }
    void setHapticCallback(HapticCallback callback) { hapticCallback_ = callback; }
    
    // Position and state
    int32_t getPosition() const { return position_; }
    int32_t getDetentPosition() const { return detentPosition_; }
    float getVelocity() const { return velocity_; }
    bool isInDetent() const { return inDetent_; }
    
    void setPosition(int32_t position);
    void resetPosition() { setPosition(0); }
    
    // Real-time processing (called from interrupt/DMA)
    void processEncoderUpdate(uint16_t rawPosition);
    void processHapticUpdate();
    
    // Main update loop (called from main thread)
    void update();
    
    // Haptic feedback control
    void triggerHaptic(HapticPattern pattern, float strength = 1.0f);
    void setHapticForce(float force); // -1.0 to 1.0
    
    // Gesture detection state
    GestureType getCurrentGesture() const { return currentGesture_; }
    bool isGestureActive() const { return gestureActive_; }
    
    // Calibration and diagnostics
    void calibrate();
    bool isCalibrated() const { return calibrated_; }
    float getEncoderHealth() const { return encoderHealth_; }
    
private:
    // Hardware interface
    struct EncoderData {
        uint16_t rawPosition = 0;
        uint16_t lastRawPosition = 0;
        int32_t position = 0;
        int32_t lastPosition = 0;
        uint32_t timestamp = 0;
        uint32_t lastTimestamp = 0;
        bool dataReady = false;
    };
    
    struct HapticData {
        float currentForce = 0.0f;
        float targetForce = 0.0f;
        HapticPattern activePattern = HapticPattern::NONE;
        float patternPhase = 0.0f;
        uint32_t patternStartTime = 0;
        float decay = 0.0f;
    };
    
    // Gesture detection state
    struct GestureState {
        GestureType current = GestureType::NONE;
        GestureType previous = GestureType::NONE;
        bool active = false;
        uint32_t startTime = 0;
        uint32_t lastMotionTime = 0;
        float startPosition = 0.0f;
        float peakVelocity = 0.0f;
        int32_t motionDirection = 0;
        bool detentDwellActive = false;
        uint32_t detentDwellStart = 0;
        bool doubleFlickState = false;
        uint32_t lastFlickTime = 0;
    };
    
    // Velocity calculation
    struct VelocityTracker {
        static constexpr int HISTORY_SIZE = 8;
        std::array<float, HISTORY_SIZE> velocityHistory;
        std::array<uint32_t, HISTORY_SIZE> timeHistory;
        int historyIndex = 0;
        float smoothedVelocity = 0.0f;
        float acceleration = 0.0f;
    };
    
    // Configuration
    DetentConfig detentConfig_;
    HapticConfig hapticConfig_;
    GestureConfig gestureConfig_;
    
    // Callbacks
    RotationCallback rotationCallback_;
    GestureCallback gestureCallback_;
    HapticCallback hapticCallback_;
    
    // State
    EncoderData encoder_;
    HapticData haptic_;
    GestureState gesture_;
    VelocityTracker velocity_;
    
    int32_t position_ = 0;
    int32_t detentPosition_ = 0;
    float velocity_ = 0.0f;
    bool inDetent_ = false;
    bool calibrated_ = false;
    float encoderHealth_ = 1.0f;
    GestureType currentGesture_ = GestureType::NONE;
    bool gestureActive_ = false;
    
    // Hardware-specific
    uint32_t sampleRate_ = 1000; // 1kHz update rate
    bool initialized_ = false;
    
    // Private methods
    void updatePosition();
    void updateVelocity();
    void updateDetentState();
    void updateGestureDetection();
    void updateHapticFeedback();
    
    int32_t calculateDetentPosition(int32_t rawPosition) const;
    float calculateDetentForce(int32_t position, int32_t detentPos) const;
    float calculateHapticForce() const;
    
    void detectDetentDwell();
    void detectDoubleFlick();
    void detectFineCoarseMode();
    
    uint32_t getTimeMs() const;
    float exponentialSmooth(float current, float target, float alpha) const;
    
    // Hardware abstraction layer
    bool initializeEncoder();
    bool initializeHaptic();
    void shutdownEncoder();
    void shutdownHaptic();
    
    uint16_t readEncoderRaw();
    void writeHapticForce(float force);
    
    // Interrupt handlers (declared as static for C linkage)
    static void encoderInterruptHandler();
    static void hapticTimerHandler();
    static SmartKnob* instance_; // For interrupt access
};