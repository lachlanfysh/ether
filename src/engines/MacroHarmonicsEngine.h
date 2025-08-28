#pragma once
#include "../synthesis/SynthEngine.h"
#include <array>
#include <memory>

/**
 * MacroHarmonics - Additive Synthesis Engine with H/T/M Mapping
 * 
 * HARMONICS: odd/even balance + level scaler (1st-8th harmonic control)
 * TIMBRE: drawbar groups (foundation + principals + mixtures)
 * MORPH: leakage + decay (mechanical modeling of tonewheel organs)
 * 
 * Features:
 * - 8-harmonic additive synthesis with independent level control
 * - Organ-style drawbar groupings for musical control
 * - Tonewheel leakage and decay modeling
 * - Odd/even harmonic balance for timbre shaping
 * - Real-time harmonic level interpolation
 */
class MacroHarmonicsEngine : public SynthEngine {
public:
    MacroHarmonicsEngine();
    ~MacroHarmonicsEngine() override;
    
    // SynthEngine interface
    EngineType getType() const override { return EngineType::MACRO_HARMONICS; }
    const char* getName() const override { return "MacroHarmonics"; }
    const char* getDescription() const override { return "Additive synthesis with H/T/M control"; }
    
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
    void setHarmonics(float harmonics);    // 0-1: odd/even balance + level scaler
    void setTimbre(float timbre);          // 0-1: drawbar groups
    void setMorph(float morph);            // 0-1: leakage + decay

private:
    // Harmonic configuration system
    struct HarmonicSettings {
        static constexpr int NUM_HARMONICS = 8;
        
        // Individual harmonic levels (0-1)
        std::array<float, NUM_HARMONICS> levels = {0.8f, 0.6f, 0.4f, 0.3f, 0.2f, 0.15f, 0.1f, 0.05f};
        
        // Drawbar groupings (organ-style)
        struct DrawbarGroups {
            float foundation = 0.8f;   // 1st, 2nd harmonics
            float principals = 0.6f;   // 3rd, 4th, 5th harmonics  
            float mixtures = 0.3f;     // 6th, 7th, 8th harmonics
        };
        
        DrawbarGroups drawbars;
        float oddEvenBalance = 0.5f;   // 0=even emphasis, 1=odd emphasis
        float levelScaler = 1.0f;      // Overall harmonic level multiplier
        
        void calculateFromHarmonics(float harmonics);
        void calculateFromTimbre(float timbre);
        void updateHarmonicLevels();
    };
    
    // Tonewheel modeling system
    struct TonewheelModel {
        float leakage = 0.0f;          // Cross-harmonic bleeding (0-0.3)
        float decay = 0.0f;            // Harmonic decay rate (0-0.5)
        float crosstalk = 0.0f;        // Adjacent note interference
        
        // Leakage matrix for harmonic cross-bleeding
        std::array<std::array<float, 8>, 8> leakageMatrix;
        
        void calculateFromMorph(float morph);
        void updateLeakageMatrix();
        float applyLeakage(const std::array<float, 8>& harmonics, int targetHarmonic) const;
    };
    
    // Additive Voice implementation
    class MacroHarmonicsVoice {
    public:
        MacroHarmonicsVoice();
        
        void noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate);
        void noteOff();
        void setAftertouch(float aftertouch);
        
        AudioFrame processSample();
        
        bool isActive() const { return active_; }
        bool isReleasing() const { return envelope_.isReleasing(); }
        uint8_t getNote() const { return note_; }
        uint32_t getAge() const { return age_; }
        
        // Parameter control
        void setHarmonicParams(const HarmonicSettings& settings);
        void setTonewheelParams(const TonewheelModel& model);
        void setVolume(float volume);
        void setEnvelopeParams(float attack, float decay, float sustain, float release);
        
    private:
        // Individual harmonic oscillator
        struct HarmonicOscillator {
            float phase = 0.0f;
            float frequency = 440.0f;
            float increment = 0.0f;
            float level = 0.0f;
            float targetLevel = 0.0f;   // For smooth level transitions
            float leakageInput = 0.0f;  // Input from other harmonics
            
            void setFrequency(float freq, float sampleRate) {
                frequency = freq;
                increment = 2.0f * M_PI * freq / sampleRate;
            }
            
            void setTargetLevel(float lvl) {
                targetLevel = std::clamp(lvl, 0.0f, 1.0f);
            }
            
            void updateLevel(float smoothingRate = 0.01f) {
                // Smooth level changes
                level += (targetLevel - level) * smoothingRate;
            }
            
            float processSine() {
                float output = std::sin(phase) * level;
                phase += increment;
                if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
                return output;
            }
        };
        
        // Click-free envelope for organ-style sounds
        struct OrganEnvelope {
            enum class Stage { IDLE, ATTACK, SUSTAIN, RELEASE };
            
            Stage stage = Stage::IDLE;
            float level = 0.0f;
            float attack = 0.005f;     // Very fast attack for organ
            float decay = 0.0f;        // No decay for organ
            float sustain = 1.0f;      // Full sustain
            float release = 0.1f;      // Quick but smooth release
            float sampleRate = 48000.0f;
            
            void noteOn() {
                if (stage == Stage::IDLE) {
                    stage = Stage::ATTACK;
                } else {
                    // Re-trigger from current level
                    stage = Stage::ATTACK;
                }
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
        
        bool active_ = false;
        uint8_t note_ = 60;
        float velocity_ = 0.8f;
        float aftertouch_ = 0.0f;
        uint32_t age_ = 0;
        
        // 8 harmonic oscillators
        std::array<HarmonicOscillator, HarmonicSettings::NUM_HARMONICS> harmonics_;
        OrganEnvelope envelope_;
        
        // Voice parameters
        float volume_ = 0.8f;
        float noteFrequency_ = 440.0f;
        
        // Current harmonic and tonewheel settings
        HarmonicSettings harmonicSettings_;
        TonewheelModel tonewheelModel_;
    };
    
    // Voice management
    std::array<MacroHarmonicsVoice, MAX_VOICES> voices_;
    uint32_t voiceCounter_ = 0;
    
    MacroHarmonicsVoice* findFreeVoice();
    MacroHarmonicsVoice* findVoice(uint8_t note);
    MacroHarmonicsVoice* stealVoice();
    
    // H/T/M Parameters
    float harmonics_ = 0.5f;     // odd/even balance + level scaler
    float timbre_ = 0.5f;        // drawbar groups
    float morph_ = 0.0f;         // leakage + decay
    
    // Derived parameter systems
    HarmonicSettings harmonicSettings_;
    TonewheelModel tonewheelModel_;
    
    // Additional parameters
    float volume_ = 0.8f;
    float attack_ = 0.005f;      // Very fast for organ
    float decay_ = 0.0f;         // No decay for organ
    float sustain_ = 1.0f;       // Full sustain
    float release_ = 0.1f;       // Quick release
    
    // Performance monitoring
    float cpuUsage_ = 0.0f;
    void updateCPUUsage(float processingTime);
    
    // Modulation
    std::array<float, static_cast<size_t>(ParameterID::COUNT)> modulation_;
    
    // Parameter calculation and voice updates
    void calculateDerivedParams();
    void updateAllVoices();
    
    // Mapping functions
    float mapOddEvenBalance(float harmonics) const;
    float mapLevelScaler(float harmonics) const;
    HarmonicSettings::DrawbarGroups mapDrawbarGroups(float timbre) const;
    float mapLeakage(float morph) const;
    float mapDecay(float morph) const;
};