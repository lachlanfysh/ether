#include "VectorPath.h"
#include <cmath>
#include <algorithm>
#include <random>

VectorPath::VectorPath() {
    // Initialize with center position
    currentPosition_ = Position(0.5f, 0.5f);
    updateCurrentBlend();
    
    // Set default playback configuration
    playbackConfig_.enabled = false;
    playbackConfig_.looping = true;
    playbackConfig_.rate = 1.0f;
    playbackConfig_.startTime = 0.0f;
    playbackConfig_.endTime = 1.0f;
    
    // Create default linear path from center to corners
    createLinearPath(Corner::A, Corner::C);
}

VectorPath::~VectorPath() {
    stopPlayback();
}

void VectorPath::setPosition(const Position& pos) {
    Position constrainedPos = diamondShape_ ? constrainToDiamond(pos) : constrainToSquare(pos);
    
    // Apply smoothing
    if (smoothingAmount_ > 0.0f) {
        float alpha = 1.0f - smoothingAmount_;
        currentPosition_.x = currentPosition_.x * smoothingAmount_ + constrainedPos.x * alpha;
        currentPosition_.y = currentPosition_.y * smoothingAmount_ + constrainedPos.y * alpha;
    } else {
        currentPosition_ = constrainedPos;
    }
    
    updateCurrentBlend();
    notifyPositionChange();
}

VectorPath::CornerBlend VectorPath::calculateBlend(const Position& pos) const {
    return diamondShape_ ? calculateDiamondBlend(pos) : calculateSquareBlend(pos);
}

void VectorPath::addWaypoint(const Waypoint& waypoint) {
    Waypoint validatedWaypoint = waypoint;
    validateWaypoint(validatedWaypoint);
    waypoints_.push_back(validatedWaypoint);
    
    // Rebuild arc-length table
    buildArcLengthLookupTable();
    
    if (waypointCallback_) {
        waypointCallback_(static_cast<int>(waypoints_.size()) - 1, validatedWaypoint);
    }
}

void VectorPath::insertWaypoint(int index, const Waypoint& waypoint) {
    if (index < 0 || index > static_cast<int>(waypoints_.size())) {
        return;
    }
    
    Waypoint validatedWaypoint = waypoint;
    validateWaypoint(validatedWaypoint);
    
    waypoints_.insert(waypoints_.begin() + index, validatedWaypoint);
    buildArcLengthLookupTable();
    
    if (waypointCallback_) {
        waypointCallback_(index, validatedWaypoint);
    }
}

void VectorPath::removeWaypoint(int index) {
    if (!isValidWaypointIndex(index)) {
        return;
    }
    
    waypoints_.erase(waypoints_.begin() + index);
    buildArcLengthLookupTable();
}

void VectorPath::clearWaypoints() {
    waypoints_.clear();
    arcLengthLUT_.valid = false;
}

void VectorPath::setWaypoint(int index, const Waypoint& waypoint) {
    if (!isValidWaypointIndex(index)) {
        return;
    }
    
    Waypoint validatedWaypoint = waypoint;
    validateWaypoint(validatedWaypoint);
    
    waypoints_[index] = validatedWaypoint;
    buildArcLengthLookupTable();
    
    if (waypointCallback_) {
        waypointCallback_(index, validatedWaypoint);
    }
}

const VectorPath::Waypoint& VectorPath::getWaypoint(int index) const {
    static const Waypoint defaultWaypoint;
    if (!isValidWaypointIndex(index)) {
        return defaultWaypoint;
    }
    return waypoints_[index];
}

VectorPath::Position VectorPath::interpolatePosition(float t) const {
    if (waypoints_.size() < 2) {
        return currentPosition_;
    }
    
    // Use arc-length parameterization for uniform motion
    if (arcLengthLUT_.valid) {
        t = arcLengthLUT_.tFromArcLength(t * arcLengthLUT_.totalLength);
    }
    
    t = std::clamp(t, 0.0f, 1.0f);
    
    // Calculate which segment we're in
    float segmentFloat = t * (waypoints_.size() - 1);
    int segment = static_cast<int>(segmentFloat);
    float localT = segmentFloat - segment;
    
    // Handle edge case
    if (segment >= static_cast<int>(waypoints_.size()) - 1) {
        segment = waypoints_.size() - 2;
        localT = 1.0f;
    }
    
    return getPathPoint(segment, localT);
}

float VectorPath::getPathLength() const {
    return arcLengthLUT_.valid ? arcLengthLUT_.totalLength : 0.0f;
}

void VectorPath::buildArcLengthLookupTable() {
    arcLengthLUT_.build(waypoints_);
}

void VectorPath::setPlaybackConfig(const PlaybackConfig& config) {
    playbackConfig_ = config;
}

void VectorPath::startPlayback() {
    if (waypoints_.size() < 2) {
        return;
    }
    
    playbackActive_ = true;
    playbackPaused_ = false;
    playbackTime_ = 0.0f;
    playbackPosition_ = playbackConfig_.startTime;
    playbackReverse_ = false;
}

void VectorPath::stopPlayback() {
    playbackActive_ = false;
    playbackPaused_ = false;
    playbackPosition_ = 0.0f;
    playbackTime_ = 0.0f;
}

void VectorPath::pausePlayback() {
    playbackPaused_ = !playbackPaused_;
}

void VectorPath::setPlaybackPosition(float t) {
    playbackPosition_ = std::clamp(t, 0.0f, 1.0f);
    
    if (playbackActive_) {
        Position newPos = interpolatePosition(playbackPosition_);
        setPosition(newPos);
    }
}

void VectorPath::update(float deltaTimeMs) {
    if (playbackActive_ && !playbackPaused_) {
        updatePlayback(deltaTimeMs);
    }
}

template<typename T>
T VectorPath::blendCornerSources(const T& sourceA, const T& sourceB, 
                                const T& sourceC, const T& sourceD) const {
    const CornerBlend& blend = currentBlend_;
    
    // Equal-power bilinear blending
    return sourceA * blend[Corner::A] + 
           sourceB * blend[Corner::B] + 
           sourceC * blend[Corner::C] + 
           sourceD * blend[Corner::D];
}

// Explicit template instantiations for common types
template float VectorPath::blendCornerSources(const float&, const float&, const float&, const float&) const;
template double VectorPath::blendCornerSources(const double&, const double&, const double&, const double&) const;

float VectorPath::calculatePathCurvature(float t) const {
    if (waypoints_.size() < 3) {
        return 0.0f;
    }
    
    const float epsilon = 0.001f;
    Position p1 = interpolatePosition(std::max(0.0f, t - epsilon));
    Position p2 = interpolatePosition(t);
    Position p3 = interpolatePosition(std::min(1.0f, t + epsilon));
    
    // Calculate curvature using finite differences
    float dx1 = p2.x - p1.x;
    float dy1 = p2.y - p1.y;
    float dx2 = p3.x - p2.x;
    float dy2 = p3.y - p2.y;
    
    float cross = dx1 * dy2 - dy1 * dx2;
    float mag1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
    float mag2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
    
    if (mag1 < epsilon || mag2 < epsilon) {
        return 0.0f;
    }
    
    return cross / (mag1 * mag2 * epsilon);
}

VectorPath::Position VectorPath::calculatePathTangent(float t) const {
    if (waypoints_.size() < 2) {
        return Position(1.0f, 0.0f);
    }
    
    const float epsilon = 0.001f;
    Position p1 = interpolatePosition(std::max(0.0f, t - epsilon));
    Position p2 = interpolatePosition(std::min(1.0f, t + epsilon));
    
    Position tangent(p2.x - p1.x, p2.y - p1.y);
    float length = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
    
    if (length > epsilon) {
        tangent.x /= length;
        tangent.y /= length;
    }
    
    return tangent;
}

float VectorPath::calculatePathSpeed(float t) const {
    if (!arcLengthLUT_.valid) {
        return 1.0f;
    }
    
    const float epsilon = 0.001f;
    float arcLength1 = arcLengthLUT_.arcLengthFromT(std::max(0.0f, t - epsilon));
    float arcLength2 = arcLengthLUT_.arcLengthFromT(std::min(1.0f, t + epsilon));
    
    return (arcLength2 - arcLength1) / (2.0f * epsilon);
}

void VectorPath::createLinearPath(Corner startCorner, Corner endCorner) {
    clearWaypoints();
    
    // Define corner positions in diamond layout
    Position cornerPositions[4] = {
        Position(0.5f, 0.0f),  // A - Top
        Position(1.0f, 0.5f),  // B - Right
        Position(0.5f, 1.0f),  // C - Bottom
        Position(0.0f, 0.5f)   // D - Left
    };
    
    addWaypoint(Waypoint(cornerPositions[static_cast<int>(startCorner)].x,
                        cornerPositions[static_cast<int>(startCorner)].y));
    addWaypoint(Waypoint(cornerPositions[static_cast<int>(endCorner)].x,
                        cornerPositions[static_cast<int>(endCorner)].y));
}

void VectorPath::createCircularPath(float radius) {
    clearWaypoints();
    
    const int numPoints = 8;
    for (int i = 0; i < numPoints; i++) {
        float angle = static_cast<float>(i) * 2.0f * M_PI / numPoints;
        float x = 0.5f + radius * std::cos(angle);
        float y = 0.5f + radius * std::sin(angle);
        
        addWaypoint(Waypoint(x, y, 0.3f)); // Lower tension for smoother curves
    }
}

void VectorPath::createFigureEightPath(float size) {
    clearWaypoints();
    
    const int numPoints = 16;
    for (int i = 0; i < numPoints; i++) {
        float t = static_cast<float>(i) / numPoints * 2.0f * M_PI;
        
        // Figure-eight parametric equations
        float x = 0.5f + size * std::sin(t);
        float y = 0.5f + size * std::sin(t) * std::cos(t);
        
        addWaypoint(Waypoint(x, y, 0.4f));
    }
}

void VectorPath::createRandomPath(int numWaypoints) {
    clearWaypoints();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.1f, 0.9f);
    std::uniform_real_distribution<float> tensionDis(0.2f, 0.8f);
    
    for (int i = 0; i < numWaypoints; i++) {
        float x = dis(gen);
        float y = dis(gen);
        float tension = tensionDis(gen);
        
        // Constrain to diamond if needed
        if (diamondShape_) {
            Position pos = constrainToDiamond(Position(x, y));
            x = pos.x;
            y = pos.y;
        }
        
        addWaypoint(Waypoint(x, y, tension));
    }
}

// Private method implementations

void VectorPath::updateCurrentBlend() {
    currentBlend_ = calculateBlend(currentPosition_);
}

void VectorPath::notifyPositionChange() {
    if (positionCallback_) {
        positionCallback_(currentPosition_, currentBlend_);
    }
}

VectorPath::CornerBlend VectorPath::calculateSquareBlend(const Position& pos) const {
    CornerBlend blend;
    
    // Standard bilinear interpolation for square
    float x = std::clamp(pos.x, 0.0f, 1.0f);
    float y = std::clamp(pos.y, 0.0f, 1.0f);
    
    // Square corners: A=top-left, B=top-right, C=bottom-right, D=bottom-left
    blend[Corner::A] = (1.0f - x) * (1.0f - y); // Top-left
    blend[Corner::B] = x * (1.0f - y);          // Top-right
    blend[Corner::C] = x * y;                   // Bottom-right
    blend[Corner::D] = (1.0f - x) * y;          // Bottom-left
    
    return blend;
}

VectorPath::CornerBlend VectorPath::calculateDiamondBlend(const Position& pos) const {
    CornerBlend blend;
    
    // Transform position to diamond coordinates
    float x = std::clamp(pos.x, 0.0f, 1.0f);
    float y = std::clamp(pos.y, 0.0f, 1.0f);
    
    // Diamond corners: A=top, B=right, C=bottom, D=left
    // Distance-based blending in diamond space
    
    // Calculate distances to each corner
    float distA = std::abs(x - 0.5f) + std::abs(y - 0.0f); // Top
    float distB = std::abs(x - 1.0f) + std::abs(y - 0.5f); // Right
    float distC = std::abs(x - 0.5f) + std::abs(y - 1.0f); // Bottom
    float distD = std::abs(x - 0.0f) + std::abs(y - 0.5f); // Left
    
    // Convert distances to weights (inverse distance)
    const float epsilon = 0.001f;
    float weightA = 1.0f / (distA + epsilon);
    float weightB = 1.0f / (distB + epsilon);
    float weightC = 1.0f / (distC + epsilon);
    float weightD = 1.0f / (distD + epsilon);
    
    // Normalize weights
    float totalWeight = weightA + weightB + weightC + weightD;
    blend[Corner::A] = weightA / totalWeight;
    blend[Corner::B] = weightB / totalWeight;
    blend[Corner::C] = weightC / totalWeight;
    blend[Corner::D] = weightD / totalWeight;
    
    return blend;
}

VectorPath::Position VectorPath::constrainToDiamond(const Position& pos) const {
    float x = std::clamp(pos.x, 0.0f, 1.0f);
    float y = std::clamp(pos.y, 0.0f, 1.0f);
    
    // Diamond constraint: |x - 0.5| + |y - 0.5| <= 0.5
    float centerX = x - 0.5f;
    float centerY = y - 0.5f;
    float manhattanDist = std::abs(centerX) + std::abs(centerY);
    
    if (manhattanDist > 0.5f) {
        // Scale position to fit within diamond
        float scale = 0.5f / manhattanDist;
        centerX *= scale;
        centerY *= scale;
        x = centerX + 0.5f;
        y = centerY + 0.5f;
    }
    
    return Position(x, y);
}

VectorPath::Position VectorPath::constrainToSquare(const Position& pos) const {
    return Position(std::clamp(pos.x, 0.0f, 1.0f), std::clamp(pos.y, 0.0f, 1.0f));
}

float VectorPath::bilinearInterpolate(float a, float b, float c, float d, float x, float y) const {
    float ab = a * (1.0f - x) + b * x;
    float cd = c * (1.0f - x) + d * x;
    return ab * (1.0f - y) + cd * y;
}

VectorPath::Position VectorPath::getPathPoint(int segment, float localT) const {
    if (segment < 0 || segment >= static_cast<int>(waypoints_.size()) - 1) {
        return currentPosition_;
    }
    
    // Get control points for Catmull-Rom interpolation
    Position p0 = getControlPoint(segment - 1);
    Position p1 = getControlPoint(segment);
    Position p2 = getControlPoint(segment + 1);
    Position p3 = getControlPoint(segment + 2);
    
    float tension = waypoints_[segment].tension;
    
    return CatmullRom::interpolate(p0, p1, p2, p3, localT, tension);
}

VectorPath::Position VectorPath::getControlPoint(int index) const {
    // Handle boundary conditions for Catmull-Rom spline
    if (index < 0) {
        // Extend before first point
        if (waypoints_.size() >= 2) {
            Position p0(waypoints_[0].x, waypoints_[0].y);
            Position p1(waypoints_[1].x, waypoints_[1].y);
            Position direction = p0 + (p0 + p1 * -1.0f);
            return direction;
        } else {
            return Position(waypoints_[0].x, waypoints_[0].y);
        }
    } else if (index >= static_cast<int>(waypoints_.size())) {
        // Extend after last point
        int lastIndex = waypoints_.size() - 1;
        if (lastIndex >= 1) {
            Position p0(waypoints_[lastIndex - 1].x, waypoints_[lastIndex - 1].y);
            Position p1(waypoints_[lastIndex].x, waypoints_[lastIndex].y);
            Position direction = p1 + (p1 + p0 * -1.0f);
            return direction;
        } else {
            return Position(waypoints_[lastIndex].x, waypoints_[lastIndex].y);
        }
    } else {
        return Position(waypoints_[index].x, waypoints_[index].y);
    }
}

void VectorPath::updatePlayback(float deltaTimeMs) {
    if (waypoints_.size() < 2) {
        return;
    }
    
    float rate = calculatePlaybackRate();
    float deltaTime = deltaTimeMs * 0.001f * rate; // Convert to seconds
    
    playbackTime_ += deltaTime;
    
    // Update playback position
    float pathDuration = 1.0f; // Normalized path duration
    float newPosition = playbackPosition_ + deltaTime / pathDuration;
    
    // Handle looping and boundaries
    if (playbackConfig_.looping) {
        if (playbackConfig_.pingPong) {
            if (newPosition > playbackConfig_.endTime) {
                playbackReverse_ = true;
                newPosition = playbackConfig_.endTime - (newPosition - playbackConfig_.endTime);
            } else if (newPosition < playbackConfig_.startTime) {
                playbackReverse_ = false;
                newPosition = playbackConfig_.startTime + (playbackConfig_.startTime - newPosition);
            }
        } else {
            while (newPosition > playbackConfig_.endTime) {
                newPosition -= (playbackConfig_.endTime - playbackConfig_.startTime);
            }
            while (newPosition < playbackConfig_.startTime) {
                newPosition += (playbackConfig_.endTime - playbackConfig_.startTime);
            }
        }
    } else {
        // Clamp to bounds and stop if reached end
        if (newPosition >= playbackConfig_.endTime) {
            newPosition = playbackConfig_.endTime;
            stopPlayback();
        } else if (newPosition <= playbackConfig_.startTime) {
            newPosition = playbackConfig_.startTime;
            stopPlayback();
        }
    }
    
    setPlaybackPosition(newPosition);
}

float VectorPath::calculatePlaybackRate() const {
    float baseRate = playbackConfig_.rate;
    
    // Apply swing (not implemented in this basic version)
    
    return baseRate;
}

void VectorPath::handlePlaybackLooping() {
    // This method could handle more complex looping behaviors
}

bool VectorPath::isValidWaypointIndex(int index) const {
    return index >= 0 && index < static_cast<int>(waypoints_.size());
}

void VectorPath::validateWaypoint(Waypoint& waypoint) const {
    waypoint.x = std::clamp(waypoint.x, 0.0f, 1.0f);
    waypoint.y = std::clamp(waypoint.y, 0.0f, 1.0f);
    waypoint.tension = std::clamp(waypoint.tension, 0.0f, 1.0f);
    waypoint.bias = std::clamp(waypoint.bias, -1.0f, 1.0f);
    waypoint.continuity = std::clamp(waypoint.continuity, -1.0f, 1.0f);
    
    if (diamondShape_) {
        Position constrainedPos = constrainToDiamond(Position(waypoint.x, waypoint.y));
        waypoint.x = constrainedPos.x;
        waypoint.y = constrainedPos.y;
    }
}

// ArcLengthLUT implementation
void VectorPath::ArcLengthLUT::build(const std::vector<Waypoint>& waypoints) {
    if (waypoints.size() < 2) {
        valid = false;
        return;
    }
    
    totalLength = 0.0f;
    arcLengths[0] = 0.0f;
    tValues[0] = 0.0f;
    
    // Calculate arc lengths using numerical integration
    const int numSamples = TABLE_SIZE - 1;
    Position lastPos(waypoints[0].x, waypoints[0].y);
    
    for (int i = 1; i < TABLE_SIZE; i++) {
        float t = static_cast<float>(i) / static_cast<float>(TABLE_SIZE - 1);
        
        // Calculate position at t using simple linear interpolation
        // (This would use the full Catmull-Rom interpolation in practice)
        float segmentFloat = t * (waypoints.size() - 1);
        int segment = static_cast<int>(segmentFloat);
        float localT = segmentFloat - segment;
        
        if (segment >= static_cast<int>(waypoints.size()) - 1) {
            segment = waypoints.size() - 2;
            localT = 1.0f;
        }
        
        Position pos1(waypoints[segment].x, waypoints[segment].y);
        Position pos2(waypoints[segment + 1].x, waypoints[segment + 1].y);
        Position currentPos = pos1 + (pos2 + pos1 * -1.0f) * localT;
        
        float segmentLength = lastPos.distanceTo(currentPos);
        totalLength += segmentLength;
        
        arcLengths[i] = totalLength;
        tValues[i] = t;
        lastPos = currentPos;
    }
    
    valid = true;
}

float VectorPath::ArcLengthLUT::tFromArcLength(float arcLength) const {
    if (!valid || totalLength <= 0.0f) {
        return 0.0f;
    }
    
    arcLength = std::clamp(arcLength, 0.0f, totalLength);
    
    // Binary search in arc length table
    int low = 0;
    int high = TABLE_SIZE - 1;
    
    while (high - low > 1) {
        int mid = (low + high) / 2;
        if (arcLengths[mid] < arcLength) {
            low = mid;
        } else {
            high = mid;
        }
    }
    
    // Linear interpolation between adjacent entries
    if (high == low) {
        return tValues[low];
    }
    
    float fraction = (arcLength - arcLengths[low]) / (arcLengths[high] - arcLengths[low]);
    return tValues[low] + fraction * (tValues[high] - tValues[low]);
}

float VectorPath::ArcLengthLUT::arcLengthFromT(float t) const {
    if (!valid) {
        return 0.0f;
    }
    
    t = std::clamp(t, 0.0f, 1.0f);
    float index = t * (TABLE_SIZE - 1);
    int low = static_cast<int>(index);
    int high = std::min(low + 1, TABLE_SIZE - 1);
    
    if (low == high) {
        return arcLengths[low];
    }
    
    float fraction = index - low;
    return arcLengths[low] + fraction * (arcLengths[high] - arcLengths[low]);
}

// CatmullRom implementation
VectorPath::Position VectorPath::CatmullRom::interpolate(const Position& p0, const Position& p1, 
                                                        const Position& p2, const Position& p3, 
                                                        float t, float tension) {
    float t2 = t * t;
    float t3 = t2 * t;
    
    // Catmull-Rom basis functions with tension parameter
    float alpha = (1.0f - tension) * 0.5f;
    
    float b0 = -alpha * t + 2.0f * alpha * t2 - alpha * t3;
    float b1 = 1.0f + (alpha - 3.0f) * t2 + (2.0f - alpha) * t3;
    float b2 = alpha * t + (3.0f - 2.0f * alpha) * t2 + (alpha - 2.0f) * t3;
    float b3 = -alpha * t2 + alpha * t3;
    
    Position result;
    result.x = p0.x * b0 + p1.x * b1 + p2.x * b2 + p3.x * b3;
    result.y = p0.y * b0 + p1.y * b1 + p2.y * b2 + p3.y * b3;
    
    return result;
}

VectorPath::Position VectorPath::CatmullRom::tangent(const Position& p0, const Position& p1,
                                                    const Position& p2, const Position& p3,
                                                    float t, float tension) {
    float t2 = t * t;
    
    // Derivative of Catmull-Rom basis functions
    float alpha = (1.0f - tension) * 0.5f;
    
    float db0 = -alpha + 4.0f * alpha * t - 3.0f * alpha * t2;
    float db1 = (2.0f * alpha - 6.0f) * t + (6.0f - 3.0f * alpha) * t2;
    float db2 = alpha + (6.0f - 4.0f * alpha) * t + (3.0f * alpha - 6.0f) * t2;
    float db3 = -2.0f * alpha * t + 3.0f * alpha * t2;
    
    Position result;
    result.x = p0.x * db0 + p1.x * db1 + p2.x * db2 + p3.x * db3;
    result.y = p0.y * db0 + p1.y * db1 + p2.y * db2 + p3.y * db3;
    
    return result;
}