#include "SlideAccentBassEngine.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef STM32H7
#include "stm32h7xx_hal.h"
#else
#include <chrono>
#endif

SlideAccentBassEngine::SlideAccentBassEngine() {
    // Initialize default configurations
    slideConfig_.mode = SlideMode::LEGATO_ONLY;
    slideConfig_.minTimeMs = 5.0f;
    slideConfig_.maxTimeMs = 120.0f;
    slideConfig_.curve = 0.7f; // Slightly exponential for natural feel
    slideConfig_.quantizeTime = false;
    slideConfig_.portamentoAmount = 1.0f;
    
    accentConfig_.mode = AccentMode::VELOCITY;
    accentConfig_.velocityThreshold = 100.0f;
    accentConfig_.volumeBoost = 6.0f;
    accentConfig_.cutoffBoost = 20.0f;
    accentConfig_.resonanceBoost = 15.0f;
    accentConfig_.driveBoost = 25.0f;
    accentConfig_.decayTime = 0.05f;
    accentConfig_.accentEnvelope = true;
    
    filterConfig_.cutoffHz = 1000.0f;
    filterConfig_.resonance = 0.3f;
    filterConfig_.keyTracking = 0.5f;
    filterConfig_.envelopeDepth = 0.6f;
    filterConfig_.velocitySensitivity = 0.4f;
    filterConfig_.autoResonanceRide = true;
    filterConfig_.saturationDrive = 0.2f;
    
    oscConfig_.shape = 0.2f; // Slightly toward saw wave
    oscConfig_.pulseWidth = 0.5f;
    oscConfig_.subLevel = 0.3f;
    oscConfig_.subShape = 0.0f; // Pure sine for sub
    oscConfig_.subOctave = -1;
    oscConfig_.drive = 0.2f;
    oscConfig_.noiseLevel = 0.05f;
    
    phaseResetPolicy_ = PhaseResetPolicy::NON_LEGATO;
    
    // Initialize state
    voiceState_.active = false;
    voiceState_.legato = false;
    voiceState_.accented = false;
    voiceState_.note = 60.0f;
    voiceState_.targetNote = 60.0f;
    voiceState_.velocity = 100.0f;
    voiceState_.slideProgress = 1.0f;
    voiceState_.slideTime = 0.0f;
    voiceState_.accentAmount = 0.0f;
    voiceState_.accentPhase = 0.0f;
    voiceState_.noteOnTime = 0;
    voiceState_.lastNoteTime = 0;
    voiceState_.phase = 0.0f;
    voiceState_.subPhase = 0.0f;
    voiceState_.phaseReset = false;
    
    sampleRate_ = 44100.0f;
    initialized_ = false;
    
    // Initialize parameter values
    harmonics_ = 0.5f;
    timbre_ = 0.5f;
    morph_ = 0.5f;
    
    currentCutoff_ = filterConfig_.cutoffHz;
    currentResonance_ = filterConfig_.resonance;
    currentDrive_ = oscConfig_.drive;
    currentVolume_ = 1.0f;
    
    baseVolume_ = 1.0f;
    baseCutoff_ = filterConfig_.cutoffHz;
    baseResonance_ = filterConfig_.resonance;
    baseDrive_ = oscConfig_.drive;
    
    cpuUsage_ = 0.0f;
}

SlideAccentBassEngine::~SlideAccentBassEngine() {
    shutdown();
}

bool SlideAccentBassEngine::initialize(float sampleRate) {
    if (initialized_) {
        return true;
    }
    
    sampleRate_ = sampleRate;
    
    // Initialize audio components
    if (!mainOsc_.initialize(sampleRate)) {
        return false;
    }
    
    if (!subOsc_.initialize(sampleRate)) {
        return false;
    }
    
    if (!filter_.initialize(sampleRate)) {
        return false;
    }
    
    if (!ampEnvelope_.initialize(sampleRate)) {
        return false;
    }
    
    if (!filterEnvelope_.initialize(sampleRate)) {
        return false;
    }
    
    // Initialize parameter smoothers
    cutoffSmoother_.initialize(sampleRate, 0.01f); // 10ms smoothing
    resonanceSmoother_.initialize(sampleRate, 0.005f); // 5ms smoothing
    driveSmoother_.initialize(sampleRate, 0.02f); // 20ms smoothing
    volumeSmoother_.initialize(sampleRate, 0.001f); // 1ms smoothing
    
    // Configure envelopes
    ampEnvelope_.setADSR(0.001f, 0.1f, 0.8f, 0.05f); // Fast attack, moderate decay
    filterEnvelope_.setADSR(0.001f, 0.3f, 0.3f, 0.1f); // Quick filter envelope
    
    // Configure oscillators
    mainOsc_.setWaveform(VirtualAnalogOscillator::SAWTOOTH);
    subOsc_.setWaveform(VirtualAnalogOscillator::SINE);
    
    // Configure filter
    filter_.setMode(ZDFLadderFilter::Mode::LOWPASS_24DB);
    filter_.setCutoff(filterConfig_.cutoffHz);
    filter_.setResonance(filterConfig_.resonance);
    filter_.setDrive(filterConfig_.saturationDrive);
    
    initialized_ = true;
    return true;
}

void SlideAccentBassEngine::shutdown() {
    if (!initialized_) {
        return;
    }
    
    allNotesOff();
    
    mainOsc_.shutdown();
    subOsc_.shutdown();
    filter_.shutdown();
    ampEnvelope_.shutdown();
    filterEnvelope_.shutdown();
    
    initialized_ = false;
}

void SlideAccentBassEngine::setSlideConfig(const SlideConfig& config) {
    slideConfig_ = config;
    
    // Clamp values to valid ranges
    slideConfig_.minTimeMs = clamp(slideConfig_.minTimeMs, MIN_SLIDE_TIME_MS, MAX_SLIDE_TIME_MS);
    slideConfig_.maxTimeMs = clamp(slideConfig_.maxTimeMs, slideConfig_.minTimeMs, MAX_SLIDE_TIME_MS);
    slideConfig_.curve = clamp(slideConfig_.curve, 0.0f, 1.0f);
    slideConfig_.portamentoAmount = clamp(slideConfig_.portamentoAmount, 0.0f, 2.0f);
}

void SlideAccentBassEngine::setAccentConfig(const AccentConfig& config) {
    accentConfig_ = config;
    
    // Clamp values to valid ranges
    accentConfig_.velocityThreshold = clamp(accentConfig_.velocityThreshold, 1.0f, 127.0f);
    accentConfig_.volumeBoost = clamp(accentConfig_.volumeBoost, 0.0f, MAX_ACCENT_BOOST_DB);
    accentConfig_.cutoffBoost = clamp(accentConfig_.cutoffBoost, 0.0f, 100.0f);
    accentConfig_.resonanceBoost = clamp(accentConfig_.resonanceBoost, 0.0f, 50.0f);
    accentConfig_.driveBoost = clamp(accentConfig_.driveBoost, 0.0f, 100.0f);
    accentConfig_.decayTime = clamp(accentConfig_.decayTime, 0.001f, 1.0f);
}

void SlideAccentBassEngine::setFilterConfig(const FilterConfig& config) {
    filterConfig_ = config;
    
    // Clamp values to valid ranges
    filterConfig_.cutoffHz = clamp(filterConfig_.cutoffHz, MIN_CUTOFF_HZ, MAX_CUTOFF_HZ);
    filterConfig_.resonance = clamp(filterConfig_.resonance, 0.0f, 1.0f);
    filterConfig_.keyTracking = clamp(filterConfig_.keyTracking, 0.0f, 2.0f);
    filterConfig_.envelopeDepth = clamp(filterConfig_.envelopeDepth, 0.0f, 2.0f);
    filterConfig_.velocitySensitivity = clamp(filterConfig_.velocitySensitivity, 0.0f, 1.0f);
    filterConfig_.saturationDrive = clamp(filterConfig_.saturationDrive, 0.0f, 1.0f);
    
    // Update base values
    baseCutoff_ = filterConfig_.cutoffHz;
    baseResonance_ = filterConfig_.resonance;
}

void SlideAccentBassEngine::setOscillatorConfig(const OscillatorConfig& config) {
    oscConfig_ = config;
    
    // Clamp values to valid ranges
    oscConfig_.shape = clamp(oscConfig_.shape, 0.0f, 1.0f);
    oscConfig_.pulseWidth = clamp(oscConfig_.pulseWidth, 0.05f, 0.95f);
    oscConfig_.subLevel = clamp(oscConfig_.subLevel, 0.0f, 1.0f);
    oscConfig_.subShape = clamp(oscConfig_.subShape, 0.0f, 1.0f);
    oscConfig_.subOctave = clamp(oscConfig_.subOctave, -3, 0);
    oscConfig_.drive = clamp(oscConfig_.drive, 0.0f, 1.0f);
    oscConfig_.noiseLevel = clamp(oscConfig_.noiseLevel, 0.0f, 0.5f);
    
    // Update base drive
    baseDrive_ = oscConfig_.drive;
}

void SlideAccentBassEngine::setHarmonics(float harmonics) {
    harmonics_ = clamp(harmonics, 0.0f, 1.0f);
    
    // Map harmonics to filter cutoff with exponential curve
    float cutoffMultiplier = mapHarmonicsToFilter(harmonics_);
    float targetCutoff = baseCutoff_ * cutoffMultiplier;
    
    // Apply key tracking
    if (voiceState_.active) {
        targetCutoff = calculateKeyTrackedCutoff(voiceState_.note, targetCutoff);
    }
    
    cutoffSmoother_.setTarget(targetCutoff);
    
    // Auto-resonance ride
    if (filterConfig_.autoResonanceRide) {
        float targetResonance = calculateAutoResonance(targetCutoff, baseResonance_);
        resonanceSmoother_.setTarget(targetResonance);
    }
}

void SlideAccentBassEngine::setTimbre(float timbre) {
    timbre_ = clamp(timbre, 0.0f, 1.0f);
    
    // Map timbre to oscillator characteristics
    float oscShape = mapTimbreToOscillator(timbre_);
    
    // Update oscillator shape (0=saw, 0.5=square, 1=triangle)
    if (oscShape < 0.5f) {
        // Saw to square transition
        mainOsc_.setWaveform(VirtualAnalogOscillator::SAWTOOTH);
        mainOsc_.setPulseWidth(0.5f + oscShape); // Slightly asymmetric
    } else {
        // Square to triangle transition
        mainOsc_.setWaveform(VirtualAnalogOscillator::SQUARE);
        mainOsc_.setPulseWidth(oscConfig_.pulseWidth);
    }
    
    // Update sub oscillator level
    float subLevel = oscConfig_.subLevel * (0.5f + timbre_ * 0.5f);
    subOsc_.setLevel(subLevel);
    
    // Update drive amount
    float targetDrive = baseDrive_ * (1.0f + timbre_ * 2.0f);
    driveSmoother_.setTarget(targetDrive);
}

void SlideAccentBassEngine::setMorph(float morph) {
    morph_ = clamp(morph, 0.0f, 1.0f);
    
    // Map morph to slide/accent characteristics
    float slideAccentAmount = mapMorphToSlideAccent(morph_);
    
    // Update slide time scaling
    slideConfig_.portamentoAmount = 0.1f + slideAccentAmount * 1.9f;
    
    // Update accent sensitivity
    accentConfig_.volumeBoost = slideAccentAmount * MAX_ACCENT_BOOST_DB;
    accentConfig_.cutoffBoost = slideAccentAmount * 50.0f;
    
    // Update filter envelope depth
    filterConfig_.envelopeDepth = slideAccentAmount * 2.0f;
    filterEnvelope_.setDepth(filterConfig_.envelopeDepth);
}

void SlideAccentBassEngine::setHTMParameters(float harmonics, float timbre, float morph) {
    setHarmonics(harmonics);
    setTimbre(timbre);
    setMorph(morph);
}

void SlideAccentBassEngine::getHTMParameters(float& harmonics, float& timbre, float& morph) const {
    harmonics = harmonics_;
    timbre = timbre_;
    morph = morph_;
}

void SlideAccentBassEngine::noteOn(float note, float velocity, bool accent, float slideTimeMs) {
    uint32_t currentTime = getTimeMs();
    
    // Store previous note for slide calculation
    float previousNote = voiceState_.note;
    bool wasActive = voiceState_.active;
    
    // Update voice state
    voiceState_.lastNoteTime = voiceState_.noteOnTime;
    voiceState_.noteOnTime = currentTime;
    voiceState_.targetNote = note;
    voiceState_.velocity = velocity;
    voiceState_.accented = accent;
    
    // Determine if this is a legato note
    bool isLegato = wasActive && (currentTime - voiceState_.lastNoteTime) < 100; // 100ms legato threshold
    voiceState_.legato = isLegato;
    
    // Calculate slide time
    float calculatedSlideTime = (slideTimeMs > 0.0f) ? slideTimeMs : calculateSlideTime(previousNote, note);
    
    // Check if we should slide
    bool shouldSlide = false;
    switch (slideConfig_.mode) {
        case SlideMode::OFF:
            shouldSlide = false;
            break;
        case SlideMode::LEGATO_ONLY:
            shouldSlide = isLegato;
            break;
        case SlideMode::ALWAYS:
            shouldSlide = wasActive;
            break;
        case SlideMode::ACCENT_ONLY:
            shouldSlide = accent;
            break;
    }
    
    if (shouldSlide && wasActive) {
        // Start slide from current note to target
        slideStartNote_ = voiceState_.note;
        slideEndNote_ = note;
        voiceState_.slideTime = calculatedSlideTime;
        voiceState_.slideProgress = 0.0f;
        slideStartTime_ = currentTime;
    } else {
        // Immediate note change
        voiceState_.note = note;
        voiceState_.slideProgress = 1.0f;
        voiceState_.slideTime = 0.0f;
    }
    
    // Handle phase reset
    if (shouldResetPhase(isLegato)) {
        resetOscillatorPhases();
    }
    
    // Update oscillator frequencies
    updateOscillatorPhases(voiceState_.note);
    
    // Process accent
    if (shouldAccent(velocity, accent)) {
        triggerAccent(calculateAccentAmount(velocity, accent));
    }
    
    // Trigger envelopes
    if (!wasActive || !isLegato) {
        ampEnvelope_.trigger();
        filterEnvelope_.trigger();
    }
    
    voiceState_.active = true;
}

void SlideAccentBassEngine::noteOff(float releaseTime) {
    if (!voiceState_.active) {
        return;
    }
    
    // Set envelope release times
    if (releaseTime > 0.0f) {
        ampEnvelope_.setRelease(releaseTime);
        filterEnvelope_.setRelease(releaseTime * 0.5f); // Faster filter release
    }
    
    // Release envelopes
    ampEnvelope_.release();
    filterEnvelope_.release();
    
    // Voice will be marked inactive when amplitude envelope completes
}

void SlideAccentBassEngine::allNotesOff() {
    voiceState_.active = false;
    voiceState_.legato = false;
    voiceState_.accented = false;
    voiceState_.slideProgress = 1.0f;
    voiceState_.accentAmount = 0.0f;
    
    ampEnvelope_.reset();
    filterEnvelope_.reset();
    
    resetOscillatorPhases();
}

void SlideAccentBassEngine::setSlideTime(float timeMs) {
    voiceState_.slideTime = clamp(timeMs, slideConfig_.minTimeMs, slideConfig_.maxTimeMs);
}

void SlideAccentBassEngine::setAccent(bool accented) {
    voiceState_.accented = accented;
    if (accented) {
        triggerAccent(1.0f);
    }
}

void SlideAccentBassEngine::triggerAccent(float amount) {
    voiceState_.accentAmount = clamp(amount, 0.0f, 1.0f);
    voiceState_.accentPhase = 0.0f;
    
    // Apply accent boosts immediately
    applyAccentBoosts(voiceState_.accentAmount);
}

float SlideAccentBassEngine::processSample() {
    if (!initialized_ || !voiceState_.active) {
        return 0.0f;
    }
    
    uint32_t startTime = getTimeMs();
    
    // Update slide progress
    updateSlideParameters();
    
    // Update accent envelope
    updateAccentParameters();
    
    // Process oscillators
    float mainSignal = mainOsc_.processSample();
    float subSignal = subOsc_.processSample();
    
    // Add noise for character
    float noise = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f;
    mainSignal += noise * oscConfig_.noiseLevel;
    
    // Mix main and sub oscillators
    float mixedSignal = mainSignal + subSignal * oscConfig_.subLevel;
    
    // Apply drive/saturation
    float drivenSignal = applySaturation(mixedSignal, currentDrive_);
    
    // Process through filter
    float filteredSignal = filter_.processSample(drivenSignal);
    
    // Apply amplitude envelope
    float ampLevel = ampEnvelope_.processSample();
    float output = filteredSignal * ampLevel * currentVolume_;
    
    // Check if voice should be deactivated
    if (ampEnvelope_.isComplete()) {
        voiceState_.active = false;
    }
    
    // Update CPU usage
    uint32_t endTime = getTimeMs();
    float processingTime = (endTime - startTime) * 0.001f;
    cpuUsage_ = cpuUsage_ * 0.99f + processingTime * 0.01f; // Smooth CPU usage
    
    return output;
}

void SlideAccentBassEngine::processBlock(float* output, int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        output[i] = processSample();
    }
}

void SlideAccentBassEngine::processParameters(float deltaTimeMs) {
    if (!initialized_) {
        return;
    }
    
    // Update parameter smoothers
    currentCutoff_ = cutoffSmoother_.process();
    currentResonance_ = resonanceSmoother_.process();
    currentDrive_ = driveSmoother_.process();
    currentVolume_ = volumeSmoother_.process();
    
    // Apply smoothed parameters to filter
    filter_.setCutoff(currentCutoff_);
    filter_.setResonance(currentResonance_);
    filter_.setDrive(currentDrive_);
}

// Private method implementations

void SlideAccentBassEngine::updateSlideParameters() {
    if (voiceState_.slideTime > 0.0f && voiceState_.slideProgress < 1.0f) {
        float newProgress = calculateSlideProgress(getTimeMs());
        voiceState_.slideProgress = clamp(newProgress, 0.0f, 1.0f);
        
        // Apply slide easing
        float easedProgress = applySlideEasing(voiceState_.slideProgress);
        
        // Interpolate note
        voiceState_.note = lerp(slideStartNote_, slideEndNote_, easedProgress);
        
        // Update oscillator frequencies
        updateOscillatorPhases(voiceState_.note);
    }
}

void SlideAccentBassEngine::updateAccentParameters() {
    if (voiceState_.accentAmount > 0.0f && accentConfig_.accentEnvelope) {
        updateAccentEnvelope(1.0f / sampleRate_ * 1000.0f); // Convert to ms
    }
}

void SlideAccentBassEngine::updateFilterParameters() {
    float envelopeValue = filterEnvelope_.processSample();
    float modulatedCutoff = currentCutoff_ * (1.0f + envelopeValue * filterConfig_.envelopeDepth);
    
    // Apply velocity sensitivity
    float velocityMod = (voiceState_.velocity / 127.0f) * filterConfig_.velocitySensitivity;
    modulatedCutoff *= (1.0f + velocityMod);
    
    filter_.setCutoff(clamp(modulatedCutoff, MIN_CUTOFF_HZ, MAX_CUTOFF_HZ));
}

void SlideAccentBassEngine::updateOscillatorParameters() {
    // Update oscillator shape based on timbre
    // This is handled in setTimbre()
}

float SlideAccentBassEngine::calculateSlideTime(float fromNote, float toNote) const {
    float interval = std::abs(toNote - fromNote);
    
    // Exponential mapping: larger intervals = longer slide times
    float normalizedInterval = clamp(interval / 12.0f, 0.0f, 1.0f); // Normalize to octave
    float timeRange = slideConfig_.maxTimeMs - slideConfig_.minTimeMs;
    float slideTime = slideConfig_.minTimeMs + normalizedInterval * timeRange;
    
    // Apply portamento amount scaling
    slideTime *= slideConfig_.portamentoAmount;
    
    return slideTime;
}

float SlideAccentBassEngine::calculateSlideProgress(uint32_t currentTime) const {
    if (voiceState_.slideTime <= 0.0f) {
        return 1.0f;
    }
    
    float elapsed = (currentTime - slideStartTime_);
    return elapsed / voiceState_.slideTime;
}

float SlideAccentBassEngine::applySlideEasing(float progress) const {
    if (slideConfig_.curve < 0.5f) {
        // Ease-in (slow start)
        float factor = slideConfig_.curve * 2.0f;
        return std::pow(progress, 1.0f + factor * 3.0f);
    } else {
        // Ease-out (slow end)
        float factor = (slideConfig_.curve - 0.5f) * 2.0f;
        return 1.0f - std::pow(1.0f - progress, 1.0f + factor * 3.0f);
    }
}

bool SlideAccentBassEngine::shouldAccent(float velocity, bool patternAccent) const {
    switch (accentConfig_.mode) {
        case AccentMode::OFF:
            return false;
        case AccentMode::VELOCITY:
            return velocity >= accentConfig_.velocityThreshold;
        case AccentMode::PATTERN:
            return patternAccent;
        case AccentMode::COMBINED:
            return (velocity >= accentConfig_.velocityThreshold) || patternAccent;
        default:
            return false;
    }
}

float SlideAccentBassEngine::calculateAccentAmount(float velocity, bool patternAccent) const {
    float amount = 0.0f;
    
    if (accentConfig_.mode == AccentMode::VELOCITY || accentConfig_.mode == AccentMode::COMBINED) {
        if (velocity >= accentConfig_.velocityThreshold) {
            amount = (velocity - accentConfig_.velocityThreshold) / (127.0f - accentConfig_.velocityThreshold);
        }
    }
    
    if (patternAccent && (accentConfig_.mode == AccentMode::PATTERN || accentConfig_.mode == AccentMode::COMBINED)) {
        amount = std::max(amount, 1.0f);
    }
    
    return clamp(amount, 0.0f, 1.0f);
}

void SlideAccentBassEngine::applyAccentBoosts(float accentAmount) {
    // Volume boost
    float volumeBoost = dbToLinear(accentAmount * accentConfig_.volumeBoost);
    volumeSmoother_.setTarget(baseVolume_ * volumeBoost);
    
    // Cutoff boost
    float cutoffBoost = 1.0f + (accentAmount * accentConfig_.cutoffBoost * 0.01f);
    float boostedCutoff = baseCutoff_ * cutoffBoost;
    cutoffSmoother_.setTarget(clamp(boostedCutoff, MIN_CUTOFF_HZ, MAX_CUTOFF_HZ));
    
    // Resonance boost
    float resonanceBoost = 1.0f + (accentAmount * accentConfig_.resonanceBoost * 0.01f);
    float boostedResonance = baseResonance_ * resonanceBoost;
    resonanceSmoother_.setTarget(clamp(boostedResonance, 0.0f, 1.0f));
    
    // Drive boost
    float driveBoost = 1.0f + (accentAmount * accentConfig_.driveBoost * 0.01f);
    float boostedDrive = baseDrive_ * driveBoost;
    driveSmoother_.setTarget(clamp(boostedDrive, 0.0f, 1.0f));
}

void SlideAccentBassEngine::updateAccentEnvelope(float deltaTimeMs) {
    if (voiceState_.accentAmount > 0.0f) {
        voiceState_.accentPhase += deltaTimeMs / (accentConfig_.decayTime * 1000.0f);
        
        if (voiceState_.accentPhase >= 1.0f) {
            voiceState_.accentPhase = 1.0f;
            voiceState_.accentAmount = 0.0f;
        } else {
            // Exponential decay
            float envelope = std::exp(-voiceState_.accentPhase * 5.0f);
            applyAccentBoosts(voiceState_.accentAmount * envelope);
        }
    }
}

float SlideAccentBassEngine::calculateKeyTrackedCutoff(float note, float baseCutoff) const {
    float keyTrackAmount = (note - 60.0f) / 12.0f; // Relative to C4
    float keyTrackMultiplier = 1.0f + keyTrackAmount * filterConfig_.keyTracking;
    return baseCutoff * keyTrackMultiplier;
}

float SlideAccentBassEngine::calculateAutoResonance(float cutoff, float baseResonance) const {
    if (!filterConfig_.autoResonanceRide) {
        return baseResonance;
    }
    
    // Increase resonance as cutoff decreases for that classic acid sound
    float cutoffNorm = (cutoff - MIN_CUTOFF_HZ) / (MAX_CUTOFF_HZ - MIN_CUTOFF_HZ);
    float autoResonance = baseResonance * (1.0f + (1.0f - cutoffNorm) * 0.5f);
    
    return clamp(autoResonance, 0.0f, 1.0f);
}

float SlideAccentBassEngine::applySaturation(float input, float drive) const {
    if (drive <= 0.0f) {
        return input;
    }
    
    float driven = input * (1.0f + drive * (MAX_DRIVE_GAIN - 1.0f));
    
    // Soft clipping saturation
    return std::tanh(driven) * 0.7f;
}

bool SlideAccentBassEngine::shouldResetPhase(bool legato) const {
    switch (phaseResetPolicy_) {
        case PhaseResetPolicy::ALWAYS:
            return true;
        case PhaseResetPolicy::NON_LEGATO:
            return !legato;
        case PhaseResetPolicy::NEVER:
            return false;
        default:
            return true;
    }
}

void SlideAccentBassEngine::resetOscillatorPhases() {
    voiceState_.phase = 0.0f;
    voiceState_.subPhase = 0.0f;
    mainOsc_.resetPhase();
    subOsc_.resetPhase();
}

void SlideAccentBassEngine::updateOscillatorPhases(float note) {
    float frequency = noteToFrequency(note);
    mainOsc_.setFrequency(frequency);
    
    float subFrequency = frequency * std::pow(2.0f, oscConfig_.subOctave);
    subOsc_.setFrequency(subFrequency);
}

// Parameter mapping functions
float SlideAccentBassEngine::mapHarmonicsToFilter(float harmonics) const {
    // Exponential mapping for cutoff (20Hz to 12kHz)
    float logMin = std::log(MIN_CUTOFF_HZ);
    float logMax = std::log(MAX_CUTOFF_HZ);
    float logCutoff = logMin + harmonics * (logMax - logMin);
    return std::exp(logCutoff) / baseCutoff_;
}

float SlideAccentBassEngine::mapTimbreToOscillator(float timbre) const {
    // Map to oscillator shape and character
    return timbre; // Direct mapping for now
}

float SlideAccentBassEngine::mapMorphToSlideAccent(float morph) const {
    // Map to slide sensitivity and accent amount
    return morph; // Direct mapping for now
}

// Utility functions
float SlideAccentBassEngine::noteToFrequency(float note) const {
    return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
}

float SlideAccentBassEngine::dbToLinear(float db) const {
    return std::pow(10.0f, db / 20.0f);
}

float SlideAccentBassEngine::linearToDb(float linear) const {
    return 20.0f * std::log10(std::max(linear, 1e-6f));
}

uint32_t SlideAccentBassEngine::getTimeMs() const {
#ifdef STM32H7
    return HAL_GetTick();
#else
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
#endif
}

float SlideAccentBassEngine::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}

float SlideAccentBassEngine::clamp(float value, float min, float max) const {
    return std::max(min, std::min(max, value));
}

float SlideAccentBassEngine::getFilterCutoff() const {
    return currentCutoff_;
}

float SlideAccentBassEngine::getCPUUsage() const {
    return cpuUsage_;
}

void SlideAccentBassEngine::reset() {
    allNotesOff();
    
    // Reset all smoothers
    cutoffSmoother_.reset();
    resonanceSmoother_.reset();
    driveSmoother_.reset();
    volumeSmoother_.reset();
    
    // Reset to base values
    currentCutoff_ = baseCutoff_;
    currentResonance_ = baseResonance_;
    currentDrive_ = baseDrive_;
    currentVolume_ = baseVolume_;
    
    cpuUsage_ = 0.0f;
}

void SlideAccentBassEngine::setPreset(const std::string& presetName) {
    // Preset loading would be implemented here
    // For now, just reset to defaults
    reset();
}