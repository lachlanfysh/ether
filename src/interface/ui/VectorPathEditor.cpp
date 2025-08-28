#include "VectorPathEditor.h"
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
#include "touchgfx/widgets/Line.hpp"
#include "touchgfx/widgets/Circle.hpp"
#else
#include <chrono>
#endif

VectorPathEditor::VectorPathEditor() {
    // Initialize display configuration
    display_.x = 0;
    display_.y = 0;
    display_.width = 400;
    display_.height = 400;
    display_.diamondCenterX = 200;
    display_.diamondCenterY = 200;
    display_.diamondRadius = 150;
    
    // Initialize corner labels
    initializeCornerLabels();
    
    // Initialize animations
    modeTransition_.active = false;
    waypointHighlight_.active = false;
    
    // Initialize path preview
    pathPreview_.valid = false;
    
    // Initialize blend visualization
    blendViz_.weights.fill(0.25f);
    blendViz_.colors = display_.cornerColors;
    blendViz_.totalWeight = 1.0f;
    blendViz_.showNumbers = true;
    blendViz_.showBars = true;
}

VectorPathEditor::~VectorPathEditor() {
    shutdown();
}

bool VectorPathEditor::initialize(VectorPath* vectorPath, SmartKnob* smartKnob) {
    if (initialized_) {
        return true;
    }
    
    vectorPath_ = vectorPath;
    smartKnob_ = smartKnob;
    
    if (!vectorPath_) {
        return false;
    }
    
    // Set up VectorPath callbacks
    vectorPath_->setPositionChangeCallback([this](const VectorPath::Position& pos, const VectorPath::CornerBlend& blend) {
        updateBlendVisualization();
        if (positionCallback_) {
            positionCallback_(pos, blend);
        }
    });
    
    vectorPath_->setWaypointChangeCallback([this](int waypointIndex, const VectorPath::Waypoint& waypoint) {
        updateWaypointVisuals();
        updatePathPreview();
        if (waypointCallback_) {
            waypointCallback_(waypointIndex, waypoint);
        }
    });
    
    // Set up SmartKnob callbacks
    if (smartKnob_) {
        smartKnob_->setRotationCallback([this](int32_t delta, float velocity, bool inDetent) {
            handleRotation(delta, velocity, inDetent);
        });
        
        smartKnob_->setGestureCallback([this](SmartKnob::GestureType gesture, float parameter) {
            handleGesture(gesture, parameter);
        });
    }
    
    // Initialize visuals
    updateWaypointVisuals();
    updatePathPreview();
    updateBlendVisualization();
    
    initialized_ = true;
    return true;
}

void VectorPathEditor::shutdown() {
    if (!initialized_) {
        return;
    }
    
    vectorPath_ = nullptr;
    smartKnob_ = nullptr;
    initialized_ = false;
}

void VectorPathEditor::setEditMode(EditMode mode) {
    if (mode == editMode_) {
        return;
    }
    
    EditMode oldMode = editMode_;
    editMode_ = mode;
    
    // Clear selection when changing modes
    if (mode != EditMode::EDIT_WAYPOINT) {
        clearSelection();
    }
    
    // Configure SmartKnob for new mode
    if (smartKnob_) {
        SmartKnob::DetentConfig detentConfig;
        SmartKnob::HapticConfig hapticConfig;
        
        switch (mode) {
            case EditMode::NAVIGATE:
                detentConfig.mode = SmartKnob::DetentMode::NONE;
                hapticConfig.pattern = SmartKnob::HapticPattern::NONE;
                break;
                
            case EditMode::ADD_WAYPOINT:
                detentConfig.mode = SmartKnob::DetentMode::LIGHT;
                hapticConfig.pattern = SmartKnob::HapticPattern::TICK;
                break;
                
            case EditMode::EDIT_WAYPOINT:
                detentConfig.mode = SmartKnob::DetentMode::MEDIUM;
                hapticConfig.pattern = SmartKnob::HapticPattern::SPRING;
                break;
                
            case EditMode::DELETE_WAYPOINT:
                detentConfig.mode = SmartKnob::DetentMode::HEAVY;
                hapticConfig.pattern = SmartKnob::HapticPattern::THUD;
                break;
                
            case EditMode::PLAYBACK:
                detentConfig.mode = SmartKnob::DetentMode::LIGHT;
                hapticConfig.pattern = SmartKnob::HapticPattern::BUMP;
                break;
        }
        
        smartKnob_->setDetentConfig(detentConfig);
        smartKnob_->setHapticConfig(hapticConfig);
    }
    
    // Trigger mode transition animation
    modeTransition_.start(200.0f);
    
    if (modeCallback_) {
        modeCallback_(mode, oldMode);
    }
}

void VectorPathEditor::setDisplayBounds(int x, int y, int width, int height) {
    display_.x = x;
    display_.y = y;
    display_.width = width;
    display_.height = height;
    
    // Recalculate diamond center and radius
    display_.diamondCenterX = x + width / 2;
    display_.diamondCenterY = y + height / 2;
    display_.diamondRadius = std::min(width, height) / 3;
}

void VectorPathEditor::setDiamondBounds(int centerX, int centerY, int radius) {
    display_.diamondCenterX = centerX;
    display_.diamondCenterY = centerY;
    display_.diamondRadius = radius;
}

void VectorPathEditor::handleTouchDown(int x, int y, int touchId) {
    if (!initialized_ || !visible_) {
        return;
    }
    
    touch_.state = TouchState::PRESSED;
    touch_.primaryTouch.x = static_cast<float>(x);
    touch_.primaryTouch.y = static_cast<float>(y);
    touch_.primaryTouch.active = true;
    touch_.primaryTouch.timestamp = getTimeMs();
    touch_.primaryTouch.trackingId = touchId;
    
    // Check if touch is within diamond bounds
    if (!isPointInDiamond(x, y)) {
        return;
    }
    
    // Find nearest waypoint for editing
    int nearestWaypoint = findNearestWaypoint(x, y, WAYPOINT_SELECT_RADIUS);
    
    switch (editMode_) {
        case EditMode::NAVIGATE:
            {
                // Set position directly
                VectorPath::Position pos = screenToNormalized(x, y);
                vectorPath_->setPosition(pos);
            }
            break;
            
        case EditMode::ADD_WAYPOINT:
            if (nearestWaypoint == -1) {
                // Create new waypoint
                VectorPath::Position pos = screenToNormalized(x, y);
                createWaypointAtPosition(pos);
            }
            break;
            
        case EditMode::EDIT_WAYPOINT:
            if (nearestWaypoint != -1) {
                // Start dragging waypoint
                selectWaypoint(nearestWaypoint);
                touch_.dragWaypointIndex = nearestWaypoint;
                touch_.dragStartPos = screenToNormalized(x, y);
                touch_.dragCurrentPos = touch_.dragStartPos;
            }
            break;
            
        case EditMode::DELETE_WAYPOINT:
            if (nearestWaypoint != -1) {
                deleteWaypoint(nearestWaypoint);
            }
            break;
            
        case EditMode::PLAYBACK:
            // Handle playback controls
            break;
    }
}

void VectorPathEditor::handleTouchMove(int x, int y, int touchId) {
    if (!initialized_ || !visible_ || !touch_.primaryTouch.active) {
        return;
    }
    
    if (touch_.primaryTouch.trackingId != touchId) {
        return;
    }
    
    touch_.primaryTouch.x = static_cast<float>(x);
    touch_.primaryTouch.y = static_cast<float>(y);
    
    // Check if we've started dragging
    float dragDistance = std::sqrt(
        std::pow(touch_.primaryTouch.x - touch_.dragStartPos.x * display_.width, 2) +
        std::pow(touch_.primaryTouch.y - touch_.dragStartPos.y * display_.height, 2)
    );
    
    if (dragDistance > WAYPOINT_DRAG_THRESHOLD && touch_.state != TouchState::DRAGGING) {
        touch_.state = TouchState::DRAGGING;
    }
    
    if (touch_.state == TouchState::DRAGGING) {
        VectorPath::Position newPos = screenToNormalized(x, y);
        touch_.dragCurrentPos = newPos;
        
        switch (editMode_) {
            case EditMode::NAVIGATE:
                vectorPath_->setPosition(newPos);
                break;
                
            case EditMode::EDIT_WAYPOINT:
                if (touch_.dragWaypointIndex != -1) {
                    moveWaypoint(touch_.dragWaypointIndex, newPos);
                }
                break;
                
            default:
                break;
        }
    }
}

void VectorPathEditor::handleTouchUp(int x, int y, int touchId) {
    if (!initialized_ || !visible_ || !touch_.primaryTouch.active) {
        return;
    }
    
    if (touch_.primaryTouch.trackingId != touchId) {
        return;
    }
    
    // Check for tap gesture
    if (touch_.state == TouchState::PRESSED && isTapGesture(touch_.primaryTouch)) {
        // Handle tap based on current mode
        // Already handled in handleTouchDown for most modes
    }
    
    // Reset touch state
    touch_.state = TouchState::IDLE;
    touch_.primaryTouch.active = false;
    touch_.dragWaypointIndex = -1;
}

void VectorPathEditor::handleMultiTouch(const std::array<TouchPoint, 5>& touches) {
    // Count active touches
    int activeTouches = 0;
    TouchPoint primaryTouch, secondaryTouch;
    
    for (const auto& touch : touches) {
        if (touch.active) {
            if (activeTouches == 0) {
                primaryTouch = touch;
            } else if (activeTouches == 1) {
                secondaryTouch = touch;
            }
            activeTouches++;
        }
    }
    
    if (activeTouches >= 2) {
        touch_.state = TouchState::GESTURE;
        touch_.multiTouchActive = true;
        processMultiTouch(primaryTouch, secondaryTouch);
    } else if (activeTouches == 1) {
        touch_.multiTouchActive = false;
        processSingleTouch(primaryTouch);
    }
}

void VectorPathEditor::handleRotation(int32_t delta, float velocity, bool inDetent) {
    if (!initialized_ || !visible_) {
        return;
    }
    
    switch (editMode_) {
        case EditMode::NAVIGATE:
            // Fine position adjustment
            if (selectedWaypointIndex_ != -1) {
                handleWaypointRotation(delta);
            }
            break;
            
        case EditMode::EDIT_WAYPOINT:
            if (selectedWaypointIndex_ != -1) {
                handleWaypointRotation(delta);
            }
            break;
            
        case EditMode::PLAYBACK:
            handlePlaybackRotation(delta);
            break;
            
        default:
            handleModeRotation(delta);
            break;
    }
}

void VectorPathEditor::handleGesture(SmartKnob::GestureType gesture, float parameter) {
    switch (gesture) {
        case SmartKnob::GestureType::DOUBLE_FLICK:
            // Cycle through edit modes
            {
                int modeIndex = static_cast<int>(editMode_);
                modeIndex = (modeIndex + 1) % 5;
                setEditMode(static_cast<EditMode>(modeIndex));
            }
            break;
            
        case SmartKnob::GestureType::DETENT_DWELL:
            // Toggle playback mode
            if (editMode_ == EditMode::PLAYBACK) {
                setEditMode(EditMode::NAVIGATE);
            } else {
                setEditMode(EditMode::PLAYBACK);
            }
            break;
            
        default:
            break;
    }
}

void VectorPathEditor::selectWaypoint(int waypointIndex) {
    if (waypointIndex < 0 || waypointIndex >= vectorPath_->getWaypointCount()) {
        clearSelection();
        return;
    }
    
    selectedWaypointIndex_ = waypointIndex;
    updateWaypointVisuals();
    
    // Trigger haptic feedback
    if (smartKnob_) {
        smartKnob_->triggerHaptic(SmartKnob::HapticPattern::TICK, 0.5f);
    }
}

void VectorPathEditor::clearSelection() {
    selectedWaypointIndex_ = -1;
    highlightedWaypointIndex_ = -1;
    updateWaypointVisuals();
}

void VectorPathEditor::setCornerLabels(const std::array<std::string, 4>& labels) {
    for (int i = 0; i < 4; i++) {
        cornerLabels_[i].label = labels[i];
    }
}

void VectorPathEditor::setCornerDescriptions(const std::array<std::string, 4>& descriptions) {
    for (int i = 0; i < 4; i++) {
        cornerLabels_[i].description = descriptions[i];
    }
}

void VectorPathEditor::update(float deltaTimeMs) {
    if (!initialized_) {
        return;
    }
    
    uint32_t currentTime = getTimeMs();
    
    // Update animations
    if (modeTransition_.active) {
        modeTransition_.update(currentTime);
    }
    
    if (waypointHighlight_.active) {
        waypointHighlight_.update(currentTime);
    }
    
    // Update path preview if needed
    updatePathPreview();
    updateBlendVisualization();
}

void VectorPathEditor::render() {
    if (!initialized_ || !visible_) {
        return;
    }
    
    renderBackground();
    renderDiamondGrid();
    renderCornerLabels();
    
    if (showPathPreview_) {
        renderPathPreview();
    }
    
    renderWaypoints();
    renderCurrentPosition();
    
    if (showBlendViz_) {
        renderBlendVisualization();
    }
    
    renderEditModeIndicator();
    
    if (editMode_ == EditMode::PLAYBACK) {
        renderPlaybackControls();
    }
}

// Private method implementations

void VectorPathEditor::initializeCornerLabels() {
    // Corner A - Top
    cornerLabels_[0] = {
        "A", "Top corner source",
        VectorPath::Position(0.5f, 0.0f),
        display_.cornerColors[0], true
    };
    
    // Corner B - Right
    cornerLabels_[1] = {
        "B", "Right corner source",
        VectorPath::Position(1.0f, 0.5f),
        display_.cornerColors[1], true
    };
    
    // Corner C - Bottom
    cornerLabels_[2] = {
        "C", "Bottom corner source",
        VectorPath::Position(0.5f, 1.0f),
        display_.cornerColors[2], true
    };
    
    // Corner D - Left
    cornerLabels_[3] = {
        "D", "Left corner source",
        VectorPath::Position(0.0f, 0.5f),
        display_.cornerColors[3], true
    };
}

void VectorPathEditor::updateWaypointVisuals() {
    waypointVisuals_.clear();
    
    if (!vectorPath_) {
        return;
    }
    
    int waypointCount = vectorPath_->getWaypointCount();
    waypointVisuals_.reserve(waypointCount);
    
    for (int i = 0; i < waypointCount; i++) {
        const VectorPath::Waypoint& waypoint = vectorPath_->getWaypoint(i);
        
        WaypointVisual visual;
        visual.position = VectorPath::Position(waypoint.x, waypoint.y);
        visual.size = (i == selectedWaypointIndex_) ? 12.0f : 8.0f;
        visual.selected = (i == selectedWaypointIndex_);
        visual.highlighted = (i == highlightedWaypointIndex_);
        visual.color = visual.selected ? display_.selectedColor : 
                      visual.highlighted ? display_.highlightColor : 
                      display_.waypointColor;
        visual.waypointIndex = i;
        
        waypointVisuals_.push_back(visual);
    }
}

void VectorPathEditor::updatePathPreview() {
    if (!vectorPath_ || vectorPath_->getWaypointCount() < 2) {
        pathPreview_.valid = false;
        return;
    }
    
    // Generate preview points along the path
    for (int i = 0; i < PathPreview::PREVIEW_POINTS; i++) {
        float t = static_cast<float>(i) / (PathPreview::PREVIEW_POINTS - 1);
        pathPreview_.points[i] = vectorPath_->interpolatePosition(t);
    }
    
    pathPreview_.valid = true;
}

void VectorPathEditor::updateBlendVisualization() {
    if (!vectorPath_) {
        return;
    }
    
    VectorPath::CornerBlend currentBlend = vectorPath_->getCurrentBlend();
    
    for (int i = 0; i < 4; i++) {
        blendViz_.weights[i] = currentBlend[i];
    }
    
    blendViz_.totalWeight = blendViz_.weights[0] + blendViz_.weights[1] + 
                           blendViz_.weights[2] + blendViz_.weights[3];
}

VectorPath::Position VectorPathEditor::screenToNormalized(int screenX, int screenY) const {
    // Convert screen coordinates to diamond space
    float relativeX = static_cast<float>(screenX - display_.diamondCenterX) / display_.diamondRadius;
    float relativeY = static_cast<float>(screenY - display_.diamondCenterY) / display_.diamondRadius;
    
    // Transform diamond coordinates to normalized square
    float normalizedX = (relativeX + relativeY) * 0.5f + 0.5f;
    float normalizedY = (-relativeX + relativeY) * 0.5f + 0.5f;
    
    return constrainToDiamond(VectorPath::Position(
        clamp(normalizedX, 0.0f, 1.0f),
        clamp(normalizedY, 0.0f, 1.0f)
    ));
}

void VectorPathEditor::normalizedToScreen(const VectorPath::Position& normalized, int& screenX, int& screenY) const {
    // Transform normalized square to diamond coordinates
    float relativeX = (normalized.x - 0.5f) - (normalized.y - 0.5f);
    float relativeY = (normalized.x - 0.5f) + (normalized.y - 0.5f);
    
    // Convert to screen coordinates
    screenX = display_.diamondCenterX + static_cast<int>(relativeX * display_.diamondRadius);
    screenY = display_.diamondCenterY + static_cast<int>(relativeY * display_.diamondRadius);
}

bool VectorPathEditor::isPointInDiamond(int screenX, int screenY) const {
    int deltaX = std::abs(screenX - display_.diamondCenterX);
    int deltaY = std::abs(screenY - display_.diamondCenterY);
    
    return (deltaX + deltaY) <= display_.diamondRadius;
}

VectorPath::Position VectorPathEditor::constrainToDiamond(const VectorPath::Position& pos) const {
    // Use VectorPath's diamond constraint method
    if (vectorPath_) {
        return vectorPath_->constrainToDiamond(pos);
    }
    
    // Fallback diamond constraint
    float centerX = pos.x - 0.5f;
    float centerY = pos.y - 0.5f;
    float manhattanDist = std::abs(centerX) + std::abs(centerY);
    
    if (manhattanDist > 0.5f) {
        float scale = 0.5f / manhattanDist;
        centerX *= scale;
        centerY *= scale;
    }
    
    return VectorPath::Position(centerX + 0.5f, centerY + 0.5f);
}

int VectorPathEditor::findNearestWaypoint(int screenX, int screenY, float maxDistance) const {
    int nearestIndex = -1;
    float minDistance = maxDistance;
    
    for (const auto& visual : waypointVisuals_) {
        int waypointScreenX, waypointScreenY;
        normalizedToScreen(visual.position, waypointScreenX, waypointScreenY);
        
        float distance = std::sqrt(
            std::pow(screenX - waypointScreenX, 2) +
            std::pow(screenY - waypointScreenY, 2)
        );
        
        if (distance < minDistance) {
            minDistance = distance;
            nearestIndex = visual.waypointIndex;
        }
    }
    
    return nearestIndex;
}

void VectorPathEditor::createWaypointAtPosition(const VectorPath::Position& pos) {
    if (!vectorPath_) {
        return;
    }
    
    VectorPath::Waypoint newWaypoint(pos.x, pos.y, 0.5f);
    vectorPath_->addWaypoint(newWaypoint);
    
    // Select the new waypoint
    selectWaypoint(vectorPath_->getWaypointCount() - 1);
    
    // Trigger haptic feedback
    if (smartKnob_) {
        smartKnob_->triggerHaptic(SmartKnob::HapticPattern::BUMP, 0.7f);
    }
}

void VectorPathEditor::deleteWaypoint(int waypointIndex) {
    if (!vectorPath_ || waypointIndex < 0 || waypointIndex >= vectorPath_->getWaypointCount()) {
        return;
    }
    
    vectorPath_->removeWaypoint(waypointIndex);
    
    // Clear selection if we deleted the selected waypoint
    if (waypointIndex == selectedWaypointIndex_) {
        clearSelection();
    }
    
    // Trigger haptic feedback
    if (smartKnob_) {
        smartKnob_->triggerHaptic(SmartKnob::HapticPattern::THUD, 0.8f);
    }
}

void VectorPathEditor::moveWaypoint(int waypointIndex, const VectorPath::Position& newPos) {
    if (!vectorPath_ || waypointIndex < 0 || waypointIndex >= vectorPath_->getWaypointCount()) {
        return;
    }
    
    VectorPath::Waypoint waypoint = vectorPath_->getWaypoint(waypointIndex);
    waypoint.x = newPos.x;
    waypoint.y = newPos.y;
    
    vectorPath_->setWaypoint(waypointIndex, waypoint);
}

// Touch gesture processing
void VectorPathEditor::processSingleTouch(const TouchPoint& touch) {
    // Handle single touch based on current mode
    // This is already handled in the main touch handlers
}

void VectorPathEditor::processMultiTouch(const TouchPoint& touch1, const TouchPoint& touch2) {
    // Handle multi-touch gestures like pinch-to-zoom
    float distance = calculateTouchDistance(touch1, touch2);
    
    if (touch_.gestureStartTime == 0) {
        touch_.gestureStartTime = getTimeMs();
        touch_.gestureStartDistance = distance;
    }
    
    // Calculate zoom factor
    float zoomFactor = distance / touch_.gestureStartDistance;
    
    // Apply zoom to diamond radius (could be used for detail editing)
    // For now, we'll just store this for potential future use
}

float VectorPathEditor::calculateTouchDistance(const TouchPoint& t1, const TouchPoint& t2) const {
    return std::sqrt(std::pow(t1.x - t2.x, 2) + std::pow(t1.y - t2.y, 2));
}

bool VectorPathEditor::isTapGesture(const TouchPoint& touch) const {
    uint32_t duration = getTimeMs() - touch.timestamp;
    return duration < TAP_MAX_DURATION;
}

bool VectorPathEditor::isDragGesture(const TouchPoint& touch) const {
    // Already calculated in handleTouchMove
    return touch_.state == TouchState::DRAGGING;
}

// SmartKnob integration
void VectorPathEditor::handleWaypointRotation(int32_t delta) {
    // Fine adjustment of selected waypoint position
    // Implementation would adjust waypoint position based on encoder input
}

void VectorPathEditor::handleModeRotation(int32_t delta) {
    // Cycle through edit modes with encoder
    if (delta > 0) {
        int modeIndex = static_cast<int>(editMode_);
        modeIndex = (modeIndex + 1) % 5;
        setEditMode(static_cast<EditMode>(modeIndex));
    } else if (delta < 0) {
        int modeIndex = static_cast<int>(editMode_);
        modeIndex = (modeIndex - 1 + 5) % 5;
        setEditMode(static_cast<EditMode>(modeIndex));
    }
}

void VectorPathEditor::handlePlaybackRotation(int32_t delta) {
    // Control playback position/rate
    if (vectorPath_) {
        float currentPos = vectorPath_->getPlaybackPosition();
        float deltaPos = static_cast<float>(delta) / 16384.0f;
        vectorPath_->setPlaybackPosition(clamp(currentPos + deltaPos, 0.0f, 1.0f));
    }
}

// Rendering methods - Placeholder implementations for TouchGFX

void VectorPathEditor::renderBackground() {
    drawRectangle(display_.x, display_.y, display_.width, display_.height, display_.backgroundColor);
}

void VectorPathEditor::renderDiamondGrid() {
    // Draw diamond outline
    drawDiamond(display_.diamondCenterX, display_.diamondCenterY, display_.diamondRadius, display_.diamondColor);
    
    // Draw grid lines
    int halfRadius = display_.diamondRadius / 2;
    drawDiamond(display_.diamondCenterX, display_.diamondCenterY, halfRadius, display_.gridColor);
    
    // Draw center cross
    drawLine(display_.diamondCenterX - 10, display_.diamondCenterY, 
             display_.diamondCenterX + 10, display_.diamondCenterY, display_.gridColor);
    drawLine(display_.diamondCenterX, display_.diamondCenterY - 10,
             display_.diamondCenterX, display_.diamondCenterY + 10, display_.gridColor);
}

void VectorPathEditor::renderCornerLabels() {
    for (const auto& label : cornerLabels_) {
        if (label.visible) {
            drawCornerLabel(label);
        }
    }
}

void VectorPathEditor::renderBlendVisualization() {
    drawBlendWeights(blendViz_);
}

void VectorPathEditor::renderPathPreview() {
    if (pathPreview_.valid) {
        drawPath(pathPreview_);
    }
}

void VectorPathEditor::renderWaypoints() {
    for (const auto& waypoint : waypointVisuals_) {
        drawWaypoint(waypoint);
    }
}

void VectorPathEditor::renderCurrentPosition() {
    if (!vectorPath_) {
        return;
    }
    
    VectorPath::Position currentPos = vectorPath_->getPosition();
    int screenX, screenY;
    normalizedToScreen(currentPos, screenX, screenY);
    
    drawCircle(screenX, screenY, 6, pathPreview_.currentPosColor);
}

void VectorPathEditor::renderEditModeIndicator() {
    // Draw mode indicator in corner
    std::string modeText;
    switch (editMode_) {
        case EditMode::NAVIGATE: modeText = "NAV"; break;
        case EditMode::ADD_WAYPOINT: modeText = "ADD"; break;
        case EditMode::EDIT_WAYPOINT: modeText = "EDIT"; break;
        case EditMode::DELETE_WAYPOINT: modeText = "DEL"; break;
        case EditMode::PLAYBACK: modeText = "PLAY"; break;
    }
    
    drawText(modeText, display_.x + 10, display_.y + 10, display_.textColor, 14);
}

void VectorPathEditor::renderPlaybackControls() {
    // Render playback-specific UI elements
    if (vectorPath_ && vectorPath_->isPlaying()) {
        drawText("PLAYING", display_.x + display_.width - 80, display_.y + 10, display_.highlightColor, 12);
    }
}

// Animation implementation
void VectorPathEditor::Animation::start(float durationMs) {
    duration = durationMs;
    progress = 0.0f;
    active = true;
    startTime = 0;
}

float VectorPathEditor::Animation::update(uint32_t currentTime) {
    if (!active) return progress;
    
    if (startTime == 0) {
        startTime = currentTime;
    }
    
    uint32_t elapsed = currentTime - startTime;
    
    if (elapsed >= static_cast<uint32_t>(duration)) {
        progress = 1.0f;
        active = false;
    } else {
        progress = static_cast<float>(elapsed) / duration;
    }
    
    return progress;
}

// Utility functions
uint32_t VectorPathEditor::getTimeMs() const {
#ifdef STM32H7
    return HAL_GetTick();
#else
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
#endif
}

float VectorPathEditor::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}

uint16_t VectorPathEditor::blendColors(uint16_t color1, uint16_t color2, float blend) const {
    blend = clamp(blend, 0.0f, 1.0f);
    
    uint8_t r1 = (color1 >> 11) & 0x1F;
    uint8_t g1 = (color1 >> 5) & 0x3F;
    uint8_t b1 = color1 & 0x1F;
    
    uint8_t r2 = (color2 >> 11) & 0x1F;
    uint8_t g2 = (color2 >> 5) & 0x3F;
    uint8_t b2 = color2 & 0x1F;
    
    uint8_t r = static_cast<uint8_t>(r1 + blend * (r2 - r1));
    uint8_t g = static_cast<uint8_t>(g1 + blend * (g2 - g1));
    uint8_t b = static_cast<uint8_t>(b1 + blend * (b2 - b1));
    
    return (r << 11) | (g << 5) | b;
}

float VectorPathEditor::clamp(float value, float min, float max) const {
    return std::max(min, std::min(max, value));
}

// Drawing primitive placeholders (would be implemented with TouchGFX)
void VectorPathEditor::drawLine(int x1, int y1, int x2, int y2, uint16_t color, int thickness) {
#ifdef STM32H7
    // TouchGFX line implementation
#endif
}

void VectorPathEditor::drawCircle(int x, int y, int radius, uint16_t color, bool filled) {
#ifdef STM32H7
    // TouchGFX circle implementation
#endif
}

void VectorPathEditor::drawRectangle(int x, int y, int width, int height, uint16_t color, bool filled) {
#ifdef STM32H7
    // TouchGFX rectangle implementation
#endif
}

void VectorPathEditor::drawText(const std::string& text, int x, int y, uint16_t color, int fontSize) {
#ifdef STM32H7
    // TouchGFX text implementation
#endif
}

void VectorPathEditor::drawCenteredText(const std::string& text, int x, int y, int width, uint16_t color, int fontSize) {
#ifdef STM32H7
    // TouchGFX centered text implementation
#endif
}

void VectorPathEditor::drawDiamond(int centerX, int centerY, int radius, uint16_t color, bool filled) {
    // Draw diamond shape with 4 lines
    drawLine(centerX, centerY - radius, centerX + radius, centerY, color, 2);
    drawLine(centerX + radius, centerY, centerX, centerY + radius, color, 2);
    drawLine(centerX, centerY + radius, centerX - radius, centerY, color, 2);
    drawLine(centerX - radius, centerY, centerX, centerY - radius, color, 2);
}

void VectorPathEditor::drawWaypoint(const WaypointVisual& waypoint) {
    int screenX, screenY;
    normalizedToScreen(waypoint.position, screenX, screenY);
    
    uint16_t color = waypoint.selected ? display_.selectedColor :
                    waypoint.highlighted ? display_.highlightColor :
                    waypoint.color;
    
    drawCircle(screenX, screenY, static_cast<int>(waypoint.size), color);
    
    if (waypoint.selected) {
        drawCircle(screenX, screenY, static_cast<int>(waypoint.size) + 3, color, false);
    }
}

void VectorPathEditor::drawPath(const PathPreview& preview) {
    if (!preview.valid) {
        return;
    }
    
    for (int i = 1; i < PathPreview::PREVIEW_POINTS; i++) {
        int x1, y1, x2, y2;
        normalizedToScreen(preview.points[i-1], x1, y1);
        normalizedToScreen(preview.points[i], x2, y2);
        
        drawLine(x1, y1, x2, y2, preview.pathColor, preview.lineThickness);
    }
}

void VectorPathEditor::drawBlendWeights(const BlendVisualization& blend) {
    if (!blend.showBars && !blend.showNumbers) {
        return;
    }
    
    // Draw blend weight bars in corners
    const std::array<std::pair<int, int>, 4> cornerPositions = {{
        {display_.diamondCenterX, display_.diamondCenterY - display_.diamondRadius - 20}, // A - Top
        {display_.diamondCenterX + display_.diamondRadius + 20, display_.diamondCenterY}, // B - Right
        {display_.diamondCenterX, display_.diamondCenterY + display_.diamondRadius + 20}, // C - Bottom
        {display_.diamondCenterX - display_.diamondRadius - 20, display_.diamondCenterY}  // D - Left
    }};
    
    for (int i = 0; i < 4; i++) {
        if (blend.showBars) {
            int barHeight = static_cast<int>(blend.weights[i] * 30);
            drawRectangle(cornerPositions[i].first - 5, cornerPositions[i].second - barHeight/2,
                         10, barHeight, blend.colors[i]);
        }
        
        if (blend.showNumbers) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << (blend.weights[i] * 100) << "%";
            drawCenteredText(ss.str(), cornerPositions[i].first - 15, cornerPositions[i].second + 15,
                           30, display_.textColor, 10);
        }
    }
}

void VectorPathEditor::drawCornerLabel(const CornerLabel& label) {
    int screenX, screenY;
    normalizedToScreen(label.position, screenX, screenY);
    
    // Offset label from corner position
    int offsetX = 0, offsetY = 0;
    if (label.position.x < 0.5f) offsetX = -30; // Left side
    if (label.position.x > 0.5f) offsetX = 10;  // Right side
    if (label.position.y < 0.5f) offsetY = -20; // Top side
    if (label.position.y > 0.5f) offsetY = 10;  // Bottom side
    
    drawText(label.label, screenX + offsetX, screenY + offsetY, label.color, 16);
}