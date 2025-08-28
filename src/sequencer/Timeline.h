#pragma once
#include "../core/Types.h"
#include <vector>

/**
 * Timeline system for linear pattern composition
 * Stub implementation for initial testing
 */
class Timeline {
public:
    Timeline();
    ~Timeline();
    
    // Pattern management
    void addPattern();
    void removePattern(size_t index);
    void copyPattern(size_t sourceIndex, size_t destIndex);
    
    // Playback
    void setPosition(size_t position);
    size_t getPosition() const { return currentPosition_; }
    size_t getLength() const { return patterns_.size(); }
    
private:
    std::vector<int> patterns_; // Simplified pattern storage
    size_t currentPosition_ = 0;
};