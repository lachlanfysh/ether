#pragma once
#include <cstdint>

/**
 * SequencerStep - Enhanced step data with per-note slide timing and accent flags
 * 
 * Extends basic step sequencing with advanced per-step parameters for:
 * - Exponential legato slide timing (5-120ms per note)
 * - Per-step accent triggers (+4-8dB VCA, +10-25% cutoff, +Q)
 * - Per-step velocity values (0-127) for latchable velocity modulation
 * - Step enable/disable and tie functionality
 * 
 * Features:
 * - Compact 64-bit step representation for efficient storage
 * - Direct integration with Slide+Accent Bass engine
 * - Support for velocity modulation depth and polarity
 * - Real-time safe parameter access
 */
class SequencerStep {
public:
    // Step flags (bit-packed for efficiency)
    enum class StepFlags : uint16_t {
        ENABLED = 0x0001,           // Step is active
        ACCENT = 0x0002,            // Accent trigger
        SLIDE = 0x0004,             // Slide/legato enabled
        TIE = 0x0008,               // Tie to next step (no retrigger)
        VELOCITY_LATCH = 0x0010,    // Velocity modulation latched
        MUTE = 0x0020,              // Step is muted
        SKIP = 0x0040,              // Skip this step in playback
        RANDOMIZE = 0x0080,         // Apply random variation
        
        // Reserved for future use
        RESERVED_8 = 0x0100,
        RESERVED_9 = 0x0200,
        RESERVED_10 = 0x0400,
        RESERVED_11 = 0x0800,
        RESERVED_12 = 0x1000,
        RESERVED_13 = 0x2000,
        RESERVED_14 = 0x4000,
        RESERVED_15 = 0x8000
    };
    
    struct StepData {
        uint8_t note;               // MIDI note number (0-127)
        uint8_t velocity;           // MIDI velocity (0-127)
        uint8_t slideTimeMs;        // Slide time in ms (5-120ms range)
        uint8_t accentAmount;       // Accent amount (0-127, maps to +0-8dB)
        uint16_t flags;             // Step flags (StepFlags enum)
        uint8_t probability;        // Step probability (0-127, 127=100%)
        uint8_t microTiming;        // Micro-timing offset (-64 to +63)
        
        // Default constructor
        StepData() : 
            note(60),           // C4
            velocity(100),      // Default velocity
            slideTimeMs(20),    // Default slide time
            accentAmount(0),    // No accent by default
            flags(0),           // Not enabled by default (empty step)
            probability(127),   // 100% probability
            microTiming(64) {}  // No offset (64 = center)
    };
    
    SequencerStep();
    SequencerStep(const StepData& data);
    SequencerStep(uint8_t note, uint8_t velocity = 100);
    ~SequencerStep() = default;
    
    // Basic step properties
    void setNote(uint8_t note);
    void setVelocity(uint8_t velocity);
    uint8_t getNote() const { return data_.note; }
    uint8_t getVelocity() const { return data_.velocity; }
    
    // Slide and accent control
    void setSlideTime(uint8_t slideTimeMs);         // 5-120ms range
    void setAccentAmount(uint8_t accentAmount);     // 0-127 range
    uint8_t getSlideTime() const { return data_.slideTimeMs; }
    uint8_t getAccentAmount() const { return data_.accentAmount; }
    
    // Slide time conversion utilities
    float getSlideTimeSeconds() const;              // Convert to seconds for audio processing
    void setSlideTimeSeconds(float timeSeconds);    // Set from seconds (clamped to range)
    
    // Accent amount conversion utilities  
    float getAccentGainDB() const;                  // Convert to dB boost (0-8dB)
    float getAccentCutoffBoost() const;             // Convert to cutoff boost (0-25%)
    void setAccentGainDB(float gainDB);             // Set from dB value
    
    // Step flags
    void setFlag(StepFlags flag, bool enabled = true);
    void clearFlag(StepFlags flag);
    void toggleFlag(StepFlags flag);
    bool hasFlag(StepFlags flag) const;
    void clearAllFlags();
    
    // Convenience flag methods
    void setEnabled(bool enabled = true);
    void setAccent(bool accent = true);
    void setSlide(bool slide = true);
    void setTie(bool tie = true);
    void setVelocityLatch(bool latch = true);
    void setMute(bool mute = true);
    
    bool isEnabled() const;
    bool isAccent() const;
    bool isSlide() const;
    bool isTie() const;
    bool isVelocityLatch() const;
    bool isMute() const;
    
    // Advanced parameters
    void setProbability(uint8_t probability);       // 0-127 (0-100%)
    void setMicroTiming(int8_t offset);             // -64 to +63 samples
    uint8_t getProbability() const { return data_.probability; }
    int8_t getMicroTiming() const { return static_cast<int8_t>(data_.microTiming - 64); }
    
    // Step data access
    const StepData& getData() const { return data_; }
    void setData(const StepData& data);
    
    // Serialization support
    uint64_t serialize() const;                     // Pack to 64-bit value
    void deserialize(uint64_t packed);              // Unpack from 64-bit value
    
    // Utility methods
    void reset();                                   // Reset to default values
    void copyFrom(const SequencerStep& other);
    bool isActive() const;                          // Enabled and not muted
    
    // Comparison operators
    bool operator==(const SequencerStep& other) const;
    bool operator!=(const SequencerStep& other) const;
    
    // Constants
    static constexpr uint8_t MIN_SLIDE_TIME_MS = 5;
    static constexpr uint8_t MAX_SLIDE_TIME_MS = 120;
    static constexpr uint8_t DEFAULT_SLIDE_TIME_MS = 20;
    static constexpr uint8_t MAX_ACCENT_AMOUNT = 127;
    static constexpr float MAX_ACCENT_GAIN_DB = 8.0f;
    static constexpr float MAX_ACCENT_CUTOFF_BOOST = 0.25f; // 25%
    
private:
    StepData data_;
    
    // Internal utility functions
    uint8_t clampSlideTime(uint8_t slideTimeMs) const;
    uint8_t clampVelocity(uint8_t velocity) const;
    uint8_t clampNote(uint8_t note) const;
    
    // Bitwise operations for flags
    void setFlagBit(uint16_t mask, bool value);
    bool getFlagBit(uint16_t mask) const;
};