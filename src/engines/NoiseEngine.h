#pragma once
#include "../synthesis/SynthEngine.h"
#include <array>
#include <memory>

/**
 * Noise - Granular and Texture Synthesis Engine with H/T/M Mapping
 * 
 * HARMONICS: grain density + size (sparse large grains → dense small grains)
 * TIMBRE: scatter + jitter (position scatter + temporal jitter)
 * MORPH: source + randomness (source material blend + chaos level)
 * 
 * Features:
 * - Granular synthesis with multiple source types
 * - Real-time grain density and size control
 * - Position and temporal scatter for texture
 * - Multiple noise sources and randomization
 * - Envelope shaping per grain
 */
class NoiseEngine : public SynthEngine {
public:
    NoiseEngine();
    ~NoiseEngine() override;
    
    // SynthEngine interface
    EngineType getType() const override { return EngineType::NOISE_PARTICLES; }
    const char* getName() const override { return "Noise"; }
    const char* getDescription() const override { return "Granular and texture synthesis with H/T/M control"; }
    
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
    void setHarmonics(float harmonics);    // 0-1: grain density + size
    void setTimbre(float timbre);          // 0-1: scatter + jitter
    void setMorph(float morph);            // 0-1: source + randomness

private:
    // Granular synthesis parameters
    struct GranularParams {
        float density = 20.0f;         // Grains per second
        float grainSize = 0.1f;        // Grain duration in seconds
        float scatter = 0.0f;          // Position scatter (0-1)
        float jitter = 0.0f;           // Temporal jitter (0-1)
        float randomness = 0.0f;       // Chaos level (0-1)
        
        void calculateFromHarmonics(float harmonics);
        void calculateFromTimbre(float timbre);
        void calculateFromMorph(float morph);
    };
    
    // Noise source types
    struct NoiseSource {
        enum class Type {
            WHITE,          // White noise
            PINK,           // Pink noise (1/f)
            BROWN,          // Brown noise (1/f²)
            BLUE,           // Blue noise (f)
            VELVET,         // Velvet noise (sparse impulses)
            CRACKLE         // Crackle noise (chaotic)
        };
        
        Type currentType = Type::WHITE;
        float blend = 0.0f;            // Blend between two sources
        
        void calculateFromMorph(float morph);
        float generateSample(uint32_t& seed) const;
        
    private:
        float generateWhite(uint32_t& seed) const;
        float generatePink(uint32_t& seed) const;
        float generateBrown(uint32_t& seed) const;
        float generateBlue(uint32_t& seed) const;
        float generateVelvet(uint32_t& seed) const;
        float generateCrackle(uint32_t& seed) const;
    };
    
    // Individual grain
    struct Grain {
        bool active = false;
        float phase = 0.0f;            // 0-1 through grain
        float duration = 0.1f;         // Grain duration in seconds
        float amplitude = 1.0f;        // Grain amplitude
        float pitch = 1.0f;            // Pitch multiplier
        float pan = 0.5f;              // Stereo position
        
        // Grain envelope (Hann window)
        float getEnvelope() const {
            if (phase >= 1.0f) return 0.0f;
            return 0.5f * (1.0f - std::cos(2.0f * M_PI * phase));
        }
        
        void trigger(float dur, float amp, float pitch_mult, float pan_pos) {
            active = true;
            phase = 0.0f;
            duration = dur;
            amplitude = amp;
            pitch = pitch_mult;
            pan = pan_pos;
        }
        
        bool update(float deltaTime) {
            if (!active) return false;
            
            phase += deltaTime / duration;
            if (phase >= 1.0f) {
                active = false;
                return false;
            }
            return true;
        }
    };
    
    // Noise Voice implementation
    class NoiseVoice {
    public:
        NoiseVoice();
        
        void noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate);
        void noteOff();
        void setAftertouch(float aftertouch);
        
        AudioFrame processSample();
        
        bool isActive() const { return active_; }
        bool isReleasing() const { return envelope_.isReleasing(); }
        uint8_t getNote() const { return note_; }
        uint32_t getAge() const { return age_; }
        
        // Parameter control
        void setGranularParams(const GranularParams& params);
        void setNoiseSource(const NoiseSource& source);
        void setVolume(float volume);
        void setEnvelopeParams(float attack, float decay, float sustain, float release);
        
    private:
        // Grain scheduler
        struct GrainScheduler {
            float nextGrainTime = 0.0f;
            float grainTimer = 0.0f;
            
            bool shouldTriggerGrain(float deltaTime, float density, float jitter, uint32_t& seed) {
                grainTimer += deltaTime;
                
                if (grainTimer >= nextGrainTime) {
                    // Calculate next grain time with jitter
                    float baseInterval = 1.0f / density;
                    float jitterAmount = jitter * baseInterval * 0.5f;
                    float jitterOffset = (randomFloat(seed) - 0.5f) * jitterAmount;
                    
                    nextGrainTime = baseInterval + jitterOffset;
                    grainTimer = 0.0f;
                    return true;
                }
                return false;
            }
            
        private:
            float randomFloat(uint32_t& seed) const {
                seed = seed * 1664525 + 1013904223;
                return static_cast<float>(seed) / 4294967296.0f;
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
        
        // Granular synthesis components
        static constexpr int MAX_GRAINS = 32;
        std::array<Grain, MAX_GRAINS> grains_;
        GrainScheduler scheduler_;
        uint32_t randomSeed_;
        
        // Voice parameters
        float volume_ = 0.8f;
        float noteFrequency_ = 440.0f;
        float sampleRate_ = 48000.0f;
        
        // Current granular and noise settings
        GranularParams granularParams_;
        NoiseSource noiseSource_;
        Envelope envelope_;
        
        // Helper methods
        void triggerGrain();
        float generateNoiseSample();
        float randomFloat();
    };
    
    // Voice management
    std::array<NoiseVoice, MAX_VOICES> voices_;
    uint32_t voiceCounter_ = 0;
    
    NoiseVoice* findFreeVoice();
    NoiseVoice* findVoice(uint8_t note);
    NoiseVoice* stealVoice();
    
    // H/T/M Parameters
    float harmonics_ = 0.5f;     // grain density + size
    float timbre_ = 0.3f;        // scatter + jitter
    float morph_ = 0.0f;         // source + randomness
    
    // Derived parameter systems
    GranularParams granularParams_;
    NoiseSource noiseSource_;
    
    // Additional parameters
    float volume_ = 0.8f;
    float attack_ = 0.01f;
    float decay_ = 0.1f;
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
    float mapGrainDensity(float harmonics) const;
    float mapGrainSize(float harmonics) const;
    float mapScatter(float timbre) const;
    float mapJitter(float timbre) const;
    NoiseSource::Type mapNoiseType(float morph, float& blend) const;
    float mapRandomness(float morph) const;
};