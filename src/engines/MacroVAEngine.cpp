#include "MacroVAEngine.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <cmath>

MacroVAEngine::MacroVAEngine() 
    : parameterManager_(std::make_unique<EtherSynth::ParameterManager>()),
      cpuTracker_(std::make_unique<EtherSynth::CPUUsageTracker>()) {
    
    std::cout << "MacroVA engine created" << std::endl;
    
    // Calculate initial derived parameters
    calculateDerivedParams();
    
    // Set up default parameters for all voices
    updateAllVoices();
}

MacroVAEngine::~MacroVAEngine() {
    allNotesOff();
    std::cout << "MacroVA engine destroyed" << std::endl;
}

void MacroVAEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    MacroVAVoice* voice = findFreeVoice();
    if (!voice) {
        voice = stealVoice();
    }
    
    if (voice) {
        voice->noteOn(note, velocity, aftertouch, sampleRate_);
        voiceCounter_++;
        
        std::cout << "MacroVA: Note ON " << static_cast<int>(note) 
                  << " vel=" << velocity << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void MacroVAEngine::noteOff(uint8_t note) {
    MacroVAVoice* voice = findVoice(note);
    if (voice) {
        voice->noteOff();
        
        std::cout << "MacroVA: Note OFF " << static_cast<int>(note) 
                  << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void MacroVAEngine::setAftertouch(uint8_t note, float aftertouch) {
    MacroVAVoice* voice = findVoice(note);
    if (voice) {
        voice->setAftertouch(aftertouch);
    }
}

void MacroVAEngine::allNotesOff() {
    for (auto& voice : voices_) {
        voice.noteOff();
    }
}

void MacroVAEngine::setParameter(ParameterID param, float value) {
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
            attack_ = std::clamp(value, 0.0005f, 5.0f); // Min 0.5ms per spec
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

float MacroVAEngine::getParameter(ParameterID param) const {
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

bool MacroVAEngine::hasParameter(ParameterID param) const {
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

void MacroVAEngine::processAudio(EtherAudioBuffer& outputBuffer) {
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
    if (cpuTracker_) {
        cpuTracker_->updateCPUUsage(processingTime);
    }
}

void MacroVAEngine::setHarmonics(float harmonics) {
    harmonics_ = std::clamp(harmonics, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroVAEngine::setTimbre(float timbre) {
    timbre_ = std::clamp(timbre, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroVAEngine::setMorph(float morph) {
    morph_ = std::clamp(morph, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroVAEngine::calculateDerivedParams() {
    // HARMONICS: LPF cutoff (20 Hz → 12 kHz) + small auto-Q
    filterCutoff_ = mapCutoffExp(harmonics_);
    filterAutoQ_ = mapAutoQ(harmonics_);
    
    // TIMBRE: saw↔pulse blend + PWM around 50% (safe range 45-55%)
    sawPulseBlend_ = timbre_;
    pwm_ = mapPWM(timbre_);
    
    // MORPH: sub/noise blend + gentle high-tilt
    subLevel_ = mapSubLevel(morph_);
    noiseLevel_ = mapNoiseLevel(morph_);
    highTilt_ = mapHighTilt(morph_);
}

float MacroVAEngine::mapCutoffExp(float harmonics) const {
    // Exponential mapping: 20 Hz to 12 kHz
    // f = 20 * 2^(x * α), pick α to hit 12 kHz at 100%
    // 12000 = 20 * 2^α, so α = log2(12000/20) ≈ 9.23
    return 20.0f * std::pow(2.0f, harmonics * 9.23f);
}

float MacroVAEngine::mapAutoQ(float harmonics) const {
    // Small auto-Q ride: +0.00 → +0.08 as cutoff opens
    return harmonics * 0.08f;
}

float MacroVAEngine::mapPWM(float timbre) const {
    // PWM around 50% with safe range 45-55%
    if (timbre < 0.5f) {
        return 0.5f; // Saw mode, keep PWM at 50%
    } else {
        // Pulse mode: map 0.5-1.0 to 45-55% PWM
        float pulseAmount = (timbre - 0.5f) * 2.0f; // 0-1
        return 0.45f + pulseAmount * 0.1f; // 45-55%
    }
}

float MacroVAEngine::mapSubLevel(float morph) const {
    // Sub: -12 → 0 dB (linear in dB)
    float subDb = morph * 12.0f - 12.0f; // -12 to 0 dB
    return std::pow(10.0f, subDb / 20.0f); // Convert to linear gain
}

float MacroVAEngine::mapNoiseLevel(float morph) const {
    // Noise: -∞ → -18 dB
    if (morph < 0.01f) return 0.0f; // -∞
    float noiseDb = morph * 18.0f - 36.0f; // Map to -36 to -18 dB range
    noiseDb = std::max(noiseDb, -36.0f); // Clamp to prevent extreme values
    return std::pow(10.0f, noiseDb / 20.0f);
}

float MacroVAEngine::mapHighTilt(float morph) const {
    // Gentle high-tilt: ±2 dB @ 4 kHz
    // Map 0-1 to -2 to +2 dB
    return (morph - 0.5f) * 4.0f;
}

// Voice management methods
MacroVAEngine::MacroVAVoice* MacroVAEngine::findFreeVoice() {
    for (auto& voice : voices_) {
        if (!voice.isActive()) {
            return &voice;
        }
    }
    return nullptr;
}

MacroVAEngine::MacroVAVoice* MacroVAEngine::findVoice(uint8_t note) {
    for (auto& voice : voices_) {
        if (voice.isActive() && voice.getNote() == note) {
            return &voice;
        }
    }
    return nullptr;
}

MacroVAEngine::MacroVAVoice* MacroVAEngine::stealVoice() {
    // Find oldest voice
    MacroVAVoice* oldest = nullptr;
    uint32_t oldestAge = 0;
    
    for (auto& voice : voices_) {
        if (voice.getAge() > oldestAge) {
            oldestAge = voice.getAge();
            oldest = &voice;
        }
    }
    
    return oldest;
}

void MacroVAEngine::updateAllVoices() {
    for (auto& voice : voices_) {
        voice.setFilterParams(filterCutoff_, filterAutoQ_);
        voice.setOscParams(sawPulseBlend_, pwm_);
        voice.setSubNoiseParams(subLevel_, noiseLevel_);
        voice.setHighTilt(highTilt_);
        voice.setVolume(volume_);
        voice.setEnvelopeParams(attack_, decay_, sustain_, release_);
    }
}

// Standard SynthEngine methods
size_t MacroVAEngine::getActiveVoiceCount() const {
    size_t count = 0;
    for (const auto& voice : voices_) {
        if (voice.isActive()) {
            count++;
        }
    }
    return count;
}

void MacroVAEngine::setVoiceCount(size_t maxVoices) {
    // Voice count is fixed for this implementation
}

void MacroVAEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
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

bool MacroVAEngine::loadPreset(const uint8_t* data, size_t size) {
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

void MacroVAEngine::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updateAllVoices();
}

void MacroVAEngine::setBufferSize(size_t bufferSize) {
    bufferSize_ = bufferSize;
}

bool MacroVAEngine::supportsModulation(ParameterID target) const {
    return hasParameter(target);
}

void MacroVAEngine::setModulation(ParameterID target, float amount) {
    size_t index = static_cast<size_t>(target);
    if (parameterManager_) {
        parameterManager_->setModulation(target, amount);
    }
}

// updateCPUUsage method removed - now using shared CPUUsageTracker component

// MacroVAVoice implementation
MacroVAEngine::MacroVAVoice::MacroVAVoice() {
    filter_.sampleRate = 48000.0f;
    filter_.updateCoefficients();
    tiltFilter_.sampleRate = 48000.0f;
    tiltFilter_.updateCoefficients();
    envelope_ = std::make_unique<EtherSynth::StandardADSR>();
    if (envelope_) {
        envelope_->setSampleRate(48000.0f);
    }
}

void MacroVAEngine::MacroVAVoice::noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate) {
    // Use shared voice state instead of individual members
    voiceState_.noteOn(note, velocity, 0, 0); // currentTime and voiceId would be passed from engine
    voiceState_.aftertouch_ = aftertouch;
    
    // Set oscillator frequencies
    mainOsc_.setFrequency(voiceState_.noteFrequency_, sampleRate);
    subOsc_.setFrequency(voiceState_.noteFrequency_, sampleRate);
    
    // Update filter sample rates
    filter_.sampleRate = sampleRate;
    filter_.updateCoefficients();
    tiltFilter_.sampleRate = sampleRate;
    tiltFilter_.updateCoefficients();
    
    // Update envelope sample rate and trigger
    if (envelope_) {
        envelope_->setSampleRate(sampleRate);
        envelope_->noteOn();
    }
}

void MacroVAEngine::MacroVAVoice::noteOff() {
    voiceState_.noteOff();
    if (envelope_) {
        envelope_->noteOff();
    }
}

void MacroVAEngine::MacroVAVoice::setAftertouch(float aftertouch) {
    voiceState_.aftertouch_ = aftertouch;
}

AudioFrame MacroVAEngine::MacroVAVoice::processSample() {
    if (!voiceState_.isActive()) {
        return AudioFrame(0.0f, 0.0f);
    }
    
    age_++;
    
    // Generate main oscillator output
    float oscOut;
    if (sawPulseBlend_ < 0.5f) {
        // Saw mode: blend between saw and less saw
        float sawAmount = 1.0f - (sawPulseBlend_ * 2.0f);
        oscOut = mainOsc_.processSaw() * sawAmount;
    } else {
        // Pulse mode
        mainOsc_.setPWM(pwm_);
        oscOut = mainOsc_.processPulse();
    }
    
    // Add sub oscillator
    float subOut = subOsc_.processSine() * subLevel_;
    
    // Add noise
    float noiseOut = noise_.processWhite() * noiseLevel_;
    
    // Mix all sources
    float mixed = oscOut + subOut + noiseOut;
    
    // Apply main filter
    float filtered = filter_.process(mixed);
    
    // Apply high-frequency tilt
    float tilted = tiltFilter_.process(filtered);
    
    // Apply envelope
    float envLevel = envelope_.process();
    
    // Check if voice should be deactivated
    if (!envelope_.isActive()) {
        active_ = false;
    }
    
    // Apply velocity and volume
    float output = tilted * envLevel * velocity_ * volume_;
    
    return AudioFrame(output, output);
}

void MacroVAEngine::MacroVAVoice::setFilterParams(float cutoff, float autoQ) {
    filter_.setCutoff(cutoff);
    filter_.setAutoQ(autoQ);
}

void MacroVAEngine::MacroVAVoice::setOscParams(float sawPulseBlend, float pwm) {
    sawPulseBlend_ = sawPulseBlend;
    pwm_ = pwm;
}

void MacroVAEngine::MacroVAVoice::setSubNoiseParams(float subLevel, float noiseLevel) {
    subLevel_ = subLevel;
    noiseLevel_ = noiseLevel;
}

void MacroVAEngine::MacroVAVoice::setHighTilt(float tiltAmount) {
    highTilt_ = tiltAmount;
    tiltFilter_.setTilt(tiltAmount);
}

void MacroVAEngine::MacroVAVoice::setVolume(float volume) {
    volume_ = volume;
}

void MacroVAEngine::MacroVAVoice::setEnvelopeParams(float attack, float decay, float sustain, float release) {
    envelope_.attack = attack;
    envelope_.decay = decay;
    envelope_.sustain = sustain;
    envelope_.release = release;
}

// Filter implementations
float MacroVAEngine::MacroVAVoice::Envelope::process() {
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

void MacroVAEngine::MacroVAVoice::VAFilter::updateCoefficients() {
    float omega = 2.0f * M_PI * cutoff / sampleRate;
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    float totalQ = baseResonance + autoQ;
    float alpha = sinOmega / (2.0f * totalQ);
    
    float b0 = 1.0f + alpha;
    a0 = (1.0f - cosOmega) / 2.0f / b0;
    a1 = (1.0f - cosOmega) / b0;
    a2 = (1.0f - cosOmega) / 2.0f / b0;
    b1 = -2.0f * cosOmega / b0;
    b2 = (1.0f - alpha) / b0;
}

void MacroVAEngine::MacroVAVoice::VAFilter::setCutoff(float freq) {
    cutoff = std::clamp(freq, 20.0f, sampleRate * 0.45f);
    updateCoefficients();
}

void MacroVAEngine::MacroVAVoice::VAFilter::setAutoQ(float q) {
    autoQ = q;
    updateCoefficients();
}

float MacroVAEngine::MacroVAVoice::VAFilter::process(float input) {
    float output = a0 * input + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
    
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    
    return output;
}

void MacroVAEngine::MacroVAVoice::TiltFilter::setTilt(float tiltDb) {
    gain = tiltDb;
    updateCoefficients();
}

void MacroVAEngine::MacroVAVoice::TiltFilter::updateCoefficients() {
    // Simple high-shelf filter at 4kHz
    float omega = 2.0f * M_PI * freq / sampleRate;
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    float A = std::pow(10.0f, gain / 40.0f); // Convert dB to amplitude
    float beta = std::sqrt(A) / 1.0f; // Q = 1.0
    
    float b0 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega + beta * sinOmega);
    float b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosOmega);
    float b2 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega - beta * sinOmega);
    float a0_denom = (A + 1.0f) - (A - 1.0f) * cosOmega + beta * sinOmega;
    float a1_val = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosOmega);
    float a2_val = (A + 1.0f) - (A - 1.0f) * cosOmega - beta * sinOmega;
    
    a0 = b0 / a0_denom;
    a1 = b1 / a0_denom;
    this->b1 = a1_val / a0_denom;
}


float MacroVAEngine::MacroVAVoice::TiltFilter::process(float input) {
    float output = a0 * input + a1 * x1 - b1 * y1;
    
    x1 = input;
    y1 = output;
    
    return output;
}