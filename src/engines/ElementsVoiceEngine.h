#pragma once
#include "../synthesis/SynthEngine.h"
#include <array>
#include <memory>

/**
 * ElementsVoice - Advanced Physical Modeling Engine with H/T/M Mapping
 * 
 * HARMONICS: exciter tone (color frequency, bow pressure, mallet sharpness, blow turbulence)
 * TIMBRE: resonator (string↔membrane balance, geometry, modal structure, material)
 * MORPH: balance + space (exciter energy, damping decay, stereo space, coupling)
 * 
 * Features:
 * - Multi-mode physical modeling (string, membrane, modal synthesis)
 * - Variable exciter types (bow, mallet, blow, pluck) with continuous morphing
 * - Geometry control for harmonic/inharmonic modal relationships
 * - Advanced damping simulation with frequency-dependent decay
 * - Stereo space processing for realistic physical placement
 */
class ElementsVoiceEngine : public SynthEngine {
public:
    ElementsVoiceEngine();
    ~ElementsVoiceEngine() override;
    
    // SynthEngine interface
    EngineType getType() const override { return EngineType::ELEMENTS_VOICE; }
    const char* getName() const override { return "ElementsVoice"; }
    const char* getDescription() const override { return "Advanced physical modeling with H/T/M control"; }
    
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
    void setHarmonics(float harmonics);    // 0-1: exciter tone
    void setTimbre(float timbre);          // 0-1: resonator
    void setMorph(float morph);            // 0-1: balance + space

private:
    // Exciter tone system
    struct ExciterTone {
        enum class Type {
            BOW,      // Continuous sawtooth excitation
            MALLET,   // Sharp percussive attack
            BLOW,     // Noise-based wind excitation
            PLUCK     // Quick string pluck
        };
        
        Type type = Type::MALLET;
        float color = 0.5f;           // Frequency/spectral content
        float pressure = 0.5f;        // Bow pressure or blow intensity
        float sharpness = 0.5f;       // Mallet hardness or attack sharpness
        float turbulence = 0.3f;      // Blow turbulence or bow noise
        
        void calculateFromHarmonics(float harmonics);
        float generateExcitation(float velocity, float phase, uint32_t& seed) const;
        
    private:
        float generateBowExcitation(float velocity, float phase) const;
        float generateMalletExcitation(float velocity, float phase) const;
        float generateBlowExcitation(float velocity, uint32_t& seed) const;
        float generatePluckExcitation(float velocity, float phase) const;
    };
    
    // Resonator system
    struct ResonatorSystem {
        enum class ModelType {
            STRING,      // Karplus-Strong string model
            MEMBRANE,    // 2D membrane with modal synthesis
            HYBRID       // Blend of both models
        };
        
        ModelType model = ModelType::HYBRID;
        float stringBalance = 0.5f;    // String↔membrane balance
        float geometry = 0.5f;         // Harmonic/inharmonic structure
        float modalSpread = 1.0f;      // Modal frequency relationships
        float materialStiffness = 0.5f; // String stiffness or membrane tension
        
        void calculateFromTimbre(float timbre);
    };
    
    // Balance and space system
    struct BalanceSpace {
        float exciterEnergy = 0.7f;    // Overall excitation strength
        float dampingAmount = 0.3f;    // Global damping factor
        float dampingDecay = 2.0f;     // Damping time constant
        float stereoSpace = 0.0f;      // Stereo width/positioning
        float coupling = 0.2f;         // Inter-resonator coupling
        
        void calculateFromMorph(float morph);
    };
    
    // Physical Voice implementation
    class ElementsVoiceImpl {
    public:
        ElementsVoiceImpl();
        
        void noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate);
        void noteOff();
        void setAftertouch(float aftertouch);
        
        AudioFrame processSample();
        
        bool isActive() const { return active_; }
        bool isReleasing() const { return envelope_.isReleasing(); }
        uint8_t getNote() const { return note_; }
        uint32_t getAge() const { return age_; }
        
        // Parameter control
        void setExciterTone(const ExciterTone& tone);
        void setResonatorSystem(const ResonatorSystem& system);
        void setBalanceSpace(const BalanceSpace& balance);
        void setVolume(float volume);
        void setEnvelopeParams(float attack, float decay, float sustain, float release);
        
    private:
        // Karplus-Strong string model
        struct StringModel {
            static constexpr int MAX_DELAY = 2048;
            
            std::array<float, MAX_DELAY> delayLine;
            int writePos = 0;
            float delayLength = 100.0f;
            float damping = 0.01f;
            float stiffness = 0.0f;
            
            // State for filters
            float dampingState = 0.0f;
            float allpassState = 0.0f;
            
            void setFrequency(float freq, float sampleRate);
            void setDamping(float damp) { damping = damp; }
            void setStiffness(float stiff) { stiffness = stiff; }
            float process(float excitation);
            void pluck(float energy, uint32_t& seed);
            void reset();
            
        private:
            void writeDelay(float sample);
            float readDelay(float delay) const;
        };
        
        // Modal membrane model
        struct MembraneModel {
            static constexpr int NUM_MODES = 8;
            
            struct Mode {
                float frequency = 220.0f;
                float amplitude = 0.0f;
                float phase = 0.0f;
                float damping = 0.01f;
                
                void reset() { phase = 0.0f; amplitude = 0.0f; }
            };
            
            std::array<Mode, NUM_MODES> modes;
            float geometry = 0.5f;
            float energy = 0.5f;
            float damping = 0.01f;
            float sampleRate = 48000.0f;
            
            void setGeometry(float geom);
            void setEnergy(float eng) { energy = eng; }
            void setDamping(float damp);
            void setFrequency(float freq);
            float process(float excitation);
            void strike(float energy);
            void reset();
            
        private:
            void updateModes();
        };
        
        // Stereo space processor
        struct SpaceProcessor {
            float width = 0.0f;
            float position = 0.0f;
            
            // Simple delay lines for space simulation
            std::array<float, 64> leftDelay;
            std::array<float, 64> rightDelay;
            int delayPos = 0;
            
            void setSpace(float space);
            AudioFrame process(float input);
        };
        
        // ADSR envelope for physical modeling
        struct Envelope {
            enum class Stage { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };
            
            Stage stage = Stage::IDLE;
            float level = 0.0f;
            float attack = 0.01f;
            float decay = 0.3f;
            float sustain = 0.8f;
            float release = 1.0f;  // Longer release for physical modeling
            float sampleRate = 48000.0f;
            
            void noteOn() { stage = Stage::ATTACK; }
            void noteOff() { if (stage != Stage::IDLE) stage = Stage::RELEASE; }
            bool isReleasing() const { return stage == Stage::RELEASE; }
            bool isActive() const { return stage != Stage::IDLE; }
            float process();
        };
        
        bool active_ = false;
        uint8_t note_ = 60;
        float velocity_ = 0.8f;
        float aftertouch_ = 0.0f;
        uint32_t age_ = 0;
        float excitationPhase_ = 0.0f;
        uint32_t randomSeed_ = 12345;
        
        // Physical modeling components
        StringModel stringModel_;
        MembraneModel membraneModel_;
        SpaceProcessor spaceProcessor_;
        Envelope envelope_;
        
        // Voice parameters
        float volume_ = 0.8f;
        float noteFrequency_ = 440.0f;
        float sampleRate_ = 48000.0f;
        
        // Current parameter settings
        ExciterTone exciterTone_;
        ResonatorSystem resonatorSystem_;
        BalanceSpace balanceSpace_;
        
        // Helper methods
        float generateNoise();
    };
    
    // Voice management
    std::array<ElementsVoiceImpl, MAX_VOICES> voices_;
    uint32_t voiceCounter_ = 0;
    
    ElementsVoiceImpl* findFreeVoice();
    ElementsVoiceImpl* findVoice(uint8_t note);
    ElementsVoiceImpl* stealVoice();
    
    // H/T/M Parameters
    float harmonics_ = 0.5f;     // exciter tone
    float timbre_ = 0.5f;        // resonator
    float morph_ = 0.5f;         // balance + space
    
    // Derived parameter systems
    ExciterTone exciterTone_;
    ResonatorSystem resonatorSystem_;
    BalanceSpace balanceSpace_;
    
    // Additional parameters
    float volume_ = 0.8f;
    float attack_ = 0.01f;
    float decay_ = 0.3f;
    float sustain_ = 0.8f;
    float release_ = 1.0f;  // Longer for physical modeling
    
    // Performance monitoring
    float cpuUsage_ = 0.0f;
    void updateCPUUsage(float processingTime);
    
    // Modulation
    std::array<float, static_cast<size_t>(ParameterID::COUNT)> modulation_;
    
    // Parameter calculation and voice updates
    void calculateDerivedParams();
    void updateAllVoices();
    
    // Mapping functions
    ExciterTone::Type mapExciterType(float harmonics, float& blend) const;
    float mapExciterColor(float harmonics) const;
    float mapExciterPressure(float harmonics) const;
    float mapExciterSharpness(float harmonics) const;
    ResonatorSystem::ModelType mapResonatorModel(float timbre, float& blend) const;
    float mapStringBalance(float timbre) const;
    float mapGeometry(float timbre) const;
    float mapModalSpread(float timbre) const;
    float mapExciterEnergy(float morph) const;
    float mapDampingAmount(float morph) const;
    float mapStereoSpace(float morph) const;
    float mapCoupling(float morph) const;
};