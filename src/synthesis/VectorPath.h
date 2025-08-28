#pragma once
#include <array>
#include <vector>
#include <functional>
#include <cstdint>

/**
 * VectorPath - Advanced 2D parameter space scrubbing system
 * 
 * Features:
 * - 2D XY coordinate system with diamond-shaped control surface
 * - Four corner sources (A/B/C/D) with equal-power bilinear blending
 * - Catmull-Rom spline interpolation for smooth path following
 * - Arc-length lookup tables for uniform motion along curves
 * - Real-time path editing with waypoint manipulation
 * - Latch mode with looping playback and rate control
 * - Corner type crossfading for seamless engine transitions
 */
class VectorPath {
public:
    // Corner source identifiers for diamond layout
    enum class Corner {
        A = 0,  // Top (North)
        B = 1,  // Right (East)  
        C = 2,  // Bottom (South)
        D = 3   // Left (West)
    };
    
    // Path waypoint with position and curve control
    struct Waypoint {
        float x = 0.5f;         // X coordinate (0.0 to 1.0)
        float y = 0.5f;         // Y coordinate (0.0 to 1.0)
        float tension = 0.5f;   // Catmull-Rom tension parameter
        float bias = 0.0f;      // Bias for asymmetric curves
        float continuity = 0.0f; // Continuity control
        uint32_t timeMs = 0;    // Time-based sequencing
        
        Waypoint() = default;
        Waypoint(float x_, float y_, float tension_ = 0.5f) 
            : x(x_), y(y_), tension(tension_) {}
    };
    
    // 2D position in normalized space
    struct Position {
        float x = 0.5f;
        float y = 0.5f;
        
        Position() = default;
        Position(float x_, float y_) : x(x_), y(y_) {}
        
        Position operator+(const Position& other) const {
            return Position(x + other.x, y + other.y);
        }
        
        Position operator*(float scalar) const {
            return Position(x * scalar, y * scalar);
        }
        
        float distanceTo(const Position& other) const {
            float dx = x - other.x;
            float dy = y - other.y;
            return std::sqrt(dx * dx + dy * dy);
        }
    };
    
    // Corner blend weights for bilinear interpolation
    struct CornerBlend {
        float weights[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // A, B, C, D
        
        float& operator[](int index) { return weights[index]; }
        const float& operator[](int index) const { return weights[index]; }
        
        float& operator[](Corner corner) { return weights[static_cast<int>(corner)]; }
        const float& operator[](Corner corner) const { return weights[static_cast<int>(corner)]; }
        
        // Normalize weights to sum to 1.0
        void normalize() {
            float sum = weights[0] + weights[1] + weights[2] + weights[3];
            if (sum > 0.0f) {
                float scale = 1.0f / sum;
                for (int i = 0; i < 4; i++) {
                    weights[i] *= scale;
                }
            }
        }
        
        // Check if position is at a pure corner
        bool isPureCorner(Corner& corner) const {
            for (int i = 0; i < 4; i++) {
                if (weights[i] > 0.99f) {
                    corner = static_cast<Corner>(i);
                    return true;
                }
            }
            return false;
        }
    };
    
    // Path playback configuration
    struct PlaybackConfig {
        bool enabled = false;
        bool looping = true;
        float rate = 1.0f;          // Playback rate multiplier
        float startTime = 0.0f;     // Start position (0.0 to 1.0)
        float endTime = 1.0f;       // End position (0.0 to 1.0)
        bool pingPong = false;      // Ping-pong vs normal looping
        bool quantized = false;     // Quantize to beat divisions
        float swing = 0.0f;         // Swing amount (0.0 to 1.0)
    };
    
    using PositionChangeCallback = std::function<void(const Position& pos, const CornerBlend& blend)>;
    using WaypointChangeCallback = std::function<void(int waypointIndex, const Waypoint& waypoint)>;
    
    VectorPath();
    ~VectorPath();
    
    // Position control
    void setPosition(const Position& pos);
    void setPosition(float x, float y) { setPosition(Position(x, y)); }
    Position getPosition() const { return currentPosition_; }
    
    // Corner blend calculation
    CornerBlend calculateBlend(const Position& pos) const;
    CornerBlend getCurrentBlend() const { return currentBlend_; }
    
    // Path waypoints
    void addWaypoint(const Waypoint& waypoint);
    void insertWaypoint(int index, const Waypoint& waypoint);
    void removeWaypoint(int index);
    void clearWaypoints();
    
    void setWaypoint(int index, const Waypoint& waypoint);
    const Waypoint& getWaypoint(int index) const;
    int getWaypointCount() const { return static_cast<int>(waypoints_.size()); }
    
    // Path interpolation
    Position interpolatePosition(float t) const; // t = 0.0 to 1.0 along path
    float getPathLength() const;
    void buildArcLengthLookupTable();
    
    // Path playback
    void setPlaybackConfig(const PlaybackConfig& config);
    const PlaybackConfig& getPlaybackConfig() const { return playbackConfig_; }
    
    void startPlayback();
    void stopPlayback();
    void pausePlayback();
    bool isPlaying() const { return playbackActive_; }
    
    void setPlaybackPosition(float t); // t = 0.0 to 1.0
    float getPlaybackPosition() const { return playbackPosition_; }
    
    // Update (call from main thread)
    void update(float deltaTimeMs);
    
    // Corner source management
    template<typename T>
    void setCornerSource(Corner corner, const T& source);
    
    template<typename T>
    T blendCornerSources(const T& sourceA, const T& sourceB, 
                        const T& sourceC, const T& sourceD) const;
    
    // Callbacks
    void setPositionChangeCallback(PositionChangeCallback callback) { positionCallback_ = callback; }
    void setWaypointChangeCallback(WaypointChangeCallback callback) { waypointCallback_ = callback; }
    
    // Configuration
    void setDiamondShape(bool diamond) { diamondShape_ = diamond; }
    bool isDiamondShape() const { return diamondShape_; }
    
    void setSmoothingAmount(float amount) { smoothingAmount_ = amount; }
    float getSmoothingAmount() const { return smoothingAmount_; }
    
    // Path analysis
    float calculatePathCurvature(float t) const;
    Position calculatePathTangent(float t) const;
    float calculatePathSpeed(float t) const;
    
    // Preset paths
    void createLinearPath(Corner startCorner, Corner endCorner);
    void createCircularPath(float radius = 0.4f);
    void createFigureEightPath(float size = 0.3f);
    void createRandomPath(int numWaypoints = 6);
    
private:
    // Arc-length lookup table for uniform motion
    struct ArcLengthLUT {
        static constexpr int TABLE_SIZE = 512;
        std::array<float, TABLE_SIZE> arcLengths;
        std::array<float, TABLE_SIZE> tValues;
        float totalLength = 0.0f;
        bool valid = false;
        
        void build(const std::vector<Waypoint>& waypoints);
        float tFromArcLength(float arcLength) const;
        float arcLengthFromT(float t) const;
    };
    
    // Catmull-Rom spline calculation
    struct CatmullRom {
        static Position interpolate(const Position& p0, const Position& p1, 
                                  const Position& p2, const Position& p3, 
                                  float t, float tension = 0.5f);
        
        static Position tangent(const Position& p0, const Position& p1,
                               const Position& p2, const Position& p3,
                               float t, float tension = 0.5f);
    };
    
    // State
    Position currentPosition_;
    CornerBlend currentBlend_;
    std::vector<Waypoint> waypoints_;
    ArcLengthLUT arcLengthLUT_;
    
    // Playback state
    PlaybackConfig playbackConfig_;
    bool playbackActive_ = false;
    bool playbackPaused_ = false;
    float playbackPosition_ = 0.0f;
    float playbackTime_ = 0.0f;
    bool playbackReverse_ = false; // For ping-pong mode
    
    // Configuration
    bool diamondShape_ = true;      // True = diamond, false = square
    float smoothingAmount_ = 0.1f;  // Position smoothing (0.0 to 1.0)
    
    // Callbacks
    PositionChangeCallback positionCallback_;
    WaypointChangeCallback waypointCallback_;
    
    // Private methods
    void updateCurrentBlend();
    void notifyPositionChange();
    
    CornerBlend calculateSquareBlend(const Position& pos) const;
    CornerBlend calculateDiamondBlend(const Position& pos) const;
    
    Position constrainToDiamond(const Position& pos) const;
    Position constrainToSquare(const Position& pos) const;
    
    // Bilinear interpolation helpers
    float bilinearInterpolate(float a, float b, float c, float d, float x, float y) const;
    
    // Path interpolation helpers
    Position getPathPoint(int segment, float localT) const;
    Position getControlPoint(int index) const; // Handle boundary conditions
    
    // Playback helpers
    void updatePlayback(float deltaTimeMs);
    float calculatePlaybackRate() const;
    void handlePlaybackLooping();
    
    // Validation
    bool isValidWaypointIndex(int index) const;
    void validateWaypoint(Waypoint& waypoint) const;
};