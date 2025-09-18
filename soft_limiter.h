#pragma once
#include <cmath>
#include <algorithm>

/**
 * Soft limiter to prevent harsh clipping and reduce audio artifacts
 */
class SoftLimiter {
private:
    static constexpr float THRESHOLD = 0.8f;
    static constexpr float KNEE = 0.1f;
    
public:
    float process(float input) {
        float abs_input = std::abs(input);
        
        if (abs_input <= THRESHOLD) {
            return input;
        }
        
        // Soft clipping using tanh
        float over = abs_input - THRESHOLD;
        float compressed = THRESHOLD + KNEE * std::tanh(over / KNEE);
        
        return std::copysign(compressed, input);
    }
};