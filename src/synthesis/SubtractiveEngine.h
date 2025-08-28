#pragma once
#include "SynthEngine.h"
#include <array>
#include <memory>

/**
 * Simple subtractive synthesis engine for testing
 * Produces basic oscillator + filter + envelope sounds
 */
class SubtractiveEngine : public SynthEngine {
public:
    SubtractiveEngine();
    ~SubtractiveEngine() override;
    
    // SynthEngine interface
    EngineType getType() const override { return EngineType::SUBTRACTIVE; }
    const char* getName() const override { return "Subtractive"; }
    const char* getDescription() const override { return "Classic analog-style synthesis"; }
    
    void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f) override;
    void noteOff(uint8_t note) override;
    void setAftertouch(uint8_t note, float aftertouch) override;
    void allNotesOff() override;
    
    void setParameter(ParameterID param, float value) override;
    float getParameter(ParameterID param) const override;
    bool hasParameter(ParameterID param) const override;
    
    void processAudio(EtherAudioBuffer& outputBuffer) override;
    
    size_t getActiveVoiceCount() const override;
    size_t getMaxVoiceCount() const override { return MAX_VOICES; }
    void setVoiceCount(size_t maxVoices) override;
    
    float getCPUUsage() const override { return cpuUsage_; }
    
    void savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const override;
    bool loadPreset(const uint8_t* data, size_t size) override;
    
    void setSampleRate(float sampleRate) override;
    void setBufferSize(size_t bufferSize) override;
    
    bool supportsPolyAftertouch() const override { return true; }
    bool supportsModulation(ParameterID target) const override;
    void setModulation(ParameterID target, float amount) override;
    
private:
    // Simple voice for subtractive synthesis
    class SubtractiveVoice {
    public:
        SubtractiveVoice();
        
        void noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate);
        void noteOff();
        void setAftertouch(float aftertouch);
        
        AudioFrame processSample();
        
        bool isActive() const { return active_; }
        bool isReleasing() const { return envelope_.isReleasing(); }
        uint8_t getNote() const { return note_; }
        uint32_t getAge() const { return age_; }
        
        // Parameter control
        void setFilterCutoff(float cutoff);
        void setFilterResonance(float resonance);
        void setOscMix(float mix);
        void setVolume(float volume);
        void setEnvelopeParams(float attack, float decay, float sustain, float release);
        
    private:
        // Simple oscillator
        struct Oscillator {
            float phase = 0.0f;
            float frequency = 440.0f;
            float increment = 0.0f;
            
            void setFrequency(float freq, float sampleRate) {
                frequency = freq;
                increment = 2.0f * M_PI * frequency / sampleRate;
            }
            
            float processSaw() {
                float output = (phase / M_PI) - 1.0f;
                phase += increment;
                if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
                return output;
            }
            
            float processSine() {
                float output = std::sin(phase);
                phase += increment;
                if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
                return output;
            }
        };
        
        // Simple ADSR envelope
        struct Envelope {
            enum class Stage { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };
            
            Stage stage = Stage::IDLE;
            float level = 0.0f;
            float attack = 0.01f;
            float decay = 0.3f;
            float sustain = 0.8f;
            float release = 0.5f;
            float sampleRate = 48000.0f;
            
            void noteOn() {
                stage = Stage::ATTACK;
            }
            
            void noteOff() {
                if (stage != Stage::IDLE) {
                    stage = Stage::RELEASE;
                }
            }
            
            bool isReleasing() const {
                return stage == Stage::RELEASE;
            }
            
            bool isActive() const {
                return stage != Stage::IDLE;
            }
            
            float process() {
                const float attackRate = 1.0f / (attack * sampleRate);
                const float decayRate = 1.0f / (decay * sampleRate);
                const float releaseRate = 1.0f / (release * sampleRate);
                
                switch (stage) {
                    case Stage::IDLE:
                        return 0.0f;
                        
                    case Stage::ATTACK:
                        level += attackRate;
                        if (level >= 1.0f) {
                            level = 1.0f;
                            stage = Stage::DECAY;
                        }
                        break;
                        
                    case Stage::DECAY:
                        level -= decayRate;
                        if (level <= sustain) {
                            level = sustain;
                            stage = Stage::SUSTAIN;
                        }
                        break;
                        
                    case Stage::SUSTAIN:
                        level = sustain;
                        break;
                        
                    case Stage::RELEASE:
                        level -= releaseRate;
                        if (level <= 0.0f) {
                            level = 0.0f;
                            stage = Stage::IDLE;
                        }
                        break;
                }
                
                return level;
            }
        };
        
        // Simple low-pass filter
        struct Filter {
            float cutoff = 1000.0f;
            float resonance = 1.0f;
            float x1 = 0.0f, x2 = 0.0f;
            float y1 = 0.0f, y2 = 0.0f;
            float a0, a1, a2, b1, b2;
            float sampleRate = 48000.0f;
            
            void updateCoefficients() {
                float omega = 2.0f * M_PI * cutoff / sampleRate;
                float sinOmega = std::sin(omega);
                float cosOmega = std::cos(omega);
                float alpha = sinOmega / (2.0f * resonance);
                
                float b0 = 1.0f + alpha;
                a0 = (1.0f - cosOmega) / 2.0f / b0;
                a1 = (1.0f - cosOmega) / b0;
                a2 = (1.0f - cosOmega) / 2.0f / b0;
                b1 = -2.0f * cosOmega / b0;
                b2 = (1.0f - alpha) / b0;
            }
            
            void setCutoff(float freq) {
                cutoff = std::clamp(freq, 20.0f, sampleRate * 0.45f);
                updateCoefficients();
            }
            
            void setResonance(float q) {
                resonance = std::clamp(q, 0.1f, 10.0f);
                updateCoefficients();
            }
            
            float process(float input) {
                float output = a0 * input + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
                
                x2 = x1;
                x1 = input;
                y2 = y1;
                y1 = output;
                
                return output;
            }
        };
        
        bool active_ = false;
        uint8_t note_ = 60;
        float velocity_ = 0.8f;
        float aftertouch_ = 0.0f;
        uint32_t age_ = 0;
        
        Oscillator osc1_, osc2_;
        Filter filter_;
        Envelope envelope_;
        
        float oscMix_ = 0.5f;
        float volume_ = 0.8f;
        float noteFrequency_ = 440.0f;
    };
    
    // Voice management
    std::array<SubtractiveVoice, MAX_VOICES> voices_;
    uint32_t voiceCounter_ = 0;
    
    SubtractiveVoice* findFreeVoice();
    SubtractiveVoice* findVoice(uint8_t note);
    SubtractiveVoice* stealVoice();
    
    // Parameters
    float filterCutoff_ = 1000.0f;
    float filterResonance_ = 1.0f;
    float oscMix_ = 0.5f;
    float volume_ = 0.8f;
    float attack_ = 0.01f;
    float decay_ = 0.3f;
    float sustain_ = 0.8f;
    float release_ = 0.5f;
    
    // Performance monitoring
    float cpuUsage_ = 0.0f;
    void updateCPUUsage(float processingTime);
    
    // Modulation
    std::array<float, static_cast<size_t>(ParameterID::COUNT)> modulation_;
    
    // Parameter application
    void updateAllVoices();
};