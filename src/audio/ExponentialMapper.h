#pragma once
#include <cmath>
#include <array>

/**
 * ExponentialMapper - Perceptual parameter mapping utilities
 * 
 * Provides exponential and logarithmic mapping functions for audio parameters
 * that need to match human perception. Essential for natural-feeling controls
 * for filter cutoff, detune, and other frequency-domain parameters.
 * 
 * Features:
 * - Exponential cutoff mapping (20Hz-12kHz from 0-1 input)
 * - Perceptual detune mapping (cents = x² × 30)
 * - Frequency to pitch and pitch to frequency conversion
 * - CPU-optimized lookup tables for common mappings
 * - Configurable ranges and curves
 */
class ExponentialMapper {
public:
    // Predefined mapping types
    enum class MappingType {
        FILTER_CUTOFF,     // 20Hz to 12kHz exponential
        AUDIO_FREQUENCY,   // 20Hz to 20kHz exponential  
        LFO_RATE,          // 0.01Hz to 100Hz exponential
        DETUNE_CENTS,      // Perceptual detune mapping
        RESONANCE,         // Linear to exponential Q mapping
        ENVELOPE_TIME,     // 0.1ms to 10s exponential
        CUSTOM             // User-defined range
    };
    
    ExponentialMapper();
    ~ExponentialMapper() = default;
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Primary mapping functions
    static float mapCutoff(float normalizedInput);           // 0-1 → 20Hz-12kHz
    static float mapAudioFrequency(float normalizedInput);   // 0-1 → 20Hz-20kHz
    static float mapLFORate(float normalizedInput);          // 0-1 → 0.01Hz-100Hz
    static float mapDetuneCents(float normalizedInput);      // 0-1 → cents (x²×30)
    static float mapResonance(float normalizedInput);        // 0-1 → Q factor
    static float mapEnvelopeTime(float normalizedInput);     // 0-1 → 0.1ms-10s
    
    // Inverse mapping functions (frequency to normalized)
    static float unmapCutoff(float frequency);
    static float unmapAudioFrequency(float frequency);
    static float unmapLFORate(float frequency);
    static float unmapDetuneCents(float cents);
    static float unmapResonance(float qFactor);
    static float unmapEnvelopeTime(float timeSeconds);
    
    // Custom range mapping
    static float mapExponential(float normalizedInput, float minValue, float maxValue);
    static float unmapExponential(float value, float minValue, float maxValue);
    static float mapPower(float normalizedInput, float minValue, float maxValue, float power);
    static float unmapPower(float value, float minValue, float maxValue, float power);
    
    // Musical utility functions
    static float noteToFrequency(float midiNote);
    static float frequencyToNote(float frequency);
    static float centsToRatio(float cents);
    static float ratioToCents(float ratio);
    
    // Perceptual curves
    static float perceptualVolume(float linearGain);         // Linear to perceived volume
    static float linearVolume(float perceptualGain);         // Perceived to linear volume
    static float perceptualPitch(float frequency);           // Frequency to perceived pitch
    static float linearPitch(float perceptualPitch);         // Perceived pitch to frequency
    
    // Lookup table optimization (for real-time use)
    void buildLookupTables();
    float mapCutoffLUT(float normalizedInput) const;
    float mapFrequencyLUT(float normalizedInput) const;
    
    // Analysis and utilities
    static float clamp(float value, float min, float max);
    static bool isInitialized() { return initialized_; }
    
private:
    static bool initialized_;
    
    // Lookup tables for optimized real-time performance
    static constexpr int LUT_SIZE = 1024;
    std::array<float, LUT_SIZE> cutoffLUT_;
    std::array<float, LUT_SIZE> frequencyLUT_;
    
    // Internal mapping functions
    static float exponentialMap(float input, float minValue, float maxValue);
    static float logarithmicMap(float value, float minValue, float maxValue);
    
    // Constants for common mappings
    static constexpr float MIN_CUTOFF_HZ = 20.0f;
    static constexpr float MAX_CUTOFF_HZ = 12000.0f;
    static constexpr float MIN_AUDIO_HZ = 20.0f;
    static constexpr float MAX_AUDIO_HZ = 20000.0f;
    static constexpr float MIN_LFO_HZ = 0.01f;
    static constexpr float MAX_LFO_HZ = 100.0f;
    static constexpr float MAX_DETUNE_CENTS = 30.0f;
    static constexpr float MIN_Q_FACTOR = 0.1f;
    static constexpr float MAX_Q_FACTOR = 50.0f;
    static constexpr float MIN_ENV_TIME_MS = 0.1f;
    static constexpr float MAX_ENV_TIME_S = 10.0f;
    
    // Musical constants
    static constexpr float A4_FREQUENCY = 440.0f;
    static constexpr float A4_MIDI_NOTE = 69.0f;
    static constexpr float CENTS_PER_OCTAVE = 1200.0f;
    static constexpr float SEMITONES_PER_OCTAVE = 12.0f;
    static constexpr float LOG_2 = 0.6931471805599453f;
    static constexpr float LOG_10 = 2.302585092994046f;
};