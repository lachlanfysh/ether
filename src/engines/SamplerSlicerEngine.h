#pragma once
#include "../synthesis/SynthEngine.h"
#include "../core/Types.h"
#include <vector>
#include <memory>

/**
 * SamplerSlicer Engine - Simplified sample slicer engine
 */
class SamplerSlicerEngine : public SynthEngine {
public:
    SamplerSlicerEngine();
    ~SamplerSlicerEngine();
    
    // Initialization
    bool initialize(float sampleRate);
    void shutdown();
    
    // SynthEngine interface implementation
    EngineType getType() const override { return EngineType::SAMPLER_SLICER; }
    const char* getName() const override { return "SamplerSlicer"; }
    const char* getDescription() const override { return "Sample slicer with transient detection"; }
    
    // SynthEngine note methods
    void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f) override;
    void noteOff(uint8_t note) override;
    void setAftertouch(uint8_t note, float aftertouch) override;
    void allNotesOff() override;
    
    // SynthEngine parameter methods
    void setParameter(ParameterID param, float value) override;
    float getParameter(ParameterID param) const override;
    bool hasParameter(ParameterID param) const override;
    
    // SynthEngine audio processing
    void processAudio(EtherAudioBuffer& outputBuffer) override;
    
    // SynthEngine voice management
    size_t getActiveVoiceCount() const override;
    size_t getMaxVoiceCount() const override { return 1; }
    void setVoiceCount(size_t maxVoices) override {}
    
    // SynthEngine preset methods
    void savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const override;
    bool loadPreset(const uint8_t* data, size_t size) override;
    
    // SynthEngine configuration
    void setSampleRate(float sampleRate) override;
    void setBufferSize(size_t bufferSize) override;
    float getCPUUsage() const override;
    
private:
    // Core state
    float sampleRate_;
    bool initialized_;
    bool active_;
    
    // HTM parameters
    float harmonics_ = 0.5f;   // Sensitivity
    float timbre_ = 0.0f;      // Crossfade  
    float morph_ = 0.0f;       // Follow action
    
    // Performance
    float cpuUsage_;
    
    // Simple processing
    float processSample();
};