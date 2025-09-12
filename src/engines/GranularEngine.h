#pragma once
#include "../synthesis/SynthEngine.h"
#include <vector>
#include <random>

class GranularEngine : public SynthEngine {
public:
    GranularEngine();
    ~GranularEngine() override = default;

    // SynthEngine interface
    EngineType getType() const override { return EngineType::COUNT; /* placeholder if not wired */ }
    const char* getName() const override { return "Granular"; }
    const char* getDescription() const override { return "Simple grain cloud"; }

    void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f) override;
    void noteOff(uint8_t note) override;
    void setAftertouch(uint8_t note, float aftertouch) override {}
    void allNotesOff() override { active_ = false; }

    void setParameter(ParameterID param, float value) override;
    float getParameter(ParameterID param) const override;
    bool hasParameter(ParameterID) const override { return true; }

    void processAudio(EtherAudioBuffer& outputBuffer) override;

    size_t getActiveVoiceCount() const override { return active_ ? 1 : 0; }
    size_t getMaxVoiceCount() const override { return 1; }
    void setVoiceCount(size_t) override {}

    float getCPUUsage() const override { return 0.0f; }

    void savePreset(uint8_t*, size_t, size_t&) const override {}
    bool loadPreset(const uint8_t*, size_t) override { return true; }

    void setSampleRate(float sr) override { sampleRate_ = sr; }
    void setBufferSize(size_t bs) override { bufferSize_ = bs; }

private:
    // Parameters (0..1)
    float position_ = 0.0f;   // Not used (placeholder)
    float size_ = 0.2f;       // seconds mapped later
    float density_ = 0.5f;    // grains per second scaler
    float jitter_ = 0.2f;     // randomness
    float texture_ = 0.5f;    // window hardness
    float pitch_ = 0.5f;      // -12..+12 semitones
    float spread_ = 0.5f;     // stereo
    float volume_ = 0.5f;
    bool active_ = false;

    std::mt19937 rng_;
    std::uniform_real_distribution<float> uni_{0.0f,1.0f};

    float phase_ = 0.0f;
};

