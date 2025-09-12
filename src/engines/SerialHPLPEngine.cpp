#include "SerialHPLPEngine.h"
#include <cmath>

SerialHPLPEngine::SerialHPLPEngine() 
    : sampleRate_(44100.0f), initialized_(false), active_(false), 
      harmonics_(0.5f), timbre_(0.5f), morph_(0.5f), cpuUsage_(0.0f) {
}

SerialHPLPEngine::~SerialHPLPEngine() {
    shutdown();
}

bool SerialHPLPEngine::initialize(float sampleRate) {
    if (initialized_) {
        return true;
    }
    
    sampleRate_ = sampleRate;
    initialized_ = true;
    return true;
}

void SerialHPLPEngine::shutdown() {
    if (!initialized_) {
        return;
    }
    
    allNotesOff();
    initialized_ = false;
}

void SerialHPLPEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    active_ = true;
}

void SerialHPLPEngine::noteOff(uint8_t note) {
    active_ = false;
}

void SerialHPLPEngine::setAftertouch(uint8_t note, float aftertouch) {
    // Not supported
}

void SerialHPLPEngine::allNotesOff() {
    active_ = false;
}

void SerialHPLPEngine::setParameter(ParameterID param, float value) {
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

float SerialHPLPEngine::getParameter(ParameterID param) const {
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

bool SerialHPLPEngine::hasParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::HARMONICS:
        case ParameterID::TIMBRE:
        case ParameterID::MORPH:
            return true;
        default:
            return false;
    }
}

void SerialHPLPEngine::processAudio(EtherAudioBuffer& outputBuffer) {
    if (!initialized_) {
        outputBuffer.fill(AudioFrame(0.0f, 0.0f));
        return;
    }
    
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        float sample = processSample();
        outputBuffer[i] = AudioFrame(sample, sample);
    }
}

size_t SerialHPLPEngine::getActiveVoiceCount() const {
    return active_ ? 1 : 0;
}

void SerialHPLPEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
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

bool SerialHPLPEngine::loadPreset(const uint8_t* data, size_t size) {
    if (size < sizeof(float) * 3) {
        return false;
    }
    
    const float* floatData = reinterpret_cast<const float*>(data);
    harmonics_ = floatData[0];
    timbre_ = floatData[1];
    morph_ = floatData[2];
    return true;
}

void SerialHPLPEngine::setSampleRate(float sampleRate) {
    if (sampleRate_ != sampleRate) {
        shutdown();
        initialize(sampleRate);
    }
}

void SerialHPLPEngine::setBufferSize(size_t bufferSize) {
    // Buffer size changes handled automatically
}

float SerialHPLPEngine::getCPUUsage() const {
    return cpuUsage_;
}

float SerialHPLPEngine::processSample() {
    if (!active_) {
        return 0.0f;
    }
    
    // Simple dual oscillator with detuning
    static float phase1 = 0.0f;
    static float phase2 = 0.0f;
    
    float freq1 = 440.0f * (1.0f + harmonics_);
    float freq2 = freq1 * (1.0f + timbre_ * 0.1f); // Slight detune
    
    phase1 += freq1 / sampleRate_;
    phase2 += freq2 / sampleRate_;
    
    if (phase1 >= 1.0f) phase1 -= 1.0f;
    if (phase2 >= 1.0f) phase2 -= 1.0f;
    
    float osc1 = std::sin(phase1 * 2.0f * M_PI);
    float osc2 = std::sin(phase2 * 2.0f * M_PI);
    
    return (osc1 + osc2) * 0.05f * (1.0f + morph_);
}