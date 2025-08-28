#pragma once
#include "../synthesis/SynthEngine.h"
#include <array>
#include <memory>

/**
 * TidesOsc - Complex Oscillator Engine with H/T/M Mapping
 * 
 * HARMONICS: slope steepness (smooth sine → steep ramp → sharp pulse trains)
 * TIMBRE: frequency ratio + material (harmonic/inharmonic + sonic character)
 * MORPH: amplitude balance + damping (oscillator blend + decay simulation)
 * 
 * Features:
 * - Variable slope oscillator with continuous waveform morphing
 * - Harmonic and inharmonic frequency ratios
 * - Material simulation (wood, metal, glass, string characteristics)
 * - Amplitude balance between multiple oscillators
 * - Damping simulation for physical modeling aspects
 */
class TidesOscEngine : public SynthEngine {
public:
    TidesOscEngine();
    ~TidesOscEngine() override;
    
    // SynthEngine interface
    EngineType getType() const override { return EngineType::TIDES_OSC; }
    const char* getName() const override { return "TidesOsc"; }
    const char* getDescription() const override { return "Complex oscillator with H/T/M control"; }
    
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
    void setHarmonics(float harmonics);    // 0-1: slope steepness
    void setTimbre(float timbre);          // 0-1: frequency ratio + material
    void setMorph(float morph);            // 0-1: amplitude balance + damping

private:
    // Complex oscillator parameters
    struct ComplexOscParams {
        float slopeRise = 0.5f;        // Rise slope steepness (0=sine, 1=sharp)
        float slopeFall = 0.5f;        // Fall slope steepness  
        float symmetry = 0.5f;         // Rise/fall time ratio
        float fold = 0.0f;             // Waveform folding amount
        
        void calculateFromHarmonics(float harmonics);
    };
    
    // Frequency ratio and material system
    struct FrequencyMaterial {
        enum class MaterialType {
            WOOD,       // Warm, rich harmonics with decay
            METAL,      // Bright, metallic with inharmonic content
            GLASS,      // Pure, crystalline with high Q resonance
            STRING,     // Complex harmonic series with beating
            MEMBRANE,   // Drum-like with pitch bending
            AIR         // Breathy, turbulent characteristics
        };
        
        MaterialType material = MaterialType::WOOD;
        float ratio = 1.0f;            // Frequency ratio (0.25 to 4.0)
        bool isHarmonic = true;        // True=harmonic, false=inharmonic
        float materialAmount = 0.5f;   // How much material character to apply
        
        void calculateFromTimbre(float timbre);
        float getMaterialModulation(float phase, MaterialType mat) const;
        
    private:
        // Material-specific harmonic content
        std::array<float, 8> getHarmonicContent(MaterialType mat) const;
    };
    
    // Amplitude and damping system
    struct AmplitudeDamping {
        float oscillatorBalance = 0.5f; // Balance between dual oscillators
        float damping = 0.0f;           // Exponential decay simulation
        float dampingRate = 1.0f;       // How fast damping occurs
        float sustainLevel = 1.0f;      // Level after damping plateau
        
        void calculateFromMorph(float morph);
    };
    
    // Complex Voice implementation
    class TidesOscVoice {
    public:
        TidesOscVoice();
        
        void noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate);
        void noteOff();
        void setAftertouch(float aftertouch);
        
        AudioFrame processSample();
        
        bool isActive() const { return active_; }
        bool isReleasing() const { return envelope_.isReleasing(); }
        uint8_t getNote() const { return note_; }
        uint32_t getAge() const { return age_; }
        
        // Parameter control
        void setComplexOscParams(const ComplexOscParams& params);
        void setFrequencyMaterial(const FrequencyMaterial& material);
        void setAmplitudeDamping(const AmplitudeDamping& damping);
        void setVolume(float volume);
        void setEnvelopeParams(float attack, float decay, float sustain, float release);
        
    private:
        // Variable slope oscillator
        struct SlopeOscillator {
            float phase = 0.0f;
            float frequency = 440.0f;
            float increment = 0.0f;
            float slopeRise = 0.5f;
            float slopeFall = 0.5f;
            float symmetry = 0.5f;
            float fold = 0.0f;
            
            void setFrequency(float freq, float sampleRate) {
                frequency = freq;
                increment = freq / sampleRate;
            }
            
            void setSlopes(float rise, float fall, float sym, float fld) {
                slopeRise = std::clamp(rise, 0.01f, 0.99f);
                slopeFall = std::clamp(fall, 0.01f, 0.99f);
                symmetry = std::clamp(sym, 0.1f, 0.9f);
                fold = std::clamp(fld, 0.0f, 1.0f);
            }
            
            float process() {
                float output = generateSlopeWave(phase);
                
                // Apply folding
                if (fold > 0.0f) {
                    output = applyFolding(output, fold);
                }
                
                // Update phase
                phase += increment;
                if (phase >= 1.0f) phase -= 1.0f;
                
                return output;
            }
            
        private:
            float generateSlopeWave(float ph) const {
                float splitPoint = symmetry;
                
                if (ph < splitPoint) {
                    // Rising portion
                    float localPhase = ph / splitPoint;
                    return applySlopeShaping(localPhase, slopeRise) * 2.0f - 1.0f;
                } else {
                    // Falling portion
                    float localPhase = (ph - splitPoint) / (1.0f - splitPoint);
                    return (1.0f - applySlopeShaping(localPhase, slopeFall)) * 2.0f - 1.0f;
                }
            }
            
            float applySlopeShaping(float x, float slope) const {
                // Variable slope using power function
                if (slope < 0.5f) {
                    // Concave (smooth)
                    float power = 0.1f + (0.5f - slope) * 3.8f; // 0.1 to 2.0
                    return std::pow(x, power);
                } else {
                    // Convex (sharp)
                    float power = 0.1f + (slope - 0.5f) * 3.8f; // 0.1 to 2.0
                    return 1.0f - std::pow(1.0f - x, power);
                }
            }
            
            float applyFolding(float input, float amount) const {
                float folded = input;
                float foldAmount = amount * 2.0f; // 0 to 2
                
                if (std::abs(folded) > 1.0f - foldAmount) {
                    float excess = std::abs(folded) - (1.0f - foldAmount);
                    folded = (folded > 0.0f ? 1.0f : -1.0f) * (1.0f - excess);
                }
                
                return folded;
            }
        };
        
        // Material filter for sonic character
        struct MaterialFilter {
            FrequencyMaterial::MaterialType type = FrequencyMaterial::MaterialType::WOOD;
            float amount = 0.5f;
            
            // Simple state variable filter
            float low = 0.0f, band = 0.0f, high = 0.0f;
            float f = 0.1f, q = 0.5f;
            float sampleRate = 48000.0f;
            
            void setMaterial(FrequencyMaterial::MaterialType mat, float amt) {
                type = mat;
                amount = std::clamp(amt, 0.0f, 1.0f);
                updateFilterParams();
            }
            
            float process(float input, float fundamentalFreq) {
                if (amount < 0.01f) return input;
                
                // Apply material-specific filtering
                float filtered = processFilter(input);
                float materialMod = getMaterialModulation(fundamentalFreq);
                
                return input * (1.0f - amount) + filtered * materialMod * amount;
            }
            
        private:
            void updateFilterParams() {
                switch (type) {
                    case FrequencyMaterial::MaterialType::WOOD:
                        f = 0.05f; q = 0.7f; // Warm, resonant
                        break;
                    case FrequencyMaterial::MaterialType::METAL:
                        f = 0.2f; q = 0.9f; // Bright, ringy
                        break;
                    case FrequencyMaterial::MaterialType::GLASS:
                        f = 0.3f; q = 0.95f; // Crystalline, pure
                        break;
                    case FrequencyMaterial::MaterialType::STRING:
                        f = 0.1f; q = 0.8f; // Harmonic richness
                        break;
                    case FrequencyMaterial::MaterialType::MEMBRANE:
                        f = 0.03f; q = 0.6f; // Punchy, percussive
                        break;
                    case FrequencyMaterial::MaterialType::AIR:
                        f = 0.4f; q = 0.3f; // Breathy, open
                        break;
                }
            }
            
            float processFilter(float input) {
                low += f * band;
                high = input - low - q * band;
                band += f * high;
                
                // Return different filter outputs based on material
                switch (type) {
                    case FrequencyMaterial::MaterialType::WOOD:
                    case FrequencyMaterial::MaterialType::MEMBRANE:
                        return low; // Low-pass character
                    case FrequencyMaterial::MaterialType::METAL:
                    case FrequencyMaterial::MaterialType::GLASS:
                        return high + band * 0.5f; // High-pass + bandpass
                    case FrequencyMaterial::MaterialType::STRING:
                        return band; // Bandpass emphasis
                    case FrequencyMaterial::MaterialType::AIR:
                        return high; // High-pass
                    default:
                        return input;
                }
            }
            
            float getMaterialModulation(float freq) const {
                // Add subtle material-specific modulation
                static float modPhase = 0.0f;
                modPhase += 0.001f; // Slow modulation
                
                switch (type) {
                    case FrequencyMaterial::MaterialType::METAL:
                        return 1.0f + 0.1f * std::sin(modPhase * 3.7f); // Metallic shimmer
                    case FrequencyMaterial::MaterialType::STRING:
                        return 1.0f + 0.05f * std::sin(modPhase * 1.3f); // String beating
                    default:
                        return 1.0f;
                }
            }
        };
        
        // Damping envelope for physical modeling
        struct DampingEnvelope {
            float damping = 0.0f;
            float dampingRate = 1.0f;
            float sustainLevel = 1.0f;
            float currentLevel = 1.0f;
            bool triggered = false;
            
            void setParams(float damp, float rate, float sustain) {
                damping = std::clamp(damp, 0.0f, 1.0f);
                dampingRate = std::clamp(rate, 0.1f, 10.0f);
                sustainLevel = std::clamp(sustain, 0.0f, 1.0f);
            }
            
            void trigger() {
                triggered = true;
                currentLevel = 1.0f;
            }
            
            float process(float sampleRate) {
                if (!triggered || damping < 0.01f) {
                    return 1.0f;
                }
                
                // Exponential decay to sustain level
                float targetLevel = sustainLevel;
                float decayRate = dampingRate / sampleRate;
                
                currentLevel += (targetLevel - currentLevel) * decayRate * damping;
                
                return currentLevel;
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
        
        // Dual complex oscillators
        SlopeOscillator oscA_, oscB_;
        MaterialFilter materialFilter_;
        DampingEnvelope dampingEnv_;
        Envelope envelope_;
        
        // Voice parameters
        float volume_ = 0.8f;
        float noteFrequency_ = 440.0f;
        
        // Current parameter settings
        ComplexOscParams complexParams_;
        FrequencyMaterial freqMaterial_;
        AmplitudeDamping ampDamping_;
    };
    
    // Voice management
    std::array<TidesOscVoice, MAX_VOICES> voices_;
    uint32_t voiceCounter_ = 0;
    
    TidesOscVoice* findFreeVoice();
    TidesOscVoice* findVoice(uint8_t note);
    TidesOscVoice* stealVoice();
    
    // H/T/M Parameters
    float harmonics_ = 0.5f;     // slope steepness
    float timbre_ = 0.3f;        // frequency ratio + material
    float morph_ = 0.5f;         // amplitude balance + damping
    
    // Derived parameter systems
    ComplexOscParams complexParams_;
    FrequencyMaterial freqMaterial_;
    AmplitudeDamping ampDamping_;
    
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
    float mapSlopeRise(float harmonics) const;
    float mapSlopeFall(float harmonics) const;
    float mapSymmetry(float harmonics) const;
    float mapFold(float harmonics) const;
    FrequencyMaterial::MaterialType mapMaterial(float timbre, float& blend) const;
    float mapFrequencyRatio(float timbre) const;
    float mapOscillatorBalance(float morph) const;
    float mapDamping(float morph) const;
};