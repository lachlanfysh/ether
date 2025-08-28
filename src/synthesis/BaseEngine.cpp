#include "BaseEngine.h"
#include <algorithm>
#include <cmath>

// ChannelStrip Implementation
ChannelStrip::ChannelStrip() 
    : mode_(Clean), enabled_(true), sampleRate_(48000.0f),
      compAmount_(0.0f), compAttack_(0.01f), compRelease_(0.12f),
      compEnvelope_(0.0f), compGainReduction_(1.0f),
      punch_(0.0f), transientState_(0.0f) {
    
    // Set default filter modes
    hpf_.setMode(DSP::SVF::HP);
    lpf_.setMode(DSP::SVF::LP);
    bodyShelf_.setMode(DSP::SVF::LP); // Low shelf approximation
    airShelf_.setMode(DSP::SVF::HP);  // High shelf approximation
    
    // Default settings for pitched instruments
    setHPFCutoff(0.1f);      // ~30 Hz
    setLPFCutoff(0.8f);      // ~16 kHz  
    setLPFResonance(0.15f);  // Moderate resonance
    setFilterKeytrack(0.5f); // 50% tracking
    setFilterADSR(5.0f, 200.0f, 0.2f, 300.0f);
    setFilterEnvAmount(0.2f);
    setCompAmount(0.15f);
    setPunch(0.2f);
    setDrive(0.1f);
    setBody(0.5f);   // +0.5 dB
    setAir(0.5f);    // +0.5 dB
}

void ChannelStrip::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    hpf_.setSampleRate(sampleRate);
    lpf_.setSampleRate(sampleRate);
    bodyShelf_.setSampleRate(sampleRate);
    airShelf_.setSampleRate(sampleRate);
    filterEnv_.setSampleRate(sampleRate);
    
    filterCutoff_.setSampleRate(sampleRate);
    filterRes_.setSampleRate(sampleRate);
    filterEnvAmt_.setSampleRate(sampleRate);
    driveAmount_.setSampleRate(sampleRate);
    driveTone_.setSampleRate(sampleRate);
    bodyGain_.setSampleRate(sampleRate);
    airGain_.setSampleRate(sampleRate);
}

void ChannelStrip::setHPFCutoff(float cutoff01) {
    float freq = BaseEngine::logScale(cutoff01, 10.0f, 1000.0f);
    hpf_.setCutoff(freq);
}

void ChannelStrip::setLPFCutoff(float cutoff01) {
    baseFilterCutoff_ = BaseEngine::logScale(cutoff01, 200.0f, 18000.0f);
    filterCutoff_.setTarget(baseFilterCutoff_);
}

void ChannelStrip::setLPFResonance(float res01) {
    float res = std::clamp(res01 * 0.9f, 0.0f, 0.9f); // Max 0.9 to prevent self-osc
    filterRes_.setTarget(res);
}

void ChannelStrip::setFilterKeytrack(float track01) {
    keytrackAmount_ = track01; // 0-100% tracking
}

void ChannelStrip::setFilterEnvAmount(float amt) {
    filterEnvAmt_.setTarget(amt); // -1..+1 range
}

void ChannelStrip::setCompAmount(float comp01) {
    compAmount_ = comp01;
}

void ChannelStrip::setPunch(float punch01) {
    punch_ = punch01;
}

void ChannelStrip::setDrive(float drive01) {
    driveAmount_.setTarget(drive01);
}

void ChannelStrip::setDriveTone(float tone01) {
    driveTone_.setTarget(tone01);
}

void ChannelStrip::setBody(float body) {
    // -6..+6 dB converted to linear gain  
    float gain = DSP::Audio::dbToLinear((body - 0.5f) * 12.0f);
    bodyGain_.setTarget(gain);
}

void ChannelStrip::setAir(float air) {
    // -6..+6 dB converted to linear gain
    float gain = DSP::Audio::dbToLinear((air - 0.5f) * 12.0f);
    airGain_.setTarget(gain);
}

void ChannelStrip::setFilterADSR(float attack_ms, float decay_ms, float sustain, float release_ms) {
    filterEnv_.setADSR(attack_ms, decay_ms, sustain, release_ms);
}

void ChannelStrip::noteOn(float note, float velocity) {
    filterEnv_.noteOn();
}

void ChannelStrip::noteOff() {
    filterEnv_.noteOff();
}

float ChannelStrip::processCompressor(float input) {
    if (compAmount_ <= 0.001f) return input;
    
    float level = std::abs(input);
    float targetGain = (level > 0.1f) ? std::min(1.0f, 0.1f / level) : 1.0f;
    
    // Simple envelope follower
    float attack = (targetGain < compEnvelope_) ? compAttack_ : compRelease_;
    compEnvelope_ += (targetGain - compEnvelope_) * attack;
    
    float gain = 1.0f - compAmount_ + compAmount_ * compEnvelope_;
    return input * gain;
}

float ChannelStrip::processTransientEnhancer(float input) {
    if (punch_ <= 0.001f) return input;
    
    // Simple high-pass derivative for transient detection
    float transient = input - transientState_;
    transientState_ = input;
    
    // Enhance transients
    return input + transient * punch_ * 0.5f;
}

float ChannelStrip::processDrive(float input) {
    float drive = driveAmount_.process();
    if (drive <= 0.001f) return input;
    
    float tone = driveTone_.process();
    
    // Asymmetric soft clipping with tone control
    float driven = input * (1.0f + drive * 8.0f);
    float clipped = DSP::Audio::softClip(driven);
    
    // Tone control: odd/even harmonic emphasis
    float even = clipped;
    float odd = DSP::Audio::tanhSat(driven * 0.5f, 2.0f);
    
    float output = DSP::Interp::linear(even, odd, tone);
    output = dcBlocker_.process(output);
    
    // Level compensation
    return output * (1.0f - drive * 0.3f);
}

float ChannelStrip::processTiltEQ(float input) {
    float body = bodyGain_.process();
    float air = airGain_.process();
    
    // Simplified tilt EQ using shelving filters
    float output = input;
    
    if (std::abs(body - 1.0f) > 0.001f) {
        bodyShelf_.setCutoff(120.0f);
        output = DSP::Interp::linear(output, bodyShelf_.process(output), body - 1.0f);
    }
    
    if (std::abs(air - 1.0f) > 0.001f) {
        airShelf_.setCutoff(7000.0f);
        output = DSP::Interp::linear(output, airShelf_.process(output), air - 1.0f);
    }
    
    return output;
}

float ChannelStrip::process(float input, float keytrackNote) {
    if (!enabled_) return input;
    
    float output = input;
    
    // Update smooth parameters
    float cutoff = filterCutoff_.process();
    float res = filterRes_.process();
    float envAmt = filterEnvAmt_.process();
    
    // Apply keytracking to filter cutoff
    float keytrackOffset = (keytrackNote - 60.0f) * keytrackAmount_ * 100.0f; // Hz per semitone
    cutoff = std::clamp(cutoff + keytrackOffset, 200.0f, 18000.0f);
    
    // Apply filter envelope
    float envValue = filterEnv_.process();
    float envModulation = envValue * envAmt * cutoff * 0.5f; // Up to 50% modulation
    cutoff = std::clamp(cutoff + envModulation, 200.0f, 18000.0f);
    
    lpf_.setCutoff(cutoff);
    lpf_.setResonance(res);
    
    // Processing order depends on mode
    if (mode_ == Clean) {
        // Clean: Core -> HPF -> LPF -> Comp -> Drive -> TiltEQ
        output = hpf_.process(output);
        output = lpf_.process(output);
        output = processCompressor(output);
        output = processTransientEnhancer(output);
        output = processDrive(output);
        output = processTiltEQ(output);
    } else {
        // Dirty: Core -> Drive -> HPF -> LPF -> Comp -> TiltEQ
        output = processDrive(output);
        output = hpf_.process(output);
        output = lpf_.process(output);
        output = processCompressor(output);
        output = processTransientEnhancer(output);
        output = processTiltEQ(output);
    }
    
    return output;
}

void ChannelStrip::processBlock(const float* input, float* output, int count, float keytrackNote) {
    for (int i = 0; i < count; ++i) {
        output[i] = process(input[i], keytrackNote);
    }
}

void ChannelStrip::reset() {
    hpf_.reset();
    lpf_.reset();
    bodyShelf_.reset();
    airShelf_.reset();
    filterEnv_.reset();
    compEnvelope_ = 0.0f;
    compGainReduction_ = 1.0f;
    transientState_ = 0.0f;
}

// BaseEngine Implementation
BaseEngine::BaseEngine(const char* name, const char* shortName, int engineID, CPUClass cpuClass)
    : name_(name), shortName_(shortName), engineID_(engineID), cpuClass_(cpuClass),
      sampleRate_(48000.0), maxBlockSize_(128), stripEnabled_(true),
      nextVoiceID_(1), currentTime_(0) {
    
    channelStrip_ = std::make_unique<ChannelStrip>();
}

void BaseEngine::prepare(double sampleRate, int maxBlockSize) {
    sampleRate_ = sampleRate;
    maxBlockSize_ = maxBlockSize;
    
    // Update smooth parameters sample rate
    harmonics_.setSampleRate(static_cast<float>(sampleRate));
    timbre_.setSampleRate(static_cast<float>(sampleRate));
    morph_.setSampleRate(static_cast<float>(sampleRate));
    level_.setSampleRate(static_cast<float>(sampleRate));
    extra1_.setSampleRate(static_cast<float>(sampleRate));
    extra2_.setSampleRate(static_cast<float>(sampleRate));
    
    if (channelStrip_) {
        channelStrip_->setSampleRate(static_cast<float>(sampleRate));
    }
}

void BaseEngine::reset() {
    voices_.clear();
    parameters_.clear();
    modulations_.clear();
    nextVoiceID_ = 1;
    currentTime_ = 0;
    
    // Reset smooth parameters
    harmonics_.setImmediate(0.5f);
    timbre_.setImmediate(0.5f);
    morph_.setImmediate(0.5f);
    level_.setImmediate(0.8f);
    extra1_.setImmediate(0.5f);
    extra2_.setImmediate(0.5f);
    
    if (channelStrip_) {
        channelStrip_->reset();
    }
}

void BaseEngine::setParam(int paramID, float v01) {
    parameters_[paramID] = std::clamp(v01, 0.0f, 1.0f);
    
    // Update common parameters
    switch (static_cast<EngineParamID>(paramID)) {
        case EngineParamID::HARMONICS:
            harmonics_.setTarget(v01);
            break;
        case EngineParamID::TIMBRE:
            timbre_.setTarget(v01);
            break;
        case EngineParamID::MORPH:
            morph_.setTarget(v01);
            break;
        case EngineParamID::LEVEL:
            level_.setTarget(v01);
            break;
        case EngineParamID::EXTRA1:
            extra1_.setTarget(v01);
            break;
        case EngineParamID::EXTRA2:
            extra2_.setTarget(v01);
            break;
        case EngineParamID::STRIP_ENABLE:
            stripEnabled_ = (v01 > 0.5f);
            if (channelStrip_) channelStrip_->setEnabled(stripEnabled_);
            break;
        case EngineParamID::STRIP_MODE:
            if (channelStrip_) {
                channelStrip_->setMode(v01 > 0.5f ? ChannelStrip::Dirty : ChannelStrip::Clean);
            }
            break;
        // Channel strip parameters
        case EngineParamID::HPF_CUTOFF:
            if (channelStrip_) channelStrip_->setHPFCutoff(v01);
            break;
        case EngineParamID::LPF_CUTOFF:
            if (channelStrip_) channelStrip_->setLPFCutoff(v01);
            break;
        case EngineParamID::LPF_RES:
            if (channelStrip_) channelStrip_->setLPFResonance(v01);
            break;
        default:
            // Engine-specific parameters handled by derived classes
            break;
    }
}

void BaseEngine::setMod(int paramID, float value, float depth) {
    modulations_[paramID] = value * depth;
}

float BaseEngine::getParam(int paramID, float defaultValue) const {
    auto it = parameters_.find(paramID);
    return (it != parameters_.end()) ? it->second : defaultValue;
}

float BaseEngine::getModulatedParam(int paramID, float baseValue) const {
    auto it = modulations_.find(paramID);
    if (it != modulations_.end()) {
        return std::clamp(baseValue + it->second, 0.0f, 1.0f);
    }
    return baseValue;
}

void BaseEngine::updateSmoothParams() {
    currentTime_++;
    
    // Update all smooth parameters
    harmonics_.process();
    timbre_.process();
    morph_.process();
    level_.process();
    extra1_.process();
    extra2_.process();
}

BaseEngine::VoiceContext* BaseEngine::findVoice(uint32_t id) {
    for (auto& voice : voices_) {
        if (voice.id == id) {
            return &voice;
        }
    }
    return nullptr;
}

BaseEngine::VoiceContext* BaseEngine::allocateVoice() {
    // Find inactive voice
    for (auto& voice : voices_) {
        if (!voice.active) {
            voice.id = nextVoiceID_++;
            voice.startTime = currentTime_;
            return &voice;
        }
    }
    
    // Add new voice if under limit
    if (voices_.size() < 16) { // Max 16 voices
        voices_.emplace_back();
        VoiceContext& voice = voices_.back();
        voice.id = nextVoiceID_++;
        voice.startTime = currentTime_;
        return &voice;
    }
    
    // Steal oldest voice
    return stealVoice();
}

BaseEngine::VoiceContext* BaseEngine::stealVoice() {
    VoiceContext* oldest = nullptr;
    uint32_t oldestTime = UINT32_MAX;
    
    // Prefer releasing voices
    for (auto& voice : voices_) {
        if (voice.releasing && voice.startTime < oldestTime) {
            oldest = &voice;
            oldestTime = voice.startTime;
        }
    }
    
    if (oldest) return oldest;
    
    // Fall back to oldest active voice
    for (auto& voice : voices_) {
        if (voice.startTime < oldestTime) {
            oldest = &voice;
            oldestTime = voice.startTime;
        }
    }
    
    return oldest;
}

// Parameter conversion utilities
float BaseEngine::logScale(float value01, float min, float max) {
    return min * std::pow(max / min, value01);
}

float BaseEngine::expScale(float value01, float min, float max) {
    return min + (max - min) * (std::exp(value01 * 3.0f) - 1.0f) / (std::exp(3.0f) - 1.0f);
}

float BaseEngine::linearScale(float value01, float min, float max) {
    return min + value01 * (max - min);
}

// Default implementations (minimal)
int BaseEngine::getParameterCount() const {
    return 10; // Common parameters only
}

const IEngine::ParameterInfo* BaseEngine::getParameterInfo(int index) const {
    static const ParameterInfo commonParams[] = {
        {static_cast<int>(EngineParamID::HARMONICS), "Harmonics", "", 0.5f, 0.0f, 1.0f, false, 0, "Synthesis"},
        {static_cast<int>(EngineParamID::TIMBRE), "Timbre", "", 0.5f, 0.0f, 1.0f, false, 0, "Synthesis"},
        {static_cast<int>(EngineParamID::MORPH), "Morph", "", 0.5f, 0.0f, 1.0f, false, 0, "Synthesis"},
        {static_cast<int>(EngineParamID::LEVEL), "Level", "dB", 0.8f, 0.0f, 1.0f, false, 0, "Mix"},
        {static_cast<int>(EngineParamID::EXTRA1), "Extra1", "", 0.5f, 0.0f, 1.0f, false, 0, "Synthesis"},
        {static_cast<int>(EngineParamID::EXTRA2), "Extra2", "", 0.5f, 0.0f, 1.0f, false, 0, "Synthesis"},
        {static_cast<int>(EngineParamID::LPF_CUTOFF), "Filter", "Hz", 0.8f, 0.0f, 1.0f, false, 0, "Filter"},
        {static_cast<int>(EngineParamID::LPF_RES), "Resonance", "", 0.15f, 0.0f, 1.0f, false, 0, "Filter"},
        {static_cast<int>(EngineParamID::DRIVE), "Drive", "", 0.1f, 0.0f, 1.0f, false, 0, "Channel"},
        {static_cast<int>(EngineParamID::COMP_AMOUNT), "Comp", "", 0.15f, 0.0f, 1.0f, false, 0, "Channel"}
    };
    
    return (index >= 0 && index < 10) ? &commonParams[index] : nullptr;
}

uint32_t BaseEngine::getModDestinations() const {
    // All common parameters support modulation
    return (1 << static_cast<int>(EngineParamID::HARMONICS)) |
           (1 << static_cast<int>(EngineParamID::TIMBRE)) |
           (1 << static_cast<int>(EngineParamID::MORPH)) |
           (1 << static_cast<int>(EngineParamID::LEVEL));
}

const IEngine::HapticInfo* BaseEngine::getHapticInfo(int paramID) const {
    static const HapticInfo commonHaptics[] = {
        {HapticPolicy::Uniform, 270.0f, nullptr, 0}, // HARMONICS
        {HapticPolicy::Uniform, 270.0f, nullptr, 0}, // TIMBRE  
        {HapticPolicy::Uniform, 270.0f, nullptr, 0}, // MORPH
        {HapticPolicy::Uniform, 270.0f, nullptr, 0}  // LEVEL
    };
    
    if (paramID >= 0 && paramID < 4) {
        return &commonHaptics[paramID];
    }
    return nullptr;
}

// BaseVoice Implementation
void BaseVoice::noteOn(float note, float velocity) {
    note_ = note;
    velocity_ = velocity;
    active_ = true;
    releasing_ = false;
    
    ampEnv_.noteOn();
    if (channelStrip_) {
        channelStrip_->noteOn(note, velocity);
    }
}

void BaseVoice::noteOff() {
    releasing_ = true;
    ampEnv_.noteOff();
    if (channelStrip_) {
        channelStrip_->noteOff();
    }
}

void BaseVoice::setSampleRate(float sampleRate) {
    ampEnv_.setSampleRate(sampleRate);
    if (channelStrip_) {
        channelStrip_->setSampleRate(sampleRate);
    }
}

void BaseVoice::reset() {
    active_ = false;
    releasing_ = false;
    ampEnv_.reset();
    if (channelStrip_) {
        channelStrip_->reset();
    }
}