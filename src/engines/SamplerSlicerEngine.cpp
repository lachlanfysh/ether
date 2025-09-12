#include "SamplerSlicerEngine.h"
#include <cmath>

SamplerSlicerEngine::SamplerSlicerEngine() 
    : sampleRate_(44100.0f), initialized_(false), active_(false), 
      harmonics_(0.5f), timbre_(0.0f), morph_(0.0f), cpuUsage_(0.0f) {
}

SamplerSlicerEngine::~SamplerSlicerEngine() {
    shutdown();
}

bool SamplerSlicerEngine::initialize(float sampleRate) {
    if (initialized_) {
        return true;
    }
    
    sampleRate_ = sampleRate;
    initialized_ = true;
    return true;
}

void SamplerSlicerEngine::shutdown() {
    if (!initialized_) {
        return;
    }
    
    allNotesOff();
    initialized_ = false;
}

void SamplerSlicerEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    active_ = true;
}

void SamplerSlicerEngine::noteOff(uint8_t note) {
    active_ = false;
}

void SamplerSlicerEngine::setAftertouch(uint8_t note, float aftertouch) {
    // Not supported
}

void SamplerSlicerEngine::allNotesOff() {
    active_ = false;
}

void SamplerSlicerEngine::setParameter(ParameterID param, float value) {
    switch (param) {
        case ParameterID::HARMONICS:
            harmonics_ = value;
            break;
        case ParameterID::TIMBRE:
            timbre_ = value;
            break;
        case ParameterID::MORPH:
            morph_ = value;
            break;
        default:
            break;
    }
}

float SamplerSlicerEngine::getParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::HARMONICS:
            return harmonics_;
        case ParameterID::TIMBRE:
            return timbre_;
        case ParameterID::MORPH:
            return morph_;
        default:
            return 0.0f;
    }
}

bool SamplerSlicerEngine::hasParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::HARMONICS:
        case ParameterID::TIMBRE:
        case ParameterID::MORPH:
            return true;
        default:
            return false;
    }
}

void SamplerSlicerEngine::processAudio(EtherAudioBuffer& outputBuffer) {
    if (!initialized_) {
        outputBuffer.fill(AudioFrame(0.0f, 0.0f));
        return;
    }
    
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        float sample = processSample();
        outputBuffer[i] = AudioFrame(sample, sample);
    }
}

size_t SamplerSlicerEngine::getActiveVoiceCount() const {
    return active_ ? 1 : 0;
}

void SamplerSlicerEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
    actualSize = 0;
    if (maxSize < sizeof(float) * 3) {
        return;
    }
    
    float* floatData = reinterpret_cast<float*>(data);
    floatData[0] = harmonics_;
    floatData[1] = timbre_;
    floatData[2] = morph_;
    actualSize = sizeof(float) * 3;
}

bool SamplerSlicerEngine::loadPreset(const uint8_t* data, size_t size) {
    if (size < sizeof(float) * 3) {
        return false;
    }
    
    const float* floatData = reinterpret_cast<const float*>(data);
    harmonics_ = floatData[0];
    timbre_ = floatData[1];
    morph_ = floatData[2];
    return true;
}

void SamplerSlicerEngine::setSampleRate(float sampleRate) {
    if (sampleRate_ != sampleRate) {
        shutdown();
        initialize(sampleRate);
    }
}

void SamplerSlicerEngine::setBufferSize(size_t bufferSize) {
    // Buffer size changes handled automatically
}

float SamplerSlicerEngine::getCPUUsage() const {
    return cpuUsage_;
}

float SamplerSlicerEngine::processSample() {
    if (!active_) {
        return 0.0f;
    }
    
    // Simple oscillator for testing
    static float phase = 0.0f;
    float freq = 440.0f * (1.0f + harmonics_);
    phase += freq / sampleRate_;
    if (phase >= 1.0f) phase -= 1.0f;
    
    return std::sin(phase * 2.0f * M_PI) * 0.1f;
}