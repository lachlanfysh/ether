#pragma once
#include "SynthEngine.h"
#include <vector>

/**
 * Wavetable synthesis engine with morphing capabilities
 * Perfect for the NSynth-style touch interface
 */
class WavetableEngine : public SynthEngine {
public:
    WavetableEngine();
    ~WavetableEngine() override;
    
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
    
    // Wavetable-specific features
    void setWavetablePosition(float position); // 0.0-1.0 through wavetable
    void setMorphAmount(float amount);         // Cross-fade between tables
    void loadWavetable(const std::vector<float>& samples, size_t tableIndex);
    
    // Touch interface integration
    void setTouchPosition(float x, float y); // Maps to wavetable + morph
    
    // Engine info
    const char* getName() const override { return "Wavetable"; }
    const char* getDescription() const override { return "Morphing wavetable synthesizer"; }
    EngineType getType() const override { return EngineType::WAVETABLE; }
    
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
    // Wavetable data
    static constexpr size_t WAVETABLE_SIZE = 2048;
    static constexpr size_t NUM_WAVETABLES = 64;
    
    std::array<std::array<float, WAVETABLE_SIZE>, NUM_WAVETABLES> wavetables_;
    
    // Playback state
    struct WavetableVoice {
        uint8_t note = 0;
        float frequency = 440.0f;
        float phase = 0.0f;
        float velocity = 0.0f;
        float amplitude = 0.0f;
        float wavetablePos = 0.0f;
        float morphAmount = 0.0f;
        bool active = false;
        uint32_t noteOnTime = 0;
        
        // Envelope
        float envPhase = 0.0f;
        float envValue = 0.0f;
        bool envReleasing = false;
    };
    
    std::array<WavetableVoice, MAX_VOICES> voices_;
    
    // Engine parameters
    float wavetablePosition_ = 0.0f;  // Position through wavetable bank
    float morphAmount_ = 0.0f;        // Cross-fade between adjacent tables
    float detune_ = 0.0f;             // Fine tuning
    float volume_ = 0.8f;
    
    // ADSR envelope
    float attack_ = 0.01f;
    float decay_ = 0.3f;
    float sustain_ = 0.7f;
    float release_ = 0.5f;
    
    // Filter
    float filterCutoff_ = 0.8f;
    float filterResonance_ = 0.1f;
    
    // Touch morphing
    float touchX_ = 0.5f;
    float touchY_ = 0.5f;
    
    // Helper methods
    float interpolateWavetable(float phase, float tablePos, float morphAmount);
    float getWavetableSample(size_t tableIndex, float phase);
    void updateEnvelope(WavetableVoice& voice, float deltaTime);
    WavetableVoice* findFreeVoice();
    void initializeWavetables();
    void generateBasicWaveforms();
    void generateSpectralWaveforms();
    
    // Constants
    static constexpr float PHASE_INCREMENT = 1.0f / SAMPLE_RATE;
    static constexpr float ENV_MIN = 0.001f;
    static constexpr float MORPH_SMOOTHING = 0.95f;
};

// Wavetable generation utilities
namespace WavetableUtils {
    void generateSine(std::array<float, 2048>& table);
    void generateSaw(std::array<float, 2048>& table);
    void generateSquare(std::array<float, 2048>& table);
    void generateTriangle(std::array<float, 2048>& table);
    void generateNoise(std::array<float, 2048>& table);
    void generateHarmonic(std::array<float, 2048>& table, int harmonic, float amplitude);
    void generateSpectrum(std::array<float, 2048>& table, const std::vector<float>& harmonics);
}