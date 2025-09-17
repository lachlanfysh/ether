#pragma once
#include "../core/DataStructures.h"
#include "../core/DIContainer.h"
#include "../../core/Types.h"  // For ParameterID
#include <string>

namespace GridSequencer {
namespace Audio {

using Core::Result;
using Core::ParamRoute;

// Audio engine interface - abstracts the EtherSynth bridge
class IAudioEngine {
public:
    virtual ~IAudioEngine() = default;

    // Engine management
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;

    // Audio processing
    virtual void processAudio(float* outputBuffer, size_t bufferSize) = 0;
    virtual void play() = 0;
    virtual void stop() = 0;

    // Instrument management
    virtual Result<bool> setInstrumentEngineType(int instrument, int engineType) = 0;
    virtual Result<int> getInstrumentEngineType(int instrument) = 0;
    virtual Result<std::string> getEngineTypeName(int engineType) = 0;
    virtual int getEngineTypeCount() = 0;

    // Parameter management
    virtual Result<bool> setParameter(int instrument, ParameterID param, float value) = 0;
    virtual Result<float> getParameter(int instrument, ParameterID param) = 0;
    virtual bool hasParameter(int instrument, ParameterID param) = 0;
    virtual ParamRoute getParameterRoute(int instrument, ParameterID param) = 0;

    // Note triggering
    virtual Result<bool> noteOn(int keyIndex, float velocity, float aftertouch = 0.0f) = 0;
    virtual Result<bool> noteOff(int keyIndex) = 0;
    virtual void allNotesOff() = 0;

    // Engine state
    virtual void setActiveInstrument(int instrument) = 0;
    virtual int getActiveInstrument() = 0;
    virtual int getActiveVoiceCount() = 0;
    virtual float getCPUUsage() = 0;
    virtual float getMemoryUsageKB() = 0;

    // Master controls
    virtual void setMasterVolume(float volume) = 0;
    virtual float getMasterVolume() = 0;

    // Voice management
    virtual void setEngineVoiceCount(int instrument, int voices) = 0;
    virtual int getEngineVoiceCount(int instrument) = 0;

    // Effects
    virtual void setEngineFXSend(int instrument, int which, float value) = 0;
    virtual float getEngineFXSend(int instrument, int which) = 0;
    virtual void setGlobalFX(int which, int param, float value) = 0;
    virtual float getGlobalFX(int which, int param) = 0;

    // LFO system
    virtual void setLFORate(unsigned char lfoId, float rate) = 0;
    virtual void setLFODepth(unsigned char lfoId, float depth) = 0;
    virtual void setLFOWaveform(unsigned char lfoId, unsigned char waveform) = 0;
    virtual void triggerInstrumentLFOs(int instrument) = 0;
};

} // namespace Audio
} // namespace GridSequencer