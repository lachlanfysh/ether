#pragma once
#include "../synthesis/SynthEngine.h"
#include <array>
#include <memory>

/**
 * MacroWaveshaper - Waveshaping Engine with H/T/M Mapping
 * 
 * HARMONICS: drive 0-1 (expo) + asymmetry 0-0.4. Oversample 2-4× around shaper only
 * TIMBRE: pre-gain −6 → +12 dB; wavebank select; pre-emphasis ±2 dB @ 2 kHz
 * MORPH: post-LPF 500 Hz–8 kHz + post sat 0-0.2
 * 
 * Features:
 * - Waveshaping with multiple wavebank algorithms
 * - Oversampling islands for alias-free distortion
 * - Pre-emphasis EQ for tonal shaping
 * - Post-processing filter and saturation
 * - Asymmetry control for even harmonic content
 */
class MacroWaveshaperEngine : public SynthEngine {
public:
    MacroWaveshaperEngine();
    ~MacroWaveshaperEngine() override;
    
    // SynthEngine interface
    EngineType getType() const override { return EngineType::MACRO_WAVESHAPER; }
    const char* getName() const override { return "MacroWaveshaper"; }
    const char* getDescription() const override { return "Waveshaping with H/T/M control"; }
    
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
    void setHarmonics(float harmonics);    // 0-1: drive + asymmetry
    void setTimbre(float timbre);          // 0-1: pre-gain + wavebank + pre-emphasis
    void setMorph(float morph);            // 0-1: post-LPF + post saturation
    
private:
    // Waveshaper Voice implementation
    class MacroWaveshaperVoice {
    public:
        MacroWaveshaperVoice();
        
        void noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate);
        void noteOff();
        void setAftertouch(float aftertouch);
        
        AudioFrame processSample();
        
        bool isActive() const { return active_; }
        bool isReleasing() const { return envelope_.isReleasing(); }
        uint8_t getNote() const { return note_; }
        uint32_t getAge() const { return age_; }
        
        // Parameter control
        void setWaveshapeParams(float drive, float asymmetry);
        void setPreParams(float preGain, int wavebank, float preEmphasis);
        void setPostParams(float postCutoff, float postSat);
        void setVolume(float volume);
        void setEnvelopeParams(float attack, float decay, float sustain, float release);
        
    private:
        // Basic oscillator
        struct Oscillator {
            float phase = 0.0f;
            float frequency = 440.0f;
            float increment = 0.0f;
            
            void setFrequency(float freq, float sampleRate) {
                frequency = freq;
                increment = 2.0f * M_PI * freq / sampleRate;
            }
            
            float processSaw() {
                float output = (2.0f * phase / (2.0f * M_PI)) - 1.0f;
                phase += increment;
                if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
                return output;
            }
        };
        
        // Waveshaper with multiple algorithms
        struct Waveshaper {
            float drive = 0.0f;
            float asymmetry = 0.0f;
            int wavebank = 0;
            
            void setParams(float drv, float asym, int bank) {
                drive = drv;
                asymmetry = asym;
                wavebank = std::clamp(bank, 0, 3);
            }
            
            float process(float input) {
                // Apply asymmetry (DC offset before shaping)
                float offset = asymmetry * 0.5f;
                float asymInput = input + offset;
                
                // Apply drive
                float drivenInput = asymInput * (1.0f + drive * 9.0f); // 1x to 10x
                
                // Clip to prevent extreme values
                drivenInput = std::clamp(drivenInput, -3.0f, 3.0f);
                
                float shaped;
                switch (wavebank) {
                    case 0: // Soft clip (tanh)
                        shaped = std::tanh(drivenInput);
                        break;
                        
                    case 1: // Fold distortion
                        shaped = foldDistortion(drivenInput);
                        break;
                        
                    case 2: // Bit reduction
                        shaped = bitReduction(drivenInput);
                        break;
                        
                    case 3: // Sine shaper
                        shaped = sineShaper(drivenInput);
                        break;
                        
                    default:
                        shaped = drivenInput;
                        break;
                }
                
                // Remove DC offset from asymmetry
                return shaped - (offset * 0.8f);
            }
            
        private:
            float foldDistortion(float input) {
                const float threshold = 1.0f;
                if (std::abs(input) <= threshold) {
                    return input;
                }
                return threshold * 2.0f - std::abs(input);
            }
            
            float bitReduction(float input) {
                const float levels = 16.0f; // Simulate lower bit depth
                return std::round(input * levels) / levels;
            }
            
            float sineShaper(float input) {
                return std::sin(input * M_PI * 0.5f);
            }
        };
        
        // Oversampling processor
        struct OversamplingProcessor {
            static constexpr int OVERSAMPLE_FACTOR = 4;
            static constexpr int FILTER_TAPS = 8;
            
            float upsampleBuffer[OVERSAMPLE_FACTOR];
            float downsampleBuffer[FILTER_TAPS];
            int bufferIndex = 0;
            
            // Simple anti-aliasing filter coefficients (8-tap FIR)
            static constexpr float FILTER_COEFFS[FILTER_TAPS] = {
                -0.02f, 0.06f, -0.18f, 0.64f, 0.64f, -0.18f, 0.06f, -0.02f
            };
            
            float process(float input, Waveshaper& shaper) {
                // Upsample: insert zeros
                upsampleBuffer[0] = input * OVERSAMPLE_FACTOR;
                for (int i = 1; i < OVERSAMPLE_FACTOR; i++) {
                    upsampleBuffer[i] = 0.0f;
                }
                
                // Process each upsampled sample through waveshaper
                for (int i = 0; i < OVERSAMPLE_FACTOR; i++) {
                    upsampleBuffer[i] = shaper.process(upsampleBuffer[i]);
                }
                
                // Downsample with filtering
                float sum = 0.0f;
                for (int i = 0; i < FILTER_TAPS; i++) {
                    int index = (bufferIndex - i + FILTER_TAPS) % FILTER_TAPS;
                    sum += downsampleBuffer[index] * FILTER_COEFFS[i];
                }
                
                // Store current sample in circular buffer
                downsampleBuffer[bufferIndex] = upsampleBuffer[0];
                bufferIndex = (bufferIndex + 1) % FILTER_TAPS;
                
                return sum;
            }
        };
        
        // Pre-emphasis filter
        struct PreEmphasisFilter {
            float gain = 0.0f; // ±2 dB @ 2kHz
            float freq = 2000.0f;
            float x1 = 0.0f, y1 = 0.0f;
            float a0, a1, b1;
            float sampleRate = 48000.0f;
            
            void setPreEmphasis(float emphasisDb);
            void updateCoefficients();
            float process(float input);
        };
        
        // Post low-pass filter
        struct PostLPFilter {
            float cutoff = 4000.0f;
            float x1 = 0.0f, x2 = 0.0f;
            float y1 = 0.0f, y2 = 0.0f;
            float a0, a1, a2, b1, b2;
            float sampleRate = 48000.0f;
            
            void setCutoff(float freq);
            void updateCoefficients();
            float process(float input);
        };
        
        // Post saturation
        struct PostSaturator {
            float amount = 0.0f;
            
            void setAmount(float sat) {
                amount = std::clamp(sat, 0.0f, 0.2f);
            }
            
            float process(float input) {
                if (amount < 0.001f) return input;
                
                // Soft saturation
                float driven = input * (1.0f + amount * 4.0f);
                return std::tanh(driven) / (1.0f + amount);
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
        
        bool active_ = false;
        uint8_t note_ = 60;
        float velocity_ = 0.8f;
        float aftertouch_ = 0.0f;
        uint32_t age_ = 0;
        
        Oscillator osc_;
        Waveshaper waveshaper_;
        OversamplingProcessor oversampler_;
        PreEmphasisFilter preEmphasis_;
        PostLPFilter postFilter_;
        PostSaturator postSat_;
        Envelope envelope_;
        
        // Voice parameters
        float drive_ = 0.0f;
        float asymmetry_ = 0.0f;
        float preGain_ = 1.0f;      // Linear gain (0.5x to 4x)
        int wavebank_ = 0;
        float preEmphasisDb_ = 0.0f;
        float postCutoff_ = 4000.0f;
        float postSaturation_ = 0.0f;
        float volume_ = 0.8f;
        float noteFrequency_ = 440.0f;
    };
    
    // Voice management
    std::array<MacroWaveshaperVoice, MAX_VOICES> voices_;
    uint32_t voiceCounter_ = 0;
    
    MacroWaveshaperVoice* findFreeVoice();
    MacroWaveshaperVoice* findVoice(uint8_t note);
    MacroWaveshaperVoice* stealVoice();
    
    // H/T/M Parameters
    float harmonics_ = 0.0f;     // drive + asymmetry
    float timbre_ = 0.5f;        // pre-gain + wavebank + pre-emphasis
    float morph_ = 0.5f;         // post-LPF + post saturation
    
    // Derived parameters from H/T/M
    float drive_ = 0.0f;
    float asymmetry_ = 0.0f;
    float preGain_ = 1.0f;
    int wavebank_ = 0;
    float preEmphasisDb_ = 0.0f;
    float postCutoff_ = 4000.0f;
    float postSaturation_ = 0.0f;
    
    // Additional parameters
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
    float mapDriveExp(float harmonics) const;
    float mapAsymmetry(float harmonics) const;
    float mapPreGain(float timbre) const;
    int mapWavebank(float timbre) const;
    float mapPreEmphasis(float timbre) const;
    float mapPostCutoff(float morph) const;
    float mapPostSaturation(float morph) const;
};