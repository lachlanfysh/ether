#include "MacroChordEngine.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <cmath>

MacroChordEngine::MacroChordEngine() {
    std::cout << "MacroChord engine created" << std::endl;
    
    // Initialize modulation array
    modulation_.fill(0.0f);
    
    // Calculate initial derived parameters
    calculateDerivedParams();
    
    // Set up default parameters for all voices
    updateAllVoices();
}

MacroChordEngine::~MacroChordEngine() {
    allNotesOff();
    std::cout << "MacroChord engine destroyed" << std::endl;
}

void MacroChordEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    MacroChordVoice* voice = findFreeVoice();
    if (!voice) {
        voice = stealVoice();
    }
    
    if (voice) {
        voice->noteOn(note, velocity, aftertouch, sampleRate_, chordVoicing_);
        voiceCounter_++;
        
        std::cout << "MacroChord: Note ON " << static_cast<int>(note) 
                  << " vel=" << velocity << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void MacroChordEngine::noteOff(uint8_t note) {
    MacroChordVoice* voice = findVoice(note);
    if (voice) {
        voice->noteOff();
        
        std::cout << "MacroChord: Note OFF " << static_cast<int>(note) 
                  << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void MacroChordEngine::setAftertouch(uint8_t note, float aftertouch) {
    MacroChordVoice* voice = findVoice(note);
    if (voice) {
        voice->setAftertouch(aftertouch);
    }
}

void MacroChordEngine::allNotesOff() {
    for (auto& voice : voices_) {
        voice.noteOff();
    }
}

void MacroChordEngine::setParameter(ParameterID param, float value) {
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

float MacroChordEngine::getParameter(ParameterID param) const {
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

bool MacroChordEngine::hasParameter(ParameterID param) const {
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

void MacroChordEngine::processAudio(EtherAudioBuffer& outputBuffer) {
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

void MacroChordEngine::setHarmonics(float harmonics) {
    harmonics_ = std::clamp(harmonics, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroChordEngine::setTimbre(float timbre) {
    timbre_ = std::clamp(timbre, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroChordEngine::setMorph(float morph) {
    morph_ = std::clamp(morph, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroChordEngine::calculateDerivedParams() {
    // HARMONICS: detune spread + LPF cutoff
    chordVoicing_.calculateDetune(harmonics_);
    filterCutoff_ = mapFilterCutoff(harmonics_);
    filterResonance_ = mapFilterResonance(harmonics_);
    
    // TIMBRE: voicing complexity (triad → 7th → 9th → 11th)
    chordVoicing_.calculateVoicing(timbre_);
    
    // MORPH: chord↔single blend
    chordSingleBlend_ = mapChordSingleBlend(morph_);
    chordVoicing_.calculateLevels(morph_, harmonics_);
}

float MacroChordEngine::mapDetuneSpread(float harmonics) const {
    // Detune spread: 0 to ±15 cents
    return harmonics * 15.0f;
}

float MacroChordEngine::mapFilterCutoff(float harmonics) const {
    // Filter cutoff: 200Hz to 8kHz (exponential)
    // At harmonics=0: tight, controlled sound
    // At harmonics=1: open, bright chord
    return 200.0f * std::pow(40.0f, harmonics); // 200Hz to 8kHz
}

float MacroChordEngine::mapFilterResonance(float harmonics) const {
    // Resonance: 0.1 to 0.6 (moderate)
    return 0.1f + harmonics * 0.5f;
}

MacroChordEngine::ChordVoicing::VoicingType MacroChordEngine::mapVoicingType(float timbre) const {
    if (timbre < 0.25f) return ChordVoicing::VoicingType::TRIAD;
    else if (timbre < 0.5f) return ChordVoicing::VoicingType::SEVENTH;
    else if (timbre < 0.75f) return ChordVoicing::VoicingType::NINTH;
    else return ChordVoicing::VoicingType::ELEVENTH;
}

float MacroChordEngine::mapChordSingleBlend(float morph) const {
    // Direct mapping: 0=full chord, 1=single note
    return morph;
}

// ChordVoicing implementation
void MacroChordEngine::ChordVoicing::calculateVoicing(float timbre) {
    // Map timbre to voicing type
    if (timbre < 0.25f) {
        currentVoicing = VoicingType::TRIAD;
        activeNoteCount = 3;
        for (int i = 0; i < activeNoteCount; i++) {
            notes[i].semitoneOffset = TRIAD_INTERVALS[i];
        }
    } else if (timbre < 0.5f) {
        currentVoicing = VoicingType::SEVENTH;
        activeNoteCount = 4;
        for (int i = 0; i < activeNoteCount; i++) {
            notes[i].semitoneOffset = SEVENTH_INTERVALS[i];
        }
    } else if (timbre < 0.75f) {
        currentVoicing = VoicingType::NINTH;
        activeNoteCount = 5;
        for (int i = 0; i < activeNoteCount; i++) {
            notes[i].semitoneOffset = NINTH_INTERVALS[i];
        }
    } else {
        currentVoicing = VoicingType::ELEVENTH;
        activeNoteCount = 6;
        for (int i = 0; i < activeNoteCount; i++) {
            notes[i].semitoneOffset = ELEVENTH_INTERVALS[i];
        }
    }
}

void MacroChordEngine::ChordVoicing::calculateDetune(float harmonics) {
    detuneSpread_ = harmonics * 15.0f; // 0 to ±15 cents
    
    // Apply detune pattern across notes
    for (int i = 0; i < MAX_CHORD_NOTES; i++) {
        if (i < activeNoteCount) {
            // Spread detune evenly across notes
            float detunePosition = (i - activeNoteCount * 0.5f) / activeNoteCount;
            notes[i].detuneAmount = detunePosition * detuneSpread_;
        } else {
            notes[i].detuneAmount = 0.0f;
        }
    }
}

void MacroChordEngine::ChordVoicing::calculateLevels(float morph, float harmonics) {
    // Calculate level scaling based on chord/single blend
    for (int i = 0; i < MAX_CHORD_NOTES; i++) {
        if (i < activeNoteCount) {
            if (i == 0) {
                // Root note always present
                notes[i].levelScale = 1.0f;
            } else {
                // Other notes fade with morph
                notes[i].levelScale = 1.0f - morph;
                
                // Higher chord extensions fade faster
                if (i >= 4) { // 9th and 11th
                    notes[i].levelScale *= (1.0f - morph * 0.5f);
                }
            }
            
            // Apply slight level variation based on harmonics
            notes[i].levelScale *= (0.8f + harmonics * 0.3f);
        } else {
            notes[i].levelScale = 0.0f;
        }
    }
}

// Voice management methods
MacroChordEngine::MacroChordVoice* MacroChordEngine::findFreeVoice() {
    for (auto& voice : voices_) {
        if (!voice.isActive()) {
            return &voice;
        }
    }
    return nullptr;
}

MacroChordEngine::MacroChordVoice* MacroChordEngine::findVoice(uint8_t note) {
    for (auto& voice : voices_) {
        if (voice.isActive() && voice.getNote() == note) {
            return &voice;
        }
    }
    return nullptr;
}

MacroChordEngine::MacroChordVoice* MacroChordEngine::stealVoice() {
    MacroChordVoice* oldest = nullptr;
    uint32_t oldestAge = 0;
    
    for (auto& voice : voices_) {
        if (voice.getAge() > oldestAge) {
            oldestAge = voice.getAge();
            oldest = &voice;
        }
    }
    
    return oldest;
}

void MacroChordEngine::updateAllVoices() {
    for (auto& voice : voices_) {
        voice.setChordParams(chordVoicing_);
        voice.setFilterParams(filterCutoff_, filterResonance_);
        voice.setMorphParams(chordSingleBlend_);
        voice.setVolume(volume_);
        voice.setEnvelopeParams(attack_, decay_, sustain_, release_);
    }
}

// Standard SynthEngine methods
size_t MacroChordEngine::getActiveVoiceCount() const {
    size_t count = 0;
    for (const auto& voice : voices_) {
        if (voice.isActive()) {
            count++;
        }
    }
    return count;
}

void MacroChordEngine::setVoiceCount(size_t maxVoices) {
    // Voice count is fixed for this implementation
}

void MacroChordEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
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

bool MacroChordEngine::loadPreset(const uint8_t* data, size_t size) {
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

void MacroChordEngine::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updateAllVoices();
}

void MacroChordEngine::setBufferSize(size_t bufferSize) {
    bufferSize_ = bufferSize;
}

bool MacroChordEngine::supportsModulation(ParameterID target) const {
    return hasParameter(target);
}

void MacroChordEngine::setModulation(ParameterID target, float amount) {
    size_t index = static_cast<size_t>(target);
    if (index < modulation_.size()) {
        modulation_[index] = amount;
    }
}

void MacroChordEngine::updateCPUUsage(float processingTime) {
    float maxTime = (BUFFER_SIZE / sampleRate_) * 1000.0f;
    cpuUsage_ = std::min(100.0f, (processingTime / maxTime) * 100.0f);
}

// MacroChordVoice implementation
MacroChordEngine::MacroChordVoice::MacroChordVoice() {
    filter_.sampleRate = 48000.0f;
    filter_.updateCoefficients();
    envelope_.sampleRate = 48000.0f;
}

void MacroChordEngine::MacroChordVoice::noteOn(uint8_t rootNote, float velocity, float aftertouch, 
                                        float sampleRate, const ChordVoicing& voicing) {
    rootNote_ = rootNote;
    velocity_ = velocity;
    aftertouch_ = aftertouch;
    active_ = true;
    age_ = 0;
    
    // Calculate root frequency
    rootFrequency_ = 440.0f * std::pow(2.0f, (rootNote - 69) / 12.0f);
    
    // Set up oscillators for chord notes
    activeOscCount_ = voicing.activeNoteCount;
    for (int i = 0; i < ChordVoicing::MAX_CHORD_NOTES; i++) {
        if (i < voicing.activeNoteCount) {
            // Calculate note frequency with semitone offset and detune
            float semitoneOffset = voicing.notes[i].semitoneOffset;
            float detuneOffset = voicing.notes[i].detuneAmount / 100.0f; // cents to semitones
            float totalOffset = semitoneOffset + detuneOffset;
            
            float noteFreq = rootFrequency_ * std::pow(2.0f, totalOffset / 12.0f);
            
            oscillators_[i].setFrequency(noteFreq, sampleRate);
            oscillators_[i].setLevel(voicing.notes[i].levelScale);
            oscillators_[i].active = true;
        } else {
            oscillators_[i].active = false;
            oscillators_[i].setLevel(0.0f);
        }
    }
    
    // Update filter sample rate
    filter_.sampleRate = sampleRate;
    filter_.updateCoefficients();
    
    // Update envelope sample rate
    envelope_.sampleRate = sampleRate;
    
    // Trigger envelope
    envelope_.noteOn();
}

void MacroChordEngine::MacroChordVoice::noteOff() {
    envelope_.noteOff();
}

void MacroChordEngine::MacroChordVoice::setAftertouch(float aftertouch) {
    aftertouch_ = aftertouch;
}

AudioFrame MacroChordEngine::MacroChordVoice::processSample() {
    if (!active_) {
        return AudioFrame(0.0f, 0.0f);
    }
    
    age_++;
    
    // Mix all active oscillators
    float mixed = 0.0f;
    float singleNote = 0.0f;
    
    for (int i = 0; i < activeOscCount_; i++) {
        float oscOut = oscillators_[i].processSaw();
        
        if (i == 0) {
            // Root note for single mode
            singleNote = oscOut;
        }
        
        mixed += oscOut;
    }
    
    // Normalize mixed signal
    if (activeOscCount_ > 1) {
        mixed /= std::sqrt(static_cast<float>(activeOscCount_));
    }
    
    // Blend between chord and single note
    float blended = mixed * (1.0f - chordSingleBlend_) + singleNote * chordSingleBlend_;
    
    // Apply low-pass filter
    float filtered = filter_.processLowpass(blended);
    
    // Apply envelope
    float envLevel = envelope_.process();
    
    // Check if voice should be deactivated
    if (!envelope_.isActive()) {
        active_ = false;
    }
    
    // Apply velocity and volume
    float output = filtered * envLevel * velocity_ * volume_;
    
    return AudioFrame(output, output);
}

void MacroChordEngine::MacroChordVoice::setChordParams(const ChordVoicing& voicing) {
    // Update oscillator frequencies and levels if voice is active
    if (active_) {
        activeOscCount_ = voicing.activeNoteCount;
        for (int i = 0; i < ChordVoicing::MAX_CHORD_NOTES; i++) {
            if (i < voicing.activeNoteCount) {
                // Recalculate frequency with new detune
                float semitoneOffset = voicing.notes[i].semitoneOffset;
                float detuneOffset = voicing.notes[i].detuneAmount / 100.0f;
                float totalOffset = semitoneOffset + detuneOffset;
                
                float noteFreq = rootFrequency_ * std::pow(2.0f, totalOffset / 12.0f);
                
                oscillators_[i].setFrequency(noteFreq, filter_.sampleRate);
                oscillators_[i].setLevel(voicing.notes[i].levelScale);
                oscillators_[i].active = true;
            } else {
                oscillators_[i].active = false;
                oscillators_[i].setLevel(0.0f);
            }
        }
    }
}

void MacroChordEngine::MacroChordVoice::setFilterParams(float cutoff, float resonance) {
    filter_.setCutoff(cutoff);
    filter_.setResonance(resonance);
}

void MacroChordEngine::MacroChordVoice::setMorphParams(float chordSingleBlend) {
    chordSingleBlend_ = chordSingleBlend;
}

void MacroChordEngine::MacroChordVoice::setVolume(float volume) {
    volume_ = volume;
}

void MacroChordEngine::MacroChordVoice::setEnvelopeParams(float attack, float decay, float sustain, float release) {
    envelope_.attack = attack;
    envelope_.decay = decay;
    envelope_.sustain = sustain;
    envelope_.release = release;
}

// Envelope implementation
float MacroChordEngine::MacroChordVoice::Envelope::process() {
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