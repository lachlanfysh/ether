#pragma once
#include <cstdint>
#include <vector>
#include <functional>
#include <memory>
#include "../IVelocityModulationView.h"

/**
 * VelocityModulationUI - V-icon system for latchable velocity modulation
 * 
 * Provides TouchGFX UI components for velocity modulation control:
 * - V-icons next to all modulatable parameters
 * - Visual indication of velocity latch state (off/latched/active)
 * - Touch interaction for toggling velocity modulation
 * - Real-time velocity modulation depth visualization
 * - Parameter-specific velocity scaling and polarity control
 * - Integration with existing parameter UI elements
 * 
 * Features:
 * - Compact V-icon design (16x16 pixels) for space efficiency
 * - Three visual states: inactive, latched, actively modulating
 * - Touch gesture support (tap to toggle, long-press for settings)
 * - Real-time modulation amount display (-200% to +200%)
 * - Color coding for different modulation polarities
 * - Batch velocity assignment across multiple parameters
 */

// Forward declarations for TouchGFX integration
class Container;
class Image;
class TextArea;
class TouchArea;

namespace VelocityModulationUI {

    // Velocity modulation visual states
    enum class VIconState {
        INACTIVE,           // No velocity modulation (grey icon)
        LATCHED,           // Velocity modulation latched (blue icon)
        ACTIVELY_MODULATING // Currently being modulated (green/red icon)
    };
    
    // Velocity modulation polarity
    enum class ModulationPolarity {
        POSITIVE,           // Positive modulation (green)
        NEGATIVE,           // Negative modulation (red)  
        BIPOLAR            // Bipolar modulation (blue)
    };
    
    // V-icon configuration
    struct VIconConfig {
        uint16_t x, y;                  // Screen position
        uint16_t width, height;         // Icon dimensions (default 16x16)
        VIconState state;               // Current icon state
        ModulationPolarity polarity;    // Modulation direction
        float modulationDepth;          // Current modulation depth (-2.0 to +2.0)
        bool showDepthText;             // Show numerical depth display
        bool enabled;                   // Icon is interactive
        
        VIconConfig() :
            x(0), y(0), width(16), height(16),
            state(VIconState::INACTIVE),
            polarity(ModulationPolarity::POSITIVE),
            modulationDepth(0.0f),
            showDepthText(false),
            enabled(true) {}
    };
    
    // Velocity modulation callbacks
    using VIconTapCallback = std::function<void(uint32_t parameterId)>;
    using VIconLongPressCallback = std::function<void(uint32_t parameterId)>;
    using ModulationUpdateCallback = std::function<void(uint32_t parameterId, float depth, ModulationPolarity polarity)>;
    
    /**
     * VIcon - Individual velocity modulation icon component
     */
    class VIcon {
    public:
        VIcon();
        ~VIcon();  // Custom destructor to handle incomplete types
        
        // Configuration
        void setConfig(const VIconConfig& config);
        const VIconConfig& getConfig() const { return config_; }
        void setParameterId(uint32_t parameterId);
        uint32_t getParameterId() const { return parameterId_; }
        
        // Visual state
        void setState(VIconState state);
        void setPolarity(ModulationPolarity polarity);
        void setModulationDepth(float depth);           // -2.0 to +2.0 (-200% to +200%)
        void setEnabled(bool enabled);
        
        VIconState getState() const { return config_.state; }
        ModulationPolarity getPolarity() const { return config_.polarity; }
        float getModulationDepth() const { return config_.modulationDepth; }
        bool isEnabled() const { return config_.enabled; }
        
        // TouchGFX integration
        void setPosition(uint16_t x, uint16_t y);
        void setSize(uint16_t width, uint16_t height);
        void setVisible(bool visible);
        bool isVisible() const { return visible_; }
        
        // Touch interaction
        void setTapCallback(VIconTapCallback callback);
        void setLongPressCallback(VIconLongPressCallback callback);
        bool handleTouch(uint16_t x, uint16_t y, bool pressed);
        
        // Rendering (called by TouchGFX framework)
        void draw(Container* parent);
        void update();                                  // Update visual state
        
    private:
        VIconConfig config_;
        uint32_t parameterId_;
        bool visible_;
        
        // Touch handling
        VIconTapCallback tapCallback_;
        VIconLongPressCallback longPressCallback_;
        bool touchActive_;
        uint32_t touchStartTime_;
        
        // Visual components (TouchGFX objects)
        std::unique_ptr<Image> iconImage_;
        std::unique_ptr<TextArea> depthText_;
        std::unique_ptr<TouchArea> touchArea_;
        
        // Internal methods
        void updateIconVisuals();
        void updateDepthText();
        uint32_t getColorForState() const;              // Get color based on state/polarity
        const char* getIconImagePath() const;           // Get icon image path
    };
    
    /**
     * VelocityModulationPanel - Container for multiple V-icons
     */
    class VelocityModulationPanel : public IVelocityModulationView {
    public:
        VelocityModulationPanel();
        ~VelocityModulationPanel() = default;
        
        // V-icon management
        VIcon* addVIcon(uint32_t parameterId, const VIconConfig& config);
        void removeVIcon(uint32_t parameterId);
        VIcon* getVIcon(uint32_t parameterId);
        void clearAllVIcons();
        
        // Batch operations
        void setAllStates(VIconState state);
        void setAllPolarities(ModulationPolarity polarity);
        void setAllDepths(float depth);
        void enableAll(bool enabled);
        
        // Layout management
        void autoLayout(uint16_t startX, uint16_t startY, uint16_t spacing = 20);
        void alignWithParameters(const std::vector<uint16_t>& parameterYPositions);
        
        // Callbacks
        void setGlobalTapCallback(VIconTapCallback callback);
        void setGlobalLongPressCallback(VIconLongPressCallback callback);
        void setModulationUpdateCallback(ModulationUpdateCallback callback);
        
        // TouchGFX integration
        void addToContainer(Container* parent);
        void removeFromContainer();
        bool handleTouch(uint16_t x, uint16_t y, bool pressed);
        void update();
        
        // Statistics
        size_t getVIconCount() const;
        size_t getActiveVIconCount() const;             // Count of latched or active icons
        std::vector<uint32_t> getActiveParameterIds() const;
        
        // IVelocityModulationView interface implementation
        void updateVIconState(uint32_t parameterId, VIconState state) override;
        void updateModulationDepth(uint32_t parameterId, float depth) override;
        void updatePolarity(uint32_t parameterId, ModulationPolarity polarity) override;
        void showVelocitySettings(uint32_t parameterId) override;
        void hideVelocitySettings() override;
        void updateAllVIconStates(VIconState state) override;
        void enableVelocityModulation(bool enabled) override;
        bool isVelocityModulationActive() const override;
        
    private:
        std::vector<std::unique_ptr<VIcon>> vIcons_;
        Container* parentContainer_;
        
        // Callbacks
        VIconTapCallback globalTapCallback_;
        VIconLongPressCallback globalLongPressCallback_;
        ModulationUpdateCallback modulationUpdateCallback_;
        
        // Layout
        uint16_t nextX_, nextY_;
        uint16_t iconSpacing_;
        
        // Internal methods
        VIcon* findVIconByParameterId(uint32_t parameterId);
        void notifyModulationUpdate(uint32_t parameterId, float depth, ModulationPolarity polarity);
    };
    
    /**
     * VelocityModulationSettings - Settings dialog for velocity modulation
     */
    class VelocityModulationSettings {
    public:
        struct Settings {
            float modulationDepth;          // -2.0 to +2.0
            ModulationPolarity polarity;    // Modulation direction
            bool invertVelocity;           // Invert velocity curve
            float velocityScale;           // Velocity sensitivity (0.1-2.0)
            bool enableVelocityToVolume;   // Special case: velocity→volume
            
            Settings() :
                modulationDepth(1.0f),
                polarity(ModulationPolarity::POSITIVE),
                invertVelocity(false),
                velocityScale(1.0f),
                enableVelocityToVolume(true) {}
        };
        
        VelocityModulationSettings();
        ~VelocityModulationSettings() = default;
        
        // Settings management
        void setSettings(const Settings& settings);
        const Settings& getSettings() const { return settings_; }
        void resetToDefaults();
        
        // Dialog display
        void show(uint32_t parameterId, const Settings& currentSettings);
        void hide();
        bool isVisible() const { return visible_; }
        
        // UI interaction
        void onDepthSliderChanged(float depth);
        void onPolarityButtonPressed(ModulationPolarity polarity);
        void onInvertToggleChanged(bool inverted);
        void onScaleSliderChanged(float scale);
        void onVolumeToggleChanged(bool enabled);
        void onOKButtonPressed();
        void onCancelButtonPressed();
        
        // Callbacks
        using SettingsCallback = std::function<void(uint32_t parameterId, const Settings& settings)>;
        void setSettingsCallback(SettingsCallback callback);
        
    private:
        Settings settings_;
        Settings originalSettings_;
        uint32_t currentParameterId_;
        bool visible_;
        
        SettingsCallback settingsCallback_;
        
        // UI components would be TouchGFX objects
        // (Slider, Button, ToggleButton, etc.)
        void updateUIFromSettings();
        void applySettingsToUI();
    };
    
    // Utility functions
    namespace Utils {
        // Color utilities
        uint32_t getVIconColor(VIconState state, ModulationPolarity polarity);
        uint32_t blendColors(uint32_t color1, uint32_t color2, float blend);
        
        // Text formatting
        const char* formatModulationDepth(float depth);         // "-150%" format
        const char* formatVelocityValue(uint8_t velocity);      // "127" format
        
        // Layout helpers
        void distributeVIconsVertically(std::vector<VIcon*>& icons, uint16_t startY, uint16_t endY);
        void alignVIconsWithSliders(std::vector<VIcon*>& icons, const std::vector<uint16_t>& sliderPositions);
        
        // Animation helpers
        void animateVIconState(VIcon* icon, VIconState targetState, uint32_t durationMs = 200);
        void pulseVIcon(VIcon* icon, uint32_t durationMs = 500);
        
        // Touch area calculation
        bool isPointInVIcon(const VIcon* icon, uint16_t x, uint16_t y);
        VIcon* findVIconAtPoint(const std::vector<VIcon*>& icons, uint16_t x, uint16_t y);
    }
    
    // Constants for UI design
    namespace Constants {
        // V-icon dimensions
        static constexpr uint16_t DEFAULT_VICON_WIDTH = 16;
        static constexpr uint16_t DEFAULT_VICON_HEIGHT = 16;
        static constexpr uint16_t DEFAULT_TOUCH_MARGIN = 4;     // Extra touch area around icon
        
        // Colors (RGB565 format)
        static constexpr uint32_t COLOR_INACTIVE = 0x7BEF;     // Light grey
        static constexpr uint32_t COLOR_LATCHED = 0x001F;      // Blue
        static constexpr uint32_t COLOR_POSITIVE = 0x07E0;     // Green
        static constexpr uint32_t COLOR_NEGATIVE = 0xF800;     // Red
        static constexpr uint32_t COLOR_BIPOLAR = 0x7C1F;      // Purple
        
        // Animation timing
        static constexpr uint32_t LONG_PRESS_TIME_MS = 500;
        static constexpr uint32_t DOUBLE_TAP_TIME_MS = 300;
        static constexpr uint32_t ANIMATION_DURATION_MS = 200;
        
        // Layout spacing
        static constexpr uint16_t DEFAULT_ICON_SPACING = 20;
        static constexpr uint16_t PARAMETER_ALIGNMENT_OFFSET = 4;
    }
    
} // namespace VelocityModulationUI