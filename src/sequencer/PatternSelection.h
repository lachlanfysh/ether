#pragma once
#include <cstdint>
#include <vector>
#include <functional>

/**
 * PatternSelection - Multi-track rectangular region selection system
 * 
 * Provides comprehensive pattern selection functionality for multi-track sequences:
 * - Rectangular selection regions spanning multiple tracks and steps
 * - Visual selection highlighting with clear start/end boundaries
 * - Selection validation (minimum 1 step/1 track, no maximum)
 * - Integration with tape squashing and pattern editing operations
 * - Touch-based selection with drag gestures and precise boundaries
 * 
 * Features:
 * - Multi-track rectangular selections (any combination of tracks/steps)
 * - Visual feedback with selection highlighting and boundary indicators
 * - Selection validation and constraint enforcement
 * - Integration with sequencer pattern data
 * - Support for copy/paste, tape squashing, and pattern manipulation
 * - Efficient selection representation and boundary calculations
 */
class PatternSelection {
public:
    // Selection boundary definition
    struct SelectionBounds {
        uint16_t startTrack;        // First track in selection (0-based)
        uint16_t endTrack;          // Last track in selection (inclusive)
        uint16_t startStep;         // First step in selection (0-based)
        uint16_t endStep;           // Last step in selection (inclusive)
        
        SelectionBounds() : startTrack(0), endTrack(0), startStep(0), endStep(0) {}
        SelectionBounds(uint16_t sTrack, uint16_t eTrack, uint16_t sStep, uint16_t eStep) :
            startTrack(sTrack), endTrack(eTrack), startStep(sStep), endStep(eStep) {}
        
        bool isValid() const {
            return (endTrack >= startTrack) && (endStep >= startStep);
        }
        
        uint16_t getTrackCount() const { return endTrack - startTrack + 1; }
        uint16_t getStepCount() const { return endStep - startStep + 1; }
        uint32_t getTotalCells() const { return getTrackCount() * getStepCount(); }
    };
    
    // Selection state
    enum class SelectionState {
        NONE,           // No active selection
        SELECTING,      // Currently selecting (drag in progress)
        SELECTED,       // Selection completed and active
        INVALID         // Invalid selection (failed validation)
    };
    
    // Selection visualization configuration
    struct SelectionVisualConfig {
        uint32_t selectionColor;        // Selection highlight color
        uint32_t boundaryColor;         // Selection boundary color
        uint32_t invalidColor;          // Invalid selection color
        uint8_t selectionAlpha;         // Selection transparency (0-255)
        uint8_t boundaryWidth;          // Boundary line width (pixels)
        bool showCornerMarkers;         // Show corner selection markers
        bool showDimensionText;         // Show selection dimensions
        
        SelectionVisualConfig() :
            selectionColor(0x3366FF),   // Blue
            boundaryColor(0xFFFFFF),    // White
            invalidColor(0xFF3333),     // Red
            selectionAlpha(64),         // 25% transparent
            boundaryWidth(2),
            showCornerMarkers(true),
            showDimensionText(true) {}
    };
    
    PatternSelection();
    ~PatternSelection() = default;
    
    // Selection management
    void startSelection(uint16_t startTrack, uint16_t startStep);
    void updateSelection(uint16_t currentTrack, uint16_t currentStep);
    void completeSelection();
    void cancelSelection();
    void clearSelection();
    
    // Selection manipulation
    void setSelection(const SelectionBounds& bounds);
    void selectAll(uint16_t maxTracks, uint16_t maxSteps);
    void selectTrack(uint16_t trackIndex, uint16_t maxSteps);
    void selectStep(uint16_t stepIndex, uint16_t maxTracks);
    void expandSelection(int16_t trackDelta, int16_t stepDelta);
    void shrinkSelection(int16_t trackDelta, int16_t stepDelta);
    
    // Selection queries
    bool hasSelection() const { return state_ == SelectionState::SELECTED; }
    bool isSelecting() const { return state_ == SelectionState::SELECTING; }
    SelectionState getSelectionState() const { return state_; }
    const SelectionBounds& getSelectionBounds() const { return currentBounds_; }
    
    // Validation and constraints
    bool isValidSelection(const SelectionBounds& bounds) const;
    bool validateCurrentSelection();
    void setMinimumSelection(uint16_t minTracks, uint16_t minSteps);
    void setMaximumSelection(uint16_t maxTracks, uint16_t maxSteps);
    
    // Cell queries
    bool isCellSelected(uint16_t track, uint16_t step) const;
    bool isTrackSelected(uint16_t track) const;
    bool isStepSelected(uint16_t step) const;
    std::vector<std::pair<uint16_t, uint16_t>> getSelectedCells() const;
    std::vector<uint16_t> getSelectedTracks() const;
    std::vector<uint16_t> getSelectedSteps() const;
    
    // Visual configuration
    void setVisualConfig(const SelectionVisualConfig& config);
    const SelectionVisualConfig& getVisualConfig() const { return visualConfig_; }
    
    // Integration with sequencer
    void setSequencerDimensions(uint16_t maxTracks, uint16_t maxSteps);
    void updateFromTouch(uint16_t x, uint16_t y, bool pressed, bool dragging);
    void setGridToCoordinateCallback(std::function<std::pair<uint16_t, uint16_t>(uint16_t, uint16_t)> callback);
    void setCoordinateToGridCallback(std::function<std::pair<uint16_t, uint16_t>(uint16_t, uint16_t)> callback);
    
    // Callbacks for selection events
    using SelectionStartCallback = std::function<void(const SelectionBounds&)>;
    using SelectionUpdateCallback = std::function<void(const SelectionBounds&)>;
    using SelectionCompleteCallback = std::function<void(const SelectionBounds&)>;
    using SelectionCancelCallback = std::function<void()>;
    
    void setSelectionStartCallback(SelectionStartCallback callback);
    void setSelectionUpdateCallback(SelectionUpdateCallback callback);
    void setSelectionCompleteCallback(SelectionCompleteCallback callback);
    void setSelectionCancelCallback(SelectionCancelCallback callback);
    
    // Drawing and visualization support
    void drawSelection(void* graphics) const;  // Would use TouchGFX Graphics in real implementation
    void drawSelectionBounds(void* graphics, const SelectionBounds& bounds) const;
    void drawCornerMarkers(void* graphics, const SelectionBounds& bounds) const;
    void drawDimensionText(void* graphics, const SelectionBounds& bounds) const;
    
    // Statistics
    uint32_t getSelectedCellCount() const;
    float getSelectionDensity() const;  // Percentage of total grid selected
    
private:
    // Selection state
    SelectionState state_;
    SelectionBounds currentBounds_;
    SelectionBounds selectionStart_;    // Starting point for drag selection
    SelectionVisualConfig visualConfig_;
    
    // Constraints
    uint16_t minTracks_;
    uint16_t minSteps_;
    uint16_t maxTracks_;
    uint16_t maxSteps_;
    
    // Sequencer integration
    uint16_t sequencerMaxTracks_;
    uint16_t sequencerMaxSteps_;
    
    // Coordinate conversion callbacks
    std::function<std::pair<uint16_t, uint16_t>(uint16_t, uint16_t)> gridToCoordinate_;
    std::function<std::pair<uint16_t, uint16_t>(uint16_t, uint16_t)> coordinateToGrid_;
    
    // Event callbacks
    SelectionStartCallback startCallback_;
    SelectionUpdateCallback updateCallback_;
    SelectionCompleteCallback completeCallback_;
    SelectionCancelCallback cancelCallback_;
    
    // Helper methods
    SelectionBounds constrainBounds(const SelectionBounds& bounds) const;
    void notifySelectionStart();
    void notifySelectionUpdate();
    void notifySelectionComplete();
    void notifySelectionCancel();
    
    // Constants
    static constexpr uint16_t DEFAULT_MIN_TRACKS = 1;
    static constexpr uint16_t DEFAULT_MIN_STEPS = 1;
    static constexpr uint16_t DEFAULT_MAX_TRACKS = 64;
    static constexpr uint16_t DEFAULT_MAX_STEPS = 64;
};