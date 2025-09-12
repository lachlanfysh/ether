#pragma once
#include "../synthesis/SynthEngine.h"
#include "Classic4OpFMEngine.h"
#include <memory>

/**
 * Wrapper to adapt Classic4OpFMEngine to the SynthEngine interface
 */
class Classic4OpFMEngineWrapper : public SynthEngine {
private:
    std::unique_ptr<Classic4OpFMEngine> engine_;
    EngineType type_;
    float sampleRate_;
    
    // H/T/M parameter storage
    float harmonics_ = 0.5f;
    float timbre_ = 0.5f; 
    float morph_ = 0.5f;
    
public:
    Classic4OpFMEngineWrapper() 
        : engine_(std::make_unique<Classic4OpFMEngine>())
        , type_(EngineType::CLASSIC_4OP_FM)
        , sampleRate_(48000.0f) {}
    
    ~Classic4OpFMEngineWrapper() override = default;
    
    // Engine identification
    EngineType getType() const override {
        return type_;
    }
    
    const char* getName() const override {
        return "Classic4OpFM";
    }
    
    const char* getDescription() const override {
        return "4-operator FM synthesis with 8 curated algorithms";
    }
    
    // Note events - adapt SynthEngine interface to Classic4OpFMEngine
    void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f) override {
        float floatNote = static_cast<float>(note);
        float floatVel = velocity; // Keep 0-1 range
        engine_->noteOn(floatNote, floatVel);
    }
    
    void noteOff(uint8_t note) override {
        engine_->noteOff();
    }
    
    void setAftertouch(uint8_t note, float aftertouch) override {
        // Classic4OpFM doesn't have per-note aftertouch, ignore
    }
    
    void allNotesOff() override {
        engine_->allNotesOff();
    }
    
    // Parameter control - map to H/T/M
    void setParameter(ParameterID param, float value) override {
        switch (param) {
            case ParameterID::HARMONICS:
                harmonics_ = value;
                engine_->setHarmonics(value);
                break;
            case ParameterID::TIMBRE:
                timbre_ = value;
                engine_->setTimbre(value);
                break;
            case ParameterID::MORPH:
                morph_ = value;
                engine_->setMorph(value);
                break;
            default:
                // Ignore unsupported parameters
                break;
        }
    }
    
    float getParameter(ParameterID param) const override {
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
    
    bool hasParameter(ParameterID param) const override {
        return param == ParameterID::HARMONICS || 
               param == ParameterID::TIMBRE || 
               param == ParameterID::MORPH;
    }
    
    // Audio processing - uses Classic4OpFM's processSampleStereo method
    void processAudio(EtherAudioBuffer& outputBuffer) override {
        // Clear buffer first
        for (auto& frame : outputBuffer) {
            frame = AudioFrame{0.0f, 0.0f};
        }
        
        // Process samples through the 4OP FM engine
        for (size_t i = 0; i < outputBuffer.size(); i++) {
            auto stereoSample = engine_->processSampleStereo();
            outputBuffer[i] = AudioFrame{stereoSample.first, stereoSample.second};
        }
    }
    
    // Voice management - 4OP FM is typically monophonic but can have 4 operators
    size_t getActiveVoiceCount() const override {
        return engine_ ? 4 : 0; // 4 operators
    }
    
    size_t getMaxVoiceCount() const override {
        return 4; // 4 operators maximum
    }
    
    void setVoiceCount(size_t maxVoices) override {
        // 4OP FM has fixed 4 operators - ignore voice count changes
    }
    
    // Performance monitoring
    float getCPUUsage() const override {
        return 15.0f; // 4-operator FM is CPU intensive
    }
    
    // Preset management - basic implementation
    void savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const override {
        if (maxSize >= sizeof(float) * 3) {
            float* presetData = reinterpret_cast<float*>(data);
            presetData[0] = harmonics_;
            presetData[1] = timbre_;
            presetData[2] = morph_;
            actualSize = sizeof(float) * 3;
        } else {
            actualSize = 0;
        }
    }
    
    bool loadPreset(const uint8_t* data, size_t size) override {
        if (size >= sizeof(float) * 3) {
            const float* presetData = reinterpret_cast<const float*>(data);
            setParameter(ParameterID::HARMONICS, presetData[0]);
            setParameter(ParameterID::TIMBRE, presetData[1]);
            setParameter(ParameterID::MORPH, presetData[2]);
            return true;
        }
        return false;
    }
    
    // Engine-specific configuration
    void setSampleRate(float sampleRate) override {
        sampleRate_ = sampleRate;
        if (engine_) {
            engine_->initialize(sampleRate);
        }
    }
    
    void setBufferSize(size_t bufferSize) override {
        // Classic4OpFMEngine doesn't have setBufferSize
    }
    
    // Access to underlying engine for specialized functionality
    Classic4OpFMEngine* getEngine() { return engine_.get(); }
    const Classic4OpFMEngine* getEngine() const { return engine_.get(); }
};