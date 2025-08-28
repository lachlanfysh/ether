#pragma once
#include "../core/Types.h"
#include "../platform/hardware/HardwareInterface.h"
#include <memory>
#include <atomic>
#include <vector>

class VoiceManager;
class InstrumentSlot;
class Timeline;
class ModulationMatrix;
class EffectsChain;

/**
 * Core audio engine for ether synthesizer
 * Manages all audio processing in real-time
 */
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();
    
    // Initialization
    bool initialize(HardwareInterface* hardware);
    void shutdown();
    
    // Real-time audio processing (called from audio thread)
    void processAudio(EtherAudioBuffer& outputBuffer);
    
    // Instrument management
    InstrumentSlot* getInstrument(InstrumentColor color);
    const InstrumentSlot* getInstrument(InstrumentColor color) const;
    void setActiveInstrument(InstrumentColor color);
    InstrumentColor getActiveInstrument() const { return activeInstrument_; }
    
    // Note events (thread-safe)
    void noteOn(uint8_t keyIndex, float velocity, float aftertouch = 0.0f);
    void noteOff(uint8_t keyIndex);
    void setAftertouch(uint8_t keyIndex, float aftertouch);
    void allNotesOff();
    
    // Parameter control (thread-safe)
    void setParameter(ParameterID param, float value);
    float getParameter(ParameterID param) const;
    void setInstrumentParameter(InstrumentColor instrument, ParameterID param, float value);
    float getInstrumentParameter(InstrumentColor instrument, ParameterID param) const;
    
    // Transport control
    void play();
    void stop();
    void record(bool enable);
    bool isPlaying() const { return isPlaying_; }
    bool isRecording() const { return isRecording_; }
    
    // Tempo and timing
    void setBPM(float bpm);
    float getBPM() const { return bpm_; }
    uint32_t getCurrentStep() const { return currentStep_; }
    uint32_t getCurrentBar() const { return currentBar_; }
    
    // Performance metrics
    float getCPUUsage() const { return cpuUsage_; }
    size_t getActiveVoiceCount() const;
    
    // Master effects
    void setMasterVolume(float volume);
    float getMasterVolume() const { return masterVolume_; }
    
    // Advanced features
    Timeline* getTimeline() { return timeline_.get(); }
    ModulationMatrix* getModulationMatrix() { return modMatrix_.get(); }
    
private:
    // Audio callback (real-time thread)
    void audioCallback(EtherAudioBuffer& buffer);
    
    // Timing and sequencer
    void updateSequencer();
    void triggerStep(uint8_t step);
    
    // Performance monitoring
    void updateCPUUsage(float processingTime);
    
    // Core components
    std::unique_ptr<VoiceManager> voiceManager_;
    std::array<std::unique_ptr<InstrumentSlot>, MAX_INSTRUMENTS> instruments_;
    std::unique_ptr<Timeline> timeline_;
    std::unique_ptr<ModulationMatrix> modMatrix_;
    std::unique_ptr<EffectsChain> masterEffects_;
    
    // Hardware interface
    HardwareInterface* hardware_ = nullptr;
    
    // Transport state
    std::atomic<bool> isPlaying_{false};
    std::atomic<bool> isRecording_{false};
    std::atomic<float> bpm_{120.0f};
    
    // Timing
    uint32_t sampleCounter_ = 0;
    uint32_t samplesPerStep_ = 0;
    uint32_t currentStep_ = 0;
    uint32_t currentBar_ = 0;
    
    // Current state
    std::atomic<InstrumentColor> activeInstrument_{InstrumentColor::CORAL};
    std::atomic<float> masterVolume_{0.8f};
    
    // Performance monitoring
    std::atomic<float> cpuUsage_{0.0f};
    
    // Parameter automation (thread-safe parameter changes)
    struct ParameterChange {
        InstrumentColor instrument;
        ParameterID parameter;
        float value;
        std::atomic<bool> pending{false};
    };
    
    static constexpr size_t MAX_PARAMETER_CHANGES = 256;
    std::array<ParameterChange, MAX_PARAMETER_CHANGES> parameterChanges_;
    std::atomic<size_t> parameterChangeIndex_{0};
    
    void applyParameterChanges();
    void queueParameterChange(InstrumentColor instrument, ParameterID param, float value);
    
    // Audio processing utilities
    void clearBuffer(EtherAudioBuffer& buffer);
    void mixBuffers(EtherAudioBuffer& dest, const EtherAudioBuffer& src, float gain = 1.0f);
    void applyMasterEffects(EtherAudioBuffer& buffer);
    
    // Initialization helpers
    void initializeInstruments();
    void initializeSequencer();
    void calculateTiming();
};