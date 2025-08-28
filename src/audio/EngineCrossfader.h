#pragma once
#include <cmath>
#include <memory>

/**
 * EngineCrossfader - Equal-power crossfading for smooth engine transitions
 * 
 * Implements professional-grade equal-power crossfading between two synthesis
 * engines with configurable crossfade time (25-40ms) to eliminate pops and
 * clicks during engine switching. Essential for seamless sound morphing.
 * 
 * Features:
 * - Equal-power crossfade law for constant perceived loudness
 * - Configurable crossfade time (25-40ms default)
 * - Smooth S-curve or linear fade shapes
 * - Real-time safe processing with no memory allocation
 * - Support for multiple crossfade curves (sine/cosine, square-root, S-curve)
 * - CPU optimized for embedded deployment
 */
class EngineCrossfader {
public:
    enum class CrossfadeType {
        EQUAL_POWER_SINE,    // Sine/cosine law (most common)
        EQUAL_POWER_SQRT,    // Square root law  
        S_CURVE,             // Smooth S-curve transition
        LINEAR,              // Linear crossfade (not equal power)
        CONSTANT_POWER       // True constant power (6dB pan law)
    };
    
    enum class CrossfadeState {
        ENGINE_A_ONLY,       // Only engine A active
        ENGINE_B_ONLY,       // Only engine B active
        CROSSFADING_A_TO_B,  // Transitioning from A to B
        CROSSFADING_B_TO_A   // Transitioning from B to A
    };
    
    EngineCrossfader();
    ~EngineCrossfader() = default;
    
    // Initialization
    bool initialize(float sampleRate, float crossfadeTimeMs = 30.0f);
    void shutdown();
    
    // Processing
    float processMix(float engineA, float engineB);
    void processBlock(float* engineABuffer, float* engineBBuffer, float* output, int numSamples);
    void processStereoBlock(float* engineALeft, float* engineARight,
                           float* engineBLeft, float* engineBRight,
                           float* outputLeft, float* outputRight, int numSamples);
    
    // Crossfade control
    void startCrossfadeToB(); // Start fade from A to B
    void startCrossfadeToA(); // Start fade from B to A
    void setCrossfadePosition(float position); // 0.0 = A only, 1.0 = B only
    void snapToEngine(bool useEngineB); // Instant switch without crossfade
    
    // Configuration
    void setCrossfadeTime(float timeMs);
    void setCrossfadeType(CrossfadeType type);
    void setSampleRate(float sampleRate);
    
    // Analysis
    CrossfadeState getCurrentState() const { return state_; }
    float getCrossfadePosition() const { return position_; }
    float getCrossfadeTimeMs() const { return crossfadeTimeMs_; }
    bool isCrossfading() const;
    bool isInitialized() const { return initialized_; }
    
    // Advanced control
    void setManualControl(bool manual); // Enable manual position control
    void pauseCrossfade(); // Pause current crossfade
    void resumeCrossfade(); // Resume paused crossfade
    
    // State management
    void reset();
    
    // Performance monitoring
    float getCPUUsage() const { return cpuUsage_; }
    
private:
    float sampleRate_ = 44100.0f;
    float crossfadeTimeMs_ = 30.0f;
    CrossfadeType crossfadeType_ = CrossfadeType::EQUAL_POWER_SINE;
    bool initialized_ = false;
    bool manualControl_ = false;
    bool paused_ = false;
    
    // Crossfade state
    CrossfadeState state_ = CrossfadeState::ENGINE_A_ONLY;
    float position_ = 0.0f; // 0.0 = A only, 1.0 = B only
    float targetPosition_ = 0.0f;
    float positionIncrement_ = 0.0f;
    
    // Performance monitoring
    mutable float cpuUsage_ = 0.0f;
    
    // Internal methods
    void calculateCrossfadeIncrement();
    void updateCrossfadeState();
    void calculateGains(float position, float& gainA, float& gainB) const;
    float applyCurve(float linearPosition) const;
    float clamp(float value, float min, float max) const;
    
    // Curve functions
    float equalPowerSine(float t) const;
    float equalPowerSqrt(float t) const;
    float sCurve(float t) const;
    float constantPower(float t) const;
    
    // Constants
    static constexpr float MIN_CROSSFADE_TIME_MS = 5.0f;
    static constexpr float MAX_CROSSFADE_TIME_MS = 500.0f;
    static constexpr float DEFAULT_CROSSFADE_TIME_MS = 30.0f;
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float HALF_PI = 1.5707963267948966f;
    static constexpr float SQRT_2 = 1.4142135623730951f;
};