#include "MacroHUD.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

// For STM32 H7 TouchGFX integration
#ifdef STM32H7
#include "touchgfx/hal/HAL.hpp"
#include "touchgfx/Color.hpp"
#include "touchgfx/widgets/Box.hpp"
#include "touchgfx/widgets/TextArea.hpp"
#else
#include <chrono>
#endif

MacroHUD::MacroHUD() {
    // Initialize default state
    macroState_.harmonics = 0.5f;
    macroState_.timbre = 0.5f;
    macroState_.morph = 0.5f;
    macroState_.activeParam = MacroParameter::HARMONICS;
    
    currentState_ = HUDState::DISPLAY;
    
    // Initialize button configurations
    initializeButtons();
    
    // Initialize curve visualizations
    initializeCurves();
    
    // Initialize animations
    for (auto& anim : paramAnimations_) {
        anim.active = false;
    }
    stateTransition_.active = false;
}

MacroHUD::~MacroHUD() {
    shutdown();
}

bool MacroHUD::initialize(SmartKnob* smartKnob) {
    if (initialized_) {
        return true;
    }
    
    smartKnob_ = smartKnob;
    
    if (smartKnob_) {
        // Set up SmartKnob callbacks
        smartKnob_->setRotationCallback([this](int32_t delta, float velocity, bool inDetent) {
            handleRotation(delta, velocity, inDetent);
        });
        
        smartKnob_->setGestureCallback([this](SmartKnob::GestureType gesture, float parameter) {
            handleGesture(gesture, parameter);
        });
    }
    
#ifdef STM32H7
    // Initialize TouchGFX if not already done
    // This would typically be handled by the main application
#endif
    
    initialized_ = true;
    return true;
}

void MacroHUD::shutdown() {
    if (!initialized_) {
        return;
    }
    
    smartKnob_ = nullptr;
    initialized_ = false;
}

void MacroHUD::setState(HUDState state) {
    if (state == currentState_) {
        return;
    }
    
    previousState_ = currentState_;
    currentState_ = state;
    
    // Trigger state transition animation
    animateStateTransition(state);
    
    // Call state change callback
    if (stateChangeCallback_) {
        stateChangeCallback_(currentState_, previousState_);
    }
    
    // Configure SmartKnob for new state
    if (smartKnob_) {
        SmartKnob::DetentConfig detentConfig;
        SmartKnob::HapticConfig hapticConfig;
        
        switch (state) {
            case HUDState::DISPLAY:
                detentConfig.mode = SmartKnob::DetentMode::MEDIUM;
                hapticConfig.pattern = SmartKnob::HapticPattern::TICK;
                break;
                
            case HUDState::EDIT_MODE:
                detentConfig.mode = SmartKnob::DetentMode::LIGHT;
                hapticConfig.pattern = SmartKnob::HapticPattern::SPRING;
                break;
                
            case HUDState::LATCH_SELECT:
                detentConfig.mode = SmartKnob::DetentMode::HEAVY;
                hapticConfig.pattern = SmartKnob::HapticPattern::BUMP;
                break;
                
            case HUDState::RESET_CONFIRM:
                detentConfig.mode = SmartKnob::DetentMode::HEAVY;
                hapticConfig.pattern = SmartKnob::HapticPattern::THUD;
                break;
        }
        
        smartKnob_->setDetentConfig(detentConfig);
        smartKnob_->setHapticConfig(hapticConfig);
    }
}

void MacroHUD::setMacroState(const MacroState& state) {
    // Animate parameter changes
    if (state.harmonics != macroState_.harmonics) {
        animateParameter(MacroParameter::HARMONICS, state.harmonics);
    }
    if (state.timbre != macroState_.timbre) {
        animateParameter(MacroParameter::TIMBRE, state.timbre);
    }
    if (state.morph != macroState_.morph) {
        animateParameter(MacroParameter::MORPH, state.morph);
    }
    
    macroState_ = state;
}

void MacroHUD::setParameter(MacroParameter param, float value) {
    value = std::clamp(value, 0.0f, 1.0f);
    
    switch (param) {
        case MacroParameter::HARMONICS:
            if (!macroState_.harmonicsLatched) {
                animateParameter(param, value);
                macroState_.harmonics = value;
            }
            break;
        case MacroParameter::TIMBRE:
            if (!macroState_.timbreLatched) {
                animateParameter(param, value);
                macroState_.timbre = value;
            }
            break;
        case MacroParameter::MORPH:
            if (!macroState_.morphLatched) {
                animateParameter(param, value);
                macroState_.morph = value;
            }
            break;
    }
    
    if (paramChangeCallback_) {
        paramChangeCallback_(param, value);
    }
}

float MacroHUD::getParameter(MacroParameter param) const {
    switch (param) {
        case MacroParameter::HARMONICS: return macroState_.harmonics;
        case MacroParameter::TIMBRE: return macroState_.timbre;
        case MacroParameter::MORPH: return macroState_.morph;
        default: return 0.0f;
    }
}

void MacroHUD::setActiveParameter(MacroParameter param) {
    macroState_.activeParam = param;
    
    // Update SmartKnob position to match active parameter
    if (smartKnob_) {
        float paramValue = getParameter(param);
        int32_t knobPosition = static_cast<int32_t>(paramValue * 16384.0f); // Scale to encoder range
        smartKnob_->setPosition(knobPosition);
    }
}

void MacroHUD::toggleLatch(MacroParameter param) {
    setLatch(param, !isLatched(param));
}

void MacroHUD::setLatch(MacroParameter param, bool latched) {
    switch (param) {
        case MacroParameter::HARMONICS:
            macroState_.harmonicsLatched = latched;
            break;
        case MacroParameter::TIMBRE:
            macroState_.timbreLatched = latched;
            break;
        case MacroParameter::MORPH:
            macroState_.morphLatched = latched;
            break;
    }
    
    // Trigger haptic feedback
    if (smartKnob_ && hapticFeedbackEnabled_) {
        smartKnob_->triggerHaptic(latched ? SmartKnob::HapticPattern::BUMP : SmartKnob::HapticPattern::TICK, 0.7f);
    }
}

bool MacroHUD::isLatched(MacroParameter param) const {
    switch (param) {
        case MacroParameter::HARMONICS: return macroState_.harmonicsLatched;
        case MacroParameter::TIMBRE: return macroState_.timbreLatched;
        case MacroParameter::MORPH: return macroState_.morphLatched;
        default: return false;
    }
}

void MacroHUD::clearAllLatches() {
    macroState_.harmonicsLatched = false;
    macroState_.timbreLatched = false;
    macroState_.morphLatched = false;
    
    if (smartKnob_ && hapticFeedbackEnabled_) {
        smartKnob_->triggerHaptic(SmartKnob::HapticPattern::THUD, 0.5f);
    }
}

void MacroHUD::setCurveVisualization(MacroParameter param, const CurveVisualization& curve) {
    int index = static_cast<int>(param);
    if (index >= 0 && index < 3) {
        curveVisualizations_[index] = curve;
    }
}

const MacroHUD::CurveVisualization& MacroHUD::getCurveVisualization(MacroParameter param) const {
    int index = static_cast<int>(param);
    if (index >= 0 && index < 3) {
        return curveVisualizations_[index];
    }
    return curveVisualizations_[0]; // Fallback
}

void MacroHUD::handleTouchDown(int x, int y) {
    touchState_.touching = true;
    touchState_.startX = x;
    touchState_.startY = y;
    touchState_.currentX = x;
    touchState_.currentY = y;
    touchState_.startTime = getTimeMs();
    touchState_.buttonPressed = false;
    
    // Check if a button was pressed
    TouchButton button = findTouchedButton(x, y);
    if (button != TouchButton::LATCH) { // Using LATCH as "none" value
        touchState_.pressedButton = button;
        touchState_.buttonPressed = true;
        
        // Trigger haptic feedback
        triggerHapticFeedback(button);
        
        // Highlight button
        buttonConfigs_[static_cast<int>(button)].highlighted = true;
    }
}

void MacroHUD::handleTouchUp(int x, int y) {
    if (!touchState_.touching) {
        return;
    }
    
    touchState_.touching = false;
    touchState_.currentX = x;
    touchState_.currentY = y;
    
    // Check if we're still over the same button
    if (touchState_.buttonPressed) {
        TouchButton button = findTouchedButton(x, y);
        if (button == touchState_.pressedButton) {
            // Valid button press
            switch (button) {
                case TouchButton::LATCH:
                    handleLatchButton();
                    break;
                case TouchButton::EDIT:
                    handleEditButton();
                    break;
                case TouchButton::RESET:
                    handleResetButton();
                    break;
                case TouchButton::HELP:
                    handleHelpButton();
                    break;
                case TouchButton::BACK:
                    handleBackButton();
                    break;
            }
            
            if (buttonPressCallback_) {
                buttonPressCallback_(button);
            }
        }
        
        // Remove highlight from all buttons
        for (auto& config : buttonConfigs_) {
            config.highlighted = false;
        }
    }
    
    touchState_.buttonPressed = false;
}

void MacroHUD::handleTouchMove(int x, int y) {
    if (!touchState_.touching) {
        return;
    }
    
    touchState_.currentX = x;
    touchState_.currentY = y;
    
    // Check if we're still over the pressed button
    if (touchState_.buttonPressed) {
        TouchButton button = findTouchedButton(x, y);
        bool stillPressed = (button == touchState_.pressedButton);
        
        buttonConfigs_[static_cast<int>(touchState_.pressedButton)].highlighted = stillPressed;
    }
}

void MacroHUD::handleRotation(int32_t delta, float velocity, bool inDetent) {
    if (currentState_ == HUDState::DISPLAY || currentState_ == HUDState::EDIT_MODE) {
        // Convert encoder delta to parameter change
        float paramDelta = static_cast<float>(delta) / 16384.0f; // Normalize to 0-1 range
        
        // Apply velocity scaling for fine/coarse control
        if (std::abs(velocity) > 2.0f) {
            paramDelta *= 2.0f; // Coarse mode
        } else if (std::abs(velocity) < 0.5f) {
            paramDelta *= 0.1f; // Fine mode
        }
        
        // Update active parameter
        float currentValue = getParameter(macroState_.activeParam);
        float newValue = std::clamp(currentValue + paramDelta, 0.0f, 1.0f);
        setParameter(macroState_.activeParam, newValue);
    } else if (currentState_ == HUDState::LATCH_SELECT) {
        // Cycle through parameters
        if (delta > 0) {
            int paramIndex = static_cast<int>(macroState_.activeParam);
            paramIndex = (paramIndex + 1) % 3;
            macroState_.activeParam = static_cast<MacroParameter>(paramIndex);
        } else if (delta < 0) {
            int paramIndex = static_cast<int>(macroState_.activeParam);
            paramIndex = (paramIndex - 1 + 3) % 3;
            macroState_.activeParam = static_cast<MacroParameter>(paramIndex);
        }
    }
}

void MacroHUD::handleGesture(SmartKnob::GestureType gesture, float parameter) {
    gestureHandler_.lastGesture = gesture;
    gestureHandler_.gestureTime = getTimeMs();
    gestureHandler_.gestureParameter = parameter;
    gestureHandler_.gestureProcessed = false;
}

void MacroHUD::update() {
    if (!initialized_) {
        return;
    }
    
    updateAnimations();
    updateGestureHandling();
}

void MacroHUD::render() {
    if (!initialized_) {
        return;
    }
    
    // Clear background
    drawRectangle(0, 0, display_.screenWidth, display_.screenHeight, display_.backgroundColor);
    
    switch (currentState_) {
        case HUDState::DISPLAY:
            renderParameterDisplay();
            renderTouchButtons();
            break;
            
        case HUDState::EDIT_MODE:
            renderParameterDisplay();
            renderCurveVisualization();
            renderTouchButtons();
            break;
            
        case HUDState::LATCH_SELECT:
            renderParameterDisplay();
            renderTouchButtons();
            break;
            
        case HUDState::RESET_CONFIRM:
            renderParameterDisplay();
            renderTouchButtons();
            break;
    }
    
    if (helpVisible_) {
        renderHelpOverlay();
    }
}

void MacroHUD::setButtonConfig(TouchButton button, const TouchButtonConfig& config) {
    int index = static_cast<int>(button);
    if (index >= 0 && index < 5) {
        buttonConfigs_[index] = config;
    }
}

void MacroHUD::showParameterHelp(MacroParameter param) {
    helpVisible_ = true;
    helpParameter_ = param;
}

void MacroHUD::hideHelp() {
    helpVisible_ = false;
}

// Private methods implementation

void MacroHUD::initializeButtons() {
    int buttonWidth = (display_.screenWidth - 5 * BUTTON_SPACING) / 4;
    
    // Latch button
    buttonConfigs_[0] = {
        TouchButton::LATCH,
        BUTTON_SPACING, BUTTON_Y, buttonWidth, BUTTON_HEIGHT,
        "LATCH", "Toggle parameter latch", true, false, 0.6f
    };
    
    // Edit button
    buttonConfigs_[1] = {
        TouchButton::EDIT,
        BUTTON_SPACING * 2 + buttonWidth, BUTTON_Y, buttonWidth, BUTTON_HEIGHT,
        "EDIT", "Enter edit mode", true, false, 0.5f
    };
    
    // Reset button
    buttonConfigs_[2] = {
        TouchButton::RESET,
        BUTTON_SPACING * 3 + buttonWidth * 2, BUTTON_Y, buttonWidth, BUTTON_HEIGHT,
        "RESET", "Reset parameters", true, false, 0.8f
    };
    
    // Help button
    buttonConfigs_[3] = {
        TouchButton::HELP,
        BUTTON_SPACING * 4 + buttonWidth * 3, BUTTON_Y, buttonWidth, BUTTON_HEIGHT,
        "HELP", "Show help", true, false, 0.3f
    };
    
    // Back button (only visible in certain states)
    buttonConfigs_[4] = {
        TouchButton::BACK,
        display_.screenWidth - buttonWidth - BUTTON_SPACING, 20, buttonWidth, 40,
        "BACK", "Go back", false, false, 0.4f
    };
}

void MacroHUD::initializeCurves() {
    // Initialize default linear curves
    for (auto& curve : curveVisualizations_) {
        for (int i = 0; i < CurveVisualization::CURVE_POINTS; i++) {
            float t = static_cast<float>(i) / (CurveVisualization::CURVE_POINTS - 1);
            curve.inputCurve[i] = t;
            curve.outputCurve[i] = t;
        }
        curve.curveName = "Linear";
        curve.description = "1:1 linear mapping";
        curve.logarithmic = false;
        curve.bipolar = false;
    }
    
    // Set parameter-specific curve names
    curveVisualizations_[0].curveName = "Harmonics Curve";
    curveVisualizations_[1].curveName = "Timbre Curve";
    curveVisualizations_[2].curveName = "Morph Curve";
}

void MacroHUD::updateAnimations() {
    uint32_t currentTime = getTimeMs();
    
    // Update parameter animations
    for (int i = 0; i < 3; i++) {
        if (paramAnimations_[i].active) {
            float value = paramAnimations_[i].update(currentTime);
            
            switch (static_cast<MacroParameter>(i)) {
                case MacroParameter::HARMONICS:
                    macroState_.harmonics = value;
                    break;
                case MacroParameter::TIMBRE:
                    macroState_.timbre = value;
                    break;
                case MacroParameter::MORPH:
                    macroState_.morph = value;
                    break;
            }
        }
    }
    
    // Update state transition animation
    if (stateTransition_.active) {
        stateTransition_.update(currentTime);
    }
}

void MacroHUD::updateGestureHandling() {
    if (gestureHandler_.gestureProcessed) {
        return;
    }
    
    uint32_t currentTime = getTimeMs();
    if (currentTime - gestureHandler_.gestureTime > 50) { // 50ms debounce
        switch (gestureHandler_.lastGesture) {
            case SmartKnob::GestureType::DETENT_DWELL:
                if (currentState_ == HUDState::DISPLAY) {
                    setState(HUDState::LATCH_SELECT);
                }
                break;
                
            case SmartKnob::GestureType::DOUBLE_FLICK:
                if (currentState_ == HUDState::DISPLAY) {
                    setState(HUDState::EDIT_MODE);
                } else if (currentState_ == HUDState::EDIT_MODE) {
                    setState(HUDState::DISPLAY);
                }
                break;
                
            case SmartKnob::GestureType::FINE_MODE:
                // Already handled in rotation callback
                break;
                
            case SmartKnob::GestureType::COARSE_MODE:
                // Already handled in rotation callback
                break;
                
            default:
                break;
        }
        
        gestureHandler_.gestureProcessed = true;
    }
}

MacroHUD::TouchButton MacroHUD::findTouchedButton(int x, int y) const {
    for (int i = 0; i < 5; i++) {
        if (buttonConfigs_[i].enabled && isPointInButton(x, y, buttonConfigs_[i])) {
            return static_cast<TouchButton>(i);
        }
    }
    return TouchButton::LATCH; // Using LATCH as "none" value
}

bool MacroHUD::isPointInButton(int x, int y, const TouchButtonConfig& button) const {
    return (x >= button.x && x < button.x + button.width &&
            y >= button.y && y < button.y + button.height);
}

void MacroHUD::handleLatchButton() {
    if (currentState_ == HUDState::LATCH_SELECT) {
        toggleLatch(macroState_.activeParam);
        setState(HUDState::DISPLAY);
    } else {
        setState(HUDState::LATCH_SELECT);
    }
}

void MacroHUD::handleEditButton() {
    if (currentState_ == HUDState::EDIT_MODE) {
        setState(HUDState::DISPLAY);
    } else {
        setState(HUDState::EDIT_MODE);
    }
}

void MacroHUD::handleResetButton() {
    if (currentState_ == HUDState::RESET_CONFIRM) {
        // Actually reset parameters
        clearAllLatches();
        setParameter(MacroParameter::HARMONICS, 0.5f);
        setParameter(MacroParameter::TIMBRE, 0.5f);
        setParameter(MacroParameter::MORPH, 0.5f);
        setState(HUDState::DISPLAY);
    } else {
        setState(HUDState::RESET_CONFIRM);
    }
}

void MacroHUD::handleHelpButton() {
    if (helpVisible_) {
        hideHelp();
    } else {
        showParameterHelp(macroState_.activeParam);
    }
}

void MacroHUD::handleBackButton() {
    setState(HUDState::DISPLAY);
}

void MacroHUD::renderParameterDisplay() {
    int paramWidth = display_.screenWidth / 3;
    int paramHeight = PARAM_DISPLAY_HEIGHT;
    
    // Render each parameter
    for (int i = 0; i < 3; i++) {
        MacroParameter param = static_cast<MacroParameter>(i);
        int x = i * paramWidth;
        int y = PARAM_DISPLAY_Y;
        
        renderParameterValue(param, x, y, paramWidth, paramHeight);
    }
}

void MacroHUD::renderParameterValue(MacroParameter param, int x, int y, int width, int height) {
    float value = getParameter(param);
    bool isActive = (param == macroState_.activeParam);
    bool isLatched = this->isLatched(param);
    
    // Choose colors
    uint16_t bgColor = display_.backgroundColor;
    uint16_t borderColor = isActive ? display_.activeColor : display_.gridColor;
    uint16_t fillColor = isLatched ? display_.latchColor : display_.primaryColor;
    uint16_t textColor = display_.textColor;
    
    // Draw border
    drawRectangle(x + 5, y, width - 10, height, borderColor, false);
    
    // Draw value bar
    int barHeight = static_cast<int>(value * (height - 40));
    int barY = y + height - 20 - barHeight;
    drawRectangle(x + 10, barY, width - 20, barHeight, fillColor);
    
    // Draw parameter name
    std::string name = getParameterName(param);
    drawText(name, x + width/2 - name.length() * 4, y + 10, textColor, 16);
    
    // Draw parameter value
    std::string valueStr = formatParameterValue(param, value);
    drawText(valueStr, x + width/2 - valueStr.length() * 3, y + height - 30, textColor, 12);
    
    // Draw latch indicator
    if (isLatched) {
        drawCircle(x + width - 20, y + 20, 8, display_.latchColor);
        drawText("L", x + width - 24, y + 16, display_.backgroundColor, 10);
    }
    
    // Draw active indicator
    if (isActive) {
        drawRectangle(x + 2, y - 3, width - 4, 3, display_.activeColor);
    }
}

void MacroHUD::renderTouchButtons() {
    for (const auto& config : buttonConfigs_) {
        if (config.enabled) {
            renderButton(config);
        }
    }
}

void MacroHUD::renderButton(const TouchButtonConfig& button) {
    uint16_t bgColor = button.highlighted ? display_.accentColor : display_.gridColor;
    uint16_t textColor = display_.textColor;
    
    // Draw button background
    drawRectangle(button.x, button.y, button.width, button.height, bgColor);
    
    // Draw button border
    drawRectangle(button.x, button.y, button.width, button.height, display_.primaryColor, false);
    
    // Draw button text
    int textX = button.x + button.width/2 - button.label.length() * 4;
    int textY = button.y + button.height/2 - 8;
    drawText(button.label, textX, textY, textColor, 16);
}

void MacroHUD::renderCurveVisualization() {
    const CurveVisualization& curve = getCurveVisualization(macroState_.activeParam);
    
    int x = 50;
    int y = CURVE_DISPLAY_Y;
    int width = display_.screenWidth - 100;
    int height = CURVE_DISPLAY_HEIGHT;
    
    // Draw background
    drawRectangle(x, y, width, height, display_.backgroundColor);
    drawRectangle(x, y, width, height, display_.gridColor, false);
    
    // Draw grid
    for (int i = 1; i < 4; i++) {
        int gridX = x + i * width / 4;
        int gridY = y + i * height / 4;
        drawLine(gridX, y, gridX, y + height, display_.gridColor);
        drawLine(x, gridY, x + width, gridY, display_.gridColor);
    }
    
    // Draw curve
    drawCurve(curve.outputCurve, x, y, width, height, display_.primaryColor);
    
    // Draw current position
    float currentValue = getParameter(macroState_.activeParam);
    int posX = x + static_cast<int>(currentValue * width);
    int posY = y + height - static_cast<int>(currentValue * height);
    drawCircle(posX, posY, 4, display_.accentColor);
    
    // Draw curve name
    drawText(curve.curveName, x + 10, y - 25, display_.textColor, 14);
}

void MacroHUD::renderHelpOverlay() {
    // Draw semi-transparent overlay
    drawRectangle(0, 0, display_.screenWidth, display_.screenHeight, 0x0000); // Black overlay
    
    // Draw help content
    int helpX = 100;
    int helpY = 100;
    int helpWidth = display_.screenWidth - 200;
    int helpHeight = display_.screenHeight - 200;
    
    drawRectangle(helpX, helpY, helpWidth, helpHeight, display_.backgroundColor);
    drawRectangle(helpX, helpY, helpWidth, helpHeight, display_.primaryColor, false);
    
    std::string paramName = getParameterName(helpParameter_);
    std::string paramDesc = getParameterDescription(helpParameter_);
    
    drawText("Help: " + paramName, helpX + 20, helpY + 20, display_.textColor, 18);
    drawText(paramDesc, helpX + 20, helpY + 50, display_.textColor, 14);
    
    drawText("Touch anywhere to close", helpX + 20, helpY + helpHeight - 40, display_.secondaryColor, 12);
}

void MacroHUD::triggerHapticFeedback(TouchButton button) {
    if (!smartKnob_ || !hapticFeedbackEnabled_) {
        return;
    }
    
    float strength = buttonConfigs_[static_cast<int>(button)].hapticStrength;
    smartKnob_->triggerHaptic(SmartKnob::HapticPattern::TICK, strength);
}

void MacroHUD::animateParameter(MacroParameter param, float newValue) {
    int index = static_cast<int>(param);
    if (index >= 0 && index < 3) {
        float currentValue = getParameter(param);
        paramAnimations_[index].start(currentValue, newValue, 200);
    }
}

void MacroHUD::animateStateTransition(HUDState newState) {
    stateTransition_.start(0.0f, 1.0f, 300);
}

uint32_t MacroHUD::getTimeMs() const {
#ifdef STM32H7
    return HAL_GetTick();
#else
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
#endif
}

float MacroHUD::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}

uint16_t MacroHUD::blendColors(uint16_t color1, uint16_t color2, float blend) const {
    // Simple color blending for RGB565
    uint8_t r1 = (color1 >> 11) & 0x1F;
    uint8_t g1 = (color1 >> 5) & 0x3F;
    uint8_t b1 = color1 & 0x1F;
    
    uint8_t r2 = (color2 >> 11) & 0x1F;
    uint8_t g2 = (color2 >> 5) & 0x3F;
    uint8_t b2 = color2 & 0x1F;
    
    uint8_t r = static_cast<uint8_t>(lerp(r1, r2, blend));
    uint8_t g = static_cast<uint8_t>(lerp(g1, g2, blend));
    uint8_t b = static_cast<uint8_t>(lerp(b1, b2, blend));
    
    return (r << 11) | (g << 5) | b;
}

// Animation implementation
void MacroHUD::Animation::start(float start, float end, uint32_t durationMs) {
    startValue = start;
    endValue = end;
    currentValue = start;
    startTime = 0; // Will be set by update()
    duration = durationMs;
    active = true;
}

float MacroHUD::Animation::update(uint32_t currentTime) {
    if (!active) {
        return currentValue;
    }
    
    if (startTime == 0) {
        startTime = currentTime;
    }
    
    uint32_t elapsed = currentTime - startTime;
    if (elapsed >= duration) {
        currentValue = endValue;
        active = false;
        return currentValue;
    }
    
    float t = static_cast<float>(elapsed) / static_cast<float>(duration);
    // Smooth easing function
    t = t * t * (3.0f - 2.0f * t);
    
    currentValue = startValue + t * (endValue - startValue);
    return currentValue;
}

// Helper functions
std::string MacroHUD::getParameterName(MacroParameter param) const {
    switch (param) {
        case MacroParameter::HARMONICS: return "HARMONICS";
        case MacroParameter::TIMBRE: return "TIMBRE";
        case MacroParameter::MORPH: return "MORPH";
        default: return "UNKNOWN";
    }
}

std::string MacroHUD::getParameterDescription(MacroParameter param) const {
    switch (param) {
        case MacroParameter::HARMONICS: 
            return "Controls harmonic content and spectral character of the sound";
        case MacroParameter::TIMBRE: 
            return "Adjusts timbral qualities and sonic texture";
        case MacroParameter::MORPH: 
            return "Morphs between different synthesis modes and characteristics";
        default: return "Unknown parameter";
    }
}

std::string MacroHUD::getParameterUnits(MacroParameter param) const {
    return "%"; // All macro parameters are percentage-based
}

std::string MacroHUD::formatParameterValue(MacroParameter param, float value) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << (value * 100.0f) << "%";
    return ss.str();
}

// Placeholder drawing functions (would be implemented with TouchGFX)
void MacroHUD::drawRectangle(int x, int y, int width, int height, uint16_t color, bool filled) {
#ifdef STM32H7
    // TouchGFX implementation would go here
#endif
}

void MacroHUD::drawCircle(int x, int y, int radius, uint16_t color, bool filled) {
#ifdef STM32H7
    // TouchGFX implementation would go here
#endif
}

void MacroHUD::drawLine(int x1, int y1, int x2, int y2, uint16_t color, int thickness) {
#ifdef STM32H7
    // TouchGFX implementation would go here
#endif
}

void MacroHUD::drawText(const std::string& text, int x, int y, uint16_t color, int fontSize) {
#ifdef STM32H7
    // TouchGFX implementation would go here
#endif
}

void MacroHUD::drawCurve(const std::array<float, CurveVisualization::CURVE_POINTS>& points, 
                        int x, int y, int width, int height, uint16_t color) {
#ifdef STM32H7
    // TouchGFX implementation would go here
    for (int i = 1; i < CurveVisualization::CURVE_POINTS; i++) {
        int x1 = x + (i - 1) * width / (CurveVisualization::CURVE_POINTS - 1);
        int y1 = y + height - static_cast<int>(points[i - 1] * height);
        int x2 = x + i * width / (CurveVisualization::CURVE_POINTS - 1);
        int y2 = y + height - static_cast<int>(points[i] * height);
        drawLine(x1, y1, x2, y2, color, 2);
    }
#endif
}