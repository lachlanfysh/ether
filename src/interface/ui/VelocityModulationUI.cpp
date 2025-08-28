#include "VelocityModulationUI.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>

// Mock TouchGFX classes for compilation
class Container {};
class Image {};
class TextArea {};
class TouchArea {};

namespace VelocityModulationUI {

// VIcon implementation
VIcon::VIcon() {
    config_ = VIconConfig();
    parameterId_ = 0;
    visible_ = true;
    touchActive_ = false;
    touchStartTime_ = 0;
    
    // Initialize TouchGFX components (would be real objects in actual implementation)
    iconImage_ = std::make_unique<Image>();
    depthText_ = std::make_unique<TextArea>();
    touchArea_ = std::make_unique<TouchArea>();
}

VIcon::~VIcon() {
    // Custom destructor needed to handle incomplete types in unique_ptr
}

void VIcon::setConfig(const VIconConfig& config) {
    config_ = config;
    updateIconVisuals();
    updateDepthText();
}

void VIcon::setParameterId(uint32_t parameterId) {
    parameterId_ = parameterId;
}

void VIcon::setState(VIconState state) {
    if (config_.state != state) {
        config_.state = state;
        updateIconVisuals();
    }
}

void VIcon::setPolarity(ModulationPolarity polarity) {
    if (config_.polarity != polarity) {
        config_.polarity = polarity;
        updateIconVisuals();
    }
}

void VIcon::setModulationDepth(float depth) {
    // Clamp depth to valid range
    float clampedDepth = std::max(-2.0f, std::min(depth, 2.0f));
    
    if (std::abs(config_.modulationDepth - clampedDepth) > 0.01f) {
        config_.modulationDepth = clampedDepth;
        updateDepthText();
        
        // Update polarity based on depth sign
        if (clampedDepth > 0.0f) {
            config_.polarity = ModulationPolarity::POSITIVE;
        } else if (clampedDepth < 0.0f) {
            config_.polarity = ModulationPolarity::NEGATIVE;
        }
        
        updateIconVisuals();
    }
}

void VIcon::setEnabled(bool enabled) {
    config_.enabled = enabled;
    updateIconVisuals();
}

void VIcon::setPosition(uint16_t x, uint16_t y) {
    config_.x = x;
    config_.y = y;
}

void VIcon::setSize(uint16_t width, uint16_t height) {
    config_.width = width;
    config_.height = height;
}

void VIcon::setVisible(bool visible) {
    visible_ = visible;
}

void VIcon::setTapCallback(VIconTapCallback callback) {
    tapCallback_ = callback;
}

void VIcon::setLongPressCallback(VIconLongPressCallback callback) {
    longPressCallback_ = callback;
}

bool VIcon::handleTouch(uint16_t x, uint16_t y, bool pressed) {
    if (!visible_ || !config_.enabled) {
        return false;
    }
    
    // Check if touch is within icon bounds (with margin)
    uint16_t margin = Constants::DEFAULT_TOUCH_MARGIN;
    bool inBounds = (x >= config_.x - margin && x <= config_.x + config_.width + margin &&
                     y >= config_.y - margin && y <= config_.y + config_.height + margin);
    
    if (!inBounds) {
        touchActive_ = false;
        return false;
    }
    
    if (pressed && !touchActive_) {
        // Touch started
        touchActive_ = true;
        touchStartTime_ = 0; // Would use real timestamp in actual implementation
        return true;
    } else if (!pressed && touchActive_) {
        // Touch ended
        touchActive_ = false;
        uint32_t touchDuration = 100; // Mock duration
        
        if (touchDuration < Constants::LONG_PRESS_TIME_MS) {
            // Short tap
            if (tapCallback_) {
                tapCallback_(parameterId_);
            }
        } else {
            // Long press
            if (longPressCallback_) {
                longPressCallback_(parameterId_);
            }
        }
        return true;
    }
    
    return touchActive_;
}

void VIcon::draw(Container* parent) {
    if (!visible_) return;
    
    // In real implementation, this would:
    // 1. Set icon image based on state/polarity
    // 2. Position the image at config_.x, config_.y
    // 3. Update text area if depth display is enabled
    // 4. Add components to parent container
}

void VIcon::update() {
    updateIconVisuals();
    updateDepthText();
}

void VIcon::updateIconVisuals() {
    // In real implementation, this would update the TouchGFX Image component
    // with appropriate color and visual state based on:
    // - config_.state (inactive/latched/active)
    // - config_.polarity (positive/negative/bipolar)
    // - config_.enabled state
}

void VIcon::updateDepthText() {
    if (config_.showDepthText) {
        // In real implementation, update TextArea with formatted depth
        // Using Utils::formatModulationDepth(config_.modulationDepth)
    }
}

uint32_t VIcon::getColorForState() const {
    if (!config_.enabled) {
        return Constants::COLOR_INACTIVE;
    }
    
    switch (config_.state) {
        case VIconState::INACTIVE:
            return Constants::COLOR_INACTIVE;
            
        case VIconState::LATCHED:
            return Constants::COLOR_LATCHED;
            
        case VIconState::ACTIVELY_MODULATING:
            switch (config_.polarity) {
                case ModulationPolarity::POSITIVE:
                    return Constants::COLOR_POSITIVE;
                case ModulationPolarity::NEGATIVE:
                    return Constants::COLOR_NEGATIVE;
                case ModulationPolarity::BIPOLAR:
                    return Constants::COLOR_BIPOLAR;
            }
            break;
    }
    
    return Constants::COLOR_INACTIVE;
}

const char* VIcon::getIconImagePath() const {
    // In real implementation, return path to appropriate icon asset
    switch (config_.state) {
        case VIconState::INACTIVE:
            return "assets/v_icon_inactive.png";
        case VIconState::LATCHED:
            return "assets/v_icon_latched.png";
        case VIconState::ACTIVELY_MODULATING:
            return "assets/v_icon_active.png";
    }
    return "assets/v_icon_inactive.png";
}

// VelocityModulationPanel implementation
VelocityModulationPanel::VelocityModulationPanel() {
    parentContainer_ = nullptr;
    nextX_ = 0;
    nextY_ = 0;
    iconSpacing_ = Constants::DEFAULT_ICON_SPACING;
}

VIcon* VelocityModulationPanel::addVIcon(uint32_t parameterId, const VIconConfig& config) {
    // Check if V-icon already exists for this parameter
    VIcon* existing = findVIconByParameterId(parameterId);
    if (existing) {
        existing->setConfig(config);
        return existing;
    }
    
    // Create new V-icon
    auto vIcon = std::make_unique<VIcon>();
    vIcon->setParameterId(parameterId);
    vIcon->setConfig(config);
    
    // Set callbacks to forward to global callbacks
    vIcon->setTapCallback([this](uint32_t paramId) {
        if (globalTapCallback_) {
            globalTapCallback_(paramId);
        }
    });
    
    vIcon->setLongPressCallback([this](uint32_t paramId) {
        if (globalLongPressCallback_) {
            globalLongPressCallback_(paramId);
        }
    });
    
    VIcon* iconPtr = vIcon.get();
    vIcons_.push_back(std::move(vIcon));
    
    return iconPtr;
}

void VelocityModulationPanel::removeVIcon(uint32_t parameterId) {
    auto it = std::find_if(vIcons_.begin(), vIcons_.end(),
        [parameterId](const std::unique_ptr<VIcon>& icon) {
            return icon->getParameterId() == parameterId;
        });
    
    if (it != vIcons_.end()) {
        vIcons_.erase(it);
    }
}

VIcon* VelocityModulationPanel::getVIcon(uint32_t parameterId) {
    return findVIconByParameterId(parameterId);
}

void VelocityModulationPanel::clearAllVIcons() {
    vIcons_.clear();
}

void VelocityModulationPanel::setAllStates(VIconState state) {
    for (auto& icon : vIcons_) {
        icon->setState(state);
    }
}

void VelocityModulationPanel::setAllPolarities(ModulationPolarity polarity) {
    for (auto& icon : vIcons_) {
        icon->setPolarity(polarity);
    }
}

void VelocityModulationPanel::setAllDepths(float depth) {
    for (auto& icon : vIcons_) {
        icon->setModulationDepth(depth);
    }
}

void VelocityModulationPanel::enableAll(bool enabled) {
    for (auto& icon : vIcons_) {
        icon->setEnabled(enabled);
    }
}

void VelocityModulationPanel::autoLayout(uint16_t startX, uint16_t startY, uint16_t spacing) {
    nextX_ = startX;
    nextY_ = startY;
    iconSpacing_ = spacing;
    
    for (auto& icon : vIcons_) {
        icon->setPosition(nextX_, nextY_);
        nextY_ += spacing;
    }
}

void VelocityModulationPanel::alignWithParameters(const std::vector<uint16_t>& parameterYPositions) {
    size_t iconIndex = 0;
    for (uint16_t yPos : parameterYPositions) {
        if (iconIndex < vIcons_.size()) {
            vIcons_[iconIndex]->setPosition(nextX_, yPos + Constants::PARAMETER_ALIGNMENT_OFFSET);
            iconIndex++;
        }
    }
}

void VelocityModulationPanel::setGlobalTapCallback(VIconTapCallback callback) {
    globalTapCallback_ = callback;
}

void VelocityModulationPanel::setGlobalLongPressCallback(VIconLongPressCallback callback) {
    globalLongPressCallback_ = callback;
}

void VelocityModulationPanel::setModulationUpdateCallback(ModulationUpdateCallback callback) {
    modulationUpdateCallback_ = callback;
}

void VelocityModulationPanel::addToContainer(Container* parent) {
    parentContainer_ = parent;
    for (auto& icon : vIcons_) {
        icon->draw(parent);
    }
}

void VelocityModulationPanel::removeFromContainer() {
    parentContainer_ = nullptr;
    // In real implementation, remove all V-icons from container
}

bool VelocityModulationPanel::handleTouch(uint16_t x, uint16_t y, bool pressed) {
    for (auto& icon : vIcons_) {
        if (icon->handleTouch(x, y, pressed)) {
            return true;
        }
    }
    return false;
}

void VelocityModulationPanel::update() {
    for (auto& icon : vIcons_) {
        icon->update();
    }
}

size_t VelocityModulationPanel::getVIconCount() const {
    return vIcons_.size();
}

size_t VelocityModulationPanel::getActiveVIconCount() const {
    return std::count_if(vIcons_.begin(), vIcons_.end(),
        [](const std::unique_ptr<VIcon>& icon) {
            VIconState state = icon->getState();
            return state == VIconState::LATCHED || state == VIconState::ACTIVELY_MODULATING;
        });
}

std::vector<uint32_t> VelocityModulationPanel::getActiveParameterIds() const {
    std::vector<uint32_t> activeIds;
    
    for (const auto& icon : vIcons_) {
        VIconState state = icon->getState();
        if (state == VIconState::LATCHED || state == VIconState::ACTIVELY_MODULATING) {
            activeIds.push_back(icon->getParameterId());
        }
    }
    
    return activeIds;
}

VIcon* VelocityModulationPanel::findVIconByParameterId(uint32_t parameterId) {
    auto it = std::find_if(vIcons_.begin(), vIcons_.end(),
        [parameterId](const std::unique_ptr<VIcon>& icon) {
            return icon->getParameterId() == parameterId;
        });
    
    return (it != vIcons_.end()) ? it->get() : nullptr;
}

void VelocityModulationPanel::notifyModulationUpdate(uint32_t parameterId, float depth, ModulationPolarity polarity) {
    if (modulationUpdateCallback_) {
        modulationUpdateCallback_(parameterId, depth, polarity);
    }
}

// VelocityModulationSettings implementation
VelocityModulationSettings::VelocityModulationSettings() {
    settings_ = Settings();
    originalSettings_ = Settings();
    currentParameterId_ = 0;
    visible_ = false;
}

void VelocityModulationSettings::setSettings(const Settings& settings) {
    settings_ = settings;
    if (visible_) {
        updateUIFromSettings();
    }
}

void VelocityModulationSettings::resetToDefaults() {
    settings_ = Settings();
    if (visible_) {
        updateUIFromSettings();
    }
}

void VelocityModulationSettings::show(uint32_t parameterId, const Settings& currentSettings) {
    currentParameterId_ = parameterId;
    originalSettings_ = currentSettings;
    settings_ = currentSettings;
    visible_ = true;
    updateUIFromSettings();
}

void VelocityModulationSettings::hide() {
    visible_ = false;
}

void VelocityModulationSettings::onDepthSliderChanged(float depth) {
    settings_.modulationDepth = std::max(-2.0f, std::min(depth, 2.0f));
}

void VelocityModulationSettings::onPolarityButtonPressed(ModulationPolarity polarity) {
    settings_.polarity = polarity;
}

void VelocityModulationSettings::onInvertToggleChanged(bool inverted) {
    settings_.invertVelocity = inverted;
}

void VelocityModulationSettings::onScaleSliderChanged(float scale) {
    settings_.velocityScale = std::max(0.1f, std::min(scale, 2.0f));
}

void VelocityModulationSettings::onVolumeToggleChanged(bool enabled) {
    settings_.enableVelocityToVolume = enabled;
}

void VelocityModulationSettings::onOKButtonPressed() {
    if (settingsCallback_) {
        settingsCallback_(currentParameterId_, settings_);
    }
    hide();
}

void VelocityModulationSettings::onCancelButtonPressed() {
    settings_ = originalSettings_;
    hide();
}

void VelocityModulationSettings::setSettingsCallback(SettingsCallback callback) {
    settingsCallback_ = callback;
}

void VelocityModulationSettings::updateUIFromSettings() {
    // In real implementation, update all UI sliders, buttons, toggles
    // based on current settings_ values
}

void VelocityModulationSettings::applySettingsToUI() {
    // In real implementation, read values from UI components
    // and update settings_ structure
}

// Utility functions
namespace Utils {

uint32_t getVIconColor(VIconState state, ModulationPolarity polarity) {
    switch (state) {
        case VIconState::INACTIVE:
            return Constants::COLOR_INACTIVE;
            
        case VIconState::LATCHED:
            return Constants::COLOR_LATCHED;
            
        case VIconState::ACTIVELY_MODULATING:
            switch (polarity) {
                case ModulationPolarity::POSITIVE:
                    return Constants::COLOR_POSITIVE;
                case ModulationPolarity::NEGATIVE:
                    return Constants::COLOR_NEGATIVE;
                case ModulationPolarity::BIPOLAR:
                    return Constants::COLOR_BIPOLAR;
            }
            break;
    }
    
    return Constants::COLOR_INACTIVE;
}

uint32_t blendColors(uint32_t color1, uint32_t color2, float blend) {
    // Simple RGB565 color blending
    blend = std::max(0.0f, std::min(blend, 1.0f));
    
    uint16_t r1 = (color1 >> 11) & 0x1F;
    uint16_t g1 = (color1 >> 5) & 0x3F;
    uint16_t b1 = color1 & 0x1F;
    
    uint16_t r2 = (color2 >> 11) & 0x1F;
    uint16_t g2 = (color2 >> 5) & 0x3F;
    uint16_t b2 = color2 & 0x1F;
    
    uint16_t r = static_cast<uint16_t>(r1 + blend * (r2 - r1));
    uint16_t g = static_cast<uint16_t>(g1 + blend * (g2 - g1));
    uint16_t b = static_cast<uint16_t>(b1 + blend * (b2 - b1));
    
    return (r << 11) | (g << 5) | b;
}

static char depthBuffer[8];
const char* formatModulationDepth(float depth) {
    int percentage = static_cast<int>(depth * 100.0f);
    snprintf(depthBuffer, sizeof(depthBuffer), "%+d%%", percentage);
    return depthBuffer;
}

static char velocityBuffer[4];
const char* formatVelocityValue(uint8_t velocity) {
    snprintf(velocityBuffer, sizeof(velocityBuffer), "%d", velocity);
    return velocityBuffer;
}

void distributeVIconsVertically(std::vector<VIcon*>& icons, uint16_t startY, uint16_t endY) {
    if (icons.empty()) return;
    
    if (icons.size() == 1) {
        icons[0]->setPosition(icons[0]->getConfig().x, (startY + endY) / 2);
        return;
    }
    
    uint16_t spacing = (endY - startY) / (icons.size() - 1);
    uint16_t currentY = startY;
    
    for (auto* icon : icons) {
        icon->setPosition(icon->getConfig().x, currentY);
        currentY += spacing;
    }
}

void alignVIconsWithSliders(std::vector<VIcon*>& icons, const std::vector<uint16_t>& sliderPositions) {
    size_t maxIcons = std::min(icons.size(), sliderPositions.size());
    
    for (size_t i = 0; i < maxIcons; i++) {
        uint16_t alignedY = sliderPositions[i] + Constants::PARAMETER_ALIGNMENT_OFFSET;
        icons[i]->setPosition(icons[i]->getConfig().x, alignedY);
    }
}

void animateVIconState(VIcon* icon, VIconState targetState, uint32_t durationMs) {
    // In real implementation, create animation from current state to target state
    // For now, just set the state immediately
    icon->setState(targetState);
}

void pulseVIcon(VIcon* icon, uint32_t durationMs) {
    // In real implementation, create pulsing animation effect
    // Could use TouchGFX animation framework
}

bool isPointInVIcon(const VIcon* icon, uint16_t x, uint16_t y) {
    const VIconConfig& config = icon->getConfig();
    uint16_t margin = Constants::DEFAULT_TOUCH_MARGIN;
    
    return (x >= config.x - margin && x <= config.x + config.width + margin &&
            y >= config.y - margin && y <= config.y + config.height + margin);
}

VIcon* findVIconAtPoint(const std::vector<VIcon*>& icons, uint16_t x, uint16_t y) {
    for (auto* icon : icons) {
        if (isPointInVIcon(icon, x, y)) {
            return icon;
        }
    }
    return nullptr;
}

} // namespace Utils

} // namespace VelocityModulationUI