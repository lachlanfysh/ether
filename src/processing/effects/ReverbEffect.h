#pragma once
#include "../core/Types.h"
#include <vector>
#include <array>

/**
 * High-Quality Reverb Effect
 * Implements Schroeder reverb with Freeverb enhancements
 */
class ReverbEffect {
public:
    ReverbEffect();
    ~ReverbEffect();
    
    void initialize(float sampleRate);
    void process(EtherAudioBuffer& buffer);
    void reset();
    
    // Parameters
    void setRoomSize(float roomSize);      // 0.0 - 1.0
    void setDamping(float damping);        // 0.0 - 1.0
    void setWetLevel(float wetLevel);      // 0.0 - 1.0
    void setDryLevel(float dryLevel);      // 0.0 - 1.0
    void setWidth(float width);            // 0.0 - 1.0 (stereo width)
    void setPreDelay(float preDelayMs);    // 0.0 - 100.0 ms
    
    float getRoomSize() const { return roomSize_; }
    float getDamping() const { return damping_; }
    float getWetLevel() const { return wetLevel_; }
    float getDryLevel() const { return dryLevel_; }
    float getWidth() const { return width_; }
    float getPreDelay() const { return preDelayMs_; }
    
    // Advanced controls
    void setEarlyReflections(bool enable);
    void setHighCut(float frequency);      // Hz
    void setLowCut(float frequency);       // Hz
    void setModulation(float rate, float depth); // Chorus-like modulation
    
    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }

private:
    // Freeverb parameters
    static constexpr int NUM_COMBS = 8;
    static constexpr int NUM_ALLPASS = 4;
    static constexpr float FIXED_GAIN = 0.015f;
    static constexpr float SCALE_WET = 3.0f;
    static constexpr float SCALE_DRY = 2.0f;
    static constexpr float SCALE_DAMP = 0.4f;
    static constexpr float SCALE_ROOM = 0.28f;
    static constexpr float OFFSET_ROOM = 0.7f;
    static constexpr float INITIAL_ROOM = 0.5f;
    static constexpr float INITIAL_DAMP = 0.5f;
    static constexpr float INITIAL_WET = 1.0f / SCALE_WET;
    static constexpr float INITIAL_DRY = 0.0f;
    static constexpr float INITIAL_WIDTH = 1.0f;
    
    // Comb filter delays (samples at 44.1kHz)
    static constexpr int COMB_DELAYS[NUM_COMBS] = {
        1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617
    };
    
    // Allpass filter delays
    static constexpr int ALLPASS_DELAYS[NUM_ALLPASS] = {
        556, 441, 341, 225
    };
    
    // Comb filter
    struct CombFilter {
        std::vector<float> buffer;
        int bufferSize = 0;
        int bufferIndex = 0;
        float feedback = 0.0f;
        float filterStore = 0.0f;
        float damping1 = 0.0f;
        float damping2 = 0.0f;
        
        void setBuffer(int size);
        void setDamping(float val);
        void setFeedback(float val);
        float process(float input);
        void clear();
    };
    
    // Allpass filter
    struct AllpassFilter {
        std::vector<float> buffer;
        int bufferSize = 0;
        int bufferIndex = 0;
        float feedback = 0.5f;
        
        void setBuffer(int size);
        void setFeedback(float val);
        float process(float input);
        void clear();
    };
    
    // Pre-delay buffer
    struct PreDelayBuffer {
        std::vector<float> buffer;
        int writeIndex = 0;
        int readIndex = 0;
        int delayLength = 0;
        
        void setDelay(int samples);
        float process(float input);
        void clear();
    };
    
    // Filter (simple one-pole)
    struct OneThePoleFilter {
        float state = 0.0f;
        float coefficient = 0.0f;
        
        void setFrequency(float frequency, float sampleRate, bool highpass = false);
        float process(float input);
        void clear();
    };
    
    // LFO for modulation
    struct LFO {
        float phase = 0.0f;
        float frequency = 0.0f;
        float sampleRate = 44100.0f;
        
        void setFrequency(float freq) { frequency = freq; }
        void setSampleRate(float sr) { sampleRate = sr; }
        float process(); // Returns sine wave -1 to 1
        void reset() { phase = 0.0f; }
    };
    
    // Filter bank for each channel
    std::array<CombFilter, NUM_COMBS> combFiltersL_;
    std::array<CombFilter, NUM_COMBS> combFiltersR_;
    std::array<AllpassFilter, NUM_ALLPASS> allpassFiltersL_;
    std::array<AllpassFilter, NUM_ALLPASS> allpassFiltersR_;
    
    // Pre-delay
    PreDelayBuffer preDelayL_;
    PreDelayBuffer preDelayR_;
    
    // EQ filters
    OneThePoleFilter highCutL_, highCutR_;
    OneThePoleFilter lowCutL_, lowCutR_;
    
    // Modulation
    LFO modulationLFO_;
    std::vector<float> modulationBufferL_;
    std::vector<float> modulationBufferR_;
    int modulationIndex_ = 0;
    
    // Parameters
    float roomSize_ = INITIAL_ROOM;
    float damping_ = INITIAL_DAMP;
    float wetLevel_ = INITIAL_WET;
    float dryLevel_ = INITIAL_DRY;
    float width_ = INITIAL_WIDTH;
    float preDelayMs_ = 0.0f;
    
    float gain_ = FIXED_GAIN;
    float roomSize1_ = 0.0f;
    float damping1_ = 0.0f;
    float wet1_ = 0.0f;
    float wet2_ = 0.0f;
    float dry_ = 0.0f;
    
    float sampleRate_ = 44100.0f;
    bool enabled_ = true;
    bool earlyReflections_ = true;
    float highCutFreq_ = 8000.0f;
    float lowCutFreq_ = 100.0f;
    float modulationRate_ = 0.5f;
    float modulationDepth_ = 0.0f;
    
    // Early reflections
    struct EarlyReflection {
        int delay;
        float gain;
        float pan; // -1 to 1
    };
    
    static constexpr int NUM_EARLY_REFLECTIONS = 6;
    std::array<EarlyReflection, NUM_EARLY_REFLECTIONS> earlyReflections_;
    std::array<std::vector<float>, NUM_EARLY_REFLECTIONS> earlyBuffers_;
    std::array<int, NUM_EARLY_REFLECTIONS> earlyIndices_;
    
    // Private methods
    void updateParameters();
    void calculateEarlyReflections();
    void processEarlyReflections(float input, float& outputL, float& outputR);
    void setupBuffers();
    
    // Tuning for different sample rates
    int adjustDelay(int baseDelay) const;
};

// Factory presets for reverb
namespace ReverbPresets {
    struct ReverbSettings {
        float roomSize;
        float damping;
        float wetLevel;
        float dryLevel;
        float width;
        float preDelay;
        float highCut;
        float lowCut;
        bool earlyReflections;
        const char* name;
    };
    
    extern const ReverbSettings SMALL_ROOM;
    extern const ReverbSettings MEDIUM_ROOM;
    extern const ReverbSettings LARGE_ROOM;
    extern const ReverbSettings HALL;
    extern const ReverbSettings CATHEDRAL;
    extern const ReverbSettings PLATE;
    extern const ReverbSettings SPRING;
    extern const ReverbSettings AMBIENT;
    extern const ReverbSettings VOCAL_HALL;
    extern const ReverbSettings DRUM_ROOM;
}