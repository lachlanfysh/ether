#pragma once
#include "SynthEngine.h"
#include <array>

/**
 * 4-Operator FM Synthesis Engine
 * Classic DX7-style frequency modulation with modern enhancements
 */
class FMEngine : public SynthEngine {
public:
    FMEngine();
    ~FMEngine() override;
    
    // Core interface
    void processAudio(EtherAudioBuffer& buffer) override;
    void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f) override;
    void noteOff(uint8_t note) override;
    void setAftertouch(uint8_t note, float aftertouch) override;
    void allNotesOff() override;
    
    // Parameter control
    void setParameter(ParameterID param, float value) override;
    float getParameter(ParameterID param) const override;
    bool hasParameter(ParameterID param) const override;
    
    // FM-specific parameters
    void setOperatorRatio(int op, float ratio);        // Frequency ratios
    void setOperatorLevel(int op, float level);        // Output levels
    void setOperatorFeedback(int op, float feedback);  // Self-feedback
    void setAlgorithm(int algorithm);                  // Routing algorithm (1-32)
    void setModulationDepth(int source, int dest, float depth);
    
    // Envelope control per operator
    void setOperatorEnvelope(int op, float attack, float decay, float sustain, float release);
    
    // Engine info
    const char* getName() const override { return "FM Synth"; }
    const char* getDescription() const override { return "4-operator FM synthesis"; }
    EngineType getType() const override { return EngineType::FM; }
    
    // Voice management
    size_t getActiveVoiceCount() const override;
    size_t getMaxVoiceCount() const override { return MAX_VOICES; }
    void setVoiceCount(size_t maxVoices) override;
    
    // Performance monitoring
    float getCPUUsage() const override;
    
    // Preset management
    void savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const override;
    bool loadPreset(const uint8_t* data, size_t size) override;
    
    // Audio configuration
    void setSampleRate(float sampleRate) override;
    void setBufferSize(size_t bufferSize) override;

private:
    static constexpr int NUM_OPERATORS = 4;
    static constexpr int NUM_ALGORITHMS = 32;
    
    // FM Operator
    struct FMOperator {
        float phase = 0.0f;
        float frequency = 440.0f;
        float ratio = 1.0f;           // Frequency ratio
        float level = 1.0f;           // Output level
        float feedback = 0.0f;        // Self-feedback amount
        float lastOutput = 0.0f;      // For feedback
        
        // Envelope (per-operator ADSR)
        float envPhase = 0.0f;
        float envValue = 0.0f;
        float attack = 0.01f;
        float decay = 0.3f;
        float sustain = 0.7f;
        float release = 0.5f;
        bool envReleasing = false;
        
        // Modulation input
        float modInput = 0.0f;
        
        float process(float modulation = 0.0f);
        void updateEnvelope(float deltaTime);
        void noteOn();
        void noteOff();
    };
    
    // FM Voice
    struct FMVoice {
        uint8_t note = 0;
        float velocity = 0.0f;
        float baseFrequency = 440.0f;
        bool active = false;
        uint32_t noteOnTime = 0;
        
        std::array<FMOperator, NUM_OPERATORS> operators;
        
        void noteOn(uint8_t noteNum, float vel);
        void noteOff();
        float process();
        bool isFinished() const;
    };
    
    std::array<FMVoice, MAX_VOICES> voices_;
    
    // FM Algorithm matrix (operator routing)
    struct Algorithm {
        std::array<std::array<float, NUM_OPERATORS>, NUM_OPERATORS> matrix;
        std::array<bool, NUM_OPERATORS> carriers; // Which operators go to output
    };
    
    std::array<Algorithm, NUM_ALGORITHMS> algorithms_;
    int currentAlgorithm_ = 0;
    
    // Global parameters
    float volume_ = 0.8f;
    float pitchBend_ = 0.0f;
    float modWheel_ = 0.0f;
    
    // Touch interface integration
    float touchX_ = 0.5f;
    float touchY_ = 0.5f;
    
    // Helper methods
    FMVoice* findFreeVoice();
    void initializeAlgorithms();
    void setupAlgorithm(int index, const std::vector<std::vector<float>>& connections, 
                       const std::vector<bool>& carriers);
    float processVoiceWithAlgorithm(FMVoice& voice);
    
    // Classic DX7 algorithms
    void setupDX7Algorithms();
    
    // Modulation matrix for operator interconnections
    std::array<std::array<float, NUM_OPERATORS>, NUM_OPERATORS> modMatrix_;
    
    // Performance monitoring
    mutable float cpuUsage_ = 0.0f;
};

// FM Algorithm presets (classic DX7 configurations)
namespace FMAlgorithms {
    // Simple 2-operator configurations
    extern const std::vector<std::vector<float>> SIMPLE_FM;      // Op1 -> Op2 -> Out
    extern const std::vector<std::vector<float>> PARALLEL_FM;    // Op1,Op2 -> Out
    
    // 4-operator stacks
    extern const std::vector<std::vector<float>> STACK_4OP;      // 1->2->3->4->Out
    extern const std::vector<std::vector<float>> BELL_ALGORITHM; // Bell-like sounds
    extern const std::vector<std::vector<float>> BRASS_ALGORITHM;// Brass-like sounds
    
    // Parallel configurations  
    extern const std::vector<std::vector<float>> QUAD_PARALLEL;  // All ops -> Out
    extern const std::vector<std::vector<float>> DUAL_CARRIER;   // 1->3, 2->4 -> Out
    
    // Complex modulation
    extern const std::vector<std::vector<float>> FEEDBACK_LOOP;  // With feedback paths
    extern const std::vector<std::vector<float>> RING_MOD;       // Ring modulation style
}