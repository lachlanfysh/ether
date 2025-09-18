#pragma once

#include <array>
#include <cmath>
#include <algorithm>

namespace EtherSynth {

enum CoreParameter {
    // Core Synth (0-2)
    PARAM_HARMONICS = 0,     // Harmonic content/overtones
    PARAM_TIMBRE = 1,        // Timbral character/brightness  
    PARAM_MORPH = 2,         // Morphing/blend control
    
    // Envelope (3-6)
    PARAM_ATTACK = 3,        // Attack time
    PARAM_DECAY = 4,         // Decay time
    PARAM_SUSTAIN = 5,       // Sustain level
    PARAM_RELEASE = 6,       // Release time
    
    // Tone/Mix (7-13)
    PARAM_FILTER_CUTOFF = 7, // Low-pass cutoff frequency
    PARAM_FILTER_RESONANCE = 8, // Filter resonance/Q
    PARAM_HPF = 9,           // High-pass filter cutoff
    PARAM_VOLUME = 10,       // Engine volume level
    PARAM_PAN = 11,          // Stereo pan position
    PARAM_AMPLITUDE = 12,    // Pre-filter amplitude
    PARAM_CLIP = 13,         // Soft clipping amount
    
    // Performance (14-15)
    PARAM_ACCENT_AMOUNT = 14, // Accent sensitivity
    PARAM_GLIDE_TIME = 15,    // Portamento/glide time
    
    CORE_PARAM_COUNT = 16
};

struct CoreParameterSpec {
    float min_val;
    float max_val;
    bool is_exponential;
    bool is_quadratic;
    const char* unit;
};

static const std::array<CoreParameterSpec, CORE_PARAM_COUNT> PARAM_SPECS = {{
    // Core Synth
    {0.0f, 1.0f, false, false, "norm"},     // HARMONICS
    {0.0f, 1.0f, false, false, "norm"},     // TIMBRE
    {0.0f, 1.0f, false, false, "norm"},     // MORPH
    
    // Envelope
    {0.001f, 10.0f, true, false, "sec"},    // ATTACK
    {0.001f, 10.0f, true, false, "sec"},    // DECAY
    {0.0f, 1.0f, false, false, "level"},    // SUSTAIN
    {0.001f, 10.0f, true, false, "sec"},    // RELEASE
    
    // Tone/Mix
    {20.0f, 20000.0f, true, false, "Hz"},   // FILTER_CUTOFF
    {0.0f, 1.0f, false, true, "Q"},         // FILTER_RESONANCE
    {20.0f, 1000.0f, true, false, "Hz"},    // HPF
    {0.0f, 1.0f, false, false, "dB"},       // VOLUME
    {-1.0f, 1.0f, false, false, "L/R"},     // PAN
    {0.0f, 2.0f, false, false, "gain"},     // AMPLITUDE
    {0.0f, 1.0f, false, false, "amount"},   // CLIP
    
    // Performance
    {0.0f, 1.0f, false, false, "norm"},     // ACCENT_AMOUNT
    {0.0f, 5.0f, true, false, "sec"}        // GLIDE_TIME
}};

struct CoreParams {
    std::array<float, CORE_PARAM_COUNT> values;
    
    CoreParams() {
        // Initialize with sensible defaults
        values[PARAM_HARMONICS] = 0.5f;
        values[PARAM_TIMBRE] = 0.7f;
        values[PARAM_MORPH] = 0.0f;
        values[PARAM_ATTACK] = 0.01f;
        values[PARAM_DECAY] = 0.3f;
        values[PARAM_SUSTAIN] = 0.7f;
        values[PARAM_RELEASE] = 0.5f;
        values[PARAM_FILTER_CUTOFF] = 0.8f;  // ~8kHz
        values[PARAM_FILTER_RESONANCE] = 0.3f;
        values[PARAM_HPF] = 0.0f;  // 20Hz
        values[PARAM_VOLUME] = 0.8f;
        values[PARAM_PAN] = 0.0f;  // Center
        values[PARAM_AMPLITUDE] = 1.0f;
        values[PARAM_CLIP] = 0.0f;
        values[PARAM_ACCENT_AMOUNT] = 0.5f;
        values[PARAM_GLIDE_TIME] = 0.0f;
    }
    
    float& operator[](CoreParameter param) { return values[param]; }
    const float& operator[](CoreParameter param) const { return values[param]; }
};

// Parameter scaling and conversion functions
class ParameterUtils {
public:
    // Exponential scaling for time-based and frequency parameters
    static float expScale(float norm, float min_val, float max_val) {
        norm = std::clamp(norm, 0.0f, 1.0f);
        return min_val * std::pow(max_val / min_val, norm);
    }
    
    // Quadratic scaling for resonance
    static float quadScale(float norm) {
        norm = std::clamp(norm, 0.0f, 1.0f);
        return 0.5f + norm * norm * 9.5f; // 0.5 → 10.0 Q
    }
    
    // Volume scaling (exponential dB)
    static float volumeScale(float norm) {
        norm = std::clamp(norm, 0.0f, 1.0f);
        if (norm <= 0.001f) return 0.0f; // -inf dB
        return norm; // Linear for now, exponential curve optional
    }
    
    // Equal power pan law
    static void panLaw(float pan, float& left_gain, float& right_gain) {
        pan = std::clamp(pan, -1.0f, 1.0f);
        float angle = (pan + 1.0f) * 0.25f * M_PI; // -1→1 maps to 0→π/2
        left_gain = std::cos(angle);
        right_gain = std::sin(angle);
    }
    
    // Convert normalized parameter to actual value
    static float getScaledValue(CoreParameter param, float norm_value) {
        const auto& spec = PARAM_SPECS[param];
        norm_value = std::clamp(norm_value, 0.0f, 1.0f);
        
        if (spec.is_exponential) {
            return expScale(norm_value, spec.min_val, spec.max_val);
        } else if (spec.is_quadratic && param == PARAM_FILTER_RESONANCE) {
            return quadScale(norm_value);
        } else if (param == PARAM_VOLUME) {
            return volumeScale(norm_value);
        } else {
            // Linear scaling
            return spec.min_val + norm_value * (spec.max_val - spec.min_val);
        }
    }
    
    // Validate and clamp parameter value
    static float validateParameter(CoreParameter param, float value) {
        if (std::isnan(value) || std::isinf(value)) {
            return PARAM_SPECS[param].min_val; // Fallback to minimum
        }
        return std::clamp(value, PARAM_SPECS[param].min_val, PARAM_SPECS[param].max_val);
    }
    
    // Get parameter name for debugging/UI
    static const char* getParameterName(CoreParameter param) {
        static const char* names[CORE_PARAM_COUNT] = {
            "Harmonics", "Timbre", "Morph",
            "Attack", "Decay", "Sustain", "Release", 
            "Filter Cutoff", "Filter Resonance", "HPF",
            "Volume", "Pan", "Amplitude", "Clip",
            "Accent Amount", "Glide Time"
        };
        return (param >= 0 && param < CORE_PARAM_COUNT) ? names[param] : "Unknown";
    }
};

} // namespace EtherSynth