#pragma once
#include <cmath>

/**
 * DCBlocker - High-pass filter to remove DC offset and ultra-low frequencies
 * 
 * Implements a 1-pole high-pass filter at 24Hz to clean up DC offset
 * and unwanted low-frequency components after nonlinear processing.
 * 
 * Uses a single-pole RC-style filter with configurable cutoff frequency.
 * Optimized for real-time audio processing with minimal CPU overhead.
 */
class DCBlocker {
public:
    DCBlocker();
    ~DCBlocker() = default;
    
    // Initialization
    bool initialize(float sampleRate, float cutoffHz = 24.0f);
    void shutdown();
    
    // Processing
    float processSample(float input);
    void processBlock(float* output, const float* input, int numSamples);
    void processBlock(float* buffer, int numSamples); // In-place processing
    
    // Configuration
    void setCutoffFrequency(float hz);
    void setSampleRate(float sampleRate);
    
    // Analysis
    float getCutoffFrequency() const { return cutoffHz_; }
    bool isInitialized() const { return initialized_; }
    
    // State management
    void reset();
    void reset(float initialValue);
    
    // Batch processing for multiple channels
    static void processMultiple(DCBlocker* blockers, float** buffers, int numChannels, int numSamples);
    
private:
    float sampleRate_ = 44100.0f;
    float cutoffHz_ = 24.0f;
    bool initialized_ = false;
    
    // Filter state
    float x1_ = 0.0f;  // Previous input
    float y1_ = 0.0f;  // Previous output
    
    // Filter coefficients
    float a1_ = 0.0f;  // Feedback coefficient
    float b0_ = 0.0f;  // Feedforward coefficient
    float b1_ = 0.0f;  // Feedforward coefficient
    
    // Internal methods
    void calculateCoefficients();
    float clamp(float value, float min, float max) const;
    
    // Constants
    static constexpr float MIN_CUTOFF_HZ = 1.0f;
    static constexpr float MAX_CUTOFF_HZ = 200.0f;
    static constexpr float DEFAULT_CUTOFF_HZ = 24.0f;
    static constexpr float PI = 3.14159265358979323846f;
};