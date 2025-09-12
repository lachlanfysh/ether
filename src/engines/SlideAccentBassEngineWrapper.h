#pragma once
#include "../synthesis/SynthEngine.h"
#include "SlideAccentBassEngine.h"
#include <memory>
#include <iostream>
#include <cmath>

/**
 * Wrapper to adapt SlideAccentBassEngine to the SynthEngine interface
 */
class SlideAccentBassEngineWrapper : public SynthEngine {
private:
    std::unique_ptr<SlideAccentBassEngine> engine_;
    EngineType type_;
    float sampleRate_;
    
    // H/T/M parameter storage
    float harmonics_ = 0.5f;
    float timbre_ = 0.5f; 
    float morph_ = 0.5f;
    
public:
    SlideAccentBassEngineWrapper() 
        : engine_(std::make_unique<SlideAccentBassEngine>())
        , type_(EngineType::SLIDE_ACCENT_BASS)
        , sampleRate_(48000.0f) {
        // std::cout << "SlideAccentBass: Wrapper created successfully" << std::endl;
    }
    
    ~SlideAccentBassEngineWrapper() override = default;
    
    // Engine identification
    EngineType getType() const override {
        return type_;
    }
    
    const char* getName() const override {
        return "SlideAccentBass";
    }
    
    const char* getDescription() const override {
        return "Mono bass with exponential slide and accent system";
    }
    
    // Note events - adapt SynthEngine interface to SlideAccentBassEngine
    void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f) override {
        float floatNote = static_cast<float>(note);
        float floatVel = velocity; // Keep 0-1 range like Classic4OpFM
        // std::cout << "SlideAccentBass: Note ON " << static_cast<int>(note) << " vel=" << floatVel << std::endl;
        engine_->noteOn(floatNote, floatVel);
    }
    
    void noteOff(uint8_t note) override {
        engine_->noteOff();
    }
    
    void setAftertouch(uint8_t note, float aftertouch) override {
        // SlideAccentBass doesn't have per-note aftertouch, ignore
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
    
    // Audio processing - uses SlideAccentBass's processSample method
    void processAudio(EtherAudioBuffer& outputBuffer) override {
        // Clear buffer first
        for (auto& frame : outputBuffer) {
            frame = AudioFrame{0.0f, 0.0f};
        }
        
        // Process samples through the bass engine
        for (size_t i = 0; i < outputBuffer.size(); i++) {
            float sample = engine_->processSample();
            outputBuffer[i] = AudioFrame{sample, sample}; // Mono to stereo
            
            // Debug removed for cleaner output
        }
    }
    
    // Voice management - mono bass has 1 voice
    size_t getActiveVoiceCount() const override {
        return engine_ ? 1 : 0;
    }
    
    size_t getMaxVoiceCount() const override {
        return 1; // Mono bass
    }
    
    void setVoiceCount(size_t maxVoices) override {
        // Mono bass - ignore voice count changes
    }
    
    // Performance monitoring
    float getCPUUsage() const override {
        return 8.0f; // Estimate for mono bass
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
            bool success = engine_->initialize(sampleRate);
            // std::cout << "SlideAccentBass: Initialize " << (success ? "SUCCESS" : "FAILED") << " at " << sampleRate << "Hz" << std::endl;
        }
    }
    
    void setBufferSize(size_t bufferSize) override {
        // SlideAccentBassEngine doesn't have setBufferSize
    }
    
    // Access to underlying engine for specialized functionality
    SlideAccentBassEngine* getEngine() { return engine_.get(); }
    const SlideAccentBassEngine* getEngine() const { return engine_.get(); }
};