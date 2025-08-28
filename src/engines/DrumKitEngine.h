#pragma once
#include "../BaseEngine.h"
#include "../DSPUtils.h"
#include <array>

namespace DrumKit {

/**
 * Individual drum synthesizer models
 */
class DrumSynthesizer {
public:
    enum class DrumType {
        KICK, SNARE, HIHAT_CLOSED, HIHAT_OPEN, CLAP, CRASH, 
        RIDE, TOM_HIGH, TOM_MID, TOM_LOW, RIMSHOT, COWBELL
    };
    
    DrumSynthesizer(DrumType type) : type_(type), sampleRate_(48000.0f) {}
    
    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        envelope_.setSampleRate(sampleRate);
        noiseEnv_.setSampleRate(sampleRate);
        
        // Configure envelope based on drum type
        switch (type_) {
            case DrumType::KICK:
                envelope_.setAttackTime(0.001f);
                envelope_.setDecayTime(0.3f);
                envelope_.setSustainLevel(0.0f);
                envelope_.setReleaseTime(0.1f);
                baseFreq_ = 60.0f;
                break;
                
            case DrumType::SNARE:
                envelope_.setAttackTime(0.001f);
                envelope_.setDecayTime(0.15f);
                envelope_.setSustainLevel(0.0f);
                envelope_.setReleaseTime(0.05f);
                noiseEnv_.setAttackTime(0.001f);
                noiseEnv_.setDecayTime(0.1f);
                noiseEnv_.setSustainLevel(0.0f);
                noiseEnv_.setReleaseTime(0.05f);
                baseFreq_ = 200.0f;
                break;
                
            case DrumType::HIHAT_CLOSED:
                envelope_.setAttackTime(0.001f);
                envelope_.setDecayTime(0.08f);
                envelope_.setSustainLevel(0.0f);
                envelope_.setReleaseTime(0.02f);
                baseFreq_ = 8000.0f;
                break;
                
            case DrumType::HIHAT_OPEN:
                envelope_.setAttackTime(0.001f);
                envelope_.setDecayTime(0.4f);
                envelope_.setSustainLevel(0.0f);
                envelope_.setReleaseTime(0.1f);
                baseFreq_ = 6000.0f;
                break;
                
            case DrumType::CLAP:
                envelope_.setAttackTime(0.001f);
                envelope_.setDecayTime(0.12f);
                envelope_.setSustainLevel(0.0f);
                envelope_.setReleaseTime(0.03f);
                baseFreq_ = 1000.0f;
                break;
                
            default:
                envelope_.setAttackTime(0.001f);
                envelope_.setDecayTime(0.2f);
                envelope_.setSustainLevel(0.0f);
                envelope_.setReleaseTime(0.05f);
                baseFreq_ = 440.0f;
                break;
        }
    }
    
    void setTuning(float tuning) {
        tuning_ = std::clamp(tuning, 0.1f, 4.0f);
    }
    
    void setDecay(float decay) {
        decay_ = std::clamp(decay, 0.1f, 4.0f);
        // Adjust envelope decay based on parameter
        float decayTime = envelope_.getDecayTime() * decay_;
        envelope_.setDecayTime(decayTime);
        if (type_ == DrumType::SNARE) {
            noiseEnv_.setDecayTime(decayTime * 0.7f);
        }
    }
    
    void setVariation(float variation) {
        variation_ = std::clamp(variation, 0.0f, 1.0f);
    }
    
    void trigger(float velocity) {
        velocity_ = velocity;
        envelope_.noteOn();
        if (type_ == DrumType::SNARE || type_ == DrumType::CLAP) {
            noiseEnv_.noteOn();
        }
        
        // Apply variation to frequency and timing
        float freqVariation = 1.0f + (random_.uniform(-1.0f, 1.0f) * variation_ * 0.2f);
        currentFreq_ = baseFreq_ * tuning_ * freqVariation;
        
        phase_ = 0.0f;
        clickPhase_ = 0.0f;
    }
    
    float process() {
        float env = envelope_.process();
        if (env <= 0.001f) return 0.0f;
        
        float sample = 0.0f;
        
        switch (type_) {
            case DrumType::KICK:
                sample = generateKick(env);
                break;
                
            case DrumType::SNARE:
                sample = generateSnare(env);
                break;
                
            case DrumType::HIHAT_CLOSED:
            case DrumType::HIHAT_OPEN:
                sample = generateHihat(env);
                break;
                
            case DrumType::CLAP:
                sample = generateClap(env);
                break;
                
            case DrumType::CRASH:
            case DrumType::RIDE:
                sample = generateCymbal(env);
                break;
                
            default:
                sample = generateTom(env);
                break;
        }
        
        return sample * velocity_;
    }
    
private:
    DrumType type_;
    float sampleRate_;
    float tuning_ = 1.0f;
    float decay_ = 1.0f;
    float variation_ = 0.0f;
    float velocity_ = 1.0f;
    float baseFreq_ = 440.0f;
    float currentFreq_ = 440.0f;
    float phase_ = 0.0f;
    float clickPhase_ = 0.0f;
    
    DSP::ADSR envelope_;
    DSP::ADSR noiseEnv_;
    DSP::Random random_;
    
    float generateKick(float env) {
        // Sine wave with rapid frequency sweep for kick
        float freqSweep = currentFreq_ * (1.0f + env * 2.0f);
        phase_ += freqSweep * 2.0f * M_PI / sampleRate_;
        if (phase_ >= 2.0f * M_PI) phase_ -= 2.0f * M_PI;
        
        float sine = std::sin(phase_);
        
        // Add click for attack
        clickPhase_ += 8000.0f * 2.0f * M_PI / sampleRate_;
        if (clickPhase_ >= 2.0f * M_PI) clickPhase_ -= 2.0f * M_PI;
        float click = std::sin(clickPhase_) * std::exp(-env * 50.0f);
        
        return (sine + click * 0.3f) * env;
    }
    
    float generateSnare(float env) {
        // Tone component
        phase_ += currentFreq_ * 2.0f * M_PI / sampleRate_;
        if (phase_ >= 2.0f * M_PI) phase_ -= 2.0f * M_PI;
        float tone = std::sin(phase_) * 0.4f;
        
        // Noise component
        float noiseEnvValue = noiseEnv_.process();
        float noise = random_.uniform(-1.0f, 1.0f) * noiseEnvValue * 0.8f;
        
        return (tone + noise) * env;
    }
    
    float generateHihat(float env) {
        // High-frequency filtered noise
        float noise = random_.uniform(-1.0f, 1.0f);
        
        // Simple highpass filtering
        static float hp1 = 0.0f, hp2 = 0.0f;
        float filtered = noise - hp1;
        hp1 = hp2;
        hp2 = noise;
        
        return filtered * env * 0.6f;
    }
    
    float generateClap(float env) {
        float noiseEnvValue = noiseEnv_.process();
        float noise = random_.uniform(-1.0f, 1.0f) * noiseEnvValue;
        
        // Multiple envelope peaks for clap effect
        float clapEnv = env;
        if (env > 0.8f) clapEnv *= 1.5f;
        else if (env > 0.6f) clapEnv *= 0.7f;
        else if (env > 0.4f) clapEnv *= 1.2f;
        
        return noise * clapEnv * 0.7f;
    }
    
    float generateCymbal(float env) {
        // Multiple high-frequency oscillators
        float sample = 0.0f;
        
        for (int i = 0; i < 6; ++i) {
            float freq = currentFreq_ * (1.0f + i * 0.31f);
            float localPhase = phase_ * (1.0f + i * 0.31f);
            sample += std::sin(localPhase) * (1.0f / (i + 1));
        }
        
        phase_ += currentFreq_ * 2.0f * M_PI / sampleRate_;
        if (phase_ >= 2.0f * M_PI) phase_ -= 2.0f * M_PI;
        
        // Add shimmer
        float noise = random_.uniform(-0.1f, 0.1f);
        
        return (sample + noise) * env * 0.3f;
    }
    
    float generateTom(float env) {
        // Sine wave with frequency sweep
        float freqSweep = currentFreq_ * (1.0f + env * 0.5f);
        phase_ += freqSweep * 2.0f * M_PI / sampleRate_;
        if (phase_ >= 2.0f * M_PI) phase_ -= 2.0f * M_PI;
        
        return std::sin(phase_) * env;
    }
};

} // namespace DrumKit

/**
 * DrumKit Voice - 12-slot drum machine voice
 */
class DrumKitVoice : public BaseVoice {
public:
    DrumKitVoice() : currentSlot_(0), tuning_(1.0f), decay_(1.0f), variation_(0.0f) {
        // Initialize 12 drum synthesizers
        drums_[0] = std::make_unique<DrumKit::DrumSynthesizer>(DrumKit::DrumSynthesizer::DrumType::KICK);
        drums_[1] = std::make_unique<DrumKit::DrumSynthesizer>(DrumKit::DrumSynthesizer::DrumType::SNARE);
        drums_[2] = std::make_unique<DrumKit::DrumSynthesizer>(DrumKit::DrumSynthesizer::DrumType::HIHAT_CLOSED);
        drums_[3] = std::make_unique<DrumKit::DrumSynthesizer>(DrumKit::DrumSynthesizer::DrumType::HIHAT_OPEN);
        drums_[4] = std::make_unique<DrumKit::DrumSynthesizer>(DrumKit::DrumSynthesizer::DrumType::CLAP);
        drums_[5] = std::make_unique<DrumKit::DrumSynthesizer>(DrumKit::DrumSynthesizer::DrumType::CRASH);
        drums_[6] = std::make_unique<DrumKit::DrumSynthesizer>(DrumKit::DrumSynthesizer::DrumType::RIDE);
        drums_[7] = std::make_unique<DrumKit::DrumSynthesizer>(DrumKit::DrumSynthesizer::DrumType::TOM_HIGH);
        drums_[8] = std::make_unique<DrumKit::DrumSynthesizer>(DrumKit::DrumSynthesizer::DrumType::TOM_MID);
        drums_[9] = std::make_unique<DrumKit::DrumSynthesizer>(DrumKit::DrumSynthesizer::DrumType::TOM_LOW);
        drums_[10] = std::make_unique<DrumKit::DrumSynthesizer>(DrumKit::DrumSynthesizer::DrumType::RIMSHOT);
        drums_[11] = std::make_unique<DrumKit::DrumSynthesizer>(DrumKit::DrumSynthesizer::DrumType::COWBELL);
    }
    
    void setSampleRate(float sampleRate) override {
        BaseVoice::setSampleRate(sampleRate);
        for (auto& drum : drums_) {
            if (drum) drum->init(sampleRate);
        }
    }
    
    void noteOn(float note, float velocity) override {
        BaseVoice::noteOn(note, velocity);
        
        // Map MIDI note to drum slot (C3 = slot 0, C#3 = slot 1, etc.)
        int drumSlot = static_cast<int>(note) % 12;
        currentSlot_ = drumSlot;
        
        if (drums_[drumSlot]) {
            drums_[drumSlot]->setTuning(tuning_);
            drums_[drumSlot]->setDecay(decay_);
            drums_[drumSlot]->setVariation(variation_);
            drums_[drumSlot]->trigger(velocity);
        }
        
        ampEnv_.noteOn();
    }
    
    void noteOff() override {
        BaseVoice::noteOff();
        // Drums don't typically respond to note off
    }
    
    float renderSample(const RenderContext& ctx) override {
        if (!active_) return 0.0f;
        
        float envelope = ampEnv_.process();
        
        if (envelope <= 0.001f && releasing_) {
            active_ = false;
            return 0.0f;
        }
        
        // Process current drum slot
        float sample = 0.0f;
        if (drums_[currentSlot_]) {
            sample = drums_[currentSlot_]->process();
        }
        
        // Apply envelope
        sample *= envelope;
        
        // Apply channel strip
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
    void setTuning(float tuning) {
        tuning_ = tuning;
        for (auto& drum : drums_) {
            if (drum) drum->setTuning(tuning_);
        }
    }
    
    void setDecay(float decay) {
        decay_ = decay;
        for (auto& drum : drums_) {
            if (drum) drum->setDecay(decay_);
        }
    }
    
    void setVariation(float variation) {
        variation_ = variation;
        for (auto& drum : drums_) {
            if (drum) drum->setVariation(variation_);
        }
    }
    
    uint32_t getAge() const { return 0; }  // TODO: Implement proper age tracking
    
private:
    std::array<std::unique_ptr<DrumKit::DrumSynthesizer>, 12> drums_;
    int currentSlot_;
    
    // Voice parameters
    float tuning_;    // HARMONICS -> drum tuning
    float decay_;     // TIMBRE -> decay time
    float variation_; // MORPH -> synthesis variation
};

/**
 * DrumKit Engine - 12-slot drum machine synthesis
 */
class DrumKitEngine : public PolyphonicBaseEngine<DrumKitVoice> {
public:
    DrumKitEngine() 
        : PolyphonicBaseEngine("DrumKit", "DRUM", 
                              static_cast<int>(EngineFactory::EngineType::DRUM_KIT),
                              CPUClass::Medium, 12) {}  // 12 voices for 12 drum slots
    
    // IEngine interface
    const char* getName() const override { return "DrumKit"; }
    const char* getShortName() const override { return "DRUM"; }
    
    void setParam(int paramID, float v01) override {
        BaseEngine::setParam(paramID, v01);  // Handle common parameters
        
        switch (static_cast<EngineParamID>(paramID)) {
            case EngineParamID::HARMONICS:
                // HARMONICS controls drum tuning
                tuning_ = 0.5f + v01 * 1.5f;  // Range: 0.5x to 2.0x
                for (auto& voice : voices_) {
                    voice->setTuning(tuning_);
                }
                break;
                
            case EngineParamID::TIMBRE:
                // TIMBRE controls decay time
                decay_ = 0.2f + v01 * 3.8f;   // Range: 0.2x to 4.0x
                for (auto& voice : voices_) {
                    voice->setDecay(decay_);
                }
                break;
                
            case EngineParamID::MORPH:
                // MORPH controls synthesis variation
                variation_ = v01;              // Range: 0.0 to 1.0
                for (auto& voice : voices_) {
                    voice->setVariation(variation_);
                }
                break;
                
            default:
                break;
        }
    }
    
    // Parameter metadata
    int getParameterCount() const override { return 5; }
    
    const ParameterInfo* getParameterInfo(int index) const override {
        static const ParameterInfo params[] = {
            {static_cast<int>(EngineParamID::HARMONICS), "Tuning", "", 0.33f, 0.0f, 1.0f, false, 0, "Drum"},
            {static_cast<int>(EngineParamID::TIMBRE), "Decay", "", 0.4f, 0.0f, 1.0f, false, 0, "Drum"},
            {static_cast<int>(EngineParamID::MORPH), "Variation", "", 0.2f, 0.0f, 1.0f, false, 0, "Drum"},
            {static_cast<int>(EngineParamID::LPF_CUTOFF), "Filter", "Hz", 0.8f, 0.0f, 1.0f, false, 0, "Filter"},
            {static_cast<int>(EngineParamID::DRIVE), "Drive", "", 0.1f, 0.0f, 1.0f, false, 0, "Channel"}
        };
        
        return (index >= 0 && index < 5) ? &params[index] : nullptr;
    }
    
private:
    // Engine-specific parameters
    float tuning_ = 1.0f;      // Drum tuning multiplier
    float decay_ = 1.0f;       // Decay time multiplier
    float variation_ = 0.2f;   // Synthesis variation amount
};