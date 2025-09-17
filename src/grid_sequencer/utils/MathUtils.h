#pragma once
#include <algorithm>
#include <cmath>

namespace GridSequencer {
namespace Utils {

// General clamping utility (using the one from Types.h as template)
template<typename T>
constexpr T clamp(T value, T min, T max) {
    return value < min ? min : (value > max ? max : value);
}

// Specialized 0-1 clamping for parameters
inline float clamp01(float v) {
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

// MIDI note utilities
inline int clampMIDINote(int note) {
    return clamp(note, 0, 127);
}

// Interpolation utilities
inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// Map value from one range to another
inline float mapRange(float value, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
}

// Convert MIDI note to frequency
inline float midiToFreq(int midiNote) {
    return 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
}

} // namespace Utils
} // namespace GridSequencer