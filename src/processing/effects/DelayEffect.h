#pragma once
#include "../core/Types.h"
#include <vector>
#include <memory>

/**
 * High-quality delay effect with modulation and filtering
 * Perfect for creating spacious, evolving sounds
 */
class DelayEffect {
public:
    DelayEffect();
    ~DelayEffect();
    
    // Core processing
    void process(EtherAudioBuffer& buffer);
    void reset();
    
    // Parameters
    void setDelayTime(float timeSeconds);      // 0.001 - 2.0 seconds
    void setFeedback(float feedback);          // 0.0 - 0.95
    void setMix(float mix);                    // 0.0 - 1.0 (dry/wet)
    void setModulationRate(float rateHz);      // 0.1 - 10.0 Hz
    void setModulationDepth(float depth);      // 0.0 - 1.0
    void setHighCut(float frequency);          // 100 - 20000 Hz
    void setLowCut(float frequency);           // 20 - 2000 Hz
    
    // Getters
    float getDelayTime() const { return delayTime_; }
    float getFeedback() const { return feedback_; }
    float getMix() const { return mix_; }
    float getModulationRate() const { return modRate_; }
    float getModulationDepth() const { return modDepth_; }
    
    // Presets
    void loadPreset(const std::string& presetName);
    
private:
    // Delay line
    std::vector<AudioFrame> delayBuffer_;
    size_t bufferSize_;
    size_t writeIndex_;
    
    // Parameters
    float delayTime_ = 0.25f;        // seconds
    float feedback_ = 0.3f;          // 0-1
    float mix_ = 0.3f;               // 0-1
    float modRate_ = 0.5f;           // Hz
    float modDepth_ = 0.0f;          // 0-1
    
    // Modulation
    float modPhase_ = 0.0f;
    float modPhaseIncrement_ = 0.0f;
    
    // Filtering (simple one-pole)
    float highCutFreq_ = 8000.0f;
    float lowCutFreq_ = 100.0f;
    float highCutCoeff_ = 0.9f;
    float lowCutCoeff_ = 0.1f;
    AudioFrame filterState_;
    
    // Helper methods
    void updateModulation();
    float getDelayTimeSamples() const;
    AudioFrame readDelayInterpolated(float delaySamples);
    void writeDelayFeedback(const AudioFrame& sample);
    AudioFrame applyFiltering(const AudioFrame& input);
    void updateFilterCoefficients();
    
    // Constants
    static constexpr float MAX_DELAY_TIME = 2.0f; // seconds
    static constexpr size_t MAX_BUFFER_SIZE = static_cast<size_t>(MAX_DELAY_TIME * SAMPLE_RATE);
};

/**
 * Reverb effect using multiple delay lines and allpass filters
 * Creates lush, realistic reverb tails
 */
class ReverbEffect {
public:
    ReverbEffect();
    ~ReverbEffect();
    
    // Core processing
    void process(EtherAudioBuffer& buffer);
    void reset();
    
    // Parameters
    void setRoomSize(float size);              // 0.0 - 1.0
    void setDamping(float damping);            // 0.0 - 1.0
    void setMix(float mix);                    // 0.0 - 1.0
    void setPreDelay(float delayMs);           // 0 - 100 ms
    void setDiffusion(float diffusion);        // 0.0 - 1.0
    
    // Getters
    float getRoomSize() const { return roomSize_; }
    float getDamping() const { return damping_; }
    float getMix() const { return mix_; }
    
    // Presets
    void loadPreset(const std::string& presetName);
    
private:
    // Reverb topology based on Freeverb algorithm
    static constexpr size_t NUM_COMBS = 8;
    static constexpr size_t NUM_ALLPASS = 4;
    
    struct CombFilter {
        std::vector<float> buffer;
        size_t bufferSize;
        size_t index;
        float feedback;
        float filterStore;
        float damp1;
        float damp2;
        
        CombFilter(size_t size);
        float process(float input);
    };
    
    struct AllpassFilter {
        std::vector<float> buffer;
        size_t bufferSize;
        size_t index;
        float feedback;
        
        AllpassFilter(size_t size);
        float process(float input);
    };
    
    // Filter banks
    std::array<CombFilter, NUM_COMBS> combFiltersL_;
    std::array<CombFilter, NUM_COMBS> combFiltersR_;
    std::array<AllpassFilter, NUM_ALLPASS> allpassFiltersL_;
    std::array<AllpassFilter, NUM_ALLPASS> allpassFiltersR_;
    
    // Pre-delay
    std::vector<AudioFrame> preDelayBuffer_;
    size_t preDelaySize_ = 0;
    size_t preDelayIndex_ = 0;
    
    // Parameters
    float roomSize_ = 0.5f;
    float damping_ = 0.5f;
    float mix_ = 0.3f;
    float gain_ = 0.015f;
    
    // Update methods
    void updateCombFilters();
    void setRoomSizeInternal(float value);
    void setDampingInternal(float value);
    
    // Comb filter sizes (in samples) - optimized for minimal coloration
    static constexpr std::array<size_t, NUM_COMBS> COMB_SIZES = {
        1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617
    };
    
    // Allpass filter sizes
    static constexpr std::array<size_t, NUM_ALLPASS> ALLPASS_SIZES = {
        556, 441, 341, 225
    };
};

/**
 * Chorus effect with multiple delay lines for rich modulation
 */
class ChorusEffect {
public:
    ChorusEffect();
    ~ChorusEffect();
    
    // Core processing
    void process(EtherAudioBuffer& buffer);
    void reset();
    
    // Parameters
    void setRate(float rateHz);                // 0.1 - 10.0 Hz
    void setDepth(float depth);                // 0.0 - 1.0
    void setMix(float mix);                    // 0.0 - 1.0
    void setFeedback(float feedback);          // 0.0 - 0.7
    void setVoices(int numVoices);             // 1 - 4
    
    // Getters
    float getRate() const { return rate_; }
    float getDepth() const { return depth_; }
    float getMix() const { return mix_; }
    
private:
    static constexpr int MAX_VOICES = 4;
    static constexpr float BASE_DELAY = 0.020f; // 20ms base delay
    
    struct ChorusVoice {
        std::vector<AudioFrame> delayBuffer;
        size_t writeIndex;
        float phase;
        float phaseIncrement;
        float delayOffset;
    };
    
    std::array<ChorusVoice, MAX_VOICES> voices_;
    int numActiveVoices_ = 2;
    
    // Parameters
    float rate_ = 0.5f;
    float depth_ = 0.3f;
    float mix_ = 0.5f;
    float feedback_ = 0.1f;
    
    // Helper methods
    void initializeVoices();
    AudioFrame readDelayInterpolated(const ChorusVoice& voice, float delaySamples);
    
    static constexpr size_t DELAY_BUFFER_SIZE = static_cast<size_t>((BASE_DELAY + 0.010f) * SAMPLE_RATE);
};