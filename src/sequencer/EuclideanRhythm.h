#pragma once
#include "../core/Types.h"
#include <array>

/**
 * Euclidean rhythm generator
 * Stub implementation for initial testing
 */
class EuclideanRhythm {
public:
    EuclideanRhythm();
    ~EuclideanRhythm();
    
    // Pattern generation
    void setPattern(uint8_t hits, uint8_t rotation = 0);
    void generatePattern();
    
    // Playback
    bool shouldTrigger(uint8_t step) const;
    bool advance();
    void reset();
    
    // Status
    uint8_t getHits() const { return hits_; }
    uint8_t getRotation() const { return rotation_; }
    uint8_t getCurrentStep() const { return currentStep_; }
    uint8_t getSteps() const { return 16; }
    
private:
    std::array<bool, 16> pattern_;
    uint8_t hits_ = 4;
    uint8_t rotation_ = 0;
    uint8_t currentStep_ = 0;
};