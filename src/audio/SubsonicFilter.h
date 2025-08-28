#pragma once
#include <cmath>
#include <array>

/**
 * SubsonicFilter - Advanced low-frequency cleanup filter
 * 
 * Implements a 2-pole high-pass filter at 24Hz with optional steepness control
 * for removing subsonic frequencies and cleaning up after nonlinear processing.
 * 
 * Features:
 * - 2-pole Butterworth or Linkwitz-Riley response
 * - Adjustable Q factor for different responses
 * - Zero-delay feedback topology for stability
 * - Optional DC blocker pre-stage
 * - CPU optimized with coefficient caching
 */
class SubsonicFilter {
public:
    enum class FilterType {
        BUTTERWORTH,    // Standard 2-pole Butterworth (Q=0.707)
        LINKWITZ_RILEY, // Linkwitz-Riley (Q=0.5, good for crossovers)
        CRITICAL,       // Critically damped (Q=0.5)
        CUSTOM          // User-defined Q factor
    };
    
    SubsonicFilter();
    ~SubsonicFilter() = default;
    
    // Initialization
    bool initialize(float sampleRate, float cutoffHz = 24.0f, FilterType type = FilterType::BUTTERWORTH);
    void shutdown();
    
    // Processing
    float processSample(float input);
    void processBlock(float* output, const float* input, int numSamples);
    void processBlock(float* buffer, int numSamples); // In-place processing
    
    // Configuration
    void setCutoffFrequency(float hz);
    void setFilterType(FilterType type);
    void setQFactor(float q); // Only used when type = CUSTOM
    void setSampleRate(float sampleRate);
    void enableDCBlocker(bool enable); // Pre-stage DC blocker
    
    // Analysis
    float getCutoffFrequency() const { return cutoffHz_; }
    float getQFactor() const { return qFactor_; }
    FilterType getFilterType() const { return filterType_; }
    bool isInitialized() const { return initialized_; }
    float getMagnitudeResponse(float frequency) const;
    
    // State management
    void reset();
    void reset(float initialValue);
    
    // Batch processing for multiple channels
    static void processMultiple(SubsonicFilter* filters, float** buffers, int numChannels, int numSamples);
    
private:
    float sampleRate_ = 44100.0f;
    float cutoffHz_ = 24.0f;
    float qFactor_ = 0.7071f;
    FilterType filterType_ = FilterType::BUTTERWORTH;
    bool dcBlockerEnabled_ = true;
    bool initialized_ = false;
    
    // Filter state (biquad)
    std::array<float, 2> x_; // Input history [x[n-1], x[n-2]]
    std::array<float, 2> y_; // Output history [y[n-1], y[n-2]]
    
    // Filter coefficients
    float b0_, b1_, b2_; // Feedforward coefficients
    float a1_, a2_;      // Feedback coefficients
    
    // DC blocker (pre-stage)
    float dcX1_ = 0.0f;
    float dcY1_ = 0.0f;
    float dcA1_ = 0.0f;
    float dcB0_ = 0.0f;
    float dcB1_ = 0.0f;
    
    // Internal methods
    void calculateCoefficients();
    void calculateDCBlockerCoefficients();
    void updateQFromFilterType();
    float clamp(float value, float min, float max) const;
    
    // Constants
    static constexpr float MIN_CUTOFF_HZ = 5.0f;
    static constexpr float MAX_CUTOFF_HZ = 200.0f;
    static constexpr float MIN_Q = 0.1f;
    static constexpr float MAX_Q = 10.0f;
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float SQRT2 = 1.4142135623730951f;
};