#pragma once
#include "../synthesis/SynthEngine.h"
#include "../core/Types.h"

/**
 * Classic4OpFM Engine - Simplified 4-operator FM synthesis engine
 */
class Classic4OpFMEngine : public SynthEngine {
public:
    Classic4OpFMEngine();
    ~Classic4OpFMEngine();
    
    // Initialization
    bool initialize(float sampleRate);
    void shutdown();
    
    // SynthEngine interface implementation
    EngineType getType() const override { return EngineType::CLASSIC_4OP_FM; }
    const char* getName() const override { return "Classic4OpFM"; }
    const char* getDescription() const override { return "4-operator FM synthesis with 8 curated algorithms"; }
    
    // SynthEngine note methods
    void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f) override;
    void noteOff(uint8_t note) override;
    void setAftertouch(uint8_t note, float aftertouch) override;
    void allNotesOff() override;
    
    // SynthEngine parameter methods
    void setParameter(ParameterID param, float value) override;
    float getParameter(ParameterID param) const override;
    bool hasParameter(ParameterID param) const override;
    
    // SynthEngine audio processing
    void processAudio(EtherAudioBuffer& outputBuffer) override;
    
    // SynthEngine voice management
    size_t getActiveVoiceCount() const override;
    size_t getMaxVoiceCount() const override { return voices_.size(); }
    void setVoiceCount(size_t maxVoices) override;
    
    // SynthEngine preset methods
    void savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const override;
    bool loadPreset(const uint8_t* data, size_t size) override;
    
    // SynthEngine configuration
    void setSampleRate(float sampleRate) override;
    void setBufferSize(size_t bufferSize) override;
    float getCPUUsage() const override;
    
private:
    // Core state
    float sampleRate_;
    bool initialized_;
    
    // HTM parameters
    float harmonics_ = 0.5f;   // Global FM index
    float timbre_ = 0.5f;      // Algorithm selection
    float morph_ = 0.5f;       // Feedback amount
    
    // Performance
    float cpuUsage_;
    
    // One voice of 4‑op FM
    struct Voice {
        bool active = false;
        uint8_t note = 0;
        float velocity = 0.8f;
        // phases
        float p1=0, p2=0, p3=0, p4=0;
        float lpfState = 0.0f;
        float lastOp1 = 0.0f; // feedback
        float freq = 440.0f;
        // envelope
        enum class EnvStage { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE } stage = EnvStage::IDLE;
        float env = 0.0f;
        uint32_t age = 0; // for stealing
    };

    // Voices
    std::vector<Voice> voices_;
    size_t maxVoices_ = 6;
    
    // Global params
    float volume_ = 0.8f;
    float pan_ = 0.5f;
    float brightness_ = 0.7f; // 0 dark .. 1 bright
    
    // Envelope params (global applied to all voices)
    float envAttack_ = 0.005f;   // fast attack
    float envDecay_ = 0.08f;     // short decay
    float envSustain_ = 0.6f;    // sustain level
    float envRelease_ = 0.08f;   // short release

    // Helpers
    float processVoiceSample(Voice& v);
    void advanceEnvelope(Voice& v);
    Voice* findFreeVoice();
    Voice* stealVoice();
};
