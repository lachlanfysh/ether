#pragma once
#include "IEngine.h"
#include "DSPUtils.h"
#include "../core/ErrorHandler.h"
#include "../audio/SIMDOptimizations.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstring>

/**
 * Channel Strip implementation for per-voice processing
 * HPF -> LPF(SVF) -> Comp -> Drive -> TiltEQ
 */
class ChannelStrip {
public:
    enum Mode { Clean, Dirty }; // Clean: Core->HPF->LPF->Comp->Drive->TiltEQ
                               // Dirty: Core->Drive->HPF->LPF->Comp->TiltEQ
    
    ChannelStrip();
    
    void setSampleRate(float sampleRate);
    void setMode(Mode mode) { mode_ = mode; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    
    // Parameter setters (normalized 0..1)
    void setHPFCutoff(float cutoff01);      // 10-1000 Hz (log)
    void setLPFCutoff(float cutoff01);      // 200 Hz-18 kHz (log)  
    void setLPFResonance(float res01);      // 0-0.90 (gain-comp)
    void setFilterKeytrack(float track01);  // 0-100% tracking
    void setFilterEnvAmount(float amt);     // -1..+1 (center=0)
    void setCompAmount(float comp01);       // 0-1 one-knob compressor
    void setPunch(float punch01);           // 0-1 transient enhancer
    void setDrive(float drive01);           // 0-1 asym soft-clip
    void setDriveTone(float tone01);        // 0-1 odd/even emphasis
    void setBody(float body);               // -6..+6 dB low shelf ~120 Hz
    void setAir(float air);                 // -6..+6 dB high shelf ~6-8 kHz
    
    // Filter envelope controls
    void setFilterADSR(float attack_ms, float decay_ms, float sustain, float release_ms);
    void noteOn(float note, float velocity);
    void noteOff();
    
    // Processing
    float process(float input, float keytrackNote = 60.0f);
    void processBlock(const float* input, float* output, int count, float keytrackNote = 60.0f);
    
    void reset();
    
private:
    Mode mode_;
    bool enabled_;
    float sampleRate_;
    
    // Filters
    DSP::SVF hpf_, lpf_;
    
    // Filter envelope
    DSP::ADSR filterEnv_;
    DSP::SmoothParam filterCutoff_, filterRes_, filterEnvAmt_;
    float baseFilterCutoff_, keytrackAmount_;
    
    // Compressor (simplified one-knob)
    float compAmount_, compAttack_, compRelease_;
    float compEnvelope_, compGainReduction_;
    
    // Transient enhancer  
    float punch_;
    float transientState_;
    
    // Drive/saturation
    DSP::SmoothParam driveAmount_, driveTone_;
    DSP::Audio::DCBlocker dcBlocker_;
    
    // Tilt EQ (shelving filters)
    DSP::SVF bodyShelf_, airShelf_;
    DSP::SmoothParam bodyGain_, airGain_;
    
    // Internal processing helpers
    float processCompressor(float input);
    float processTransientEnhancer(float input);
    float processDrive(float input);
    float processTiltEQ(float input);
};

/**
 * Base engine class providing common functionality
 */
class BaseEngine : public IEngine {
public:
    BaseEngine(const char* name, const char* shortName, int engineID, CPUClass cpuClass);
    virtual ~BaseEngine() = default;
    
    // IEngine interface - common implementations
    void prepare(double sampleRate, int maxBlockSize) override;
    void reset() override;
    
    const char* getName() const override { return name_; }
    const char* getShortName() const override { return shortName_; }
    int getEngineID() const override { return engineID_; }
    CPUClass getCPUClass() const override { return cpuClass_; }
    
    void setParam(int paramID, float v01) override;
    void setMod(int paramID, float value, float depth) override;
    
    bool isStereo() const override { return false; } // Most engines are mono
    
    // Parameter metadata - derived classes should override
    int getParameterCount() const override;
    const ParameterInfo* getParameterInfo(int index) const override;
    uint32_t getModDestinations() const override;
    const HapticInfo* getHapticInfo(int paramID) const override;
    
protected:
    const char* name_;
    const char* shortName_;
    int engineID_;
    CPUClass cpuClass_;
    
    double sampleRate_;
    int maxBlockSize_;
    
    // Common parameters with smoothing
    DSP::SmoothParam harmonics_, timbre_, morph_, level_;
    DSP::SmoothParam extra1_, extra2_;
    
    // Channel strip (per voice in derived classes)
    std::unique_ptr<ChannelStrip> channelStrip_;
    bool stripEnabled_;
    
    // Parameter storage
    std::unordered_map<int, float> parameters_;
    std::unordered_map<int, float> modulations_;
    
    // Voice management helpers
    struct VoiceContext {
        uint32_t id;
        float note;
        float velocity;
        bool active;
        bool releasing;
        uint32_t startTime;
        
        VoiceContext() : id(0), note(60.0f), velocity(0.8f), 
                        active(false), releasing(false), startTime(0) {}
    };
    
    std::vector<VoiceContext> voices_;
    uint32_t nextVoiceID_;
    uint32_t currentTime_;
    
    // Voice management
    VoiceContext* findVoice(uint32_t id);
    VoiceContext* allocateVoice();
    void deallocateVoice(uint32_t id);
    VoiceContext* stealVoice(); // LRU voice stealing
    
    // Parameter utilities
    float getParam(int paramID, float defaultValue = 0.0f) const;
    float getModulatedParam(int paramID, float baseValue) const;
    
    // Common parameter ranges and conversions
    static float logScale(float value01, float min, float max);
    static float expScale(float value01, float min, float max);
    static float linearScale(float value01, float min, float max);
    
    // Update smoothed parameters
    void updateSmoothParams();
    
    // Default parameter info tables
    static const ParameterInfo commonParams_[];
    static const HapticInfo commonHaptics_[];
    static const int commonParamCount_;
};

/**
 * Voice base class for polyphonic engines
 */
class BaseVoice {
public:
    BaseVoice() : note_(60.0f), velocity_(0.8f), active_(false), releasing_(false) {
        channelStrip_ = std::make_unique<ChannelStrip>();
    }
    
    virtual ~BaseVoice() = default;
    
    // Voice lifecycle
    virtual void noteOn(float note, float velocity);
    virtual void noteOff();
    virtual void kill() { active_ = false; }
    
    // State
    bool isActive() const { return active_; }
    bool isReleasing() const { return releasing_; }
    float getNote() const { return note_; }
    float getVelocity() const { return velocity_; }
    
    // Processing - derived classes must implement
    virtual float renderSample(const RenderContext& ctx) = 0;
    virtual void renderBlock(const RenderContext& ctx, float* output, int count) = 0;
    
    // Common setup
    virtual void setSampleRate(float sampleRate);
    virtual void reset();
    
protected:
    float note_, velocity_;
    bool active_, releasing_;
    
    // Each voice has its own channel strip
    std::unique_ptr<ChannelStrip> channelStrip_;
    
    // Common DSP elements
    DSP::ADSR ampEnv_;
    DSP::Random rng_;
};

/**
 * Template for creating polyphonic engines
 */
template<typename VoiceType>
class PolyphonicBaseEngine : public BaseEngine {
public:
    PolyphonicBaseEngine(const char* name, const char* shortName, int engineID, 
                        CPUClass cpuClass, int maxVoices = 8)
        : BaseEngine(name, shortName, engineID, cpuClass), maxVoices_(maxVoices) {
        voices_.reserve(maxVoices);
        for (int i = 0; i < maxVoices; ++i) {
            voices_.emplace_back(std::make_unique<VoiceType>());
        }
    }
    
    void noteOn(float note, float velocity, uint32_t id) override {
        VoiceType* voice = findAvailableVoice();
        if (!voice) {
            voice = stealVoice();
        }
        
        if (voice) {
            voice->noteOn(note, velocity);
            assignVoiceID(voice, id, note, velocity);
        }
    }
    
    void noteOff(uint32_t id) override {
        VoiceType* voice = findVoiceByID(id);
        if (voice) {
            voice->noteOff();
        }
    }
    
    void render(const RenderContext& ctx, float* out, int n) override {
        // SIMD-optimized buffer clearing
        EtherSynth::SIMD::clearBuffer(out, n);
        
        // Collect active voices and their buffers for cache-friendly processing
        int activeCount = 0;
        VoiceType* activeVoices[32]; // Max voices
        float* voiceBuffers[32];
        
        for (auto& voice : voices_) {
            if (voice->isActive()) {
                activeVoices[activeCount] = voice.get();
                voiceBuffers[activeCount] = voiceOutputBuffers_[activeCount].data();
                activeCount++;
            }
        }
        
        // Render all active voices to separate buffers
        for (int voiceIdx = 0; voiceIdx < activeCount; ++voiceIdx) {
            activeVoices[voiceIdx]->renderBlock(ctx, voiceBuffers[voiceIdx], n);
        }
        
        // SIMD-optimized voice accumulation
        if (activeCount > 0) {
            EtherSynth::SIMD::accumulateVoices(out, voiceBuffers, activeCount, n);
        }
    }
    
    void prepare(double sampleRate, int maxBlockSize) override {
        BaseEngine::prepare(sampleRate, maxBlockSize);
        tempBuffer_.resize(maxBlockSize);
        
        // Initialize SIMD optimization buffers
        voiceOutputBuffers_.resize(maxVoices_);
        for (auto& buffer : voiceOutputBuffers_) {
            buffer.resize(maxBlockSize);
        }
        
        for (auto& voice : voices_) {
            voice->setSampleRate(static_cast<float>(sampleRate));
        }
    }
    
    void reset() override {
        BaseEngine::reset();
        for (auto& voice : voices_) {
            voice->reset();
        }
        voiceMap_.clear();
    }
    
protected:
    std::vector<std::unique_ptr<VoiceType>> voices_;
    std::vector<float> tempBuffer_;
    std::unordered_map<uint32_t, VoiceType*> voiceMap_; // ID -> voice mapping
    int maxVoices_;
    
    // SIMD optimization: separate buffers for each voice
    std::vector<std::vector<float>> voiceOutputBuffers_;
    
    VoiceType* findAvailableVoice() {
        for (auto& voice : voices_) {
            if (!voice->isActive()) {
                return voice.get();
            }
        }
        return nullptr;
    }
    
    VoiceType* findVoiceByID(uint32_t id) {
        auto it = voiceMap_.find(id);
        return (it != voiceMap_.end()) ? it->second : nullptr;
    }
    
    VoiceType* stealVoice() {
        // Find oldest releasing voice, or oldest active voice
        VoiceType* candidate = nullptr;
        uint32_t oldestTime = UINT32_MAX;
        
        for (auto& voice : voices_) {
            if (voice->isReleasing() && voice->getAge() < oldestTime) {
                candidate = voice.get();
                oldestTime = voice->getAge();
            }
        }
        
        if (!candidate) {
            for (auto& voice : voices_) {
                if (voice->getAge() < oldestTime) {
                    candidate = voice.get();
                    oldestTime = voice->getAge();
                }
            }
        }
        
        return candidate;
    }
    
    void assignVoiceID(VoiceType* voice, uint32_t id, float note, float velocity) {
        // Remove old mapping if voice was already assigned
        for (auto it = voiceMap_.begin(); it != voiceMap_.end(); ++it) {
            if (it->second == voice) {
                voiceMap_.erase(it);
                break;
            }
        }
        
        voiceMap_[id] = voice;
    }
};