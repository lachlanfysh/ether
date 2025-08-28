#include "VelocityLatchSystem.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// Static member definition
VelocityLatchSystem::ParameterVelocityConfig VelocityLatchSystem::defaultConfig_;

VelocityLatchSystem::VelocityLatchSystem() {
    systemEnabled_ = true;
    lastVelocity_ = 100;  // Default MIDI velocity
    lastVelocitySource_ = VelocityCapture::VelocitySource::NONE;
    lastVelocityUpdateTime_ = std::chrono::steady_clock::now();
}

void VelocityLatchSystem::initialize(std::shared_ptr<VelocityCapture> velocityCapture) {
    velocityCapture_ = velocityCapture;
}

void VelocityLatchSystem::setVelocityModulationPanel(std::shared_ptr<VelocityModulationUI::VelocityModulationPanel> panel) {
    modulationPanel_ = panel;
    
    if (modulationPanel_) {
        // Set up V-icon interaction callbacks
        modulationPanel_->setGlobalTapCallback([this](uint32_t parameterId) {
            handleVIconTap(parameterId);
        });
        
        modulationPanel_->setGlobalLongPressCallback([this](uint32_t parameterId) {
            handleVIconLongPress(parameterId);
        });
        
        modulationPanel_->setModulationUpdateCallback([this](uint32_t parameterId, float depth, VelocityModulationUI::ModulationPolarity polarity) {
            setParameterModulationDepth(parameterId, depth);
            setParameterPolarity(parameterId, polarity);
        });
    }
}

// Parameter registration
void VelocityLatchSystem::registerParameter(uint32_t parameterId, const ParameterVelocityConfig& config) {
    parameterConfigs_[parameterId] = config;
    currentModulatedValues_[parameterId] = config.baseValue;
    
    // Create V-icon for this parameter if panel exists
    if (modulationPanel_) {
        VelocityModulationUI::VIconConfig vIconConfig;
        vIconConfig.state = config.enabled ? VelocityModulationUI::VIconState::LATCHED : VelocityModulationUI::VIconState::INACTIVE;
        vIconConfig.polarity = config.polarity;
        vIconConfig.modulationDepth = config.modulationDepth;
        vIconConfig.enabled = true;
        
        modulationPanel_->addVIcon(parameterId, vIconConfig);
    }
}

void VelocityLatchSystem::unregisterParameter(uint32_t parameterId) {
    parameterConfigs_.erase(parameterId);
    currentModulatedValues_.erase(parameterId);
    
    if (modulationPanel_) {
        modulationPanel_->removeVIcon(parameterId);
    }
}

bool VelocityLatchSystem::isParameterRegistered(uint32_t parameterId) const {
    return parameterConfigs_.find(parameterId) != parameterConfigs_.end();
}

// Velocity latch control
void VelocityLatchSystem::toggleVelocityLatch(uint32_t parameterId) {
    auto it = parameterConfigs_.find(parameterId);
    if (it != parameterConfigs_.end()) {
        it->second.enabled = !it->second.enabled;
        updateParameterVIcon(parameterId);
        
        std::cout << "Velocity latch toggled for parameter " << parameterId 
                  << " (now " << (it->second.enabled ? "enabled" : "disabled") << ")" << std::endl;
    }
}

void VelocityLatchSystem::enableVelocityLatch(uint32_t parameterId, bool enabled) {
    auto it = parameterConfigs_.find(parameterId);
    if (it != parameterConfigs_.end() && it->second.enabled != enabled) {
        it->second.enabled = enabled;
        updateParameterVIcon(parameterId);
    }
}

void VelocityLatchSystem::disableVelocityLatch(uint32_t parameterId) {
    enableVelocityLatch(parameterId, false);
}

bool VelocityLatchSystem::isVelocityLatchEnabled(uint32_t parameterId) const {
    auto it = parameterConfigs_.find(parameterId);
    return (it != parameterConfigs_.end()) ? it->second.enabled : false;
}

// Parameter configuration
void VelocityLatchSystem::setParameterConfig(uint32_t parameterId, const ParameterVelocityConfig& config) {
    parameterConfigs_[parameterId] = config;
    currentModulatedValues_[parameterId] = config.baseValue;
    updateParameterVIcon(parameterId);
}

const VelocityLatchSystem::ParameterVelocityConfig& VelocityLatchSystem::getParameterConfig(uint32_t parameterId) const {
    auto it = parameterConfigs_.find(parameterId);
    return (it != parameterConfigs_.end()) ? it->second : defaultConfig_;
}

void VelocityLatchSystem::setParameterBaseValue(uint32_t parameterId, float baseValue) {
    auto it = parameterConfigs_.find(parameterId);
    if (it != parameterConfigs_.end()) {
        it->second.baseValue = clampParameterValue(baseValue);
    }
}

void VelocityLatchSystem::setParameterModulationDepth(uint32_t parameterId, float depth) {
    auto it = parameterConfigs_.find(parameterId);
    if (it != parameterConfigs_.end()) {
        it->second.modulationDepth = std::max(MIN_MODULATION_DEPTH, std::min(depth, MAX_MODULATION_DEPTH));
        updateParameterVIcon(parameterId);
    }
}

void VelocityLatchSystem::setParameterPolarity(uint32_t parameterId, VelocityModulationUI::ModulationPolarity polarity) {
    auto it = parameterConfigs_.find(parameterId);
    if (it != parameterConfigs_.end()) {
        it->second.polarity = polarity;
        updateParameterVIcon(parameterId);
    }
}

// Real-time velocity modulation
void VelocityLatchSystem::updateVelocityModulation() {
    if (!systemEnabled_ || !velocityCapture_) {
        return;
    }
    
    // Get current velocity and source
    uint8_t currentVelocity = velocityCapture_->getCurrentVelocity();
    VelocityCapture::VelocitySource currentSource = velocityCapture_->getActiveSource();
    auto now = std::chrono::steady_clock::now();
    
    // Check if velocity has changed or if we have recent velocity activity
    bool velocityChanged = (currentVelocity != lastVelocity_ || currentSource != lastVelocitySource_);
    bool velocityActive = velocityCapture_->isSourceActive(currentSource);
    
    if (velocityChanged || velocityActive) {
        lastVelocity_ = currentVelocity;
        lastVelocitySource_ = currentSource;
        lastVelocityUpdateTime_ = now;
        
        // Update all parameters with velocity modulation enabled
        for (auto& [parameterId, config] : parameterConfigs_) {
            if (config.enabled) {
                float modulatedValue = applyVelocityToParameter(parameterId, config.baseValue, currentVelocity);
                currentModulatedValues_[parameterId] = modulatedValue;
                
                // Notify parameter update
                if (parameterUpdateCallback_) {
                    parameterUpdateCallback_(parameterId, modulatedValue);
                }
                
                // Update V-icon state to show active modulation
                if (velocityActive && modulationPanel_) {
                    auto* vIcon = modulationPanel_->getVIcon(parameterId);
                    if (vIcon) {
                        vIcon->setState(VelocityModulationUI::VIconState::ACTIVELY_MODULATING);
                    }
                }
            }
        }
    } else {
        // Check if velocity activity has timed out
        auto timeSinceLastUpdate = now - lastVelocityUpdateTime_;
        if (timeSinceLastUpdate > VELOCITY_UPDATE_TIMEOUT) {
            // Update V-icons to latched state (not actively modulating)
            for (auto& [parameterId, config] : parameterConfigs_) {
                if (config.enabled && modulationPanel_) {
                    auto* vIcon = modulationPanel_->getVIcon(parameterId);
                    if (vIcon && vIcon->getState() == VelocityModulationUI::VIconState::ACTIVELY_MODULATING) {
                        vIcon->setState(VelocityModulationUI::VIconState::LATCHED);
                    }
                }
            }
        }
    }
}

float VelocityLatchSystem::getModulatedParameterValue(uint32_t parameterId) const {
    auto it = currentModulatedValues_.find(parameterId);
    if (it != currentModulatedValues_.end()) {
        return it->second;
    }
    
    // Return base value if no modulation
    auto configIt = parameterConfigs_.find(parameterId);
    return (configIt != parameterConfigs_.end()) ? configIt->second.baseValue : 0.5f;
}

uint8_t VelocityLatchSystem::getCurrentVelocity() const {
    return velocityCapture_ ? velocityCapture_->getCurrentVelocity() : lastVelocity_;
}

VelocityCapture::VelocitySource VelocityLatchSystem::getActiveVelocitySource() const {
    return velocityCapture_ ? velocityCapture_->getActiveSource() : VelocityCapture::VelocitySource::NONE;
}

// Velocity modulation calculation
float VelocityLatchSystem::calculateVelocityModulation(uint32_t parameterId, uint8_t velocity) const {
    auto it = parameterConfigs_.find(parameterId);
    if (it == parameterConfigs_.end() || !it->second.enabled) {
        return 0.0f;  // No modulation
    }
    
    const auto& config = it->second;
    float normalizedVelocity = static_cast<float>(velocity) / 127.0f;
    
    // Apply velocity scale and inversion
    normalizedVelocity *= config.velocityScale;
    if (config.invertVelocity) {
        normalizedVelocity = 1.0f - normalizedVelocity;
    }
    normalizedVelocity = std::max(0.0f, std::min(normalizedVelocity, 1.0f));
    
    // Calculate modulation based on polarity
    switch (config.polarity) {
        case VelocityModulationUI::ModulationPolarity::POSITIVE:
            return config.modulationDepth * normalizedVelocity;
            
        case VelocityModulationUI::ModulationPolarity::NEGATIVE:
            return -config.modulationDepth * normalizedVelocity;
            
        case VelocityModulationUI::ModulationPolarity::BIPOLAR:
            // Bipolar: velocity 0-63 = negative, 64-127 = positive
            return config.modulationDepth * (normalizedVelocity * 2.0f - 1.0f);
    }
    
    return 0.0f;
}

float VelocityLatchSystem::applyVelocityToParameter(uint32_t parameterId, float baseValue, uint8_t velocity) const {
    float modulation = calculateVelocityModulation(parameterId, velocity);
    float modulatedValue = baseValue + modulation;
    return clampParameterValue(modulatedValue);
}

// Batch operations
void VelocityLatchSystem::enableAllVelocityLatches() {
    for (auto& [parameterId, config] : parameterConfigs_) {
        config.enabled = true;
    }
    updateAllVIconStates();
}

void VelocityLatchSystem::disableAllVelocityLatches() {
    for (auto& [parameterId, config] : parameterConfigs_) {
        config.enabled = false;
    }
    updateAllVIconStates();
}

void VelocityLatchSystem::setAllModulationDepths(float depth) {
    float clampedDepth = std::max(MIN_MODULATION_DEPTH, std::min(depth, MAX_MODULATION_DEPTH));
    for (auto& [parameterId, config] : parameterConfigs_) {
        config.modulationDepth = clampedDepth;
    }
    updateAllVIconStates();
}

void VelocityLatchSystem::setAllPolarities(VelocityModulationUI::ModulationPolarity polarity) {
    for (auto& [parameterId, config] : parameterConfigs_) {
        config.polarity = polarity;
    }
    updateAllVIconStates();
}

// Statistics and monitoring
size_t VelocityLatchSystem::getActiveVelocityLatchCount() const {
    return std::count_if(parameterConfigs_.begin(), parameterConfigs_.end(),
        [](const auto& pair) { return pair.second.enabled; });
}

std::vector<uint32_t> VelocityLatchSystem::getActiveVelocityLatchIds() const {
    std::vector<uint32_t> activeIds;
    for (const auto& [parameterId, config] : parameterConfigs_) {
        if (config.enabled) {
            activeIds.push_back(parameterId);
        }
    }
    return activeIds;
}

float VelocityLatchSystem::getSystemVelocityModulationLoad() const {
    // Estimate CPU load: each active velocity latch adds ~0.1% CPU load
    return static_cast<float>(getActiveVelocityLatchCount()) * 0.001f;
}

// Callbacks
void VelocityLatchSystem::setParameterUpdateCallback(ParameterUpdateCallback callback) {
    parameterUpdateCallback_ = callback;
}

void VelocityLatchSystem::setVIconStateUpdateCallback(VIconStateUpdateCallback callback) {
    vIconStateUpdateCallback_ = callback;
}

// System control
void VelocityLatchSystem::setEnabled(bool enabled) {
    systemEnabled_ = enabled;
}

void VelocityLatchSystem::reset() {
    parameterConfigs_.clear();
    currentModulatedValues_.clear();
    lastVelocity_ = 100;
    lastVelocitySource_ = VelocityCapture::VelocitySource::NONE;
    lastVelocityUpdateTime_ = std::chrono::steady_clock::now();
    
    if (modulationPanel_) {
        modulationPanel_->clearAllVIcons();
    }
}

// Private methods
void VelocityLatchSystem::handleVIconTap(uint32_t parameterId) {
    // Short tap: toggle velocity latch on/off
    toggleVelocityLatch(parameterId);
}

void VelocityLatchSystem::handleVIconLongPress(uint32_t parameterId) {
    // Long press: open velocity modulation settings dialog
    // This would open the VelocityModulationSettings dialog in a real implementation
    std::cout << "Long press on V-icon for parameter " << parameterId 
              << " (would open settings dialog)" << std::endl;
}

void VelocityLatchSystem::updateParameterVIcon(uint32_t parameterId) {
    if (!modulationPanel_) return;
    
    auto* vIcon = modulationPanel_->getVIcon(parameterId);
    if (!vIcon) return;
    
    auto it = parameterConfigs_.find(parameterId);
    if (it == parameterConfigs_.end()) return;
    
    const auto& config = it->second;
    
    // Update V-icon state based on configuration
    VelocityModulationUI::VIconState state = getVIconStateForParameter(parameterId);
    vIcon->setState(state);
    vIcon->setPolarity(config.polarity);
    vIcon->setModulationDepth(config.modulationDepth);
    
    // Notify V-icon state update
    if (vIconStateUpdateCallback_) {
        vIconStateUpdateCallback_(parameterId, state);
    }
}

void VelocityLatchSystem::updateAllVIconStates() {
    for (const auto& [parameterId, config] : parameterConfigs_) {
        updateParameterVIcon(parameterId);
    }
}

VelocityModulationUI::VIconState VelocityLatchSystem::getVIconStateForParameter(uint32_t parameterId) const {
    auto it = parameterConfigs_.find(parameterId);
    if (it == parameterConfigs_.end() || !it->second.enabled) {
        return VelocityModulationUI::VIconState::INACTIVE;
    }
    
    // Check if velocity is actively being modulated
    if (velocityCapture_ && velocityCapture_->isSourceActive(velocityCapture_->getActiveSource())) {
        auto timeSinceLastUpdate = std::chrono::steady_clock::now() - lastVelocityUpdateTime_;
        if (timeSinceLastUpdate < VELOCITY_UPDATE_TIMEOUT) {
            return VelocityModulationUI::VIconState::ACTIVELY_MODULATING;
        }
    }
    
    return VelocityModulationUI::VIconState::LATCHED;
}

float VelocityLatchSystem::clampParameterValue(float value) const {
    return std::max(0.0f, std::min(value, 1.0f));
}

float VelocityLatchSystem::applyVelocityCurve(float velocity, bool invert) const {
    if (invert) {
        velocity = 1.0f - velocity;
    }
    return velocity;
}