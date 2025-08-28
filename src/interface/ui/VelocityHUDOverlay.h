#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include "VelocityUIStrings.h"

/**
 * VelocityHUDOverlay - Comprehensive heads-up display for velocity modulation
 * 
 * Provides an intuitive overlay interface for real-time velocity control:
 * - Context-sensitive button layouts that adapt to current synthesis engine
 * - Progressive disclosure: essential controls visible, advanced features in sub-menus
 * - Real-time feedback with status indicators and parameter value displays
 * - Accessibility support with screen reader integration and keyboard navigation
 * - Touch-optimized gesture support with haptic feedback
 * - Performance view with live voice monitoring and CPU usage display
 * 
 * HUD Layout System:
 * - Main strip: Enable, Depth, Curve, Engine select (always visible)
 * - Engine panel: Context-specific controls for current synthesis engine
 * - Status bar: Real-time feedback, voice count, system status
 * - Help overlay: Context-sensitive hints and documentation
 * - Settings panel: Advanced configuration and customization options
 * 
 * Features:
 * - Responsive layout adapting to screen size and orientation
 * - Customizable button assignments and layouts
 * - Visual feedback with color coding and animation
 * - Integration with hardware controls and MIDI controllers
 * - Session state persistence and user preference storage
 */

class VelocityHUDOverlay {
public:
    // HUD display modes for different use cases
    enum class DisplayMode {
        MINIMAL,        // Essential controls only (Enable, Depth)
        STANDARD,       // Full control set with engine panel
        PERFORMANCE,    // Performance-optimized with large controls
        ADVANCED,       // All features including advanced settings
        HELP,          // Help overlay with contextual guidance
        SETTINGS       // Configuration and customization panel
    };
    
    // Button types and their behavior characteristics
    enum class ButtonType {
        TOGGLE,         // On/off toggle (velocity enable)
        MOMENTARY,      // Press and hold (help display)
        SLIDER,         // Continuous value (depth control)
        SELECTOR,       // Multiple choice (curve type, engine select)
        ACTION,         // Single action trigger (preset load, reset)
        INDICATOR       // Read-only status display (voice count, CPU)
    };
    
    // Visual feedback states for buttons and indicators
    enum class VisualState {
        NORMAL,         // Default appearance
        HIGHLIGHTED,    // Mouse hover or touch focus
        ACTIVE,         // Currently pressed or selected
        ENABLED,        // Feature is active (e.g., velocity enabled)
        DISABLED,       // Feature unavailable or inactive
        ERROR,          // Error state requiring attention
        WARNING         // Warning state with advisory information
    };
    
    // HUD button configuration and layout
    struct HUDButton {
        std::string id;                    // Unique button identifier
        std::string stringId;              // UI string identifier from VelocityUIStrings
        ButtonType type;                   // Button interaction behavior
        std::function<void()> action;      // Button press callback
        std::function<std::string()> valueGetter; // Dynamic value display
        std::function<void(float)> valueSetter;   // Slider value callback
        
        // Visual properties
        int x, y;                          // Screen position
        int width, height;                 // Button dimensions
        VisualState state;                 // Current visual state
        bool visible;                      // Button visibility
        float value;                       // Current value (for sliders/indicators)
        float minValue, maxValue;          // Value range (for sliders)
        
        // Accessibility
        std::string ariaLabel;             // Screen reader label
        std::string keyboardShortcut;      // Keyboard accelerator
        bool keyboardFocusable;            // Can receive keyboard focus
        
        HUDButton() : type(ButtonType::ACTION), x(0), y(0), width(100), height(40),
                     state(VisualState::NORMAL), visible(true), value(0.0f),
                     minValue(0.0f), maxValue(1.0f), keyboardFocusable(true) {}
    };
    
    // HUD layout configuration for different screen sizes
    struct LayoutConfig {
        DisplayMode mode;                  // Current display mode
        int screenWidth, screenHeight;     // Available screen space
        int buttonSpacing;                 // Spacing between buttons
        int panelMargin;                   // Margin around panels
        bool showLabels;                   // Show text labels on buttons
        bool showValues;                   // Show current values
        float scale;                       // UI scaling factor
        
        LayoutConfig() : mode(DisplayMode::STANDARD), screenWidth(800), screenHeight(600),
                        buttonSpacing(10), panelMargin(20), showLabels(true),
                        showValues(true), scale(1.0f) {}
    };
    
    // Real-time status information for performance monitoring
    struct PerformanceStatus {
        size_t activeVoices;               // Currently processing voices
        float cpuUsage;                    // Velocity system CPU usage (0-1)
        float memoryUsage;                 // Memory usage in MB
        size_t parametersModulated;        // Active velocity-modulated parameters
        bool systemHealthy;                // Overall system health status
        std::string lastError;             // Most recent error message
        
        PerformanceStatus() : activeVoices(0), cpuUsage(0.0f), memoryUsage(0.0f),
                             parametersModulated(0), systemHealthy(true) {}
    };
    
    VelocityHUDOverlay();
    ~VelocityHUDOverlay() = default;
    
    // HUD lifecycle and display management
    void initialize(int screenWidth, int screenHeight);
    void setDisplayMode(DisplayMode mode);
    DisplayMode getDisplayMode() const { return layoutConfig_.mode; }
    void show();
    void hide();
    bool isVisible() const { return visible_; }
    
    // Layout and visual configuration
    void setLayoutConfig(const LayoutConfig& config);
    const LayoutConfig& getLayoutConfig() const { return layoutConfig_; }
    void updateLayout();
    void setUIScale(float scale);
    void setTheme(const std::string& themeName);
    
    // Button management and interaction
    void addButton(const HUDButton& button);
    void removeButton(const std::string& buttonId);
    HUDButton* getButton(const std::string& buttonId);
    void updateButtonState(const std::string& buttonId, VisualState state);
    void updateButtonValue(const std::string& buttonId, float value);
    void setButtonVisibility(const std::string& buttonId, bool visible);
    
    // Event handling for user interaction
    void handleMouseMove(int x, int y);
    void handleMousePress(int x, int y);
    void handleMouseRelease(int x, int y);
    void handleKeyPress(int keyCode);
    void handleKeyRelease(int keyCode);
    void handleGesture(const std::string& gestureType, float x, float y, float value);
    
    // Real-time status updates
    void updatePerformanceStatus(const PerformanceStatus& status);
    void showStatusMessage(const std::string& message, float displayTime = 2.0f);
    void showErrorMessage(const std::string& error, float displayTime = 5.0f);
    void clearStatusMessages();
    
    // Context-sensitive help system
    void showContextHelp(const std::string& contextId);
    void hideContextHelp();
    bool isContextHelpVisible() const { return contextHelpVisible_; }
    void setHelpContent(const std::string& contextId, const std::string& content);
    
    // Engine-specific HUD adaptation
    void setCurrentEngine(const std::string& engineType);
    void updateEngineControls();
    void showEngineSpecificPanel(bool show);
    
    // Accessibility support
    void setAccessibilityMode(bool enabled);
    void announceToScreenReader(const std::string& message);
    void setKeyboardNavigationEnabled(bool enabled);
    std::vector<std::string> getFocusableElements() const;
    void setKeyboardFocus(const std::string& elementId);
    
    // Customization and preferences
    void saveLayoutPreferences();
    void loadLayoutPreferences();
    void resetToDefaultLayout();
    void setCustomButtonLayout(const std::vector<HUDButton>& buttons);
    
    // Animation and visual effects
    void setAnimationEnabled(bool enabled);
    void animateButtonHighlight(const std::string& buttonId);
    void animateValueChange(const std::string& buttonId, float fromValue, float toValue);
    void pulseErrorState(const std::string& buttonId);
    
    // Performance optimization
    void setUpdateRate(float fps);
    void enableBatchUpdates(bool enabled);
    void optimizeForPerformance(bool lowPowerMode);
    
    // Integration callbacks
    using ButtonCallback = std::function<void(const std::string& buttonId, float value)>;
    using StatusCallback = std::function<void(const std::string& message)>;
    
    void setButtonCallback(ButtonCallback callback) { buttonCallback_ = callback; }
    void setStatusCallback(StatusCallback callback) { statusCallback_ = callback; }
    
private:
    // Core system components
    std::unique_ptr<VelocityUIStrings> uiStrings_;
    LayoutConfig layoutConfig_;
    PerformanceStatus performanceStatus_;
    
    // Button and control management
    std::unordered_map<std::string, HUDButton> buttons_;
    std::vector<std::string> buttonOrder_;           // Layout order
    std::string focusedButton_;                      // Keyboard focus
    std::string hoveredButton_;                      // Mouse hover
    
    // Visual state
    bool visible_;
    bool contextHelpVisible_;
    std::string currentEngine_;
    std::string activeTheme_;
    float uiScale_;
    
    // Status and messaging
    std::vector<std::pair<std::string, float>> statusMessages_; // message, remaining time
    std::string currentContextHelp_;
    std::unordered_map<std::string, std::string> helpContent_;
    
    // Accessibility
    bool accessibilityMode_;
    bool keyboardNavigationEnabled_;
    std::function<void(const std::string&)> screenReaderCallback_;
    
    // Callbacks
    ButtonCallback buttonCallback_;
    StatusCallback statusCallback_;
    
    // Layout calculation methods
    void calculateMainStripLayout();
    void calculateEnginePanelLayout();
    void calculateStatusBarLayout();
    void calculateHelpOverlayLayout();
    void calculateSettingsPanelLayout();
    
    // Button creation helpers
    HUDButton createToggleButton(const std::string& id, const std::string& stringId, 
                                std::function<void()> action);
    HUDButton createSliderButton(const std::string& id, const std::string& stringId,
                                float minVal, float maxVal, std::function<void(float)> setter);
    HUDButton createSelectorButton(const std::string& id, const std::string& stringId,
                                  std::function<void()> action);
    HUDButton createIndicatorButton(const std::string& id, const std::string& stringId,
                                   std::function<std::string()> valueGetter);
    
    // Event processing
    HUDButton* findButtonAtPosition(int x, int y);
    void processButtonPress(HUDButton* button);
    void processSliderDrag(HUDButton* button, int x, int y);
    void updateButtonHover(const std::string& buttonId, bool hover);
    
    // Visual state management
    void updateVisualStates();
    void updateStatusMessageDisplay(float deltaTime);
    void updateAnimations(float deltaTime);
    
    // Engine-specific layouts
    void createVAEngineLayout();
    void createFMEngineLayout();
    void createHarmonicsEngineLayout();
    void createWavetableEngineLayout();
    void createGenericEngineLayout();
    
    // Utility methods
    std::string formatButtonValue(const HUDButton& button) const;
    bool isPointInButton(int x, int y, const HUDButton& button) const;
    void clampButtonValue(HUDButton& button);
    void notifyButtonChange(const std::string& buttonId, float value);
    
    // Constants
    static constexpr float DEFAULT_UPDATE_RATE = 30.0f;  // 30 FPS
    static constexpr int MIN_BUTTON_SIZE = 40;           // Minimum touch target
    static constexpr int DEFAULT_BUTTON_WIDTH = 100;
    static constexpr int DEFAULT_BUTTON_HEIGHT = 40;
    static constexpr float STATUS_MESSAGE_FADE_TIME = 0.5f;
};