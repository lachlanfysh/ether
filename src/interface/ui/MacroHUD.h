#pragma once
#include "../platform/hardware/SmartKnob.h"
#include <array>
#include <functional>
#include <string>

/**
 * MacroHUD - Heads-Up Display for H/T/M Macro Control
 * 
 * Features:
 * - Real-time H/T/M parameter display with visual feedback
 * - Touch buttons: Latch, Edit, Reset with haptic confirmation
 * - SmartKnob integration with rotation gesture detection
 * - Curve visualization for macro parameter mapping
 * - TouchGFX optimized for STM32 H7 GPU acceleration
 * - Context-sensitive help and parameter hints
 */
class MacroHUD {
public:
    enum class MacroParameter {
        HARMONICS,
        TIMBRE,
        MORPH
    };
    
    enum class HUDState {
        DISPLAY,        // Normal parameter display
        LATCH_SELECT,   // Selecting parameters to latch
        EDIT_MODE,      // Editing parameter curves
        RESET_CONFIRM   // Confirming reset operation
    };
    
    enum class TouchButton {
        LATCH,
        EDIT,
        RESET,
        HELP,
        BACK
    };
    
    struct MacroState {
        float harmonics = 0.5f;     // 0.0 to 1.0
        float timbre = 0.5f;        // 0.0 to 1.0
        float morph = 0.5f;         // 0.0 to 1.0
        bool harmonicsLatched = false;
        bool timbreLatched = false;
        bool morphLatched = false;
        MacroParameter activeParam = MacroParameter::HARMONICS;
    };
    
    struct CurveVisualization {
        static constexpr int CURVE_POINTS = 64;
        std::array<float, CURVE_POINTS> inputCurve;
        std::array<float, CURVE_POINTS> outputCurve;
        std::string curveName;
        std::string description;
        bool logarithmic = false;
        bool bipolar = false;
        float inputRange[2] = {0.0f, 1.0f};
        float outputRange[2] = {0.0f, 1.0f};
    };
    
    struct TouchButtonConfig {
        TouchButton button;
        int x, y, width, height;
        std::string label;
        std::string hint;
        bool enabled = true;
        bool highlighted = false;
        float hapticStrength = 0.5f;
    };
    
    using ParameterChangeCallback = std::function<void(MacroParameter param, float value)>;
    using ButtonPressCallback = std::function<void(TouchButton button)>;
    using StateChangeCallback = std::function<void(HUDState newState, HUDState oldState)>;
    
    MacroHUD();
    ~MacroHUD();
    
    // Initialization
    bool initialize(SmartKnob* smartKnob);
    void shutdown();
    
    // Callbacks
    void setParameterChangeCallback(ParameterChangeCallback callback) { paramChangeCallback_ = callback; }
    void setButtonPressCallback(ButtonPressCallback callback) { buttonPressCallback_ = callback; }
    void setStateChangeCallback(StateChangeCallback callback) { stateChangeCallback_ = callback; }
    
    // State management
    void setState(HUDState state);
    HUDState getState() const { return currentState_; }
    
    void setMacroState(const MacroState& state);
    const MacroState& getMacroState() const { return macroState_; }
    
    // Parameter control
    void setParameter(MacroParameter param, float value);
    float getParameter(MacroParameter param) const;
    void setActiveParameter(MacroParameter param);
    MacroParameter getActiveParameter() const { return macroState_.activeParam; }
    
    // Latch control
    void toggleLatch(MacroParameter param);
    void setLatch(MacroParameter param, bool latched);
    bool isLatched(MacroParameter param) const;
    void clearAllLatches();
    
    // Curve visualization
    void setCurveVisualization(MacroParameter param, const CurveVisualization& curve);
    const CurveVisualization& getCurveVisualization(MacroParameter param) const;
    
    // Touch handling
    void handleTouchDown(int x, int y);
    void handleTouchUp(int x, int y);
    void handleTouchMove(int x, int y);
    
    // SmartKnob integration
    void handleRotation(int32_t delta, float velocity, bool inDetent);
    void handleGesture(SmartKnob::GestureType gesture, float parameter);
    
    // Update and rendering
    void update();
    void render();
    
    // Configuration
    void setButtonConfig(TouchButton button, const TouchButtonConfig& config);
    void setHapticFeedback(bool enabled) { hapticFeedbackEnabled_ = enabled; }
    void setAnimationSpeed(float speed) { animationSpeed_ = speed; }
    
    // Context help
    void showParameterHelp(MacroParameter param);
    void hideHelp();
    
private:
    // TouchGFX integration
    struct DisplayContext {
        int screenWidth = 800;
        int screenHeight = 480;
        int dpi = 120;
        bool gpuAccelerated = true;
        
        // Colors (RGB565 format for TouchGFX)
        uint16_t backgroundColor = 0x0000;      // Black
        uint16_t primaryColor = 0x07FF;         // Cyan
        uint16_t secondaryColor = 0xFFE0;       // Yellow
        uint16_t accentColor = 0xF800;          // Red
        uint16_t textColor = 0xFFFF;            // White
        uint16_t gridColor = 0x39E7;            // Dark gray
        uint16_t latchColor = 0x07E0;           // Green
        uint16_t activeColor = 0xFD20;          // Orange
    };
    
    // Animation system
    struct Animation {
        float startValue = 0.0f;
        float endValue = 0.0f;
        float currentValue = 0.0f;
        uint32_t startTime = 0;
        uint32_t duration = 200; // ms
        bool active = false;
        
        void start(float start, float end, uint32_t durationMs = 200);
        float update(uint32_t currentTime);
        bool isComplete() const { return !active; }
    };
    
    // Touch state
    struct TouchState {
        bool touching = false;
        int startX = 0, startY = 0;
        int currentX = 0, currentY = 0;
        uint32_t startTime = 0;
        TouchButton pressedButton = TouchButton::LATCH;
        bool buttonPressed = false;
    };
    
    // Gesture handling
    struct GestureHandler {
        SmartKnob::GestureType lastGesture = SmartKnob::GestureType::NONE;
        uint32_t gestureTime = 0;
        float gestureParameter = 0.0f;
        bool gestureProcessed = false;
    };
    
    // State
    HUDState currentState_ = HUDState::DISPLAY;
    HUDState previousState_ = HUDState::DISPLAY;
    MacroState macroState_;
    
    // Hardware integration
    SmartKnob* smartKnob_ = nullptr;
    bool initialized_ = false;
    
    // Callbacks
    ParameterChangeCallback paramChangeCallback_;
    ButtonPressCallback buttonPressCallback_;
    StateChangeCallback stateChangeCallback_;
    
    // Display and rendering
    DisplayContext display_;
    TouchState touchState_;
    GestureHandler gestureHandler_;
    
    // Button configurations
    std::array<TouchButtonConfig, 5> buttonConfigs_;
    
    // Curve visualizations
    std::array<CurveVisualization, 3> curveVisualizations_;
    
    // Animation states
    std::array<Animation, 3> paramAnimations_;
    Animation stateTransition_;
    
    // Settings
    bool hapticFeedbackEnabled_ = true;
    float animationSpeed_ = 1.0f;
    bool helpVisible_ = false;
    MacroParameter helpParameter_ = MacroParameter::HARMONICS;
    
    // Layout constants
    static constexpr int PARAM_DISPLAY_Y = 50;
    static constexpr int PARAM_DISPLAY_HEIGHT = 200;
    static constexpr int BUTTON_Y = 300;
    static constexpr int BUTTON_HEIGHT = 60;
    static constexpr int BUTTON_SPACING = 10;
    static constexpr int CURVE_DISPLAY_Y = 100;
    static constexpr int CURVE_DISPLAY_HEIGHT = 150;
    
    // Private methods
    void initializeButtons();
    void initializeCurves();
    void updateAnimations();
    void updateGestureHandling();
    
    TouchButton findTouchedButton(int x, int y) const;
    bool isPointInButton(int x, int y, const TouchButtonConfig& button) const;
    
    void handleLatchButton();
    void handleEditButton();
    void handleResetButton();
    void handleHelpButton();
    void handleBackButton();
    
    void renderParameterDisplay();
    void renderTouchButtons();
    void renderCurveVisualization();
    void renderParameterValue(MacroParameter param, int x, int y, int width, int height);
    void renderButton(const TouchButtonConfig& button);
    void renderHelpOverlay();
    
    void triggerHapticFeedback(TouchButton button);
    void animateParameter(MacroParameter param, float newValue);
    void animateStateTransition(HUDState newState);
    
    uint32_t getTimeMs() const;
    float lerp(float a, float b, float t) const;
    uint16_t blendColors(uint16_t color1, uint16_t color2, float blend) const;
    
    // TouchGFX drawing functions
    void drawRectangle(int x, int y, int width, int height, uint16_t color, bool filled = true);
    void drawCircle(int x, int y, int radius, uint16_t color, bool filled = true);
    void drawLine(int x1, int y1, int x2, int y2, uint16_t color, int thickness = 1);
    void drawText(const std::string& text, int x, int y, uint16_t color, int fontSize = 16);
    void drawCurve(const std::array<float, CurveVisualization::CURVE_POINTS>& points, 
                   int x, int y, int width, int height, uint16_t color);
    
    // Parameter mapping helpers
    std::string getParameterName(MacroParameter param) const;
    std::string getParameterDescription(MacroParameter param) const;
    std::string getParameterUnits(MacroParameter param) const;
    std::string formatParameterValue(MacroParameter param, float value) const;
};