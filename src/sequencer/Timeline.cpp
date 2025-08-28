#include "Timeline.h"
#include <iostream>

Timeline::Timeline() {
    std::cout << "Timeline created" << std::endl;
}

Timeline::~Timeline() {
    std::cout << "Timeline destroyed" << std::endl;
}

void Timeline::addPattern() {
    patterns_.push_back(static_cast<int>(patterns_.size()));
    std::cout << "Timeline: Added pattern " << patterns_.size() - 1 << std::endl;
}

void Timeline::removePattern(size_t index) {
    if (index < patterns_.size()) {
        patterns_.erase(patterns_.begin() + index);
        std::cout << "Timeline: Removed pattern " << index << std::endl;
    }
}

void Timeline::copyPattern(size_t sourceIndex, size_t destIndex) {
    if (sourceIndex < patterns_.size() && destIndex < patterns_.size()) {
        patterns_[destIndex] = patterns_[sourceIndex];
        std::cout << "Timeline: Copied pattern " << sourceIndex << " to " << destIndex << std::endl;
    }
}

void Timeline::setPosition(size_t position) {
    currentPosition_ = position;
}