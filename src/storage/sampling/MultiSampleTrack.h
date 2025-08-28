#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <functional>
#include <array>
#include "AutoSampleLoader.h"

/**
 * MultiSampleTrack - Enhanced sampler tracks supporting multiple concurrent samples
 * 
 * Provides comprehensive multi-sample playback capabilities:
 * - Multiple sample slots per track with individual triggering
 * - Sample layering and velocity-based sample switching
 * - Per-sample parameter control (pitch, gain, pan, etc.)
 * - Real-time sample swapping and hot-loading
 * - Integration with pattern sequencer for complex arrangements
 * - Hardware-optimized for STM32 H7 embedded platform
 * 
 * Features:
 * - Up to 16 sample slots per track with independent control
 * - Velocity-based sample selection and layering modes
 * - Round-robin, random, and velocity-switch sample triggering
 * - Per-sample voice allocation and polyphony management
 * - Real-time sample parameter modulation
 * - Sample crossfading and overlap control
 * - Integration with tape squashing workflow for dynamic loading
 * - Memory-efficient voice management for embedded platform
 */
class MultiSampleTrack {
public:
    // Sample triggering modes
    enum class TriggerMode {
        SINGLE_SHOT,        // Play single sample based on trigger
        VELOCITY_LAYERS,    // Select sample based on velocity ranges
        VELOCITY_CROSSFADE, // Crossfade between samples based on velocity
        ROUND_ROBIN,        // Cycle through available samples
        RANDOM,             // Randomly select from available samples
        STACK_ALL,          // Trigger all samples simultaneously
        CHORD_MODE          // Trigger samples at different pitches for chords
    };
    
    // Sample slot configuration
    struct SampleSlotConfig {
        uint8_t slotId;                    // Sample slot ID (0-15)
        bool isActive;                     // Whether slot is active
        float velocityMin;                 // Minimum velocity for this slot (0.0-1.0)
        float velocityMax;                 // Maximum velocity for this slot (0.0-1.0)
        float gain;                        // Sample gain multiplier
        float pitchOffset;                 // Pitch offset in semitones
        float panPosition;                 // Pan position (-1.0 to 1.0)
        uint8_t priority;                  // Playback priority (0-15)
        bool allowLayering;                // Allow layering with other samples
        float crossfadeAmount;             // Crossfade amount with adjacent velocity layers
        int8_t chordInterval;              // Chord interval in semitones (for chord mode)
        
        SampleSlotConfig() :
            slotId(255),
            isActive(false),
            velocityMin(0.0f),
            velocityMax(1.0f),
            gain(1.0f),
            pitchOffset(0.0f),
            panPosition(0.0f),
            priority(8),
            allowLayering(true),
            crossfadeAmount(0.1f),
            chordInterval(0) {}
    };
    
    // Voice state for active sample playback
    struct SampleVoice {
        uint8_t voiceId;                   // Unique voice ID
        uint8_t sampleSlot;                // Sample slot being played
        bool isActive;                     // Whether voice is currently playing
        float currentGain;                 // Current gain level
        float targetGain;                  // Target gain level
        float currentPitch;                // Current pitch multiplier
        float targetPitch;                 // Target pitch multiplier
        float currentPan;                  // Current pan position
        float targetPan;                   // Target pan position
        uint32_t samplePosition;           // Current sample position
        uint32_t fadeInSamples;            // Fade-in duration in samples
        uint32_t fadeOutSamples;           // Fade-out duration in samples
        bool isLooping;                    // Whether voice is looping
        uint32_t loopStart;                // Loop start position
        uint32_t loopEnd;                  // Loop end position
        
        SampleVoice() :
            voiceId(255),
            sampleSlot(255),
            isActive(false),
            currentGain(0.0f),
            targetGain(1.0f),
            currentPitch(1.0f),
            targetPitch(1.0f),
            currentPan(0.0f),
            targetPan(0.0f),
            samplePosition(0),
            fadeInSamples(0),
            fadeOutSamples(0),
            isLooping(false),
            loopStart(0),
            loopEnd(0) {}
    };
    
    // Track configuration
    struct TrackConfig {
        uint8_t trackId;                   // Track ID
        TriggerMode triggerMode;           // Sample triggering mode
        uint8_t maxPolyphony;              // Maximum concurrent voices
        float masterGain;                  // Master track gain
        float masterPitch;                 // Master track pitch offset
        float masterPan;                   // Master track pan
        bool enableSampleCrossfade;        // Enable crossfading between samples
        uint16_t voiceFadeTimeMs;          // Voice fade in/out time
        uint8_t roundRobinPosition;        // Current round-robin position
        bool enableVelocityScaling;        // Scale sample parameters by velocity
        float pitchBendSensitivity;        // Pitch bend sensitivity in semitones
        
        TrackConfig() :
            trackId(255),
            triggerMode(TriggerMode::SINGLE_SHOT),
            maxPolyphony(4),
            masterGain(1.0f),
            masterPitch(0.0f),
            masterPan(0.0f),
            enableSampleCrossfade(true),
            voiceFadeTimeMs(10),
            roundRobinPosition(0),
            enableVelocityScaling(true),
            pitchBendSensitivity(2.0f) {}
    };
    
    MultiSampleTrack(uint8_t trackId);
    ~MultiSampleTrack() = default;
    
    // Configuration
    void setTrackConfig(const TrackConfig& config);
    const TrackConfig& getTrackConfig() const { return trackConfig_; }
    void setTriggerMode(TriggerMode mode);
    
    // Sample Slot Management
    bool assignSampleToSlot(uint8_t slotId, uint8_t sampleSlotId, const SampleSlotConfig& config);
    bool removeSampleFromSlot(uint8_t slotId);
    void clearAllSamples();
    SampleSlotConfig getSampleSlotConfig(uint8_t slotId) const;
    bool setSampleSlotConfig(uint8_t slotId, const SampleSlotConfig& config);
    
    // Sample Information
    std::vector<uint8_t> getActiveSampleSlots() const;
    bool isSampleSlotActive(uint8_t slotId) const;
    uint8_t getSampleCount() const;
    
    // Playback Control
    void triggerSample(float velocity, float pitch = 1.0f, float pan = 0.0f);
    void stopAllVoices();
    void stopVoice(uint8_t voiceId);
    bool isAnyVoicePlaying() const;
    uint8_t getActiveVoiceCount() const;
    
    // Voice Management
    const std::vector<SampleVoice>& getActiveVoices() const { return voices_; }
    SampleVoice* getVoice(uint8_t voiceId);
    uint8_t allocateVoice(uint8_t sampleSlot, float velocity);
    void releaseVoice(uint8_t voiceId, bool immediate = false);
    
    // Parameter Control
    void setMasterGain(float gain);
    void setMasterPitch(float pitchOffset);
    void setMasterPan(float pan);
    void setSampleSlotGain(uint8_t slotId, float gain);
    void setSampleSlotPitch(uint8_t slotId, float pitchOffset);
    void setSampleSlotPan(uint8_t slotId, float pan);
    
    // Real-time Modulation
    void modulateParameter(uint8_t slotId, const std::string& parameter, float value);
    void setGlobalModulation(const std::string& parameter, float value);
    void updateVoiceParameters();  // Called from audio thread
    
    // Audio Processing (called from audio thread)
    void processAudio(float* outputBuffer, uint32_t sampleCount, uint32_t sampleRate);
    void mixVoiceToBuffer(const SampleVoice& voice, float* buffer, uint32_t sampleCount, uint32_t sampleRate);
    
    // Integration
    void integrateWithAutoSampleLoader(AutoSampleLoader* sampleLoader);
    void integrateWithSequencer(class SequencerEngine* sequencer);
    void setSampleAccessCallback(std::function<const AutoSampleLoader::SamplerSlot&(uint8_t)> callback);
    
    // Hot Loading (dynamic sample replacement)
    bool hotSwapSample(uint8_t slotId, uint8_t newSampleSlotId);
    bool hotLoadSample(uint8_t slotId, std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData);
    void enableHotLoadingMode(bool enabled);
    
    // Sample Analysis and Auto-Configuration
    void analyzeAndConfigureSample(uint8_t slotId, uint8_t sampleSlotId);
    void autoConfigureVelocityLayers();
    void optimizeSlotConfiguration();
    
    // Callbacks
    using VoiceStateChangeCallback = std::function<void(uint8_t voiceId, bool started)>;
    using SampleTriggerCallback = std::function<void(uint8_t slotId, float velocity)>;
    using ParameterChangeCallback = std::function<void(uint8_t slotId, const std::string& param, float value)>;
    
    void setVoiceStateChangeCallback(VoiceStateChangeCallback callback);
    void setSampleTriggerCallback(SampleTriggerCallback callback);
    void setParameterChangeCallback(ParameterChangeCallback callback);
    
    // Memory Management
    size_t getEstimatedMemoryUsage() const;
    void optimizeMemoryUsage();
    
private:
    // Track configuration
    TrackConfig trackConfig_;
    
    // Sample slot configurations
    static constexpr uint8_t MAX_SAMPLE_SLOTS = 16;
    std::array<SampleSlotConfig, MAX_SAMPLE_SLOTS> sampleSlots_;
    
    // Voice management
    static constexpr uint8_t MAX_VOICES = 8;
    std::vector<SampleVoice> voices_;
    uint8_t nextVoiceId_;
    
    // Integration
    AutoSampleLoader* sampleLoader_;
    class SequencerEngine* sequencer_;
    std::function<const AutoSampleLoader::SamplerSlot&(uint8_t)> sampleAccessCallback_;
    
    // Callbacks
    VoiceStateChangeCallback voiceStateChangeCallback_;
    SampleTriggerCallback sampleTriggerCallback_;
    ParameterChangeCallback parameterChangeCallback_;
    
    // Hot loading
    bool hotLoadingEnabled_;
    
    // Internal methods
    uint8_t selectSampleSlot(float velocity, TriggerMode mode);
    std::vector<uint8_t> selectMultipleSampleSlots(float velocity, TriggerMode mode);
    bool isSlotInVelocityRange(uint8_t slotId, float velocity) const;
    float calculateSlotWeight(uint8_t slotId, float velocity) const;
    
    // Voice allocation
    uint8_t findFreeVoice();
    uint8_t stealVoice();  // Steal least important voice
    void initializeVoice(SampleVoice& voice, uint8_t sampleSlot, float velocity);
    void updateVoice(SampleVoice& voice, uint32_t sampleCount, uint32_t sampleRate);
    
    // Audio processing helpers
    float interpolateSample(const std::vector<float>& audioData, float position, uint8_t channels) const;
    void applyCrossfade(float* buffer, uint32_t sampleCount, float fadeIn, float fadeOut);
    float calculateVoiceGain(const SampleVoice& voice, float velocity) const;
    float calculateVoicePitch(const SampleVoice& voice, float pitch) const;
    
    // Parameter smoothing
    void smoothParameter(float& current, float target, float rate);
    uint32_t msToSamples(uint16_t ms, uint32_t sampleRate) const;
    
    // Round-robin helpers
    void advanceRoundRobin();
    uint8_t getRoundRobinSlot() const;
    
    // Auto-configuration helpers
    void detectVelocityRanges(uint8_t slotId);
    void suggestOptimalTriggerMode();
    float analyzeAudioContent(const std::vector<float>& audioData) const;
    
    // Validation helpers
    bool isValidSlotId(uint8_t slotId) const;
    bool isValidVoiceId(uint8_t voiceId) const;
    bool isValidSampleSlotId(uint8_t sampleSlotId) const;
    
    // Notification helpers
    void notifyVoiceStateChange(uint8_t voiceId, bool started);
    void notifySampleTriggered(uint8_t slotId, float velocity);
    void notifyParameterChange(uint8_t slotId, const std::string& parameter, float value);
    
    // Constants
    static constexpr float MIN_VELOCITY = 0.001f;
    static constexpr float MAX_VELOCITY = 1.0f;
    static constexpr float VOICE_STEAL_PRIORITY_THRESHOLD = 0.1f;
    static constexpr uint32_t PARAMETER_SMOOTH_RATE_HZ = 100;
    static constexpr float DEFAULT_CROSSFADE_TIME = 0.010f;  // 10ms
};