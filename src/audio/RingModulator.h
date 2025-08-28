#pragma once
#include <cmath>

/**
 * RingModulator - Simple ring modulation effect
 * 
 * Multiplies input signal with an oscillator for ring modulation effects.
 * Provides both internal oscillator and external modulation input modes.
 */
class RingModulator {
public:
    RingModulator();
    ~RingModulator() = default;
    
    // Configuration
    void setModulationFrequency(float frequency);
    void setModulationDepth(float depth);  // 0.0 to 1.0
    void setSampleRate(float sampleRate);
    
    // Audio processing
    float process(float input);
    float process(float input, float externalModulator);  // Use external modulator
    
    // Reset state
    void reset();
    
private:
    float sampleRate_ = 48000.0f;
    float modFreq_ = 440.0f;
    float modDepth_ = 1.0f;
    
    // Oscillator state
    float phase_ = 0.0f;
    float phaseIncrement_ = 0.0f;
    
    void updatePhaseIncrement();
};