#include "PatternSelection.h"
#include <algorithm>

PatternSelection::PatternSelection() {
    state_ = SelectionState::NONE;
    currentBounds_ = SelectionBounds();
    selectionStart_ = SelectionBounds();
    visualConfig_ = SelectionVisualConfig();
    
    // Default constraints
    minTracks_ = DEFAULT_MIN_TRACKS;
    minSteps_ = DEFAULT_MIN_STEPS;
    maxTracks_ = DEFAULT_MAX_TRACKS;
    maxSteps_ = DEFAULT_MAX_STEPS;
    
    // Default sequencer dimensions
    sequencerMaxTracks_ = 16;
    sequencerMaxSteps_ = 16;
}

// Selection management
void PatternSelection::startSelection(uint16_t startTrack, uint16_t startStep) {
    // Constrain starting point to sequencer bounds
    startTrack = std::min(startTrack, static_cast<uint16_t>(sequencerMaxTracks_ - 1));
    startStep = std::min(startStep, static_cast<uint16_t>(sequencerMaxSteps_ - 1));
    
    state_ = SelectionState::SELECTING;
    selectionStart_ = SelectionBounds(startTrack, startTrack, startStep, startStep);
    currentBounds_ = selectionStart_;
    
    notifySelectionStart();
}

void PatternSelection::updateSelection(uint16_t currentTrack, uint16_t currentStep) {
    if (state_ != SelectionState::SELECTING) {
        return;
    }
    
    // Constrain current point to sequencer bounds
    currentTrack = std::min(currentTrack, static_cast<uint16_t>(sequencerMaxTracks_ - 1));
    currentStep = std::min(currentStep, static_cast<uint16_t>(sequencerMaxSteps_ - 1));
    
    // Calculate selection bounds from start to current position
    currentBounds_.startTrack = std::min(selectionStart_.startTrack, currentTrack);
    currentBounds_.endTrack = std::max(selectionStart_.startTrack, currentTrack);
    currentBounds_.startStep = std::min(selectionStart_.startStep, currentStep);
    currentBounds_.endStep = std::max(selectionStart_.startStep, currentStep);
    
    // Constrain to valid bounds
    currentBounds_ = constrainBounds(currentBounds_);
    
    notifySelectionUpdate();
}

void PatternSelection::completeSelection() {
    if (state_ != SelectionState::SELECTING) {
        return;
    }
    
    if (isValidSelection(currentBounds_)) {
        state_ = SelectionState::SELECTED;
        notifySelectionComplete();
    } else {
        state_ = SelectionState::INVALID;
        // Invalid selections are automatically cancelled after a brief display
        cancelSelection();
    }
}

void PatternSelection::cancelSelection() {
    if (state_ == SelectionState::NONE) {
        return;
    }
    
    state_ = SelectionState::NONE;
    currentBounds_ = SelectionBounds();
    selectionStart_ = SelectionBounds();
    
    notifySelectionCancel();
}

void PatternSelection::clearSelection() {
    cancelSelection();
}

// Selection manipulation
void PatternSelection::setSelection(const SelectionBounds& bounds) {
    if (isValidSelection(bounds)) {
        currentBounds_ = constrainBounds(bounds);
        state_ = SelectionState::SELECTED;
        notifySelectionComplete();
    }
}

void PatternSelection::selectAll(uint16_t maxTracks, uint16_t maxSteps) {
    SelectionBounds bounds(0, maxTracks - 1, 0, maxSteps - 1);
    setSelection(bounds);
}

void PatternSelection::selectTrack(uint16_t trackIndex, uint16_t maxSteps) {
    if (trackIndex < sequencerMaxTracks_) {
        SelectionBounds bounds(trackIndex, trackIndex, 0, maxSteps - 1);
        setSelection(bounds);
    }
}

void PatternSelection::selectStep(uint16_t stepIndex, uint16_t maxTracks) {
    if (stepIndex < sequencerMaxSteps_) {
        SelectionBounds bounds(0, maxTracks - 1, stepIndex, stepIndex);
        setSelection(bounds);
    }
}

void PatternSelection::expandSelection(int16_t trackDelta, int16_t stepDelta) {
    if (!hasSelection()) {
        return;
    }
    
    SelectionBounds newBounds = currentBounds_;
    
    // Expand tracks
    if (trackDelta > 0) {
        newBounds.endTrack = std::min(static_cast<uint16_t>(newBounds.endTrack + trackDelta), 
                                     static_cast<uint16_t>(sequencerMaxTracks_ - 1));
    } else if (trackDelta < 0) {
        int16_t newStartTrack = static_cast<int16_t>(newBounds.startTrack) + trackDelta;
        newBounds.startTrack = std::max(newStartTrack, static_cast<int16_t>(0));
    }
    
    // Expand steps
    if (stepDelta > 0) {
        newBounds.endStep = std::min(static_cast<uint16_t>(newBounds.endStep + stepDelta), 
                                    static_cast<uint16_t>(sequencerMaxSteps_ - 1));
    } else if (stepDelta < 0) {
        int16_t newStartStep = static_cast<int16_t>(newBounds.startStep) + stepDelta;
        newBounds.startStep = std::max(newStartStep, static_cast<int16_t>(0));
    }
    
    if (isValidSelection(newBounds)) {
        currentBounds_ = newBounds;
        notifySelectionUpdate();
    }
}

void PatternSelection::shrinkSelection(int16_t trackDelta, int16_t stepDelta) {
    expandSelection(-trackDelta, -stepDelta);
}

// Validation and constraints
bool PatternSelection::isValidSelection(const SelectionBounds& bounds) const {
    if (!bounds.isValid()) {
        return false;
    }
    
    // Check minimum constraints
    if (bounds.getTrackCount() < minTracks_ || bounds.getStepCount() < minSteps_) {
        return false;
    }
    
    // Check maximum constraints
    if (bounds.getTrackCount() > maxTracks_ || bounds.getStepCount() > maxSteps_) {
        return false;
    }
    
    // Check sequencer bounds
    if (bounds.endTrack >= sequencerMaxTracks_ || bounds.endStep >= sequencerMaxSteps_) {
        return false;
    }
    
    return true;
}

bool PatternSelection::validateCurrentSelection() {
    bool valid = isValidSelection(currentBounds_);
    
    if (valid && state_ == SelectionState::INVALID) {
        state_ = SelectionState::SELECTED;
    } else if (!valid && state_ == SelectionState::SELECTED) {
        state_ = SelectionState::INVALID;
    }
    
    return valid;
}

void PatternSelection::setMinimumSelection(uint16_t minTracks, uint16_t minSteps) {
    minTracks_ = std::max(static_cast<uint16_t>(1), minTracks);
    minSteps_ = std::max(static_cast<uint16_t>(1), minSteps);
    
    // Validate current selection with new constraints
    validateCurrentSelection();
}

void PatternSelection::setMaximumSelection(uint16_t maxTracks, uint16_t maxSteps) {
    maxTracks_ = std::min(maxTracks, static_cast<uint16_t>(DEFAULT_MAX_TRACKS));
    maxSteps_ = std::min(maxSteps, static_cast<uint16_t>(DEFAULT_MAX_STEPS));
    
    // Validate current selection with new constraints
    validateCurrentSelection();
}

// Cell queries
bool PatternSelection::isCellSelected(uint16_t track, uint16_t step) const {
    if (!hasSelection()) {
        return false;
    }
    
    return (track >= currentBounds_.startTrack && track <= currentBounds_.endTrack &&
            step >= currentBounds_.startStep && step <= currentBounds_.endStep);
}

bool PatternSelection::isTrackSelected(uint16_t track) const {
    if (!hasSelection()) {
        return false;
    }
    
    return (track >= currentBounds_.startTrack && track <= currentBounds_.endTrack);
}

bool PatternSelection::isStepSelected(uint16_t step) const {
    if (!hasSelection()) {
        return false;
    }
    
    return (step >= currentBounds_.startStep && step <= currentBounds_.endStep);
}

std::vector<std::pair<uint16_t, uint16_t>> PatternSelection::getSelectedCells() const {
    std::vector<std::pair<uint16_t, uint16_t>> cells;
    
    if (!hasSelection()) {
        return cells;
    }
    
    cells.reserve(currentBounds_.getTotalCells());
    
    for (uint16_t track = currentBounds_.startTrack; track <= currentBounds_.endTrack; ++track) {
        for (uint16_t step = currentBounds_.startStep; step <= currentBounds_.endStep; ++step) {
            cells.emplace_back(track, step);
        }
    }
    
    return cells;
}

std::vector<uint16_t> PatternSelection::getSelectedTracks() const {
    std::vector<uint16_t> tracks;
    
    if (!hasSelection()) {
        return tracks;
    }
    
    tracks.reserve(currentBounds_.getTrackCount());
    
    for (uint16_t track = currentBounds_.startTrack; track <= currentBounds_.endTrack; ++track) {
        tracks.push_back(track);
    }
    
    return tracks;
}

std::vector<uint16_t> PatternSelection::getSelectedSteps() const {
    std::vector<uint16_t> steps;
    
    if (!hasSelection()) {
        return steps;
    }
    
    steps.reserve(currentBounds_.getStepCount());
    
    for (uint16_t step = currentBounds_.startStep; step <= currentBounds_.endStep; ++step) {
        steps.push_back(step);
    }
    
    return steps;
}

// Visual configuration
void PatternSelection::setVisualConfig(const SelectionVisualConfig& config) {
    visualConfig_ = config;
}

// Integration with sequencer
void PatternSelection::setSequencerDimensions(uint16_t maxTracks, uint16_t maxSteps) {
    sequencerMaxTracks_ = maxTracks;
    sequencerMaxSteps_ = maxSteps;
    
    // Validate current selection with new sequencer bounds
    validateCurrentSelection();
}

void PatternSelection::updateFromTouch(uint16_t x, uint16_t y, bool pressed, bool dragging) {
    if (!coordinateToGrid_) {
        return;  // No coordinate conversion available
    }
    
    auto [track, step] = coordinateToGrid_(x, y);
    
    if (pressed && !dragging) {
        // Start new selection
        startSelection(track, step);
    } else if (pressed && dragging && state_ == SelectionState::SELECTING) {
        // Update selection during drag
        updateSelection(track, step);
    } else if (!pressed && state_ == SelectionState::SELECTING) {
        // Complete selection on release
        completeSelection();
    }
}

void PatternSelection::setGridToCoordinateCallback(std::function<std::pair<uint16_t, uint16_t>(uint16_t, uint16_t)> callback) {
    gridToCoordinate_ = callback;
}

void PatternSelection::setCoordinateToGridCallback(std::function<std::pair<uint16_t, uint16_t>(uint16_t, uint16_t)> callback) {
    coordinateToGrid_ = callback;
}

// Callbacks for selection events
void PatternSelection::setSelectionStartCallback(SelectionStartCallback callback) {
    startCallback_ = callback;
}

void PatternSelection::setSelectionUpdateCallback(SelectionUpdateCallback callback) {
    updateCallback_ = callback;
}

void PatternSelection::setSelectionCompleteCallback(SelectionCompleteCallback callback) {
    completeCallback_ = callback;
}

void PatternSelection::setSelectionCancelCallback(SelectionCancelCallback callback) {
    cancelCallback_ = callback;
}

// Drawing and visualization support
void PatternSelection::drawSelection(void* graphics) const {
    if (state_ == SelectionState::NONE) {
        return;
    }
    
    drawSelectionBounds(graphics, currentBounds_);
    
    if (visualConfig_.showCornerMarkers) {
        drawCornerMarkers(graphics, currentBounds_);
    }
    
    if (visualConfig_.showDimensionText) {
        drawDimensionText(graphics, currentBounds_);
    }
}

void PatternSelection::drawSelectionBounds(void* graphics, const SelectionBounds& bounds) const {
    // In real implementation, this would use TouchGFX Graphics to:
    // 1. Draw selection highlight rectangle with selectionColor and selectionAlpha
    // 2. Draw boundary lines with boundaryColor and boundaryWidth
    // 3. Use invalidColor if state is INVALID
}

void PatternSelection::drawCornerMarkers(void* graphics, const SelectionBounds& bounds) const {
    // In real implementation, draw small corner markers at selection boundaries
}

void PatternSelection::drawDimensionText(void* graphics, const SelectionBounds& bounds) const {
    // In real implementation, draw text showing selection dimensions (e.g., "4×8")
}

// Statistics
uint32_t PatternSelection::getSelectedCellCount() const {
    return hasSelection() ? currentBounds_.getTotalCells() : 0;
}

float PatternSelection::getSelectionDensity() const {
    if (!hasSelection()) {
        return 0.0f;
    }
    
    uint32_t totalCells = sequencerMaxTracks_ * sequencerMaxSteps_;
    if (totalCells == 0) {
        return 0.0f;
    }
    
    return static_cast<float>(getSelectedCellCount()) / static_cast<float>(totalCells);
}

// Helper methods
PatternSelection::SelectionBounds PatternSelection::constrainBounds(const SelectionBounds& bounds) const {
    SelectionBounds constrained = bounds;
    
    // Constrain to sequencer dimensions
    constrained.startTrack = std::min(constrained.startTrack, static_cast<uint16_t>(sequencerMaxTracks_ - 1));
    constrained.endTrack = std::min(constrained.endTrack, static_cast<uint16_t>(sequencerMaxTracks_ - 1));
    constrained.startStep = std::min(constrained.startStep, static_cast<uint16_t>(sequencerMaxSteps_ - 1));
    constrained.endStep = std::min(constrained.endStep, static_cast<uint16_t>(sequencerMaxSteps_ - 1));
    
    // Ensure valid bounds
    if (constrained.startTrack > constrained.endTrack) {
        std::swap(constrained.startTrack, constrained.endTrack);
    }
    if (constrained.startStep > constrained.endStep) {
        std::swap(constrained.startStep, constrained.endStep);
    }
    
    return constrained;
}

void PatternSelection::notifySelectionStart() {
    if (startCallback_) {
        startCallback_(currentBounds_);
    }
}

void PatternSelection::notifySelectionUpdate() {
    if (updateCallback_) {
        updateCallback_(currentBounds_);
    }
}

void PatternSelection::notifySelectionComplete() {
    if (completeCallback_) {
        completeCallback_(currentBounds_);
    }
}

void PatternSelection::notifySelectionCancel() {
    if (cancelCallback_) {
        cancelCallback_();
    }
}