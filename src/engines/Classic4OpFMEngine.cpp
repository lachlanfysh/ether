#include "Classic4OpFMEngine.h"
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

Classic4OpFMEngine::Classic4OpFMEngine() {
    // Initialize default operator configurations
    for (int i = 0; i < NUM_OPERATORS; i++) {
        operatorConfigs_[i].ratio = (i == 0) ? 1.0f : (1.0f + i * 0.5f); // 1.0, 1.5, 2.0, 2.5
        operatorConfigs_[i].level = (i < 2) ? 0.8f : 1.0f; // Modulators slightly quieter
        operatorConfigs_[i].detune = 0.0f;
        operatorConfigs_[i].waveform = OperatorWaveform::SINE;
        operatorConfigs_[i].fixedFreq = false;
        operatorConfigs_[i].fixedFreqHz = 440.0f;
        operatorConfigs_[i].velocitySensitivity = 0.5f;
        operatorConfigs_[i].keyScaling = 0.0f;
        operatorConfigs_[i].enabled = true;
        
        currentRatios_[i] = operatorConfigs_[i].ratio;
        currentLevels_[i] = operatorConfigs_[i].level;
    }
    
    // Initialize default envelope configurations
    for (int i = 0; i < NUM_OPERATORS; i++) {
        envelopeConfigs_[i].attack = (i < 2) ? 0.001f : 0.01f; // Faster attack for carriers
        envelopeConfigs_[i].decay = 0.1f + i * 0.1f; // Staggered decay times
        envelopeConfigs_[i].sustain = 0.7f - i * 0.1f; // Decreasing sustain levels
        envelopeConfigs_[i].release = 0.5f + i * 0.2f; // Staggered release times
        envelopeConfigs_[i].depth = 1.0f;
        envelopeConfigs_[i].velocitySensitivity = 0.3f;
        envelopeConfigs_[i].exponential = true;
    }
    
    // Initialize algorithm configuration
    algorithmConfig_.algorithm = Algorithm::STACK_4_3_2_1;
    algorithmConfig_.feedback = 0.0f;
    algorithmConfig_.operatorBalance[0] = 1.0f;
    algorithmConfig_.operatorBalance[1] = 1.0f;
    algorithmConfig_.operatorBalance[2] = 1.0f;
    algorithmConfig_.operatorBalance[3] = 1.0f;
    algorithmConfig_.carrierLevel = 1.0f;
    algorithmConfig_.modulatorLevel = 1.0f;
    algorithmConfig_.antiClick = true;
    algorithmConfig_.transitionTime = 0.02f;
    
    // Initialize global configuration
    globalConfig_.masterLevel = 1.0f;
    globalConfig_.brightness = 0.0f;
    globalConfig_.warmth = 0.0f;
    globalConfig_.oversample = true;
    globalConfig_.noiseLevel = 0.0f;
    globalConfig_.analogDrift = 0.0f;
    globalConfig_.monoMode = false;
    globalConfig_.portamentoTime = 0.0f;
    
    // Initialize voice state
    voiceState_.active = false;
    voiceState_.notePressed = false;
    voiceState_.note = 60.0f;
    voiceState_.targetNote = 60.0f;
    voiceState_.velocity = 100.0f;
    voiceState_.pitchBend = 0.0f;
    voiceState_.operatorPhases.fill(0.0f);
    voiceState_.operatorFreqs.fill(440.0f);
    voiceState_.operatorLevels.fill(1.0f);
    voiceState_.operatorActive.fill(true);
    voiceState_.feedbackSample = 0.0f;
    voiceState_.lastOutput = 0.0f;
    voiceState_.portamentoPhase = 1.0f;
    voiceState_.algorithmCrossfade = 1.0f;
    voiceState_.previousAlgorithm = Algorithm::STACK_4_3_2_1;
    voiceState_.algorithmSwitching = false;
    voiceState_.switchStartTime = 0;
    voiceState_.noteOnTime = 0;
    voiceState_.noteOffTime = 0;
    
    // Initialize processing state
    sampleRate_ = 44100.0f;
    initialized_ = false;
    
    harmonics_ = 0.5f;
    timbre_ = 0.5f;
    morph_ = 0.5f;
    
    currentIndex_ = 0.5f;
    currentFeedback_ = 0.0f;
    currentBrightness_ = 0.0f;
    
    previousAlgorithmOutput_ = 0.0f;
    currentAlgorithmOutput_ = 0.0f;
    
    operatorRamps_.fill(1.0f);
    masterRamp_ = 1.0f;
    ramping_ = false;
    
    noisePhase_ = 0.0f;
    noiseState_ = 1;
    
    cpuUsage_ = 0.0f;
}

Classic4OpFMEngine::~Classic4OpFMEngine() {
    shutdown();
}

bool Classic4OpFMEngine::initialize(float sampleRate) {
    if (initialized_) {
        return true;
    }
    
    sampleRate_ = sampleRate;
    
    // Initialize operators
    for (int i = 0; i < NUM_OPERATORS; i++) {
        if (!operators_[i].initialize(sampleRate)) {
            return false;
        }
        operators_[i].setWaveform(static_cast<FMOperator::Waveform>(operatorConfigs_[i].waveform));
        operators_[i].setLevel(operatorConfigs_[i].level);
    }
    
    // Initialize envelopes
    for (int i = 0; i < NUM_OPERATORS; i++) {
        if (!envelopes_[i].initialize(sampleRate)) {
            return false;
        }
        envelopes_[i].setADSR(
            envelopeConfigs_[i].attack,
            envelopeConfigs_[i].decay,
            envelopeConfigs_[i].sustain,
            envelopeConfigs_[i].release
        );
        envelopes_[i].setExponential(envelopeConfigs_[i].exponential);
    }
    
    // Initialize oversampler
    if (globalConfig_.oversample) {
        if (!oversampler_.initialize(sampleRate, OversamplingProcessor::Factor::X2)) {
            return false;
        }
    }
    
    // Initialize parameter smoothers
    feedbackSmoother_.initialize(sampleRate, 0.01f); // 10ms smoothing
    indexSmoother_.initialize(sampleRate, 0.02f);    // 20ms smoothing
    brightnessSmoother_.initialize(sampleRate, 0.05f); // 50ms smoothing
    
    for (int i = 0; i < NUM_OPERATORS; i++) {
        ratioSmoothers_[i].initialize(sampleRate, 0.01f);  // 10ms smoothing
        levelSmoothers_[i].initialize(sampleRate, 0.005f); // 5ms smoothing
    }
    
    // Set initial parameter values
    feedbackSmoother_.setValue(currentFeedback_);
    indexSmoother_.setValue(currentIndex_);
    brightnessSmoother_.setValue(currentBrightness_);
    
    for (int i = 0; i < NUM_OPERATORS; i++) {
        ratioSmoothers_[i].setValue(currentRatios_[i]);
        levelSmoothers_[i].setValue(currentLevels_[i]);
    }
    
    initialized_ = true;
    return true;
}

void Classic4OpFMEngine::shutdown() {
    if (!initialized_) {
        return;
    }
    
    allNotesOff();
    
    for (int i = 0; i < NUM_OPERATORS; i++) {
        operators_[i].shutdown();
        envelopes_[i].shutdown();
    }
    
    if (globalConfig_.oversample) {
        oversampler_.shutdown();
    }
    
    initialized_ = false;
}

void Classic4OpFMEngine::setOperatorConfig(int operatorIndex, const OperatorConfig& config) {
    if (operatorIndex < 0 || operatorIndex >= NUM_OPERATORS) {
        return;
    }
    
    operatorConfigs_[operatorIndex] = config;
    
    // Clamp values to valid ranges
    operatorConfigs_[operatorIndex].ratio = clamp(config.ratio, MIN_RATIO, MAX_RATIO);
    operatorConfigs_[operatorIndex].level = clamp(config.level, 0.0f, 2.0f);
    operatorConfigs_[operatorIndex].detune = clamp(config.detune, -100.0f, 100.0f);
    operatorConfigs_[operatorIndex].velocitySensitivity = clamp(config.velocitySensitivity, 0.0f, 1.0f);
    operatorConfigs_[operatorIndex].keyScaling = clamp(config.keyScaling, 0.0f, 2.0f);
    
    // Update operator
    if (initialized_) {
        operators_[operatorIndex].setWaveform(static_cast<FMOperator::Waveform>(config.waveform));
        operators_[operatorIndex].setEnabled(config.enabled);
        ratioSmoothers_[operatorIndex].setTarget(operatorConfigs_[operatorIndex].ratio);
        levelSmoothers_[operatorIndex].setTarget(operatorConfigs_[operatorIndex].level);
    }
}

const Classic4OpFMEngine::OperatorConfig& Classic4OpFMEngine::getOperatorConfig(int operatorIndex) const {
    static const OperatorConfig defaultConfig;
    if (operatorIndex < 0 || operatorIndex >= NUM_OPERATORS) {
        return defaultConfig;
    }
    return operatorConfigs_[operatorIndex];
}

void Classic4OpFMEngine::setEnvelopeConfig(int operatorIndex, const EnvelopeConfig& config) {
    if (operatorIndex < 0 || operatorIndex >= NUM_OPERATORS) {
        return;
    }
    
    envelopeConfigs_[operatorIndex] = config;
    
    // Clamp values to valid ranges
    envelopeConfigs_[operatorIndex].attack = clamp(config.attack, 0.001f, 10.0f);
    envelopeConfigs_[operatorIndex].decay = clamp(config.decay, 0.001f, 10.0f);
    envelopeConfigs_[operatorIndex].sustain = clamp(config.sustain, 0.0f, 1.0f);
    envelopeConfigs_[operatorIndex].release = clamp(config.release, 0.001f, 10.0f);
    envelopeConfigs_[operatorIndex].depth = clamp(config.depth, 0.0f, 2.0f);
    envelopeConfigs_[operatorIndex].velocitySensitivity = clamp(config.velocitySensitivity, 0.0f, 1.0f);
    
    // Update envelope
    if (initialized_) {
        envelopes_[operatorIndex].setADSR(
            envelopeConfigs_[operatorIndex].attack,
            envelopeConfigs_[operatorIndex].decay,
            envelopeConfigs_[operatorIndex].sustain,
            envelopeConfigs_[operatorIndex].release
        );
        envelopes_[operatorIndex].setDepth(envelopeConfigs_[operatorIndex].depth);
        envelopes_[operatorIndex].setExponential(envelopeConfigs_[operatorIndex].exponential);
    }
}

const Classic4OpFMEngine::EnvelopeConfig& Classic4OpFMEngine::getEnvelopeConfig(int operatorIndex) const {
    static const EnvelopeConfig defaultConfig;
    if (operatorIndex < 0 || operatorIndex >= NUM_OPERATORS) {
        return defaultConfig;
    }
    return envelopeConfigs_[operatorIndex];
}

void Classic4OpFMEngine::setAlgorithmConfig(const AlgorithmConfig& config) {
    algorithmConfig_ = config;
    
    // Clamp values to valid ranges
    algorithmConfig_.feedback = clamp(config.feedback, 0.0f, MAX_FEEDBACK);
    algorithmConfig_.carrierLevel = clamp(config.carrierLevel, 0.0f, 2.0f);
    algorithmConfig_.modulatorLevel = clamp(config.modulatorLevel, 0.0f, 2.0f);
    algorithmConfig_.transitionTime = clamp(config.transitionTime, 0.001f, 0.1f);
    
    for (int i = 0; i < NUM_OPERATORS; i++) {
        algorithmConfig_.operatorBalance[i] = clamp(config.operatorBalance[i], 0.0f, 2.0f);
    }
    
    // Update smoothers
    if (initialized_) {
        feedbackSmoother_.setTarget(algorithmConfig_.feedback);
    }
}

void Classic4OpFMEngine::setGlobalConfig(const GlobalConfig& config) {
    globalConfig_ = config;
    
    // Clamp values to valid ranges
    globalConfig_.masterLevel = clamp(config.masterLevel, 0.0f, 2.0f);
    globalConfig_.brightness = clamp(config.brightness, -1.0f, 1.0f);
    globalConfig_.warmth = clamp(config.warmth, -1.0f, 1.0f);
    globalConfig_.noiseLevel = clamp(config.noiseLevel, 0.0f, 0.1f);
    globalConfig_.analogDrift = clamp(config.analogDrift, 0.0f, 0.1f);
    globalConfig_.portamentoTime = clamp(config.portamentoTime, 0.0f, PORTAMENTO_MAX_TIME_MS);
    
    // Update smoothers
    if (initialized_) {
        brightnessSmoother_.setTarget(globalConfig_.brightness);
    }
}

void Classic4OpFMEngine::setHarmonics(float harmonics) {
    harmonics_ = clamp(harmonics, 0.0f, 1.0f);
    
    // Map harmonics to global FM index with exponential curve
    float targetIndex = mapHarmonicsToIndex(harmonics_);
    indexSmoother_.setTarget(targetIndex);
    
    // Update ratio spread for more harmonic complexity at higher values
    updateRatioSpread(harmonics_);
    
    // Apply high-frequency tilt for brightness
    float brightness = harmonics_ * 0.5f; // Subtle brightness increase
    brightnessSmoother_.setTarget(brightness);
}

void Classic4OpFMEngine::setTimbre(float timbre) {
    timbre_ = clamp(timbre, 0.0f, 1.0f);
    
    // Map timbre to algorithm selection
    Algorithm newAlgorithm = selectAlgorithmFromTimbre(timbre_);
    if (newAlgorithm != algorithmConfig_.algorithm && initialized_) {
        switchAlgorithm(newAlgorithm);
    }
    
    // Update operator waveforms based on timbre
    for (int i = 0; i < NUM_OPERATORS; i++) {
        OperatorWaveform waveform = (timbre_ < 0.33f) ? OperatorWaveform::SINE :
                                   (timbre_ < 0.66f) ? OperatorWaveform::SAW_APPROX :
                                   OperatorWaveform::SQUARE_APPROX;
        
        if (initialized_) {
            operators_[i].setWaveform(static_cast<FMOperator::Waveform>(waveform));
        }
        operatorConfigs_[i].waveform = waveform;
    }
    
    // Update brightness EQ
    float extraBrightness = (timbre_ - 0.5f) * 0.5f;
    brightnessSmoother_.setTarget(globalConfig_.brightness + extraBrightness);
}

void Classic4OpFMEngine::setMorph(float morph) {
    morph_ = clamp(morph, 0.0f, 1.0f);
    
    // Map morph to feedback amount
    float targetFeedback = mapMorphToFeedback(morph_);
    feedbackSmoother_.setTarget(targetFeedback);
    
    // Update envelope speeds (higher morph = faster envelopes)
    updateEnvelopeSpeeds(morph_);
    
    // Update carrier/modulator balance
    float carrierBalance = 0.5f + morph_ * 0.5f; // Favor carriers at high morph
    float modulatorBalance = 1.5f - morph_ * 0.5f; // Reduce modulators at high morph
    
    algorithmConfig_.carrierLevel = carrierBalance;
    algorithmConfig_.modulatorLevel = modulatorBalance;
}

void Classic4OpFMEngine::setHTMParameters(float harmonics, float timbre, float morph) {
    setHarmonics(harmonics);
    setTimbre(timbre);
    setMorph(morph);
}

void Classic4OpFMEngine::getHTMParameters(float& harmonics, float& timbre, float& morph) const {
    harmonics = harmonics_;
    timbre = timbre_;
    morph = morph_;
}

void Classic4OpFMEngine::noteOn(float note, float velocity) {
    uint32_t currentTime = getTimeMs();
    
    bool wasActive = voiceState_.active;
    
    // Update voice state
    voiceState_.notePressed = true;
    voiceState_.targetNote = note;
    voiceState_.velocity = velocity;
    voiceState_.noteOnTime = currentTime;
    
    // Handle portamento
    if (globalConfig_.monoMode && wasActive && globalConfig_.portamentoTime > 0.0f) {
        voiceState_.portamentoPhase = 0.0f;
    } else {
        voiceState_.note = note;
        voiceState_.portamentoPhase = 1.0f;
    }
    
    // Update operator frequencies and levels
    updateOperatorFrequencies();
    updateOperatorLevels(velocity);
    
    // Trigger envelopes
    if (!wasActive || !globalConfig_.monoMode) {
        for (int i = 0; i < NUM_OPERATORS; i++) {
            if (operatorConfigs_[i].enabled) {
                envelopes_[i].trigger();
                voiceState_.operatorActive[i] = true;
            }
        }
    }
    
    // Initialize anti-click
    if (algorithmConfig_.antiClick) {
        initializeAntiClick();
    }
    
    voiceState_.active = true;
}

void Classic4OpFMEngine::noteOff(float releaseTime) {
    if (!voiceState_.notePressed) {
        return;
    }
    
    voiceState_.notePressed = false;
    voiceState_.noteOffTime = getTimeMs();
    
    // Set envelope release times
    for (int i = 0; i < NUM_OPERATORS; i++) {
        if (releaseTime > 0.0f) {
            envelopes_[i].setRelease(releaseTime);
        }
        envelopes_[i].release();
    }
}

void Classic4OpFMEngine::allNotesOff() {
    voiceState_.active = false;
    voiceState_.notePressed = false;
    voiceState_.portamentoPhase = 1.0f;
    voiceState_.feedbackSample = 0.0f;
    voiceState_.lastOutput = 0.0f;
    
    for (int i = 0; i < NUM_OPERATORS; i++) {
        envelopes_[i].reset();
        voiceState_.operatorActive[i] = false;
        voiceState_.operatorPhases[i] = 0.0f;
    }
    
    operatorRamps_.fill(1.0f);
    masterRamp_ = 1.0f;
    ramping_ = false;
}

void Classic4OpFMEngine::setPitchBend(float bendAmount) {
    voiceState_.pitchBend = clamp(bendAmount, -1.0f, 1.0f);
    updateOperatorFrequencies();
}

float Classic4OpFMEngine::processSample() {
    if (!initialized_ || !voiceState_.active) {
        return 0.0f;
    }
    
    uint32_t startTime = getTimeMs();
    
    // Update portamento
    if (voiceState_.portamentoPhase < 1.0f) {
        updatePortamento(1.0f / sampleRate_ * 1000.0f);
    }
    
    // Process operators
    processOperators();
    
    // Generate operator outputs
    std::array<float, 4> operatorOutputs;
    for (int i = 0; i < NUM_OPERATORS; i++) {
        if (voiceState_.operatorActive[i]) {
            float modulationInput = 0.0f;
            
            // Calculate modulation input based on algorithm
            // This would normally be computed in the algorithm processing
            // For now, use a simplified approach
            if (i > 0) {
                modulationInput = operatorOutputs[i-1] * currentIndex_;
            }
            
            operatorOutputs[i] = generateOperatorOutput(i, modulationInput);
        } else {
            operatorOutputs[i] = 0.0f;
        }
    }
    
    // Process algorithm
    float algorithmOutput = processAlgorithm(algorithmConfig_.algorithm, operatorOutputs);
    
    // Handle algorithm crossfade if switching
    if (voiceState_.algorithmSwitching) {
        processAlgorithmCrossfade();
        algorithmOutput = crossfade(previousAlgorithmOutput_, algorithmOutput, voiceState_.algorithmCrossfade);
    }
    
    // Apply anti-click processing
    if (ramping_) {
        processAntiClick();
        algorithmOutput *= masterRamp_;
    }
    
    // Process EQ
    auto eqResult = processEQ(algorithmOutput);
    float output = eqResult.first; // Use left channel for mono
    
    // Add noise for character
    if (globalConfig_.noiseLevel > 0.0f) {
        output += generateNoise() * globalConfig_.noiseLevel;
    }
    
    // Apply master level
    output *= globalConfig_.masterLevel;
    
    // Update feedback state
    updateFeedbackState(output);
    
    // Check if voice should be deactivated
    bool anyEnvelopeActive = false;
    for (int i = 0; i < NUM_OPERATORS; i++) {
        if (envelopes_[i].isActive()) {
            anyEnvelopeActive = true;
            break;
        }
    }
    
    if (!anyEnvelopeActive && !voiceState_.notePressed) {
        voiceState_.active = false;
    }
    
    // Update CPU usage
    uint32_t endTime = getTimeMs();
    float processingTime = (endTime - startTime) * 0.001f;
    cpuUsage_ = cpuUsage_ * CPU_USAGE_SMOOTH + processingTime * (1.0f - CPU_USAGE_SMOOTH);
    
    return output;
}

std::pair<float, float> Classic4OpFMEngine::processSampleStereo() {
    float mono = processSample();
    // For now, return mono signal on both channels
    // Future enhancement: implement stereo width/panning
    return {mono, mono};
}

void Classic4OpFMEngine::processBlock(float* output, int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        output[i] = processSample();
    }
}

void Classic4OpFMEngine::processBlockStereo(float* outputL, float* outputR, int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        auto stereoSample = processSampleStereo();
        outputL[i] = stereoSample.first;
        outputR[i] = stereoSample.second;
    }
}

void Classic4OpFMEngine::processParameters(float deltaTimeMs) {
    if (!initialized_) {
        return;
    }
    
    // Update parameter smoothers
    currentIndex_ = indexSmoother_.process();
    currentFeedback_ = feedbackSmoother_.process();
    currentBrightness_ = brightnessSmoother_.process();
    
    for (int i = 0; i < NUM_OPERATORS; i++) {
        currentRatios_[i] = ratioSmoothers_[i].process();
        currentLevels_[i] = levelSmoothers_[i].process();
    }
    
    // Update analog drift
    if (globalConfig_.analogDrift > 0.0f) {
        updateAnalogDrift();
    }
}

// Private method implementations

void Classic4OpFMEngine::processOperators() {
    // Update operator frequencies based on current note and ratios
    float baseFreq = noteToFrequency(calculatePortamentoNote());
    baseFreq *= std::pow(2.0f, voiceState_.pitchBend * 2.0f / 12.0f); // ±2 semitone bend
    
    for (int i = 0; i < NUM_OPERATORS; i++) {
        if (voiceState_.operatorActive[i]) {
            float freq = operatorConfigs_[i].fixedFreq ? 
                        operatorConfigs_[i].fixedFreqHz :
                        ratioToFrequency(currentRatios_[i], baseFreq);
            
            // Apply detune
            freq *= centsToRatio(operatorConfigs_[i].detune);
            
            voiceState_.operatorFreqs[i] = freq;
            operators_[i].setFrequency(freq);
        }
    }
}

float Classic4OpFMEngine::generateOperatorOutput(int operatorIndex, float modulationInput) {
    if (!voiceState_.operatorActive[operatorIndex] || !operatorConfigs_[operatorIndex].enabled) {
        return 0.0f;
    }
    
    // Get envelope level
    float envelopeLevel = envelopes_[operatorIndex].processSample();
    if (!envelopes_[operatorIndex].isActive()) {
        voiceState_.operatorActive[operatorIndex] = false;
        return 0.0f;
    }
    
    // Apply envelope depth
    envelopeLevel *= envelopeConfigs_[operatorIndex].depth;
    
    // Apply velocity sensitivity to envelope
    float velocityMod = 1.0f + (voiceState_.velocity / 127.0f - 1.0f) * 
                       envelopeConfigs_[operatorIndex].velocitySensitivity;
    envelopeLevel *= velocityMod;
    
    // Generate operator output
    float output = operators_[operatorIndex].processSample(modulationInput);
    
    // Apply operator level
    output *= currentLevels_[operatorIndex] * voiceState_.operatorLevels[operatorIndex];
    
    // Apply envelope
    output *= envelopeLevel;
    
    // Apply operator balance
    output *= algorithmConfig_.operatorBalance[operatorIndex];
    
    return output;
}

float Classic4OpFMEngine::processAlgorithm(Algorithm algorithm, const std::array<float, 4>& operatorInputs) {
    switch (algorithm) {
        case Algorithm::STACK_4_3_2_1:
            return processStack4321(operatorInputs);
        case Algorithm::STACK_4_32_1:
            return processStack432_1(operatorInputs);
        case Algorithm::PARALLEL_2X2:
            return processParallel2x2(operatorInputs);
        case Algorithm::CROSS_MOD:
            return processCrossMod(operatorInputs);
        case Algorithm::RING_4321:
            return processRing4321(operatorInputs);
        case Algorithm::CASCADE_42_31:
            return processCascade42_31(operatorInputs);
        case Algorithm::FEEDBACK_PAIR:
            return processFeedbackPair(operatorInputs);
        case Algorithm::ALL_PARALLEL:
            return processAllParallel(operatorInputs);
        default:
            return processStack4321(operatorInputs);
    }
}

// Algorithm implementations
float Classic4OpFMEngine::processStack4321(const std::array<float, 4>& ops) {
    // Linear stack: 4→3→2→1
    float mod4to3 = ops[3] * currentIndex_;
    float out3 = generateOperatorOutput(2, mod4to3);
    
    float mod3to2 = out3 * currentIndex_;
    float out2 = generateOperatorOutput(1, mod3to2);
    
    float mod2to1 = out2 * currentIndex_;
    float out1 = generateOperatorOutput(0, mod2to1);
    
    return out1 * algorithmConfig_.carrierLevel;
}

float Classic4OpFMEngine::processStack432_1(const std::array<float, 4>& ops) {
    // Split stack: 4→(3,2)→1
    float mod4to3 = ops[3] * currentIndex_;
    float out3 = generateOperatorOutput(2, mod4to3);
    float out2 = generateOperatorOutput(1, mod4to3); // Same modulation as op3
    
    float mod32to1 = (out3 + out2) * 0.5f * currentIndex_;
    float out1 = generateOperatorOutput(0, mod32to1);
    
    return out1 * algorithmConfig_.carrierLevel;
}

float Classic4OpFMEngine::processParallel2x2(const std::array<float, 4>& ops) {
    // Two 2-op pairs: (4→3)+(2→1)
    float mod4to3 = ops[3] * currentIndex_;
    float out3 = generateOperatorOutput(2, mod4to3);
    
    float mod2to1 = ops[1] * currentIndex_;
    float out1 = generateOperatorOutput(0, mod2to1);
    
    return (out3 + out1) * 0.5f * algorithmConfig_.carrierLevel;
}

float Classic4OpFMEngine::processCrossMod(const std::array<float, 4>& ops) {
    // Cross modulation: 4⇄3, 2→1
    float mod4to3 = ops[3] * currentIndex_;
    float mod3to4 = ops[2] * currentIndex_ * 0.5f; // Reduced cross-mod
    
    float out3 = generateOperatorOutput(2, mod4to3 + mod3to4);
    float out4 = generateOperatorOutput(3, mod3to4 + mod4to3);
    
    float mod2to1 = ops[1] * currentIndex_;
    float out1 = generateOperatorOutput(0, mod2to1);
    
    return (out3 * 0.3f + out1 * 0.7f) * algorithmConfig_.carrierLevel;
}

float Classic4OpFMEngine::processRing4321(const std::array<float, 4>& ops) {
    // Ring: 4→3→2→1→4 (with feedback)
    float feedback = processFeedback(voiceState_.lastOutput, currentFeedback_);
    
    float mod4to3 = (ops[3] + feedback) * currentIndex_;
    float out3 = generateOperatorOutput(2, mod4to3);
    
    float mod3to2 = out3 * currentIndex_;
    float out2 = generateOperatorOutput(1, mod3to2);
    
    float mod2to1 = out2 * currentIndex_;
    float out1 = generateOperatorOutput(0, mod2to1);
    
    return out1 * algorithmConfig_.carrierLevel;
}

float Classic4OpFMEngine::processCascade42_31(const std::array<float, 4>& ops) {
    // Cascade: (4→2)+(3→1)
    float mod4to2 = ops[3] * currentIndex_;
    float out2 = generateOperatorOutput(1, mod4to2);
    
    float mod3to1 = ops[2] * currentIndex_;
    float out1 = generateOperatorOutput(0, mod3to1);
    
    return (out2 + out1) * 0.5f * algorithmConfig_.carrierLevel;
}

float Classic4OpFMEngine::processFeedbackPair(const std::array<float, 4>& ops) {
    // (4→3 with feedback)+(2→1)
    float feedback = processFeedback(ops[2], currentFeedback_);
    float mod4to3 = (ops[3] + feedback) * currentIndex_;
    float out3 = generateOperatorOutput(2, mod4to3);
    
    float mod2to1 = ops[1] * currentIndex_;
    float out1 = generateOperatorOutput(0, mod2to1);
    
    return (out3 * 0.4f + out1 * 0.6f) * algorithmConfig_.carrierLevel;
}

float Classic4OpFMEngine::processAllParallel(const std::array<float, 4>& ops) {
    // All parallel: 4+3+2+1
    float sum = 0.0f;
    for (int i = 0; i < NUM_OPERATORS; i++) {
        sum += generateOperatorOutput(i, 0.0f); // No modulation
    }
    return sum * 0.25f * algorithmConfig_.carrierLevel;
}

// Parameter mapping functions
float Classic4OpFMEngine::mapHarmonicsToIndex(float harmonics) const {
    // Exponential mapping: 0 → 0.0, 1 → MAX_FM_INDEX
    return harmonics * harmonics * MAX_FM_INDEX;
}

Classic4OpFMEngine::Algorithm Classic4OpFMEngine::selectAlgorithmFromTimbre(float timbre) const {
    // Map timbre to algorithm selection
    int algorithmIndex = static_cast<int>(timbre * 7.99f); // 0-7
    return static_cast<Algorithm>(algorithmIndex);
}

float Classic4OpFMEngine::mapMorphToFeedback(float morph) const {
    // Map morph to feedback with gentle curve
    return morph * morph * MAX_FEEDBACK;
}

void Classic4OpFMEngine::updateRatioSpread(float harmonics) {
    // Increase ratio spread for more complex harmonics
    float spread = 1.0f + harmonics * 2.0f;
    
    for (int i = 0; i < NUM_OPERATORS; i++) {
        float baseRatio = operatorConfigs_[i].ratio;
        float adjustedRatio = baseRatio * (1.0f + (i * 0.2f * spread));
        ratioSmoothers_[i].setTarget(adjustedRatio);
    }
}

void Classic4OpFMEngine::updateEnvelopeSpeeds(float morph) {
    float speedMultiplier = 1.0f + morph * 3.0f; // Up to 4x faster
    
    for (int i = 0; i < NUM_OPERATORS; i++) {
        float newAttack = envelopeConfigs_[i].attack / speedMultiplier;
        float newDecay = envelopeConfigs_[i].decay / speedMultiplier;
        float newRelease = envelopeConfigs_[i].release / speedMultiplier;
        
        envelopes_[i].setADSR(
            clamp(newAttack, 0.001f, 10.0f),
            clamp(newDecay, 0.001f, 10.0f),
            envelopeConfigs_[i].sustain,
            clamp(newRelease, 0.001f, 10.0f)
        );
    }
}

// Utility functions
float Classic4OpFMEngine::noteToFrequency(float note) const {
    return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f);
}

float Classic4OpFMEngine::ratioToFrequency(float ratio, float baseFreq) const {
    return baseFreq * ratio;
}

float Classic4OpFMEngine::centsToRatio(float cents) const {
    return std::pow(2.0f, cents / 1200.0f);
}

float Classic4OpFMEngine::dbToLinear(float db) const {
    return std::pow(10.0f, db / 20.0f);
}

float Classic4OpFMEngine::calculatePortamentoNote() {
    if (voiceState_.portamentoPhase >= 1.0f) {
        return voiceState_.targetNote;
    }
    return lerp(voiceState_.note, voiceState_.targetNote, voiceState_.portamentoPhase);
}

void Classic4OpFMEngine::updatePortamento(float deltaTimeMs) {
    if (voiceState_.portamentoPhase < 1.0f && globalConfig_.portamentoTime > 0.0f) {
        voiceState_.portamentoPhase += deltaTimeMs / globalConfig_.portamentoTime;
        voiceState_.portamentoPhase = clamp(voiceState_.portamentoPhase, 0.0f, 1.0f);
        
        voiceState_.note = calculatePortamentoNote();
        updateOperatorFrequencies();
    }
}

void Classic4OpFMEngine::switchAlgorithm(Algorithm newAlgorithm) {
    if (newAlgorithm == algorithmConfig_.algorithm) {
        return;
    }
    
    voiceState_.previousAlgorithm = algorithmConfig_.algorithm;
    algorithmConfig_.algorithm = newAlgorithm;
    voiceState_.algorithmSwitching = true;
    voiceState_.algorithmCrossfade = 0.0f;
    voiceState_.switchStartTime = getTimeMs();
}

void Classic4OpFMEngine::processAlgorithmCrossfade() {
    if (!voiceState_.algorithmSwitching) {
        return;
    }
    
    uint32_t elapsed = getTimeMs() - voiceState_.switchStartTime;
    float progress = elapsed / ALGORITHM_SWITCH_TIME_MS;
    
    if (progress >= 1.0f) {
        voiceState_.algorithmSwitching = false;
        voiceState_.algorithmCrossfade = 1.0f;
    } else {
        voiceState_.algorithmCrossfade = progress;
    }
}

void Classic4OpFMEngine::updateOperatorFrequencies() {
    float baseFreq = noteToFrequency(voiceState_.note);
    baseFreq *= std::pow(2.0f, voiceState_.pitchBend * 2.0f / 12.0f);
    
    for (int i = 0; i < NUM_OPERATORS; i++) {
        if (voiceState_.operatorActive[i]) {
            float freq = operatorConfigs_[i].fixedFreq ? 
                        operatorConfigs_[i].fixedFreqHz :
                        ratioToFrequency(currentRatios_[i], baseFreq);
            
            freq *= centsToRatio(operatorConfigs_[i].detune);
            
            voiceState_.operatorFreqs[i] = freq;
            if (initialized_) {
                operators_[i].setFrequency(freq);
            }
        }
    }
}

void Classic4OpFMEngine::updateOperatorLevels(float velocity) {
    float velocityNorm = velocity / 127.0f;
    
    for (int i = 0; i < NUM_OPERATORS; i++) {
        float baseLevel = operatorConfigs_[i].level;
        float velocityMod = 1.0f + (velocityNorm - 1.0f) * operatorConfigs_[i].velocitySensitivity;
        voiceState_.operatorLevels[i] = baseLevel * velocityMod;
        
        levelSmoothers_[i].setTarget(voiceState_.operatorLevels[i]);
    }
}

float Classic4OpFMEngine::processFeedback(float input, float feedbackAmount) {
    if (feedbackAmount <= 0.0f) {
        return 0.0f;
    }
    
    float feedback = voiceState_.feedbackSample * feedbackAmount;
    voiceState_.feedbackSample = input; // Update for next sample
    return feedback;
}

void Classic4OpFMEngine::updateFeedbackState(float output) {
    voiceState_.lastOutput = output;
}

std::pair<float, float> Classic4OpFMEngine::processEQ(float input) {
    float output = input;
    
    // Apply brightness (high-frequency emphasis)
    if (currentBrightness_ != 0.0f) {
        output = applyBrightness(output, currentBrightness_);
    }
    
    // Apply warmth (low-mid emphasis)
    if (globalConfig_.warmth != 0.0f) {
        output = applyWarmth(output, globalConfig_.warmth);
    }
    
    return {output, output};
}

float Classic4OpFMEngine::applyBrightness(float input, float brightness) {
    // Simple high-frequency emphasis
    static float lastInput = 0.0f;
    float highFreq = input - lastInput;
    lastInput = input;
    
    return input + highFreq * brightness * 0.5f;
}

float Classic4OpFMEngine::applyWarmth(float input, float warmth) {
    // Simple low-frequency emphasis
    static float lowFreq = 0.0f;
    lowFreq = lowFreq * 0.99f + input * 0.01f;
    
    return input + lowFreq * warmth * 0.3f;
}

float Classic4OpFMEngine::generateNoise() {
    // Simple pseudo-random noise generator
    noiseState_ = noiseState_ * 1664525 + 1013904223;
    return (static_cast<float>(noiseState_) / 4294967295.0f - 0.5f) * 2.0f;
}

void Classic4OpFMEngine::initializeAntiClick() {
    ramping_ = true;
    masterRamp_ = 0.0f;
    operatorRamps_.fill(0.0f);
}

void Classic4OpFMEngine::processAntiClick() {
    float rampSpeed = 1.0f / (ANTI_CLICK_TIME_MS * sampleRate_ * 0.001f);
    
    if (masterRamp_ < 1.0f) {
        masterRamp_ += rampSpeed;
        if (masterRamp_ >= 1.0f) {
            masterRamp_ = 1.0f;
            ramping_ = false;
        }
    }
}

void Classic4OpFMEngine::updateAnalogDrift() {
    // Subtle pitch drift simulation
    static float driftPhase = 0.0f;
    driftPhase += 0.001f; // Very slow drift
    
    float drift = std::sin(driftPhase) * globalConfig_.analogDrift * 0.01f; // ±1 cent max
    
    for (int i = 0; i < NUM_OPERATORS; i++) {
        if (voiceState_.operatorActive[i]) {
            float driftedFreq = voiceState_.operatorFreqs[i] * (1.0f + drift);
            operators_[i].setFrequency(driftedFreq);
        }
    }
}

uint32_t Classic4OpFMEngine::getTimeMs() const {
#ifdef STM32H7
    return HAL_GetTick();
#else
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
#endif
}

float Classic4OpFMEngine::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}

float Classic4OpFMEngine::clamp(float value, float min, float max) const {
    return std::max(min, std::min(max, value));
}

float Classic4OpFMEngine::crossfade(float a, float b, float mix) const {
    return lerp(a, b, mix);
}

float Classic4OpFMEngine::getCPUUsage() const {
    return cpuUsage_;
}

float Classic4OpFMEngine::getOperatorLevel(int operatorIndex) const {
    if (operatorIndex < 0 || operatorIndex >= NUM_OPERATORS) {
        return 0.0f;
    }
    return currentLevels_[operatorIndex];
}

float Classic4OpFMEngine::getOperatorFrequency(int operatorIndex) const {
    if (operatorIndex < 0 || operatorIndex >= NUM_OPERATORS) {
        return 0.0f;
    }
    return voiceState_.operatorFreqs[operatorIndex];
}

bool Classic4OpFMEngine::isOperatorActive(int operatorIndex) const {
    if (operatorIndex < 0 || operatorIndex >= NUM_OPERATORS) {
        return false;
    }
    return voiceState_.operatorActive[operatorIndex];
}

float Classic4OpFMEngine::getEnvelopeLevel(int operatorIndex) const {
    if (operatorIndex < 0 || operatorIndex >= NUM_OPERATORS || !initialized_) {
        return 0.0f;
    }
    return envelopes_[operatorIndex].getCurrentLevel();
}

void Classic4OpFMEngine::reset() {
    allNotesOff();
    
    // Reset all smoothers
    feedbackSmoother_.reset();
    indexSmoother_.reset();
    brightnessSmoother_.reset();
    
    for (int i = 0; i < NUM_OPERATORS; i++) {
        ratioSmoothers_[i].reset();
        levelSmoothers_[i].reset();
    }
    
    // Reset processing state
    voiceState_.feedbackSample = 0.0f;
    voiceState_.lastOutput = 0.0f;
    previousAlgorithmOutput_ = 0.0f;
    currentAlgorithmOutput_ = 0.0f;
    
    operatorRamps_.fill(1.0f);
    masterRamp_ = 1.0f;
    ramping_ = false;
    
    cpuUsage_ = 0.0f;
}

void Classic4OpFMEngine::setPreset(const std::string& presetName) {
    // Preset loading would be implemented here
    // For now, just reset to defaults
    reset();
}