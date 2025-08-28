#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <unordered_map>
#include <chrono>

/**
 * GestureSettings - Advanced touch gesture system for expressive control
 * 
 * Provides sophisticated gesture recognition for musical expression:
 * - Detent Dwell: Hold at specific values for quantized parameter control
 * - Double-Flick: Quick successive touches for rapid parameter changes  
 * - Fine Adjust: Precision control mode with reduced sensitivity
 * - Velocity Gestures: Touch velocity affects parameter change rate
 * - Multi-Touch: Simultaneous control of multiple parameters
 * - Pressure Sensitivity: Variable pressure for continuous control (if supported)
 * 
 * Key Features:
 * - Configurable sensitivity and timing parameters
 * - Per-parameter gesture customization  
 * - Musical context awareness (quantized vs continuous parameters)
 * - Haptic feedback integration for tactile response
 * - Accessibility support with alternative input methods
 * - Performance optimization for real-time audio processing
 * 
 * Gesture Types:
 * - TAP: Single touch for toggle or trigger actions
 * - HOLD: Sustained touch for continuous parameter changes
 * - DRAG: Linear movement for slider-style control
 * - FLICK: Quick movement for rapid value changes
 * - PINCH: Two-finger scaling for range adjustments
 * - ROTATE: Rotational movement for circular controls
 * - MULTI_TOUCH: Multiple simultaneous touch points
 */

class GestureSettings {
public:
    // Gesture recognition types with musical applications
    enum class GestureType {
        TAP,            // Single tap: toggle velocity enable, select preset
        DOUBLE_TAP,     // Double tap: reset parameter, enter edit mode
        HOLD,           // Touch and hold: show context menu, fine adjust mode
        DRAG,           // Linear drag: adjust depth, curve amount
        FLICK,          // Quick flick: rapid parameter change, preset navigation
        DOUBLE_FLICK,   // Two quick flicks: jump to extreme values
        PINCH,          // Pinch/spread: zoom UI, adjust parameter range
        ROTATE,         // Rotation: circular controls, filter cutoff
        MULTI_TOUCH,    // Multiple fingers: simultaneous parameter control
        PRESSURE,       // Variable pressure: continuous control depth
        VELOCITY_TOUCH  // Touch velocity: affects change rate
    };
    
    // Parameter control modes for different musical contexts
    enum class ControlMode {
        CONTINUOUS,     // Smooth continuous values (filter cutoff, depth)
        STEPPED,        // Discrete steps (curve type, engine select)
        QUANTIZED,      // Musical quantization (note values, tempo sync)
        BIPOLAR,        // Positive/negative around center (detune, pan)
        LOGARITHMIC,    // Non-linear response (frequency, time)
        CUSTOM         // User-defined response curve
    };
    
    // Detent behavior for parameter quantization
    enum class DetentBehavior {
        NONE,           // No detents, smooth continuous control
        SOFT,           // Gentle pull toward detent values
        HARD,           // Strong snap to exact detent positions
        MUSICAL,        // Musically relevant positions (octaves, fifths)
        USER_DEFINED    // Custom detent positions
    };
    
    // Haptic feedback types for tactile response
    enum class HapticFeedback {
        NONE,           // No haptic feedback
        LIGHT,          // Subtle vibration for parameter changes
        MEDIUM,         // Moderate feedback for detents, limits
        STRONG,         // Strong feedback for errors, confirmations
        MUSICAL,        // Rhythm-synchronized feedback patterns
        CUSTOM          // User-programmed feedback patterns
    };
    
    // Comprehensive gesture configuration
    struct GestureConfig {
        // Basic gesture parameters
        float sensitivity;              // Overall gesture sensitivity (0.1-5.0)
        float deadZone;                 // Minimum movement to register (pixels)
        float velocity_threshold;       // Minimum velocity for flick detection
        float pressure_threshold;       // Pressure activation threshold (0-1)
        
        // Timing parameters  
        uint32_t tap_timeout;           // Max time for tap detection (ms)
        uint32_t double_tap_window;     // Time window for double tap (ms)
        uint32_t hold_delay;            // Time before hold activation (ms)
        uint32_t flick_timeout;         // Max time for flick gesture (ms)
        uint32_t dwell_time;            // Time to activate detent dwell (ms)
        
        // Advanced gesture features
        bool enable_detent_dwell;       // Enable detent dwell behavior
        DetentBehavior detent_mode;     // Type of detent behavior
        std::vector<float> detent_positions; // Specific detent positions (0-1)
        float detent_strength;          // Pull strength toward detents (0-1)
        float detent_width;             // Detent capture zone width (0-1)
        
        bool enable_double_flick;       // Enable double-flick gestures
        float double_flick_sensitivity; // Sensitivity for double-flick detection
        float double_flick_jump_amount; // How much to jump on double-flick (0-1)
        
        bool enable_fine_adjust;        // Enable fine adjustment mode
        float fine_adjust_ratio;        // Sensitivity reduction in fine mode (0-1)
        GestureType fine_adjust_trigger; // Gesture to enter fine mode
        
        bool enable_velocity_touch;     // Touch velocity affects parameters
        float touch_velocity_scale;     // Scale factor for touch velocity (0-5)
        
        // Multi-touch configuration
        bool enable_multi_touch;        // Allow simultaneous parameter control
        uint8_t max_touch_points;       // Maximum simultaneous touches (1-10)
        float multi_touch_separation;   // Minimum separation between touches
        
        // Haptic feedback
        HapticFeedback haptic_mode;     // Type of haptic feedback
        float haptic_intensity;         // Feedback strength (0-1)
        
        // Accessibility
        bool large_gesture_mode;        // Larger gesture targets
        bool sticky_drag_mode;          // Maintain drag without continuous contact
        float accessibility_multiplier; // Overall sensitivity adjustment
        
        GestureConfig() :
            sensitivity(1.0f), deadZone(5.0f), velocity_threshold(100.0f), pressure_threshold(0.3f),
            tap_timeout(150), double_tap_window(300), hold_delay(500), flick_timeout(200), dwell_time(300),
            enable_detent_dwell(true), detent_mode(DetentBehavior::SOFT), detent_strength(0.3f), detent_width(0.05f),
            enable_double_flick(true), double_flick_sensitivity(1.5f), double_flick_jump_amount(0.5f),
            enable_fine_adjust(true), fine_adjust_ratio(0.1f), fine_adjust_trigger(GestureType::HOLD),
            enable_velocity_touch(true), touch_velocity_scale(1.0f),
            enable_multi_touch(false), max_touch_points(2), multi_touch_separation(50.0f),
            haptic_mode(HapticFeedback::LIGHT), haptic_intensity(0.5f),
            large_gesture_mode(false), sticky_drag_mode(false), accessibility_multiplier(1.0f) {}
    };
    
    // Per-parameter gesture customization
    struct ParameterGestureConfig {
        std::string parameterId;        // Parameter identifier
        ControlMode controlMode;        // Control behavior mode
        GestureConfig gestureConfig;    // Parameter-specific gesture settings
        
        // Parameter-specific settings
        float min_value;                // Parameter minimum value
        float max_value;                // Parameter maximum value
        float default_value;            // Default/reset value
        float step_size;                // Discrete step size (for stepped mode)
        
        // Visual feedback
        bool show_value_display;        // Show numeric value during adjustment
        bool show_parameter_name;       // Show parameter name during adjustment
        std::string units;              // Value units for display (%, Hz, dB)
        
        ParameterGestureConfig() :
            controlMode(ControlMode::CONTINUOUS),
            min_value(0.0f), max_value(1.0f), default_value(0.5f), step_size(0.01f),
            show_value_display(true), show_parameter_name(false), units("") {}
    };
    
    // Touch point tracking for gesture recognition
    struct TouchPoint {
        uint32_t id;                    // Unique touch identifier
        float x, y;                     // Screen coordinates
        float pressure;                 // Touch pressure (0-1, if available)
        float velocity_x, velocity_y;   // Touch velocity (pixels/second)
        std::chrono::steady_clock::time_point start_time; // Touch start time
        std::chrono::steady_clock::time_point last_time;  // Last update time
        bool active;                    // Touch currently active
        
        TouchPoint() : id(0), x(0), y(0), pressure(0), velocity_x(0), velocity_y(0), active(false) {}
    };
    
    // Gesture recognition result
    struct GestureResult {
        GestureType type;               // Recognized gesture type
        std::string parameterId;        // Target parameter (if any)
        float value;                    // Gesture value (normalized 0-1)
        float delta;                    // Change from previous value
        float velocity;                 // Gesture velocity
        bool completed;                 // Gesture completed (vs in progress)
        bool triggered_detent;          // Hit a detent position
        bool fine_adjust_active;        // Fine adjustment mode active
        
        GestureResult() : type(GestureType::TAP), value(0), delta(0), velocity(0),
                         completed(false), triggered_detent(false), fine_adjust_active(false) {}
    };
    
    GestureSettings();
    ~GestureSettings() = default;
    
    // Configuration management
    void setGlobalGestureConfig(const GestureConfig& config);
    const GestureConfig& getGlobalGestureConfig() const { return globalConfig_; }
    
    void setParameterGestureConfig(const std::string& parameterId, const ParameterGestureConfig& config);
    const ParameterGestureConfig& getParameterGestureConfig(const std::string& parameterId) const;
    bool hasParameterConfig(const std::string& parameterId) const;
    void removeParameterConfig(const std::string& parameterId);
    
    // Touch input processing
    void touchDown(uint32_t touchId, float x, float y, float pressure = 0.0f);
    void touchMove(uint32_t touchId, float x, float y, float pressure = 0.0f);
    void touchUp(uint32_t touchId, float x, float y);
    void cancelAllTouches();
    
    // Gesture recognition and processing
    std::vector<GestureResult> processGestures(float deltaTime);
    GestureResult processParameterGesture(const std::string& parameterId, 
                                         const TouchPoint& touch, float deltaTime);
    
    // Detent system
    void addDetentPosition(const std::string& parameterId, float position);
    void removeDetentPosition(const std::string& parameterId, float position);
    void clearDetentPositions(const std::string& parameterId);
    std::vector<float> getDetentPositions(const std::string& parameterId) const;
    float findNearestDetent(const std::string& parameterId, float value) const;
    bool isNearDetent(const std::string& parameterId, float value, float tolerance) const;
    
    // Fine adjustment mode
    void enterFineAdjustMode(const std::string& parameterId);
    void exitFineAdjustMode(const std::string& parameterId);
    bool isFineAdjustActive(const std::string& parameterId) const;
    float applyFineAdjustment(const std::string& parameterId, float delta) const;
    
    // Multi-touch management
    void setMultiTouchEnabled(bool enabled);
    bool isMultiTouchEnabled() const { return globalConfig_.enable_multi_touch; }
    size_t getActiveTouchCount() const;
    std::vector<TouchPoint> getActiveTouches() const;
    
    // Haptic feedback
    void triggerHapticFeedback(HapticFeedback type, float intensity = 1.0f);
    void setHapticEnabled(bool enabled);
    bool isHapticEnabled() const;
    
    // Accessibility features
    void setAccessibilityMode(bool enabled);
    void setLargeGestureMode(bool enabled);
    void setStickyDragMode(bool enabled);
    void setAccessibilitySensitivity(float multiplier);
    
    // Calibration and customization
    void calibrateGestureSensitivity();
    void resetToDefaults();
    void saveUserPreferences();
    void loadUserPreferences();
    
    // Performance optimization
    void setUpdateRate(float fps);
    void enableBatchProcessing(bool enabled);
    void optimizeForLowLatency(bool enabled);
    
    // Integration callbacks
    using GestureCallback = std::function<void(const GestureResult& result)>;
    using HapticCallback = std::function<void(HapticFeedback type, float intensity)>;
    
    void setGestureCallback(GestureCallback callback) { gestureCallback_ = callback; }
    void setHapticCallback(HapticCallback callback) { hapticCallback_ = callback; }
    
private:
    // Configuration storage
    GestureConfig globalConfig_;
    std::unordered_map<std::string, ParameterGestureConfig> parameterConfigs_;
    ParameterGestureConfig defaultParameterConfig_;
    
    // Touch tracking
    std::unordered_map<uint32_t, TouchPoint> activeTouches_;
    std::vector<TouchPoint> touchHistory_;
    uint32_t nextTouchId_;
    
    // Gesture recognition state
    std::unordered_map<std::string, bool> fineAdjustActive_;
    std::unordered_map<uint32_t, GestureType> activeGestures_;
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> gestureStartTimes_;
    
    // Detent positions per parameter
    std::unordered_map<std::string, std::vector<float>> detentPositions_;
    
    // System state
    bool enabled_;
    bool hapticEnabled_;
    bool accessibilityMode_;
    float updateRate_;
    
    // Callbacks
    GestureCallback gestureCallback_;
    HapticCallback hapticCallback_;
    
    // Gesture recognition methods
    GestureType recognizeGesture(const TouchPoint& touch) const;
    bool isTapGesture(const TouchPoint& touch) const;
    bool isHoldGesture(const TouchPoint& touch) const;
    bool isDragGesture(const TouchPoint& touch) const;
    bool isFlickGesture(const TouchPoint& touch) const;
    bool isDoubleFlickGesture(uint32_t touchId) const;
    bool isPinchGesture(const std::vector<TouchPoint>& touches) const;
    
    // Parameter value processing
    float processParameterValue(const std::string& parameterId, float rawValue) const;
    float applyControlMode(ControlMode mode, float value, float min, float max, float step) const;
    float applyDetentInfluence(const std::string& parameterId, float value) const;
    float calculateGestureVelocity(const TouchPoint& touch) const;
    
    // Multi-touch processing
    std::vector<GestureResult> processMultiTouch(float deltaTime);
    bool touchesAreSeparated(const TouchPoint& touch1, const TouchPoint& touch2) const;
    
    // Utility methods
    float distance(float x1, float y1, float x2, float y2) const;
    float calculateTouchVelocity(const TouchPoint& touch) const;
    void updateTouchVelocity(TouchPoint& touch, float x, float y);
    void cleanupInactiveTouches();
    void notifyGestureRecognized(const GestureResult& result);
    
    // Constants
    static constexpr float DEFAULT_SENSITIVITY = 1.0f;
    static constexpr float DEFAULT_DEAD_ZONE = 5.0f;
    static constexpr uint32_t DEFAULT_TAP_TIMEOUT = 150;
    static constexpr uint32_t DEFAULT_DOUBLE_TAP_WINDOW = 300;
    static constexpr uint32_t DEFAULT_HOLD_DELAY = 500;
    static constexpr float DEFAULT_DETENT_STRENGTH = 0.3f;
    static constexpr float DEFAULT_FINE_ADJUST_RATIO = 0.1f;
    static constexpr size_t MAX_TOUCH_HISTORY = 100;
};