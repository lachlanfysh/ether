#include "SubtractiveEngine.h"
#include <iostream>
#include <chrono>
#include <cstring>

SubtractiveEngine::SubtractiveEngine() {
    std::cout << "SubtractiveEngine created" << std::endl;
    
    // Initialize modulation array
    modulation_.fill(0.0f);
    
    // Set up default parameters
    for (auto& voice : voices_) {
        voice.setEnvelopeParams(attack_, decay_, sustain_, release_);
        voice.setFilterCutoff(filterCutoff_);
        voice.setFilterResonance(filterResonance_);
        voice.setOscMix(oscMix_);
        voice.setVolume(volume_);
    }
}

SubtractiveEngine::~SubtractiveEngine() {
    allNotesOff();
    std::cout << "SubtractiveEngine destroyed" << std::endl;
}

void SubtractiveEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    SubtractiveVoice* voice = findFreeVoice();
    if (!voice) {
        voice = stealVoice();
    }
    
    if (voice) {
        voice->noteOn(note, velocity, aftertouch, sampleRate_);
        voiceCounter_++;
        
        std::cout << "SubtractiveEngine: Note ON " << static_cast<int>(note) 
                  << " vel=" << velocity << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void SubtractiveEngine::noteOff(uint8_t note) {
    SubtractiveVoice* voice = findVoice(note);
    if (voice) {
        voice->noteOff();
        
        std::cout << "SubtractiveEngine: Note OFF " << static_cast<int>(note) 
                  << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void SubtractiveEngine::setAftertouch(uint8_t note, float aftertouch) {
    SubtractiveVoice* voice = findVoice(note);
    if (voice) {
        voice->setAftertouch(aftertouch);
    }
}

void SubtractiveEngine::allNotesOff() {
    for (auto& voice : voices_) {
        voice.noteOff();
    }
}

void SubtractiveEngine::setParameter(ParameterID param, float value) {
    switch (param) {
        case ParameterID::FILTER_CUTOFF:
            filterCutoff_ = std::clamp(value, 20.0f, 20000.0f);
            updateAllVoices();
            break;
            
        case ParameterID::FILTER_RESONANCE:
            filterResonance_ = std::clamp(value, 0.1f, 10.0f);
            updateAllVoices();
            break;
            
        case ParameterID::OSC_MIX:
            oscMix_ = std::clamp(value, 0.0f, 1.0f);
            updateAllVoices();
            break;
            
        case ParameterID::VOLUME:
            volume_ = std::clamp(value, 0.0f, 1.0f);
            updateAllVoices();
            break;
            
        case ParameterID::ATTACK:
            attack_ = std::clamp(value, 0.001f, 5.0f);
            updateAllVoices();
            break;
            
        case ParameterID::DECAY:
            decay_ = std::clamp(value, 0.001f, 5.0f);
            updateAllVoices();
            break;
            
        case ParameterID::SUSTAIN:
            sustain_ = std::clamp(value, 0.0f, 1.0f);
            updateAllVoices();
            break;
            
        case ParameterID::RELEASE:
            release_ = std::clamp(value, 0.001f, 5.0f);
            updateAllVoices();
            break;
            
        default:
            // Parameter not supported by this engine
            break;
    }
}

float SubtractiveEngine::getParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::FILTER_CUTOFF: return filterCutoff_;
        case ParameterID::FILTER_RESONANCE: return filterResonance_;
        case ParameterID::OSC_MIX: return oscMix_;
        case ParameterID::VOLUME: return volume_;
        case ParameterID::ATTACK: return attack_;
        case ParameterID::DECAY: return decay_;
        case ParameterID::SUSTAIN: return sustain_;
        case ParameterID::RELEASE: return release_;
        default: return 0.0f;
    }
}

bool SubtractiveEngine::hasParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::FILTER_CUTOFF:
        case ParameterID::FILTER_RESONANCE:
        case ParameterID::OSC_MIX:
        case ParameterID::VOLUME:
        case ParameterID::ATTACK:
        case ParameterID::DECAY:
        case ParameterID::SUSTAIN:
        case ParameterID::RELEASE:
            return true;
        default:
            return false;
    }
}

void SubtractiveEngine::processAudio(EtherAudioBuffer& outputBuffer) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Clear output buffer
    for (auto& frame : outputBuffer) {
        frame.left = 0.0f;
        frame.right = 0.0f;
    }
    
    // Process all active voices
    size_t activeVoices = 0;
    for (auto& voice : voices_) {
        if (voice.isActive()) {
            activeVoices++;
            
            for (size_t i = 0; i < BUFFER_SIZE; i++) {
                AudioFrame voiceFrame = voice.processSample();
                outputBuffer[i] += voiceFrame;
            }
        }
    }
    
    // Apply voice scaling to prevent clipping
    if (activeVoices > 1) {
        float scale = 0.8f / std::sqrt(static_cast<float>(activeVoices));
        for (auto& frame : outputBuffer) {
            frame = frame * scale;
        }
    }
    
    // Update CPU usage
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    float processingTime = duration.count() / 1000.0f; // Convert to milliseconds
    updateCPUUsage(processingTime);
}

size_t SubtractiveEngine::getActiveVoiceCount() const {
    size_t count = 0;
    for (const auto& voice : voices_) {
        if (voice.isActive()) {
            count++;
        }
    }
    return count;
}

void SubtractiveEngine::setVoiceCount(size_t maxVoices) {
    // Voice count is fixed for this simple implementation
}

void SubtractiveEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
    // Simple preset save - just store parameter values
    struct PresetData {
        float filterCutoff;
        float filterResonance;
        float oscMix;
        float volume;
        float attack;
        float decay;
        float sustain;
        float release;
    };
    
    PresetData preset = {
        filterCutoff_, filterResonance_, oscMix_, volume_,
        attack_, decay_, sustain_, release_
    };
    
    actualSize = sizeof(PresetData);
    if (maxSize >= actualSize) {
        memcpy(data, &preset, actualSize);
    }
}

bool SubtractiveEngine::loadPreset(const uint8_t* data, size_t size) {
    struct PresetData {
        float filterCutoff;
        float filterResonance;
        float oscMix;
        float volume;
        float attack;
        float decay;
        float sustain;
        float release;
    };
    
    if (size != sizeof(PresetData)) {
        return false;
    }
    
    const PresetData* preset = reinterpret_cast<const PresetData*>(data);
    
    filterCutoff_ = preset->filterCutoff;
    filterResonance_ = preset->filterResonance;
    oscMix_ = preset->oscMix;
    volume_ = preset->volume;
    attack_ = preset->attack;
    decay_ = preset->decay;
    sustain_ = preset->sustain;
    release_ = preset->release;
    
    updateAllVoices();
    return true;
}

void SubtractiveEngine::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updateAllVoices();
}

void SubtractiveEngine::setBufferSize(size_t bufferSize) {
    bufferSize_ = bufferSize;
}

bool SubtractiveEngine::supportsModulation(ParameterID target) const {
    return hasParameter(target);
}

void SubtractiveEngine::setModulation(ParameterID target, float amount) {
    size_t index = static_cast<size_t>(target);
    if (index < modulation_.size()) {
        modulation_[index] = amount;
        // Apply modulation would happen in updateAllVoices()
    }
}

SubtractiveEngine::SubtractiveVoice* SubtractiveEngine::findFreeVoice() {
    for (auto& voice : voices_) {
        if (!voice.isActive()) {
            return &voice;
        }
    }
    return nullptr;
}

SubtractiveEngine::SubtractiveVoice* SubtractiveEngine::findVoice(uint8_t note) {
    for (auto& voice : voices_) {
        if (voice.isActive() && voice.getNote() == note) {
            return &voice;
        }
    }
    return nullptr;
}

SubtractiveEngine::SubtractiveVoice* SubtractiveEngine::stealVoice() {
    // Find oldest voice
    SubtractiveVoice* oldest = nullptr;
    uint32_t oldestAge = 0;
    
    for (auto& voice : voices_) {
        if (voice.getAge() > oldestAge) {
            oldestAge = voice.getAge();
            oldest = &voice;
        }
    }
    
    return oldest;
}

void SubtractiveEngine::updateCPUUsage(float processingTime) {
    float maxTime = (BUFFER_SIZE / sampleRate_) * 1000.0f; // Available time in ms
    cpuUsage_ = std::min(100.0f, (processingTime / maxTime) * 100.0f);
}

void SubtractiveEngine::updateAllVoices() {
    for (auto& voice : voices_) {
        voice.setFilterCutoff(filterCutoff_);
        voice.setFilterResonance(filterResonance_);
        voice.setOscMix(oscMix_);
        voice.setVolume(volume_);
        voice.setEnvelopeParams(attack_, decay_, sustain_, release_);
    }
}

// SubtractiveVoice implementation
SubtractiveEngine::SubtractiveVoice::SubtractiveVoice() {
    filter_.sampleRate = 48000.0f;
    filter_.updateCoefficients();
    envelope_.sampleRate = 48000.0f;
}

void SubtractiveEngine::SubtractiveVoice::noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate) {
    note_ = note;
    velocity_ = velocity;
    aftertouch_ = aftertouch;
    active_ = true;
    age_ = 0;
    
    // Calculate note frequency
    noteFrequency_ = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    
    // Set oscillator frequencies
    osc1_.setFrequency(noteFrequency_, sampleRate);
    osc2_.setFrequency(noteFrequency_ * 1.005f, sampleRate); // Slight detune
    
    // Update filter sample rate
    filter_.sampleRate = sampleRate;
    filter_.updateCoefficients();
    
    // Update envelope sample rate
    envelope_.sampleRate = sampleRate;
    
    // Trigger envelope
    envelope_.noteOn();
}

void SubtractiveEngine::SubtractiveVoice::noteOff() {
    envelope_.noteOff();
}

void SubtractiveEngine::SubtractiveVoice::setAftertouch(float aftertouch) {
    aftertouch_ = aftertouch;
}

AudioFrame SubtractiveEngine::SubtractiveVoice::processSample() {
    if (!active_) {
        return AudioFrame(0.0f, 0.0f);
    }
    
    age_++;
    
    // Generate oscillator output
    float osc1Out = osc1_.processSaw();
    float osc2Out = osc2_.processSine();
    
    // Mix oscillators
    float mixed = osc1Out * (1.0f - oscMix_) + osc2Out * oscMix_;
    
    // Apply filter
    float filtered = filter_.process(mixed);
    
    // Apply envelope
    float envLevel = envelope_.process();
    
    // Check if voice should be deactivated
    if (!envelope_.isActive()) {
        active_ = false;
    }
    
    // Apply velocity and volume
    float output = filtered * envLevel * velocity_ * volume_;
    
    return AudioFrame(output, output); // Mono output copied to both channels
}

void SubtractiveEngine::SubtractiveVoice::setFilterCutoff(float cutoff) {
    filter_.setCutoff(cutoff);
}

void SubtractiveEngine::SubtractiveVoice::setFilterResonance(float resonance) {
    filter_.setResonance(resonance);
}

void SubtractiveEngine::SubtractiveVoice::setOscMix(float mix) {
    oscMix_ = mix;
}

void SubtractiveEngine::SubtractiveVoice::setVolume(float volume) {
    volume_ = volume;
}

void SubtractiveEngine::SubtractiveVoice::setEnvelopeParams(float attack, float decay, float sustain, float release) {
    envelope_.attack = attack;
    envelope_.decay = decay;
    envelope_.sustain = sustain;
    envelope_.release = release;
}