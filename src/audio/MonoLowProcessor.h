#pragma once
#include <cmath>
#include <array>

/**
 * MonoLowProcessor - Bass frequency mono-summing processor
 * 
 * Implements frequency-selective mono summing below a configurable crossover 
 * frequency (default 120Hz) to maintain bass coherence and prevent phase 
 * cancellation issues in the low end after stereo width/chorus processing.
 * 
 * Features:
 * - Linkwitz-Riley crossover at 120Hz (configurable)
 * - Preserves stereo image above crossover frequency
 * - Mono-sums bass frequencies for club system compatibility
 * - Real-time safe processing with minimal latency
 * - CPU optimized for embedded deployment
 */
class MonoLowProcessor {
public:
    MonoLowProcessor();
    ~MonoLowProcessor() = default;
    
    // Initialization
    bool initialize(float sampleRate, float crossoverHz = 120.0f);
    void shutdown();
    
    // Processing
    void processStereo(float& left, float& right);
    void processBlock(float* leftChannel, float* rightChannel, int numSamples);
    
    // Configuration  
    void setCrossoverFrequency(float hz);
    void setBypass(bool bypass);
    void setSampleRate(float sampleRate);
    void setMonoGain(float gain = 1.0f); // Gain applied to mono-summed low frequencies
    
    // Analysis
    float getCrossoverFrequency() const { return crossoverHz_; }
    bool isBypassed() const { return bypassed_; }
    bool isInitialized() const { return initialized_; }
    float getMagnitudeResponse(float frequency, bool lowBand = true) const;
    
    // State management
    void reset();
    void reset(float initialValue);
    
    // Performance monitoring
    float getCPUUsage() const { return cpuUsage_; }
    
private:
    float sampleRate_ = 44100.0f;
    float crossoverHz_ = 120.0f;
    float monoGain_ = 1.0f;
    bool bypassed_ = false;
    bool initialized_ = false;
    
    // Linkwitz-Riley crossover filters (4th order = 2x 2nd order in series)
    // Low-pass filters for extracting bass
    struct BiquadState {
        float x1 = 0.0f, x2 = 0.0f;  // Input history
        float y1 = 0.0f, y2 = 0.0f;  // Output history
    };
    
    struct BiquadCoeffs {
        float b0, b1, b2;  // Feedforward coefficients
        float a1, a2;      // Feedback coefficients (a0 normalized to 1)
    };
    
    // Left channel filters
    BiquadState leftLowPass1_, leftLowPass2_;    // Low-pass chain for bass extraction
    BiquadState leftHighPass1_, leftHighPass2_;  // High-pass chain for highs
    
    // Right channel filters  
    BiquadState rightLowPass1_, rightLowPass2_;   // Low-pass chain for bass extraction
    BiquadState rightHighPass1_, rightHighPass2_; // High-pass chain for highs
    
    // Filter coefficients
    BiquadCoeffs lowPassCoeffs_;
    BiquadCoeffs highPassCoeffs_;
    
    // Performance monitoring
    mutable float cpuUsage_ = 0.0f;
    
    // Internal methods
    void calculateCoefficients();
    float processBiquad(float input, BiquadState& state, const BiquadCoeffs& coeffs) const;
    void resetBiquadState(BiquadState& state);
    float clamp(float value, float min, float max) const;
    
    // Constants
    static constexpr float MIN_CROSSOVER_HZ = 40.0f;
    static constexpr float MAX_CROSSOVER_HZ = 300.0f;
    static constexpr float DEFAULT_CROSSOVER_HZ = 120.0f;
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float SQRT2 = 1.4142135623730951f;
};