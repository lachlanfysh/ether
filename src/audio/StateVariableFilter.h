#pragma once

/**
 * StateVariableFilter - Basic state variable filter implementation
 * 
 * Provides simultaneous lowpass, highpass, and bandpass outputs
 * with resonance control. Optimized for real-time audio processing.
 */
class StateVariableFilter {
public:
    StateVariableFilter();
    ~StateVariableFilter() = default;
    
    // Filter configuration
    bool initialize(float sampleRate);  // Initialize with sample rate
    void setCutoffFrequency(float frequency);
    void setResonance(float resonance);  // Q factor (0.1 to 30.0)
    void setSampleRate(float sampleRate);
    
    // Audio processing
    float processLowpass(float input);
    float processHighpass(float input);
    float processBandpass(float input);
    
    // Process and return all outputs simultaneously
    struct FilterOutputs {
        float lowpass;
        float highpass;
        float bandpass;
    };
    FilterOutputs processAll(float input);
    
    // Reset filter state
    void reset();
    
private:
    float sampleRate_ = 48000.0f;
    float cutoffFreq_ = 1000.0f;
    float resonance_ = 1.0f;
    
    // Filter coefficients
    float f_;  // Cutoff coefficient
    float q_;  // Damping coefficient
    
    // Filter state variables
    float bp_ = 0.0f;  // Bandpass state
    float lp_ = 0.0f;  // Lowpass state
    
    void updateCoefficients();
};