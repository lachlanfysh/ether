#pragma once
#include "DSPUtils.h"
#include <memory>
#include <array>

namespace InsertFX {

/**
 * Base class for all insert effects
 */
class Effect {
public:
    Effect() : sampleRate_(48000.0f), bypass_(false) {}
    virtual ~Effect() = default;
    
    virtual void init(float sampleRate) { sampleRate_ = sampleRate; }
    virtual void reset() = 0;
    virtual float process(float input) = 0;
    virtual void processBlock(float* buffer, size_t frames);
    
    // Parameter interface
    virtual void setParam(int paramID, float value) = 0;
    virtual float getParam(int paramID) const = 0;
    virtual int getParameterCount() const = 0;
    virtual const char* getParameterName(int paramID) const = 0;
    
    // Effect info
    virtual const char* getName() const = 0;
    virtual const char* getShortName() const = 0;
    
    void setBypass(bool bypass) { bypass_ = bypass; }
    bool isBypassed() const { return bypass_; }
    
protected:
    float sampleRate_;
    bool bypass_;
};

/**
 * Transient Shaper - enhances or reduces transients
 */
class TransientShaper : public Effect {
public:
    enum ParamID { ATTACK = 0, SUSTAIN = 1, PARAM_COUNT = 2 };
    
    TransientShaper();
    void reset() override;
    float process(float input) override;
    
    void setParam(int paramID, float value) override;
    float getParam(int paramID) const override;
    int getParameterCount() const override { return PARAM_COUNT; }
    const char* getParameterName(int paramID) const override;
    
    const char* getName() const override { return "Transient Shaper"; }
    const char* getShortName() const override { return "TRAN"; }
    
private:
    float attack_;      // -1.0 to +1.0 (reduce to enhance)
    float sustain_;     // -1.0 to +1.0 (reduce to enhance)
    
    // Dual-band envelope followers
    DSP::EnvelopeFollower fastEnv_;   // Transient detection
    DSP::EnvelopeFollower slowEnv_;   // Sustain detection
    
    // Delay line for lookahead
    std::array<float, 128> delayLine_;
    size_t delayPos_;
    
    float calculateGain(float input, float transientLevel, float sustainLevel);
};

/**
 * Chorus - classic modulated delay chorus effect
 */
class Chorus : public Effect {
public:
    enum ParamID { RATE = 0, DEPTH = 1, MIX = 2, FEEDBACK = 3, PARAM_COUNT = 4 };
    
    Chorus();
    void init(float sampleRate) override;
    void reset() override;
    float process(float input) override;
    
    void setParam(int paramID, float value) override;
    float getParam(int paramID) const override;
    int getParameterCount() const override { return PARAM_COUNT; }
    const char* getParameterName(int paramID) const override;
    
    const char* getName() const override { return "Chorus"; }
    const char* getShortName() const override { return "CHOR"; }
    
private:
    float rate_;        // 0.1 to 5.0 Hz
    float depth_;       // 0.0 to 1.0
    float mix_;         // 0.0 to 1.0
    float feedback_;    // 0.0 to 0.7
    
    // Modulated delay line
    std::vector<float> delayBuffer_;
    size_t delaySize_;
    size_t writePos_;
    
    // LFO for modulation
    DSP::Oscillator lfo_;
    
    // Smoothing
    DSP::OnePoleFilter mixSmooth_;
    DSP::OnePoleFilter depthSmooth_;
    
    float interpolatedRead(float delaySamples);
};

/**
 * Bitcrusher - digital distortion and sample rate reduction
 */
class Bitcrusher : public Effect {
public:
    enum ParamID { BITS = 0, SAMPLE_RATE = 1, MIX = 2, PARAM_COUNT = 3 };
    
    Bitcrusher();
    void reset() override;
    float process(float input) override;
    
    void setParam(int paramID, float value) override;
    float getParam(int paramID) const override;
    int getParameterCount() const override { return PARAM_COUNT; }
    const char* getParameterName(int paramID) const override;
    
    const char* getName() const override { return "Bitcrusher"; }
    const char* getShortName() const override { return "BITS"; }
    
private:
    float bits_;        // 1.0 to 16.0
    float crushRate_;   // 0.0 to 1.0 (fraction of original sample rate)
    float mix_;         // 0.0 to 1.0
    
    // Sample rate reduction
    float holdSample_;
    int holdCounter_;
    int holdPeriod_;
    
    // Bit depth reduction
    float quantize(float input, int bits);
};

/**
 * Micro Delay - short delay with filtering and modulation  
 */
class MicroDelay : public Effect {
public:
    enum ParamID { TIME = 0, FEEDBACK = 1, FILTER = 2, MIX = 3, PARAM_COUNT = 4 };
    
    MicroDelay();
    void init(float sampleRate) override;
    void reset() override;
    float process(float input) override;
    
    void setParam(int paramID, float value) override;
    float getParam(int paramID) const override;
    int getParameterCount() const override { return PARAM_COUNT; }
    const char* getParameterName(int paramID) const override;
    
    const char* getName() const override { return "Micro Delay"; }
    const char* getShortName() const override { return "DELY"; }
    
private:
    float delayTime_;   // 1ms to 100ms
    float feedback_;    // 0.0 to 0.95
    float filterFreq_;  // 200Hz to 8kHz  
    float mix_;         // 0.0 to 1.0
    
    std::vector<float> delayBuffer_;
    size_t maxDelaySize_;
    size_t writePos_;
    
    DSP::BiquadFilter filter_;
    DSP::OnePoleFilter mixSmooth_;
};

/**
 * Saturator - analog-style soft saturation/distortion
 */
class Saturator : public Effect {
public:
    enum ParamID { DRIVE = 0, TONE = 1, MIX = 2, PARAM_COUNT = 3 };
    
    Saturator();
    void reset() override;
    float process(float input) override;
    
    void setParam(int paramID, float value) override;
    float getParam(int paramID) const override;
    int getParameterCount() const override { return PARAM_COUNT; }
    const char* getParameterName(int paramID) const override;
    
    const char* getName() const override { return "Saturator"; }
    const char* getShortName() const override { return "SAT"; }
    
private:
    float drive_;       // 0.0 to 1.0
    float tone_;        // 0.0 to 1.0 (dark to bright)
    float mix_;         // 0.0 to 1.0
    
    DSP::BiquadFilter toneFilter_;
    DSP::OnePoleFilter driveSmooth_;
    
    float softClip(float input, float drive);
    float asymmetricClip(float input, float amount);
};

/**
 * Effect type enumeration
 */
enum class EffectType {
    NONE = 0,
    TRANSIENT_SHAPER,
    CHORUS,
    BITCRUSHER,
    MICRO_DELAY,
    SATURATOR,
    COUNT
};

/**
 * Insert FX chain - manages up to 2 effects per voice/engine
 */
class InsertChain {
public:
    static constexpr int MAX_INSERTS = 2;
    
    InsertChain();
    ~InsertChain();
    
    void init(float sampleRate);
    void reset();
    
    // Effect management
    void setEffect(int slot, EffectType type);
    EffectType getEffectType(int slot) const;
    Effect* getEffect(int slot);
    const Effect* getEffect(int slot) const;
    
    // Processing
    float process(float input);
    void processBlock(float* buffer, size_t frames);
    
    // Parameter access
    void setEffectParam(int slot, int paramID, float value);
    float getEffectParam(int slot, int paramID) const;
    
    // Bypass control
    void setEffectBypass(int slot, bool bypass);
    bool isEffectBypassed(int slot) const;
    
    void setChainBypass(bool bypass) { chainBypass_ = bypass; }
    bool isChainBypassed() const { return chainBypass_; }
    
    // Utility
    static const char* getEffectName(EffectType type);
    static const char* getEffectShortName(EffectType type);
    static std::unique_ptr<Effect> createEffect(EffectType type);
    
private:
    float sampleRate_;
    bool chainBypass_;
    std::array<std::unique_ptr<Effect>, MAX_INSERTS> effects_;
    std::array<EffectType, MAX_INSERTS> effectTypes_;
};

/**
 * Global insert FX manager for the entire system
 */
class InsertFXManager {
public:
    static InsertFXManager& getInstance();
    
    void init(float sampleRate);
    
    // Create insert chains for engines/voices
    std::unique_ptr<InsertChain> createChain();
    
    // Effect factory
    std::unique_ptr<Effect> createEffect(EffectType type);
    
    // Effect info
    int getEffectCount() const { return static_cast<int>(EffectType::COUNT) - 1; }
    const char* getEffectName(EffectType type) const;
    const char* getEffectShortName(EffectType type) const;
    
    // CPU monitoring
    void addCPULoad(float load) { totalCPULoad_ += load; }
    float getTotalCPULoad() const { return totalCPULoad_; }
    void resetCPULoad() { totalCPULoad_ = 0.0f; }
    
private:
    InsertFXManager() = default;
    float sampleRate_;
    std::atomic<float> totalCPULoad_;
};

} // namespace InsertFX