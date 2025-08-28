#pragma once
#include "../synthesis/SynthEngine.h"
#include <array>
#include <memory>

/**
 * Formant - Vocal Synthesis Engine with H/T/M Mapping
 * 
 * HARMONICS: formant frequency + Q control (F1/F2/F3 position and sharpness)
 * TIMBRE: vowel morphing (A → E → I → O → U interpolation)
 * MORPH: breath + consonant simulation (noise + transients)
 * 
 * Features:
 * - 3-formant vocal synthesis with resonant filters
 * - Vowel morphing between 5 cardinal vowels
 * - Breath noise and consonant simulation
 * - Real-time formant frequency and Q control
 * - Pitch-independent vocal character
 */
class FormantEngine : public SynthEngine {
public:
    FormantEngine();
    ~FormantEngine() override;
    
    // SynthEngine interface
    EngineType getType() const override { return EngineType::FORMANT_VOCAL; }
    const char* getName() const override { return "Formant"; }
    const char* getDescription() const override { return "Vocal synthesis with H/T/M control"; }
    
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
    void setHarmonics(float harmonics);    // 0-1: formant frequency + Q
    void setTimbre(float timbre);          // 0-1: vowel morphing A→E→I→O→U
    void setMorph(float morph);            // 0-1: breath + consonant

private:
    // Vowel formant database
    struct VowelFormants {
        enum class Vowel { A, E, I, O, U };
        
        struct FormantData {
            float f1, f2, f3;      // Formant frequencies (Hz)
            float a1, a2, a3;      // Formant amplitudes (0-1)
            float q1, q2, q3;      // Formant Q factors
        };
        
        // Cardinal vowel formants (average adult male)
        static constexpr FormantData VOWEL_DATA[5] = {
            {730, 1090, 2440, 1.0f, 0.8f, 0.6f, 8.0f, 12.0f, 15.0f},  // A
            {270, 2290, 3010, 1.0f, 0.9f, 0.5f, 6.0f, 15.0f, 20.0f},  // E
            {390, 1990, 2550, 1.0f, 0.7f, 0.4f, 10.0f, 18.0f, 25.0f}, // I
            {570, 840, 2410, 1.0f, 0.6f, 0.3f, 9.0f, 10.0f, 18.0f},   // O
            {440, 1020, 2240, 1.0f, 0.5f, 0.2f, 7.0f, 8.0f, 12.0f}    // U
        };
        
        FormantData currentFormants;
        
        void morphToVowel(float timbre);
        void applyHarmonicsControl(float harmonics);
        
    private:
        FormantData interpolateVowels(const FormantData& a, const FormantData& b, float t) const;
    };
    
    // Vocal tract modeling
    struct VocalTract {
        float breathNoise = 0.0f;      // Background breath noise level
        float consonantMix = 0.0f;     // Consonant simulation level
        float fricativeFreq = 4000.0f; // Fricative center frequency
        float voicing = 0.8f;          // Voiced/unvoiced balance
        
        void calculateFromMorph(float morph);
    };
    
    // Formant Voice implementation
    class FormantVoice {
    public:
        FormantVoice();
        
        void noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate);
        void noteOff();
        void setAftertouch(float aftertouch);
        
        AudioFrame processSample();
        
        bool isActive() const { return active_; }
        bool isReleasing() const { return envelope_.isReleasing(); }
        uint8_t getNote() const { return note_; }
        uint32_t getAge() const { return age_; }
        
        // Parameter control
        void setFormantParams(const VowelFormants& vowelFormants);
        void setVocalTractParams(const VocalTract& vocalTract);
        void setVolume(float volume);
        void setEnvelopeParams(float attack, float decay, float sustain, float release);
        
    private:
        // Pulse generator for vocal excitation
        struct PulseGenerator {
            float phase = 0.0f;
            float frequency = 100.0f;  // Fundamental frequency
            float increment = 0.0f;
            float pulseWidth = 0.1f;   // Glottal pulse width
            float shimmer = 0.02f;     // Frequency variation
            
            void setFrequency(float freq, float sampleRate) {
                frequency = freq;
                increment = freq / sampleRate;
            }
            
            float processPulse() {
                float output = 0.0f;
                
                // Generate glottal pulse
                if (phase < pulseWidth) {
                    float pulsePhase = phase / pulseWidth;
                    // Rosenberg glottal pulse approximation
                    output = 0.5f * (1.0f - std::cos(2.0f * M_PI * pulsePhase));
                    output *= std::exp(-3.0f * pulsePhase); // Decay
                }
                
                // Add shimmer (natural frequency variation)
                float jitter = (std::sin(phase * 137.5f) * 0.5f + 0.5f) * shimmer;
                float actualIncrement = increment * (1.0f + jitter);
                
                phase += actualIncrement;
                if (phase >= 1.0f) phase -= 1.0f;
                
                return output;
            }
        };
        
        // Formant filter (resonant bandpass)
        struct FormantFilter {
            float frequency = 1000.0f;
            float q = 10.0f;
            float amplitude = 1.0f;
            
            // Biquad coefficients
            float a0, a1, a2, b1, b2;
            float x1 = 0.0f, x2 = 0.0f;
            float y1 = 0.0f, y2 = 0.0f;
            float sampleRate = 48000.0f;
            
            void setParams(float freq, float Q, float amp) {
                frequency = std::clamp(freq, 50.0f, sampleRate * 0.45f);
                q = std::clamp(Q, 0.5f, 50.0f);
                amplitude = std::clamp(amp, 0.0f, 2.0f);
                updateCoefficients();
            }
            
            void updateCoefficients() {
                float omega = 2.0f * M_PI * frequency / sampleRate;
                float alpha = std::sin(omega) / (2.0f * q);
                float cosOmega = std::cos(omega);
                
                float b0 = 1.0f + alpha;
                a0 = alpha / b0;
                a1 = 0.0f;
                a2 = -alpha / b0;
                b1 = -2.0f * cosOmega / b0;
                b2 = (1.0f - alpha) / b0;
            }
            
            float process(float input) {
                float output = a0 * input + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
                output *= amplitude;
                
                x2 = x1;
                x1 = input;
                y2 = y1;
                y1 = output;
                
                return output;
            }
        };
        
        // Noise generator for breath and consonants
        struct NoiseGenerator {
            uint32_t seed = 12345;
            float level = 0.0f;
            float fricativeFreq = 4000.0f;
            
            // High-pass filter for fricatives
            float hpx1 = 0.0f, hpy1 = 0.0f;
            float hpCutoff = 2000.0f;
            float sampleRate = 48000.0f;
            
            void setLevel(float lvl) {
                level = std::clamp(lvl, 0.0f, 1.0f);
            }
            
            void setFricativeFreq(float freq) {
                fricativeFreq = freq;
                hpCutoff = freq * 0.5f;
            }
            
            float processNoise() {
                // Simple linear congruential generator
                seed = seed * 1664525 + 1013904223;
                float noise = (static_cast<float>(seed) / 4294967296.0f) - 0.5f;
                noise *= level;
                
                // High-pass filter for fricative character
                float alpha = 0.9f; // Simple HPF
                float filtered = noise - hpx1 + alpha * hpy1;
                hpx1 = noise;
                hpy1 = filtered;
                
                return filtered * 2.0f; // Boost filtered noise
            }
        };
        
        // ADSR envelope
        struct Envelope {
            enum class Stage { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };
            
            Stage stage = Stage::IDLE;
            float level = 0.0f;
            float attack = 0.01f;
            float decay = 0.1f;
            float sustain = 0.8f;
            float release = 0.3f;
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
        
        bool active_ = false;
        uint8_t note_ = 60;
        float velocity_ = 0.8f;
        float aftertouch_ = 0.0f;
        uint32_t age_ = 0;
        
        // Vocal synthesis components
        PulseGenerator pulseGen_;
        std::array<FormantFilter, 3> formantFilters_; // F1, F2, F3
        NoiseGenerator noiseGen_;
        Envelope envelope_;
        
        // Voice parameters
        float volume_ = 0.8f;
        float noteFrequency_ = 440.0f;
        
        // Current formant and vocal tract settings
        VowelFormants vowelFormants_;
        VocalTract vocalTract_;
    };
    
    // Voice management
    std::array<FormantVoice, MAX_VOICES> voices_;
    uint32_t voiceCounter_ = 0;
    
    FormantVoice* findFreeVoice();
    FormantVoice* findVoice(uint8_t note);
    FormantVoice* stealVoice();
    
    // H/T/M Parameters
    float harmonics_ = 0.5f;     // formant frequency + Q
    float timbre_ = 0.0f;        // vowel morphing A→E→I→O→U
    float morph_ = 0.0f;         // breath + consonant
    
    // Derived parameter systems
    VowelFormants vowelFormants_;
    VocalTract vocalTract_;
    
    // Additional parameters
    float volume_ = 0.8f;
    float attack_ = 0.01f;
    float decay_ = 0.1f;
    float sustain_ = 0.8f;
    float release_ = 0.3f;
    
    // Performance monitoring
    float cpuUsage_ = 0.0f;
    void updateCPUUsage(float processingTime);
    
    // Modulation
    std::array<float, static_cast<size_t>(ParameterID::COUNT)> modulation_;
    
    // Parameter calculation and voice updates
    void calculateDerivedParams();
    void updateAllVoices();
    
    // Mapping functions
    float mapFormantFreqShift(float harmonics) const;
    float mapFormantQ(float harmonics) const;
    VowelFormants::Vowel mapVowelPosition(float timbre, float& blend) const;
    float mapBreathNoise(float morph) const;
    float mapConsonantMix(float morph) const;
};