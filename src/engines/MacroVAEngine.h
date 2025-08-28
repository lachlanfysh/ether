#pragma once
#include "../synthesis/SynthEngine.h"
#include "../synthesis/SharedEngineComponents.h"
#include <array>
#include <memory>

/**
 * MacroVA - Virtual Analog Engine with H/T/M Mapping
 * 
 * HARMONICS: LPF cutoff (exp map) + small auto-Q
 * TIMBRE: saw↔pulse blend; when pulse, PWM around 50% (safe range 45-55%)
 * MORPH: sub/noise blend (sub −12 → 0 dB; noise −∞ → −18 dB) + gentle high-tilt (±2 dB @ 4 kHz)
 * 
 * Features:
 * - Classic subtractive synthesis with modern enhancements
 * - Auto-Q that follows cutoff for musical response
 * - Safe PWM range to prevent extreme timbres
 * - Sub oscillator and noise for fullness
 * - High-frequency tilt for air and presence
 */
class MacroVAEngine : public SynthEngine {
public:
    MacroVAEngine();
    ~MacroVAEngine() override;
    
    // SynthEngine interface
    EngineType getType() const override { return EngineType::MACRO_VA; }
    const char* getName() const override { return "MacroVA"; }
    const char* getDescription() const override { return "Virtual Analog with H/T/M control"; }
    
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
    
    float getCPUUsage() const override { return cpuTracker_ ? cpuTracker_->getCPUUsage() : 0.0f; }
    
    void savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const override;
    bool loadPreset(const uint8_t* data, size_t size) override;
    
    void setSampleRate(float sampleRate) override;
    void setBufferSize(size_t bufferSize) override;
    
    bool supportsPolyAftertouch() const override { return true; }
    bool supportsModulation(ParameterID target) const override;
    void setModulation(ParameterID target, float amount) override;
    
    // H/T/M Macro Controls
    void setHarmonics(float harmonics);    // 0-1: LPF cutoff + auto-Q
    void setTimbre(float timbre);          // 0-1: saw↔pulse blend + PWM
    void setMorph(float morph);            // 0-1: sub/noise blend + high-tilt
    
private:
    // Enhanced voice for MacroVA
    class MacroVAVoice {
    public:
        MacroVAVoice();
        
        void noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate);
        void noteOff();
        void setAftertouch(float aftertouch);
        
        AudioFrame processSample();
        
        bool isActive() const { return voiceState_.isActive(); }
        bool isReleasing() const { return voiceState_.isReleasing(); }
        uint8_t getNote() const { return static_cast<uint8_t>(voiceState_.noteNumber); }
        uint32_t getAge() const { return voiceState_.getAge(0); } // Age calculation needs current time
        
        // Parameter control
        void setFilterParams(float cutoff, float autoQ);
        void setOscParams(float sawPulseBlend, float pwm);
        void setSubNoiseParams(float subLevel, float noiseLevel);
        void setHighTilt(float tiltAmount);
        void setVolume(float volume);
        void setEnvelopeParams(float attack, float decay, float sustain, float release);
        
    private:
        // Enhanced oscillator with PWM
        struct VAOscillator {
            float phase = 0.0f;
            float frequency = 440.0f;
            float increment = 0.0f;
            float pwm = 0.5f; // Pulse width modulation
            
            void setFrequency(float freq, float sampleRate) {
                frequency = freq;
                increment = freq / sampleRate;
            }
            
            void setPWM(float width) {
                pwm = std::clamp(width, 0.01f, 0.99f);
            }
            
            float processSaw() {
                float output = 2.0f * phase - 1.0f;
                phase += increment;
                if (phase >= 1.0f) phase -= 1.0f;
                return output;
            }
            
            float processPulse() {
                float output = (phase < pwm) ? 1.0f : -1.0f;
                phase += increment;
                if (phase >= 1.0f) phase -= 1.0f;
                return output;
            }
        };
        
        // Sub oscillator (-1 octave)
        struct SubOscillator {
            float phase = 0.0f;
            float frequency = 220.0f;
            float increment = 0.0f;
            
            void setFrequency(float freq, float sampleRate) {
                frequency = freq * 0.5f; // -1 octave
                increment = frequency / sampleRate;
            }
            
            float processSine() {
                float output = std::sin(2.0f * M_PI * phase);
                phase += increment;
                if (phase >= 1.0f) phase -= 1.0f;
                return output;
            }
        };
        
        // Noise generator
        struct NoiseGenerator {
            uint32_t seed = 12345;
            
            float processWhite() {
                seed = seed * 1664525 + 1013904223; // Linear congruential generator
                return (static_cast<float>(seed) / static_cast<float>(UINT32_MAX)) * 2.0f - 1.0f;
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
        
        // Enhanced filter with auto-Q
        struct VAFilter {
            float cutoff = 1000.0f;
            float baseResonance = 1.0f;
            float autoQ = 0.0f;
            float x1 = 0.0f, x2 = 0.0f;
            float y1 = 0.0f, y2 = 0.0f;
            float a0, a1, a2, b1, b2;
            float sampleRate = 48000.0f;
            
            void updateCoefficients();
            void setCutoff(float freq);
            void setAutoQ(float q);
            float process(float input);
        };
        
        // High-frequency tilt filter
        struct TiltFilter {
            float gain = 0.0f; // ±2 dB @ 4kHz
            float freq = 4000.0f;
            float x1 = 0.0f, y1 = 0.0f;
            float a0, a1, b1;
            float sampleRate = 48000.0f;
            
            void setTilt(float tiltDb);
            void updateCoefficients();
            float process(float input);
        };
        
        // Use shared voice state (eliminates duplicate code)
        EtherSynth::VoiceState voiceState_;
        
        VAOscillator mainOsc_;
        SubOscillator subOsc_;
        NoiseGenerator noise_;
        VAFilter filter_;
        TiltFilter tiltFilter_;
        
        // Use shared components (eliminates duplicate envelope code)
        std::unique_ptr<EtherSynth::StandardADSR> envelope_;
        
        // Voice parameters
        float sawPulseBlend_ = 0.0f; // 0=saw, 1=pulse
        float pwm_ = 0.5f;
        float subLevel_ = 0.0f;      // -12 to 0 dB
        float noiseLevel_ = 0.0f;    // -∞ to -18 dB
        float highTilt_ = 0.0f;      // ±2 dB @ 4kHz
        float volume_ = 0.8f;
        float noteFrequency_ = 440.0f;
    };
    
    // Voice management
    std::array<MacroVAVoice, MAX_VOICES> voices_;
    uint32_t voiceCounter_ = 0;
    
    MacroVAVoice* findFreeVoice();
    MacroVAVoice* findVoice(uint8_t note);
    MacroVAVoice* stealVoice();
    
    // H/T/M Parameters
    float harmonics_ = 0.5f;    // LPF cutoff + auto-Q
    float timbre_ = 0.0f;       // saw↔pulse blend + PWM
    float morph_ = 0.0f;        // sub/noise blend + high-tilt
    
    // Derived parameters from H/T/M
    float filterCutoff_ = 1000.0f;
    float filterAutoQ_ = 0.0f;
    float sawPulseBlend_ = 0.0f;
    float pwm_ = 0.5f;
    float subLevel_ = 0.0f;
    float noiseLevel_ = 0.0f;
    float highTilt_ = 0.0f;
    
    // Additional parameters
    float volume_ = 0.8f;
    float attack_ = 0.01f;
    float decay_ = 0.3f;
    float sustain_ = 0.8f;
    float release_ = 0.5f;
    
    // Shared components (eliminates duplicate code)
    std::unique_ptr<EtherSynth::ParameterManager> parameterManager_;
    std::unique_ptr<EtherSynth::CPUUsageTracker> cpuTracker_;
    
    // Parameter calculation and voice updates
    void calculateDerivedParams();
    void updateAllVoices();
    
    // Mapping functions
    float mapCutoffExp(float harmonics) const;
    float mapAutoQ(float harmonics) const;
    float mapPWM(float timbre) const;
    float mapSubLevel(float morph) const;
    float mapNoiseLevel(float morph) const;
    float mapHighTilt(float morph) const;
};