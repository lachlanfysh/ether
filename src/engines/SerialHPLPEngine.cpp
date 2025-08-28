#include "SerialHPLPEngine.h"
#include <cmath>
#include <algorithm>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef STM32H7
#include "stm32h7xx_hal.h"
#else
#include <chrono>
#endif

SerialHPLPEngine::SerialHPLPEngine() {
    // Initialize default configurations
    filterConfig_.mode = FilterMode::SERIAL_HP_LP;
    filterConfig_.hpCutoffHz = 80.0f;
    filterConfig_.lpCutoffHz = 2000.0f;
    filterConfig_.hpResonance = 0.2f;
    filterConfig_.lpResonance = 0.4f;
    filterConfig_.hpDrive = 0.3f;
    filterConfig_.filterBalance = 0.5f;
    filterConfig_.keyTracking = 0.7f;
    filterConfig_.vintageModeling = true;
    filterConfig_.saturation = 0.2f;
    
    oscConfig_.waveform1 = OscillatorWaveform::SAWTOOTH;
    oscConfig_.waveform2 = OscillatorWaveform::SAWTOOTH;
    oscConfig_.detune = 0.05f;
    oscConfig_.level1 = 0.7f;
    oscConfig_.level2 = 0.7f;
    oscConfig_.pulseWidth1 = 0.5f;
    oscConfig_.pulseWidth2 = 0.5f;
    oscConfig_.unisonDetune = 0.1f;
    oscConfig_.unisonVoices = 3;
    oscConfig_.noiseLevel = 0.0f;
    oscConfig_.hardSync = false;
    
    ringModConfig_.mode = RingModMode::OFF;
    ringModConfig_.amount = 0.0f;
    ringModConfig_.frequency = 1.0f;
    ringModConfig_.feedback = 0.0f;
    ringModConfig_.enableSidechain = false;
    ringModConfig_.sidechainLevel = 0.0f;
    ringModConfig_.dryWetBalance = 0.5f;
    
    stereoConfig_.width = 0.5f;
    stereoConfig_.chorusEnable = false;
    stereoConfig_.chorusRate = 2.0f;
    stereoConfig_.chorusDepth = 0.3f;
    stereoConfig_.chorusDelay = 5.0f;
    stereoConfig_.chorusFeedback = 0.1f;
    stereoConfig_.monoLowEnd = true;
    stereoConfig_.monoFreq = 120.0f;
    
    // Initialize state
    voiceState_.active = false;
    voiceState_.note = 60.0f;
    voiceState_.velocity = 100.0f;
    voiceState_.pitchBend = 0.0f;
    voiceState_.phase1 = 0.0f;
    voiceState_.phase2 = 0.0f;
    voiceState_.unisonPhases.fill(0.0f);
    voiceState_.hpCutoff = filterConfig_.hpCutoffHz;
    voiceState_.lpCutoff = filterConfig_.lpCutoffHz;
    voiceState_.ringModPhase = 0.0f;
    voiceState_.ringModAmount = 0.0f;
    voiceState_.noteOnTime = 0;
    voiceState_.ampEnvActive = false;
    voiceState_.filterEnvActive = false;
    
    sampleRate_ = 44100.0f;
    initialized_ = false;
    
    // Initialize parameter values
    harmonics_ = 0.5f;
    timbre_ = 0.5f;
    morph_ = 0.5f;
    
    currentHPCutoff_ = filterConfig_.hpCutoffHz;
    currentLPCutoff_ = filterConfig_.lpCutoffHz;
    currentHPDrive_ = filterConfig_.hpDrive;
    currentDetune_ = oscConfig_.detune;
    currentRingMod_ = 0.0f;
    currentWidth_ = stereoConfig_.width;
    
    lastHPOutput_ = 0.0f;
    lastLPOutput_ = 0.0f;
    
    // Initialize delay lines
    chorusDelayLineL_.fill(0.0f);
    chorusDelayLineR_.fill(0.0f);
    chorusDelayIndexL_ = 0;
    chorusDelayIndexR_ = 0;
    chorusLFOPhase_ = 0.0f;
    
    monoLowSignal_ = 0.0f;
    cpuUsage_ = 0.0f;
}

SerialHPLPEngine::~SerialHPLPEngine() {
    shutdown();
}

bool SerialHPLPEngine::initialize(float sampleRate) {
    if (initialized_) {
        return true;
    }
    
    sampleRate_ = sampleRate;
    
    // Initialize oscillators
    if (!osc1_.initialize(sampleRate)) return false;
    if (!osc2_.initialize(sampleRate)) return false;
    if (!noiseOsc_.initialize(sampleRate)) return false;
    
    // Initialize unison oscillators
    for (auto& unisonOsc : unisonOscs_) {
        if (!unisonOsc.initialize(sampleRate)) return false;
    }
    
    // Initialize filters
    if (!hpFilter_.initialize(sampleRate)) return false;
    if (!lpFilter_.initialize(sampleRate)) return false;
    if (!monoHPFilter_.initialize(sampleRate)) return false;
    
    // Initialize ring modulator
    if (!ringMod_.initialize(sampleRate)) return false;
    
    // Initialize envelopes
    if (!ampEnvelope_.initialize(sampleRate)) return false;
    if (!filterEnvelope_.initialize(sampleRate)) return false;
    
    // Initialize parameter smoothers
    hpCutoffSmoother_.initialize(sampleRate, 0.02f); // 20ms smoothing
    lpCutoffSmoother_.initialize(sampleRate, 0.02f);
    hpDriveSmoother_.initialize(sampleRate, 0.01f);
    detuneSmoother_.initialize(sampleRate, 0.05f); // Slower for pitch changes
    ringModSmoother_.initialize(sampleRate, 0.01f);
    widthSmoother_.initialize(sampleRate, 0.03f);
    
    // Configure envelopes
    ampEnvelope_.setADSR(0.001f, 0.1f, 0.8f, 0.05f);
    filterEnvelope_.setADSR(0.001f, 0.2f, 0.4f, 0.1f);
    
    // Configure filters
    hpFilter_.setMode(StateVariableFilter::HIGHPASS);
    lpFilter_.setMode(StateVariableFilter::LOWPASS);
    monoHPFilter_.setMode(StateVariableFilter::HIGHPASS);
    monoHPFilter_.setCutoff(stereoConfig_.monoFreq);
    monoHPFilter_.setResonance(0.1f);
    
    // Configure oscillators
    osc1_.setWaveform(VirtualAnalogOscillator::SAWTOOTH);
    osc2_.setWaveform(VirtualAnalogOscillator::SAWTOOTH);
    noiseOsc_.setWaveform(VirtualAnalogOscillator::NOISE);
    
    // Configure unison oscillators
    for (auto& unisonOsc : unisonOscs_) {
        unisonOsc.setWaveform(VirtualAnalogOscillator::SAWTOOTH);
    }
    
    initialized_ = true;
    return true;
}

void SerialHPLPEngine::shutdown() {
    if (!initialized_) {
        return;
    }
    
    allNotesOff();
    
    osc1_.shutdown();
    osc2_.shutdown();
    noiseOsc_.shutdown();
    
    for (auto& unisonOsc : unisonOscs_) {
        unisonOsc.shutdown();
    }
    
    hpFilter_.shutdown();
    lpFilter_.shutdown();
    monoHPFilter_.shutdown();
    ringMod_.shutdown();
    ampEnvelope_.shutdown();
    filterEnvelope_.shutdown();
    
    initialized_ = false;
}

void SerialHPLPEngine::setFilterConfig(const FilterConfig& config) {
    filterConfig_ = config;
    
    // Clamp values to valid ranges
    filterConfig_.hpCutoffHz = clamp(filterConfig_.hpCutoffHz, MIN_HP_CUTOFF_HZ, MAX_HP_CUTOFF_HZ);
    filterConfig_.lpCutoffHz = clamp(filterConfig_.lpCutoffHz, MIN_LP_CUTOFF_HZ, MAX_LP_CUTOFF_HZ);
    filterConfig_.hpResonance = clamp(filterConfig_.hpResonance, 0.0f, 1.0f);
    filterConfig_.lpResonance = clamp(filterConfig_.lpResonance, 0.0f, 1.0f);
    filterConfig_.hpDrive = clamp(filterConfig_.hpDrive, 0.0f, 1.0f);
    filterConfig_.filterBalance = clamp(filterConfig_.filterBalance, 0.0f, 1.0f);
    filterConfig_.keyTracking = clamp(filterConfig_.keyTracking, 0.0f, 2.0f);
    filterConfig_.saturation = clamp(filterConfig_.saturation, 0.0f, 1.0f);
    
    // Update filter settings
    if (initialized_) {
        hpFilter_.setCutoff(filterConfig_.hpCutoffHz);
        hpFilter_.setResonance(filterConfig_.hpResonance);
        lpFilter_.setCutoff(filterConfig_.lpCutoffHz);
        lpFilter_.setResonance(filterConfig_.lpResonance);
    }
}

void SerialHPLPEngine::setOscillatorConfig(const OscillatorConfig& config) {
    oscConfig_ = config;
    
    // Clamp values to valid ranges
    oscConfig_.detune = clamp(oscConfig_.detune, -MAX_DETUNE_SEMITONES, MAX_DETUNE_SEMITONES);
    oscConfig_.level1 = clamp(oscConfig_.level1, 0.0f, 1.0f);
    oscConfig_.level2 = clamp(oscConfig_.level2, 0.0f, 1.0f);
    oscConfig_.pulseWidth1 = clamp(oscConfig_.pulseWidth1, 0.05f, 0.95f);
    oscConfig_.pulseWidth2 = clamp(oscConfig_.pulseWidth2, 0.05f, 0.95f);
    oscConfig_.unisonDetune = clamp(oscConfig_.unisonDetune, 0.0f, 1.0f);
    oscConfig_.unisonVoices = clamp(oscConfig_.unisonVoices, 1, MAX_UNISON_VOICES);
    oscConfig_.noiseLevel = clamp(oscConfig_.noiseLevel, 0.0f, 1.0f);
    
    updateOscillatorParameters();
}

void SerialHPLPEngine::setRingModConfig(const RingModConfig& config) {
    ringModConfig_ = config;
    
    // Clamp values to valid ranges
    ringModConfig_.amount = clamp(ringModConfig_.amount, 0.0f, 1.0f);
    ringModConfig_.frequency = clamp(ringModConfig_.frequency, 0.1f, 20.0f);
    ringModConfig_.feedback = clamp(ringModConfig_.feedback, 0.0f, 0.9f);
    ringModConfig_.sidechainLevel = clamp(ringModConfig_.sidechainLevel, 0.0f, 1.0f);
    ringModConfig_.dryWetBalance = clamp(ringModConfig_.dryWetBalance, 0.0f, 1.0f);
}

void SerialHPLPEngine::setStereoConfig(const StereoConfig& config) {
    stereoConfig_ = config;
    
    // Clamp values to valid ranges
    stereoConfig_.width = clamp(stereoConfig_.width, 0.0f, 2.0f);
    stereoConfig_.chorusRate = clamp(stereoConfig_.chorusRate, 0.1f, 10.0f);
    stereoConfig_.chorusDepth = clamp(stereoConfig_.chorusDepth, 0.0f, 1.0f);
    stereoConfig_.chorusDelay = clamp(stereoConfig_.chorusDelay, 1.0f, CHORUS_MAX_DELAY_MS);
    stereoConfig_.chorusFeedback = clamp(stereoConfig_.chorusFeedback, 0.0f, 0.8f);
    stereoConfig_.monoFreq = clamp(stereoConfig_.monoFreq, 50.0f, 300.0f);
    
    // Update mono filter
    if (initialized_) {
        monoHPFilter_.setCutoff(stereoConfig_.monoFreq);
    }
}

void SerialHPLPEngine::setHarmonics(float harmonics) {
    harmonics_ = clamp(harmonics, 0.0f, 1.0f);
    
    // Map harmonics to HP cutoff with exponential curve
    float hpMultiplier = mapHarmonicsToHP(harmonics_);
    float targetHPCutoff = filterConfig_.hpCutoffHz * hpMultiplier;
    
    // Apply key tracking
    if (voiceState_.active) {
        targetHPCutoff = calculateKeyTrackedCutoff(voiceState_.note, targetHPCutoff);
    }
    
    hpCutoffSmoother_.setTarget(clamp(targetHPCutoff, MIN_HP_CUTOFF_HZ, MAX_HP_CUTOFF_HZ));
    
    // Map to HP drive for bite
    float targetDrive = filterConfig_.hpDrive * (0.5f + harmonics_ * 1.5f);
    hpDriveSmoother_.setTarget(targetDrive);
    
    // Map to ring modulation amount
    float targetRingMod = harmonics_ * ringModConfig_.amount;
    ringModSmoother_.setTarget(targetRingMod);
}

void SerialHPLPEngine::setTimbre(float timbre) {
    timbre_ = clamp(timbre, 0.0f, 1.0f);
    
    // Map timbre to oscillator detune
    float targetDetune = mapTimbreToDetune(timbre_);
    detuneSmoother_.setTarget(targetDetune);
    
    // Map to ring modulation frequency
    float ringModFreq = 0.5f + timbre_ * 4.0f; // 0.5x to 4.5x frequency
    ringModConfig_.frequency = ringModFreq;
    
    // Map to LP filter characteristics
    float lpCharacter = timbre_;
    float targetLPCutoff = filterConfig_.lpCutoffHz * (0.3f + lpCharacter * 1.4f);
    
    if (voiceState_.active) {
        targetLPCutoff = calculateKeyTrackedCutoff(voiceState_.note, targetLPCutoff);
    }
    
    lpCutoffSmoother_.setTarget(clamp(targetLPCutoff, MIN_LP_CUTOFF_HZ, MAX_LP_CUTOFF_HZ));
}

void SerialHPLPEngine::setMorph(float morph) {
    morph_ = clamp(morph, 0.0f, 1.0f);
    
    // Map morph to HP→LP balance
    float balance = mapMorphToBalance(morph_);
    filterConfig_.filterBalance = balance;
    
    // Map to filter envelope depth
    float envelopeDepth = morph_ * 2.0f;
    filterEnvelope_.setDepth(envelopeDepth);
    
    // Map to stereo width
    float targetWidth = stereoConfig_.width * (0.2f + morph_ * 1.6f);
    widthSmoother_.setTarget(clamp(targetWidth, 0.0f, 2.0f));
}

void SerialHPLPEngine::setHTMParameters(float harmonics, float timbre, float morph) {
    setHarmonics(harmonics);
    setTimbre(timbre);
    setMorph(morph);
}

void SerialHPLPEngine::getHTMParameters(float& harmonics, float& timbre, float& morph) const {
    harmonics = harmonics_;
    timbre = timbre_;
    morph = morph_;
}

void SerialHPLPEngine::noteOn(float note, float velocity) {
    uint32_t currentTime = getTimeMs();
    
    // Update voice state
    voiceState_.note = note;
    voiceState_.velocity = velocity;
    voiceState_.noteOnTime = currentTime;
    voiceState_.active = true;
    voiceState_.ampEnvActive = true;
    voiceState_.filterEnvActive = true;
    
    // Update oscillator frequencies
    updateOscillatorParameters();
    
    // Update filter cutoffs with key tracking
    float hpCutoff = calculateKeyTrackedCutoff(note, filterConfig_.hpCutoffHz);
    float lpCutoff = calculateKeyTrackedCutoff(note, filterConfig_.lpCutoffHz);
    
    voiceState_.hpCutoff = hpCutoff;
    voiceState_.lpCutoff = lpCutoff;
    
    hpCutoffSmoother_.setTarget(hpCutoff);
    lpCutoffSmoother_.setTarget(lpCutoff);
    
    // Trigger envelopes
    ampEnvelope_.trigger();
    filterEnvelope_.trigger();
}

void SerialHPLPEngine::noteOff(float releaseTime) {
    if (!voiceState_.active) {
        return;
    }
    
    // Set envelope release times
    if (releaseTime > 0.0f) {
        ampEnvelope_.setRelease(releaseTime);
        filterEnvelope_.setRelease(releaseTime * 0.7f);
    }
    
    // Release envelopes
    ampEnvelope_.release();
    filterEnvelope_.release();
}

void SerialHPLPEngine::allNotesOff() {
    voiceState_.active = false;
    voiceState_.ampEnvActive = false;
    voiceState_.filterEnvActive = false;
    
    ampEnvelope_.reset();
    filterEnvelope_.reset();
    
    // Reset oscillator phases
    voiceState_.phase1 = 0.0f;
    voiceState_.phase2 = 0.0f;
    voiceState_.unisonPhases.fill(0.0f);
    voiceState_.ringModPhase = 0.0f;
}

void SerialHPLPEngine::setPitchBend(float bendAmount) {
    voiceState_.pitchBend = clamp(bendAmount, -1.0f, 1.0f);
    updateOscillatorParameters();
}

void SerialHPLPEngine::setFilterCutoffs(float hpCutoff, float lpCutoff) {
    hpCutoffSmoother_.setTarget(clamp(hpCutoff, MIN_HP_CUTOFF_HZ, MAX_HP_CUTOFF_HZ));
    lpCutoffSmoother_.setTarget(clamp(lpCutoff, MIN_LP_CUTOFF_HZ, MAX_LP_CUTOFF_HZ));
}

void SerialHPLPEngine::setDetune(float detune) {
    detuneSmoother_.setTarget(clamp(detune, -MAX_DETUNE_SEMITONES, MAX_DETUNE_SEMITONES));
}

void SerialHPLPEngine::setRingModAmount(float amount) {
    ringModSmoother_.setTarget(clamp(amount, 0.0f, 1.0f));
}

void SerialHPLPEngine::setStereoWidth(float width) {
    widthSmoother_.setTarget(clamp(width, 0.0f, 2.0f));
}

std::pair<float, float> SerialHPLPEngine::processSampleStereo() {
    if (!initialized_ || !voiceState_.active) {
        return {0.0f, 0.0f};
    }
    
    uint32_t startTime = getTimeMs();
    
    // Process oscillators
    float oscOutput = processOscillators();
    
    // Process ring modulation
    if (ringModConfig_.mode != RingModMode::OFF && currentRingMod_ > 0.0f) {
        float modulator = generateRingModulator();
        oscOutput = processRingModulation(oscOutput, modulator);
    }
    
    // Process serial filters
    auto [hpOut, lpOut] = processFilters(oscOutput);
    
    // Apply filter balance
    float filteredSignal = lerp(hpOut, lpOut, filterConfig_.filterBalance);
    
    // Apply amplitude envelope
    float ampLevel = ampEnvelope_.processSample();
    float envelopedSignal = filteredSignal * ampLevel;
    
    // Process stereo effects
    auto [leftOut, rightOut] = processStereoEffects(envelopedSignal);
    
    // Check if voice should be deactivated
    if (ampEnvelope_.isComplete()) {
        voiceState_.active = false;
    }
    
    // Update CPU usage
    uint32_t endTime = getTimeMs();
    float processingTime = (endTime - startTime) * 0.001f;
    cpuUsage_ = cpuUsage_ * 0.99f + processingTime * 0.01f;
    
    return {leftOut, rightOut};
}

float SerialHPLPEngine::processSample() {
    auto [left, right] = processSampleStereo();
    return (left + right) * 0.5f; // Mono sum
}

void SerialHPLPEngine::processBlockStereo(float* outputL, float* outputR, int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        auto [left, right] = processSampleStereo();
        outputL[i] = left;
        outputR[i] = right;
    }
}

void SerialHPLPEngine::processBlock(float* output, int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        output[i] = processSample();
    }
}

void SerialHPLPEngine::processParameters(float deltaTimeMs) {
    if (!initialized_) {
        return;
    }
    
    // Update parameter smoothers
    currentHPCutoff_ = hpCutoffSmoother_.process();
    currentLPCutoff_ = lpCutoffSmoother_.process();
    currentHPDrive_ = hpDriveSmoother_.process();
    currentDetune_ = detuneSmoother_.process();
    currentRingMod_ = ringModSmoother_.process();
    currentWidth_ = widthSmoother_.process();
    
    // Apply smoothed parameters
    updateFilterParameters();
    updateRingModParameters();
    updateStereoParameters();
}

// Private method implementations

void SerialHPLPEngine::updateOscillatorParameters() {
    if (!initialized_ || !voiceState_.active) {
        return;
    }
    
    float baseFreq = noteToFrequency(voiceState_.note + voiceState_.pitchBend * 2.0f); // ±2 semitones pitch bend
    
    // Set main oscillator frequencies
    osc1_.setFrequency(baseFreq);
    
    float detunedFreq = baseFreq * semitonesToRatio(currentDetune_);
    osc2_.setFrequency(detunedFreq);
    
    // Set oscillator levels
    osc1_.setLevel(oscConfig_.level1);
    osc2_.setLevel(oscConfig_.level2);
    
    // Set pulse widths
    osc1_.setPulseWidth(oscConfig_.pulseWidth1);
    osc2_.setPulseWidth(oscConfig_.pulseWidth2);
    
    // Configure noise oscillator
    noiseOsc_.setLevel(oscConfig_.noiseLevel);
    
    // Configure unison oscillators for supersaw
    if (oscConfig_.waveform1 == OscillatorWaveform::SUPERSAW || 
        oscConfig_.waveform2 == OscillatorWaveform::SUPERSAW) {
        for (int i = 0; i < oscConfig_.unisonVoices; i++) {
            float detuneAmount = (static_cast<float>(i) - oscConfig_.unisonVoices * 0.5f) * oscConfig_.unisonDetune;
            float unisonFreq = baseFreq * semitonesToRatio(detuneAmount);
            unisonOscs_[i].setFrequency(unisonFreq);
            unisonOscs_[i].setLevel(1.0f / oscConfig_.unisonVoices); // Equal levels
        }
    }
}

void SerialHPLPEngine::updateFilterParameters() {
    // Apply filter envelope modulation
    float filterEnvValue = filterEnvelope_.processSample();
    
    float modulatedHPCutoff = currentHPCutoff_ * (1.0f + filterEnvValue * 0.5f);
    float modulatedLPCutoff = currentLPCutoff_ * (1.0f + filterEnvValue * 2.0f);
    
    hpFilter_.setCutoff(clamp(modulatedHPCutoff, MIN_HP_CUTOFF_HZ, MAX_HP_CUTOFF_HZ));
    lpFilter_.setCutoff(clamp(modulatedLPCutoff, MIN_LP_CUTOFF_HZ, MAX_LP_CUTOFF_HZ));
    
    hpFilter_.setResonance(filterConfig_.hpResonance);
    lpFilter_.setResonance(filterConfig_.lpResonance);
}

void SerialHPLPEngine::updateRingModParameters() {
    voiceState_.ringModAmount = currentRingMod_;
}

void SerialHPLPEngine::updateStereoParameters() {
    // Update chorus LFO
    float deltaTime = 1.0f / sampleRate_;
    chorusLFOPhase_ += deltaTime * stereoConfig_.chorusRate * 2.0f * M_PI;
    if (chorusLFOPhase_ > 2.0f * M_PI) {
        chorusLFOPhase_ -= 2.0f * M_PI;
    }
}

float SerialHPLPEngine::processOscillators() {
    float output = 0.0f;
    
    // Process main oscillators
    if (oscConfig_.waveform1 == OscillatorWaveform::SUPERSAW) {
        output += processSupersaw() * oscConfig_.level1;
    } else {
        output += osc1_.processSample() * oscConfig_.level1;
    }
    
    if (oscConfig_.waveform2 == OscillatorWaveform::SUPERSAW) {
        output += processSupersaw() * oscConfig_.level2;
    } else {
        output += osc2_.processSample() * oscConfig_.level2;
    }
    
    // Add noise
    output += noiseOsc_.processSample() * oscConfig_.noiseLevel;
    
    // Apply hard sync if enabled
    if (oscConfig_.hardSync) {
        // Simplified hard sync implementation
        if (osc1_.getPhase() < voiceState_.phase1) {
            osc2_.resetPhase();
        }
        voiceState_.phase1 = osc1_.getPhase();
    }
    
    return output;
}

float SerialHPLPEngine::processSupersaw() {
    float output = 0.0f;
    
    for (int i = 0; i < oscConfig_.unisonVoices; i++) {
        output += unisonOscs_[i].processSample();
    }
    
    return output;
}

std::pair<float, float> SerialHPLPEngine::processFilters(float input) {
    // Apply pre-HP drive
    float drivenInput = applyFilterDrive(input, currentHPDrive_);
    
    // Process through HP filter
    float hpOutput = hpFilter_.processSample(drivenInput);
    lastHPOutput_ = hpOutput;
    
    // Apply vintage characteristics
    if (filterConfig_.vintageModeling) {
        hpOutput = applyVintageCharacteristics(hpOutput, currentHPCutoff_);
    }
    
    float lpOutput;
    
    switch (filterConfig_.mode) {
        case FilterMode::SERIAL_HP_LP:
            // Serial: HP → LP
            lpOutput = lpFilter_.processSample(hpOutput);
            break;
            
        case FilterMode::PARALLEL:
            // Parallel: HP + LP
            lpOutput = lpFilter_.processSample(drivenInput);
            break;
            
        case FilterMode::LP_ONLY:
            // LP only
            lpOutput = lpFilter_.processSample(drivenInput);
            hpOutput = drivenInput; // Bypass HP
            break;
            
        case FilterMode::HP_ONLY:
            // HP only
            lpOutput = hpOutput; // Use HP output
            break;
            
        case FilterMode::BANDPASS:
            // HP → LP with resonant peak
            lpOutput = lpFilter_.processSample(hpOutput);
            // Increase resonance for bandpass effect
            lpFilter_.setResonance(filterConfig_.lpResonance * 1.5f);
            break;
            
        default:
            lpOutput = lpFilter_.processSample(hpOutput);
            break;
    }
    
    lastLPOutput_ = lpOutput;
    
    // Apply saturation
    if (filterConfig_.saturation > 0.0f) {
        lpOutput = applyFilterSaturation(lpOutput, filterConfig_.saturation);
    }
    
    return {hpOutput, lpOutput};
}

float SerialHPLPEngine::applyFilterDrive(float input, float drive) {
    if (drive <= 0.0f) {
        return input;
    }
    
    float driveGain = 1.0f + drive * (MAX_DRIVE_GAIN - 1.0f);
    float driven = input * driveGain;
    
    // Soft saturation
    return std::tanh(driven) * 0.8f;
}

float SerialHPLPEngine::applyVintageCharacteristics(float input, float cutoff) {
    // Subtle non-linearities for vintage character
    float nonlinear = input + input * input * input * 0.1f;
    
    // Frequency-dependent saturation
    float satAmount = (cutoff / MAX_HP_CUTOFF_HZ) * 0.2f;
    
    return lerp(input, nonlinear, satAmount);
}

float SerialHPLPEngine::processRingModulation(float carrier, float modulator) {
    float ringModOutput;
    
    switch (ringModConfig_.mode) {
        case RingModMode::CLASSIC:
            ringModOutput = carrier * modulator;
            break;
            
        case RingModMode::BALANCED:
            ringModOutput = carrier * modulator - carrier * 0.5f;
            break;
            
        case RingModMode::FREQUENCY:
            {
                float modFreq = noteToFrequency(voiceState_.note) * ringModConfig_.frequency;
                float freqMod = std::sin(voiceState_.ringModPhase);
                voiceState_.ringModPhase += 2.0f * M_PI * modFreq / sampleRate_;
                if (voiceState_.ringModPhase > 2.0f * M_PI) {
                    voiceState_.ringModPhase -= 2.0f * M_PI;
                }
                ringModOutput = carrier * freqMod;
            }
            break;
            
        case RingModMode::SYNC:
            // Hard sync implementation
            ringModOutput = carrier; // Simplified for now
            break;
            
        default:
            ringModOutput = carrier;
            break;
    }
    
    // Apply ring mod amount
    return lerp(carrier, ringModOutput, currentRingMod_);
}

float SerialHPLPEngine::generateRingModulator() {
    float modFreq = noteToFrequency(voiceState_.note) * ringModConfig_.frequency;
    float modulator = std::sin(voiceState_.ringModPhase);
    
    voiceState_.ringModPhase += 2.0f * M_PI * modFreq / sampleRate_;
    if (voiceState_.ringModPhase > 2.0f * M_PI) {
        voiceState_.ringModPhase -= 2.0f * M_PI;
    }
    
    return modulator;
}

std::pair<float, float> SerialHPLPEngine::processStereoEffects(float input) {
    std::pair<float, float> output = {input, input};
    
    // Apply chorus if enabled
    if (stereoConfig_.chorusEnable) {
        output = processChorus(output.first, output.second);
    }
    
    // Apply stereo width
    output = applyStereoWidth(output.first, output.second, currentWidth_);
    
    // Apply mono low-end processing
    if (stereoConfig_.monoLowEnd) {
        output = processMonoLowEnd(output.first, output.second);
    }
    
    return output;
}

std::pair<float, float> SerialHPLPEngine::processChorus(float inputL, float inputR) {
    // Generate chorus LFO
    float lfoValue = std::sin(chorusLFOPhase_);
    
    // Calculate delay times
    float baseDelay = stereoConfig_.chorusDelay * sampleRate_ / 1000.0f;
    float delayL = baseDelay + lfoValue * stereoConfig_.chorusDepth * 100.0f;
    float delayR = baseDelay - lfoValue * stereoConfig_.chorusDepth * 100.0f;
    
    // Simple delay line processing (linear interpolation would be better)
    int delayIndexL = static_cast<int>(delayL);
    int delayIndexR = static_cast<int>(delayR);
    
    delayIndexL = clamp(delayIndexL, 0, static_cast<int>(chorusDelayLineL_.size()) - 1);
    delayIndexR = clamp(delayIndexR, 0, static_cast<int>(chorusDelayLineR_.size()) - 1);
    
    int readIndexL = (chorusDelayIndexL_ - delayIndexL + chorusDelayLineL_.size()) % chorusDelayLineL_.size();
    int readIndexR = (chorusDelayIndexR_ - delayIndexR + chorusDelayLineR_.size()) % chorusDelayLineR_.size();
    
    float delayedL = chorusDelayLineL_[readIndexL];
    float delayedR = chorusDelayLineR_[readIndexR];
    
    // Write to delay lines
    chorusDelayLineL_[chorusDelayIndexL_] = inputL + delayedL * stereoConfig_.chorusFeedback;
    chorusDelayLineR_[chorusDelayIndexR_] = inputR + delayedR * stereoConfig_.chorusFeedback;
    
    // Update delay indices
    chorusDelayIndexL_ = (chorusDelayIndexL_ + 1) % chorusDelayLineL_.size();
    chorusDelayIndexR_ = (chorusDelayIndexR_ + 1) % chorusDelayLineR_.size();
    
    // Mix dry and wet signals
    float outputL = inputL + delayedL * stereoConfig_.chorusDepth;
    float outputR = inputR + delayedR * stereoConfig_.chorusDepth;
    
    return {outputL, outputR};
}

std::pair<float, float> SerialHPLPEngine::processMonoLowEnd(float inputL, float inputR) {
    // Extract mono low frequencies
    float monoInput = (inputL + inputR) * 0.5f;
    float lowFreqs = monoInput - monoHPFilter_.processSample(monoInput);
    
    // Extract high frequencies from stereo signal
    float highL = monoHPFilter_.processSample(inputL);
    float highR = monoHPFilter_.processSample(inputR);
    
    // Combine mono low with stereo high
    return {lowFreqs + highL, lowFreqs + highR};
}

std::pair<float, float> SerialHPLPEngine::applyStereoWidth(float inputL, float inputR, float width) {
    if (width <= 0.0f) {
        // Mono
        float mono = (inputL + inputR) * 0.5f;
        return {mono, mono};
    }
    
    // Calculate mid/side
    float mid = (inputL + inputR) * 0.5f;
    float side = (inputL - inputR) * 0.5f;
    
    // Apply width to side signal
    side *= width;
    
    // Convert back to left/right
    float outputL = mid + side;
    float outputR = mid - side;
    
    return {outputL, outputR};
}

// Parameter mapping functions
float SerialHPLPEngine::mapHarmonicsToHP(float harmonics) const {
    // Exponential mapping for HP cutoff
    return 0.2f + harmonics * harmonics * 4.8f; // 0.2x to 5.0x range
}

float SerialHPLPEngine::mapTimbreToDetune(float timbre) const {
    // Map to detune range
    return (timbre - 0.5f) * MAX_DETUNE_SEMITONES; // ±12 semitones
}

float SerialHPLPEngine::mapMorphToBalance(float morph) const {
    // S-curve for smooth balance transition
    float sCurve = morph * morph * (3.0f - 2.0f * morph);
    return sCurve;
}

float SerialHPLPEngine::calculateKeyTrackedCutoff(float note, float baseCutoff) const {
    float keyTrackAmount = (note - 60.0f) / 12.0f; // Relative to C4
    float keyTrackMultiplier = 1.0f + keyTrackAmount * filterConfig_.keyTracking;
    return baseCutoff * keyTrackMultiplier;
}

float SerialHPLPEngine::applyFilterSaturation(float input, float amount) const {
    if (amount <= 0.0f) return input;
    
    float saturated = std::tanh(input * (1.0f + amount * 3.0f));
    return lerp(input, saturated, amount);
}

// Utility functions
float SerialHPLPEngine::noteToFrequency(float note) const {
    return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
}

float SerialHPLPEngine::semitonesToRatio(float semitones) const {
    return std::pow(2.0f, semitones / 12.0f);
}

float SerialHPLPEngine::dbToLinear(float db) const {
    return std::pow(10.0f, db / 20.0f);
}

uint32_t SerialHPLPEngine::getTimeMs() const {
#ifdef STM32H7
    return HAL_GetTick();
#else
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
#endif
}

float SerialHPLPEngine::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}

float SerialHPLPEngine::clamp(float value, float min, float max) const {
    return std::max(min, std::min(max, value));
}

float SerialHPLPEngine::getCPUUsage() const {
    return cpuUsage_;
}

float SerialHPLPEngine::getFilterDrive() const {
    return currentHPDrive_;
}

void SerialHPLPEngine::reset() {
    allNotesOff();
    
    // Reset all smoothers
    hpCutoffSmoother_.reset();
    lpCutoffSmoother_.reset();
    hpDriveSmoother_.reset();
    detuneSmoother_.reset();
    ringModSmoother_.reset();
    widthSmoother_.reset();
    
    // Clear delay lines
    chorusDelayLineL_.fill(0.0f);
    chorusDelayLineR_.fill(0.0f);
    chorusDelayIndexL_ = 0;
    chorusDelayIndexR_ = 0;
    chorusLFOPhase_ = 0.0f;
    
    cpuUsage_ = 0.0f;
}

void SerialHPLPEngine::setPreset(const std::string& presetName) {
    // Preset loading would be implemented here
    reset();
}