#pragma once
#include <cmath>

/**
 * Simple DC blocking filter to prevent audio pops
 * Implements y[n] = x[n] - x[n-1] + 0.995 * y[n-1]
 */
class DCBlocker {
private:
    float x1 = 0.0f;  // Previous input
    float y1 = 0.0f;  // Previous output
    static constexpr float POLE = 0.995f;
    
public:
    float process(float input) {
        float output = input - x1 + POLE * y1;
        x1 = input;
        y1 = output;
        return output;
    }
    
    void reset() {
        x1 = y1 = 0.0f;
    }
};