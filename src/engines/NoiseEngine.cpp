#include "NoiseEngine.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <cmath>

NoiseEngine::NoiseEngine() {
    std::cout << "Noise engine created" << std::endl;
    
    // Initialize modulation array
    modulation_.fill(0.0f);
    
    // Calculate initial derived parameters
    calculateDerivedParams();
    
    // Set up default parameters for all voices
    updateAllVoices();
}

NoiseEngine::~NoiseEngine() {
    allNotesOff();
    std::cout << "Noise engine destroyed" << std::endl;
}

void NoiseEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    NoiseVoice* voice = findFreeVoice();
    if (!voice) {
        voice = stealVoice();
    }
    
    if (voice) {
        voice->noteOn(note, velocity, aftertouch, sampleRate_);
        voiceCounter_++;
        
        std::cout << "Noise: Note ON " << static_cast<int>(note) 
                  << " vel=" << velocity << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void NoiseEngine::noteOff(uint8_t note) {
    NoiseVoice* voice = findVoice(note);
    if (voice) {
        voice->noteOff();
        
        std::cout << "Noise: Note OFF " << static_cast<int>(note) 
                  << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void NoiseEngine::setAftertouch(uint8_t note, float aftertouch) {
    NoiseVoice* voice = findVoice(note);
    if (voice) {
        voice->setAftertouch(aftertouch);
    }
}

void NoiseEngine::allNotesOff() {
    for (auto& voice : voices_) {
        voice.noteOff();
    }
}

void NoiseEngine::setParameter(ParameterID param, float value) {
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

float NoiseEngine::getParameter(ParameterID param) const {
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

bool NoiseEngine::hasParameter(ParameterID param) const {
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

void NoiseEngine::processAudio(EtherAudioBuffer& outputBuffer) {
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

void NoiseEngine::setHarmonics(float harmonics) {
    harmonics_ = std::clamp(harmonics, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void NoiseEngine::setTimbre(float timbre) {
    timbre_ = std::clamp(timbre, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void NoiseEngine::setMorph(float morph) {
    morph_ = std::clamp(morph, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void NoiseEngine::calculateDerivedParams() {
    // HARMONICS: grain density + size
    granularParams_.calculateFromHarmonics(harmonics_);
    
    // TIMBRE: scatter + jitter
    granularParams_.calculateFromTimbre(timbre_);
    
    // MORPH: source + randomness
    granularParams_.calculateFromMorph(morph_);
    noiseSource_.calculateFromMorph(morph_);
}

float NoiseEngine::mapGrainDensity(float harmonics) const {
    // Grain density: 5 to 100 grains per second (exponential)
    return 5.0f * std::pow(20.0f, harmonics);
}

float NoiseEngine::mapGrainSize(float harmonics) const {
    // Grain size: 0.2s to 0.01s (inverse relationship with density)
    return 0.2f * std::pow(0.05f, harmonics); // 200ms to 10ms
}

float NoiseEngine::mapScatter(float timbre) const {
    // Position scatter: 0 to 1
    return timbre;
}

float NoiseEngine::mapJitter(float timbre) const {
    // Temporal jitter: 0 to 0.8
    return timbre * 0.8f;
}

NoiseEngine::NoiseSource::Type NoiseEngine::mapNoiseType(float morph, float& blend) const {
    // Map morph to noise type progression
    float scaledMorph = morph * 5.0f; // 0-5 range
    int typeIndex = static_cast<int>(scaledMorph);
    blend = scaledMorph - typeIndex;
    
    typeIndex = std::clamp(typeIndex, 0, 5);
    return static_cast<NoiseSource::Type>(typeIndex);
}

float NoiseEngine::mapRandomness(float morph) const {
    // Randomness: 0 to 1 (chaos level)
    return morph;
}

// GranularParams implementation
void NoiseEngine::GranularParams::calculateFromHarmonics(float harmonics) {
    // Inverse relationship: higher harmonics = more density, smaller grains
    density = 5.0f * std::pow(20.0f, harmonics); // 5 to 100 grains/sec
    grainSize = 0.2f * std::pow(0.05f, harmonics); // 200ms to 10ms
}

void NoiseEngine::GranularParams::calculateFromTimbre(float timbre) {
    scatter = timbre;
    jitter = timbre * 0.8f;
}

void NoiseEngine::GranularParams::calculateFromMorph(float morph) {
    randomness = morph;
}

// NoiseSource implementation
void NoiseEngine::NoiseSource::calculateFromMorph(float morph) {
    float scaledMorph = morph * 5.0f;
    int typeIndex = static_cast<int>(scaledMorph);
    blend = scaledMorph - typeIndex;
    
    typeIndex = std::clamp(typeIndex, 0, 5);
    currentType = static_cast<Type>(typeIndex);
}

float NoiseEngine::NoiseSource::generateSample(uint32_t& seed) const {
    float sample1 = 0.0f;
    float sample2 = 0.0f;
    
    // Generate primary sample
    switch (currentType) {
        case Type::WHITE:
            sample1 = generateWhite(seed);
            sample2 = generatePink(seed);
            break;
        case Type::PINK:
            sample1 = generatePink(seed);
            sample2 = generateBrown(seed);
            break;
        case Type::BROWN:
            sample1 = generateBrown(seed);
            sample2 = generateBlue(seed);
            break;
        case Type::BLUE:
            sample1 = generateBlue(seed);
            sample2 = generateVelvet(seed);
            break;
        case Type::VELVET:
            sample1 = generateVelvet(seed);
            sample2 = generateCrackle(seed);
            break;
        case Type::CRACKLE:
            sample1 = generateCrackle(seed);
            sample2 = generateWhite(seed);
            break;
    }
    
    // Blend between types
    return sample1 * (1.0f - blend) + sample2 * blend;
}

float NoiseEngine::NoiseSource::generateWhite(uint32_t& seed) const {
    seed = seed * 1664525 + 1013904223;
    return (static_cast<float>(seed) / 4294967296.0f) - 0.5f;
}

float NoiseEngine::NoiseSource::generatePink(uint32_t& seed) const {
    // Simplified pink noise using white noise and filtering
    static float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f, b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;
    
    float white = generateWhite(seed);
    b0 = 0.99886f * b0 + white * 0.0555179f;
    b1 = 0.99332f * b1 + white * 0.0750759f;
    b2 = 0.96900f * b2 + white * 0.1538520f;
    b3 = 0.86650f * b3 + white * 0.3104856f;
    b4 = 0.55000f * b4 + white * 0.5329522f;
    b5 = -0.7616f * b5 - white * 0.0168980f;
    
    float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
    b6 = white * 0.115926f;
    
    return pink * 0.11f; // Scale down
}

float NoiseEngine::NoiseSource::generateBrown(uint32_t& seed) const {
    // Brown noise (integrated white noise)
    static float lastBrown = 0.0f;
    
    float white = generateWhite(seed);
    lastBrown = (lastBrown + white * 0.02f);
    lastBrown = std::clamp(lastBrown, -1.0f, 1.0f);
    
    return lastBrown;
}

float NoiseEngine::NoiseSource::generateBlue(uint32_t& seed) const {
    // Blue noise (differentiated white noise)
    static float lastWhite = 0.0f;
    
    float white = generateWhite(seed);
    float blue = white - lastWhite;
    lastWhite = white;
    
    return blue * 0.5f;
}

float NoiseEngine::NoiseSource::generateVelvet(uint32_t& seed) const {
    // Velvet noise (sparse impulses)
    float sample = 0.0f;
    
    if ((seed % 1000) < 10) { // ~1% chance of impulse
        sample = (generateWhite(seed) > 0.0f) ? 1.0f : -1.0f;
    }
    
    return sample;
}

float NoiseEngine::NoiseSource::generateCrackle(uint32_t& seed) const {
    // Crackle noise (chaotic bursts)
    static float energy = 0.0f;
    static int burstLength = 0;
    
    if (burstLength > 0) {
        burstLength--;
        return generateWhite(seed) * energy;
    } else if ((seed % 10000) < 5) { // Rare bursts
        burstLength = 10 + (seed % 50);
        energy = 0.5f + (generateWhite(seed) + 0.5f) * 0.5f;
        return generateWhite(seed) * energy;
    }
    
    return 0.0f;
}

// Voice management methods
NoiseEngine::NoiseVoice* NoiseEngine::findFreeVoice() {
    for (auto& voice : voices_) {
        if (!voice.isActive()) {
            return &voice;
        }
    }
    return nullptr;
}

NoiseEngine::NoiseVoice* NoiseEngine::findVoice(uint8_t note) {
    for (auto& voice : voices_) {
        if (voice.isActive() && voice.getNote() == note) {
            return &voice;
        }
    }
    return nullptr;
}

NoiseEngine::NoiseVoice* NoiseEngine::stealVoice() {
    NoiseVoice* oldest = nullptr;
    uint32_t oldestAge = 0;
    
    for (auto& voice : voices_) {
        if (voice.getAge() > oldestAge) {
            oldestAge = voice.getAge();
            oldest = &voice;
        }
    }
    
    return oldest;
}

void NoiseEngine::updateAllVoices() {
    for (auto& voice : voices_) {
        voice.setGranularParams(granularParams_);
        voice.setNoiseSource(noiseSource_);
        voice.setVolume(volume_);
        voice.setEnvelopeParams(attack_, decay_, sustain_, release_);
    }
}

// Standard SynthEngine methods
size_t NoiseEngine::getActiveVoiceCount() const {
    size_t count = 0;
    for (const auto& voice : voices_) {
        if (voice.isActive()) {
            count++;
        }
    }
    return count;
}

void NoiseEngine::setVoiceCount(size_t maxVoices) {
    // Voice count is fixed for this implementation
}

void NoiseEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
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

bool NoiseEngine::loadPreset(const uint8_t* data, size_t size) {
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

void NoiseEngine::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updateAllVoices();
}

void NoiseEngine::setBufferSize(size_t bufferSize) {
    bufferSize_ = bufferSize;
}

bool NoiseEngine::supportsModulation(ParameterID target) const {
    return hasParameter(target);
}

void NoiseEngine::setModulation(ParameterID target, float amount) {
    size_t index = static_cast<size_t>(target);
    if (index < modulation_.size()) {
        modulation_[index] = amount;
    }
}

void NoiseEngine::updateCPUUsage(float processingTime) {
    float maxTime = (BUFFER_SIZE / sampleRate_) * 1000.0f;
    cpuUsage_ = std::min(100.0f, (processingTime / maxTime) * 100.0f);
}

// NoiseVoice implementation
NoiseEngine::NoiseVoice::NoiseVoice() {
    envelope_.sampleRate = 48000.0f;
    randomSeed_ = 12345 + reinterpret_cast<uintptr_t>(this); // Unique seed per voice
}

void NoiseEngine::NoiseVoice::noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate) {
    note_ = note;
    velocity_ = velocity;
    aftertouch_ = aftertouch;
    active_ = true;
    age_ = 0;
    sampleRate_ = sampleRate;
    
    // Calculate note frequency (affects grain pitch)
    noteFrequency_ = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    
    // Reset grain scheduler
    scheduler_.nextGrainTime = 0.0f;
    scheduler_.grainTimer = 0.0f;
    
    // Deactivate all grains
    for (auto& grain : grains_) {
        grain.active = false;
    }
    
    // Update envelope sample rate
    envelope_.sampleRate = sampleRate;
    
    // Trigger envelope
    envelope_.noteOn();
}

void NoiseEngine::NoiseVoice::noteOff() {
    envelope_.noteOff();
}

void NoiseEngine::NoiseVoice::setAftertouch(float aftertouch) {
    aftertouch_ = aftertouch;
}

AudioFrame NoiseEngine::NoiseVoice::processSample() {
    if (!active_) {
        return AudioFrame(0.0f, 0.0f);
    }
    
    age_++;
    
    float deltaTime = 1.0f / sampleRate_;
    
    // Check if we should trigger a new grain
    if (scheduler_.shouldTriggerGrain(deltaTime, granularParams_.density, 
                                    granularParams_.jitter, randomSeed_)) {
        triggerGrain();
    }
    
    // Process all active grains
    float left = 0.0f, right = 0.0f;
    
    for (auto& grain : grains_) {
        if (grain.active) {
            // Update grain
            if (!grain.update(deltaTime)) {
                continue; // Grain finished
            }
            
            // Generate noise sample for this grain
            float noiseSample = generateNoiseSample();
            
            // Apply grain envelope and amplitude
            float grainOutput = noiseSample * grain.getEnvelope() * grain.amplitude;
            
            // Apply pitch (simple time-stretching effect)
            grainOutput *= grain.pitch;
            
            // Apply panning
            left += grainOutput * (1.0f - grain.pan);
            right += grainOutput * grain.pan;
        }
    }
    
    // Apply envelope
    float envLevel = envelope_.process();
    
    // Check if voice should be deactivated
    if (!envelope_.isActive()) {
        // Check if any grains are still active
        bool anyGrainsActive = false;
        for (const auto& grain : grains_) {
            if (grain.active) {
                anyGrainsActive = true;
                break;
            }
        }
        if (!anyGrainsActive) {
            active_ = false;
        }
    }
    
    // Apply velocity and volume
    left *= envLevel * velocity_ * volume_;
    right *= envLevel * velocity_ * volume_;
    
    return AudioFrame(left, right);
}

void NoiseEngine::NoiseVoice::setGranularParams(const GranularParams& params) {
    granularParams_ = params;
}

void NoiseEngine::NoiseVoice::setNoiseSource(const NoiseSource& source) {
    noiseSource_ = source;
}

void NoiseEngine::NoiseVoice::setVolume(float volume) {
    volume_ = volume;
}

void NoiseEngine::NoiseVoice::setEnvelopeParams(float attack, float decay, float sustain, float release) {
    envelope_.attack = attack;
    envelope_.decay = decay;
    envelope_.sustain = sustain;
    envelope_.release = release;
}

void NoiseEngine::NoiseVoice::triggerGrain() {
    // Find an inactive grain
    for (auto& grain : grains_) {
        if (!grain.active) {
            // Calculate grain parameters with randomization
            float duration = granularParams_.grainSize;
            
            // Add randomness to duration
            if (granularParams_.randomness > 0.0f) {
                float randomFactor = 1.0f + (randomFloat() - 0.5f) * granularParams_.randomness;
                duration *= randomFactor;
            }
            
            // Calculate amplitude with scatter
            float amplitude = 0.8f;
            if (granularParams_.scatter > 0.0f) {
                amplitude *= (1.0f - granularParams_.scatter * 0.5f + randomFloat() * granularParams_.scatter);
            }
            
            // Calculate pitch based on note frequency
            float pitch = noteFrequency_ / 440.0f; // Relative to A4
            if (granularParams_.randomness > 0.0f) {
                float pitchRandomness = (randomFloat() - 0.5f) * granularParams_.randomness * 0.2f;
                pitch *= (1.0f + pitchRandomness);
            }
            
            // Calculate pan position
            float pan = 0.5f; // Center by default
            if (granularParams_.scatter > 0.0f) {
                pan = 0.5f + (randomFloat() - 0.5f) * granularParams_.scatter;
                pan = std::clamp(pan, 0.0f, 1.0f);
            }
            
            // Trigger the grain
            grain.trigger(duration, amplitude, pitch, pan);
            break;
        }
    }
}

float NoiseEngine::NoiseVoice::generateNoiseSample() {
    return noiseSource_.generateSample(randomSeed_);
}

float NoiseEngine::NoiseVoice::randomFloat() {
    randomSeed_ = randomSeed_ * 1664525 + 1013904223;
    return static_cast<float>(randomSeed_) / 4294967296.0f;
}

// Envelope implementation
float NoiseEngine::NoiseVoice::Envelope::process() {
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