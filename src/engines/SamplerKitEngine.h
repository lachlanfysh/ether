#pragma once
#include "../BaseEngine.h"
#include "../DSPUtils.h"
#include "../SampleBuffer.h"
#include <array>
#include <memory>

namespace SamplerKit {

/**
 * Pad configuration and state
 */
struct Pad {
    std::shared_ptr<Sample::SampleBuffer> sampleBuffer;
    float pitch = 0.0f;        // Semitones -24 to +24
    float gain = 1.0f;         // 0.0 to 2.0
    float pan = 0.0f;          // -1.0 to +1.0
    float startPos = 0.0f;     // 0.0 to 1.0
    float endPos = 1.0f;       // 0.0 to 1.0
    bool reverse = false;
    bool mute = false;
    int chokeGroup = 0;        // 0 = no choke, 1-8 = choke groups
    int maxPolyphony = 1;      // 1-8 voices per pad
    bool cutSelf = false;      // Mono retrigger with crossfade
    
    // Round-robin
    int roundRobinCount = 1;   // 1, 2, or 4
    int roundRobinIndex = 0;   // Current RR position
    std::array<std::shared_ptr<Sample::SampleBuffer>, 4> rrSamples;
    
    // Envelope (AHDSR)
    float attack = 0.001f;
    float hold = 0.0f;
    float decay = 0.3f;
    float sustain = 0.0f;
    float release = 0.1f;
    bool fastMode = false;     // Sub-10ms envelope for drums
    
    // Filter
    float lpfCutoff = 20000.0f;  // 20Hz to 20kHz
    float lpfResonance = 0.0f;   // 0.0 to 1.0
    
    // Velocity response
    float velToLevel = 1.0f;     // 0.0 to 1.0
    float velToPitch = 0.0f;     // -12 to +12 semitones
    float velToLPF = 0.0f;       // 0.0 to 1.0
    
    // FX sends
    float sendA = 0.0f;
    float sendB = 0.0f;
    float sendC = 0.0f;
    
    // Humanization
    float timingHumanize = 0.0f;  // ±8ms
    float velocityHumanize = 0.0f; // ±20 (bipolar)
    
    Pad() {
        rrSamples.fill(nullptr);
    }
    
    std::shared_ptr<Sample::SampleBuffer> getCurrentSample() {
        if (roundRobinCount == 1 || !rrSamples[0]) {
            return sampleBuffer;
        }
        
        auto sample = rrSamples[roundRobinIndex];
        roundRobinIndex = (roundRobinIndex + 1) % roundRobinCount;
        return sample ? sample : sampleBuffer;
    }
};

} // namespace SamplerKit

/**
 * SamplerKit Voice - handles individual pad voices with full feature set
 */
class SamplerKitVoice : public BaseVoice {
public:
    SamplerKitVoice() : pad_(0), chokeGroup_(0), sampleBuffer_(nullptr), 
                       pitchMacro_(0.0f), filterMacro_(1.0f), envMacro_(1.0f) {}
    
    void setSampleRate(float sampleRate) override {
        BaseVoice::setSampleRate(sampleRate);
        // AHDSR envelope is handled by ampEnv_ from BaseVoice
    }
    
    void noteOn(float note, float velocity) override {
        BaseVoice::noteOn(note, velocity);
        
        // Map MIDI note to pad (C3 = pad 0, C#3 = pad 1, etc.)
        pad_ = static_cast<int>(note) % 25;
        
        if (padConfig_ && padConfig_->sampleBuffer) {
            sampleBuffer_ = padConfig_->getCurrentSample();
            chokeGroup_ = padConfig_->chokeGroup;
            
            if (sampleBuffer_) {
                // Configure envelope based on pad settings
                float attackTime = padConfig_->attack * envMacro_;
                float decayTime = padConfig_->decay;
                float releaseTime = padConfig_->release;
                
                if (padConfig_->fastMode) {
                    attackTime = std::min(attackTime, 0.010f);  // Max 10ms in fast mode
                    releaseTime = std::min(releaseTime, 0.050f); // Max 50ms release
                }
                
                ampEnv_.setAttackTime(attackTime);
                ampEnv_.setDecayTime(decayTime);
                ampEnv_.setSustainLevel(padConfig_->sustain);
                ampEnv_.setReleaseTime(releaseTime);
                
                // Setup sample playback
                float finalPitch = padConfig_->pitch + pitchMacro_;
                
                // Apply velocity to pitch if configured
                if (padConfig_->velToPitch != 0.0f) {
                    finalPitch += padConfig_->velToPitch * (velocity - 0.5f) * 2.0f;
                }
                
                sampleBuffer_->setPitch(finalPitch);
                sampleBuffer_->setPosition(padConfig_->startPos);
                sampleBuffer_->startPlayback(padConfig_->startPos, false);  // One-shots don't loop
            }
        }
    }
    
    void noteOff() override {
        BaseVoice::noteOff();
        // One-shot samples typically ignore note-off, but we respect it for envelope
    }
    
    float renderSample(const RenderContext& ctx) override {
        if (!active_ || !sampleBuffer_ || !padConfig_) return 0.0f;
        
        float envelope = ampEnv_.process();
        
        if (envelope <= 0.001f && releasing_) {
            active_ = false;
            if (sampleBuffer_) {
                sampleBuffer_->stopPlayback();
            }
            return 0.0f;
        }
        
        // Render sample from buffer
        int16_t sampleData;
        sampleBuffer_->renderSamples(&sampleData, 1, padConfig_->gain);
        float sample = static_cast<float>(sampleData) / 32768.0f;
        
        // Apply pan (simple amplitude panning)
        if (padConfig_->pan != 0.0f) {
            sample *= (padConfig_->pan > 0.0f) ? (1.0f - padConfig_->pan) : 1.0f;
        }
        
        // Apply velocity curve
        float velGain = padConfig_->velToLevel * velocity_ + (1.0f - padConfig_->velToLevel);
        sample *= velGain;
        
        // Apply envelope and final gain
        sample *= envelope;
        
        // Simple lowpass filter (if cutoff < nyquist)
        if (padConfig_->lpfCutoff < sampleRate_ * 0.45f) {
            float cutoffNorm = (padConfig_->lpfCutoff * filterMacro_) / (sampleRate_ * 0.5f);
            cutoffNorm = std::clamp(cutoffNorm, 0.001f, 0.99f);
            
            // Simple one-pole lowpass
            filterState_ = filterState_ + cutoffNorm * (sample - filterState_);
            sample = filterState_;
        }
        
        // Apply channel strip if present
        if (channelStrip_) {
            sample = channelStrip_->process(sample, note_);
        }
        
        return sample;
    }
    
    void renderBlock(const RenderContext& ctx, float* output, int count) override {
        for (int i = 0; i < count; ++i) {
            output[i] = renderSample(ctx);
        }
    }
    
    // Parameter setters
    void setPadConfig(SamplerKit::Pad* config) { padConfig_ = config; }
    void setPitchMacro(float pitch) { pitchMacro_ = pitch; }
    void setFilterMacro(float filter) { filterMacro_ = filter; }
    void setEnvMacro(float env) { envMacro_ = env; }
    
    int getPad() const { return pad_; }
    int getChokeGroup() const { return chokeGroup_; }
    
    uint32_t getAge() const { return 0; }  // TODO: Implement proper age tracking
    
private:
    int pad_;
    int chokeGroup_;
    SamplerKit::Pad* padConfig_ = nullptr;
    std::shared_ptr<Sample::SampleBuffer> sampleBuffer_;
    
    // Macro values
    float pitchMacro_;
    float filterMacro_;
    float envMacro_;
    
    // Simple filter state
    float filterState_ = 0.0f;
};

/**
 * SamplerKit Engine - 25-pad MPC-style sampler
 */
class SamplerKitEngine : public PolyphonicBaseEngine<SamplerKitVoice> {
public:
    SamplerKitEngine() 
        : PolyphonicBaseEngine("SamplerKit", "SKIT", 
                              static_cast<int>(EngineFactory::EngineType::SAMPLER_KIT),
                              CPUClass::Medium, 64) {}  // Up to 64 voices total
    
    // IEngine interface
    const char* getName() const override { return "SamplerKit"; }
    const char* getShortName() const override { return "SKIT"; }
    
    void setParam(int paramID, float v01) override {
        BaseEngine::setParam(paramID, v01);  // Handle common parameters
        
        switch (static_cast<EngineParamID>(paramID)) {
            case EngineParamID::HARMONICS:
                // HARMONICS controls global pitch macro
                pitchMacro_ = (v01 - 0.5f) * 48.0f;  // ±24 semitones
                for (auto& voice : voices_) {
                    voice->setPitchMacro(pitchMacro_);
                }
                break;
                
            case EngineParamID::TIMBRE:
                // TIMBRE controls filter macro
                filterMacro_ = 0.1f + v01 * 0.9f;  // 0.1x to 1.0x filter scaling
                for (auto& voice : voices_) {
                    voice->setFilterMacro(filterMacro_);
                }
                break;
                
            case EngineParamID::MORPH:
                // MORPH controls envelope macro
                envMacro_ = 0.1f + v01 * 3.9f;  // 0.1x to 4.0x envelope scaling
                for (auto& voice : voices_) {
                    voice->setEnvMacro(envMacro_);
                }
                break;
                
            default:
                break;
        }
    }
    
    // Voice allocation with choke groups
    void noteOn(float note, float velocity, uint32_t id) override {
        int pad = static_cast<int>(note) % 25;
        
        // Handle choke groups
        if (pads_[pad].chokeGroup > 0) {
            chokeGroup(pads_[pad].chokeGroup);
        }
        
        // Handle "Cut Self" mode
        if (pads_[pad].cutSelf) {
            chokePad(pad);
        }
        
        // Check polyphony limits
        int activeVoicesForPad = countVoicesForPad(pad);
        if (activeVoicesForPad >= pads_[pad].maxPolyphony) {
            // Steal oldest voice for this pad
            stealOldestVoiceForPad(pad);
        }
        
        // Allocate new voice
        auto* voice = allocateVoice();
        if (voice) {
            voice->setPadConfig(&pads_[pad]);
            voice->setPitchMacro(pitchMacro_);
            voice->setFilterMacro(filterMacro_);
            voice->setEnvMacro(envMacro_);
            voice->noteOn(note, velocity);
        }
    }
    
    // Pad management
    SamplerKit::Pad& getPad(int index) { return pads_[std::clamp(index, 0, 24)]; }
    const SamplerKit::Pad& getPad(int index) const { return pads_[std::clamp(index, 0, 24)]; }
    
    void loadSampleToPad(int pad, const std::string& filePath) {
        if (pad >= 0 && pad < 25) {
            auto sampleBuffer = std::make_shared<Sample::SampleBuffer>();
            if (sampleBuffer->load(filePath)) {
                pads_[pad].sampleBuffer = sampleBuffer;
            }
        }
    }
    
    void chokeGroup(int groupID) {
        if (groupID <= 0 || groupID > 8) return;
        
        for (auto& voice : voices_) {
            if (voice->getChokeGroup() == groupID && voice->isActive()) {
                voice->noteOff();
            }
        }
    }
    
    void chokePad(int pad) {
        for (auto& voice : voices_) {
            if (voice->getPad() == pad && voice->isActive()) {
                voice->noteOff();
            }
        }
    }
    
    // Parameter metadata
    int getParameterCount() const override { return 8; }
    
    const ParameterInfo* getParameterInfo(int index) const override {
        static const ParameterInfo params[] = {
            {static_cast<int>(EngineParamID::HARMONICS), "Pitch", "st", 0.5f, 0.0f, 1.0f, false, 0, "Macro"},
            {static_cast<int>(EngineParamID::TIMBRE), "Filter", "", 1.0f, 0.0f, 1.0f, false, 0, "Macro"},
            {static_cast<int>(EngineParamID::MORPH), "Envelope", "", 0.25f, 0.0f, 1.0f, false, 0, "Macro"},
            {static_cast<int>(EngineParamID::LPF_CUTOFF), "LPF", "Hz", 0.8f, 0.0f, 1.0f, false, 0, "Filter"},
            {static_cast<int>(EngineParamID::LPF_RESONANCE), "Resonance", "", 0.3f, 0.0f, 1.0f, false, 0, "Filter"},
            {static_cast<int>(EngineParamID::DRIVE), "Drive", "", 0.1f, 0.0f, 1.0f, false, 0, "Channel"},
            {static_cast<int>(EngineParamID::COMPRESSOR), "Comp", "", 0.0f, 0.0f, 1.0f, false, 0, "Channel"},
            {static_cast<int>(EngineParamID::VOLUME), "Level", "dB", 0.8f, 0.0f, 1.0f, false, 0, "Output"}
        };
        
        return (index >= 0 && index < 8) ? &params[index] : nullptr;
    }
    
private:
    std::array<SamplerKit::Pad, 25> pads_;
    
    // Engine-specific parameters (hero macros)
    float pitchMacro_ = 0.0f;      // ±24 semitones
    float filterMacro_ = 1.0f;     // Filter scaling factor
    float envMacro_ = 1.0f;        // Envelope time scaling
    
    int countVoicesForPad(int pad) const {
        int count = 0;
        for (const auto& voice : voices_) {
            if (voice->getPad() == pad && voice->isActive()) {
                count++;
            }
        }
        return count;
    }
    
    void stealOldestVoiceForPad(int pad) {
        SamplerKitVoice* oldestVoice = nullptr;
        uint32_t oldestAge = 0;
        
        for (auto& voice : voices_) {
            if (voice->getPad() == pad && voice->isActive()) {
                uint32_t age = voice->getAge();
                if (!oldestVoice || age > oldestAge) {
                    oldestVoice = voice.get();
                    oldestAge = age;
                }
            }
        }
        
        if (oldestVoice) {
            oldestVoice->noteOff();
        }
    }
};