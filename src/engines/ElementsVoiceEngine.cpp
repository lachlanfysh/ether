#include "ElementsVoiceEngine.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <cmath>

ElementsVoiceEngine::ElementsVoiceEngine() {
    std::cout << "ElementsVoice engine created" << std::endl;
    
    // Initialize modulation array
    modulation_.fill(0.0f);
    
    // Calculate initial derived parameters
    calculateDerivedParams();
    
    // Set up default parameters for all voices
    updateAllVoices();
}

ElementsVoiceEngine::~ElementsVoiceEngine() {
    allNotesOff();
    std::cout << "ElementsVoice engine destroyed" << std::endl;
}

void ElementsVoiceEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    ElementsVoiceImpl* voice = findFreeVoice();
    if (!voice) {
        voice = stealVoice();
    }
    
    if (voice) {
        voice->noteOn(note, velocity, aftertouch, sampleRate_);
        voiceCounter_++;
        
        std::cout << "ElementsVoice: Note ON " << static_cast<int>(note) 
                  << " vel=" << velocity << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void ElementsVoiceEngine::noteOff(uint8_t note) {
    ElementsVoiceImpl* voice = findVoice(note);
    if (voice) {
        voice->noteOff();
        
        std::cout << "ElementsVoice: Note OFF " << static_cast<int>(note) 
                  << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void ElementsVoiceEngine::setAftertouch(uint8_t note, float aftertouch) {
    ElementsVoiceImpl* voice = findVoice(note);
    if (voice) {
        voice->setAftertouch(aftertouch);
    }
}

void ElementsVoiceEngine::allNotesOff() {
    for (auto& voice : voices_) {
        voice.noteOff();
    }
}

void ElementsVoiceEngine::setParameter(ParameterID param, float value) {
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

float ElementsVoiceEngine::getParameter(ParameterID param) const {
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

bool ElementsVoiceEngine::hasParameter(ParameterID param) const {
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

void ElementsVoiceEngine::processAudio(EtherAudioBuffer& outputBuffer) {
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
    
    // Apply voice scaling to prevent clipping (more conservative for physical modeling)
    if (activeVoices > 1) {
        float scale = 0.6f / std::sqrt(static_cast<float>(activeVoices));
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

void ElementsVoiceEngine::setHarmonics(float harmonics) {
    harmonics_ = std::clamp(harmonics, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void ElementsVoiceEngine::setTimbre(float timbre) {
    timbre_ = std::clamp(timbre, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void ElementsVoiceEngine::setMorph(float morph) {
    morph_ = std::clamp(morph, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void ElementsVoiceEngine::calculateDerivedParams() {
    // HARMONICS: exciter tone
    exciterTone_.calculateFromHarmonics(harmonics_);
    
    // TIMBRE: resonator
    resonatorSystem_.calculateFromTimbre(timbre_);
    
    // MORPH: balance + space
    balanceSpace_.calculateFromMorph(morph_);
}

ElementsVoiceEngine::ExciterTone::Type ElementsVoiceEngine::mapExciterType(float harmonics, float& blend) const {
    float scaledHarmonics = harmonics * 3.0f; // 0-3 range for 4 types
    int typeIndex = static_cast<int>(scaledHarmonics);
    blend = scaledHarmonics - typeIndex;
    
    typeIndex = std::clamp(typeIndex, 0, 3);
    return static_cast<ExciterTone::Type>(typeIndex);
}

float ElementsVoiceEngine::mapExciterColor(float harmonics) const {
    // Color frequency: 50Hz to 2kHz (exponential)
    return harmonics;
}

float ElementsVoiceEngine::mapExciterPressure(float harmonics) const {
    // Pressure/intensity: 0.1 to 1.0
    return 0.1f + harmonics * 0.9f;
}

float ElementsVoiceEngine::mapExciterSharpness(float harmonics) const {
    // Sharpness: 0.2 to 1.0 (attack character)
    return 0.2f + harmonics * 0.8f;
}

ElementsVoiceEngine::ResonatorSystem::ModelType ElementsVoiceEngine::mapResonatorModel(float timbre, float& blend) const {
    // First half: pure string to pure membrane
    // Second half: hybrid blending
    if (timbre < 0.5f) {
        blend = timbre * 2.0f;
        return ResonatorSystem::ModelType::HYBRID;
    } else {
        blend = (timbre - 0.5f) * 2.0f;
        return ResonatorSystem::ModelType::HYBRID;
    }
}

float ElementsVoiceEngine::mapStringBalance(float timbre) const {
    // String↔membrane balance
    return timbre;
}

float ElementsVoiceEngine::mapGeometry(float timbre) const {
    // Harmonic to inharmonic structure
    return timbre;
}

float ElementsVoiceEngine::mapModalSpread(float timbre) const {
    // Modal frequency relationships
    return 0.8f + timbre * 0.4f; // 0.8 to 1.2
}

float ElementsVoiceEngine::mapExciterEnergy(float morph) const {
    // Overall excitation strength
    return 0.3f + morph * 0.7f; // 0.3 to 1.0
}

float ElementsVoiceEngine::mapDampingAmount(float morph) const {
    // Global damping factor
    return morph * 0.5f; // 0 to 0.5
}

float ElementsVoiceEngine::mapStereoSpace(float morph) const {
    // Stereo width/positioning
    return morph;
}

float ElementsVoiceEngine::mapCoupling(float morph) const {
    // Inter-resonator coupling
    return morph * 0.4f; // 0 to 0.4
}

// ExciterTone implementation
void ElementsVoiceEngine::ExciterTone::calculateFromHarmonics(float harmonics) {
    // Map harmonics to exciter characteristics
    float scaledHarmonics = harmonics * 3.0f;
    int typeIndex = static_cast<int>(scaledHarmonics);
    float blend = scaledHarmonics - typeIndex;
    
    type = static_cast<Type>(std::clamp(typeIndex, 0, 3));
    color = harmonics;
    pressure = 0.1f + harmonics * 0.9f;
    sharpness = 0.2f + harmonics * 0.8f;
    turbulence = 0.1f + harmonics * 0.4f;
}

float ElementsVoiceEngine::ExciterTone::generateExcitation(float velocity, float phase, uint32_t& seed) const {
    switch (type) {
        case Type::BOW:
            return generateBowExcitation(velocity, phase);
        case Type::MALLET:
            return generateMalletExcitation(velocity, phase);
        case Type::BLOW:
            return generateBlowExcitation(velocity, seed);
        case Type::PLUCK:
            return generatePluckExcitation(velocity, phase);
        default:
            return 0.0f;
    }
}

float ElementsVoiceEngine::ExciterTone::generateBowExcitation(float velocity, float phase) const {
    // Sawtooth-like bow excitation with noise
    float bowFreq = 50.0f + color * 200.0f; // 50-250 Hz bow rate
    float bowPhase = std::fmod(phase * bowFreq / 440.0f, 1.0f);
    float sawtooth = (bowPhase * 2.0f - 1.0f) * pressure;
    
    // Add bow noise
    static uint32_t noiseState = 54321;
    noiseState = noiseState * 1664525 + 1013904223;
    float noise = (static_cast<float>(noiseState) / 4294967296.0f - 0.5f) * turbulence;
    
    return (sawtooth + noise) * velocity;
}

float ElementsVoiceEngine::ExciterTone::generateMalletExcitation(float velocity, float phase) const {
    // Sharp exponential decay with controllable hardness
    float decay = 5.0f + sharpness * 20.0f; // 5-25 decay rate
    float excitation = std::exp(-phase * decay) * velocity;
    
    // Add spectral content based on color
    if (color > 0.1f) {
        float spectralFreq = 100.0f + color * 1900.0f; // 100Hz-2kHz
        excitation *= (1.0f + 0.5f * std::sin(phase * spectralFreq * 2.0f * M_PI / 440.0f));
    }
    
    return excitation * pressure;
}

float ElementsVoiceEngine::ExciterTone::generateBlowExcitation(float velocity, uint32_t& seed) const {
    // Turbulent noise with resonance
    seed = seed * 1664525 + 1013904223;
    float noise = (static_cast<float>(seed) / 4294967296.0f - 0.5f) * 2.0f;
    
    // Filter noise based on color
    static float filterState = 0.0f;
    float cutoff = 0.1f + color * 0.4f; // 0.1 to 0.5
    filterState += cutoff * (noise - filterState);
    
    return filterState * velocity * pressure * (0.5f + turbulence);
}

float ElementsVoiceEngine::ExciterTone::generatePluckExcitation(float velocity, float phase) const {
    // Quick impulse with spectral content
    if (phase < 0.001f) {
        // Initial pluck burst
        static uint32_t pluckSeed = 12345;
        pluckSeed = pluckSeed * 1664525 + 1013904223;
        float noise = (static_cast<float>(pluckSeed) / 4294967296.0f - 0.5f);
        return noise * velocity * pressure;
    }
    return 0.0f;
}

// ResonatorSystem implementation
void ElementsVoiceEngine::ResonatorSystem::calculateFromTimbre(float timbre) {
    stringBalance = timbre;
    geometry = timbre;
    modalSpread = 0.8f + timbre * 0.4f;
    materialStiffness = timbre * 0.5f;
    
    // Determine model type
    if (timbre < 0.33f) {
        model = ModelType::STRING;
    } else if (timbre < 0.67f) {
        model = ModelType::HYBRID;
    } else {
        model = ModelType::MEMBRANE;
    }
}

// BalanceSpace implementation
void ElementsVoiceEngine::BalanceSpace::calculateFromMorph(float morph) {
    exciterEnergy = 0.3f + morph * 0.7f;
    dampingAmount = morph * 0.5f;
    dampingDecay = 1.0f + morph * 4.0f; // 1 to 5 seconds
    stereoSpace = morph;
    coupling = morph * 0.4f;
}

// Voice management methods
ElementsVoiceEngine::ElementsVoiceImpl* ElementsVoiceEngine::findFreeVoice() {
    for (auto& voice : voices_) {
        if (!voice.isActive()) {
            return &voice;
        }
    }
    return nullptr;
}

ElementsVoiceEngine::ElementsVoiceImpl* ElementsVoiceEngine::findVoice(uint8_t note) {
    for (auto& voice : voices_) {
        if (voice.isActive() && voice.getNote() == note) {
            return &voice;
        }
    }
    return nullptr;
}

ElementsVoiceEngine::ElementsVoiceImpl* ElementsVoiceEngine::stealVoice() {
    ElementsVoiceImpl* oldest = nullptr;
    uint32_t oldestAge = 0;
    
    for (auto& voice : voices_) {
        if (voice.getAge() > oldestAge) {
            oldestAge = voice.getAge();
            oldest = &voice;
        }
    }
    
    return oldest;
}

void ElementsVoiceEngine::updateAllVoices() {
    for (auto& voice : voices_) {
        voice.setExciterTone(exciterTone_);
        voice.setResonatorSystem(resonatorSystem_);
        voice.setBalanceSpace(balanceSpace_);
        voice.setVolume(volume_);
        voice.setEnvelopeParams(attack_, decay_, sustain_, release_);
    }
}

// Standard SynthEngine methods
size_t ElementsVoiceEngine::getActiveVoiceCount() const {
    size_t count = 0;
    for (const auto& voice : voices_) {
        if (voice.isActive()) {
            count++;
        }
    }
    return count;
}

void ElementsVoiceEngine::setVoiceCount(size_t maxVoices) {
    // Voice count is fixed for this implementation
}

void ElementsVoiceEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
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

bool ElementsVoiceEngine::loadPreset(const uint8_t* data, size_t size) {
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

void ElementsVoiceEngine::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updateAllVoices();
}

void ElementsVoiceEngine::setBufferSize(size_t bufferSize) {
    bufferSize_ = bufferSize;
}

bool ElementsVoiceEngine::supportsModulation(ParameterID target) const {
    return hasParameter(target);
}

void ElementsVoiceEngine::setModulation(ParameterID target, float amount) {
    size_t index = static_cast<size_t>(target);
    if (index < modulation_.size()) {
        modulation_[index] = amount;
    }
}

void ElementsVoiceEngine::updateCPUUsage(float processingTime) {
    float maxTime = (BUFFER_SIZE / sampleRate_) * 1000.0f;
    cpuUsage_ = std::min(100.0f, (processingTime / maxTime) * 100.0f);
}

// ElementsVoiceImpl implementation
ElementsVoiceEngine::ElementsVoiceImpl::ElementsVoiceImpl() {
    envelope_.sampleRate = 48000.0f;
    randomSeed_ = 12345 + reinterpret_cast<uintptr_t>(this);
}

void ElementsVoiceEngine::ElementsVoiceImpl::noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate) {
    note_ = note;
    velocity_ = velocity;
    aftertouch_ = aftertouch;
    active_ = true;
    age_ = 0;
    sampleRate_ = sampleRate;
    excitationPhase_ = 0.0f;
    
    // Calculate note frequency
    noteFrequency_ = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    
    // Set up string model
    stringModel_.setFrequency(noteFrequency_, sampleRate);
    stringModel_.reset();
    
    // Set up membrane model
    membraneModel_.setFrequency(noteFrequency_);
    membraneModel_.sampleRate = sampleRate;
    membraneModel_.reset();
    
    // Initial excitation based on exciter type
    if (exciterTone_.type == ExciterTone::Type::PLUCK) {
        stringModel_.pluck(velocity, randomSeed_);
    } else if (exciterTone_.type == ExciterTone::Type::MALLET) {
        membraneModel_.strike(velocity);
    }
    
    // Update envelope sample rate
    envelope_.sampleRate = sampleRate;
    
    // Trigger envelope
    envelope_.noteOn();
}

void ElementsVoiceEngine::ElementsVoiceImpl::noteOff() {
    envelope_.noteOff();
}

void ElementsVoiceEngine::ElementsVoiceImpl::setAftertouch(float aftertouch) {
    aftertouch_ = aftertouch;
}

AudioFrame ElementsVoiceEngine::ElementsVoiceImpl::processSample() {
    if (!active_) {
        return AudioFrame(0.0f, 0.0f);
    }
    
    age_++;
    excitationPhase_ += noteFrequency_ / sampleRate_;
    if (excitationPhase_ >= 1.0f) excitationPhase_ -= 1.0f;
    
    // Generate excitation
    float excitation = exciterTone_.generateExcitation(velocity_, excitationPhase_, randomSeed_);
    excitation *= balanceSpace_.exciterEnergy;
    
    // Process through physical models
    float stringOutput = stringModel_.process(excitation);
    float membraneOutput = membraneModel_.process(excitation);
    
    // Balance between string and membrane based on resonator system
    float mixed = stringOutput * (1.0f - resonatorSystem_.stringBalance) + 
                  membraneOutput * resonatorSystem_.stringBalance;
    
    // Apply envelope
    float envLevel = envelope_.process();
    
    // Check if voice should be deactivated
    if (!envelope_.isActive()) {
        active_ = false;
    }
    
    // Apply velocity and volume
    mixed *= envLevel * velocity_ * volume_;
    
    // Apply stereo space processing
    AudioFrame output = spaceProcessor_.process(mixed);
    
    return output;
}

void ElementsVoiceEngine::ElementsVoiceImpl::setExciterTone(const ExciterTone& tone) {
    exciterTone_ = tone;
}

void ElementsVoiceEngine::ElementsVoiceImpl::setResonatorSystem(const ResonatorSystem& system) {
    resonatorSystem_ = system;
    
    // Update string model parameters
    stringModel_.setDamping(balanceSpace_.dampingAmount);
    stringModel_.setStiffness(system.materialStiffness);
    
    // Update membrane model parameters
    membraneModel_.setGeometry(system.geometry);
    membraneModel_.setDamping(balanceSpace_.dampingAmount);
}

void ElementsVoiceEngine::ElementsVoiceImpl::setBalanceSpace(const BalanceSpace& balance) {
    balanceSpace_ = balance;
    
    // Update space processor
    spaceProcessor_.setSpace(balance.stereoSpace);
}

void ElementsVoiceEngine::ElementsVoiceImpl::setVolume(float volume) {
    volume_ = volume;
}

void ElementsVoiceEngine::ElementsVoiceImpl::setEnvelopeParams(float attack, float decay, float sustain, float release) {
    envelope_.attack = attack;
    envelope_.decay = decay;
    envelope_.sustain = sustain;
    envelope_.release = release;
}

float ElementsVoiceEngine::ElementsVoiceImpl::generateNoise() {
    randomSeed_ = randomSeed_ * 1664525 + 1013904223;
    return (static_cast<float>(randomSeed_) / 4294967296.0f) - 0.5f;
}

// StringModel implementation
void ElementsVoiceEngine::ElementsVoiceImpl::StringModel::setFrequency(float freq, float sampleRate) {
    delayLength = std::clamp(sampleRate / freq, 10.0f, static_cast<float>(MAX_DELAY - 1));
}

float ElementsVoiceEngine::ElementsVoiceImpl::StringModel::process(float excitation) {
    // Read from delay line
    float delayedSample = readDelay(delayLength);
    
    // Apply damping (lowpass filtering)
    dampingState = dampingState + damping * (delayedSample - dampingState);
    float dampedSample = dampingState;
    
    // Apply stiffness (allpass filtering for dispersion)
    if (stiffness > 0.0f) {
        float allpassOut = -stiffness * dampedSample + allpassState;
        allpassState = dampedSample + stiffness * allpassOut;
        dampedSample = allpassOut;
    }
    
    // Feedback into delay line
    writeDelay(dampedSample * 0.995f + excitation);
    
    return dampedSample;
}

void ElementsVoiceEngine::ElementsVoiceImpl::StringModel::pluck(float energy, uint32_t& seed) {
    // Initialize with triangular displacement
    for (int i = 0; i < static_cast<int>(delayLength); ++i) {
        float pos = static_cast<float>(i) / delayLength;
        float envelope = std::sin(pos * M_PI);
        
        // Add some noise for realism
        seed = seed * 1664525 + 1013904223;
        float noise = (static_cast<float>(seed) / 4294967296.0f - 0.5f) * 0.1f;
        
        delayLine[(writePos + i) % MAX_DELAY] = envelope * energy + noise;
    }
}

void ElementsVoiceEngine::ElementsVoiceImpl::StringModel::reset() {
    delayLine.fill(0.0f);
    dampingState = 0.0f;
    allpassState = 0.0f;
    writePos = 0;
}

void ElementsVoiceEngine::ElementsVoiceImpl::StringModel::writeDelay(float sample) {
    delayLine[writePos] = sample;
    writePos = (writePos + 1) % MAX_DELAY;
}

float ElementsVoiceEngine::ElementsVoiceImpl::StringModel::readDelay(float delay) const {
    float readPos = writePos - delay;
    if (readPos < 0) readPos += MAX_DELAY;
    
    // Linear interpolation
    int pos1 = static_cast<int>(readPos) % MAX_DELAY;
    int pos2 = (pos1 + 1) % MAX_DELAY;
    float frac = readPos - static_cast<int>(readPos);
    
    return delayLine[pos1] * (1.0f - frac) + delayLine[pos2] * frac;
}

// MembraneModel implementation
void ElementsVoiceEngine::ElementsVoiceImpl::MembraneModel::setGeometry(float geom) {
    geometry = geom;
    updateModes();
}

void ElementsVoiceEngine::ElementsVoiceImpl::MembraneModel::setDamping(float damp) {
    damping = damp;
    updateModes();
}

void ElementsVoiceEngine::ElementsVoiceImpl::MembraneModel::setFrequency(float freq) {
    updateModes();
    // Scale all mode frequencies
    for (auto& mode : modes) {
        mode.frequency *= freq / 220.0f;
    }
}

float ElementsVoiceEngine::ElementsVoiceImpl::MembraneModel::process(float excitation) {
    float output = 0.0f;
    
    for (auto& mode : modes) {
        if (mode.amplitude > 0.001f) {
            // Update phase
            mode.phase += mode.frequency * 2.0f * M_PI / sampleRate;
            if (mode.phase >= 2.0f * M_PI) {
                mode.phase -= 2.0f * M_PI;
            }
            
            // Generate mode output
            float modeOutput = std::sin(mode.phase) * mode.amplitude;
            output += modeOutput;
            
            // Apply damping
            mode.amplitude *= (1.0f - mode.damping / sampleRate);
        }
    }
    
    // Excite modes
    if (std::abs(excitation) > 0.001f) {
        for (int i = 0; i < NUM_MODES; ++i) {
            modes[i].amplitude += excitation * energy * (1.0f / (i + 1));
            modes[i].amplitude = std::clamp(modes[i].amplitude, 0.0f, 1.0f);
        }
    }
    
    return output * 0.25f;
}

void ElementsVoiceEngine::ElementsVoiceImpl::MembraneModel::strike(float energy) {
    for (int i = 0; i < NUM_MODES; ++i) {
        modes[i].amplitude += energy * this->energy * (1.0f / (i + 1));
        modes[i].amplitude = std::clamp(modes[i].amplitude, 0.0f, 1.0f);
    }
}

void ElementsVoiceEngine::ElementsVoiceImpl::MembraneModel::reset() {
    for (auto& mode : modes) {
        mode.reset();
    }
}

void ElementsVoiceEngine::ElementsVoiceImpl::MembraneModel::updateModes() {
    for (int i = 0; i < NUM_MODES; ++i) {
        float harmonicRatio = (i + 1);
        float inharmonicRatio = std::sqrt((i + 1) * (i + 1) + geometry * i * 2.0f);
        
        float ratio = harmonicRatio * (1.0f - geometry) + inharmonicRatio * geometry;
        modes[i].frequency = 220.0f * ratio;
        modes[i].damping = damping * (1.0f + i * 0.1f);
    }
}

// SpaceProcessor implementation
void ElementsVoiceEngine::ElementsVoiceImpl::SpaceProcessor::setSpace(float space) {
    width = space;
    position = (space - 0.5f) * 2.0f; // -1 to 1
}

AudioFrame ElementsVoiceEngine::ElementsVoiceImpl::SpaceProcessor::process(float input) {
    // Simple stereo width and positioning
    float left = input * (1.0f - position * 0.5f);
    float right = input * (1.0f + position * 0.5f);
    
    // Apply width
    float mid = (left + right) * 0.5f;
    float side = (left - right) * width;
    
    left = mid + side;
    right = mid - side;
    
    return AudioFrame(left, right);
}

// Envelope implementation
float ElementsVoiceEngine::ElementsVoiceImpl::Envelope::process() {
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