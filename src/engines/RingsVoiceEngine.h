#pragma once
#include "../synthesis/SynthEngine.h"
#include <array>
#include <memory>

/**
 * RingsVoice - Physical Modeling Engine with H/T/M Mapping
 * 
 * HARMONICS: resonator frequency + Q (formant position and sharpness)
 * TIMBRE: material properties (stiffness, density, damping characteristics)
 * MORPH: exciter balance (bow/blow/strike blend and intensity)
 * 
 * Features:
 * - Multi-mode resonator with variable frequency and Q
 * - Physical material simulation (wood, metal, glass, string, etc.)
 * - Multiple exciter types (bow, blow, strike) with crossfading
 * - Resonator coupling and feedback for complex interactions
 * - Real-time parameter morphing for expressive control
 */
class RingsVoiceEngine : public SynthEngine {
public:
    RingsVoiceEngine();
    ~RingsVoiceEngine() override;
    
    // SynthEngine interface
    EngineType getType() const override { return EngineType::RINGS_VOICE; }
    const char* getName() const override { return "RingsVoice"; }
    const char* getDescription() const override { return "Physical modeling with H/T/M control"; }
    
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
    void setHarmonics(float harmonics);    // 0-1: resonator frequency + Q
    void setTimbre(float timbre);          // 0-1: material properties
    void setMorph(float morph);            // 0-1: exciter balance

private:
    // Resonator system
    struct ResonatorParams {
        float frequency = 440.0f;     // Base resonant frequency
        float q = 10.0f;              // Quality factor (sharpness)
        float harmonicSpread = 1.0f;  // Harmonic series deviation
        float coupling = 0.3f;        // Inter-resonator coupling
        
        void calculateFromHarmonics(float harmonics, float noteFreq);
    };
    
    // Material properties
    struct MaterialProps {
        enum class MaterialType {
            WOOD,       // Warm, organic damping
            METAL,      // Bright, sustained resonance
            GLASS,      // Pure, crystalline overtones
            STRING,     // Flexible, rich harmonics
            MEMBRANE,   // Percussive, quick decay
            CRYSTAL     // Ethereal, long sustain
        };
        
        MaterialType type = MaterialType::WOOD;
        float stiffness = 0.5f;       // Material stiffness (0=soft, 1=rigid)
        float density = 0.5f;         // Material density affecting frequency
        float damping = 0.3f;         // Internal damping factor
        float nonlinearity = 0.1f;    // Nonlinear behavior amount
        
        void calculateFromTimbre(float timbre);
        float getDampingForFreq(float freq) const;
        float getStiffnessModulation(float input) const;
        
    private:
        std::array<float, 6> getMaterialDampingCurve(MaterialType mat) const;
    };
    
    // Exciter system
    struct ExciterSystem {
        enum class ExciterType {
            BOW,        // Bowed string character
            BLOW,       // Wind instrument character  
            STRIKE      // Struck/plucked character
        };
        
        float bowAmount = 0.33f;      // Bow exciter level
        float blowAmount = 0.33f;     // Blow exciter level
        float strikeAmount = 0.34f;   // Strike exciter level
        float intensity = 0.5f;       // Overall excitation intensity
        
        void calculateFromMorph(float morph);
        float generateExcitation(float velocity, float sampleRate) const;
        
    private:
        float generateBowExcitation(float velocity, float phase) const;
        float generateBlowExcitation(float velocity, float noise) const;
        float generateStrikeExcitation(float velocity, float time) const;
    };
    
    // Physical Voice implementation
    class RingsVoiceImpl {
    public:
        RingsVoiceImpl();
        
        void noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate);
        void noteOff();
        void setAftertouch(float aftertouch);
        
        AudioFrame processSample();
        
        bool isActive() const { return active_; }
        bool isReleasing() const { return envelope_.isReleasing(); }
        uint8_t getNote() const { return note_; }
        uint32_t getAge() const { return age_; }
        
        // Parameter control
        void setResonatorParams(const ResonatorParams& params);
        void setMaterialProps(const MaterialProps& props);
        void setExciterSystem(const ExciterSystem& system);
        void setVolume(float volume);
        void setEnvelopeParams(float attack, float decay, float sustain, float release);
        
    private:
        // Multi-mode resonator filter
        struct Resonator {
            float frequency = 440.0f;
            float q = 10.0f;
            float amplitude = 1.0f;
            
            // State variable filter with feedback
            float low = 0.0f, band = 0.0f, high = 0.0f;
            float f = 0.1f, qFactor = 0.5f;
            float feedback = 0.0f;
            float sampleRate = 48000.0f;
            
            void setParams(float freq, float Q, float amp) {
                frequency = std::clamp(freq, 20.0f, sampleRate * 0.45f);
                q = std::clamp(Q, 0.5f, 100.0f);
                amplitude = std::clamp(amp, 0.0f, 2.0f);
                updateCoefficients();
            }
            
            void updateCoefficients() {
                f = 2.0f * std::sin(M_PI * frequency / sampleRate);
                qFactor = 1.0f / q;
                feedback = q * 0.1f; // Controlled feedback for resonance
            }
            
            float process(float input, float damping = 1.0f) {
                // Add controlled feedback for resonance
                float feedbackInput = input + band * feedback * 0.01f;
                
                low += f * band;
                high = feedbackInput - low - qFactor * band;
                band += f * high;
                
                // Apply damping
                low *= damping;
                band *= damping;
                high *= damping;
                
                // Return resonant output (primarily bandpass)
                return band * amplitude;
            }
        };
        
        // Physical damping model
        struct DampingModel {
            MaterialProps::MaterialType material = MaterialProps::MaterialType::WOOD;
            float globalDamping = 0.95f;
            float frequencyDamping = 0.98f;
            float nonlinearDamping = 0.99f;
            
            void setMaterial(MaterialProps::MaterialType mat, float damping) {
                material = mat;
                calculateDampingFactors(damping);
            }
            
            float process(float input, float frequency) {
                // Frequency-dependent damping
                float freqDamping = getFrequencyDamping(frequency);
                
                // Nonlinear damping based on amplitude
                float amplitude = std::abs(input);
                float nonlinearDamp = 1.0f - (amplitude * nonlinearDamping * 0.1f);
                
                return input * globalDamping * freqDamping * nonlinearDamp;
            }
            
        private:
            void calculateDampingFactors(float damping) {
                globalDamping = 1.0f - damping * 0.3f;      // 0.7 to 1.0
                frequencyDamping = 1.0f - damping * 0.1f;   // 0.9 to 1.0
                nonlinearDamping = damping * 0.2f;          // 0.0 to 0.2
            }
            
            float getFrequencyDamping(float freq) const {
                // Higher frequencies damp more (physical behavior)
                float normalizedFreq = freq / 1000.0f; // Normalize around 1kHz
                return 1.0f - (normalizedFreq * 0.05f); // Slight high-freq damping
            }
        };
        
        // Coupling network between resonators
        struct CouplingNetwork {
            float couplingAmount = 0.3f;
            float lastOutput[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            
            void setCoupling(float amount) {
                couplingAmount = std::clamp(amount, 0.0f, 0.8f);
            }
            
            void process(std::array<float, 4>& resonatorOutputs) {
                if (couplingAmount < 0.01f) return;
                
                // Store current outputs
                for (int i = 0; i < 4; i++) {
                    lastOutput[i] = resonatorOutputs[i];
                }
                
                // Apply coupling (each resonator affects neighbors)
                for (int i = 0; i < 4; i++) {
                    float coupling = 0.0f;
                    
                    // Couple with adjacent resonators
                    if (i > 0) coupling += lastOutput[i-1] * 0.3f;
                    if (i < 3) coupling += lastOutput[i+1] * 0.3f;
                    
                    // Couple with all other resonators (weaker)
                    for (int j = 0; j < 4; j++) {
                        if (j != i) coupling += lastOutput[j] * 0.1f;
                    }
                    
                    resonatorOutputs[i] += coupling * couplingAmount * 0.1f;
                }
            }
        };
        
        // ADSR envelope with physical modeling characteristics
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
        float excitationTime_ = 0.0f;
        
        // Physical modeling components
        std::array<Resonator, 4> resonators_;  // Multiple resonators for rich timbre
        DampingModel dampingModel_;
        CouplingNetwork coupling_;
        Envelope envelope_;
        uint32_t noiseState_ = 12345;  // For breath noise generation
        
        // Voice parameters
        float volume_ = 0.8f;
        float noteFrequency_ = 440.0f;
        float sampleRate_ = 48000.0f;
        
        // Current parameter settings
        ResonatorParams resonatorParams_;
        MaterialProps materialProps_;
        ExciterSystem exciterSystem_;
        
        // Helper methods
        float generateNoise();
        float calculateResonatorFreq(int index, float baseFreq, float spread) const;
    };
    
    // Voice management
    std::array<RingsVoiceImpl, MAX_VOICES> voices_;
    uint32_t voiceCounter_ = 0;
    
    RingsVoiceImpl* findFreeVoice();
    RingsVoiceImpl* findVoice(uint8_t note);
    RingsVoiceImpl* stealVoice();
    
    // H/T/M Parameters
    float harmonics_ = 0.5f;     // resonator frequency + Q
    float timbre_ = 0.3f;        // material properties
    float morph_ = 0.5f;         // exciter balance
    
    // Derived parameter systems
    ResonatorParams resonatorParams_;
    MaterialProps materialProps_;
    ExciterSystem exciterSystem_;
    
    // Additional parameters
    float volume_ = 0.8f;
    float pan_ = 0.5f;
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
    float mapResonatorFreq(float harmonics) const;
    float mapResonatorQ(float harmonics) const;
    float mapHarmonicSpread(float harmonics) const;
    MaterialProps::MaterialType mapMaterial(float timbre, float& blend) const;
    float mapStiffness(float timbre) const;
    float mapDamping(float timbre) const;
    void mapExciterBalance(float morph, float& bow, float& blow, float& strike) const;
    float mapIntensity(float morph) const;
};
