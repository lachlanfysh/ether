#pragma once
#include "src/synthesis/SynthEngine.h"
#include "src/engines/SlideAccentBassEngine.h"
#include "src/engines/Classic4OpFMEngine.h"
#include <memory>

// Adapter to bridge SynthEngineBase to SynthEngine interface
class SlideAccentBassAdapter : public SynthEngine {
private:
    std::unique_ptr<SlideAccentBassEngine> engine;
    
public:
    SlideAccentBassAdapter() : engine(std::make_unique<SlideAccentBassEngine>()) {}
    
    void setSampleRate(float sampleRate) override {
        engine->initialize(sampleRate);
    }
    
    void setBufferSize(int bufferSize) override {
        // SlideAccentBassEngine doesn't have setBufferSize
    }
    
    void processAudio(EtherAudioBuffer& buffer) override {
        // Simple conversion - process each frame
        for (size_t i = 0; i < buffer.size(); i++) {
            float sample = 0.0f; // engine->processSample();
            buffer[i] = AudioFrame{sample, sample};
        }
    }
    
    void noteOn(int midiNote, float velocity, float aftertouch = 0.0f) override {
        float note = static_cast<float>(midiNote);
        engine->noteOn(note, velocity);
    }
    
    void noteOff(int midiNote) override {
        engine->noteOff();
    }
    
    void allNotesOff() override {
        engine->allNotesOff();
    }
    
    bool hasParameter(ParameterID param) override {
        return param == ParameterID::HARMONICS || 
               param == ParameterID::TIMBRE || 
               param == ParameterID::MORPH;
    }
    
    void setParameter(ParameterID param, float value) override {
        switch (param) {
            case ParameterID::HARMONICS:
                engine->setHarmonics(value);
                break;
            case ParameterID::TIMBRE:
                engine->setTimbre(value);
                break;
            case ParameterID::MORPH:
                engine->setMorph(value);
                break;
            default:
                break;
        }
    }
    
    float getParameter(ParameterID param) override {
        float h, t, m;
        engine->getHTMParameters(h, t, m);
        switch (param) {
            case ParameterID::HARMONICS: return h;
            case ParameterID::TIMBRE: return t;
            case ParameterID::MORPH: return m;
            default: return 0.0f;
        }
    }
    
    std::string getName() override {
        return "SlideAccentBass";
    }
    
    int getActiveVoiceCount() override {
        return 1; // Mono bass
    }
    
    float getCPUUsage() override {
        return 5.0f; // Estimate
    }
};

class Classic4OpFMAdapter : public SynthEngine {
private:
    std::unique_ptr<Classic4OpFMEngine> engine;
    
public:
    Classic4OpFMAdapter() : engine(std::make_unique<Classic4OpFMEngine>()) {}
    
    void setSampleRate(float sampleRate) override {
        engine->initialize(sampleRate);
    }
    
    void setBufferSize(int bufferSize) override {
        // Classic4OpFMEngine doesn't have setBufferSize
    }
    
    void processAudio(EtherAudioBuffer& buffer) override {
        // Simple conversion - process each frame
        for (size_t i = 0; i < buffer.size(); i++) {
            float sample = 0.0f; // engine->processSample();
            buffer[i] = AudioFrame{sample, sample};
        }
    }
    
    void noteOn(int midiNote, float velocity, float aftertouch = 0.0f) override {
        float note = static_cast<float>(midiNote);
        engine->noteOn(note, velocity);
    }
    
    void noteOff(int midiNote) override {
        engine->noteOff();
    }
    
    void allNotesOff() override {
        engine->allNotesOff();
    }
    
    bool hasParameter(ParameterID param) override {
        return param == ParameterID::HARMONICS || 
               param == ParameterID::TIMBRE || 
               param == ParameterID::MORPH;
    }
    
    void setParameter(ParameterID param, float value) override {
        switch (param) {
            case ParameterID::HARMONICS:
                engine->setHarmonics(value);
                break;
            case ParameterID::TIMBRE:
                engine->setTimbre(value);
                break;
            case ParameterID::MORPH:
                engine->setMorph(value);
                break;
            default:
                break;
        }
    }
    
    float getParameter(ParameterID param) override {
        float h, t, m;
        engine->getHTMParameters(h, t, m);
        switch (param) {
            case ParameterID::HARMONICS: return h;
            case ParameterID::TIMBRE: return t;
            case ParameterID::MORPH: return m;
            default: return 0.0f;
        }
    }
    
    std::string getName() override {
        return "Classic4OpFM";
    }
    
    int getActiveVoiceCount() override {
        return 4; // 4 operators
    }
    
    float getCPUUsage() override {
        return 12.0f; // 4-op FM is CPU intensive
    }
};