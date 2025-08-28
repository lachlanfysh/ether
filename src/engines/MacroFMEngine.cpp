#include "MacroFMEngine.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <cmath>

MacroFMEngine::MacroFMEngine() {
    std::cout << "MacroFM engine created" << std::endl;
    
    // Initialize modulation array
    modulation_.fill(0.0f);
    
    // Calculate initial derived parameters
    calculateDerivedParams();
    
    // Set up default parameters for all voices
    updateAllVoices();
}

MacroFMEngine::~MacroFMEngine() {
    allNotesOff();
    std::cout << "MacroFM engine destroyed" << std::endl;
}

void MacroFMEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    MacroFMVoice* voice = findFreeVoice();
    if (!voice) {
        voice = stealVoice();
    }
    
    if (voice) {
        voice->noteOn(note, velocity, aftertouch, sampleRate_);
        voiceCounter_++;
        
        std::cout << "MacroFM: Note ON " << static_cast<int>(note) 
                  << " vel=" << velocity << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void MacroFMEngine::noteOff(uint8_t note) {
    MacroFMVoice* voice = findVoice(note);
    if (voice) {
        voice->noteOff();
        
        std::cout << "MacroFM: Note OFF " << static_cast<int>(note) 
                  << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void MacroFMEngine::setAftertouch(uint8_t note, float aftertouch) {
    MacroFMVoice* voice = findVoice(note);
    if (voice) {
        voice->setAftertouch(aftertouch);
    }
}

void MacroFMEngine::allNotesOff() {
    for (auto& voice : voices_) {
        voice.noteOff();
    }
}

void MacroFMEngine::setParameter(ParameterID param, float value) {
    switch (param) {
        case ParameterID::HARMONICS:
            setHarmonics(value);
            break;
            
        case ParameterID::TIMBRE:
            setTimbre(value);
            break;
            
        case ParameterID::MORPH:
            setMorph(value);
            break;
            
        case ParameterID::VOLUME:
            volume_ = std::clamp(value, 0.0f, 1.0f);
            updateAllVoices();
            break;
            
        case ParameterID::SUB_LEVEL:
            subLevel_ = std::clamp(value, 0.0f, 1.0f);
            updateAllVoices();
            break;
            
        case ParameterID::SUB_ANCHOR:
            subAnchorEnabled_ = value > 0.5f;
            updateAllVoices();
            break;
            
        case ParameterID::ATTACK:
            attack_ = std::clamp(value, 0.0005f, 5.0f);
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
            break;
    }
}

float MacroFMEngine::getParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::HARMONICS: return harmonics_;
        case ParameterID::TIMBRE: return timbre_;
        case ParameterID::MORPH: return morph_;
        case ParameterID::VOLUME: return volume_;
        case ParameterID::SUB_LEVEL: return subLevel_;
        case ParameterID::SUB_ANCHOR: return subAnchorEnabled_ ? 1.0f : 0.0f;
        case ParameterID::ATTACK: return attack_;
        case ParameterID::DECAY: return decay_;
        case ParameterID::SUSTAIN: return sustain_;
        case ParameterID::RELEASE: return release_;
        default: return 0.0f;
    }
}

bool MacroFMEngine::hasParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::HARMONICS:
        case ParameterID::TIMBRE:
        case ParameterID::MORPH:
        case ParameterID::VOLUME:
        case ParameterID::SUB_LEVEL:
        case ParameterID::SUB_ANCHOR:
        case ParameterID::ATTACK:
        case ParameterID::DECAY:
        case ParameterID::SUSTAIN:
        case ParameterID::RELEASE:
            return true;
        default:
            return false;
    }
}

void MacroFMEngine::processAudio(EtherAudioBuffer& outputBuffer) {
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
    float processingTime = duration.count() / 1000.0f;
    updateCPUUsage(processingTime);
}

void MacroFMEngine::setHarmonics(float harmonics) {
    harmonics_ = std::clamp(harmonics, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroFMEngine::setTimbre(float timbre) {
    timbre_ = std::clamp(timbre, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroFMEngine::setMorph(float morph) {
    morph_ = std::clamp(morph, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroFMEngine::calculateDerivedParams() {
    // HARMONICS: FM index 0-0.8 (expo) + small bright tilt (+1.5 dB @ 2 kHz max)
    fmIndex_ = mapFMIndexExp(harmonics_);
    brightTilt_ = mapBrightTilt(harmonics_);
    
    // TIMBRE: mod ratio set {0.5, 1.0, 1.5, 2.0, 3.0} + sine↔triangle
    modRatio_ = mapModRatio(timbre_);
    sineTriBlend_ = mapSineTriBlend(timbre_);
    
    // MORPH: feedback 0-0.3 + mod-env decay 30 → 6 ms (linked)
    feedback_ = mapFeedback(morph_);
    modEnvDecay_ = mapModEnvDecay(morph_);
}

float MacroFMEngine::mapFMIndexExp(float harmonics) const {
    // Exponential mapping: 0 to 0.8
    return harmonics * harmonics * 0.8f;
}

float MacroFMEngine::mapBrightTilt(float harmonics) const {
    // Small bright tilt: 0 to +1.5 dB @ 2 kHz
    return harmonics * 1.5f;
}

float MacroFMEngine::mapModRatio(float timbre) const {
    // Map to curated ratio set {0.5, 1.0, 1.5, 2.0, 3.0}
    static constexpr float RATIOS[5] = {0.5f, 1.0f, 1.5f, 2.0f, 3.0f};
    
    // Quantized selection
    int index = static_cast<int>(timbre * 4.99f); // 0-4
    index = std::clamp(index, 0, 4);
    
    return RATIOS[index];
}

float MacroFMEngine::mapSineTriBlend(float timbre) const {
    // Continuous blend between sine and triangle
    // Map 0-1 across the full range for smooth morphing
    return timbre;
}

float MacroFMEngine::mapFeedback(float morph) const {
    // Feedback: 0 to 0.3
    return morph * 0.3f;
}

float MacroFMEngine::mapModEnvDecay(float morph) const {
    // Mod-env decay: 30ms → 6ms (linked with feedback)
    // Higher morph = shorter decay + more feedback
    return 0.03f - (morph * 0.024f); // 30ms to 6ms
}

// Voice management methods
MacroFMEngine::MacroFMVoice* MacroFMEngine::findFreeVoice() {
    for (auto& voice : voices_) {
        if (!voice.isActive()) {
            return &voice;
        }
    }
    return nullptr;
}

MacroFMEngine::MacroFMVoice* MacroFMEngine::findVoice(uint8_t note) {
    for (auto& voice : voices_) {
        if (voice.isActive() && voice.getNote() == note) {
            return &voice;
        }
    }
    return nullptr;
}

MacroFMEngine::MacroFMVoice* MacroFMEngine::stealVoice() {
    MacroFMVoice* oldest = nullptr;
    uint32_t oldestAge = 0;
    
    for (auto& voice : voices_) {
        if (voice.getAge() > oldestAge) {
            oldestAge = voice.getAge();
            oldest = &voice;
        }
    }
    
    return oldest;
}

void MacroFMEngine::updateAllVoices() {
    for (auto& voice : voices_) {
        voice.setFMParams(fmIndex_, brightTilt_);
        voice.setModParams(modRatio_, sineTriBlend_);
        voice.setFeedbackParams(feedback_, modEnvDecay_);
        voice.setSubParams(subAnchorEnabled_ ? subLevel_ : 0.0f);
        voice.setVolume(volume_);
        voice.setEnvelopeParams(attack_, decay_, sustain_, release_);
    }
}

// Standard SynthEngine methods
size_t MacroFMEngine::getActiveVoiceCount() const {
    size_t count = 0;
    for (const auto& voice : voices_) {
        if (voice.isActive()) {
            count++;
        }
    }
    return count;
}

void MacroFMEngine::setVoiceCount(size_t maxVoices) {
    // Voice count is fixed for this implementation
}

void MacroFMEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
    struct PresetData {
        float harmonics, timbre, morph, volume, subLevel;
        bool subAnchorEnabled;
        float attack, decay, sustain, release;
    };
    
    PresetData preset = {
        harmonics_, timbre_, morph_, volume_, subLevel_,
        subAnchorEnabled_,
        attack_, decay_, sustain_, release_
    };
    
    actualSize = sizeof(PresetData);
    if (maxSize >= actualSize) {
        memcpy(data, &preset, actualSize);
    }
}

bool MacroFMEngine::loadPreset(const uint8_t* data, size_t size) {
    struct PresetData {
        float harmonics, timbre, morph, volume, subLevel;
        bool subAnchorEnabled;
        float attack, decay, sustain, release;
    };
    
    if (size != sizeof(PresetData)) {
        return false;
    }
    
    const PresetData* preset = reinterpret_cast<const PresetData*>(data);
    
    harmonics_ = preset->harmonics;
    timbre_ = preset->timbre;
    morph_ = preset->morph;
    volume_ = preset->volume;
    subLevel_ = preset->subLevel;
    subAnchorEnabled_ = preset->subAnchorEnabled;
    attack_ = preset->attack;
    decay_ = preset->decay;
    sustain_ = preset->sustain;
    release_ = preset->release;
    
    calculateDerivedParams();
    updateAllVoices();
    return true;
}

void MacroFMEngine::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updateAllVoices();
}

void MacroFMEngine::setBufferSize(size_t bufferSize) {
    bufferSize_ = bufferSize;
}

bool MacroFMEngine::supportsModulation(ParameterID target) const {
    return hasParameter(target);
}

void MacroFMEngine::setModulation(ParameterID target, float amount) {
    size_t index = static_cast<size_t>(target);
    if (index < modulation_.size()) {
        modulation_[index] = amount;
    }
}

void MacroFMEngine::updateCPUUsage(float processingTime) {
    float maxTime = (BUFFER_SIZE / sampleRate_) * 1000.0f;
    cpuUsage_ = std::min(100.0f, (processingTime / maxTime) * 100.0f);
}

// MacroFMVoice implementation
MacroFMEngine::MacroFMVoice::MacroFMVoice() {
    brightFilter_.sampleRate = 48000.0f;
    brightFilter_.updateCoefficients();
    carrierEnv_.sampleRate = 48000.0f;
    modEnv_.sampleRate = 48000.0f;
}

void MacroFMEngine::MacroFMVoice::noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate) {
    note_ = note;
    velocity_ = velocity;
    aftertouch_ = aftertouch;
    active_ = true;
    age_ = 0;
    
    // Calculate note frequency
    noteFrequency_ = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    
    // Set carrier frequency
    carrier_.setFrequency(noteFrequency_, sampleRate);
    
    // Set modulator frequency based on ratio
    modulator_.setFrequency(noteFrequency_ * modRatio_, sampleRate);
    
    // Set sub oscillator frequency
    subOsc_.setFrequency(noteFrequency_, sampleRate);
    
    // Update filter sample rate
    brightFilter_.sampleRate = sampleRate;
    brightFilter_.updateCoefficients();
    
    // Update envelope sample rates
    carrierEnv_.sampleRate = sampleRate;
    modEnv_.sampleRate = sampleRate;
    
    // Set modulator envelope decay
    modEnv_.decay = modEnvDecay_;
    
    // Trigger envelopes
    carrierEnv_.noteOn();
    modEnv_.noteOn();
}

void MacroFMEngine::MacroFMVoice::noteOff() {
    carrierEnv_.noteOff();
    modEnv_.noteOff();
}

void MacroFMEngine::MacroFMVoice::setAftertouch(float aftertouch) {
    aftertouch_ = aftertouch;
}

AudioFrame MacroFMEngine::MacroFMVoice::processSample() {
    if (!active_) {
        return AudioFrame(0.0f, 0.0f);
    }
    
    age_++;
    
    // Process envelopes
    float carrierEnvLevel = carrierEnv_.process();
    float modEnvLevel = modEnv_.process();
    
    // Generate modulator output
    float modOut;
    if (sineTriBlend_ < 0.5f) {
        // More sine
        float sineAmount = 1.0f - (sineTriBlend_ * 2.0f);
        modOut = modulator_.processSine() * sineAmount;
    } else {
        // More triangle
        float triAmount = (sineTriBlend_ - 0.5f) * 2.0f;
        float sineOut = modulator_.processSine();
        float triOut = modulator_.processTriangle();
        modOut = sineOut * (1.0f - triAmount) + triOut * triAmount;
    }
    
    // Apply modulator envelope and index
    float modulation = modOut * modEnvLevel * fmIndex_;
    
    // Set carrier feedback
    carrier_.setFeedback(feedback_);
    
    // Generate carrier output with FM
    float carrierOut = carrier_.processSine(modulation);
    
    // Add sub oscillator if enabled
    float subOut = 0.0f;
    if (subLevel_ > 0.0f) {
        subOut = subOsc_.processSine() * subLevel_;
        // Sub level mapping: 0-1 → -12 to -6 dB
        float subDb = subLevel_ * 6.0f - 12.0f; // -12 to -6 dB
        float subGain = std::pow(10.0f, subDb / 20.0f);
        subOut *= subGain;
    }
    
    // Mix carrier and sub
    float mixed = carrierOut + subOut;
    
    // Apply bright tilt filter
    float tilted = brightFilter_.process(mixed);
    
    // Apply carrier envelope
    float output = tilted * carrierEnvLevel;
    
    // Check if voice should be deactivated
    if (!carrierEnv_.isActive() && !modEnv_.isActive()) {
        active_ = false;
    }
    
    // Apply velocity and volume
    output *= velocity_ * volume_;
    
    return AudioFrame(output, output);
}

void MacroFMEngine::MacroFMVoice::setFMParams(float index, float brightTilt) {
    fmIndex_ = index;
    brightTilt_ = brightTilt;
    brightFilter_.setBrightTilt(brightTilt);
}

void MacroFMEngine::MacroFMVoice::setModParams(float ratio, float sineTriBlend) {
    modRatio_ = ratio;
    sineTriBlend_ = sineTriBlend;
    
    // Update modulator frequency if voice is active
    if (active_) {
        modulator_.setFrequency(noteFrequency_ * modRatio_, brightFilter_.sampleRate);
    }
}

void MacroFMEngine::MacroFMVoice::setFeedbackParams(float feedback, float modEnvDecay) {
    feedback_ = feedback;
    modEnvDecay_ = modEnvDecay;
    modEnv_.decay = modEnvDecay;
}

void MacroFMEngine::MacroFMVoice::setSubParams(float subLevel) {
    subLevel_ = subLevel;
}

void MacroFMEngine::MacroFMVoice::setVolume(float volume) {
    volume_ = volume;
}

void MacroFMEngine::MacroFMVoice::setEnvelopeParams(float attack, float decay, float sustain, float release) {
    carrierEnv_.attack = attack;
    carrierEnv_.decay = decay;
    carrierEnv_.sustain = sustain;
    carrierEnv_.release = release;
    
    // Modulator envelope uses faster settings
    modEnv_.attack = attack * 0.5f; // Faster attack
    modEnv_.sustain = sustain;
    modEnv_.release = release * 0.8f; // Faster release
}

// Envelope implementation
float MacroFMEngine::MacroFMVoice::Envelope::process() {
    const float attackRate = 1.0f / (attack * sampleRate);
    const float decayRate = 1.0f / (decay * sampleRate);
    const float releaseRate = 1.0f / (release * sampleRate);
    
    switch (stage) {
        case Stage::IDLE:
            return 0.0f;
            
        case Stage::ATTACK:
            level += attackRate;
            if (level >= 1.0f) {
                level = 1.0f;
                stage = Stage::DECAY;
            }
            break;
            
        case Stage::DECAY:
            level -= decayRate;
            if (level <= sustain) {
                level = sustain;
                stage = Stage::SUSTAIN;
            }
            break;
            
        case Stage::SUSTAIN:
            level = sustain;
            break;
            
        case Stage::RELEASE:
            level -= releaseRate;
            if (level <= 0.0f) {
                level = 0.0f;
                stage = Stage::IDLE;
            }
            break;
    }
    
    return level;
}

// Bright tilt filter implementation
void MacroFMEngine::MacroFMVoice::BrightTiltFilter::setBrightTilt(float tiltDb) {
    gain = tiltDb;
    updateCoefficients();
}

void MacroFMEngine::MacroFMVoice::BrightTiltFilter::updateCoefficients() {
    // Simple high-shelf filter at 2kHz
    float omega = 2.0f * M_PI * freq / sampleRate;
    float gainLinear = std::pow(10.0f, gain / 20.0f);
    
    // Simplified one-pole high-shelf
    float alpha = std::exp(-omega);
    a0 = gainLinear * (1.0f - alpha);
    a1 = 0.0f;
    b1 = -alpha;
}

float MacroFMEngine::MacroFMVoice::BrightTiltFilter::process(float input) {
    float output = a0 * input + a1 * x1 - b1 * y1;
    
    x1 = input;
    y1 = output;
    
    return output;
}