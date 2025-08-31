#pragma once
#include "../synthesis/EngineFactory.h"
#include "../core/Types.h"
#include <array>
#include <vector>
#include <memory>
#include <string>
#include <map>

/**
 * Enhanced Multi-Engine Chord System for EtherSynth V1.0
 * 
 * Bicep-Style Production: Multi-instrument chord distribution
 * - Up to 5 chord voices, each with independent synthesis engines
 * - Professional voice leading and harmonic arrangement
 * - Real-time chord type morphing and engine swapping
 * - Integration with 8-track groovebox workflow
 * 
 * This is the flagship chord feature - no other synth offers
 * this level of per-voice engine flexibility!
 */

namespace EtherSynth {

// Forward declarations
class IEngine;
class EngineFactory;

class EnhancedChordSystem {
public:
    static constexpr int MAX_CHORD_VOICES = 5;
    static constexpr int MAX_INSTRUMENTS = 8;
    
    // Enhanced chord types (32 total)
    enum class ChordType : uint8_t {
        // Major family
        MAJOR = 0, MAJOR_6, MAJOR_7, MAJOR_9, MAJOR_ADD9,
        MAJOR_11, MAJOR_13, MAJOR_6_9,
        
        // Minor family  
        MINOR, MINOR_6, MINOR_7, MINOR_9, MINOR_ADD9,
        MINOR_11, MINOR_13, MINOR_MAJ7,
        
        // Dominant family
        DOMINANT_7, DOMINANT_9, DOMINANT_11, DOMINANT_13,
        DOMINANT_7_SHARP_5, DOMINANT_7_FLAT_5,
        
        // Diminished family
        DIMINISHED, DIMINISHED_7, HALF_DIMINISHED_7,
        
        // Augmented family
        AUGMENTED, AUGMENTED_7, AUGMENTED_MAJ7,
        
        // Sus family
        SUS_2, SUS_4, SEVEN_SUS_4,
        
        // Extended/Special
        COUNT
    };
    
    // Voice role assignment for professional arrangements
    enum class VoiceRole : uint8_t {
        ROOT = 0,      // Foundation - typically bass engines
        HARMONY_1,     // 3rd/5th - mid-range engines  
        HARMONY_2,     // Extensions - lead engines
        COLOR,         // 9th/11th/13th - texture engines
        DOUBLING       // Octave doubling - any engine
    };
    
    // Chord voice configuration
    struct ChordVoice {
        EngineType engineType = EngineType::MACRO_VA;
        VoiceRole role = VoiceRole::ROOT;
        float noteOffset = 0.0f;        // Semitones from root
        float level = 1.0f;             // Mix level (0-1)
        float pan = 0.0f;               // Stereo position (-1 to 1)
        float detune = 0.0f;            // Fine detune in cents
        bool active = true;             // Voice enabled
        
        // Engine instance (lazy instantiation)
        std::unique_ptr<IEngine> engineInstance = nullptr;
        bool engineNeedsUpdate = true;
        
        ChordVoice() = default;
        ChordVoice(EngineType engine, VoiceRole r, float offset) 
            : engineType(engine), role(r), noteOffset(offset) {}
    };
    
    // Multi-instrument chord assignment
    struct InstrumentChordAssignment {
        int instrumentIndex = 0;        // Target instrument (0-7)
        std::vector<int> voiceIndices;  // Which chord voices this instrument plays
        float strumOffset = 0.0f;       // Strum timing offset (ms)
        float velocityScale = 1.0f;     // Velocity scaling for this instrument
        bool arpeggiate = false;        // Enable arpeggiator for this instrument
        float arpeggioRate = 8.0f;      // Arpeggio rate (16th notes = 16.0)
        
        InstrumentChordAssignment() = default;
        InstrumentChordAssignment(int inst) : instrumentIndex(inst) {}
    };
    
    // Complete chord configuration
    struct ChordConfiguration {
        ChordType chordType = ChordType::MAJOR;
        std::array<ChordVoice, MAX_CHORD_VOICES> voices;
        std::vector<InstrumentChordAssignment> instrumentAssignments;
        
        // Global chord parameters
        float spread = 12.0f;           // Voice spread in semitones (0-24)
        float strumTime = 0.0f;         // Global strum time (0-100ms)
        bool strumUp = true;            // Strum direction
        float humanization = 0.0f;      // Timing humanization (0-1)
        
        // Voice leading
        bool enableVoiceLeading = true; // Smooth voice movement
        float voiceLeadingStrength = 0.8f; // How much to prioritize smooth movement
        
        std::string name = "Untitled Chord";
        
        ChordConfiguration() {
            initializeDefaultVoicing();
        }
        
        void initializeDefaultVoicing();
    };
    
    EnhancedChordSystem();
    ~EnhancedChordSystem();
    
    // Core chord management
    void setCurrentChord(ChordType chordType, uint8_t rootNote);
    void setChordConfiguration(const ChordConfiguration& config);
    const ChordConfiguration& getChordConfiguration() const { return currentConfig_; }
    
    // Voice management
    void setVoiceEngine(int voiceIndex, EngineType engineType);
    void setVoiceRole(int voiceIndex, VoiceRole role);
    void setVoiceLevel(int voiceIndex, float level);
    void setVoicePan(int voiceIndex, float pan);
    void enableVoice(int voiceIndex, bool enabled);
    
    // Multi-instrument assignment
    void assignInstrument(int instrumentIndex, const std::vector<int>& voiceIndices);
    void setInstrumentStrumOffset(int instrumentIndex, float offsetMs);
    void setInstrumentArpeggio(int instrumentIndex, bool enabled, float rate = 8.0f);
    void clearInstrumentAssignments();
    
    // Chord playback control
    void playChord(uint8_t rootNote, float velocity);
    void releaseChord();
    void arpeggiate(uint8_t rootNote, float velocity, float rate = 8.0f);
    
    // Professional arrangement presets
    void loadArrangementPreset(const std::string& presetName);
    void saveArrangementPreset(const std::string& presetName);
    std::vector<std::string> getArrangementPresets() const;
    
    // Voice leading system
    void calculateVoiceLeading(ChordType fromChord, ChordType toChord, uint8_t fromRoot, uint8_t toRoot);
    void applyVoiceLeading(bool enabled) { enableVoiceLeading_ = enabled; }
    
    // Analysis and visualization
    std::vector<std::string> getChordToneNames(ChordType chordType, uint8_t rootNote) const;
    std::vector<float> getChordToneFrequencies(ChordType chordType, uint8_t rootNote) const;
    std::string getChordSymbol(ChordType chordType, uint8_t rootNote) const;
    
    // Hardware integration
    void processHardwareChordInput(int keyIndex, bool pressed);
    void visualizeChordAssignment(uint32_t* displayBuffer, int width, int height) const;
    
    // Performance monitoring
    float getCPUUsage() const;
    int getActiveVoiceCount() const;
    
private:
    // Chord interval definitions
    struct ChordIntervals {
        std::vector<int> intervals;     // Semitone intervals from root
        std::string symbol;             // Chord symbol (e.g., "maj7")
        std::string fullName;           // Full name (e.g., "Major 7th")
    };
    
    // Static chord database
    static const std::map<ChordType, ChordIntervals> chordDatabase_;
    static void initializeChordDatabase();
    
    // Voice leading engine
    class VoiceLeadingEngine {
    public:
        struct VoiceMovement {
            int voiceIndex;
            float fromNote;
            float toNote;
            float distance;
        };
        
        std::vector<VoiceMovement> calculateOptimalVoicing(
            const std::vector<float>& fromNotes,
            const std::vector<float>& toNotes,
            float maxMovement = 7.0f  // Prefer movements ≤ 7 semitones
        );
        
        void applyInversion(std::vector<float>& notes, int inversion);
        float calculateVoicingSpread(const std::vector<float>& notes);
    };
    
    // Arrangement engine for professional distributions
    class ArrangementEngine {
    public:
        enum class ArrangementStyle {
            BICEP_STYLE,    // Multi-instrument chord distribution
            JAZZ_VOICING,   // Close harmony with extensions
            ORCHESTRAL,     // Wide spread with doubling
            MODERN_POP,     // Root + color tones
            AMBIENT_PAD     // Wide, washy textures
        };
        
        void generateArrangement(
            ChordConfiguration& config,
            ArrangementStyle style,
            const std::vector<EngineType>& availableEngines
        );
        
        void optimizeForGenre(ChordConfiguration& config, const std::string& genre);
    };
    
    // Engine factory for voice instantiation
    std::unique_ptr<EngineFactory> engineFactory_;
    VoiceLeadingEngine voiceLeadingEngine_;
    ArrangementEngine arrangementEngine_;
    
    // Current state
    ChordConfiguration currentConfig_;
    ChordType currentChordType_ = ChordType::MAJOR;
    uint8_t currentRootNote_ = 60;  // Middle C
    bool enableVoiceLeading_ = true;
    
    // Playback state
    std::vector<uint8_t> activeNotes_;
    uint64_t lastChordTime_ = 0;
    
    // Preset storage
    std::map<std::string, ChordConfiguration> arrangementPresets_;
    
    // Performance monitoring
    float cpuUsage_ = 0.0f;
    int activeVoiceCount_ = 0;
    
    // Helper functions
    std::vector<float> generateChordNotes(ChordType chordType, uint8_t rootNote, float spread) const;
    void updateEngineInstances();
    void distributeToInstruments(uint8_t rootNote, float velocity);
    void initializeDefaultPresets();
    
    // Note conversion utilities
    std::string noteNumberToName(uint8_t noteNumber) const;
    float noteNumberToFrequency(uint8_t noteNumber) const;
    uint8_t constrainToOctave(int noteNumber) const;
};

// Preset arrangement configurations
namespace ChordArrangementPresets {
    // Bicep-style arrangements
    EnhancedChordSystem::ChordConfiguration bicepHouse();      // House/Techno chord stabs
    EnhancedChordSystem::ChordConfiguration bicepAmbient();    // Lush ambient pads
    EnhancedChordSystem::ChordConfiguration bicepJazz();       // Jazz extended harmony
    
    // Genre-specific arrangements
    EnhancedChordSystem::ChordConfiguration modernPop();      // Contemporary pop chords
    EnhancedChordSystem::ChordConfiguration vintageKeys();    // Classic electric piano
    EnhancedChordSystem::ChordConfiguration orchestralPad();  // Cinematic orchestration
    EnhancedChordSystem::ChordConfiguration trapChords();     // Hip-hop/trap progressions
}

// Chord utility functions
namespace ChordUtils {
    // Music theory helpers
    std::vector<std::string> getScaleDegreeNames();
    std::vector<int> getCommonProgressions();
    std::string analyzeHarmonicFunction(EnhancedChordSystem::ChordType chord, int key);
    
    // Voice leading analysis
    float calculateVoiceLeadingEfficiency(
        const std::vector<float>& chord1,
        const std::vector<float>& chord2
    );
    
    // MIDI integration
    std::vector<uint8_t> chordToMIDINotes(
        EnhancedChordSystem::ChordType chordType,
        uint8_t rootNote,
        int octave = 4
    );
}

} // namespace EtherSynth