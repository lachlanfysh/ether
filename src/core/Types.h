#pragma once
#include <cstdint>
#include <array>

// Core system constants
constexpr float SAMPLE_RATE = 48000.0f;
constexpr size_t BUFFER_SIZE = 128;  // Reduced for lower latency
constexpr float TWO_PI = 2.0f * M_PI;
constexpr size_t MAX_VOICES = 16;
constexpr size_t MAX_INSTRUMENTS = 8;
constexpr size_t PATTERN_STEPS = 16;

// Instrument color system (Muted palette to match SwiftUI)
enum class InstrumentColor : uint8_t {
    CORAL = 0,
    PEACH = 1, 
    CREAM = 2,
    SAGE = 3,
    TEAL = 4,
    SLATE = 5,
    PEARL = 6,
    STONE = 7,
    COUNT = 8
};

// RGB color values for each instrument (Muted palette)
constexpr std::array<uint32_t, 8> INSTRUMENT_COLORS = {
    0xD1AE9E,  // Coral (0.82, 0.68, 0.62)
    0xE3C8BC,  // Peach (0.89, 0.78, 0.74)
    0xEDE8E0,  // Cream (0.93, 0.91, 0.88)
    0xBDCFC2,  // Sage (0.74, 0.81, 0.76)
    0xA6C0BA,  // Teal (0.65, 0.75, 0.73)
    0x8A8A8A,  // Slate (0.54, 0.54, 0.54)
    0xE8E6DD,  // Pearl (0.91, 0.90, 0.87)
    0xE0D9D1   // Stone (0.88, 0.85, 0.82)
};

// Parameter system
enum class ParameterID : uint8_t {
    // Synthesis parameters
    HARMONICS = 0,
    TIMBRE,
    MORPH,
    OSC_MIX,
    DETUNE,
    SUB_LEVEL,
    SUB_ANCHOR,
    
    // Filter parameters
    FILTER_CUTOFF,
    FILTER_RESONANCE,
    FILTER_TYPE,
    
    // Envelope parameters
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE,
    
    // Modulation parameters
    LFO_RATE,
    LFO_DEPTH,
    LFO_SHAPE,
    
    // Effects parameters
    REVERB_SIZE,
    REVERB_DAMPING,
    REVERB_MIX,
    DELAY_TIME,
    DELAY_FEEDBACK,
    
    // Mix parameters
    VOLUME,
    PAN,
    
    COUNT
};

// Synthesis engine types - matches EngineFactory::EngineType
enum class EngineType : uint8_t {
    MACRO_VA = 0,
    MACRO_FM,
    MACRO_WAVESHAPER,
    MACRO_WAVETABLE,
    MACRO_CHORD,
    MACRO_HARMONICS,
    FORMANT_VOCAL,
    NOISE_PARTICLES,
    TIDES_OSC,
    RINGS_VOICE,
    ELEMENTS_VOICE,
    DRUM_KIT,
    SAMPLER_KIT,
    SAMPLER_SLICER,
    COUNT
};

// Operating modes
enum class Mode : uint8_t {
    INSTRUMENT = 0,  // Deep synthesis control
    SEQUENCER,       // Timeline and pattern editing
    CHORD,           // Chord assignment and distribution
    TAPE,            // 4-track recording
    MODULATION,      // Global modulation setup
    PUNCH_FX,        // Performance effects
    PROJECT,         // Song settings and file management
    COUNT
};

// Audio processing types
struct AudioFrame {
    float left;
    float right;
    
    AudioFrame() : left(0.0f), right(0.0f) {}
    AudioFrame(float mono) : left(mono), right(mono) {}
    AudioFrame(float l, float r) : left(l), right(r) {}
    
    AudioFrame& operator+=(const AudioFrame& other) {
        left += other.left;
        right += other.right;
        return *this;
    }
    
    AudioFrame operator*(float gain) const {
        return AudioFrame(left * gain, right * gain);
    }
};

using EtherAudioBuffer = std::array<AudioFrame, BUFFER_SIZE>;

// Hardware interface types
struct KeyState {
    bool pressed;
    float velocity;      // 0.0 - 1.0
    float aftertouch;    // 0.0 - 1.0 (polyphonic aftertouch)
    uint32_t pressTime;  // For note timing
};

struct EncoderState {
    float value;         // 0.0 - 1.0
    bool changed;
    uint32_t lastUpdate;
};

// UI state types
struct TouchPoint {
    float x, y;          // Normalized coordinates 0.0 - 1.0
    bool active;
    uint32_t id;
};

// Color utilities
inline uint8_t getRed(uint32_t color) { return (color >> 16) & 0xFF; }
inline uint8_t getGreen(uint32_t color) { return (color >> 8) & 0xFF; }
inline uint8_t getBlue(uint32_t color) { return color & 0xFF; }

inline uint32_t makeColor(uint8_t r, uint8_t g, uint8_t b) {
    return (r << 16) | (g << 8) | b;
}

// Utility functions
template<typename T>
constexpr T clamp(T value, T min, T max) {
    return value < min ? min : (value > max ? max : value);
}

inline float noteToFrequency(int noteNumber) {
    return 440.0f * std::pow(2.0f, (noteNumber - 69) / 12.0f);
}

inline int frequencyToNote(float frequency) {
    return static_cast<int>(69 + 12 * std::log2(frequency / 440.0f));
}