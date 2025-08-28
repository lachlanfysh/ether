#include "RingsVoiceEngine.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <cmath>

RingsVoiceEngine::RingsVoiceEngine() {
    std::cout << "RingsVoice engine created" << std::endl;
    
    // Initialize modulation array
    modulation_.fill(0.0f);
    
    // Calculate initial derived parameters
    calculateDerivedParams();
    
    // Set up default parameters for all voices
    updateAllVoices();
}

RingsVoiceEngine::~RingsVoiceEngine() {
    allNotesOff();
    std::cout << "RingsVoice engine destroyed" << std::endl;
}

void RingsVoiceEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    RingsVoiceImpl* voice = findFreeVoice();
    if (!voice) {
        voice = stealVoice();
    }
    
    if (voice) {
        voice->noteOn(note, velocity, aftertouch, sampleRate_);
        voiceCounter_++;
        
        std::cout << "RingsVoice: Note ON " << static_cast<int>(note) 
                  << " vel=" << velocity << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void RingsVoiceEngine::noteOff(uint8_t note) {
    RingsVoiceImpl* voice = findVoice(note);
    if (voice) {
        voice->noteOff();
        
        std::cout << "RingsVoice: Note OFF " << static_cast<int>(note) 
                  << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void RingsVoiceEngine::setAftertouch(uint8_t note, float aftertouch) {
    RingsVoiceImpl* voice = findVoice(note);
    if (voice) {
        voice->setAftertouch(aftertouch);
    }
}

void RingsVoiceEngine::allNotesOff() {
    for (auto& voice : voices_) {
        voice.noteOff();
    }
}

void RingsVoiceEngine::setParameter(ParameterID param, float value) {
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
            release_ = std::clamp(value, 0.01f, 15.0f); // Longer release for physical modeling
            updateAllVoices();
            break;
            
        default:
            break;
    }
}

float RingsVoiceEngine::getParameter(ParameterID param) const {
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

bool RingsVoiceEngine::hasParameter(ParameterID param) const {
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

void RingsVoiceEngine::processAudio(EtherAudioBuffer& outputBuffer) {
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

void RingsVoiceEngine::setHarmonics(float harmonics) {
    harmonics_ = std::clamp(harmonics, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void RingsVoiceEngine::setTimbre(float timbre) {
    timbre_ = std::clamp(timbre, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void RingsVoiceEngine::setMorph(float morph) {
    morph_ = std::clamp(morph, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void RingsVoiceEngine::calculateDerivedParams() {
    // HARMONICS: resonator frequency + Q
    resonatorParams_.frequency = mapResonatorFreq(harmonics_);
    resonatorParams_.q = mapResonatorQ(harmonics_);
    resonatorParams_.harmonicSpread = mapHarmonicSpread(harmonics_);
    
    // TIMBRE: material properties
    materialProps_.calculateFromTimbre(timbre_);
    
    // MORPH: exciter balance
    exciterSystem_.calculateFromMorph(morph_);
}

float RingsVoiceEngine::mapResonatorFreq(float harmonics) const {
    // Resonator frequency multiplier: 0.5x to 4x
    return 0.5f * std::pow(8.0f, harmonics);
}

float RingsVoiceEngine::mapResonatorQ(float harmonics) const {
    // Q factor: 2 to 50 (exponential for musical response)
    return 2.0f * std::pow(25.0f, harmonics);
}

float RingsVoiceEngine::mapHarmonicSpread(float harmonics) const {
    // Harmonic spread: 0.8 to 1.5 (slightly compressed to stretched)
    return 0.8f + harmonics * 0.7f;
}

RingsVoiceEngine::MaterialProps::MaterialType RingsVoiceEngine::mapMaterial(float timbre, float& blend) const {
    float scaledTimbre = timbre * 5.0f; // 0-5 range
    int materialIndex = static_cast<int>(scaledTimbre);
    blend = scaledTimbre - materialIndex;
    
    materialIndex = std::clamp(materialIndex, 0, 5);
    return static_cast<MaterialProps::MaterialType>(materialIndex);
}

float RingsVoiceEngine::mapStiffness(float timbre) const {
    // Material stiffness varies with timbre
    return timbre;
}

float RingsVoiceEngine::mapDamping(float timbre) const {
    // Damping: 0.1 to 0.8
    return 0.1f + timbre * 0.7f;
}

void RingsVoiceEngine::mapExciterBalance(float morph, float& bow, float& blow, float& strike) const {
    if (morph < 0.33f) {
        // Bow dominant
        float localMorph = morph * 3.0f;
        bow = 1.0f - localMorph * 0.5f;
        blow = localMorph * 0.3f;
        strike = localMorph * 0.2f;
    } else if (morph < 0.66f) {
        // Blow dominant
        float localMorph = (morph - 0.33f) * 3.0f;
        bow = 0.5f - localMorph * 0.3f;
        blow = 0.3f + localMorph * 0.5f;
        strike = 0.2f + localMorph * 0.3f;
    } else {
        // Strike dominant
        float localMorph = (morph - 0.66f) * 3.0f;
        bow = 0.2f - localMorph * 0.1f;
        blow = 0.8f - localMorph * 0.3f;
        strike = 0.5f + localMorph * 0.4f;
    }
}

float RingsVoiceEngine::mapIntensity(float morph) const {
    // Excitation intensity
    return 0.3f + morph * 0.7f; // 0.3 to 1.0
}

// ResonatorParams implementation
void RingsVoiceEngine::ResonatorParams::calculateFromHarmonics(float harmonics, float noteFreq) {
    frequency = noteFreq * (0.5f * std::pow(8.0f, harmonics));
    q = 2.0f * std::pow(25.0f, harmonics);
    harmonicSpread = 0.8f + harmonics * 0.7f;
    coupling = 0.1f + harmonics * 0.4f; // More coupling at higher harmonics
}

// MaterialProps implementation
void RingsVoiceEngine::MaterialProps::calculateFromTimbre(float timbre) {
    float scaledTimbre = timbre * 5.0f;
    int materialIndex = static_cast<int>(scaledTimbre);
    float blend = scaledTimbre - materialIndex;
    
    type = static_cast<MaterialType>(std::clamp(materialIndex, 0, 5));
    
    // Material-specific properties
    switch (type) {
        case MaterialType::WOOD:
            stiffness = 0.3f + blend * 0.2f;
            density = 0.6f + blend * 0.2f;
            damping = 0.4f + blend * 0.2f;
            nonlinearity = 0.1f + blend * 0.1f;
            break;
            
        case MaterialType::METAL:
            stiffness = 0.7f + blend * 0.2f;
            density = 0.8f + blend * 0.1f;
            damping = 0.1f + blend * 0.1f;
            nonlinearity = 0.05f + blend * 0.05f;
            break;
            
        case MaterialType::GLASS:
            stiffness = 0.9f + blend * 0.05f;
            density = 0.5f + blend * 0.1f;
            damping = 0.05f + blend * 0.05f;
            nonlinearity = 0.02f + blend * 0.02f;
            break;
            
        case MaterialType::STRING:
            stiffness = 0.2f + blend * 0.3f;
            density = 0.4f + blend * 0.3f;
            damping = 0.3f + blend * 0.3f;
            nonlinearity = 0.2f + blend * 0.1f;
            break;
            
        case MaterialType::MEMBRANE:
            stiffness = 0.1f + blend * 0.2f;
            density = 0.3f + blend * 0.4f;
            damping = 0.6f + blend * 0.2f;
            nonlinearity = 0.3f + blend * 0.2f;
            break;
            
        case MaterialType::CRYSTAL:
            stiffness = 0.95f + blend * 0.03f;
            density = 0.7f + blend * 0.1f;
            damping = 0.02f + blend * 0.03f;
            nonlinearity = 0.01f + blend * 0.01f;
            break;
    }
}

float RingsVoiceEngine::MaterialProps::getDampingForFreq(float freq) const {
    // Frequency-dependent damping
    float normalizedFreq = freq / 1000.0f;
    float freqDamping = 1.0f + normalizedFreq * 0.2f; // Higher frequencies damp more
    return damping * freqDamping;
}

float RingsVoiceEngine::MaterialProps::getStiffnessModulation(float input) const {
    // Nonlinear stiffness effect
    if (nonlinearity < 0.01f) return 1.0f;
    
    float amplitude = std::abs(input);
    return 1.0f + amplitude * nonlinearity * stiffness * 0.1f;
}

std::array<float, 6> RingsVoiceEngine::MaterialProps::getMaterialDampingCurve(MaterialType mat) const {
    switch (mat) {
        case MaterialType::WOOD:
            return {1.0f, 0.8f, 0.6f, 0.4f, 0.3f, 0.2f}; // Warm rolloff
        case MaterialType::METAL:
            return {1.0f, 0.9f, 0.8f, 0.7f, 0.6f, 0.5f}; // Sustained
        case MaterialType::GLASS:
            return {1.0f, 0.95f, 0.9f, 0.85f, 0.8f, 0.75f}; // Pure
        case MaterialType::STRING:
            return {1.0f, 0.7f, 0.5f, 0.4f, 0.3f, 0.25f}; // Rich low end
        case MaterialType::MEMBRANE:
            return {1.0f, 0.5f, 0.2f, 0.1f, 0.05f, 0.02f}; // Percussive
        case MaterialType::CRYSTAL:
            return {1.0f, 0.98f, 0.95f, 0.92f, 0.9f, 0.88f}; // Ethereal
        default:
            return {1.0f, 0.8f, 0.6f, 0.4f, 0.3f, 0.2f};
    }
}

// ExciterSystem implementation
void RingsVoiceEngine::ExciterSystem::calculateFromMorph(float morph) {
    if (morph < 0.33f) {
        // Bow dominant
        float localMorph = morph * 3.0f;
        bowAmount = 1.0f - localMorph * 0.5f;
        blowAmount = localMorph * 0.3f;
        strikeAmount = localMorph * 0.2f;
    } else if (morph < 0.66f) {
        // Blow dominant
        float localMorph = (morph - 0.33f) * 3.0f;
        bowAmount = 0.5f - localMorph * 0.3f;
        blowAmount = 0.3f + localMorph * 0.5f;
        strikeAmount = 0.2f + localMorph * 0.3f;
    } else {
        // Strike dominant
        float localMorph = (morph - 0.66f) * 3.0f;
        bowAmount = 0.2f - localMorph * 0.1f;
        blowAmount = 0.8f - localMorph * 0.3f;
        strikeAmount = 0.5f + localMorph * 0.4f;
    }
    
    intensity = 0.3f + morph * 0.7f;
    
    // Normalize
    float total = bowAmount + blowAmount + strikeAmount;
    if (total > 0.0f) {
        bowAmount /= total;
        blowAmount /= total;
        strikeAmount /= total;
    }
}

float RingsVoiceEngine::ExciterSystem::generateExcitation(float velocity, float sampleRate) const {
    static float phase = 0.0f;
    static float time = 0.0f;
    static uint32_t noiseState = 54321;
    
    phase += 1.0f / sampleRate;
    time += 1.0f / sampleRate;
    
    float bowEx = generateBowExcitation(velocity, phase);
    float blowEx = generateBlowExcitation(velocity, noiseState / 4294967296.0f);
    float strikeEx = generateStrikeExcitation(velocity, time);
    
    noiseState = noiseState * 1664525 + 1013904223;
    
    return (bowEx * bowAmount + blowEx * blowAmount + strikeEx * strikeAmount) * intensity;
}

float RingsVoiceEngine::ExciterSystem::generateBowExcitation(float velocity, float phase) const {
    // Sawtooth-like bow excitation
    float sawPhase = std::fmod(phase * 100.0f, 1.0f); // ~100 Hz bow rate
    return (2.0f * sawPhase - 1.0f) * velocity;
}

float RingsVoiceEngine::ExciterSystem::generateBlowExcitation(float velocity, float noise) const {
    // Turbulent noise for wind instruments
    return (noise - 0.5f) * 2.0f * velocity * 0.5f;
}

float RingsVoiceEngine::ExciterSystem::generateStrikeExcitation(float velocity, float time) const {
    // Exponential decay for struck instruments
    float decay = std::exp(-time * 20.0f); // Fast decay
    return decay * velocity;
}

// Voice management methods
RingsVoiceEngine::RingsVoiceImpl* RingsVoiceEngine::findFreeVoice() {
    for (auto& voice : voices_) {
        if (!voice.isActive()) {
            return &voice;
        }
    }
    return nullptr;
}

RingsVoiceEngine::RingsVoiceImpl* RingsVoiceEngine::findVoice(uint8_t note) {
    for (auto& voice : voices_) {
        if (voice.isActive() && voice.getNote() == note) {
            return &voice;
        }
    }
    return nullptr;
}

RingsVoiceEngine::RingsVoiceImpl* RingsVoiceEngine::stealVoice() {
    RingsVoiceImpl* oldest = nullptr;
    uint32_t oldestAge = 0;
    
    for (auto& voice : voices_) {
        if (voice.getAge() > oldestAge) {
            oldestAge = voice.getAge();
            oldest = &voice;
        }
    }
    
    return oldest;
}

void RingsVoiceEngine::updateAllVoices() {
    for (auto& voice : voices_) {
        voice.setResonatorParams(resonatorParams_);
        voice.setMaterialProps(materialProps_);
        voice.setExciterSystem(exciterSystem_);
        voice.setVolume(volume_);
        voice.setEnvelopeParams(attack_, decay_, sustain_, release_);
    }
}

// Standard SynthEngine methods (same pattern as other engines)
size_t RingsVoiceEngine::getActiveVoiceCount() const {
    size_t count = 0;
    for (const auto& voice : voices_) {
        if (voice.isActive()) {
            count++;
        }
    }
    return count;
}

void RingsVoiceEngine::setVoiceCount(size_t maxVoices) {
    // Voice count is fixed for this implementation
}

void RingsVoiceEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
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

bool RingsVoiceEngine::loadPreset(const uint8_t* data, size_t size) {
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

void RingsVoiceEngine::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updateAllVoices();
}

void RingsVoiceEngine::setBufferSize(size_t bufferSize) {
    bufferSize_ = bufferSize;
}

bool RingsVoiceEngine::supportsModulation(ParameterID target) const {
    return hasParameter(target);
}

void RingsVoiceEngine::setModulation(ParameterID target, float amount) {
    size_t index = static_cast<size_t>(target);
    if (index < modulation_.size()) {
        modulation_[index] = amount;
    }
}

void RingsVoiceEngine::updateCPUUsage(float processingTime) {
    float maxTime = (BUFFER_SIZE / sampleRate_) * 1000.0f;
    cpuUsage_ = std::min(100.0f, (processingTime / maxTime) * 100.0f);
}

// RingsVoiceImpl implementation
RingsVoiceEngine::RingsVoiceImpl::RingsVoiceImpl() {
    envelope_.sampleRate = 48000.0f;
    noiseState_ = 12345 + reinterpret_cast<uintptr_t>(this);
    
    // Initialize resonators with harmonic frequencies
    for (int i = 0; i < 4; i++) {
        resonators_[i].sampleRate = 48000.0f;
    }
}

void RingsVoiceEngine::RingsVoiceImpl::noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate) {
    note_ = note;
    velocity_ = velocity;
    aftertouch_ = aftertouch;
    active_ = true;
    age_ = 0;
    sampleRate_ = sampleRate;
    excitationTime_ = 0.0f;
    
    // Calculate note frequency
    noteFrequency_ = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    
    // Set up resonators with harmonic series
    for (int i = 0; i < 4; i++) {
        float resonatorFreq = calculateResonatorFreq(i, noteFrequency_, resonatorParams_.harmonicSpread);
        resonators_[i].setParams(resonatorFreq, resonatorParams_.q, 1.0f / (i + 1)); // Decreasing amplitude
        resonators_[i].sampleRate = sampleRate;
        resonators_[i].updateCoefficients();
    }
    
    // Set up damping model
    dampingModel_.setMaterial(materialProps_.type, materialProps_.damping);
    
    // Set up coupling
    coupling_.setCoupling(resonatorParams_.coupling);
    
    // Update envelope sample rate
    envelope_.sampleRate = sampleRate;
    
    // Trigger envelope
    envelope_.noteOn();
}

void RingsVoiceEngine::RingsVoiceImpl::noteOff() {
    envelope_.noteOff();
}

void RingsVoiceEngine::RingsVoiceImpl::setAftertouch(float aftertouch) {
    aftertouch_ = aftertouch;
}

AudioFrame RingsVoiceEngine::RingsVoiceImpl::processSample() {
    if (!active_) {
        return AudioFrame(0.0f, 0.0f);
    }
    
    age_++;
    excitationTime_ += 1.0f / sampleRate_;
    
    // Generate excitation
    float excitation = exciterSystem_.generateExcitation(velocity_, sampleRate_);
    
    // Process through resonators
    std::array<float, 4> resonatorOutputs;
    for (int i = 0; i < 4; i++) {
        float dampingFactor = materialProps_.getDampingForFreq(resonators_[i].frequency);
        resonatorOutputs[i] = resonators_[i].process(excitation, dampingFactor);
        
        // Apply material nonlinearity
        float stiffnessMod = materialProps_.getStiffnessModulation(resonatorOutputs[i]);
        resonatorOutputs[i] *= stiffnessMod;
        
        // Apply damping model
        resonatorOutputs[i] = dampingModel_.process(resonatorOutputs[i], resonators_[i].frequency);
    }
    
    // Apply coupling between resonators
    coupling_.process(resonatorOutputs);
    
    // Mix resonator outputs
    float mixed = 0.0f;
    for (int i = 0; i < 4; i++) {
        mixed += resonatorOutputs[i];
    }
    mixed *= 0.25f; // Normalize
    
    // Apply envelope
    float envLevel = envelope_.process();
    
    // Check if voice should be deactivated
    if (!envelope_.isActive()) {
        active_ = false;
    }
    
    // Apply velocity and volume
    float output = mixed * envLevel * velocity_ * volume_ * 0.5f; // Scale for physical modeling
    
    return AudioFrame(output, output);
}

void RingsVoiceEngine::RingsVoiceImpl::setResonatorParams(const ResonatorParams& params) {
    resonatorParams_ = params;
    
    // Update resonator frequencies if voice is active
    if (active_) {
        for (int i = 0; i < 4; i++) {
            float resonatorFreq = calculateResonatorFreq(i, noteFrequency_, params.harmonicSpread);
            resonators_[i].setParams(resonatorFreq, params.q, 1.0f / (i + 1));
        }
        coupling_.setCoupling(params.coupling);
    }
}

void RingsVoiceEngine::RingsVoiceImpl::setMaterialProps(const MaterialProps& props) {
    materialProps_ = props;
    dampingModel_.setMaterial(props.type, props.damping);
}

void RingsVoiceEngine::RingsVoiceImpl::setExciterSystem(const ExciterSystem& system) {
    exciterSystem_ = system;
}

void RingsVoiceEngine::RingsVoiceImpl::setVolume(float volume) {
    volume_ = volume;
}

void RingsVoiceEngine::RingsVoiceImpl::setEnvelopeParams(float attack, float decay, float sustain, float release) {
    envelope_.attack = attack;
    envelope_.decay = decay;
    envelope_.sustain = sustain;
    envelope_.release = release;
}

float RingsVoiceEngine::RingsVoiceImpl::generateNoise() {
    noiseState_ = noiseState_ * 1664525 + 1013904223;
    return (static_cast<float>(noiseState_) / 4294967296.0f) - 0.5f;
}

float RingsVoiceEngine::RingsVoiceImpl::calculateResonatorFreq(int index, float baseFreq, float spread) const {
    // Generate harmonic series with spread variation
    float harmonic = static_cast<float>(index + 1) * spread;
    return baseFreq * harmonic * resonatorParams_.frequency;
}

// Envelope implementation
float RingsVoiceEngine::RingsVoiceImpl::Envelope::process() {
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