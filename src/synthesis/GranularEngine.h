#pragma once
#include "SynthEngine.h"
#include <vector>
#include <random>

/**
 * Real-time Granular Synthesis Engine
 * Creates textures from micro-sounds with extensive modulation
 */
class GranularEngine : public SynthEngine {
public:
    GranularEngine();
    ~GranularEngine() override;
    
    // Core interface
    void processAudio(EtherAudioBuffer& buffer) override;
    void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f) override;
    void noteOff(uint8_t note) override;
    void setAftertouch(uint8_t note, float aftertouch) override;
    void allNotesOff() override;
    
    // Parameter control
    void setParameter(ParameterID param, float value) override;
    float getParameter(ParameterID param) const override;
    bool hasParameter(ParameterID param) const override;
    
    // Granular-specific parameters
    void setGrainSize(float sizeMs);              // 1-500ms
    void setGrainDensity(float density);          // Grains per second
    void setGrainPitch(float pitch);              // Pitch shift ratio
    void setGrainSpread(float spread);            // Stereo spread
    void setGrainRandomness(float randomness);    // Chaos factor
    void setTextureMode(int mode);                // Different grain generation modes
    
    // Touch interface integration for granular exploration
    void setTouchPosition(float x, float y);      // X: density, Y: grain size
    
    // Engine info
    const char* getName() const override { return "Granular"; }
    const char* getDescription() const override { return "Real-time granular synthesis"; }
    EngineType getType() const override { return EngineType::GRANULAR; }
    
    // Voice management
    size_t getActiveVoiceCount() const override;
    size_t getMaxVoiceCount() const override { return MAX_VOICES; }
    void setVoiceCount(size_t maxVoices) override;
    
    // Performance monitoring
    float getCPUUsage() const override;
    
    // Preset management
    void savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const override;
    bool loadPreset(const uint8_t* data, size_t size) override;
    
    // Audio configuration
    void setSampleRate(float sampleRate) override;
    void setBufferSize(size_t bufferSize) override;

private:
    // Grain structure
    struct Grain {
        float* waveform = nullptr;    // Pointer to source waveform
        size_t waveformSize = 0;      // Size of source waveform
        float position = 0.0f;        // Current position in waveform
        float increment = 1.0f;       // Playback speed
        float amplitude = 0.0f;       // Current amplitude
        float pan = 0.5f;             // Stereo position
        
        // Envelope
        float envPhase = 0.0f;        // 0.0 to 1.0
        float envDuration = 0.1f;     // Grain duration in seconds
        bool active = false;
        
        float process();              // Process one sample
        void trigger(float* source, size_t sourceSize, float pitch, float amp, float panPos, float duration);
        void updateEnvelope(float deltaTime);
        bool isFinished() const { return envPhase >= 1.0f; }
    };
    
    // Granular voice
    struct GranularVoice {
        uint8_t note = 0;
        float velocity = 0.0f;
        float baseFrequency = 440.0f;
        bool active = false;
        uint32_t noteOnTime = 0;
        
        // Grain management
        static constexpr size_t MAX_GRAINS = 64;
        std::array<Grain, MAX_GRAINS> grains;
        
        // Grain spawning
        float grainSpawnTimer = 0.0f;
        float grainSpawnInterval = 0.1f;  // Time between grain spawns
        
        float process(GranularEngine* engine);
        void spawnGrain(GranularEngine* engine);
        Grain* findFreeGrain();
    };
    
    std::array<GranularVoice, MAX_VOICES> voices_;
    
    // Source waveforms for granular processing
    std::vector<std::vector<float>> sourceWaveforms_;
    int currentWaveform_ = 0;
    
    // Granular parameters
    float grainSize_ = 50.0f;         // Milliseconds
    float grainDensity_ = 20.0f;      // Grains per second
    float grainPitch_ = 1.0f;         // Pitch shift ratio
    float grainSpread_ = 0.5f;        // Stereo spread
    float grainRandomness_ = 0.2f;    // Randomization amount
    int textureMode_ = 0;             // Grain generation mode
    
    // Global parameters
    float volume_ = 0.8f;
    float attack_ = 0.01f;
    float decay_ = 0.3f;
    float sustain_ = 0.7f;
    float release_ = 0.5f;
    
    // Touch interface
    float touchX_ = 0.5f;
    float touchY_ = 0.5f;
    
    // Random number generation
    mutable std::mt19937 rng_;
    mutable std::uniform_real_distribution<float> uniformDist_;
    
    // Helper methods
    GranularVoice* findFreeVoice();
    void initializeSourceWaveforms();
    void generateWaveform(std::vector<float>& waveform, int type);
    float getRandomValue() const;
    float applyRandomness(float value, float randomness) const;
    
    // Grain envelope functions
    float grainEnvelope(float phase) const;  // Hann window by default
    
    // Performance monitoring
    mutable float cpuUsage_ = 0.0f;
    
    // Texture modes
    enum class TextureMode {
        FORWARD = 0,      // Normal playback
        REVERSE,          // Reverse playback
        PINGPONG,         // Forward then reverse
        RANDOM_JUMP,      // Random position jumps
        FREEZE,           // Freeze at current position
        STRETCH,          // Time stretching
        COUNT
    };
    
    // Waveform types for source material
    enum class WaveformType {
        SINE = 0,
        TRIANGLE,
        SAW,
        SQUARE,
        NOISE,
        HARMONIC_RICH,
        FORMANT,
        VOCAL,
        COUNT
    };
};