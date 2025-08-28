#include "InsertFX.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace InsertFX {

//-----------------------------------------------------------------------------
// Base Effect Implementation
//-----------------------------------------------------------------------------

void Effect::processBlock(float* buffer, size_t frames) {
    if (bypass_) return;
    
    for (size_t i = 0; i < frames; ++i) {
        buffer[i] = process(buffer[i]);
    }
}

//-----------------------------------------------------------------------------
// TransientShaper Implementation
//-----------------------------------------------------------------------------

TransientShaper::TransientShaper() 
    : attack_(0.0f), sustain_(0.0f), delayPos_(0) {
    delayLine_.fill(0.0f);
}

void TransientShaper::reset() {
    fastEnv_.reset();
    slowEnv_.reset();
    delayLine_.fill(0.0f);
    delayPos_ = 0;
}

float TransientShaper::process(float input) {
    // Envelope followers for transient and sustain detection
    fastEnv_.setAttackTime(0.001f);   // 1ms attack
    fastEnv_.setReleaseTime(0.010f);  // 10ms release
    slowEnv_.setAttackTime(0.050f);   // 50ms attack
    slowEnv_.setReleaseTime(0.200f);  // 200ms release
    
    float fastLevel = fastEnv_.process(std::abs(input));
    float slowLevel = slowEnv_.process(std::abs(input));
    
    // Store in delay line for lookahead
    delayLine_[delayPos_] = input;
    size_t readPos = (delayPos_ + 1) % delayLine_.size();
    float delayedInput = delayLine_[readPos];
    
    delayPos_ = (delayPos_ + 1) % delayLine_.size();
    
    // Calculate transient and sustain levels
    float transientLevel = fastLevel - slowLevel;
    transientLevel = std::max(0.0f, transientLevel);
    
    // Calculate gain modification
    float gain = calculateGain(delayedInput, transientLevel, slowLevel);
    
    return delayedInput * gain;
}

float TransientShaper::calculateGain(float input, float transientLevel, float sustainLevel) {
    float gain = 1.0f;
    
    // Attack gain (affects transients)
    if (transientLevel > 0.01f) {
        float attackGain = 1.0f + attack_ * 2.0f;  // -1 to +1 -> 0.5 to 3.0
        attackGain = std::clamp(attackGain, 0.1f, 5.0f);
        gain *= std::pow(attackGain, transientLevel * 2.0f);
    }
    
    // Sustain gain (affects sustain portion)
    if (sustainLevel > 0.01f) {
        float sustainGain = 1.0f + sustain_ * 1.5f;  // -1 to +1 -> -0.5 to 2.5
        sustainGain = std::clamp(sustainGain, 0.1f, 3.0f);
        gain *= std::pow(sustainGain, sustainLevel);
    }
    
    return std::clamp(gain, 0.1f, 5.0f);
}

void TransientShaper::setParam(int paramID, float value) {
    switch (paramID) {
        case ATTACK: attack_ = std::clamp(value * 2.0f - 1.0f, -1.0f, 1.0f); break;
        case SUSTAIN: sustain_ = std::clamp(value * 2.0f - 1.0f, -1.0f, 1.0f); break;
    }
}

float TransientShaper::getParam(int paramID) const {
    switch (paramID) {
        case ATTACK: return (attack_ + 1.0f) * 0.5f;
        case SUSTAIN: return (sustain_ + 1.0f) * 0.5f;
        default: return 0.0f;
    }
}

const char* TransientShaper::getParameterName(int paramID) const {
    switch (paramID) {
        case ATTACK: return "Attack";
        case SUSTAIN: return "Sustain";
        default: return "Unknown";
    }
}

//-----------------------------------------------------------------------------
// Chorus Implementation
//-----------------------------------------------------------------------------

Chorus::Chorus() : rate_(1.0f), depth_(0.5f), mix_(0.5f), feedback_(0.2f),
                   delaySize_(0), writePos_(0) {}

void Chorus::init(float sampleRate) {
    Effect::init(sampleRate);
    
    // Allocate delay buffer for max 50ms delay
    delaySize_ = static_cast<size_t>(sampleRate * 0.050f);
    delayBuffer_.resize(delaySize_);
    
    lfo_.init(sampleRate);
    lfo_.setFrequency(rate_);
    lfo_.setWaveform(DSP::Oscillator::Waveform::SINE);
    
    mixSmooth_.init(sampleRate, 10.0f);    // 10Hz cutoff
    depthSmooth_.init(sampleRate, 10.0f);
}

void Chorus::reset() {
    std::fill(delayBuffer_.begin(), delayBuffer_.end(), 0.0f);
    writePos_ = 0;
    lfo_.reset();
    mixSmooth_.reset();
    depthSmooth_.reset();
}

float Chorus::process(float input) {
    // Write to delay buffer
    delayBuffer_[writePos_] = input + feedback_ * delayBuffer_[writePos_];
    
    // Generate LFO modulation
    float lfoValue = lfo_.process();
    float smoothDepth = depthSmooth_.process(depth_);
    
    // Calculate modulated delay time (5-25ms base + modulation)
    float baseDelay = 0.015f * sampleRate_;  // 15ms
    float modulation = smoothDepth * 0.010f * sampleRate_;  // ±10ms
    float totalDelay = baseDelay + lfoValue * modulation;
    
    // Read from delay buffer with interpolation
    float delayedSample = interpolatedRead(totalDelay);
    
    // Mix dry and wet signals
    float smoothMix = mixSmooth_.process(mix_);
    float output = input * (1.0f - smoothMix) + delayedSample * smoothMix;
    
    writePos_ = (writePos_ + 1) % delaySize_;
    return output;
}

float Chorus::interpolatedRead(float delaySamples) {
    float readPos = writePos_ - delaySamples;
    if (readPos < 0) readPos += delaySize_;
    
    size_t pos1 = static_cast<size_t>(readPos) % delaySize_;
    size_t pos2 = (pos1 + 1) % delaySize_;
    float frac = readPos - std::floor(readPos);
    
    return delayBuffer_[pos1] * (1.0f - frac) + delayBuffer_[pos2] * frac;
}

void Chorus::setParam(int paramID, float value) {
    switch (paramID) {
        case RATE: 
            rate_ = 0.1f + value * 4.9f;  // 0.1 to 5.0 Hz
            lfo_.setFrequency(rate_);
            break;
        case DEPTH: depth_ = value; break;
        case MIX: mix_ = value; break;
        case FEEDBACK: feedback_ = value * 0.7f; break;  // 0 to 0.7
    }
}

float Chorus::getParam(int paramID) const {
    switch (paramID) {
        case RATE: return (rate_ - 0.1f) / 4.9f;
        case DEPTH: return depth_;
        case MIX: return mix_;
        case FEEDBACK: return feedback_ / 0.7f;
        default: return 0.0f;
    }
}

const char* Chorus::getParameterName(int paramID) const {
    switch (paramID) {
        case RATE: return "Rate";
        case DEPTH: return "Depth";
        case MIX: return "Mix";
        case FEEDBACK: return "Feedback";
        default: return "Unknown";
    }
}

//-----------------------------------------------------------------------------
// Bitcrusher Implementation
//-----------------------------------------------------------------------------

Bitcrusher::Bitcrusher() : bits_(8.0f), crushRate_(0.5f), mix_(1.0f),
                           holdSample_(0.0f), holdCounter_(0), holdPeriod_(1) {}

void Bitcrusher::reset() {
    holdSample_ = 0.0f;
    holdCounter_ = 0;
    holdPeriod_ = 1;
}

float Bitcrusher::process(float input) {
    // Sample rate crushing
    holdCounter_++;
    holdPeriod_ = std::max(1, static_cast<int>(1.0f / crushRate_));
    
    if (holdCounter_ >= holdPeriod_) {
        holdSample_ = input;
        holdCounter_ = 0;
    }
    
    // Bit depth crushing
    float crushed = quantize(holdSample_, static_cast<int>(bits_));
    
    // Mix dry/wet
    return input * (1.0f - mix_) + crushed * mix_;
}

float Bitcrusher::quantize(float input, int bits) {
    if (bits >= 16) return input;
    
    int levels = 1 << bits;  // 2^bits
    float scale = levels - 1;
    
    // Quantize to bit depth
    int quantized = static_cast<int>((input + 1.0f) * 0.5f * scale);
    quantized = std::clamp(quantized, 0, static_cast<int>(scale));
    
    return (quantized / scale) * 2.0f - 1.0f;
}

void Bitcrusher::setParam(int paramID, float value) {
    switch (paramID) {
        case BITS: bits_ = 1.0f + value * 15.0f; break;      // 1-16 bits
        case SAMPLE_RATE: crushRate_ = 0.01f + value * 0.99f; break; // 1% to 100%
        case MIX: mix_ = value; break;
    }
}

float Bitcrusher::getParam(int paramID) const {
    switch (paramID) {
        case BITS: return (bits_ - 1.0f) / 15.0f;
        case SAMPLE_RATE: return (crushRate_ - 0.01f) / 0.99f;
        case MIX: return mix_;
        default: return 0.0f;
    }
}

const char* Bitcrusher::getParameterName(int paramID) const {
    switch (paramID) {
        case BITS: return "Bits";
        case SAMPLE_RATE: return "Sample Rate";
        case MIX: return "Mix";
        default: return "Unknown";
    }
}

//-----------------------------------------------------------------------------
// MicroDelay Implementation
//-----------------------------------------------------------------------------

MicroDelay::MicroDelay() : delayTime_(0.020f), feedback_(0.3f), filterFreq_(2000.0f),
                           mix_(0.3f), maxDelaySize_(0), writePos_(0) {}

void MicroDelay::init(float sampleRate) {
    Effect::init(sampleRate);
    
    // Allocate delay buffer for max 100ms
    maxDelaySize_ = static_cast<size_t>(sampleRate * 0.100f);
    delayBuffer_.resize(maxDelaySize_);
    
    filter_.init(sampleRate);
    filter_.setType(DSP::BiquadFilter::Type::LOWPASS);
    filter_.setFrequency(filterFreq_);
    filter_.setQ(0.7f);
    
    mixSmooth_.init(sampleRate, 20.0f);
}

void MicroDelay::reset() {
    std::fill(delayBuffer_.begin(), delayBuffer_.end(), 0.0f);
    writePos_ = 0;
    filter_.reset();
    mixSmooth_.reset();
}

float MicroDelay::process(float input) {
    // Calculate delay in samples
    size_t delaySamples = static_cast<size_t>(delayTime_ * sampleRate_);
    delaySamples = std::clamp(delaySamples, size_t(1), maxDelaySize_ - 1);
    
    // Read delayed sample
    size_t readPos = (writePos_ + maxDelaySize_ - delaySamples) % maxDelaySize_;
    float delayed = delayBuffer_[readPos];
    
    // Apply filter to feedback
    float filtered = filter_.process(delayed);
    
    // Write input + feedback to delay line
    delayBuffer_[writePos_] = input + filtered * feedback_;
    
    // Mix dry and wet
    float smoothMix = mixSmooth_.process(mix_);
    float output = input * (1.0f - smoothMix) + delayed * smoothMix;
    
    writePos_ = (writePos_ + 1) % maxDelaySize_;
    return output;
}

void MicroDelay::setParam(int paramID, float value) {
    switch (paramID) {
        case TIME: 
            delayTime_ = 0.001f + value * 0.099f;  // 1ms to 100ms
            break;
        case FEEDBACK: 
            feedback_ = value * 0.95f;  // 0 to 0.95
            break;
        case FILTER:
            filterFreq_ = 200.0f + value * 7800.0f;  // 200Hz to 8kHz
            filter_.setFrequency(filterFreq_);
            break;
        case MIX: 
            mix_ = value;
            break;
    }
}

float MicroDelay::getParam(int paramID) const {
    switch (paramID) {
        case TIME: return (delayTime_ - 0.001f) / 0.099f;
        case FEEDBACK: return feedback_ / 0.95f;
        case FILTER: return (filterFreq_ - 200.0f) / 7800.0f;
        case MIX: return mix_;
        default: return 0.0f;
    }
}

const char* MicroDelay::getParameterName(int paramID) const {
    switch (paramID) {
        case TIME: return "Time";
        case FEEDBACK: return "Feedback";
        case FILTER: return "Filter";
        case MIX: return "Mix";
        default: return "Unknown";
    }
}

//-----------------------------------------------------------------------------
// Saturator Implementation
//-----------------------------------------------------------------------------

Saturator::Saturator() : drive_(0.3f), tone_(0.5f), mix_(1.0f) {}

void Saturator::reset() {
    toneFilter_.reset();
    driveSmooth_.reset();
}

float Saturator::process(float input) {
    float smoothDrive = driveSmooth_.process(drive_);
    
    // Apply saturation
    float saturated = softClip(input, smoothDrive);
    
    // Add asymmetric clipping for warmth
    saturated = asymmetricClip(saturated, smoothDrive * 0.3f);
    
    // Tone control (tilt EQ)
    toneFilter_.setType(DSP::BiquadFilter::Type::HIGHSHELF);
    toneFilter_.setFrequency(3000.0f);
    toneFilter_.setGain((tone_ - 0.5f) * 12.0f);  // ±6dB at 3kHz
    
    float toned = toneFilter_.process(saturated);
    
    // Mix
    return input * (1.0f - mix_) + toned * mix_;
}

float Saturator::softClip(float input, float drive) {
    float driven = input * (1.0f + drive * 4.0f);  // 1x to 5x gain
    
    // Soft clipping using tanh
    return std::tanh(driven * 0.7f) * 1.2f;
}

float Saturator::asymmetricClip(float input, float amount) {
    if (amount < 0.01f) return input;
    
    // Asymmetric clipping - different behavior for positive/negative
    if (input > 0.0f) {
        return input * (1.0f - amount * 0.1f);  // Slight compression on positive
    } else {
        return input * (1.0f + amount * 0.15f); // Slight expansion on negative
    }
}

void Saturator::setParam(int paramID, float value) {
    switch (paramID) {
        case DRIVE: drive_ = value; break;
        case TONE: tone_ = value; break;
        case MIX: mix_ = value; break;
    }
}

float Saturator::getParam(int paramID) const {
    switch (paramID) {
        case DRIVE: return drive_;
        case TONE: return tone_;
        case MIX: return mix_;
        default: return 0.0f;
    }
}

const char* Saturator::getParameterName(int paramID) const {
    switch (paramID) {
        case DRIVE: return "Drive";
        case TONE: return "Tone";
        case MIX: return "Mix";
        default: return "Unknown";
    }
}

//-----------------------------------------------------------------------------
// InsertChain Implementation
//-----------------------------------------------------------------------------

InsertChain::InsertChain() : sampleRate_(48000.0f), chainBypass_(false) {
    effectTypes_.fill(EffectType::NONE);
}

InsertChain::~InsertChain() = default;

void InsertChain::init(float sampleRate) {
    sampleRate_ = sampleRate;
    
    for (auto& effect : effects_) {
        if (effect) {
            effect->init(sampleRate);
        }
    }
}

void InsertChain::reset() {
    for (auto& effect : effects_) {
        if (effect) {
            effect->reset();
        }
    }
}

void InsertChain::setEffect(int slot, EffectType type) {
    if (slot < 0 || slot >= MAX_INSERTS) return;
    
    effectTypes_[slot] = type;
    
    if (type == EffectType::NONE) {
        effects_[slot].reset();
    } else {
        effects_[slot] = createEffect(type);
        if (effects_[slot]) {
            effects_[slot]->init(sampleRate_);
        }
    }
}

float InsertChain::process(float input) {
    if (chainBypass_) return input;
    
    float output = input;
    
    for (int i = 0; i < MAX_INSERTS; ++i) {
        if (effects_[i] && !effects_[i]->isBypassed()) {
            output = effects_[i]->process(output);
        }
    }
    
    return output;
}

std::unique_ptr<Effect> InsertChain::createEffect(EffectType type) {
    switch (type) {
        case EffectType::TRANSIENT_SHAPER: return std::make_unique<TransientShaper>();
        case EffectType::CHORUS: return std::make_unique<Chorus>();
        case EffectType::BITCRUSHER: return std::make_unique<Bitcrusher>();
        case EffectType::MICRO_DELAY: return std::make_unique<MicroDelay>();
        case EffectType::SATURATOR: return std::make_unique<Saturator>();
        default: return nullptr;
    }
}

const char* InsertChain::getEffectName(EffectType type) {
    switch (type) {
        case EffectType::NONE: return "None";
        case EffectType::TRANSIENT_SHAPER: return "Transient Shaper";
        case EffectType::CHORUS: return "Chorus";
        case EffectType::BITCRUSHER: return "Bitcrusher";
        case EffectType::MICRO_DELAY: return "Micro Delay";
        case EffectType::SATURATOR: return "Saturator";
        default: return "Unknown";
    }
}

//-----------------------------------------------------------------------------
// InsertFXManager Implementation
//-----------------------------------------------------------------------------

InsertFXManager& InsertFXManager::getInstance() {
    static InsertFXManager instance;
    return instance;
}

void InsertFXManager::init(float sampleRate) {
    sampleRate_ = sampleRate;
    totalCPULoad_.store(0.0f);
}

std::unique_ptr<InsertChain> InsertFXManager::createChain() {
    auto chain = std::make_unique<InsertChain>();
    chain->init(sampleRate_);
    return chain;
}

std::unique_ptr<Effect> InsertFXManager::createEffect(EffectType type) {
    return InsertChain::createEffect(type);
}

} // namespace InsertFX