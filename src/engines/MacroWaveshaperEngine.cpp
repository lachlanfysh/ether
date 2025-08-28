#include "MacroWaveshaperEngine.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <cmath>

MacroWaveshaperEngine::MacroWaveshaperEngine() {
    std::cout << "MacroWaveshaper engine created" << std::endl;
    
    // Initialize modulation array
    modulation_.fill(0.0f);
    
    // Calculate initial derived parameters
    calculateDerivedParams();
    
    // Set up default parameters for all voices
    updateAllVoices();
}

MacroWaveshaperEngine::~MacroWaveshaperEngine() {
    allNotesOff();
    std::cout << "MacroWaveshaper engine destroyed" << std::endl;
}

void MacroWaveshaperEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    MacroWaveshaperVoice* voice = findFreeVoice();
    if (!voice) {
        voice = stealVoice();
    }
    
    if (voice) {
        voice->noteOn(note, velocity, aftertouch, sampleRate_);
        voiceCounter_++;
        
        std::cout << "MacroWaveshaper: Note ON " << static_cast<int>(note) 
                  << " vel=" << velocity << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void MacroWaveshaperEngine::noteOff(uint8_t note) {
    MacroWaveshaperVoice* voice = findVoice(note);
    if (voice) {
        voice->noteOff();
        
        std::cout << "MacroWaveshaper: Note OFF " << static_cast<int>(note) 
                  << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void MacroWaveshaperEngine::setAftertouch(uint8_t note, float aftertouch) {
    MacroWaveshaperVoice* voice = findVoice(note);
    if (voice) {
        voice->setAftertouch(aftertouch);
    }
}

void MacroWaveshaperEngine::allNotesOff() {
    for (auto& voice : voices_) {
        voice.noteOff();
    }
}

void MacroWaveshaperEngine::setParameter(ParameterID param, float value) {
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

float MacroWaveshaperEngine::getParameter(ParameterID param) const {
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

bool MacroWaveshaperEngine::hasParameter(ParameterID param) const {
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

void MacroWaveshaperEngine::processAudio(EtherAudioBuffer& outputBuffer) {
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

void MacroWaveshaperEngine::setHarmonics(float harmonics) {
    harmonics_ = std::clamp(harmonics, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroWaveshaperEngine::setTimbre(float timbre) {
    timbre_ = std::clamp(timbre, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroWaveshaperEngine::setMorph(float morph) {
    morph_ = std::clamp(morph, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroWaveshaperEngine::calculateDerivedParams() {
    // HARMONICS: drive 0-1 (expo) + asymmetry 0-0.4
    drive_ = mapDriveExp(harmonics_);
    asymmetry_ = mapAsymmetry(harmonics_);
    
    // TIMBRE: pre-gain −6 → +12 dB + wavebank select + pre-emphasis ±2 dB @ 2 kHz
    preGain_ = mapPreGain(timbre_);
    wavebank_ = mapWavebank(timbre_);
    preEmphasisDb_ = mapPreEmphasis(timbre_);
    
    // MORPH: post-LPF 500 Hz–8 kHz + post sat 0-0.2
    postCutoff_ = mapPostCutoff(morph_);
    postSaturation_ = mapPostSaturation(morph_);
}

float MacroWaveshaperEngine::mapDriveExp(float harmonics) const {
    // Exponential drive mapping for musical response
    return harmonics * harmonics * 1.0f; // 0 to 1
}

float MacroWaveshaperEngine::mapAsymmetry(float harmonics) const {
    // Asymmetry follows harmonics: 0 to 0.4
    return harmonics * 0.4f;
}

float MacroWaveshaperEngine::mapPreGain(float timbre) const {
    // Pre-gain: -6 → +12 dB (linear in dB, exponential in amplitude)
    float gainDb = (timbre - 0.33f) * 18.0f; // -6 to +12 dB range
    gainDb = std::clamp(gainDb, -6.0f, 12.0f);
    return std::pow(10.0f, gainDb / 20.0f); // Convert to linear gain
}

int MacroWaveshaperEngine::mapWavebank(float timbre) const {
    // Wavebank selection: 4 banks (0-3)
    return static_cast<int>(timbre * 3.99f);
}

float MacroWaveshaperEngine::mapPreEmphasis(float timbre) const {
    // Pre-emphasis: ±2 dB @ 2 kHz
    // Map around center for neutral at 0.5
    return (timbre - 0.5f) * 4.0f; // -2 to +2 dB
}

float MacroWaveshaperEngine::mapPostCutoff(float morph) const {
    // Post-LPF: 500 Hz to 8 kHz (exponential)
    // f = 500 * 2^(morph * α), where α = log2(8000/500) ≈ 4
    return 500.0f * std::pow(2.0f, morph * 4.0f);
}

float MacroWaveshaperEngine::mapPostSaturation(float morph) const {
    // Post saturation: 0 to 0.2
    return morph * 0.2f;
}

// Voice management methods
MacroWaveshaperEngine::MacroWaveshaperVoice* MacroWaveshaperEngine::findFreeVoice() {
    for (auto& voice : voices_) {
        if (!voice.isActive()) {
            return &voice;
        }
    }
    return nullptr;
}

MacroWaveshaperEngine::MacroWaveshaperVoice* MacroWaveshaperEngine::findVoice(uint8_t note) {
    for (auto& voice : voices_) {
        if (voice.isActive() && voice.getNote() == note) {
            return &voice;
        }
    }
    return nullptr;
}

MacroWaveshaperEngine::MacroWaveshaperVoice* MacroWaveshaperEngine::stealVoice() {
    MacroWaveshaperVoice* oldest = nullptr;
    uint32_t oldestAge = 0;
    
    for (auto& voice : voices_) {
        if (voice.getAge() > oldestAge) {
            oldestAge = voice.getAge();
            oldest = &voice;
        }
    }
    
    return oldest;
}

void MacroWaveshaperEngine::updateAllVoices() {
    for (auto& voice : voices_) {
        voice.setWaveshapeParams(drive_, asymmetry_);
        voice.setPreParams(preGain_, wavebank_, preEmphasisDb_);
        voice.setPostParams(postCutoff_, postSaturation_);
        voice.setVolume(volume_);
        voice.setEnvelopeParams(attack_, decay_, sustain_, release_);
    }
}

// Standard SynthEngine methods
size_t MacroWaveshaperEngine::getActiveVoiceCount() const {
    size_t count = 0;
    for (const auto& voice : voices_) {
        if (voice.isActive()) {
            count++;
        }
    }
    return count;
}

void MacroWaveshaperEngine::setVoiceCount(size_t maxVoices) {
    // Voice count is fixed for this implementation
}

void MacroWaveshaperEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
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

bool MacroWaveshaperEngine::loadPreset(const uint8_t* data, size_t size) {
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

void MacroWaveshaperEngine::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updateAllVoices();
}

void MacroWaveshaperEngine::setBufferSize(size_t bufferSize) {
    bufferSize_ = bufferSize;
}

bool MacroWaveshaperEngine::supportsModulation(ParameterID target) const {
    return hasParameter(target);
}

void MacroWaveshaperEngine::setModulation(ParameterID target, float amount) {
    size_t index = static_cast<size_t>(target);
    if (index < modulation_.size()) {
        modulation_[index] = amount;
    }
}

void MacroWaveshaperEngine::updateCPUUsage(float processingTime) {
    float maxTime = (BUFFER_SIZE / sampleRate_) * 1000.0f;
    cpuUsage_ = std::min(100.0f, (processingTime / maxTime) * 100.0f);
}

// MacroWaveshaperVoice implementation
MacroWaveshaperEngine::MacroWaveshaperVoice::MacroWaveshaperVoice() {
    preEmphasis_.sampleRate = 48000.0f;
    preEmphasis_.updateCoefficients();
    postFilter_.sampleRate = 48000.0f;
    postFilter_.updateCoefficients();
    envelope_.sampleRate = 48000.0f;
}

void MacroWaveshaperEngine::MacroWaveshaperVoice::noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate) {
    note_ = note;
    velocity_ = velocity;
    aftertouch_ = aftertouch;
    active_ = true;
    age_ = 0;
    
    // Calculate note frequency
    noteFrequency_ = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    
    // Set oscillator frequency
    osc_.setFrequency(noteFrequency_, sampleRate);
    
    // Update filter sample rates
    preEmphasis_.sampleRate = sampleRate;
    preEmphasis_.updateCoefficients();
    postFilter_.sampleRate = sampleRate;
    postFilter_.updateCoefficients();
    
    // Update envelope sample rate
    envelope_.sampleRate = sampleRate;
    
    // Trigger envelope
    envelope_.noteOn();
}

void MacroWaveshaperEngine::MacroWaveshaperVoice::noteOff() {
    envelope_.noteOff();
}

void MacroWaveshaperEngine::MacroWaveshaperVoice::setAftertouch(float aftertouch) {
    aftertouch_ = aftertouch;
}

AudioFrame MacroWaveshaperEngine::MacroWaveshaperVoice::processSample() {
    if (!active_) {
        return AudioFrame(0.0f, 0.0f);
    }
    
    age_++;
    
    // Generate oscillator output
    float oscOut = osc_.processSaw();
    
    // Apply pre-gain
    float preGained = oscOut * preGain_;
    
    // Apply pre-emphasis EQ
    float preEmphasized = preEmphasis_.process(preGained);
    
    // Apply waveshaping with oversampling
    float shaped = oversampler_.process(preEmphasized, waveshaper_);
    
    // Apply post low-pass filter
    float filtered = postFilter_.process(shaped);
    
    // Apply post saturation
    float saturated = postSat_.process(filtered);
    
    // Apply envelope
    float envLevel = envelope_.process();
    
    // Check if voice should be deactivated
    if (!envelope_.isActive()) {
        active_ = false;
    }
    
    // Apply velocity and volume
    float output = saturated * envLevel * velocity_ * volume_;
    
    return AudioFrame(output, output);
}

void MacroWaveshaperEngine::MacroWaveshaperVoice::setWaveshapeParams(float drive, float asymmetry) {
    drive_ = drive;
    asymmetry_ = asymmetry;
    waveshaper_.setParams(drive, asymmetry, wavebank_);
}

void MacroWaveshaperEngine::MacroWaveshaperVoice::setPreParams(float preGain, int wavebank, float preEmphasis) {
    preGain_ = preGain;
    wavebank_ = wavebank;
    preEmphasisDb_ = preEmphasis;
    
    preEmphasis_.setPreEmphasis(preEmphasis);
    waveshaper_.setParams(drive_, asymmetry_, wavebank);
}

void MacroWaveshaperEngine::MacroWaveshaperVoice::setPostParams(float postCutoff, float postSat) {
    postCutoff_ = postCutoff;
    postSaturation_ = postSat;
    
    postFilter_.setCutoff(postCutoff);
    postSat_.setAmount(postSat);
}

void MacroWaveshaperEngine::MacroWaveshaperVoice::setVolume(float volume) {
    volume_ = volume;
}

void MacroWaveshaperEngine::MacroWaveshaperVoice::setEnvelopeParams(float attack, float decay, float sustain, float release) {
    envelope_.attack = attack;
    envelope_.decay = decay;
    envelope_.sustain = sustain;
    envelope_.release = release;
}

// Filter and effect implementations
float MacroWaveshaperEngine::MacroWaveshaperVoice::Envelope::process() {
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

void MacroWaveshaperEngine::MacroWaveshaperVoice::PreEmphasisFilter::setPreEmphasis(float emphasisDb) {
    gain = emphasisDb;
    updateCoefficients();
}

void MacroWaveshaperEngine::MacroWaveshaperVoice::PreEmphasisFilter::updateCoefficients() {
    // Simple high-shelf filter at 2kHz
    float omega = 2.0f * M_PI * freq / sampleRate;
    float gainLinear = std::pow(10.0f, gain / 20.0f);
    
    float alpha = std::exp(-omega);
    a0 = gainLinear * (1.0f - alpha);
    a1 = 0.0f;
    b1 = -alpha;
}

float MacroWaveshaperEngine::MacroWaveshaperVoice::PreEmphasisFilter::process(float input) {
    float output = a0 * input + a1 * x1 - b1 * y1;
    
    x1 = input;
    y1 = output;
    
    return output;
}

void MacroWaveshaperEngine::MacroWaveshaperVoice::PostLPFilter::setCutoff(float freq) {
    cutoff = std::clamp(freq, 20.0f, sampleRate * 0.45f);
    updateCoefficients();
}

void MacroWaveshaperEngine::MacroWaveshaperVoice::PostLPFilter::updateCoefficients() {
    float omega = 2.0f * M_PI * cutoff / sampleRate;
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    float Q = 0.707f; // Butterworth response
    float alpha = sinOmega / (2.0f * Q);
    
    float b0 = 1.0f + alpha;
    a0 = (1.0f - cosOmega) / 2.0f / b0;
    a1 = (1.0f - cosOmega) / b0;
    a2 = (1.0f - cosOmega) / 2.0f / b0;
    b1 = -2.0f * cosOmega / b0;
    b2 = (1.0f - alpha) / b0;
}

float MacroWaveshaperEngine::MacroWaveshaperVoice::PostLPFilter::process(float input) {
    float output = a0 * input + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
    
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    
    return output;
}