#pragma once
#include "SynthEngineBase.h"
#include "../audio/FMOperator.h"
#include "../audio/ADSREnvelope.h"
#include "../audio/ParameterSmoother.h"
#include "../audio/OversamplingProcessor.h"
#include <array>

/**
 * Classic4OpFM - 4-operator FM synthesis engine with 8 curated algorithms
 * 
 * Features:
 * - 8 curated FM algorithms covering stacks and 2×2 configurations
 * - Per-operator ADSR envelopes with individual timing control
 * - Operator feedback paths with anti-click ramping
 * - FM index smoothing with zero-cross detection for glitch-free operation
 * - Phase continuity during parameter changes
 * - Oversampling for FM operators to reduce aliasing
 * - Velocity-sensitive operator levels and envelope scaling
 * - Optimized for classic FM timbres from gentle to aggressive
 * 
 * H/T/M Parameter Mapping:
 * - HARMONICS: Global FM index + operator ratio spread + high-frequency tilt
 * - TIMBRE: Algorithm selection + operator waveforms + brightness EQ
 * - MORPH: Feedback amount + envelope speeds + carrier/modulator balance
 */
class Classic4OpFMEngine {
public:
    enum class Algorithm {
        STACK_4_3_2_1,      // Linear stack: 4→3→2→1 (mellow)
        STACK_4_32_1,       // Split stack: 4→(3,2)→1 (fuller)
        PARALLEL_2X2,       // Two 2-op pairs: (4→3)+(2→1) (bright)
        CROSS_MOD,          // Cross modulation: 4⇄3, 2→1 (complex)
        RING_4321,          // Ring: 4→3→2→1→4 (metallic)
        CASCADE_42_31,      // Cascade: (4→2)+(3→1) (percussive)
        FEEDBACK_PAIR,      // (4→3 w/feedback)+(2→1) (harmonically rich)
        ALL_PARALLEL        // All parallel: 4+3+2+1 (additive)
    };
    
    enum class OperatorWaveform {
        SINE,               // Pure sine wave
        SAW_APPROX,         // Sine-based saw approximation
        SQUARE_APPROX,      // Sine-based square approximation
        TRIANGLE_APPROX,    // Sine-based triangle approximation
        HALF_SINE,          // Half-wave rectified sine
        FULL_SINE,          // Full-wave rectified sine
        QUARTER_SINE,       // Quarter sine wave
        ALT_SINE            // Alternating sine polarity
    };
    
    struct OperatorConfig {
        float ratio = 1.0f;             // Frequency ratio (0.5 - 8.0)
        float level = 1.0f;             // Operator output level
        float detune = 0.0f;            // Fine detune in cents
        OperatorWaveform waveform = OperatorWaveform::SINE;
        bool fixedFreq = false;         // Fixed frequency mode
        float fixedFreqHz = 440.0f;     // Fixed frequency value
        float velocitySensitivity = 0.5f; // Velocity to level sensitivity
        float keyScaling = 0.0f;        // Key scaling amount
        bool enabled = true;            // Operator enable/disable
    };
    
    struct EnvelopeConfig {
        float attack = 0.001f;          // Attack time (0.001 - 10.0s)
        float decay = 0.1f;             // Decay time (0.001 - 10.0s)
        float sustain = 0.7f;           // Sustain level (0.0 - 1.0)
        float release = 0.5f;           // Release time (0.001 - 10.0s)
        float depth = 1.0f;             // Envelope depth scaling
        float velocitySensitivity = 0.3f; // Velocity to envelope scaling
        bool exponential = true;        // Exponential vs linear segments
    };
    
    struct AlgorithmConfig {
        Algorithm algorithm = Algorithm::STACK_4_3_2_1;
        float feedback = 0.0f;          // Feedback amount (0.0 - 1.0)
        float operatorBalance[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // Per-op balance
        float carrierLevel = 1.0f;      // Overall carrier level
        float modulatorLevel = 1.0f;    // Overall modulator level
        bool antiClick = true;          // Enable anti-click processing
        float transitionTime = 0.02f;   // Algorithm switch time
    };
    
    struct GlobalConfig {
        float masterLevel = 1.0f;       // Master output level
        float brightness = 0.0f;        // High-frequency emphasis
        float warmth = 0.0f;            // Low-mid emphasis
        bool oversample = true;         // Enable 2x oversampling
        float noiseLevel = 0.0f;        // Noise blend for character
        float analogDrift = 0.0f;       // Analog-style pitch drift
        bool monoMode = false;          // Monophonic mode
        float portamentoTime = 0.0f;    // Portamento time (mono mode)
    };
    
    struct VoiceState {
        bool active = false;
        bool notePressed = false;
        float note = 60.0f;
        float targetNote = 60.0f;       // For portamento
        float velocity = 100.0f;
        float pitchBend = 0.0f;
        
        // Operator states
        std::array<float, 4> operatorPhases;
        std::array<float, 4> operatorFreqs;
        std::array<float, 4> operatorLevels;
        std::array<bool, 4> operatorActive;
        
        // Feedback state
        float feedbackSample = 0.0f;
        float lastOutput = 0.0f;
        
        // Portamento state
        float portamentoPhase = 1.0f;
        
        // Anti-click state
        float algorithmCrossfade = 1.0f;
        Algorithm previousAlgorithm = Algorithm::STACK_4_3_2_1;
        bool algorithmSwitching = false;
        uint32_t switchStartTime = 0;
        
        // Timing
        uint32_t noteOnTime = 0;
        uint32_t noteOffTime = 0;
    };
    
    Classic4OpFMEngine();
    ~Classic4OpFMEngine();
    
    // Initialization
    bool initialize(float sampleRate);
    void shutdown();
    
    // Configuration
    void setOperatorConfig(int operatorIndex, const OperatorConfig& config);
    const OperatorConfig& getOperatorConfig(int operatorIndex) const;
    
    void setEnvelopeConfig(int operatorIndex, const EnvelopeConfig& config);
    const EnvelopeConfig& getEnvelopeConfig(int operatorIndex) const;
    
    void setAlgorithmConfig(const AlgorithmConfig& config);
    const AlgorithmConfig& getAlgorithmConfig() const { return algorithmConfig_; }
    
    void setGlobalConfig(const GlobalConfig& config);
    const GlobalConfig& getGlobalConfig() const { return globalConfig_; }
    
    // H/T/M Parameter control (implements engine interface)
    void setHarmonics(float harmonics);    // 0-1: FM index + ratio spread + high tilt
    void setTimbre(float timbre);          // 0-1: Algorithm + waveforms + brightness
    void setMorph(float morph);            // 0-1: Feedback + envelope speed + balance
    
    void setHTMParameters(float harmonics, float timbre, float morph);
    void getHTMParameters(float& harmonics, float& timbre, float& morph) const;
    
    // Voice control
    void noteOn(float note, float velocity);
    void noteOff(float releaseTime = 0.0f);
    void allNotesOff();
    void setPitchBend(float bendAmount); // -1 to +1 (typically ±2 semitones)
    
    // Real-time parameter control
    void setAlgorithm(Algorithm algorithm);
    void setFeedback(float feedback);
    void setOperatorRatio(int operatorIndex, float ratio);
    void setOperatorLevel(int operatorIndex, float level);
    void setGlobalIndex(float index); // Master FM index scaling
    
    // Audio processing
    std::pair<float, float> processSampleStereo(); // Returns {left, right}
    float processSample(); // Mono output
    void processBlockStereo(float* outputL, float* outputR, int numSamples);
    void processBlock(float* output, int numSamples);
    void processParameters(float deltaTimeMs);
    
    // Analysis
    bool isActive() const { return voiceState_.active; }
    float getCurrentNote() const { return voiceState_.note; }
    Algorithm getCurrentAlgorithm() const { return algorithmConfig_.algorithm; }
    float getFeedbackAmount() const { return algorithmConfig_.feedback; }
    float getCPUUsage() const;
    
    // Operator analysis
    float getOperatorLevel(int operatorIndex) const;
    float getOperatorFrequency(int operatorIndex) const;
    bool isOperatorActive(int operatorIndex) const;
    float getEnvelopeLevel(int operatorIndex) const;
    
    // Preset management
    void reset();
    void setPreset(const std::string& presetName);
    
private:
    // Core audio components
    std::array<FMOperator, 4> operators_;
    std::array<ADSREnvelope, 4> envelopes_;
    OversamplingProcessor oversampler_;
    
    // Parameter smoothing
    ParameterSmoother feedbackSmoother_;
    ParameterSmoother indexSmoother_;
    ParameterSmoother brightnessSmoother_;
    std::array<ParameterSmoother, 4> ratioSmoothers_;
    std::array<ParameterSmoother, 4> levelSmoothers_;
    
    // Configuration
    std::array<OperatorConfig, 4> operatorConfigs_;
    std::array<EnvelopeConfig, 4> envelopeConfigs_;
    AlgorithmConfig algorithmConfig_;
    GlobalConfig globalConfig_;
    
    // State
    VoiceState voiceState_;
    float sampleRate_;
    bool initialized_;
    
    // Current parameter values
    float harmonics_ = 0.5f;
    float timbre_ = 0.5f;
    float morph_ = 0.5f;
    
    // Internal processing state
    float currentIndex_ = 0.5f;
    float currentFeedback_ = 0.0f;
    float currentBrightness_ = 0.0f;
    std::array<float, 4> currentRatios_;
    std::array<float, 4> currentLevels_;
    
    // Algorithm processing
    float previousAlgorithmOutput_ = 0.0f;
    float currentAlgorithmOutput_ = 0.0f;
    
    // Anti-click processing
    std::array<float, 4> operatorRamps_;
    float masterRamp_ = 1.0f;
    bool ramping_ = false;
    
    // Noise generation
    float noisePhase_ = 0.0f;
    uint32_t noiseState_ = 1;
    
    // Performance monitoring
    float cpuUsage_ = 0.0f;
    uint32_t processingStartTime_ = 0;
    
    // Private methods
    void updateOperatorParameters();
    void updateEnvelopeParameters();
    void updateAlgorithmParameters();
    void updateGlobalParameters();
    
    // Algorithm processing
    float processAlgorithm(Algorithm algorithm, const std::array<float, 4>& operatorInputs);
    float processStack4321(const std::array<float, 4>& ops);
    float processStack432_1(const std::array<float, 4>& ops);
    float processParallel2x2(const std::array<float, 4>& ops);
    float processCrossMod(const std::array<float, 4>& ops);
    float processRing4321(const std::array<float, 4>& ops);
    float processCascade42_31(const std::array<float, 4>& ops);
    float processFeedbackPair(const std::array<float, 4>& ops);
    float processAllParallel(const std::array<float, 4>& ops);
    
    // Operator processing
    void processOperators();
    float generateOperatorOutput(int operatorIndex, float modulationInput);
    void updateOperatorFrequencies();
    void updateOperatorLevels(float velocity);
    
    // Anti-click processing
    void initializeAntiClick();
    void processAntiClick();
    bool shouldRampOperator(int operatorIndex, float newValue, float currentValue);
    void startOperatorRamp(int operatorIndex, float targetValue);
    void updateOperatorRamps();
    
    // Feedback processing
    float processFeedback(float input, float feedbackAmount);
    void updateFeedbackState(float output);
    
    // Portamento processing
    void updatePortamento(float deltaTimeMs);
    float calculatePortamentoNote();
    
    // Algorithm switching
    void switchAlgorithm(Algorithm newAlgorithm);
    void processAlgorithmCrossfade();
    bool isAlgorithmSwitchComplete() const;
    
    // Parameter mapping
    float mapHarmonicsToIndex(float harmonics) const;
    float mapTimbreToAlgorithm(float timbre) const;
    Algorithm selectAlgorithmFromTimbre(float timbre) const;
    float mapMorphToFeedback(float morph) const;
    void updateRatioSpread(float harmonics);
    void updateEnvelopeSpeeds(float morph);
    
    // EQ processing
    std::pair<float, float> processEQ(float input);
    float applyBrightness(float input, float brightness);
    float applyWarmth(float input, float warmth);
    
    // Noise processing
    float generateNoise();
    void updateAnalogDrift();
    
    // Utility functions
    float noteToFrequency(float note) const;
    float ratioToFrequency(float ratio, float baseFreq) const;
    float centsToRatio(float cents) const;
    float dbToLinear(float db) const;
    float linearToDb(float linear) const;
    uint32_t getTimeMs() const;
    float lerp(float a, float b, float t) const;
    float clamp(float value, float min, float max) const;
    float crossfade(float a, float b, float mix) const;
    
    // Waveform generation
    float generateWaveform(OperatorWaveform waveform, float phase) const;
    float sineApproximation(float phase) const;
    float sawApproximation(float phase) const;
    float squareApproximation(float phase) const;
    
    // Performance utilities
    bool isOperatorAudible(int operatorIndex) const;
    void optimizeInactiveOperators();
    
    // Constants
    static constexpr int NUM_OPERATORS = 4;
    static constexpr float MIN_RATIO = 0.125f;
    static constexpr float MAX_RATIO = 16.0f;
    static constexpr float MAX_FM_INDEX = 8.0f;
    static constexpr float MAX_FEEDBACK = 0.9f;
    static constexpr float ANTI_CLICK_TIME_MS = 2.0f;
    static constexpr float PORTAMENTO_MAX_TIME_MS = 1000.0f;
    static constexpr float NOISE_AMPLITUDE = 0.001f;
    static constexpr float CPU_USAGE_SMOOTH = 0.99f;
    static constexpr float ALGORITHM_SWITCH_TIME_MS = 20.0f;
    
    // Algorithm topology constants
    static constexpr float ALGORITHM_MIX_RATIOS[8][4] = {
        {0.0f, 0.0f, 0.0f, 1.0f},  // STACK_4_3_2_1: only op1 to output
        {0.0f, 0.0f, 0.5f, 0.5f},  // STACK_4_32_1: op2+op1 to output
        {0.0f, 0.5f, 0.0f, 0.5f},  // PARALLEL_2X2: op3+op1 to output
        {0.0f, 0.3f, 0.0f, 0.7f},  // CROSS_MOD: weighted op3+op1
        {0.0f, 0.0f, 0.0f, 1.0f},  // RING_4321: op1 to output
        {0.0f, 0.5f, 0.0f, 0.5f},  // CASCADE_42_31: op3+op1 to output
        {0.0f, 0.4f, 0.0f, 0.6f},  // FEEDBACK_PAIR: weighted op3+op1
        {0.25f, 0.25f, 0.25f, 0.25f} // ALL_PARALLEL: all ops equal
    };
};