#pragma once
#include "../core/Types.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <random>
#include <array>

/**
 * GenerativeSequencer - AI-powered intelligent music composition engine
 * 
 * Professional groovebox AI system for EtherSynth V1.0:
 * - Real-time pattern generation using music theory algorithms
 * - Adaptive sequencing that learns from user performance
 * - Intelligent harmony and melody generation
 * - Style-aware rhythm generation with genre templates
 * - Performance-responsive adaptive composition
 * - Hardware-optimized for 960×320 + 2×16 key interface
 */

namespace EtherSynth {

class GenerativeSequencer {
public:
    enum class GenerationMode : uint8_t {
        ASSIST = 0,           // AI assists user input
        GENERATE,             // Full AI generation
        EVOLVE,               // Evolve existing patterns
        RESPOND,              // Respond to user performance
        HARMONIZE,            // Generate harmonies
        RHYTHMIZE,            // Generate rhythmic variations
        COUNT
    };
    
    enum class MusicalStyle : uint8_t {
        ELECTRONIC = 0,       // Electronic/EDM styles
        TECHNO,               // Techno and minimal
        HOUSE,                // House and deep house
        AMBIENT,              // Ambient and experimental
        DRUM_AND_BASS,        // D&B and breakbeat
        ACID,                 // Acid house/techno
        INDUSTRIAL,           // Industrial and harsh
        MELODIC,              // Melodic electronic
        EXPERIMENTAL,         // Experimental and avant-garde
        CUSTOM,               // User-defined style
        COUNT
    };
    
    enum class GenerationComplexity : uint8_t {
        SIMPLE = 0,           // Simple, sparse patterns
        MODERATE,             // Moderate complexity
        COMPLEX,              // Complex, dense patterns
        ADAPTIVE,             // Adapts to context
        COUNT
    };
    
    // Musical analysis structures
    struct ScaleAnalysis {
        int rootNote = 0;             // Root note (0-11, C=0)
        int scaleType = 0;            // Major, minor, dorian, etc.
        std::array<bool, 12> notes;   // Active notes in scale
        float confidence = 0.0f;      // Analysis confidence (0-1)
        
        ScaleAnalysis() { notes.fill(false); }
    };
    
    struct RhythmicPattern {
        std::vector<bool> kicks;      // Kick drum pattern
        std::vector<bool> snares;     // Snare pattern
        std::vector<bool> hihats;     // Hi-hat pattern
        std::vector<float> velocities; // Velocity pattern
        int subdivision = 16;         // Pattern subdivision
        float swing = 0.0f;           // Swing amount
        float humanization = 0.0f;    // Timing humanization
    };
    
    struct MelodicPhrase {
        std::vector<int> notes;       // Note sequence
        std::vector<float> durations; // Note durations
        std::vector<float> velocities; // Note velocities
        int octave = 4;               // Base octave
        float tension = 0.0f;         // Harmonic tension level
    };
    
    // AI generation parameters
    struct GenerationParams {
        GenerationMode mode = GenerationMode::ASSIST;
        MusicalStyle style = MusicalStyle::ELECTRONIC;
        GenerationComplexity complexity = GenerationComplexity::MODERATE;
        
        // Musical parameters
        float density = 0.5f;         // Note density (0-1)
        float tension = 0.5f;         // Harmonic tension (0-1)
        float rhythmicVariation = 0.5f; // Rhythm variation (0-1)
        float melodicRange = 0.5f;    // Melodic range (0-1)
        
        // Style parameters
        float quantization = 1.0f;    // Timing quantization (0-1)
        float swing = 0.0f;           // Swing amount (-1 to 1)
        float humanization = 0.1f;    // Timing humanization (0-1)
        
        // AI behavior
        float creativity = 0.5f;      // Creativity vs. predictability
        float responsiveness = 0.7f;  // How much to respond to user
        float evolution = 0.3f;       // Pattern evolution rate
        
        // Hardware constraints
        bool respectKeyLayout = true;  // Consider 2×16 key layout
        bool realTimeOptimized = true; // Optimize for real-time use
        int maxVoices = 8;            // Maximum simultaneous voices
    };
    
    // Performance analysis for adaptive generation
    struct PerformanceAnalysis {
        float averageVelocity = 0.7f;
        float rhythmicConsistency = 0.8f;
        float melodicComplexity = 0.5f;
        float harmonicComplexity = 0.5f;
        float playbackTempo = 120.0f;
        
        // User preferences (learned over time)
        std::map<int, float> preferredNotes;
        std::map<int, float> preferredRhythms;
        float preferredDensity = 0.5f;
        
        // Current session analysis
        int notesPlayed = 0;
        int patternsCreated = 0;
        float sessionEnergy = 0.5f;
    };
    
    GenerativeSequencer();
    ~GenerativeSequencer() = default;
    
    // Core generation functions
    uint32_t generatePattern(const GenerationParams& params, int trackIndex);
    void evolvePattern(uint32_t patternId, float evolutionAmount);
    uint32_t generateHarmony(uint32_t sourcePatternId, const GenerationParams& params);
    uint32_t generateRhythmVariation(uint32_t sourcePatternId, float variationAmount);
    
    // Real-time adaptive generation
    void analyzeUserPerformance(const std::vector<NoteEvent>& events);
    void updateAdaptiveModel(const PerformanceAnalysis& analysis);
    uint32_t generateAdaptiveResponse(float deltaTime);
    
    // Musical analysis
    ScaleAnalysis analyzeScale(const std::vector<NoteEvent>& events);
    RhythmicPattern analyzeRhythm(const std::vector<NoteEvent>& events);
    MelodicPhrase extractMelody(const std::vector<NoteEvent>& events);
    
    // Style-based generation
    void setStyleTemplate(MusicalStyle style);
    void loadCustomStyle(const std::string& styleName);
    RhythmicPattern generateStyleRhythm(MusicalStyle style, int bars = 4);
    MelodicPhrase generateStyleMelody(MusicalStyle style, const ScaleAnalysis& scale);
    
    // Interactive generation (hardware integration)
    void processGenerativeKey(int keyIndex, bool pressed, float velocity);
    void processGenerativeKnob(float value, int paramIndex);
    std::vector<uint32_t> getGenerativeSuggestions(int count = 4);
    
    // Pattern intelligence
    float calculatePatternInterest(uint32_t patternId);
    float calculatePatternComplexity(uint32_t patternId);
    void seedRandomFromPattern(uint32_t patternId);
    
    // Generation control
    void setGenerationMode(GenerationMode mode) { currentMode_ = mode; }
    GenerationMode getGenerationMode() const { return currentMode_; }
    void setGenerationParams(const GenerationParams& params) { params_ = params; }
    const GenerationParams& getGenerationParams() const { return params_; }
    
    // Learning system
    void learnFromUserInput(const std::vector<NoteEvent>& events);
    void saveUserPreferences();
    void loadUserPreferences();
    void resetLearningModel();
    
    // Real-time performance
    void processRealtimeGeneration(float deltaTime);
    bool shouldGenerateNext() const;
    void triggerGenerativeEvent(int eventType);
    
    // Quantization and timing
    std::vector<NoteEvent> quantizeEvents(const std::vector<NoteEvent>& events, float strength = 1.0f);
    std::vector<NoteEvent> addSwing(const std::vector<NoteEvent>& events, float swingAmount);
    std::vector<NoteEvent> humanizeEvents(const std::vector<NoteEvent>& events, float amount);
    
    // Hardware optimization
    void optimizeForHardware(uint32_t patternId);
    bool isPatternHardwareFriendly(uint32_t patternId);
    void mapToKeyLayout(uint32_t patternId);
    
private:
    GenerationParams params_;
    GenerationMode currentMode_ = GenerationMode::ASSIST;
    MusicalStyle currentStyle_ = MusicalStyle::ELECTRONIC;
    
    // AI state
    PerformanceAnalysis performanceAnalysis_;
    ScaleAnalysis currentScale_;
    std::map<MusicalStyle, RhythmicPattern> styleTemplates_;
    
    // Generation engine
    std::mt19937 rng_;
    std::uniform_real_distribution<float> dist_;
    
    // Pattern database
    std::map<uint32_t, float> patternComplexityCache_;
    std::map<uint32_t, float> patternInterestCache_;
    
    // Learning model
    struct LearningModel {
        std::map<int, float> notePreferences;      // Note preference weights
        std::map<int, float> velocityPreferences;  // Velocity preference weights
        std::map<int, float> timingPreferences;    // Timing preference weights
        std::map<std::pair<int,int>, float> sequencePreferences; // Note sequence preferences
        
        float adaptationRate = 0.1f;
        float decayRate = 0.99f;
        int sessionCount = 0;
    } learningModel_;
    
    // Real-time generation state
    float generationTimer_ = 0.0f;
    float generationInterval_ = 4.0f;    // Seconds between generation
    bool realtimeMode_ = false;
    
    // Music theory engine
    struct MusicTheory {
        static const std::array<std::array<int, 7>, 12> scales; // Scale patterns
        static const std::array<int, 24> chordProgressions;     // Common progressions
        static const std::map<MusicalStyle, float> styleTension; // Style tension levels
        
        static bool isNoteInScale(int note, const ScaleAnalysis& scale);
        static int getScaleDegree(int note, const ScaleAnalysis& scale);
        static std::vector<int> getChordNotes(int root, int chordType, const ScaleAnalysis& scale);
        static float calculateHarmonicTension(const std::vector<int>& notes);
    } theory_;
    
    // Generation algorithms
    std::vector<NoteEvent> generateMelodicLine(const ScaleAnalysis& scale, int bars);
    std::vector<NoteEvent> generateBassLine(const ScaleAnalysis& scale, const RhythmicPattern& rhythm);
    std::vector<NoteEvent> generatePercussion(const RhythmicPattern& template, float variation);
    std::vector<NoteEvent> generateArpeggio(const ScaleAnalysis& scale, int pattern);
    
    // Pattern evolution
    void mutatePattern(uint32_t patternId, float mutationRate);
    void crossbreedPatterns(uint32_t pattern1, uint32_t pattern2, uint32_t targetPattern);
    void applyEvolutionPressure(uint32_t patternId, const GenerationParams& fitness);
    
    // Style analysis
    void analyzeStyleFromPatterns(const std::vector<uint32_t>& patternIds);
    MusicalStyle detectStyle(const std::vector<NoteEvent>& events);
    void adaptParamsToStyle(MusicalStyle style);
    
    // Utility functions
    uint32_t generateUniquePatternId();
    float calculatePatternSimilarity(uint32_t pattern1, uint32_t pattern2);
    void normalizeVelocities(std::vector<NoteEvent>& events);
    void ensureValidTiming(std::vector<NoteEvent>& events);
};

// Style templates and presets
namespace GenerativePresets {
    GenerativeSequencer::GenerationParams getTechnoParams();
    GenerativeSequencer::GenerationParams getHouseParams();
    GenerativeSequencer::GenerationParams getAmbientParams();
    GenerativeSequencer::GenerationParams getDrumAndBassParams();
    GenerativeSequencer::GenerationParams getAcidParams();
    
    GenerativeSequencer::RhythmicPattern getFourOnFloor();
    GenerativeSequencer::RhythmicPattern getBreakbeat();
    GenerativeSequencer::RhythmicPattern getMinimalTechno();
}

// Hardware integration utilities
namespace GenerativeHardware {
    void mapGenerationToKeys(const GenerativeSequencer::GenerationParams& params, 
                           std::array<bool, 32>& keyStates);
    void processKeyToGeneration(int keyIndex, bool pressed, 
                              GenerativeSequencer::GenerationParams& params);
    void visualizeGeneration(const GenerativeSequencer::GenerationParams& params,
                           uint32_t* displayBuffer, int width, int height);
}

} // namespace EtherSynth