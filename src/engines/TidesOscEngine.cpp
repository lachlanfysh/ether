#include "TidesOscEngine.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <cmath>

TidesOscEngine::TidesOscEngine() {
    std::cout << "TidesOsc engine created" << std::endl;
    
    // Initialize modulation array
    modulation_.fill(0.0f);
    
    // Calculate initial derived parameters
    calculateDerivedParams();
    
    // Set up default parameters for all voices
    updateAllVoices();
}

TidesOscEngine::~TidesOscEngine() {
    allNotesOff();
    std::cout << "TidesOsc engine destroyed" << std::endl;
}

void TidesOscEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    TidesOscVoice* voice = findFreeVoice();
    if (!voice) {
        voice = stealVoice();
    }
    
    if (voice) {
        voice->noteOn(note, velocity, aftertouch, sampleRate_);
        voiceCounter_++;
        
        std::cout << "TidesOsc: Note ON " << static_cast<int>(note) 
                  << " vel=" << velocity << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void TidesOscEngine::noteOff(uint8_t note) {
    TidesOscVoice* voice = findVoice(note);
    if (voice) {
        voice->noteOff();
        
        std::cout << "TidesOsc: Note OFF " << static_cast<int>(note) 
                  << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void TidesOscEngine::setAftertouch(uint8_t note, float aftertouch) {
    TidesOscVoice* voice = findVoice(note);
    if (voice) {
        voice->setAftertouch(aftertouch);
    }
}

void TidesOscEngine::allNotesOff() {
    for (auto& voice : voices_) {
        voice.noteOff();
    }
}

void TidesOscEngine::setParameter(ParameterID param, float value) {
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
            attack_ = std::clamp(value, 0.001f, 5.0f);
            updateAllVoices();
            break;
            
        case ParameterID::DECAY:
            decay_ = std::clamp(value, 0.01f, 10.0f);
            updateAllVoices();
            break;
            
        case ParameterID::SUSTAIN:
            sustain_ = std::clamp(value, 0.0f, 1.0f);
            updateAllVoices();
            break;
            
        case ParameterID::RELEASE:
            release_ = std::clamp(value, 0.01f, 10.0f);
            updateAllVoices();
            break;
            
        default:
            break;
    }
}

float TidesOscEngine::getParameter(ParameterID param) const {
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

bool TidesOscEngine::hasParameter(ParameterID param) const {
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

void TidesOscEngine::processAudio(EtherAudioBuffer& outputBuffer) {
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

void TidesOscEngine::setHarmonics(float harmonics) {
    harmonics_ = std::clamp(harmonics, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void TidesOscEngine::setTimbre(float timbre) {
    timbre_ = std::clamp(timbre, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void TidesOscEngine::setMorph(float morph) {
    morph_ = std::clamp(morph, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void TidesOscEngine::calculateDerivedParams() {
    // HARMONICS: slope steepness
    complexParams_.calculateFromHarmonics(harmonics_);
    
    // TIMBRE: frequency ratio + material
    freqMaterial_.calculateFromTimbre(timbre_);
    
    // MORPH: amplitude balance + damping
    ampDamping_.calculateFromMorph(morph_);
}

float TidesOscEngine::mapSlopeRise(float harmonics) const {
    // Rise slope: smooth to sharp
    return harmonics;
}

float TidesOscEngine::mapSlopeFall(float harmonics) const {
    // Fall slope: smooth to sharp
    return harmonics;
}

float TidesOscEngine::mapSymmetry(float harmonics) const {
    // Symmetry variation based on harmonics
    return 0.3f + harmonics * 0.4f; // 0.3 to 0.7
}

float TidesOscEngine::mapFold(float harmonics) const {
    // Folding amount increases with harmonics
    return harmonics * 0.5f; // 0 to 0.5
}

TidesOscEngine::FrequencyMaterial::MaterialType TidesOscEngine::mapMaterial(float timbre, float& blend) const {
    // Map timbre to material progression
    float scaledTimbre = timbre * 5.0f; // 0-5 range
    int materialIndex = static_cast<int>(scaledTimbre);
    blend = scaledTimbre - materialIndex;
    
    materialIndex = std::clamp(materialIndex, 0, 5);
    return static_cast<FrequencyMaterial::MaterialType>(materialIndex);
}

float TidesOscEngine::mapFrequencyRatio(float timbre) const {
    // Frequency ratio: 0.25 to 4.0 (2 octaves down to 2 octaves up)
    // Exponential mapping for musical intervals
    return 0.25f * std::pow(16.0f, timbre); // 0.25x to 4x
}

float TidesOscEngine::mapOscillatorBalance(float morph) const {
    // Balance between dual oscillators
    return morph;
}

float TidesOscEngine::mapDamping(float morph) const {
    // Damping amount
    return morph * 0.8f; // 0 to 0.8
}

// ComplexOscParams implementation
void TidesOscEngine::ComplexOscParams::calculateFromHarmonics(float harmonics) {
    slopeRise = harmonics;
    slopeFall = harmonics;
    symmetry = 0.3f + harmonics * 0.4f; // 0.3 to 0.7
    fold = harmonics * 0.5f; // 0 to 0.5
}

// FrequencyMaterial implementation
void TidesOscEngine::FrequencyMaterial::calculateFromTimbre(float timbre) {
    // Map first half of timbre to frequency ratio
    if (timbre < 0.5f) {
        ratio = 0.25f * std::pow(16.0f, timbre * 2.0f); // 0.25x to 4x over first half
        materialAmount = 0.3f;
        isHarmonic = true;
    } else {
        // Second half controls material type and amount
        float materialT = (timbre - 0.5f) * 2.0f;
        float scaledMat = materialT * 5.0f;
        int matIndex = static_cast<int>(scaledMat);
        material = static_cast<MaterialType>(std::clamp(matIndex, 0, 5));
        materialAmount = 0.3f + materialT * 0.7f; // 0.3 to 1.0
        isHarmonic = (materialT < 0.7f); // Become inharmonic at higher settings
    }
}

float TidesOscEngine::FrequencyMaterial::getMaterialModulation(float phase, MaterialType mat) const {
    // Material-specific phase modulation
    switch (mat) {
        case MaterialType::WOOD:
            return 1.0f + 0.02f * std::sin(phase * 2.0f * M_PI * 3.0f); // Warm modulation
            
        case MaterialType::METAL:
            return 1.0f + 0.05f * std::sin(phase * 2.0f * M_PI * 7.0f); // Metallic shimmer
            
        case MaterialType::GLASS:
            return 1.0f + 0.01f * std::sin(phase * 2.0f * M_PI * 11.0f); // Crystalline purity
            
        case MaterialType::STRING:
            return 1.0f + 0.03f * (std::sin(phase * 2.0f * M_PI * 2.0f) + 
                                 0.5f * std::sin(phase * 2.0f * M_PI * 5.0f)); // Complex harmonics
            
        case MaterialType::MEMBRANE:
            return 1.0f + 0.1f * std::exp(-phase * 5.0f) * std::sin(phase * 2.0f * M_PI * 2.0f); // Percussive decay
            
        case MaterialType::AIR:
            return 1.0f + 0.08f * (phase - 0.5f) * 2.0f; // Breathy instability
            
        default:
            return 1.0f;
    }
}

std::array<float, 8> TidesOscEngine::FrequencyMaterial::getHarmonicContent(MaterialType mat) const {
    switch (mat) {
        case MaterialType::WOOD:
            return {1.0f, 0.6f, 0.4f, 0.3f, 0.2f, 0.15f, 0.1f, 0.05f}; // Warm rolloff
            
        case MaterialType::METAL:
            return {1.0f, 0.3f, 0.7f, 0.2f, 0.5f, 0.15f, 0.4f, 0.1f}; // Inharmonic content
            
        case MaterialType::GLASS:
            return {1.0f, 0.1f, 0.05f, 0.02f, 0.01f, 0.005f, 0.002f, 0.001f}; // Very pure
            
        case MaterialType::STRING:
            return {1.0f, 0.8f, 0.6f, 0.5f, 0.4f, 0.3f, 0.25f, 0.2f}; // Rich harmonics
            
        case MaterialType::MEMBRANE:
            return {1.0f, 0.2f, 0.4f, 0.1f, 0.2f, 0.05f, 0.1f, 0.02f}; // Drum-like
            
        case MaterialType::AIR:
            return {1.0f, 0.4f, 0.3f, 0.3f, 0.25f, 0.2f, 0.2f, 0.15f}; // Broad spectrum
            
        default:
            return {1.0f, 0.5f, 0.25f, 0.125f, 0.0625f, 0.03f, 0.015f, 0.007f};
    }
}

// AmplitudeDamping implementation
void TidesOscEngine::AmplitudeDamping::calculateFromMorph(float morph) {
    oscillatorBalance = morph;
    damping = morph * 0.8f;
    dampingRate = 1.0f + morph * 4.0f; // 1 to 5
    sustainLevel = 1.0f - morph * 0.6f; // 1.0 to 0.4
}

// Voice management methods
TidesOscEngine::TidesOscVoice* TidesOscEngine::findFreeVoice() {
    for (auto& voice : voices_) {
        if (!voice.isActive()) {
            return &voice;
        }
    }
    return nullptr;
}

TidesOscEngine::TidesOscVoice* TidesOscEngine::findVoice(uint8_t note) {
    for (auto& voice : voices_) {
        if (voice.isActive() && voice.getNote() == note) {
            return &voice;
        }
    }
    return nullptr;
}

TidesOscEngine::TidesOscVoice* TidesOscEngine::stealVoice() {
    TidesOscVoice* oldest = nullptr;
    uint32_t oldestAge = 0;
    
    for (auto& voice : voices_) {
        if (voice.getAge() > oldestAge) {
            oldestAge = voice.getAge();
            oldest = &voice;
        }
    }
    
    return oldest;
}

void TidesOscEngine::updateAllVoices() {
    for (auto& voice : voices_) {
        voice.setComplexOscParams(complexParams_);
        voice.setFrequencyMaterial(freqMaterial_);
        voice.setAmplitudeDamping(ampDamping_);
        voice.setVolume(volume_);
        voice.setEnvelopeParams(attack_, decay_, sustain_, release_);
    }
}

// Standard SynthEngine methods
size_t TidesOscEngine::getActiveVoiceCount() const {
    size_t count = 0;
    for (const auto& voice : voices_) {
        if (voice.isActive()) {
            count++;
        }
    }
    return count;
}

void TidesOscEngine::setVoiceCount(size_t maxVoices) {
    // Voice count is fixed for this implementation
}

void TidesOscEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
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

bool TidesOscEngine::loadPreset(const uint8_t* data, size_t size) {
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

void TidesOscEngine::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updateAllVoices();
}

void TidesOscEngine::setBufferSize(size_t bufferSize) {
    bufferSize_ = bufferSize;
}

bool TidesOscEngine::supportsModulation(ParameterID target) const {
    return hasParameter(target);
}

void TidesOscEngine::setModulation(ParameterID target, float amount) {
    size_t index = static_cast<size_t>(target);
    if (index < modulation_.size()) {
        modulation_[index] = amount;
    }
}

void TidesOscEngine::updateCPUUsage(float processingTime) {
    float maxTime = (BUFFER_SIZE / sampleRate_) * 1000.0f;
    cpuUsage_ = std::min(100.0f, (processingTime / maxTime) * 100.0f);
}

// TidesOscVoice implementation
TidesOscEngine::TidesOscVoice::TidesOscVoice() {
    envelope_.sampleRate = 48000.0f;
    materialFilter_.sampleRate = 48000.0f;
}

void TidesOscEngine::TidesOscVoice::noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate) {
    note_ = note;
    velocity_ = velocity;
    aftertouch_ = aftertouch;
    active_ = true;
    age_ = 0;
    
    // Calculate note frequency
    noteFrequency_ = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    
    // Set oscillator frequencies
    oscA_.setFrequency(noteFrequency_, sampleRate);
    oscB_.setFrequency(noteFrequency_ * freqMaterial_.ratio, sampleRate);
    
    // Update material filter sample rate
    materialFilter_.sampleRate = sampleRate;
    
    // Update envelope sample rate
    envelope_.sampleRate = sampleRate;
    
    // Trigger damping envelope
    dampingEnv_.trigger();
    
    // Trigger main envelope
    envelope_.noteOn();
}

void TidesOscEngine::TidesOscVoice::noteOff() {
    envelope_.noteOff();
}

void TidesOscEngine::TidesOscVoice::setAftertouch(float aftertouch) {
    aftertouch_ = aftertouch;
}

AudioFrame TidesOscEngine::TidesOscVoice::processSample() {
    if (!active_) {
        return AudioFrame(0.0f, 0.0f);
    }
    
    age_++;
    
    // Process dual oscillators
    float oscAOut = oscA_.process();
    float oscBOut = oscB_.process();
    
    // Apply oscillator balance
    float mixed = oscAOut * (1.0f - ampDamping_.oscillatorBalance) + 
                  oscBOut * ampDamping_.oscillatorBalance;
    
    // Apply material filtering
    float filtered = materialFilter_.process(mixed, noteFrequency_);
    
    // Apply damping envelope
    float dampingLevel = dampingEnv_.process(envelope_.sampleRate);
    filtered *= dampingLevel;
    
    // Apply main envelope
    float envLevel = envelope_.process();
    
    // Check if voice should be deactivated
    if (!envelope_.isActive()) {
        active_ = false;
    }
    
    // Apply velocity and volume
    float output = filtered * envLevel * velocity_ * volume_;
    
    return AudioFrame(output, output);
}

void TidesOscEngine::TidesOscVoice::setComplexOscParams(const ComplexOscParams& params) {
    complexParams_ = params;
    
    // Update oscillator slope parameters
    oscA_.setSlopes(params.slopeRise, params.slopeFall, params.symmetry, params.fold);
    oscB_.setSlopes(params.slopeRise, params.slopeFall, params.symmetry, params.fold);
}

void TidesOscEngine::TidesOscVoice::setFrequencyMaterial(const FrequencyMaterial& material) {
    freqMaterial_ = material;
    
    // Update oscillator B frequency with new ratio
    if (active_) {
        oscB_.setFrequency(noteFrequency_ * material.ratio, envelope_.sampleRate);
    }
    
    // Update material filter
    materialFilter_.setMaterial(material.material, material.materialAmount);
}

void TidesOscEngine::TidesOscVoice::setAmplitudeDamping(const AmplitudeDamping& damping) {
    ampDamping_ = damping;
    
    // Update damping envelope
    dampingEnv_.setParams(damping.damping, damping.dampingRate, damping.sustainLevel);
}

void TidesOscEngine::TidesOscVoice::setVolume(float volume) {
    volume_ = volume;
}

void TidesOscEngine::TidesOscVoice::setEnvelopeParams(float attack, float decay, float sustain, float release) {
    envelope_.attack = attack;
    envelope_.decay = decay;
    envelope_.sustain = sustain;
    envelope_.release = release;
}

// Envelope implementation
float TidesOscEngine::TidesOscVoice::Envelope::process() {
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