#pragma once
#include "../synthesis/SynthEngine.h"
#include <array>
#include <cmath>

class DrumKitEngine : public SynthEngine {
public:
    DrumKitEngine();
    ~DrumKitEngine() override = default;
 
     EngineType getType() const override { return EngineType::DRUM_KIT; }
     const char* getName() const override { return "AnalogDrumKit"; }
     const char* getDescription() const override { return "808/909-style synthesized kit"; }
 
     void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f) override;
     void noteOff(uint8_t note) override;
     void setAftertouch(uint8_t note, float aftertouch) override {}
     void allNotesOff() override;
 
     void setParameter(ParameterID param, float value) override;
     float getParameter(ParameterID param) const override;
     bool hasParameter(ParameterID param) const override { return true; }
 
     void processAudio(EtherAudioBuffer& outputBuffer) override;
 
     size_t getActiveVoiceCount() const override;
     size_t getMaxVoiceCount() const override { return voices_.size(); }
     void setVoiceCount(size_t maxVoices) override {}
 
     float getCPUUsage() const override { return 0.0f; }
     void savePreset(uint8_t*, size_t, size_t&) const override {}
     bool loadPreset(const uint8_t*, size_t) override { return true; }
     void setSampleRate(float sr) override { sampleRate_ = sr; }
     void setBufferSize(size_t) override {}
 
 private:
    enum class Kit { K808, K909 };
    Kit kit_ = Kit::K808;
    float volume_ = 0.8f;
    float pan_ = 0.5f;
    float sampleRate_ = 48000.0f;
    // Internal headroom to prevent clipping when multiple voices sum
    float headroom_ = 0.7f; // ~-3.1 dB
    // Global decay scaler that stacks with per-pad decay
    float decayScale_ = 1.0f; // maps from DECAY param 0..1 -> 0.25..2.0
    // Per-pad parameters (16 pads)
    std::array<float,16> padDecay_{ };
    std::array<float,16> padTune_{ };
    std::array<float,16> padLevel_{ };
    std::array<float,16> padPan_{ };
 
    struct DrumVoice {
        enum class Type { KICK, KICK_2, SNARE, SNARE_2, RIM, CLAP, HAT_C, HAT_P, HAT_O, TOM_L1, TOM_L2, TOM_M1, TOM_M2, TOM_H1, TOM_H2, CRASH, RIDE, COWBELL, SHAKER } type;
        bool active = false;
        int padIndex = 0;
        // amplitude envelopes
        float amp = 0.0f;     // main amp
        float ampMul = 0.9995f; // per-sample multiplier
        float noise = 0.0f;   // noise env
        float noiseMul = 0.9995f;
        // tonal oscillator
        float phase = 0.0f;
        float freq = 100.0f;
        // optional second oscillator (snare body)
        float phase2 = 0.0f;
        float freq2 = 0.0f;
        // snare tone envelope
        float toneEnv = 1.0f;
        float toneMul = 0.9995f;
        // pitch envelope for kick/toms
        float pitch = 0.0f;          // legacy single-stage (toms)
        float pitchMul = 0.995f;
        // kick: dual-stage pitch envelope (fast + slow)
        float pitchFast = 0.0f;
        float pitchSlow = 0.0f;
        float pitchMulFast = 0.0f;
        float pitchMulSlow = 0.0f;
        // kick: transient drive envelope for gentle harmonics
        float driveEnv = 1.0f;
        float driveMul = 0.9995f;
        // open-hat hold
        float openHold = 0.0f;
        // clap multi-burst timing
        float clapTime = 0.0f; // seconds since trigger
        // simple filter state per voice
        float hpf_y1 = 0.0f, hpf_x1 = 0.0f, hpf_a = 0.0f;
        float lp_y1 = 0.0f, lp_a = 0.0f;
        // metallic partial phases
        float metalPh[6] = {0};
        float metalFreq[6] = {0};
        // noise seed
        float noiseSeed = 0.1234f;
        // legacy
        float envDecay = 0.0025f;
        // life limiting
        int lifeSamples = 0;
        int maxSamples = 48000; // default 1s @ 48k
    };
 
    std::array<DrumVoice, 16> voices_{};

public:
    // Per-pad control API
    void setPadDecay(int pad, float v) { if (pad>=0 && pad<16) padDecay_[pad] = std::clamp(v, 0.0f, 1.0f); }
    void setPadTune(int pad, float v) { if (pad>=0 && pad<16) padTune_[pad] = std::clamp(v, -1.0f, 1.0f); }
    void setPadLevel(int pad, float v){ if (pad>=0 && pad<16) padLevel_[pad]= std::clamp(v, 0.0f, 1.0f); }
    void setPadPan(int pad, float v)  { if (pad>=0 && pad<16) padPan_[pad]  = std::clamp(v, 0.0f, 1.0f); }
    float getPadDecay(int pad) const { return (pad>=0&&pad<16)?padDecay_[pad]:0.0f; }
    float getPadTune(int pad) const  { return (pad>=0&&pad<16)?padTune_[pad]:0.0f; }
    float getPadLevel(int pad) const { return (pad>=0&&pad<16)?padLevel_[pad]:1.0f; }
    float getPadPan(int pad) const   { return (pad>=0&&pad<16)?padPan_[pad]:0.5f; }

    static int mapNoteToPad(int note);
 
     static float frand(float &seed) {
         seed = std::fmod(seed * 1.13137f + 0.1379f, 1.0f);
         return seed * 2.0f - 1.0f;
     }
 };
