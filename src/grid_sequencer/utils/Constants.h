#pragma once

// Grid Sequencer Constants
constexpr int MAX_ENGINES = 17;  // 13 real + 4 fallback = 17 total engines
constexpr int GRID_WIDTH = 16;
constexpr int GRID_HEIGHT = 8;

// Pattern system constants
constexpr int PATTERN_STEPS = 16;
constexpr int PATTERN_BANKS = 4;
constexpr int PATTERNS_PER_BANK = 16;
constexpr int TOTAL_PATTERNS = 64;

// Grid layout constants
constexpr int PAD_W = 4;
constexpr int PAD_H = 4;
constexpr int PAD_ORIGIN_X = 0;
constexpr int PAD_ORIGIN_Y = 1;

// OSC and timing constants
constexpr int DEFAULT_GRID_OSC_PORT = 7001;
constexpr int DOUBLE_PRESS_TIMEOUT_MS = 300;

// Parameter ranges
constexpr float PARAM_MIN = 0.0f;
constexpr float PARAM_MAX = 1.0f;
constexpr int OCTAVE_MIN = -4;
constexpr int OCTAVE_MAX = 4;
constexpr float PITCH_MIN = -12.0f;
constexpr float PITCH_MAX = 12.0f;

// Build information
#ifndef BUILD_VERSION
#define BUILD_VERSION "Grid dev"
#endif