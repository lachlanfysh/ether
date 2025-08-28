#pragma once
#include "SequencerStep.h"
#include <vector>
#include <array>
#include <memory>

/**
 * SequencerPattern - Multi-track pattern with advanced step parameters
 * 
 * Manages sequences of steps across multiple tracks with support for:
 * - Variable pattern lengths (1-64 steps)
 * - Multiple tracks (up to 8 tracks per pattern) 
 * - Per-step slide timing and accent flags
 * - Pattern-level swing, shuffle, and timing parameters
 * - Real-time safe step access and modification
 * - Pattern selection and clipboard operations
 * 
 * Features:
 * - Efficient step storage with 64-bit serialization
 * - Track muting, soloing, and level control
 * - Pattern chaining and loop modes
 * - Velocity modulation integration
 * - Compatible with tape squashing workflow
 */
class SequencerPattern {
public:
    // Pattern configuration
    static constexpr int MAX_TRACKS = 8;
    static constexpr int MAX_STEPS = 64;
    static constexpr int MIN_STEPS = 1;
    static constexpr int DEFAULT_STEPS = 16;
    
    // Track types
    enum class TrackType {
        MONO_SYNTH,     // Monophonic synthesizer track
        POLY_SYNTH,     // Polyphonic synthesizer track  
        DRUM,           // Drum/percussion track
        SAMPLER,        // Multi-sample track
        AUX             // Auxiliary/modulation track
    };
    
    // Track configuration
    struct TrackConfig {
        TrackType type;
        bool enabled;
        bool muted;
        bool solo;
        float level;            // Track level (0.0-1.0)
        uint8_t midiChannel;    // MIDI channel (0-15)
        int8_t transpose;       // Transpose in semitones (-24 to +24)
        
        TrackConfig() : 
            type(TrackType::MONO_SYNTH),
            enabled(true),
            muted(false), 
            solo(false),
            level(0.8f),
            midiChannel(0),
            transpose(0) {}
    };
    
    // Pattern timing configuration
    struct TimingConfig {
        float swing;            // Swing amount (0.0-1.0)
        float shuffle;          // Shuffle amount (0.0-1.0)  
        int8_t humanize;        // Humanization amount (-64 to +63)
        float gateTime;         // Default gate time (0.1-2.0)
        bool quantizeInput;     // Quantize input recording
        
        TimingConfig() :
            swing(0.0f),
            shuffle(0.0f),
            humanize(0),
            gateTime(0.8f),
            quantizeInput(true) {}
    };
    
    SequencerPattern();
    SequencerPattern(int numSteps, int numTracks = 1);
    ~SequencerPattern() = default;
    
    // Pattern structure
    void setLength(int numSteps);
    void setNumTracks(int numTracks);
    int getLength() const { return numSteps_; }
    int getNumTracks() const { return numTracks_; }
    
    // Step access
    SequencerStep* getStep(int track, int step);
    const SequencerStep* getStep(int track, int step) const;
    void setStep(int track, int step, const SequencerStep& stepData);
    void clearStep(int track, int step);
    void copyStep(int fromTrack, int fromStep, int toTrack, int toStep);
    
    // Step convenience methods
    void setStepNote(int track, int step, uint8_t note, uint8_t velocity = 100);
    void setStepAccent(int track, int step, bool accent, uint8_t amount = 64);
    void setStepSlide(int track, int step, bool slide, uint8_t timeMs = 20);
    void toggleStepAccent(int track, int step);
    void toggleStepSlide(int track, int step);
    
    // Track configuration
    void setTrackConfig(int track, const TrackConfig& config);
    const TrackConfig& getTrackConfig(int track) const;
    void setTrackType(int track, TrackType type);
    void setTrackMute(int track, bool muted);
    void setTrackSolo(int track, bool solo);
    void setTrackLevel(int track, float level);
    void setTrackTranspose(int track, int8_t semitones);
    
    // Track state queries
    bool isTrackMuted(int track) const;
    bool isTrackSolo(int track) const;
    bool isTrackAudible(int track) const;   // Considers mute/solo state
    float getTrackLevel(int track) const;
    
    // Pattern operations
    void clear();
    void clearTrack(int track);
    void clearRange(int track, int startStep, int endStep);
    void shiftPattern(int steps);           // Shift entire pattern
    void shiftTrack(int track, int steps);  // Shift single track
    void reversePattern();
    void reverseTrack(int track);
    void randomizeVelocities(int track, uint8_t minVel = 80, uint8_t maxVel = 127);
    void randomizeAccents(int track, float probability = 0.2f);
    
    // Pattern selection support (for tape squashing)
    struct Selection {
        int startTrack, endTrack;   // Track range (inclusive)
        int startStep, endStep;     // Step range (inclusive)
        bool isValid() const;
    };
    
    void setSelection(const Selection& selection);
    const Selection& getSelection() const { return selection_; }
    void clearSelection();
    bool hasSelection() const;
    
    // Selected region operations
    void copySelection();                   // Copy to internal clipboard
    void cutSelection();                    // Cut to clipboard
    void pasteSelection(int targetTrack, int targetStep);
    bool hasClipboard() const;
    
    // Timing configuration
    void setTimingConfig(const TimingConfig& config);
    const TimingConfig& getTimingConfig() const { return timing_; }
    void setSwing(float swing);
    void setShuffle(float shuffle);
    void setHumanize(int8_t humanize);
    void setGateTime(float gateTime);
    
    // Pattern analysis
    int countActiveSteps(int track) const;
    int countAccentSteps(int track) const;
    int countSlideSteps(int track) const;
    bool isEmpty() const;
    bool isTrackEmpty(int track) const;
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);
    size_t getSerializedSize() const;
    
    // Pattern validation
    bool isValidTrack(int track) const;
    bool isValidStep(int step) const;
    bool isValidPosition(int track, int step) const;
    
    // Pattern metadata
    void setName(const std::string& name) { name_ = name; }
    const std::string& getName() const { return name_; }
    void setTempo(float bpm) { tempo_ = bpm; }
    float getTempo() const { return tempo_; }
    
private:
    int numSteps_;
    int numTracks_;
    std::string name_;
    float tempo_;
    
    // Step data storage [track][step]
    std::array<std::array<SequencerStep, MAX_STEPS>, MAX_TRACKS> steps_;
    
    // Track configurations
    std::array<TrackConfig, MAX_TRACKS> trackConfigs_;
    
    // Timing configuration
    TimingConfig timing_;
    
    // Selection and clipboard
    Selection selection_;
    std::vector<std::vector<SequencerStep>> clipboard_;
    bool hasValidClipboard_;
    
    // Internal utilities
    void initializeSteps();
    void validateSelection(Selection& selection) const;
    void copyStepsToClipboard(const Selection& selection);
    bool validateTrackRange(int track) const;
    bool validateStepRange(int step) const;
    
    // Constants for validation
    static constexpr float MIN_SWING = 0.0f;
    static constexpr float MAX_SWING = 1.0f;
    static constexpr float MIN_LEVEL = 0.0f;
    static constexpr float MAX_LEVEL = 2.0f;  // Allow some headroom
    static constexpr int8_t MIN_TRANSPOSE = -24;
    static constexpr int8_t MAX_TRANSPOSE = 24;
};