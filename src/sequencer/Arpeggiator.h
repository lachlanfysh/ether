#pragma once
#include "../core/Types.h"
#include "../audio/SIMDOptimizations.h"
#include <vector>
#include <array>
#include <cstdint>

namespace EtherSynth {

/**
 * Professional Arpeggiator for EtherSynth V1.0
 * 
 * Features:
 * - 8 arpeggio patterns (Up, Down, Up/Down, Random, etc.)
 * - Adjustable note range (1-4 octaves)
 * - Swing/groove support
 * - Note length control
 * - Gate length adjustment
 * - Real-time pattern switching
 * - MIDI sync and internal clock
 * - Hold mode for sustained arpeggios
 */

class Arpeggiator {
public:
    // Arpeggio patterns
    enum class Pattern : uint8_t {
        UP = 0,           // C E G C E G...
        DOWN,             // C G E C G E...
        UP_DOWN,          // C E G E C E G E...
        DOWN_UP,          // C G E G C G E G...
        UP_DOWN_INCLUSIVE, // C E G C G E C E G C...
        RANDOM,           // Random order
        PLAYED_ORDER,     // Order notes were played
        CHORD,            // All notes together
        COUNT
    };
    
    // Time divisions
    enum class Division : uint8_t {
        WHOLE = 0,        // 1/1
        HALF,             // 1/2
        QUARTER,          // 1/4
        EIGHTH,           // 1/8
        SIXTEENTH,        // 1/16
        THIRTY_SECOND,    // 1/32
        EIGHTH_TRIPLET,   // 1/8T
        SIXTEENTH_TRIPLET,// 1/16T
        COUNT
    };
    
    struct ArpNote {
        uint8_t midiNote;
        float velocity;
        uint32_t startTime;  // When note was pressed
        bool held;           // Is note being held
        
        ArpNote() : midiNote(60), velocity(0.8f), startTime(0), held(false) {}
        ArpNote(uint8_t note, float vel, uint32_t time) 
            : midiNote(note), velocity(vel), startTime(time), held(true) {}
    };
    
    struct ArpSettings {
        Pattern pattern = Pattern::UP;
        Division division = Division::SIXTEENTH;
        uint8_t octaves = 1;          // 1-4 octaves
        uint8_t noteLength = 80;      // 1-100% of step length
        uint8_t swing = 50;           // 0-100% swing
        float velocity = 100.0f;      // 0-200% velocity scaling
        bool hold = false;            // Hold mode
        bool latch = false;           // Latch mode (toggle on/off)
        int8_t transpose = 0;         // -24 to +24 semitones
        uint8_t gateLength = 80;      // 1-100% gate length
        
        // Advanced features
        bool retrigger = true;        // Retrigger on new notes
        uint8_t stepSkip = 0;         // Skip every N steps
        int8_t accentPattern = 0;     // Accent pattern (0=off, 1-8=patterns)
    };
    
    Arpeggiator();
    ~Arpeggiator() = default;
    
    // Configuration
    void setSettings(const ArpSettings& settings);
    const ArpSettings& getSettings() const { return settings_; }
    void setPattern(Pattern pattern);
    void setDivision(Division division);
    void setOctaves(uint8_t octaves);
    
    // Clock and timing
    void setSampleRate(float sampleRate);
    void setTempo(float bpm);
    void syncToExternalClock(bool sync);
    void reset();
    
    // Note input
    void noteOn(uint8_t midiNote, float velocity);
    void noteOff(uint8_t midiNote);
    void allNotesOff();
    void panicStop(); // Immediate stop
    
    // Processing
    struct ArpOutput {
        bool noteOn;
        bool noteOff;
        uint8_t midiNote;
        float velocity;
        uint32_t noteId;
    };
    
    ArpOutput process(); // Call once per audio sample
    bool isActive() const { return !heldNotes_.empty() || (settings_.hold && !arpNotes_.empty()); }
    
    // Pattern generation
    void generatePattern();
    void shufflePattern(); // For random mode
    
    // State queries
    size_t getHeldNoteCount() const { return heldNotes_.size(); }
    size_t getArpNoteCount() const { return arpNotes_.size(); }
    int getCurrentStep() const { return currentStep_; }
    Pattern getCurrentPattern() const { return settings_.pattern; }
    float getCurrentTempo() const { return tempo_; }
    
    // MIDI learn and automation
    void setMIDILearnMode(bool enabled) { midiLearnMode_ = enabled; }
    void assignMIDICC(uint8_t parameter, uint8_t ccNumber);
    void processMIDICC(uint8_t ccNumber, uint8_t value);
    
private:
    ArpSettings settings_;
    
    // Timing and clock
    float sampleRate_ = 48000.0f;
    float tempo_ = 120.0f;
    bool externalSync_ = false;
    
    // Clock division calculations
    float samplesPerBeat_;
    float samplesPerStep_;
    uint32_t sampleCounter_;
    uint32_t nextStepTime_;
    
    // Note management
    std::vector<ArpNote> heldNotes_;    // Currently held notes
    std::vector<ArpNote> arpNotes_;     // Generated arpeggio pattern
    int currentStep_;
    int patternLength_;
    
    // State management
    bool isRunning_;
    bool latchActive_;
    uint32_t currentNoteId_;
    uint32_t lastNoteOnTime_;
    uint32_t lastNoteOffTime_;
    
    // Swing and groove
    float swingOffset_;
    bool isSwingStep_;
    
    // MIDI automation
    bool midiLearnMode_ = false;
    std::array<uint8_t, 16> ccMappings_; // Parameter to CC mapping
    
    // Random number generation for random pattern
    uint32_t randomSeed_;
    
    // Internal methods
    void updateClockDivision();
    void updateSwingTiming();
    bool shouldTriggerStep() const;
    void addNoteToHeld(uint8_t midiNote, float velocity);
    void removeNoteFromHeld(uint8_t midiNote);
    ArpNote* findHeldNote(uint8_t midiNote);
    
    // Pattern generation methods
    void generateUpPattern();
    void generateDownPattern();
    void generateUpDownPattern();
    void generateDownUpPattern();
    void generateUpDownInclusivePattern();
    void generateRandomPattern();
    void generatePlayedOrderPattern();
    void generateChordPattern();
    
    // Utility methods
    uint32_t fastRandom(); // Fast random number generator
    float getDivisionMultiplier(Division division) const;
    bool isAccentStep(int step) const;
    float getVelocityForStep(int step, float baseVelocity) const;
    uint8_t transposeNote(uint8_t note, int8_t semitones) const;
    
    // Performance optimization
    inline uint32_t getCurrentTime() const {
        return sampleCounter_;
    }
    
    inline bool isStepTime() const {
        return sampleCounter_ >= nextStepTime_;
    }
};

// Factory function for easy creation
inline std::unique_ptr<Arpeggiator> createArpeggiator() {
    return std::make_unique<Arpeggiator>();
}

// Pattern names for UI display
inline const char* getPatternName(Arpeggiator::Pattern pattern) {
    switch (pattern) {
        case Arpeggiator::Pattern::UP: return "Up";
        case Arpeggiator::Pattern::DOWN: return "Down";
        case Arpeggiator::Pattern::UP_DOWN: return "Up/Down";
        case Arpeggiator::Pattern::DOWN_UP: return "Down/Up";
        case Arpeggiator::Pattern::UP_DOWN_INCLUSIVE: return "Up/Down Inc";
        case Arpeggiator::Pattern::RANDOM: return "Random";
        case Arpeggiator::Pattern::PLAYED_ORDER: return "As Played";
        case Arpeggiator::Pattern::CHORD: return "Chord";
        default: return "Unknown";
    }
}

// Division names for UI display
inline const char* getDivisionName(Arpeggiator::Division division) {
    switch (division) {
        case Arpeggiator::Division::WHOLE: return "1/1";
        case Arpeggiator::Division::HALF: return "1/2";
        case Arpeggiator::Division::QUARTER: return "1/4";
        case Arpeggiator::Division::EIGHTH: return "1/8";
        case Arpeggiator::Division::SIXTEENTH: return "1/16";
        case Arpeggiator::Division::THIRTY_SECOND: return "1/32";
        case Arpeggiator::Division::EIGHTH_TRIPLET: return "1/8T";
        case Arpeggiator::Division::SIXTEENTH_TRIPLET: return "1/16T";
        default: return "1/16";
    }
}

} // namespace EtherSynth