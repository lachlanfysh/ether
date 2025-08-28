#include "MacroHarmonicsEngine.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <cmath>

MacroHarmonicsEngine::MacroHarmonicsEngine() {
    std::cout << "MacroHarmonics engine created" << std::endl;
    
    // Initialize modulation array
    modulation_.fill(0.0f);
    
    // Calculate initial derived parameters
    calculateDerivedParams();
    
    // Set up default parameters for all voices
    updateAllVoices();
}

MacroHarmonicsEngine::~MacroHarmonicsEngine() {
    allNotesOff();
    std::cout << "MacroHarmonics engine destroyed" << std::endl;
}

void MacroHarmonicsEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    MacroHarmonicsVoice* voice = findFreeVoice();
    if (!voice) {
        voice = stealVoice();
    }
    
    if (voice) {
        voice->noteOn(note, velocity, aftertouch, sampleRate_);
        voiceCounter_++;
        
        std::cout << "MacroHarmonics: Note ON " << static_cast<int>(note) 
                  << " vel=" << velocity << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void MacroHarmonicsEngine::noteOff(uint8_t note) {
    MacroHarmonicsVoice* voice = findVoice(note);
    if (voice) {
        voice->noteOff();
        
        std::cout << "MacroHarmonics: Note OFF " << static_cast<int>(note) 
                  << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void MacroHarmonicsEngine::setAftertouch(uint8_t note, float aftertouch) {
    MacroHarmonicsVoice* voice = findVoice(note);
    if (voice) {
        voice->setAftertouch(aftertouch);
    }
}

void MacroHarmonicsEngine::allNotesOff() {
    for (auto& voice : voices_) {
        voice.noteOff();
    }
}

void MacroHarmonicsEngine::setParameter(ParameterID param, float value) {
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
            attack_ = std::clamp(value, 0.001f, 0.1f); // Limited range for organ
            updateAllVoices();
            break;
            
        case ParameterID::DECAY:
            decay_ = 0.0f; // Always 0 for organ
            break;
            
        case ParameterID::SUSTAIN:
            sustain_ = 1.0f; // Always 1 for organ
            break;
            
        case ParameterID::RELEASE:
            release_ = std::clamp(value, 0.01f, 1.0f);
            updateAllVoices();
            break;
            
        default:
            break;
    }
}

float MacroHarmonicsEngine::getParameter(ParameterID param) const {
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

bool MacroHarmonicsEngine::hasParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::HARMONICS:
        case ParameterID::TIMBRE:
        case ParameterID::MORPH:
        case ParameterID::VOLUME:
        case ParameterID::ATTACK:
        case ParameterID::RELEASE:
            return true;
        case ParameterID::DECAY:
        case ParameterID::SUSTAIN:
            return false; // Fixed for organ
        default:
            return false;
    }
}

void MacroHarmonicsEngine::processAudio(EtherAudioBuffer& outputBuffer) {
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

void MacroHarmonicsEngine::setHarmonics(float harmonics) {
    harmonics_ = std::clamp(harmonics, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroHarmonicsEngine::setTimbre(float timbre) {
    timbre_ = std::clamp(timbre, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroHarmonicsEngine::setMorph(float morph) {
    morph_ = std::clamp(morph, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroHarmonicsEngine::calculateDerivedParams() {
    // HARMONICS: odd/even balance + level scaler
    harmonicSettings_.calculateFromHarmonics(harmonics_);
    
    // TIMBRE: drawbar groups
    harmonicSettings_.calculateFromTimbre(timbre_);
    
    // MORPH: leakage + decay
    tonewheelModel_.calculateFromMorph(morph_);
    
    // Update harmonic levels based on all parameters
    harmonicSettings_.updateHarmonicLevels();
    tonewheelModel_.updateLeakageMatrix();
}

float MacroHarmonicsEngine::mapOddEvenBalance(float harmonics) const {
    // 0 = even harmonics emphasis, 1 = odd harmonics emphasis
    return harmonics;
}

float MacroHarmonicsEngine::mapLevelScaler(float harmonics) const {
    // Level scaler affects overall harmonic content
    // 0 = softer harmonics, 1 = fuller harmonics
    return 0.3f + harmonics * 0.7f;
}

MacroHarmonicsEngine::HarmonicSettings::DrawbarGroups MacroHarmonicsEngine::mapDrawbarGroups(float timbre) const {
    HarmonicSettings::DrawbarGroups groups;
    
    if (timbre < 0.33f) {
        // Foundation emphasis (low harmonics)
        groups.foundation = 0.9f;
        groups.principals = 0.4f + timbre * 0.6f;
        groups.mixtures = 0.1f + timbre * 0.3f;
    } else if (timbre < 0.66f) {
        // Principals emphasis (mid harmonics)
        float localT = (timbre - 0.33f) * 3.0f;
        groups.foundation = 0.9f - localT * 0.3f;
        groups.principals = 0.8f;
        groups.mixtures = 0.2f + localT * 0.4f;
    } else {
        // Mixtures emphasis (high harmonics)
        float localT = (timbre - 0.66f) * 3.0f;
        groups.foundation = 0.6f;
        groups.principals = 0.8f - localT * 0.2f;
        groups.mixtures = 0.6f + localT * 0.3f;
    }
    
    return groups;
}

float MacroHarmonicsEngine::mapLeakage(float morph) const {
    // Tonewheel leakage: 0 to 0.3
    return morph * 0.3f;
}

float MacroHarmonicsEngine::mapDecay(float morph) const {
    // Harmonic decay: 0 to 0.5
    return morph * 0.5f;
}

// HarmonicSettings implementation
void MacroHarmonicsEngine::HarmonicSettings::calculateFromHarmonics(float harmonics) {
    oddEvenBalance = harmonics;
    levelScaler = 0.3f + harmonics * 0.7f;
}

void MacroHarmonicsEngine::HarmonicSettings::calculateFromTimbre(float timbre) {
    if (timbre < 0.33f) {
        // Foundation emphasis
        drawbars.foundation = 0.9f;
        drawbars.principals = 0.4f + timbre * 0.6f;
        drawbars.mixtures = 0.1f + timbre * 0.3f;
    } else if (timbre < 0.66f) {
        // Principals emphasis
        float localT = (timbre - 0.33f) * 3.0f;
        drawbars.foundation = 0.9f - localT * 0.3f;
        drawbars.principals = 0.8f;
        drawbars.mixtures = 0.2f + localT * 0.4f;
    } else {
        // Mixtures emphasis
        float localT = (timbre - 0.66f) * 3.0f;
        drawbars.foundation = 0.6f;
        drawbars.principals = 0.8f - localT * 0.2f;
        drawbars.mixtures = 0.6f + localT * 0.3f;
    }
}

void MacroHarmonicsEngine::HarmonicSettings::updateHarmonicLevels() {
    // Base harmonic levels
    levels[0] = 1.0f;      // Fundamental
    levels[1] = 0.8f;      // 2nd harmonic
    levels[2] = 0.6f;      // 3rd harmonic
    levels[3] = 0.5f;      // 4th harmonic
    levels[4] = 0.4f;      // 5th harmonic
    levels[5] = 0.3f;      // 6th harmonic
    levels[6] = 0.2f;      // 7th harmonic
    levels[7] = 0.15f;     // 8th harmonic
    
    // Apply odd/even balance
    for (int i = 0; i < NUM_HARMONICS; i++) {
        if (i % 2 == 0) {
            // Even harmonics (including fundamental)
            levels[i] *= (2.0f - oddEvenBalance);
        } else {
            // Odd harmonics (2nd, 4th, 6th, 8th)
            levels[i] *= (1.0f + oddEvenBalance);
        }
    }
    
    // Apply drawbar groupings
    // Foundation: 1st, 2nd harmonics
    levels[0] *= drawbars.foundation;
    levels[1] *= drawbars.foundation;
    
    // Principals: 3rd, 4th, 5th harmonics
    levels[2] *= drawbars.principals;
    levels[3] *= drawbars.principals;
    levels[4] *= drawbars.principals;
    
    // Mixtures: 6th, 7th, 8th harmonics
    levels[5] *= drawbars.mixtures;
    levels[6] *= drawbars.mixtures;
    levels[7] *= drawbars.mixtures;
    
    // Apply overall level scaler
    for (int i = 0; i < NUM_HARMONICS; i++) {
        levels[i] *= levelScaler;
        levels[i] = std::clamp(levels[i], 0.0f, 1.0f);
    }
}

// TonewheelModel implementation
void MacroHarmonicsEngine::TonewheelModel::calculateFromMorph(float morph) {
    leakage = morph * 0.3f;
    decay = morph * 0.5f;
    crosstalk = morph * 0.1f;
}

void MacroHarmonicsEngine::TonewheelModel::updateLeakageMatrix() {
    // Initialize leakage matrix
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (i == j) {
                leakageMatrix[i][j] = 1.0f; // Self
            } else {
                // Adjacent harmonics leak more
                int distance = std::abs(i - j);
                float leakageAmount = leakage / (1.0f + distance * 0.5f);
                leakageMatrix[i][j] = leakageAmount;
            }
        }
    }
}

float MacroHarmonicsEngine::TonewheelModel::applyLeakage(const std::array<float, 8>& harmonics, int targetHarmonic) const {
    float result = 0.0f;
    
    for (int i = 0; i < 8; i++) {
        result += harmonics[i] * leakageMatrix[targetHarmonic][i];
    }
    
    return result;
}

// Voice management methods
MacroHarmonicsEngine::MacroHarmonicsVoice* MacroHarmonicsEngine::findFreeVoice() {
    for (auto& voice : voices_) {
        if (!voice.isActive()) {
            return &voice;
        }
    }
    return nullptr;
}

MacroHarmonicsEngine::MacroHarmonicsVoice* MacroHarmonicsEngine::findVoice(uint8_t note) {
    for (auto& voice : voices_) {
        if (voice.isActive() && voice.getNote() == note) {
            return &voice;
        }
    }
    return nullptr;
}

MacroHarmonicsEngine::MacroHarmonicsVoice* MacroHarmonicsEngine::stealVoice() {
    MacroHarmonicsVoice* oldest = nullptr;
    uint32_t oldestAge = 0;
    
    for (auto& voice : voices_) {
        if (voice.getAge() > oldestAge) {
            oldestAge = voice.getAge();
            oldest = &voice;
        }
    }
    
    return oldest;
}

void MacroHarmonicsEngine::updateAllVoices() {
    for (auto& voice : voices_) {
        voice.setHarmonicParams(harmonicSettings_);
        voice.setTonewheelParams(tonewheelModel_);
        voice.setVolume(volume_);
        voice.setEnvelopeParams(attack_, decay_, sustain_, release_);
    }
}

// Standard SynthEngine methods
size_t MacroHarmonicsEngine::getActiveVoiceCount() const {
    size_t count = 0;
    for (const auto& voice : voices_) {
        if (voice.isActive()) {
            count++;
        }
    }
    return count;
}

void MacroHarmonicsEngine::setVoiceCount(size_t maxVoices) {
    // Voice count is fixed for this implementation
}

void MacroHarmonicsEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
    struct PresetData {
        float harmonics, timbre, morph, volume;
        float attack, release;
    };
    
    PresetData preset = {
        harmonics_, timbre_, morph_, volume_,
        attack_, release_
    };
    
    actualSize = sizeof(PresetData);
    if (maxSize >= actualSize) {
        memcpy(data, &preset, actualSize);
    }
}

bool MacroHarmonicsEngine::loadPreset(const uint8_t* data, size_t size) {
    struct PresetData {
        float harmonics, timbre, morph, volume;
        float attack, release;
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
    release_ = preset->release;
    
    calculateDerivedParams();
    updateAllVoices();
    return true;
}

void MacroHarmonicsEngine::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updateAllVoices();
}

void MacroHarmonicsEngine::setBufferSize(size_t bufferSize) {
    bufferSize_ = bufferSize;
}

bool MacroHarmonicsEngine::supportsModulation(ParameterID target) const {
    return hasParameter(target);
}

void MacroHarmonicsEngine::setModulation(ParameterID target, float amount) {
    size_t index = static_cast<size_t>(target);
    if (index < modulation_.size()) {
        modulation_[index] = amount;
    }
}

void MacroHarmonicsEngine::updateCPUUsage(float processingTime) {
    float maxTime = (BUFFER_SIZE / sampleRate_) * 1000.0f;
    cpuUsage_ = std::min(100.0f, (processingTime / maxTime) * 100.0f);
}

// MacroHarmonicsVoice implementation
MacroHarmonicsEngine::MacroHarmonicsVoice::MacroHarmonicsVoice() {
    envelope_.sampleRate = 48000.0f;
}

void MacroHarmonicsEngine::MacroHarmonicsVoice::noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate) {
    note_ = note;
    velocity_ = velocity;
    aftertouch_ = aftertouch;
    active_ = true;
    age_ = 0;
    
    // Calculate note frequency
    noteFrequency_ = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    
    // Set up harmonic oscillators
    for (int i = 0; i < HarmonicSettings::NUM_HARMONICS; i++) {
        float harmonicFreq = noteFrequency_ * (i + 1); // 1st, 2nd, 3rd... harmonics
        harmonics_[i].setFrequency(harmonicFreq, sampleRate);
        harmonics_[i].setTargetLevel(harmonicSettings_.levels[i]);
    }
    
    // Update envelope sample rate
    envelope_.sampleRate = sampleRate;
    
    // Trigger envelope
    envelope_.noteOn();
}

void MacroHarmonicsEngine::MacroHarmonicsVoice::noteOff() {
    envelope_.noteOff();
}

void MacroHarmonicsEngine::MacroHarmonicsVoice::setAftertouch(float aftertouch) {
    aftertouch_ = aftertouch;
}

AudioFrame MacroHarmonicsEngine::MacroHarmonicsVoice::processSample() {
    if (!active_) {
        return AudioFrame(0.0f, 0.0f);
    }
    
    age_++;
    
    // Update harmonic levels smoothly
    for (int i = 0; i < HarmonicSettings::NUM_HARMONICS; i++) {
        harmonics_[i].updateLevel();
    }
    
    // Generate harmonic outputs
    std::array<float, HarmonicSettings::NUM_HARMONICS> harmonicOutputs;
    for (int i = 0; i < HarmonicSettings::NUM_HARMONICS; i++) {
        harmonicOutputs[i] = harmonics_[i].processSine();
    }
    
    // Apply tonewheel leakage
    float mixed = 0.0f;
    for (int i = 0; i < HarmonicSettings::NUM_HARMONICS; i++) {
        float harmonicWithLeakage = tonewheelModel_.applyLeakage(harmonicOutputs, i);
        mixed += harmonicWithLeakage;
    }
    
    // Normalize
    mixed /= HarmonicSettings::NUM_HARMONICS;
    
    // Apply envelope
    float envLevel = envelope_.process();
    
    // Check if voice should be deactivated
    if (!envelope_.isActive()) {
        active_ = false;
    }
    
    // Apply velocity and volume
    float output = mixed * envLevel * velocity_ * volume_;
    
    return AudioFrame(output, output);
}

void MacroHarmonicsEngine::MacroHarmonicsVoice::setHarmonicParams(const HarmonicSettings& settings) {
    harmonicSettings_ = settings;
    
    // Update target levels for smooth transitions
    for (int i = 0; i < HarmonicSettings::NUM_HARMONICS; i++) {
        harmonics_[i].setTargetLevel(settings.levels[i]);
    }
}

void MacroHarmonicsEngine::MacroHarmonicsVoice::setTonewheelParams(const TonewheelModel& model) {
    tonewheelModel_ = model;
}

void MacroHarmonicsEngine::MacroHarmonicsVoice::setVolume(float volume) {
    volume_ = volume;
}

void MacroHarmonicsEngine::MacroHarmonicsVoice::setEnvelopeParams(float attack, float decay, float sustain, float release) {
    envelope_.attack = attack;
    envelope_.decay = 0.0f;     // Always 0 for organ
    envelope_.sustain = 1.0f;   // Always 1 for organ
    envelope_.release = release;
}

// OrganEnvelope implementation
float MacroHarmonicsEngine::MacroHarmonicsVoice::OrganEnvelope::process() {
    const float attackRate = 1.0f / (attack * sampleRate);
    const float releaseRate = 1.0f / (release * sampleRate);
    
    switch (stage) {
        case Stage::IDLE:
            return 0.0f;
            
        case Stage::ATTACK:
            level += attackRate;
            if (level >= 1.0f) {
                level = 1.0f;
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