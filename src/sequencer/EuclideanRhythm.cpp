#include "EuclideanRhythm.h"
#include <iostream>
#include <algorithm>

EuclideanRhythm::EuclideanRhythm() {
    pattern_.fill(false);
    generatePattern();
    std::cout << "EuclideanRhythm created" << std::endl;
}

EuclideanRhythm::~EuclideanRhythm() {
    std::cout << "EuclideanRhythm destroyed" << std::endl;
}

void EuclideanRhythm::setPattern(uint8_t hits, uint8_t rotation) {
    hits_ = std::min(hits, static_cast<uint8_t>(16));
    rotation_ = rotation % 16;
    generatePattern();
}

void EuclideanRhythm::generatePattern() {
    pattern_.fill(false);
    
    if (hits_ == 0) return;
    
    // Simple Euclidean algorithm (Bjorklund's algorithm)
    int bucket = 0;
    for (int i = 0; i < 16; i++) {
        bucket += hits_;
        if (bucket >= 16) {
            bucket -= 16;
            pattern_[i] = true;
        }
    }
    
    // Apply rotation
    if (rotation_ > 0) {
        std::rotate(pattern_.begin(), pattern_.begin() + rotation_, pattern_.end());
    }
    
    std::cout << "EuclideanRhythm: Generated pattern " << static_cast<int>(hits_) 
              << "/" << 16 << " rotation " << static_cast<int>(rotation_) << std::endl;
}

bool EuclideanRhythm::shouldTrigger(uint8_t step) const {
    if (step < 16) {
        return pattern_[step];
    }
    return false;
}

bool EuclideanRhythm::advance() {
    bool trigger = pattern_[currentStep_];
    currentStep_ = (currentStep_ + 1) % 16;
    return trigger;
}

void EuclideanRhythm::reset() {
    currentStep_ = 0;
}