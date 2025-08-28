#include "GestureSettings.h"
#include <algorithm>
#include <cmath>
#include <iostream>

GestureSettings::GestureSettings() 
    : nextTouchId_(1), enabled_(true), hapticEnabled_(true), accessibilityMode_(false), 
      updateRate_(60.0f) {
    
    // Initialize with sensible defaults
    setGlobalGestureConfig(GestureConfig());
}

void GestureSettings::setGlobalGestureConfig(const GestureConfig& config) {
    globalConfig_ = config;
    
    // Validate configuration values
    globalConfig_.sensitivity = std::clamp(config.sensitivity, 0.1f, 5.0f);
    globalConfig_.deadZone = std::clamp(config.deadZone, 1.0f, 50.0f);
    globalConfig_.detent_strength = std::clamp(config.detent_strength, 0.0f, 1.0f);
    globalConfig_.fine_adjust_ratio = std::clamp(config.fine_adjust_ratio, 0.01f, 1.0f);
    globalConfig_.max_touch_points = std::clamp(config.max_touch_points, uint8_t(1), uint8_t(10));
}

void GestureSettings::setParameterGestureConfig(const std::string& parameterId, 
                                               const ParameterGestureConfig& config) {
    parameterConfigs_[parameterId] = config;
}

const GestureSettings::ParameterGestureConfig& 
GestureSettings::getParameterGestureConfig(const std::string& parameterId) const {
    auto it = parameterConfigs_.find(parameterId);
    return (it != parameterConfigs_.end()) ? it->second : defaultParameterConfig_;
}

bool GestureSettings::hasParameterConfig(const std::string& parameterId) const {
    return parameterConfigs_.find(parameterId) != parameterConfigs_.end();
}

void GestureSettings::removeParameterConfig(const std::string& parameterId) {
    parameterConfigs_.erase(parameterId);
}

// Touch input processing
void GestureSettings::touchDown(uint32_t touchId, float x, float y, float pressure) {
    auto now = std::chrono::steady_clock::now();
    
    TouchPoint& touch = activeTouches_[touchId];
    touch.id = touchId;
    touch.x = x;
    touch.y = y;
    touch.pressure = pressure;
    touch.velocity_x = 0.0f;
    touch.velocity_y = 0.0f;
    touch.start_time = now;
    touch.last_time = now;
    touch.active = true;
    
    // Limit active touches based on configuration
    if (activeTouches_.size() > globalConfig_.max_touch_points) {
        // Remove oldest touch
        auto oldest = std::min_element(activeTouches_.begin(), activeTouches_.end(),
            [](const auto& a, const auto& b) {
                return a.second.start_time < b.second.start_time;
            });
        if (oldest != activeTouches_.end()) {
            activeTouches_.erase(oldest);
        }
    }
}

void GestureSettings::touchMove(uint32_t touchId, float x, float y, float pressure) {
    auto it = activeTouches_.find(touchId);
    if (it == activeTouches_.end()) {
        return; // Touch not found
    }
    
    TouchPoint& touch = it->second;
    auto now = std::chrono::steady_clock::now();
    
    // Calculate velocity
    updateTouchVelocity(touch, x, y);
    
    // Update touch position
    touch.x = x;
    touch.y = y;
    touch.pressure = pressure;
    touch.last_time = now;
}

void GestureSettings::touchUp(uint32_t touchId, float x, float y) {
    auto it = activeTouches_.find(touchId);
    if (it != activeTouches_.end()) {
        TouchPoint& touch = it->second;
        touch.x = x;
        touch.y = y;
        touch.active = false;
        
        // Store in history for gesture analysis
        if (touchHistory_.size() >= MAX_TOUCH_HISTORY) {
            touchHistory_.erase(touchHistory_.begin());
        }
        touchHistory_.push_back(touch);
        
        // Remove from active touches
        activeTouches_.erase(it);
    }
}

void GestureSettings::cancelAllTouches() {
    activeTouches_.clear();
    fineAdjustActive_.clear();
    activeGestures_.clear();
    gestureStartTimes_.clear();
}

// Gesture recognition and processing
std::vector<GestureSettings::GestureResult> GestureSettings::processGestures(float deltaTime) {
    std::vector<GestureResult> results;
    
    if (!enabled_) {
        return results;
    }
    
    // Process single-touch gestures
    for (auto& [touchId, touch] : activeTouches_) {
        if (!touch.active) continue;
        
        GestureType recognizedType = recognizeGesture(touch);
        
        // Create gesture result
        GestureResult result;
        result.type = recognizedType;
        result.velocity = calculateGestureVelocity(touch);
        result.completed = false; // Still in progress
        
        // Store active gesture type
        activeGestures_[touchId] = recognizedType;
        
        results.push_back(result);
    }
    
    // Process multi-touch gestures if enabled
    if (globalConfig_.enable_multi_touch && activeTouches_.size() >= 2) {
        auto multiResults = processMultiTouch(deltaTime);
        results.insert(results.end(), multiResults.begin(), multiResults.end());
    }
    
    // Cleanup inactive touches
    cleanupInactiveTouches();
    
    return results;
}

GestureSettings::GestureResult 
GestureSettings::processParameterGesture(const std::string& parameterId, 
                                        const TouchPoint& touch, float deltaTime) {
    const auto& config = getParameterGestureConfig(parameterId);
    
    GestureResult result;
    result.parameterId = parameterId;
    result.type = recognizeGesture(touch);
    
    // Calculate base value from touch position (this would depend on UI layout)
    float rawValue = 0.5f; // Placeholder - would calculate from touch.x, touch.y
    
    // Apply control mode processing
    result.value = applyControlMode(config.controlMode, rawValue, 
                                   config.min_value, config.max_value, config.step_size);
    
    // Apply detent influence if enabled
    if (globalConfig_.enable_detent_dwell) {
        result.value = applyDetentInfluence(parameterId, result.value);
        result.triggered_detent = isNearDetent(parameterId, result.value, 
                                              globalConfig_.detent_width);
    }
    
    // Apply fine adjustment if active
    if (isFineAdjustActive(parameterId)) {
        result.fine_adjust_active = true;
        float delta = result.value - 0.5f; // Previous value would be stored
        result.delta = applyFineAdjustment(parameterId, delta);
        result.value = 0.5f + result.delta; // Apply fine adjustment
    }
    
    result.velocity = calculateGestureVelocity(touch);
    
    return result;
}

// Gesture recognition methods
GestureSettings::GestureType GestureSettings::recognizeGesture(const TouchPoint& touch) const {
    if (!touch.active) {
        return GestureType::TAP; // Completed gesture
    }
    
    auto currentTime = std::chrono::steady_clock::now();
    auto touchDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - touch.start_time).count();
    
    // Calculate movement distance
    float startX = touch.x; // Would need to store start position
    float startY = touch.y;
    float distance = std::sqrt((touch.x - startX) * (touch.x - startX) + 
                              (touch.y - startY) * (touch.y - startY));
    
    // Recognize gesture based on duration and movement
    if (touchDuration < globalConfig_.tap_timeout && distance < globalConfig_.deadZone) {
        return GestureType::TAP;
    } else if (touchDuration > globalConfig_.hold_delay && distance < globalConfig_.deadZone) {
        return GestureType::HOLD;
    } else if (distance > globalConfig_.deadZone) {
        // Check velocity for flick detection
        float velocity = calculateTouchVelocity(touch);
        if (velocity > globalConfig_.velocity_threshold) {
            return isDoubleFlickGesture(touch.id) ? GestureType::DOUBLE_FLICK : GestureType::FLICK;
        } else {
            return GestureType::DRAG;
        }
    }
    
    return GestureType::TAP; // Default fallback
}

bool GestureSettings::isTapGesture(const TouchPoint& touch) const {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - touch.start_time).count();
    return duration < globalConfig_.tap_timeout;
}

bool GestureSettings::isHoldGesture(const TouchPoint& touch) const {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - touch.start_time).count();
    return duration >= globalConfig_.hold_delay;
}

bool GestureSettings::isDragGesture(const TouchPoint& touch) const {
    // Calculate distance moved (would need start position)
    float distance = 0.0f; // Placeholder
    return distance > globalConfig_.deadZone;
}

bool GestureSettings::isFlickGesture(const TouchPoint& touch) const {
    float velocity = calculateTouchVelocity(touch);
    return velocity > globalConfig_.velocity_threshold;
}

bool GestureSettings::isDoubleFlickGesture(uint32_t touchId) const {
    // Check if there was a recent flick gesture from this touch
    // This would require storing gesture history
    return false; // Placeholder
}

// Parameter value processing
float GestureSettings::processParameterValue(const std::string& parameterId, float rawValue) const {
    const auto& config = getParameterGestureConfig(parameterId);
    
    // Apply control mode
    float processedValue = applyControlMode(config.controlMode, rawValue,
                                           config.min_value, config.max_value, config.step_size);
    
    // Apply detent influence
    if (globalConfig_.enable_detent_dwell) {
        processedValue = applyDetentInfluence(parameterId, processedValue);
    }
    
    // Apply fine adjustment
    if (isFineAdjustActive(parameterId)) {
        // Fine adjustment would modify the processing
        processedValue *= globalConfig_.fine_adjust_ratio;
    }
    
    return processedValue;
}

float GestureSettings::applyControlMode(ControlMode mode, float value, float min, float max, float step) const {
    switch (mode) {
        case ControlMode::CONTINUOUS:
            return std::clamp(value, min, max);
            
        case ControlMode::STEPPED:
            {
                float range = max - min;
                float steps = range / step;
                float steppedValue = std::round(value * steps) / steps;
                return std::clamp(steppedValue, min, max);
            }
            
        case ControlMode::QUANTIZED:
            // Musical quantization (e.g., to semitones, beats)
            return std::round(value * 12.0f) / 12.0f; // Example: semitone quantization
            
        case ControlMode::BIPOLAR:
            // Center around 0.5, allow positive/negative
            return std::clamp((value - 0.5f) * 2.0f, -1.0f, 1.0f);
            
        case ControlMode::LOGARITHMIC:
            // Exponential curve for frequency-like parameters
            return std::pow(value, 2.0f);
            
        case ControlMode::CUSTOM:
            // Would apply user-defined curve
            return value;
    }
    
    return value;
}

// Detent system implementation
void GestureSettings::addDetentPosition(const std::string& parameterId, float position) {
    position = std::clamp(position, 0.0f, 1.0f);
    auto& positions = detentPositions_[parameterId];
    
    // Add if not already present
    if (std::find(positions.begin(), positions.end(), position) == positions.end()) {
        positions.push_back(position);
        std::sort(positions.begin(), positions.end());
    }
}

void GestureSettings::removeDetentPosition(const std::string& parameterId, float position) {
    auto it = detentPositions_.find(parameterId);
    if (it != detentPositions_.end()) {
        auto& positions = it->second;
        positions.erase(std::remove(positions.begin(), positions.end(), position), positions.end());
    }
}

void GestureSettings::clearDetentPositions(const std::string& parameterId) {
    detentPositions_.erase(parameterId);
}

std::vector<float> GestureSettings::getDetentPositions(const std::string& parameterId) const {
    auto it = detentPositions_.find(parameterId);
    return (it != detentPositions_.end()) ? it->second : std::vector<float>();
}

float GestureSettings::findNearestDetent(const std::string& parameterId, float value) const {
    auto positions = getDetentPositions(parameterId);
    if (positions.empty()) {
        return value;
    }
    
    auto nearest = std::min_element(positions.begin(), positions.end(),
        [value](float a, float b) {
            return std::abs(a - value) < std::abs(b - value);
        });
    
    return *nearest;
}

bool GestureSettings::isNearDetent(const std::string& parameterId, float value, float tolerance) const {
    float nearest = findNearestDetent(parameterId, value);
    return std::abs(value - nearest) <= tolerance;
}

float GestureSettings::applyDetentInfluence(const std::string& parameterId, float value) const {
    if (globalConfig_.detent_mode == DetentBehavior::NONE) {
        return value;
    }
    
    float nearestDetent = findNearestDetent(parameterId, value);
    float distance = std::abs(value - nearestDetent);
    
    if (distance <= globalConfig_.detent_width) {
        float influence = globalConfig_.detent_strength;
        
        if (globalConfig_.detent_mode == DetentBehavior::HARD) {
            influence = 1.0f; // Snap completely to detent
        } else if (globalConfig_.detent_mode == DetentBehavior::SOFT) {
            // Gradual pull toward detent
            influence *= (1.0f - distance / globalConfig_.detent_width);
        }
        
        return value + (nearestDetent - value) * influence;
    }
    
    return value;
}

// Fine adjustment mode
void GestureSettings::enterFineAdjustMode(const std::string& parameterId) {
    fineAdjustActive_[parameterId] = true;
    
    // Trigger haptic feedback
    if (hapticEnabled_) {
        triggerHapticFeedback(HapticFeedback::LIGHT, 0.5f);
    }
}

void GestureSettings::exitFineAdjustMode(const std::string& parameterId) {
    fineAdjustActive_[parameterId] = false;
}

bool GestureSettings::isFineAdjustActive(const std::string& parameterId) const {
    auto it = fineAdjustActive_.find(parameterId);
    return (it != fineAdjustActive_.end()) ? it->second : false;
}

float GestureSettings::applyFineAdjustment(const std::string& parameterId, float delta) const {
    return delta * globalConfig_.fine_adjust_ratio;
}

// Multi-touch processing
std::vector<GestureSettings::GestureResult> GestureSettings::processMultiTouch(float deltaTime) {
    std::vector<GestureResult> results;
    
    if (activeTouches_.size() < 2) {
        return results;
    }
    
    // Convert to vector for easier processing
    std::vector<TouchPoint> touches;
    for (const auto& [id, touch] : activeTouches_) {
        if (touch.active) {
            touches.push_back(touch);
        }
    }
    
    // Check for pinch gesture
    if (touches.size() == 2 && isPinchGesture(touches)) {
        GestureResult result;
        result.type = GestureType::PINCH;
        result.completed = false;
        
        // Calculate pinch scale (distance between touches)
        float distance = this->distance(touches[0].x, touches[0].y, touches[1].x, touches[1].y);
        result.value = distance / 100.0f; // Normalize distance
        
        results.push_back(result);
    }
    
    return results;
}

bool GestureSettings::isPinchGesture(const std::vector<TouchPoint>& touches) const {
    if (touches.size() != 2) {
        return false;
    }
    
    // Check if touches are sufficiently separated
    float dist = distance(touches[0].x, touches[0].y, touches[1].x, touches[1].y);
    return dist > globalConfig_.multi_touch_separation;
}

bool GestureSettings::touchesAreSeparated(const TouchPoint& touch1, const TouchPoint& touch2) const {
    float dist = distance(touch1.x, touch1.y, touch2.x, touch2.y);
    return dist >= globalConfig_.multi_touch_separation;
}

// Haptic feedback
void GestureSettings::triggerHapticFeedback(HapticFeedback type, float intensity) {
    if (!hapticEnabled_ || !hapticCallback_) {
        return;
    }
    
    float adjustedIntensity = intensity * globalConfig_.haptic_intensity;
    hapticCallback_(type, adjustedIntensity);
}

void GestureSettings::setHapticEnabled(bool enabled) {
    hapticEnabled_ = enabled;
}

bool GestureSettings::isHapticEnabled() const {
    return hapticEnabled_ && globalConfig_.haptic_mode != HapticFeedback::NONE;
}

// Utility methods
float GestureSettings::distance(float x1, float y1, float x2, float y2) const {
    return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

float GestureSettings::calculateTouchVelocity(const TouchPoint& touch) const {
    return std::sqrt(touch.velocity_x * touch.velocity_x + touch.velocity_y * touch.velocity_y);
}

float GestureSettings::calculateGestureVelocity(const TouchPoint& touch) const {
    return calculateTouchVelocity(touch) * globalConfig_.touch_velocity_scale;
}

void GestureSettings::updateTouchVelocity(TouchPoint& touch, float x, float y) {
    auto now = std::chrono::steady_clock::now();
    auto deltaTime = std::chrono::duration<float>(now - touch.last_time).count();
    
    if (deltaTime > 0.0f) {
        touch.velocity_x = (x - touch.x) / deltaTime;
        touch.velocity_y = (y - touch.y) / deltaTime;
    }
}

void GestureSettings::cleanupInactiveTouches() {
    // Remove touches that have been inactive for too long
    auto now = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds(1); // 1 second timeout
    
    for (auto it = activeTouches_.begin(); it != activeTouches_.end();) {
        if (!it->second.active && (now - it->second.last_time) > timeout) {
            it = activeTouches_.erase(it);
        } else {
            ++it;
        }
    }
}

void GestureSettings::notifyGestureRecognized(const GestureResult& result) {
    if (gestureCallback_) {
        gestureCallback_(result);
    }
}

// Accessibility and customization
void GestureSettings::setAccessibilityMode(bool enabled) {
    accessibilityMode_ = enabled;
    
    if (enabled) {
        // Adjust settings for better accessibility
        globalConfig_.large_gesture_mode = true;
        globalConfig_.sticky_drag_mode = true;
        globalConfig_.deadZone *= 1.5f; // Larger dead zone
        globalConfig_.tap_timeout *= 2; // Longer tap timeout
    }
}

void GestureSettings::setLargeGestureMode(bool enabled) {
    globalConfig_.large_gesture_mode = enabled;
}

void GestureSettings::setStickyDragMode(bool enabled) {
    globalConfig_.sticky_drag_mode = enabled;
}

void GestureSettings::setAccessibilitySensitivity(float multiplier) {
    globalConfig_.accessibility_multiplier = std::clamp(multiplier, 0.1f, 5.0f);
    globalConfig_.sensitivity *= multiplier;
}

// System management
void GestureSettings::setMultiTouchEnabled(bool enabled) {
    globalConfig_.enable_multi_touch = enabled;
}

size_t GestureSettings::getActiveTouchCount() const {
    return activeTouches_.size();
}

std::vector<GestureSettings::TouchPoint> GestureSettings::getActiveTouches() const {
    std::vector<TouchPoint> touches;
    for (const auto& [id, touch] : activeTouches_) {
        if (touch.active) {
            touches.push_back(touch);
        }
    }
    return touches;
}

void GestureSettings::resetToDefaults() {
    globalConfig_ = GestureConfig();
    parameterConfigs_.clear();
    cancelAllTouches();
}

void GestureSettings::saveUserPreferences() {
    // Implementation would save to persistent storage
    // This is a placeholder for the save functionality
}

void GestureSettings::loadUserPreferences() {
    // Implementation would load from persistent storage
    // This is a placeholder for the load functionality
}