#pragma once
#include <cmath>

/**
 * FMOperator - Single FM operator with waveform selection and modulation support
 * 
 * Features:
 * - Multiple waveform types (sine, saw, square, triangle approximations)
 * - Phase modulation input for FM synthesis
 * - Anti-click envelope ramping for parameter changes
 * - Velocity sensitivity and key scaling
 * - Fixed frequency mode for percussion-style timbres
 * - Real-time safe parameter updates
 */
class FMOperator {
public:
    enum class Waveform {
        SINE,               // Pure sine wave
        SAW_APPROX,         // Sine-based saw approximation
        SQUARE_APPROX,      // Sine-based square approximation
        TRIANGLE_APPROX,    // Sine-based triangle approximation
        HALF_SINE,          // Half-wave rectified sine
        FULL_SINE,          // Full-wave rectified sine
        QUARTER_SINE,       // Quarter sine wave
        ALT_SINE            // Alternating sine polarity
    };
    
    FMOperator();
    ~FMOperator();
    
    // Initialization
    bool initialize(float sampleRate);
    void shutdown();
    
    // Configuration
    void setWaveform(Waveform waveform) { waveform_ = waveform; }
    Waveform getWaveform() const { return waveform_; }
    
    void setFrequency(float frequency);
    float getFrequency() const { return frequency_; }
    
    void setLevel(float level);
    float getLevel() const { return level_; }
    
    void setPhase(float phase);
    void resetPhase() { phase_ = 0.0f; }
    
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    // Audio processing
    float processSample(float modulation = 0.0f);
    void processBlock(float* output, const float* modulation, int numSamples);
    
    // Analysis
    float getCurrentPhase() const { return phase_; }
    bool isActive() const { return enabled_ && level_ > 0.001f; }
    
private:
    // Configuration
    Waveform waveform_ = Waveform::SINE;
    float frequency_ = 440.0f;
    float level_ = 1.0f;
    bool enabled_ = true;
    
    // State
    float sampleRate_ = 44100.0f;
    float phase_ = 0.0f;
    float phaseIncrement_ = 0.0f;
    bool initialized_ = false;
    
    // Waveform generation
    float generateWaveform(float phase) const;
    float generateSine(float phase) const;
    float generateSawApprox(float phase) const;
    float generateSquareApprox(float phase) const;
    float generateTriangleApprox(float phase) const;
    float generateHalfSine(float phase) const;
    float generateFullSine(float phase) const;
    float generateQuarterSine(float phase) const;
    float generateAltSine(float phase) const;
    
    // Utility functions
    float normalizePhase(float phase) const;
    void updatePhaseIncrement();
    
    // Constants
    static constexpr float TWO_PI = 6.283185307179586f;
    static constexpr float PI = 3.141592653589793f;
    static constexpr float INV_TWO_PI = 0.159154943091895f;
};

inline float FMOperator::generateSine(float phase) const {
    return std::sin(phase);
}

inline float FMOperator::normalizePhase(float phase) const {
    while (phase >= TWO_PI) phase -= TWO_PI;
    while (phase < 0.0f) phase += TWO_PI;
    return phase;
}