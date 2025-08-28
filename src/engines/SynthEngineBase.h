#pragma once
#include <string>

/**
 * SynthEngineBase - Base class for all synthesis engines
 * 
 * Provides common interface for H/T/M parameter control,
 * voice management, and preset handling.
 */
class SynthEngineBase {
public:
    virtual ~SynthEngineBase() = default;
    
    // Initialization
    virtual bool initialize(float sampleRate) = 0;
    virtual void shutdown() = 0;
    
    // H/T/M Parameter control (0.0 - 1.0)
    virtual void setHarmonics(float harmonics) = 0;
    virtual void setTimbre(float timbre) = 0;
    virtual void setMorph(float morph) = 0;
    
    virtual void setHTMParameters(float harmonics, float timbre, float morph) = 0;
    virtual void getHTMParameters(float& harmonics, float& timbre, float& morph) const = 0;
    
    // Voice control
    virtual void noteOn(float note, float velocity) = 0;
    virtual void noteOff(float releaseTime = 0.0f) = 0;
    virtual void allNotesOff() = 0;
    
    // Audio processing
    virtual float processSample() = 0;
    virtual void processBlock(float* output, int numSamples) = 0;
    
    // Analysis
    virtual bool isActive() const = 0;
    virtual float getCPUUsage() const = 0;
    
    // Preset management
    virtual void reset() = 0;
    virtual void setPreset(const std::string& presetName) = 0;
};