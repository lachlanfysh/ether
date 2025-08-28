#include "ExponentialMapper.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bool ExponentialMapper::initialized_ = false;

ExponentialMapper::ExponentialMapper() {
    cutoffLUT_.fill(0.0f);
    frequencyLUT_.fill(0.0f);
}

bool ExponentialMapper::initialize() {
    if (initialized_) {
        return true;
    }
    
    initialized_ = true;
    return true;
}

void ExponentialMapper::shutdown() {
    initialized_ = false;
}

// Primary mapping functions

float ExponentialMapper::mapCutoff(float normalizedInput) {
    float clamped = clamp(normalizedInput, 0.0f, 1.0f);
    return exponentialMap(clamped, MIN_CUTOFF_HZ, MAX_CUTOFF_HZ);
}

float ExponentialMapper::mapAudioFrequency(float normalizedInput) {
    float clamped = clamp(normalizedInput, 0.0f, 1.0f);
    return exponentialMap(clamped, MIN_AUDIO_HZ, MAX_AUDIO_HZ);
}

float ExponentialMapper::mapLFORate(float normalizedInput) {
    float clamped = clamp(normalizedInput, 0.0f, 1.0f);
    return exponentialMap(clamped, MIN_LFO_HZ, MAX_LFO_HZ);
}

float ExponentialMapper::mapDetuneCents(float normalizedInput) {
    float clamped = clamp(normalizedInput, 0.0f, 1.0f);
    
    // Perceptual detune mapping: cents = x² × 30
    // This gives fine control near center and wider range at extremes
    float centered = (clamped - 0.5f) * 2.0f; // -1 to +1
    float squared = centered * centered * (centered < 0.0f ? -1.0f : 1.0f); // Preserve sign
    
    return squared * MAX_DETUNE_CENTS;
}

float ExponentialMapper::mapResonance(float normalizedInput) {
    float clamped = clamp(normalizedInput, 0.0f, 1.0f);
    
    // Exponential Q mapping for musical resonance control
    return exponentialMap(clamped, MIN_Q_FACTOR, MAX_Q_FACTOR);
}

float ExponentialMapper::mapEnvelopeTime(float normalizedInput) {
    float clamped = clamp(normalizedInput, 0.0f, 1.0f);
    
    // Map directly to seconds range (0.0001s to 10s)
    return exponentialMap(clamped, MIN_ENV_TIME_MS * 0.001f, MAX_ENV_TIME_S);
}

// Inverse mapping functions

float ExponentialMapper::unmapCutoff(float frequency) {
    float clamped = clamp(frequency, MIN_CUTOFF_HZ, MAX_CUTOFF_HZ);
    return logarithmicMap(clamped, MIN_CUTOFF_HZ, MAX_CUTOFF_HZ);
}

float ExponentialMapper::unmapAudioFrequency(float frequency) {
    float clamped = clamp(frequency, MIN_AUDIO_HZ, MAX_AUDIO_HZ);
    return logarithmicMap(clamped, MIN_AUDIO_HZ, MAX_AUDIO_HZ);
}

float ExponentialMapper::unmapLFORate(float frequency) {
    float clamped = clamp(frequency, MIN_LFO_HZ, MAX_LFO_HZ);
    return logarithmicMap(clamped, MIN_LFO_HZ, MAX_LFO_HZ);
}

float ExponentialMapper::unmapDetuneCents(float cents) {
    float clamped = clamp(cents, -MAX_DETUNE_CENTS, MAX_DETUNE_CENTS);
    
    // Inverse of perceptual detune mapping
    float normalized = clamped / MAX_DETUNE_CENTS; // -1 to +1
    float sign = (normalized < 0.0f) ? -1.0f : 1.0f;
    float sqrt_val = std::sqrt(std::abs(normalized)) * sign;
    
    return (sqrt_val * 0.5f) + 0.5f; // Back to 0-1 range
}

float ExponentialMapper::unmapResonance(float qFactor) {
    float clamped = clamp(qFactor, MIN_Q_FACTOR, MAX_Q_FACTOR);
    return logarithmicMap(clamped, MIN_Q_FACTOR, MAX_Q_FACTOR);
}

float ExponentialMapper::unmapEnvelopeTime(float timeSeconds) {
    float clamped = clamp(timeSeconds, MIN_ENV_TIME_MS * 0.001f, MAX_ENV_TIME_S);
    return logarithmicMap(clamped, MIN_ENV_TIME_MS * 0.001f, MAX_ENV_TIME_S);
}

// Custom range mapping

float ExponentialMapper::mapExponential(float normalizedInput, float minValue, float maxValue) {
    float clamped = clamp(normalizedInput, 0.0f, 1.0f);
    return exponentialMap(clamped, minValue, maxValue);
}

float ExponentialMapper::unmapExponential(float value, float minValue, float maxValue) {
    float clamped = clamp(value, minValue, maxValue);
    return logarithmicMap(clamped, minValue, maxValue);
}

float ExponentialMapper::mapPower(float normalizedInput, float minValue, float maxValue, float power) {
    float clamped = clamp(normalizedInput, 0.0f, 1.0f);
    float powered = std::pow(clamped, power);
    return minValue + powered * (maxValue - minValue);
}

float ExponentialMapper::unmapPower(float value, float minValue, float maxValue, float power) {
    float clamped = clamp(value, minValue, maxValue);
    float normalized = (clamped - minValue) / (maxValue - minValue);
    return std::pow(normalized, 1.0f / power);
}

// Musical utility functions

float ExponentialMapper::noteToFrequency(float midiNote) {
    return A4_FREQUENCY * std::pow(2.0f, (midiNote - A4_MIDI_NOTE) / SEMITONES_PER_OCTAVE);
}

float ExponentialMapper::frequencyToNote(float frequency) {
    if (frequency <= 0.0f) return 0.0f;
    return A4_MIDI_NOTE + SEMITONES_PER_OCTAVE * std::log2(frequency / A4_FREQUENCY);
}

float ExponentialMapper::centsToRatio(float cents) {
    return std::pow(2.0f, cents / CENTS_PER_OCTAVE);
}

float ExponentialMapper::ratioToCents(float ratio) {
    if (ratio <= 0.0f) return 0.0f;
    return CENTS_PER_OCTAVE * std::log2(ratio);
}

// Perceptual curves

float ExponentialMapper::perceptualVolume(float linearGain) {
    // Convert linear gain to dB, then apply perceptual curve
    if (linearGain <= 0.0f) return 0.0f;
    float dB = 20.0f * std::log10(linearGain);
    
    // Perceptual loudness curve (approximate)
    float perceptual = (dB + 60.0f) / 60.0f; // Rough mapping
    return clamp(perceptual, 0.0f, 1.0f);
}

float ExponentialMapper::linearVolume(float perceptualGain) {
    float clamped = clamp(perceptualGain, 0.0f, 1.0f);
    
    // Inverse of perceptual curve
    float dB = (clamped * 60.0f) - 60.0f;
    return std::pow(10.0f, dB / 20.0f);
}

float ExponentialMapper::perceptualPitch(float frequency) {
    if (frequency <= 0.0f) return 0.0f;
    
    // Convert to mel scale (perceptual pitch)
    return 2595.0f * std::log10(1.0f + frequency / 700.0f);
}

float ExponentialMapper::linearPitch(float perceptualPitch) {
    // Convert from mel scale back to Hz
    return 700.0f * (std::pow(10.0f, perceptualPitch / 2595.0f) - 1.0f);
}

// Lookup table optimization

void ExponentialMapper::buildLookupTables() {
    // Build cutoff frequency LUT
    for (int i = 0; i < LUT_SIZE; i++) {
        float normalized = static_cast<float>(i) / (LUT_SIZE - 1);
        cutoffLUT_[i] = mapCutoff(normalized);
    }
    
    // Build audio frequency LUT
    for (int i = 0; i < LUT_SIZE; i++) {
        float normalized = static_cast<float>(i) / (LUT_SIZE - 1);
        frequencyLUT_[i] = mapAudioFrequency(normalized);
    }
}

float ExponentialMapper::mapCutoffLUT(float normalizedInput) const {
    float clamped = clamp(normalizedInput, 0.0f, 1.0f);
    float floatIndex = clamped * (LUT_SIZE - 1);
    int index = static_cast<int>(floatIndex);
    
    if (index >= LUT_SIZE - 1) {
        return cutoffLUT_[LUT_SIZE - 1];
    }
    
    // Linear interpolation between LUT points
    float fraction = floatIndex - index;
    return cutoffLUT_[index] + fraction * (cutoffLUT_[index + 1] - cutoffLUT_[index]);
}

float ExponentialMapper::mapFrequencyLUT(float normalizedInput) const {
    float clamped = clamp(normalizedInput, 0.0f, 1.0f);
    float floatIndex = clamped * (LUT_SIZE - 1);
    int index = static_cast<int>(floatIndex);
    
    if (index >= LUT_SIZE - 1) {
        return frequencyLUT_[LUT_SIZE - 1];
    }
    
    // Linear interpolation between LUT points
    float fraction = floatIndex - index;
    return frequencyLUT_[index] + fraction * (frequencyLUT_[index + 1] - frequencyLUT_[index]);
}

// Utility functions

float ExponentialMapper::clamp(float value, float min, float max) {
    return std::max(min, std::min(max, value));
}

// Private internal mapping functions

float ExponentialMapper::exponentialMap(float input, float minValue, float maxValue) {
    if (minValue <= 0.0f || maxValue <= 0.0f) {
        // Linear fallback for invalid ranges
        return minValue + input * (maxValue - minValue);
    }
    
    float logMin = std::log(minValue);
    float logMax = std::log(maxValue);
    float logRange = logMax - logMin;
    
    return std::exp(logMin + input * logRange);
}

float ExponentialMapper::logarithmicMap(float value, float minValue, float maxValue) {
    if (minValue <= 0.0f || maxValue <= 0.0f || value <= 0.0f) {
        // Linear fallback for invalid ranges
        return (value - minValue) / (maxValue - minValue);
    }
    
    float logMin = std::log(minValue);
    float logMax = std::log(maxValue);
    float logValue = std::log(value);
    float logRange = logMax - logMin;
    
    if (logRange == 0.0f) return 0.0f;
    
    return (logValue - logMin) / logRange;
}