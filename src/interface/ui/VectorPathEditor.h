#pragma once
#include "../synthesis/VectorPath.h"
#include "../platform/hardware/SmartKnob.h"
#include <array>
#include <functional>
#include <string>

/**
 * VectorPathEditor - TouchGFX interface for Vector Path manipulation
 * 
 * Features:
 * - Diamond-shaped touch surface for 2D parameter control
 * - Visual waypoint manipulation with touch drag
 * - Real-time path preview with Catmull-Rom interpolation
 * - Corner source labeling and blend visualization
 * - SmartKnob integration for fine waypoint adjustment
 * - Path playback controls and rate adjustment
 * - Visual feedback for blend weights and position
 */
class VectorPathEditor {
public:
    enum class EditMode {
        NAVIGATE,       // Navigate existing path
        ADD_WAYPOINT,   // Add new waypoints
        EDIT_WAYPOINT,  // Edit existing waypoints
        DELETE_WAYPOINT,// Delete waypoints
        PLAYBACK        // Playback control mode
    };
    
    enum class TouchState {
        IDLE,           // No touch input
        PRESSED,        // Touch pressed, waiting for drag
        DRAGGING,       // Actively dragging waypoint
        GESTURE         // Processing gesture (pinch, etc.)
    };
    
    struct TouchPoint {
        float x = 0.0f;         // Normalized coordinates (0-1)
        float y = 0.0f;
        bool active = false;
        uint32_t timestamp = 0;
        int trackingId = -1;    // For multi-touch tracking
    };
    
    struct WaypointVisual {
        VectorPath::Position position;
        float size = 8.0f;      // Visual size in pixels
        bool selected = false;
        bool highlighted = false;
        uint16_t color = 0xFFFF;
        int waypointIndex = -1;
    };
    
    struct CornerLabel {
        std::string label;
        std::string description;
        VectorPath::Position position;
        uint16_t color = 0xFFFF;
        bool visible = true;
    };
    
    struct BlendVisualization {
        std::array<float, 4> weights; // A, B, C, D corner weights
        std::array<uint16_t, 4> colors;
        float totalWeight = 1.0f;
        bool showNumbers = true;
        bool showBars = false;
    };
    
    struct PathPreview {
        static constexpr int PREVIEW_POINTS = 128;
        std::array<VectorPath::Position, PREVIEW_POINTS> points;
        uint16_t pathColor = 0x07FF;    // Cyan
        uint16_t currentPosColor = 0xFFE0; // Yellow
        int lineThickness = 2;
        bool valid = false;
    };
    
    using PositionChangeCallback = std::function<void(const VectorPath::Position& pos, const VectorPath::CornerBlend& blend)>;
    using WaypointChangeCallback = std::function<void(int waypointIndex, const VectorPath::Waypoint& waypoint)>;
    using ModeChangeCallback = std::function<void(EditMode newMode, EditMode oldMode)>;
    
    VectorPathEditor();
    ~VectorPathEditor();
    
    // Initialization
    bool initialize(VectorPath* vectorPath, SmartKnob* smartKnob);
    void shutdown();
    
    // Editor control
    void setEditMode(EditMode mode);
    EditMode getEditMode() const { return editMode_; }
    
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }
    
    // Display configuration
    void setDisplayBounds(int x, int y, int width, int height);
    void setDiamondBounds(int centerX, int centerY, int radius);
    
    // Touch handling
    void handleTouchDown(int x, int y, int touchId = 0);
    void handleTouchMove(int x, int y, int touchId = 0);
    void handleTouchUp(int x, int y, int touchId = 0);
    void handleMultiTouch(const std::array<TouchPoint, 5>& touches);
    
    // SmartKnob integration
    void handleRotation(int32_t delta, float velocity, bool inDetent);
    void handleGesture(SmartKnob::GestureType gesture, float parameter);
    
    // Waypoint management
    void selectWaypoint(int waypointIndex);
    int getSelectedWaypoint() const { return selectedWaypointIndex_; }
    void clearSelection();
    
    // Path visualization
    void setCornerLabels(const std::array<std::string, 4>& labels);
    void setCornerDescriptions(const std::array<std::string, 4>& descriptions);
    void setShowBlendVisualization(bool show) { showBlendViz_ = show; }
    void setShowPathPreview(bool show) { showPathPreview_ = show; }
    
    // Callbacks
    void setPositionChangeCallback(PositionChangeCallback callback) { positionCallback_ = callback; }
    void setWaypointChangeCallback(WaypointChangeCallback callback) { waypointCallback_ = callback; }
    void setModeChangeCallback(ModeChangeCallback callback) { modeCallback_ = callback; }
    
    // Update and rendering
    void update(float deltaTimeMs);
    void render();
    
private:
    // Display configuration
    struct DisplayConfig {
        int x = 0;
        int y = 0;
        int width = 400;
        int height = 400;
        
        // Diamond configuration
        int diamondCenterX = 200;
        int diamondCenterY = 200;
        int diamondRadius = 150;
        
        // Colors
        uint16_t backgroundColor = 0x0841;    // Dark blue
        uint16_t diamondColor = 0x18C3;       // Medium blue
        uint16_t gridColor = 0x39E7;          // Light gray
        uint16_t pathColor = 0x07FF;          // Cyan
        uint16_t waypointColor = 0xFFFF;      // White
        uint16_t selectedColor = 0xFFE0;      // Yellow
        uint16_t highlightColor = 0xFD20;     // Orange
        uint16_t textColor = 0xFFFF;          // White
        
        // Corner colors
        std::array<uint16_t, 4> cornerColors = {
            0xF800,  // Red (A - Top)
            0x07E0,  // Green (B - Right)
            0x001F,  // Blue (C - Bottom)
            0xFFE0   // Yellow (D - Left)
        };
    };
    
    // Touch interaction
    struct TouchInteraction {
        TouchState state = TouchState::IDLE;
        TouchPoint primaryTouch;
        TouchPoint secondaryTouch;
        
        int dragWaypointIndex = -1;
        VectorPath::Position dragStartPos;
        VectorPath::Position dragCurrentPos;
        
        uint32_t gestureStartTime = 0;
        float gestureStartDistance = 0.0f;
        bool multiTouchActive = false;
    };
    
    // Animation system
    struct Animation {
        float progress = 0.0f;
        float duration = 300.0f; // ms
        bool active = false;
        uint32_t startTime = 0;
        
        void start(float durationMs = 300.0f);
        float update(uint32_t currentTime);
        bool isComplete() const { return !active; }
    };
    
    // State
    VectorPath* vectorPath_ = nullptr;
    SmartKnob* smartKnob_ = nullptr;
    bool initialized_ = false;
    bool visible_ = true;
    
    EditMode editMode_ = EditMode::NAVIGATE;
    TouchInteraction touch_;
    DisplayConfig display_;
    
    // Selection and editing
    int selectedWaypointIndex_ = -1;
    int highlightedWaypointIndex_ = -1;
    std::vector<WaypointVisual> waypointVisuals_;
    
    // Visual elements
    std::array<CornerLabel, 4> cornerLabels_;
    BlendVisualization blendViz_;
    PathPreview pathPreview_;
    bool showBlendViz_ = true;
    bool showPathPreview_ = true;
    
    // Animations
    Animation modeTransition_;
    Animation waypointHighlight_;
    
    // Callbacks
    PositionChangeCallback positionCallback_;
    WaypointChangeCallback waypointCallback_;
    ModeChangeCallback modeCallback_;
    
    // Private methods
    void initializeCornerLabels();
    void updateWaypointVisuals();
    void updatePathPreview();
    void updateBlendVisualization();
    
    // Coordinate conversion
    VectorPath::Position screenToNormalized(int screenX, int screenY) const;
    void normalizedToScreen(const VectorPath::Position& normalized, int& screenX, int& screenY) const;
    bool isPointInDiamond(int screenX, int screenY) const;
    VectorPath::Position constrainToDiamond(const VectorPath::Position& pos) const;
    
    // Waypoint interaction
    int findNearestWaypoint(int screenX, int screenY, float maxDistance = 20.0f) const;
    void createWaypointAtPosition(const VectorPath::Position& pos);
    void deleteWaypoint(int waypointIndex);
    void moveWaypoint(int waypointIndex, const VectorPath::Position& newPos);
    
    // Touch gesture processing
    void processSingleTouch(const TouchPoint& touch);
    void processMultiTouch(const TouchPoint& touch1, const TouchPoint& touch2);
    float calculateTouchDistance(const TouchPoint& t1, const TouchPoint& t2) const;
    bool isTapGesture(const TouchPoint& touch) const;
    bool isDragGesture(const TouchPoint& touch) const;
    
    // SmartKnob integration
    void handleWaypointRotation(int32_t delta);
    void handleModeRotation(int32_t delta);
    void handlePlaybackRotation(int32_t delta);
    
    // Rendering methods
    void renderBackground();
    void renderDiamondGrid();
    void renderCornerLabels();
    void renderBlendVisualization();
    void renderPathPreview();
    void renderWaypoints();
    void renderCurrentPosition();
    void renderEditModeIndicator();
    void renderPlaybackControls();
    
    void drawDiamond(int centerX, int centerY, int radius, uint16_t color, bool filled = false);
    void drawWaypoint(const WaypointVisual& waypoint);
    void drawPath(const PathPreview& preview);
    void drawBlendWeights(const BlendVisualization& blend);
    void drawCornerLabel(const CornerLabel& label);
    
    // Drawing primitives
    void drawLine(int x1, int y1, int x2, int y2, uint16_t color, int thickness = 1);
    void drawCircle(int x, int y, int radius, uint16_t color, bool filled = true);
    void drawRectangle(int x, int y, int width, int height, uint16_t color, bool filled = true);
    void drawText(const std::string& text, int x, int y, uint16_t color, int fontSize = 12);
    void drawCenteredText(const std::string& text, int x, int y, int width, uint16_t color, int fontSize = 12);
    
    // Utility functions
    uint32_t getTimeMs() const;
    float lerp(float a, float b, float t) const;
    uint16_t blendColors(uint16_t color1, uint16_t color2, float blend) const;
    float clamp(float value, float min, float max) const;
    
    // Constants
    static constexpr float WAYPOINT_SELECT_RADIUS = 20.0f;
    static constexpr float WAYPOINT_DRAG_THRESHOLD = 5.0f;
    static constexpr uint32_t TAP_MAX_DURATION = 200; // ms
    static constexpr uint32_t DOUBLE_TAP_WINDOW = 300; // ms
    static constexpr float ZOOM_SENSITIVITY = 0.1f;
    static constexpr int MIN_WAYPOINT_DISTANCE = 15; // pixels
};