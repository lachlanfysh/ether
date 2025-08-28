#include "FormantEngine.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <cmath>

// Initialize static vowel data
constexpr FormantEngine::VowelFormants::FormantData FormantEngine::VowelFormants::VOWEL_DATA[5];

FormantEngine::FormantEngine() {
    std::cout << "Formant engine created" << std::endl;
    
    // Initialize modulation array
    modulation_.fill(0.0f);
    
    // Calculate initial derived parameters
    calculateDerivedParams();
    
    // Set up default parameters for all voices
    updateAllVoices();
}

FormantEngine::~FormantEngine() {
    allNotesOff();
    std::cout << "Formant engine destroyed" << std::endl;
}

void FormantEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    FormantVoice* voice = findFreeVoice();
    if (!voice) {
        voice = stealVoice();
    }
    
    if (voice) {
        voice->noteOn(note, velocity, aftertouch, sampleRate_);
        voiceCounter_++;
        
        std::cout << "Formant: Note ON " << static_cast<int>(note) 
                  << " vel=" << velocity << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void FormantEngine::noteOff(uint8_t note) {
    FormantVoice* voice = findVoice(note);
    if (voice) {
        voice->noteOff();
        
        std::cout << "Formant: Note OFF " << static_cast<int>(note) 
                  << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void FormantEngine::setAftertouch(uint8_t note, float aftertouch) {
    FormantVoice* voice = findVoice(note);
    if (voice) {
        voice->setAftertouch(aftertouch);
    }
}

void FormantEngine::allNotesOff() {
    for (auto& voice : voices_) {
        voice.noteOff();
    }
}

void FormantEngine::setParameter(ParameterID param, float value) {
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
            
        case ParameterID::ATTACK:
            attack_ = std::clamp(value, 0.001f, 2.0f);
            updateAllVoices();
            break;
            
        case ParameterID::DECAY:
            decay_ = std::clamp(value, 0.01f, 5.0f);
            updateAllVoices();
            break;
            
        case ParameterID::SUSTAIN:
            sustain_ = std::clamp(value, 0.0f, 1.0f);
            updateAllVoices();
            break;
            
        case ParameterID::RELEASE:
            release_ = std::clamp(value, 0.01f, 5.0f);
            updateAllVoices();
            break;
            
        default:
            break;
    }
}

float FormantEngine::getParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::HARMONICS: return harmonics_;
        case ParameterID::TIMBRE: return timbre_;
        case ParameterID::MORPH: return morph_;
        case ParameterID::VOLUME: return volume_;
        case ParameterID::ATTACK: return attack_;
        case ParameterID::DECAY: return decay_;
        case ParameterID::SUSTAIN: return sustain_;
        case ParameterID::RELEASE: return release_;
        default: return 0.0f;
    }
}

bool FormantEngine::hasParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::HARMONICS:
        case ParameterID::TIMBRE:
        case ParameterID::MORPH:
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

void FormantEngine::processAudio(EtherAudioBuffer& outputBuffer) {
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

void FormantEngine::setHarmonics(float harmonics) {
    harmonics_ = std::clamp(harmonics, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void FormantEngine::setTimbre(float timbre) {
    timbre_ = std::clamp(timbre, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void FormantEngine::setMorph(float morph) {
    morph_ = std::clamp(morph, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void FormantEngine::calculateDerivedParams() {
    // HARMONICS: formant frequency + Q
    vowelFormants_.applyHarmonicsControl(harmonics_);
    
    // TIMBRE: vowel morphing A→E→I→O→U
    vowelFormants_.morphToVowel(timbre_);
    
    // MORPH: breath + consonant
    vocalTract_.calculateFromMorph(morph_);
}

float FormantEngine::mapFormantFreqShift(float harmonics) const {
    // Formant frequency shift: -20% to +50%
    return -0.2f + harmonics * 0.7f;
}

float FormantEngine::mapFormantQ(float harmonics) const {
    // Formant Q: 5 to 25 (higher = sharper formants)
    return 5.0f + harmonics * 20.0f;
}

FormantEngine::VowelFormants::Vowel FormantEngine::mapVowelPosition(float timbre, float& blend) const {
    // Map continuous timbre to vowel positions with blend
    float scaledTimbre = timbre * 4.0f; // 0-4 range
    int vowelIndex = static_cast<int>(scaledTimbre);
    blend = scaledTimbre - vowelIndex;
    
    vowelIndex = std::clamp(vowelIndex, 0, 4);
    return static_cast<VowelFormants::Vowel>(vowelIndex);
}

float FormantEngine::mapBreathNoise(float morph) const {
    // Breath noise: 0 to 0.3
    return morph * 0.3f;
}

float FormantEngine::mapConsonantMix(float morph) const {
    // Consonant mix: 0 to 0.5
    return morph * 0.5f;
}

// VowelFormants implementation
void FormantEngine::VowelFormants::morphToVowel(float timbre) {
    // Map timbre to vowel progression: A → E → I → O → U
    float scaledTimbre = timbre * 4.0f; // 0-4 range
    int vowelIndex = static_cast<int>(scaledTimbre);
    float blend = scaledTimbre - vowelIndex;
    
    vowelIndex = std::clamp(vowelIndex, 0, 3); // Max 3 for interpolation
    
    if (blend < 0.001f) {
        // Exact vowel
        currentFormants = VOWEL_DATA[vowelIndex];
    } else {
        // Interpolate between vowels
        currentFormants = interpolateVowels(VOWEL_DATA[vowelIndex], 
                                          VOWEL_DATA[vowelIndex + 1], blend);
    }
}

void FormantEngine::VowelFormants::applyHarmonicsControl(float harmonics) {
    // Apply formant frequency shift
    float freqShift = -0.2f + harmonics * 0.7f; // -20% to +50%
    currentFormants.f1 *= (1.0f + freqShift);
    currentFormants.f2 *= (1.0f + freqShift);
    currentFormants.f3 *= (1.0f + freqShift);
    
    // Apply Q control
    float qScale = 0.5f + harmonics * 2.0f; // 0.5x to 2.5x
    currentFormants.q1 *= qScale;
    currentFormants.q2 *= qScale;
    currentFormants.q3 *= qScale;
    
    // Clamp to reasonable ranges
    currentFormants.f1 = std::clamp(currentFormants.f1, 200.0f, 1200.0f);
    currentFormants.f2 = std::clamp(currentFormants.f2, 600.0f, 3000.0f);
    currentFormants.f3 = std::clamp(currentFormants.f3, 1500.0f, 4000.0f);
    
    currentFormants.q1 = std::clamp(currentFormants.q1, 3.0f, 30.0f);
    currentFormants.q2 = std::clamp(currentFormants.q2, 5.0f, 40.0f);
    currentFormants.q3 = std::clamp(currentFormants.q3, 8.0f, 50.0f);
}

FormantEngine::VowelFormants::FormantData FormantEngine::VowelFormants::interpolateVowels(
    const FormantData& a, const FormantData& b, float t) const {
    
    FormantData result;
    
    // Linear interpolation of all formant parameters
    result.f1 = a.f1 + t * (b.f1 - a.f1);
    result.f2 = a.f2 + t * (b.f2 - a.f2);
    result.f3 = a.f3 + t * (b.f3 - a.f3);
    
    result.a1 = a.a1 + t * (b.a1 - a.a1);
    result.a2 = a.a2 + t * (b.a2 - a.a2);
    result.a3 = a.a3 + t * (b.a3 - a.a3);
    
    result.q1 = a.q1 + t * (b.q1 - a.q1);
    result.q2 = a.q2 + t * (b.q2 - a.q2);
    result.q3 = a.q3 + t * (b.q3 - a.q3);
    
    return result;
}

// VocalTract implementation
void FormantEngine::VocalTract::calculateFromMorph(float morph) {
    breathNoise = morph * 0.3f;
    consonantMix = morph * 0.5f;
    fricativeFreq = 2000.0f + morph * 4000.0f; // 2kHz to 6kHz
    voicing = 1.0f - morph * 0.4f; // Reduce voicing with more consonants
}

// Voice management methods
FormantEngine::FormantVoice* FormantEngine::findFreeVoice() {
    for (auto& voice : voices_) {
        if (!voice.isActive()) {
            return &voice;
        }
    }
    return nullptr;
}

FormantEngine::FormantVoice* FormantEngine::findVoice(uint8_t note) {
    for (auto& voice : voices_) {
        if (voice.isActive() && voice.getNote() == note) {
            return &voice;
        }
    }
    return nullptr;
}

FormantEngine::FormantVoice* FormantEngine::stealVoice() {
    FormantVoice* oldest = nullptr;
    uint32_t oldestAge = 0;
    
    for (auto& voice : voices_) {
        if (voice.getAge() > oldestAge) {
            oldestAge = voice.getAge();
            oldest = &voice;
        }
    }
    
    return oldest;
}

void FormantEngine::updateAllVoices() {
    for (auto& voice : voices_) {
        voice.setFormantParams(vowelFormants_);
        voice.setVocalTractParams(vocalTract_);
        voice.setVolume(volume_);
        voice.setEnvelopeParams(attack_, decay_, sustain_, release_);
    }
}

// Standard SynthEngine methods
size_t FormantEngine::getActiveVoiceCount() const {
    size_t count = 0;
    for (const auto& voice : voices_) {
        if (voice.isActive()) {
            count++;
        }
    }
    return count;
}

void FormantEngine::setVoiceCount(size_t maxVoices) {
    // Voice count is fixed for this implementation
}

void FormantEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
    struct PresetData {
        float harmonics, timbre, morph, volume;
        float attack, decay, sustain, release;
    };
    
    PresetData preset = {
        harmonics_, timbre_, morph_, volume_,
        attack_, decay_, sustain_, release_
    };
    
    actualSize = sizeof(PresetData);
    if (maxSize >= actualSize) {
        memcpy(data, &preset, actualSize);
    }
}

bool FormantEngine::loadPreset(const uint8_t* data, size_t size) {
    struct PresetData {
        float harmonics, timbre, morph, volume;
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
    attack_ = preset->attack;
    decay_ = preset->decay;
    sustain_ = preset->sustain;
    release_ = preset->release;
    
    calculateDerivedParams();
    updateAllVoices();
    return true;
}

void FormantEngine::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updateAllVoices();
}

void FormantEngine::setBufferSize(size_t bufferSize) {
    bufferSize_ = bufferSize;
}

bool FormantEngine::supportsModulation(ParameterID target) const {
    return hasParameter(target);
}

void FormantEngine::setModulation(ParameterID target, float amount) {
    size_t index = static_cast<size_t>(target);
    if (index < modulation_.size()) {
        modulation_[index] = amount;
    }
}

void FormantEngine::updateCPUUsage(float processingTime) {
    float maxTime = (BUFFER_SIZE / sampleRate_) * 1000.0f;
    cpuUsage_ = std::min(100.0f, (processingTime / maxTime) * 100.0f);
}

// FormantVoice implementation
FormantEngine::FormantVoice::FormantVoice() {
    envelope_.sampleRate = 48000.0f;
    
    // Initialize formant filters
    for (auto& filter : formantFilters_) {
        filter.sampleRate = 48000.0f;
    }
    
    noiseGen_.sampleRate = 48000.0f;
}

void FormantEngine::FormantVoice::noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate) {
    note_ = note;
    velocity_ = velocity;
    aftertouch_ = aftertouch;
    active_ = true;
    age_ = 0;
    
    // Calculate note frequency
    noteFrequency_ = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    
    // Set pulse generator frequency
    pulseGen_.setFrequency(noteFrequency_, sampleRate);
    
    // Update formant filter sample rates
    for (auto& filter : formantFilters_) {
        filter.sampleRate = sampleRate;
    }
    
    // Update envelope sample rate
    envelope_.sampleRate = sampleRate;
    
    // Set noise generator sample rate
    noiseGen_.sampleRate = sampleRate;
    
    // Trigger envelope
    envelope_.noteOn();
}

void FormantEngine::FormantVoice::noteOff() {
    envelope_.noteOff();
}

void FormantEngine::FormantVoice::setAftertouch(float aftertouch) {
    aftertouch_ = aftertouch;
}

AudioFrame FormantEngine::FormantVoice::processSample() {
    if (!active_) {
        return AudioFrame(0.0f, 0.0f);
    }
    
    age_++;
    
    // Generate glottal pulse
    float pulse = pulseGen_.processPulse();
    
    // Generate breath noise
    float noise = noiseGen_.processNoise();
    
    // Mix voiced and unvoiced components
    float excitation = pulse * vocalTract_.voicing + noise * vocalTract_.breathNoise;
    
    // Add consonant noise
    excitation += noise * vocalTract_.consonantMix;
    
    // Process through formant filters
    float f1Output = formantFilters_[0].process(excitation);
    float f2Output = formantFilters_[1].process(excitation);
    float f3Output = formantFilters_[2].process(excitation);
    
    // Mix formant outputs
    float mixed = f1Output + f2Output + f3Output;
    
    // Apply envelope
    float envLevel = envelope_.process();
    
    // Check if voice should be deactivated
    if (!envelope_.isActive()) {
        active_ = false;
    }
    
    // Apply velocity and volume
    float output = mixed * envLevel * velocity_ * volume_ * 0.3f; // Scale down for formants
    
    return AudioFrame(output, output);
}

void FormantEngine::FormantVoice::setFormantParams(const VowelFormants& vowelFormants) {
    vowelFormants_ = vowelFormants;
    
    // Update formant filters
    formantFilters_[0].setParams(vowelFormants.currentFormants.f1, 
                                vowelFormants.currentFormants.q1,
                                vowelFormants.currentFormants.a1);
    
    formantFilters_[1].setParams(vowelFormants.currentFormants.f2, 
                                vowelFormants.currentFormants.q2,
                                vowelFormants.currentFormants.a2);
    
    formantFilters_[2].setParams(vowelFormants.currentFormants.f3, 
                                vowelFormants.currentFormants.q3,
                                vowelFormants.currentFormants.a3);
}

void FormantEngine::FormantVoice::setVocalTractParams(const VocalTract& vocalTract) {
    vocalTract_ = vocalTract;
    
    // Update noise generator
    noiseGen_.setLevel(vocalTract.breathNoise + vocalTract.consonantMix);
    noiseGen_.setFricativeFreq(vocalTract.fricativeFreq);
}

void FormantEngine::FormantVoice::setVolume(float volume) {
    volume_ = volume;
}

void FormantEngine::FormantVoice::setEnvelopeParams(float attack, float decay, float sustain, float release) {
    envelope_.attack = attack;
    envelope_.decay = decay;
    envelope_.sustain = sustain;
    envelope_.release = release;
}

// Envelope implementation
float FormantEngine::FormantVoice::Envelope::process() {
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