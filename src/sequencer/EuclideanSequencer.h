#pragma once
#include "../core/Types.h"
#include <vector>
#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <random>

/**
 * EuclideanSequencer - Euclidean rhythm generation within 16-step patterns
 * 
 * Professional euclidean rhythm system for EtherSynth V1.0:
 * - Euclidean distribution within fixed 16-step patterns
 * - Per-track euclidean parameters (steps=16, pulses, rotation)
 * - Probability-based variation for organic feel
 * - Real-time parameter adjustment during playback
 * - Pattern chaining compatibility (always 16 steps)
 * - Hardware integration for 960×320 + 2×16 key interface
 */

namespace EtherSynth {

class EuclideanSequencer {
public:
    // Euclidean pattern configuration
    struct EuclideanPattern {
        int totalSteps = 16;          // Always 16 for pattern compatibility
        int pulses = 4;               // Number of hits to distribute
        int rotation = 0;             // Rotate pattern (0-15)
        float probability = 1.0f;     // Per-hit probability (0.0-1.0)
        
        // Advanced parameters
        float swing = 0.0f;           // Swing amount (-1.0 to 1.0)
        float humanization = 0.0f;    // Timing humanization (0.0-1.0)
        float velocityVariation = 0.0f; // Velocity randomization (0.0-1.0)
        
        // Pattern state
        std::array<bool, 16> pattern; // Generated euclidean pattern
        std::array<float, 16> velocities; // Per-step velocities
        std::array<float, 16> timingOffsets; // Micro-timing offsets
        
        bool isDirty = true;          // Needs regeneration
        
        EuclideanPattern() {
            pattern.fill(false);
            velocities.fill(0.7f);
            timingOffsets.fill(0.0f);
        }
    };
    
    // Euclidean generation algorithms
    enum class Algorithm : uint8_t {
        BJORKLUND = 0,    // Classic Bjorklund algorithm
        BRESENHAM,        // Bresenham line algorithm
        FRACTIONAL,       // Fractional distribution
        GOLDEN_RATIO,     // Golden ratio distribution
        COUNT
    };
    
    EuclideanSequencer();
    ~EuclideanSequencer() = default;
    
    // Pattern Generation
    void generatePattern(EuclideanPattern& pattern, Algorithm algorithm = Algorithm::BJORKLUND);
    void regenerateAllPatterns();
    
    // Per-Track Pattern Management
    void setTrackPattern(int trackIndex, int pulses, int rotation = 0);
    void setTrackProbability(int trackIndex, float probability);
    void setTrackSwing(int trackIndex, float swing);
    void setTrackHumanization(int trackIndex, float humanization);
    
    EuclideanPattern& getTrackPattern(int trackIndex);
    const EuclideanPattern& getTrackPattern(int trackIndex) const;
    
    // Real-time Pattern Queries
    bool shouldTriggerStep(int trackIndex, int stepIndex, float randomValue = -1.0f) const;
    float getStepVelocity(int trackIndex, int stepIndex) const;
    float getStepTiming(int trackIndex, int stepIndex) const;
    
    // Pattern Analysis
    float getPatternDensity(int trackIndex) const;
    int getPatternComplexity(int trackIndex) const;
    std::vector<int> getActiveSteps(int trackIndex) const;
    
    // Preset Patterns
    void loadPresetPattern(int trackIndex, const std::string& presetName);
    void savePresetPattern(int trackIndex, const std::string& presetName);
    std::vector<std::string> getAvailablePresets() const;
    
    // Pattern Morphing
    void morphBetweenPatterns(int trackIndex, const EuclideanPattern& targetPattern, float morphAmount);
    void randomizePattern(int trackIndex, float amount);
    
    // Hardware Integration
    void processHardwareInput(int keyIndex, bool pressed, int trackIndex);
    void visualizePattern(int trackIndex, uint32_t* displayBuffer, int width, int height) const;
    
    // Advanced Features
    void enablePolyrhythm(int trackIndex, bool enabled);
    void setPatternOffset(int trackIndex, int offset);  // Phase offset between tracks
    void linkPatterns(int track1, int track2, bool linked); // Link pattern changes
    
private:
    // Per-track euclidean patterns (8 tracks)
    std::array<EuclideanPattern, 8> trackPatterns_;
    
    // Pattern generation state
    Algorithm defaultAlgorithm_ = Algorithm::BJORKLUND;
    std::array<bool, 8> polyrhythmEnabled_;
    std::array<int, 8> patternOffsets_;
    std::array<std::array<int, 8>, 8> linkedTracks_; // Track linking matrix
    
    // Preset storage
    std::map<std::string, EuclideanPattern> presetPatterns_;
    
    // Random number generation
    std::mt19937 rng_;
    std::uniform_real_distribution<float> dist_;
    
    // Algorithm implementations
    std::vector<bool> bjorklundAlgorithm(int steps, int pulses) const;
    std::vector<bool> bresenhamAlgorithm(int steps, int pulses) const;
    std::vector<bool> fractionalAlgorithm(int steps, int pulses) const;
    std::vector<bool> goldenRatioAlgorithm(int steps, int pulses) const;
    
    // Pattern processing
    void applyRotation(std::vector<bool>& pattern, int rotation) const;
    void applySwing(EuclideanPattern& pattern) const;
    void applyHumanization(EuclideanPattern& pattern);
    void generateVelocities(EuclideanPattern& pattern);
    
    // Utility functions
    void initializePresets();
    void validateTrackIndex(int trackIndex) const;
    float calculateSwingOffset(int stepIndex, float swingAmount) const;
};

// Euclidean Preset Patterns
namespace EuclideanPresets {
    // Drum patterns
    EuclideanSequencer::EuclideanPattern fourOnFloor();      // (16, 4, 0) - House kick
    EuclideanSequencer::EuclideanPattern offBeatHats();      // (16, 8, 2) - Hi-hat pattern
    EuclideanSequencer::EuclideanPattern snareBackbeat();    // (16, 2, 8) - Snare on 2&4
    EuclideanSequencer::EuclideanPattern clave();            // (16, 5, 0) - Son clave
    EuclideanSequencer::EuclideanPattern tresillo();         // (16, 3, 0) - Cuban tresillo
    
    // Rhythmic patterns
    EuclideanSequencer::EuclideanPattern fiveAgainstFour();  // (16, 5, 0) - Polyrhythmic
    EuclideanSequencer::EuclideanPattern sevenEight();       // (16, 7, 0) - Complex rhythm
    EuclideanSequencer::EuclideanPattern goldenRatio();      // (16, 3, 5) - Golden ratio based
    
    // Melodic patterns
    EuclideanSequencer::EuclideanPattern arpeggioPattern();  // (16, 6, 1) - Melodic arpeggio
    EuclideanSequencer::EuclideanPattern bassLine();         // (16, 3, 0) - Bass pattern
    EuclideanSequencer::EuclideanPattern ambientPulse();     // (16, 2, 0) - Sparse ambient
    
    // Generative patterns
    EuclideanSequencer::EuclideanPattern randomWalk();       // Probabilistic pattern
    EuclideanSequencer::EuclideanPattern fibonacci();        // Fibonacci-based spacing
    EuclideanSequencer::EuclideanPattern primePulses();      // Prime number distribution
}

// Hardware mapping utilities
namespace EuclideanHardware {
    // Map euclidean parameters to hardware controls
    void mapPulsesToKey(int keyIndex, int& pulses);
    void mapRotationToKnob(float knobValue, int& rotation);
    void mapProbabilityToKnob(float knobValue, float& probability);
    
    // Visual feedback
    void renderEuclideanPattern(const EuclideanSequencer::EuclideanPattern& pattern,
                              uint32_t* displayBuffer, int width, int height,
                              uint32_t activeColor = 0xFF6B73, uint32_t inactiveColor = 0x333333);
    
    // Interactive editing
    void processPatternEdit(int stepIndex, bool enabled, EuclideanSequencer::EuclideanPattern& pattern);
}

} // namespace EtherSynth