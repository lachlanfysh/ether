#pragma once
#include "../synthesis/SynthEngine.h"
#include <array>
#include <memory>

/**
 * MacroFM - 2-Operator FM Engine with H/T/M Mapping
 * 
 * HARMONICS: FM index 0-0.8 (expo); small bright tilt (+1.5 dB @ 2 kHz max)
 * TIMBRE: mod ratio set {0.5, 1.0, 1.5, 2.0, 3.0}; sine↔triangle
 * MORPH: feedback 0-0.3 + mod-env decay 30 → 6 ms (linked)
 * 
 * Features:
 * - Classic 2-operator FM with curated ratio set
 * - Exponential FM index mapping for musical response
 * - Linked feedback and modulator envelope decay
 * - Optional sub anchor for bass programs
 * - Bright tilt compensation for harsh high frequencies
 */
class MacroFMEngine : public SynthEngine {
public:
    MacroFMEngine();
    ~MacroFMEngine() override;
    
    // SynthEngine interface
    EngineType getType() const override { return EngineType::MACRO_FM; }
    const char* getName() const override { return "MacroFM"; }
    const char* getDescription() const override { return "2-Operator FM with H/T/M control"; }
    
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
    
    // H/T/M Macro Controls
    void setHarmonics(float harmonics);    // 0-1: FM index + bright tilt
    void setTimbre(float timbre);          // 0-1: mod ratio + sine↔triangle
    void setMorph(float morph);            // 0-1: feedback + mod-env decay
    
private:
    // FM Voice implementation
    class MacroFMVoice {
    public:
        MacroFMVoice();
        
        void noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate);
        void noteOff();
        void setAftertouch(float aftertouch);
        
        AudioFrame processSample();
        
        bool isActive() const { return active_; }
        bool isReleasing() const { return carrierEnv_.isReleasing(); }
        uint8_t getNote() const { return note_; }
        uint32_t getAge() const { return age_; }
        
        // Parameter control
        void setFMParams(float index, float brightTilt);
        void setModParams(float ratio, float sineTriBlend);
        void setFeedbackParams(float feedback, float modEnvDecay);
        void setSubParams(float subLevel);
        void setVolume(float volume);
        void setEnvelopeParams(float attack, float decay, float sustain, float release);
        
    private:
        // FM Operator
        struct FMOperator {
            float phase = 0.0f;
            float frequency = 440.0f;
            float increment = 0.0f;
            float output = 0.0f;
            float feedback = 0.0f;
            float feedbackAmount = 0.0f;
            
            void setFrequency(float freq, float sampleRate) {
                frequency = freq;
                increment = 2.0f * M_PI * freq / sampleRate;
            }
            
            void setFeedback(float fb) {
                feedbackAmount = fb;
            }
            
            float processSine(float modulation = 0.0f) {
                float input = phase + modulation + (output * feedbackAmount);
                output = std::sin(input);
                phase += increment;
                if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
                return output;
            }
            
            float processTriangle(float modulation = 0.0f) {
                float input = phase + modulation + (output * feedbackAmount);
                // Triangle wave approximation
                float triPhase = std::fmod(input + M_PI, 2.0f * M_PI) - M_PI;
                output = (4.0f / M_PI) * std::abs(triPhase) - 1.0f;
                phase += increment;
                if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
                return output;
            }
        };
        
        // Sub oscillator
        struct SubOscillator {
            float phase = 0.0f;
            float frequency = 220.0f;
            float increment = 0.0f;
            
            void setFrequency(float freq, float sampleRate) {
                frequency = freq * 0.5f; // -1 octave
                increment = 2.0f * M_PI * frequency / sampleRate;
            }
            
            float processSine() {
                float output = std::sin(phase);
                phase += increment;
                if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
                return output;
            }
        };
        
        // ADSR envelope
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
            
            float process();
        };
        
        // Bright tilt filter
        struct BrightTiltFilter {
            float gain = 0.0f; // +1.5 dB @ 2kHz max
            float freq = 2000.0f;
            float x1 = 0.0f, y1 = 0.0f;
            float a0, a1, b1;
            float sampleRate = 48000.0f;
            
            void setBrightTilt(float tiltDb);
            void updateCoefficients();
            float process(float input);
        };
        
        bool active_ = false;
        uint8_t note_ = 60;
        float velocity_ = 0.8f;
        float aftertouch_ = 0.0f;
        uint32_t age_ = 0;
        
        FMOperator carrier_;
        FMOperator modulator_;
        SubOscillator subOsc_;
        BrightTiltFilter brightFilter_;
        Envelope carrierEnv_;
        Envelope modEnv_;
        
        // Voice parameters
        float fmIndex_ = 0.0f;
        float brightTilt_ = 0.0f;
        float modRatio_ = 1.0f;
        float sineTriBlend_ = 0.0f; // 0=sine, 1=triangle
        float feedback_ = 0.0f;
        float modEnvDecay_ = 0.03f; // 30ms default
        float subLevel_ = 0.0f;
        float volume_ = 0.8f;
        float noteFrequency_ = 440.0f;
        
        // Curated ratio set
        static constexpr float RATIO_SET[5] = {0.5f, 1.0f, 1.5f, 2.0f, 3.0f};
        int currentRatioIndex_ = 1; // Start at 1.0
    };
    
    // Voice management
    std::array<MacroFMVoice, MAX_VOICES> voices_;
    uint32_t voiceCounter_ = 0;
    
    MacroFMVoice* findFreeVoice();
    MacroFMVoice* findVoice(uint8_t note);
    MacroFMVoice* stealVoice();
    
    // H/T/M Parameters
    float harmonics_ = 0.0f;     // FM index + bright tilt
    float timbre_ = 0.5f;        // mod ratio + sine↔triangle
    float morph_ = 0.0f;         // feedback + mod-env decay
    
    // Derived parameters from H/T/M
    float fmIndex_ = 0.0f;
    float brightTilt_ = 0.0f;
    float modRatio_ = 1.0f;
    float sineTriBlend_ = 0.0f;
    float feedback_ = 0.0f;
    float modEnvDecay_ = 0.03f;
    
    // Additional parameters
    float subLevel_ = 0.0f;      // -12 → -6 dB
    bool subAnchorEnabled_ = false;
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
    
    // Parameter calculation and voice updates
    void calculateDerivedParams();
    void updateAllVoices();
    
    // Mapping functions
    float mapFMIndexExp(float harmonics) const;
    float mapBrightTilt(float harmonics) const;
    float mapModRatio(float timbre) const;
    float mapSineTriBlend(float timbre) const;
    float mapFeedback(float morph) const;
    float mapModEnvDecay(float morph) const;
};