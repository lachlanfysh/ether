#pragma once
#include <cstdint>

namespace GridSequencer {
namespace Core {

// Parameter routing enum
enum class ParamRoute : uint8_t {
    Engine,
    PostFX,
    Unsupported
};

// Step data structure
struct StepData {
    bool active = false;
    int note = 60;
    float velocity = 0.6f;
    bool hasAccent = false;      // Accent mode flag
    bool hasRetrigger = false;   // Retrigger mode flag
    bool hasArpeggiator = false; // Arpeggiator mode flag
};

// Arpeggiator pattern types
enum class ArpPattern : int {
    UP = 0,          // C E G C E G...
    DOWN,            // C G E C G E...
    UP_DOWN,         // C E G E C E G E...
    DOWN_UP,         // C G E G C G E G...
    RANDOM,          // Random order
    AS_PLAYED,       // Order notes were played in
    CHORD           // All notes together (chord mode)
};

// Arpeggiator settings
struct ArpeggiatorSettings {
    ArpPattern pattern = ArpPattern::UP;  // Arpeggiator pattern
    int length = 3;                       // Number of notes in arp (1-8)
    int cycles = -1;                      // Number of cycles (-1 = infinite/latch)
    int octaveRange = 1;                  // Octave range (1-4)
    int speed = 4;                        // Speed multiplier (1=1/16, 2=1/8, 4=1/4, 8=1/2, 16=whole)
    int gateLength = 75;                  // Gate length percentage (25-100%)
    float velocityScale = 1.0f;           // Velocity scaling (0.1-2.0)
    float swing = 0.0f;                   // Swing amount (0.0-1.0)
};

// Retrigger timing types
enum class RetriggerTiming : int {
    ACCELERATING = 0,    // Getting faster (1/8, 1/16, 1/32...)
    DECELERATING,        // Getting slower (1/32, 1/16, 1/8...)
    CONSTANT,            // Same timing interval
    EXPONENTIAL,         // Exponential curve
    LOGARITHMIC          // Logarithmic curve
};

// Retrigger velocity curves
enum class RetriggerVelocity : int {
    CONSTANT = 0,        // Same velocity for all triggers
    DECAYING,            // Each trigger gets quieter
    BUILDING,            // Each trigger gets louder
    RANDOM               // Random velocity variation
};

// Retrigger settings
struct RetriggerSettings {
    int numTriggers = 4;                          // Number of retriggered hits (2-16)
    float timeWindow = 0.25f;                     // Time window for retriggers (0.125-1.0)
    float octaveStep = 1.0f;                      // Octave step for each retrigger (0.0-2.0)
    RetriggerTiming timing = RetriggerTiming::ACCELERATING;  // Timing pattern
    RetriggerVelocity velocityPattern = RetriggerVelocity::DECAYING;  // Velocity pattern
    float intensityCurve = 0.5f;                  // Curve intensity for exponential/logarithmic (0.0-1.0)
};

// Encoder button state for timing
struct EncoderButtonState {
    std::chrono::steady_clock::time_point lastPressTime;
    bool pendingSinglePress = false;
};

// Parameter latch information for encoders
struct ParameterLatch {
    bool active = false;
    ParameterID paramId;
    int engineRow = -1;
    std::string paramName;
};

} // namespace Core
} // namespace GridSequencer