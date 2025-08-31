#pragma once
#include "../core/Types.h"
#include "TapeEffectsProcessor.h"
#include "DelayEffect.h"
#include "ReverbEffect.h"
#include "../engines/EngineParameterLayouts.h"
#include <memory>
#include <vector>
#include <array>
#include <string>

/**
 * Professional Effects Chain for EtherSynth V1.0
 * 
 * Comprehensive effects processing with real-time parameter control via 16-key interface:
 * - Professional tape saturation and compression
 * - High-quality reverb with multiple algorithms
 * - Multi-tap delay with filtering and feedback control
 * - LUFS-compliant loudness normalization
 * - Professional limiter for output protection
 * - Real-time performance effects for groovebox workflow
 * 
 * Integration with 960×320 + 2×16 hardware:
 * - Effects accessible via FX overlay
 * - SmartKnob control for effect parameters
 * - Visual feedback with waveform displays
 * - Performance-oriented preset system
 */

namespace EtherSynth {

class ProfessionalEffectsChain {
public:
    enum class EffectType : uint8_t {
        TAPE_SATURATION = 0,    // Analog tape saturation and compression
        DELAY,                  // Multi-tap delay with filtering
        REVERB,                 // High-quality reverb algorithms
        FILTER,                 // Performance filter (HP/LP/BP/Notch)
        BITCRUSH,              // Digital degradation
        CHORUS,                 // Chorus/flanger effects
        PHASER,                // Phaser effect
        COMPRESSOR,            // Dynamics control
        EQ_3BAND,              // 3-band EQ
        DISTORTION,            // Harmonic distortion
        LUFS_NORMALIZER,       // Loudness normalization
        PEAK_LIMITER,          // Output protection limiter
        COUNT
    };
    
    enum class EffectSlot : uint8_t {
        PRE_FILTER = 0,        // Before synthesis filtering
        POST_ENGINE,           // After synthesis engine
        SEND_1,                // Send effect 1 (typically reverb)
        SEND_2,                // Send effect 2 (typically delay)
        MASTER_INSERT,         // Master bus insert
        OUTPUT_PROCESSING,     // Final output stage
        COUNT
    };
    
    struct EffectInstance {
        EffectType type;
        EffectSlot slot;
        bool enabled = true;
        bool bypassed = false;
        float wetDryMix = 1.0f;        // 0.0 = dry, 1.0 = wet
        float inputGain = 1.0f;
        float outputGain = 1.0f;
        
        // Parameter mapping to 16-key interface
        std::array<float, 16> parameters;
        std::array<std::string, 16> parameterNames;
        std::array<std::pair<float, float>, 16> parameterRanges;
        
        // Effect-specific data
        std::unique_ptr<TapeEffectsProcessor> tapeProcessor;
        // std::unique_ptr<DelayEffect> delayProcessor;
        // std::unique_ptr<ReverbEffect> reverbProcessor;
        // Other effect processors...
        
        uint32_t id;               // Unique identifier
        char name[32];             // User-defined name
        
        EffectInstance() {
            parameters.fill(0.0f);
            parameterNames.fill("PARAM");
            parameterRanges.fill({0.0f, 1.0f});
            strcpy(name, "FX");
        }
    };
    
    struct PerformanceEffectState {
        // Real-time performance controls (accessed via FX overlay)
        bool reverbThrow = false;      // Reverb throw active
        bool delayThrow = false;       // Delay throw active
        float filterCutoff = 1.0f;     // Performance filter cutoff
        float filterResonance = 0.0f;  // Performance filter resonance
        int filterType = 0;            // 0=LP, 1=HP, 2=BP, 3=Notch
        
        // Note repeat system
        bool noteRepeatActive = false;
        int noteRepeatDivision = 4;    // 16th notes
        float noteRepeatRate = 16.0f;  // Rate in Hz
        
        // FX sends
        float reverbSend = 0.0f;       // Global reverb send
        float delaySend = 0.0f;        // Global delay send
        
        // Master processing
        bool limiterEnabled = true;
        float lufsTarget = -14.0f;     // LUFS loudness target
        bool autoGainEnabled = false;
    };
    
    ProfessionalEffectsChain();
    ~ProfessionalEffectsChain() = default;
    
    // System management
    void initialize(float sampleRate);
    void setSampleRate(float sampleRate);
    void reset();
    
    // Effect management
    uint32_t addEffect(EffectType type, EffectSlot slot);
    void removeEffect(uint32_t effectId);
    EffectInstance* getEffect(uint32_t effectId);
    std::vector<EffectInstance*> getEffectsInSlot(EffectSlot slot);
    
    // Processing
    void processInstrumentChannel(AudioFrame* buffer, int bufferSize, int instrumentIndex);
    void processMasterBus(AudioFrame* buffer, int bufferSize);
    void processPerformanceEffects(AudioFrame* buffer, int bufferSize);
    
    // Parameter control (16-key interface integration)
    void setEffectParameter(uint32_t effectId, int keyIndex, float value);
    float getEffectParameter(uint32_t effectId, int keyIndex);
    const char* getEffectParameterName(uint32_t effectId, int keyIndex);
    std::pair<float, float> getEffectParameterRange(uint32_t effectId, int keyIndex);
    
    // Performance effects (FX overlay interface)
    void setPerformanceState(const PerformanceEffectState& state);
    const PerformanceEffectState& getPerformanceState() const { return perfState_; }
    void triggerReverbThrow();
    void triggerDelayThrow();
    void setPerformanceFilter(float cutoff, float resonance, int type);
    void toggleNoteRepeat(int division);
    
    // Preset management
    struct EffectPreset {
        char name[32];
        std::vector<EffectInstance> effects;
        PerformanceEffectState performanceState;
        
        EffectPreset() { strcpy(name, "Default"); }
    };
    
    void savePreset(int slot, const char* name);
    bool loadPreset(int slot);
    const EffectPreset* getPreset(int slot) const;
    std::vector<std::string> getPresetNames() const;
    
    // Analysis and metering
    float getReverbLevel() const;
    float getDelayLevel() const;
    float getCompressionReduction() const;
    float getLUFSLevel() const;
    float getPeakLevel() const;
    bool isLimiterActive() const;
    
    // UI integration helpers
    struct EffectDisplayInfo {
        EffectType type;
        bool active;
        float wetDryMix;
        float inputLevel;
        float outputLevel;
        std::string name;
        uint32_t visualColor;      // Color for UI feedback
    };
    
    std::vector<EffectDisplayInfo> getActiveEffectsInfo() const;
    uint32_t getEffectColor(EffectType type) const;
    
    // Hardware optimization
    void setLowLatencyMode(bool enabled);    // Reduce buffer sizes for hardware
    void setCPUUsageLimit(float percentage); // Limit CPU usage for real-time performance
    
private:
    std::vector<std::unique_ptr<EffectInstance>> effects_;
    std::array<EffectPreset, 16> presets_;
    PerformanceEffectState perfState_;
    
    float sampleRate_ = 48000.0f;
    uint32_t nextEffectId_ = 1;
    bool lowLatencyMode_ = true;
    float cpuUsageLimit_ = 0.8f;
    
    // Processing buffers
    std::array<AudioFrame, BUFFER_SIZE> tempBuffer1_;
    std::array<AudioFrame, BUFFER_SIZE> tempBuffer2_;
    std::array<AudioFrame, BUFFER_SIZE> sendBuffer1_;
    std::array<AudioFrame, BUFFER_SIZE> sendBuffer2_;
    
    // Master processing components
    std::unique_ptr<class LUFSNormalizer> lufsNormalizer_;
    std::unique_ptr<class PeakLimiter> peakLimiter_;
    std::unique_ptr<class PerformanceFilter> performanceFilter_;
    std::unique_ptr<class NoteRepeatProcessor> noteRepeatProcessor_;
    
    // Metering
    float reverbLevel_ = 0.0f;
    float delayLevel_ = 0.0f;
    float compressionReduction_ = 0.0f;
    float lufsLevel_ = -14.0f;
    float peakLevel_ = 0.0f;
    bool limiterActive_ = false;
    
    // Parameter mapping helpers
    void initializeEffectParameters(EffectInstance* effect);
    void mapTapeEffectParameters(EffectInstance* effect);
    void mapDelayEffectParameters(EffectInstance* effect);
    void mapReverbEffectParameters(EffectInstance* effect);
    void mapFilterEffectParameters(EffectInstance* effect);
    
    // Processing helpers
    void processEffectSlot(EffectSlot slot, AudioFrame* buffer, int bufferSize);
    void updateMetering(const AudioFrame* buffer, int bufferSize);
    void applySendEffects(AudioFrame* buffer, int bufferSize);
    
    // Performance optimization
    bool shouldBypassEffect(const EffectInstance* effect, float inputLevel) const;
    void optimizeEffectOrder();
    
    // Default presets
    void initializeDefaultPresets();
    void createBasicPreset();
    void createPerformancePreset();
    void createMasteringPreset();
    void createCreativePreset();
};

// Performance effect helper classes
class PerformanceFilter {
public:
    enum Type { LOWPASS, HIGHPASS, BANDPASS, NOTCH };
    
    void setType(Type type) { type_ = type; }
    void setCutoff(float cutoff) { cutoff_ = cutoff; }
    void setResonance(float resonance) { resonance_ = resonance; }
    void setSampleRate(float sr) { sampleRate_ = sr; updateCoefficients(); }
    
    float process(float input);
    void reset();
    
private:
    Type type_ = LOWPASS;
    float cutoff_ = 1000.0f;
    float resonance_ = 0.0f;
    float sampleRate_ = 48000.0f;
    
    // State variables filter coefficients
    float g_, k_, a1_, a2_, a3_;
    float ic1eq_ = 0.0f, ic2eq_ = 0.0f;
    
    void updateCoefficients();
};

class NoteRepeatProcessor {
public:
    void setDivision(int division) { division_ = division; }
    void setRate(float rate) { rate_ = rate; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    void trigger() { triggered_ = true; }
    
    void process(AudioFrame* buffer, int bufferSize);
    void setSampleRate(float sr) { sampleRate_ = sr; }
    
private:
    int division_ = 4;
    float rate_ = 16.0f;
    bool enabled_ = false;
    bool triggered_ = false;
    float sampleRate_ = 48000.0f;
    
    float phase_ = 0.0f;
    float lastTriggerTime_ = 0.0f;
};

// Utility functions for UI
const char* effectTypeToString(ProfessionalEffectsChain::EffectType type);
const char* effectSlotToString(ProfessionalEffectsChain::EffectSlot slot);
uint32_t getEffectTypeColor(ProfessionalEffectsChain::EffectType type);

} // namespace EtherSynth