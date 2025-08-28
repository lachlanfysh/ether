#include "EngineCrossfader.h"
#include <algorithm>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

EngineCrossfader::EngineCrossfader() {
    sampleRate_ = 44100.0f;
    crossfadeTimeMs_ = DEFAULT_CROSSFADE_TIME_MS;
    crossfadeType_ = CrossfadeType::EQUAL_POWER_SINE;
    initialized_ = false;
    manualControl_ = false;
    paused_ = false;
    
    state_ = CrossfadeState::ENGINE_A_ONLY;
    position_ = 0.0f;
    targetPosition_ = 0.0f;
    positionIncrement_ = 0.0f;
    
    cpuUsage_ = 0.0f;
}

bool EngineCrossfader::initialize(float sampleRate, float crossfadeTimeMs) {
    if (sampleRate <= 0.0f) {
        return false;
    }
    
    sampleRate_ = sampleRate;
    crossfadeTimeMs_ = clamp(crossfadeTimeMs, MIN_CROSSFADE_TIME_MS, MAX_CROSSFADE_TIME_MS);
    
    calculateCrossfadeIncrement();
    reset();
    
    initialized_ = true;
    return true;
}

void EngineCrossfader::shutdown() {
    if (!initialized_) {
        return;
    }
    
    reset();
    initialized_ = false;
}

float EngineCrossfader::processMix(float engineA, float engineB) {
    if (!initialized_) {
        return engineA; // Default to engine A if not initialized
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Update crossfade position if not in manual control
    if (!manualControl_ && !paused_) {
        updateCrossfadeState();
    }
    
    // Calculate gains for both engines
    float gainA, gainB;
    calculateGains(position_, gainA, gainB);
    
    // Mix the engines
    float output = engineA * gainA + engineB * gainB;
    
    // Update CPU usage
    auto endTime = std::chrono::high_resolution_clock::now();
    float processingTime = std::chrono::duration<float, std::micro>(endTime - startTime).count();
    cpuUsage_ = cpuUsage_ * 0.999f + processingTime * 0.001f;
    
    return output;
}

void EngineCrossfader::processBlock(float* engineABuffer, float* engineBBuffer, float* output, int numSamples) {
    if (!initialized_) {
        // Copy engine A to output if not initialized
        std::copy(engineABuffer, engineABuffer + numSamples, output);
        return;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numSamples; i++) {
        // Update crossfade position if not in manual control
        if (!manualControl_ && !paused_) {
            updateCrossfadeState();
        }
        
        // Calculate gains for both engines
        float gainA, gainB;
        calculateGains(position_, gainA, gainB);
        
        // Mix the engines
        output[i] = engineABuffer[i] * gainA + engineBBuffer[i] * gainB;
    }
    
    // Update CPU usage
    auto endTime = std::chrono::high_resolution_clock::now();
    float processingTime = std::chrono::duration<float, std::micro>(endTime - startTime).count();
    cpuUsage_ = cpuUsage_ * 0.99f + (processingTime / numSamples) * 0.01f;
}

void EngineCrossfader::processStereoBlock(float* engineALeft, float* engineARight,
                                         float* engineBLeft, float* engineBRight,
                                         float* outputLeft, float* outputRight, int numSamples) {
    if (!initialized_) {
        // Copy engine A to output if not initialized
        std::copy(engineALeft, engineALeft + numSamples, outputLeft);
        std::copy(engineARight, engineARight + numSamples, outputRight);
        return;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numSamples; i++) {
        // Update crossfade position if not in manual control
        if (!manualControl_ && !paused_) {
            updateCrossfadeState();
        }
        
        // Calculate gains for both engines
        float gainA, gainB;
        calculateGains(position_, gainA, gainB);
        
        // Mix both channels
        outputLeft[i] = engineALeft[i] * gainA + engineBLeft[i] * gainB;
        outputRight[i] = engineARight[i] * gainA + engineBRight[i] * gainB;
    }
    
    // Update CPU usage
    auto endTime = std::chrono::high_resolution_clock::now();
    float processingTime = std::chrono::duration<float, std::micro>(endTime - startTime).count();
    cpuUsage_ = cpuUsage_ * 0.99f + (processingTime / numSamples) * 0.01f;
}

void EngineCrossfader::startCrossfadeToB() {
    if (!initialized_) {
        return;
    }
    
    targetPosition_ = 1.0f;
    state_ = CrossfadeState::CROSSFADING_A_TO_B;
    paused_ = false;
}

void EngineCrossfader::startCrossfadeToA() {
    if (!initialized_) {
        return;
    }
    
    targetPosition_ = 0.0f;
    state_ = CrossfadeState::CROSSFADING_B_TO_A;
    paused_ = false;
}

void EngineCrossfader::setCrossfadePosition(float position) {
    position_ = clamp(position, 0.0f, 1.0f);
    targetPosition_ = position_;
    
    // Update state based on position
    if (position_ == 0.0f) {
        state_ = CrossfadeState::ENGINE_A_ONLY;
    } else if (position_ == 1.0f) {
        state_ = CrossfadeState::ENGINE_B_ONLY;
    } else {
        state_ = (position_ > 0.5f) ? CrossfadeState::CROSSFADING_A_TO_B : 
                                      CrossfadeState::CROSSFADING_B_TO_A;
    }
}

void EngineCrossfader::snapToEngine(bool useEngineB) {
    position_ = useEngineB ? 1.0f : 0.0f;
    targetPosition_ = position_;
    state_ = useEngineB ? CrossfadeState::ENGINE_B_ONLY : CrossfadeState::ENGINE_A_ONLY;
    paused_ = false;
}

void EngineCrossfader::setCrossfadeTime(float timeMs) {
    crossfadeTimeMs_ = clamp(timeMs, MIN_CROSSFADE_TIME_MS, MAX_CROSSFADE_TIME_MS);
    if (initialized_) {
        calculateCrossfadeIncrement();
    }
}

void EngineCrossfader::setCrossfadeType(CrossfadeType type) {
    crossfadeType_ = type;
}

void EngineCrossfader::setSampleRate(float sampleRate) {
    if (sampleRate > 0.0f && std::abs(sampleRate - sampleRate_) > 0.1f) {
        sampleRate_ = sampleRate;
        if (initialized_) {
            calculateCrossfadeIncrement();
        }
    }
}

bool EngineCrossfader::isCrossfading() const {
    return (state_ == CrossfadeState::CROSSFADING_A_TO_B || 
            state_ == CrossfadeState::CROSSFADING_B_TO_A) && !paused_;
}

void EngineCrossfader::setManualControl(bool manual) {
    manualControl_ = manual;
}

void EngineCrossfader::pauseCrossfade() {
    paused_ = true;
}

void EngineCrossfader::resumeCrossfade() {
    paused_ = false;
}

void EngineCrossfader::reset() {
    position_ = 0.0f;
    targetPosition_ = 0.0f;
    state_ = CrossfadeState::ENGINE_A_ONLY;
    paused_ = false;
    manualControl_ = false;
    cpuUsage_ = 0.0f;
}

// Private method implementations

void EngineCrossfader::calculateCrossfadeIncrement() {
    if (sampleRate_ > 0.0f && crossfadeTimeMs_ > 0.0f) {
        float crossfadeTimeSeconds = crossfadeTimeMs_ * 0.001f;
        float totalSamples = crossfadeTimeSeconds * sampleRate_;
        positionIncrement_ = 1.0f / totalSamples;
    } else {
        positionIncrement_ = 0.01f; // Fallback
    }
}

void EngineCrossfader::updateCrossfadeState() {
    switch (state_) {
        case CrossfadeState::CROSSFADING_A_TO_B:
            position_ += positionIncrement_;
            if (position_ >= targetPosition_) {
                position_ = 1.0f;
                state_ = CrossfadeState::ENGINE_B_ONLY;
            }
            break;
            
        case CrossfadeState::CROSSFADING_B_TO_A:
            position_ -= positionIncrement_;
            if (position_ <= targetPosition_) {
                position_ = 0.0f;
                state_ = CrossfadeState::ENGINE_A_ONLY;
            }
            break;
            
        case CrossfadeState::ENGINE_A_ONLY:
        case CrossfadeState::ENGINE_B_ONLY:
            // No position update needed
            break;
    }
    
    position_ = clamp(position_, 0.0f, 1.0f);
}

void EngineCrossfader::calculateGains(float position, float& gainA, float& gainB) const {
    // Apply the selected crossfade curve
    float curvedPosition = applyCurve(position);
    
    switch (crossfadeType_) {
        case CrossfadeType::EQUAL_POWER_SINE:
            gainA = std::cos(curvedPosition * HALF_PI);
            gainB = std::sin(curvedPosition * HALF_PI);
            break;
            
        case CrossfadeType::EQUAL_POWER_SQRT:
            gainA = std::sqrt(1.0f - curvedPosition);
            gainB = std::sqrt(curvedPosition);
            break;
            
        case CrossfadeType::LINEAR:
            gainA = 1.0f - curvedPosition;
            gainB = curvedPosition;
            break;
            
        case CrossfadeType::CONSTANT_POWER:
            {
                // 6dB pan law
                float panAngle = (curvedPosition - 0.5f) * static_cast<float>(M_PI);
                gainA = (std::cos(panAngle) + std::sin(panAngle)) / SQRT_2;
                gainB = (std::cos(panAngle) - std::sin(panAngle)) / SQRT_2;
                gainA = std::max(0.0f, gainA);
                gainB = std::max(0.0f, gainB);
            }
            break;
            
        case CrossfadeType::S_CURVE:
        default:
            // S-curve is applied in applyCurve, then use sine/cosine
            gainA = std::cos(curvedPosition * HALF_PI);
            gainB = std::sin(curvedPosition * HALF_PI);
            break;
    }
    
    // Ensure gains are within reasonable bounds
    gainA = clamp(gainA, 0.0f, 1.0f);
    gainB = clamp(gainB, 0.0f, 1.0f);
}

float EngineCrossfader::applyCurve(float linearPosition) const {
    if (crossfadeType_ == CrossfadeType::S_CURVE) {
        return sCurve(linearPosition);
    }
    return linearPosition; // No additional curve for other types
}

float EngineCrossfader::equalPowerSine(float t) const {
    return std::sin(t * HALF_PI);
}

float EngineCrossfader::equalPowerSqrt(float t) const {
    return std::sqrt(t);
}

float EngineCrossfader::sCurve(float t) const {
    // Smooth S-curve using smoothstep function
    return t * t * (3.0f - 2.0f * t);
}

float EngineCrossfader::constantPower(float t) const {
    // Constant power crossfade
    return 0.5f * (1.0f + std::sin((t - 0.5f) * static_cast<float>(M_PI)));
}

float EngineCrossfader::clamp(float value, float min, float max) const {
    return std::max(min, std::min(max, value));
}