#include "FMAntiClick.h"
#include <cmath>
#include <algorithm>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

FMAntiClick::FMAntiClick() {
    globalConfig_.rampType = RampType::ADAPTIVE;
    globalConfig_.minRampTimeMs = 0.5f;
    globalConfig_.maxRampTimeMs = 5.0f;
    globalConfig_.clickThreshold = 0.1f;
    globalConfig_.enablePhaseCorrection = true;
    globalConfig_.enableZeroCrossing = true;
    globalConfig_.adaptiveSpeed = 1.0f;
    globalConfig_.enableContentAnalysis = true;
    
    sampleRate_ = 44100.0f;
    numOperators_ = 4;
    initialized_ = false;
    analysisIndex_ = 0;
    cpuUsage_ = 0.0f;
    
    analysisBuffer_.fill(0.0f);
}

FMAntiClick::~FMAntiClick() {
    shutdown();
}

bool FMAntiClick::initialize(float sampleRate, int numOperators) {
    if (initialized_) {
        shutdown();
    }
    
    sampleRate_ = sampleRate;
    numOperators_ = numOperators;
    
    // Initialize operator states
    operatorStates_.resize(numOperators_);
    operatorEnabled_.resize(numOperators_, true);
    lastSamples_.resize(numOperators_, 0.0f);
    zeroCrossCounters_.resize(numOperators_, 0);
    signalEnergy_.resize(numOperators_, 0.0f);
    signalVariance_.resize(numOperators_, 0.0f);
    
    // Initialize operator states
    for (int i = 0; i < numOperators_; i++) {
        operatorStates_[i] = OperatorState();
        operatorEnabled_[i] = true;
    }
    
    initialized_ = true;
    return true;
}

void FMAntiClick::shutdown() {
    if (!initialized_) {
        return;
    }
    
    operatorStates_.clear();
    operatorEnabled_.clear();
    lastSamples_.clear();
    zeroCrossCounters_.clear();
    signalEnergy_.clear();
    signalVariance_.clear();
    
    initialized_ = false;
}

void FMAntiClick::setGlobalConfig(const GlobalConfig& config) {
    globalConfig_ = config;
    
    // Clamp values to valid ranges
    globalConfig_.minRampTimeMs = clamp(config.minRampTimeMs, 0.1f, 10.0f);
    globalConfig_.maxRampTimeMs = clamp(config.maxRampTimeMs, globalConfig_.minRampTimeMs, 50.0f);
    globalConfig_.clickThreshold = clamp(config.clickThreshold, 0.01f, 1.0f);
    globalConfig_.adaptiveSpeed = clamp(config.adaptiveSpeed, 0.1f, 5.0f);
}

void FMAntiClick::setOperatorEnabled(int operatorIndex, bool enabled) {
    if (operatorIndex >= 0 && operatorIndex < numOperators_) {
        operatorEnabled_[operatorIndex] = enabled;
    }
}

bool FMAntiClick::isOperatorEnabled(int operatorIndex) const {
    if (operatorIndex >= 0 && operatorIndex < numOperators_) {
        return operatorEnabled_[operatorIndex];
    }
    return false;
}

void FMAntiClick::onParameterChange(int operatorIndex, float oldValue, float newValue, float changeRate) {
    if (!initialized_ || operatorIndex < 0 || operatorIndex >= numOperators_ || !operatorEnabled_[operatorIndex]) {
        return;
    }
    
    float change = std::abs(newValue - oldValue);
    if (change < 0.001f) {
        return; // Negligible change
    }
    
    OperatorState& state = operatorStates_[operatorIndex];
    
    // Calculate parameter velocity
    state.parameterVelocity = calculateParameterVelocity(operatorIndex, change, 1.0f / changeRate);
    
    // Check if we need to start ramping
    if (shouldRampParameter(operatorIndex, change)) {
        float rampTime = calculateOptimalRampTime(operatorIndex, change);
        startRamp(operatorIndex, newValue, rampTime);
    }
}

void FMAntiClick::onFrequencyChange(int operatorIndex, float oldFreq, float newFreq) {
    if (!initialized_ || operatorIndex < 0 || operatorIndex >= numOperators_ || !operatorEnabled_[operatorIndex]) {
        return;
    }
    
    float freqRatio = (newFreq > 0.0f && oldFreq > 0.0f) ? newFreq / oldFreq : 1.0f;
    float change = std::abs(freqRatio - 1.0f);
    
    // Frequency changes can cause significant phase discontinuities
    onParameterChange(operatorIndex, oldFreq, newFreq, 1.0f);
    
    // Check if phase correction is needed
    if (globalConfig_.enablePhaseCorrection && change > 0.01f) {
        OperatorState& state = operatorStates_[operatorIndex];
        
        // Calculate expected phase jump
        float phaseJump = state.lastPhase * (freqRatio - 1.0f);
        
        if (needsPhaseCorrection(phaseJump)) {
            calculatePhaseCorrection(operatorIndex, state.lastPhase, state.lastPhase + phaseJump);
        }
    }
}

void FMAntiClick::onLevelChange(int operatorIndex, float oldLevel, float newLevel) {
    if (!initialized_ || operatorIndex < 0 || operatorIndex >= numOperators_ || !operatorEnabled_[operatorIndex]) {
        return;
    }
    
    float levelChange = std::abs(newLevel - oldLevel);
    
    // Level changes are less critical but still need smoothing for large jumps
    if (levelChange > globalConfig_.clickThreshold) {
        onParameterChange(operatorIndex, oldLevel, newLevel, 2.0f); // Faster ramp for level
    }
}

void FMAntiClick::onPhaseReset(int operatorIndex, float newPhase) {
    if (!initialized_ || operatorIndex < 0 || operatorIndex >= numOperators_ || !operatorEnabled_[operatorIndex]) {
        return;
    }
    
    OperatorState& state = operatorStates_[operatorIndex];
    
    if (globalConfig_.enablePhaseCorrection) {
        calculatePhaseCorrection(operatorIndex, state.lastPhase, newPhase);
    }
    
    state.lastPhase = newPhase;
}

float FMAntiClick::processOperatorSample(int operatorIndex, float input, float currentPhase) {
    if (!initialized_ || operatorIndex < 0 || operatorIndex >= numOperators_ || !operatorEnabled_[operatorIndex]) {
        return input;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    OperatorState& state = operatorStates_[operatorIndex];
    float output = input;
    
    // Update phase tracking
    state.lastPhase = currentPhase;
    
    // Apply content analysis if enabled
    if (globalConfig_.enableContentAnalysis) {
        analyzeSignalContent(operatorIndex, input);
    }
    
    // Process ramping if active
    if (state.ramping) {
        switch (globalConfig_.rampType) {
            case RampType::LINEAR:
                output = processLinearRamp(operatorIndex, input);
                break;
            case RampType::EXPONENTIAL:
                output = processExponentialRamp(operatorIndex, input);
                break;
            case RampType::ZERO_CROSS:
                output = processZeroCrossRamp(operatorIndex, input);
                break;
            case RampType::ADAPTIVE:
                output = processAdaptiveRamp(operatorIndex, input);
                break;
        }
        
        updateRampProgress(operatorIndex);
    }
    
    // Apply phase correction if active
    if (state.phaseCorrectActive) {
        output = applyPhaseCorrection(operatorIndex, output, currentPhase);
    }
    
    // Update last output for next iteration
    state.lastOutput = output;
    lastSamples_[operatorIndex] = output;
    
    // Update zero-crossing detection
    if (globalConfig_.enableZeroCrossing) {
        updateZeroCrossCounter(operatorIndex);
    }
    
    // Update CPU usage
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
    float processingTimeMs = duration.count() * 1e-6f;
    cpuUsage_ = cpuUsage_ * CPU_USAGE_SMOOTH + processingTimeMs * (1.0f - CPU_USAGE_SMOOTH);
    
    return output;
}

void FMAntiClick::processOperatorBlock(int operatorIndex, const float* input, float* output, 
                                      const float* phases, int numSamples) {
    if (!initialized_ || operatorIndex < 0 || operatorIndex >= numOperators_ || !operatorEnabled_[operatorIndex]) {
        for (int i = 0; i < numSamples; i++) {
            output[i] = input[i];
        }
        return;
    }
    
    for (int i = 0; i < numSamples; i++) {
        float phase = (phases != nullptr) ? phases[i] : 0.0f;
        output[i] = processOperatorSample(operatorIndex, input[i], phase);
    }
}

bool FMAntiClick::isRamping(int operatorIndex) const {
    if (operatorIndex >= 0 && operatorIndex < numOperators_) {
        return operatorStates_[operatorIndex].ramping;
    }
    return false;
}

float FMAntiClick::getRampProgress(int operatorIndex) const {
    if (operatorIndex >= 0 && operatorIndex < numOperators_) {
        return operatorStates_[operatorIndex].rampProgress;
    }
    return 1.0f;
}

bool FMAntiClick::hasClickPotential(int operatorIndex, float newValue, float oldValue) const {
    if (operatorIndex < 0 || operatorIndex >= numOperators_) {
        return false;
    }
    
    float change = std::abs(newValue - oldValue);
    float probability = getClickProbability(operatorIndex, change);
    
    return probability > globalConfig_.clickThreshold;
}

// Private method implementations

float FMAntiClick::processLinearRamp(int operatorIndex, float input) {
    const OperatorState& state = operatorStates_[operatorIndex];
    float rampValue = linearInterpolate(0.0f, state.rampTarget, state.rampProgress);
    return input * rampValue;
}

float FMAntiClick::processExponentialRamp(int operatorIndex, float input) {
    const OperatorState& state = operatorStates_[operatorIndex];
    float rampValue = exponentialInterpolate(0.0f, state.rampTarget, state.rampProgress);
    return input * rampValue;
}

float FMAntiClick::processZeroCrossRamp(int operatorIndex, float input) {
    OperatorState& state = operatorStates_[operatorIndex];
    
    if (state.zeroCrossCountdown > 0) {
        state.zeroCrossCountdown--;
        
        // Check for zero crossing
        if (detectZeroCrossing(operatorIndex, input)) {
            state.zeroCrossCountdown = 0; // Start ramping immediately
        } else if (state.zeroCrossCountdown == 0) {
            // Timeout - start ramping anyway
        }
        
        return input; // No ramping until zero crossing or timeout
    }
    
    // Normal exponential ramp after zero crossing
    return processExponentialRamp(operatorIndex, input);
}

float FMAntiClick::processAdaptiveRamp(int operatorIndex, float input) {
    const OperatorState& state = operatorStates_[operatorIndex];
    
    // Analyze signal complexity to choose ramp type
    float complexity = calculateSignalComplexity(operatorIndex);
    
    if (complexity < 0.3f) {
        // Simple signal - use zero crossing
        return processZeroCrossRamp(operatorIndex, input);
    } else if (complexity < 0.7f) {
        // Medium complexity - use exponential
        return processExponentialRamp(operatorIndex, input);
    } else {
        // Complex signal - use linear for predictability
        return processLinearRamp(operatorIndex, input);
    }
}

bool FMAntiClick::detectZeroCrossing(int operatorIndex, float currentSample) {
    if (operatorIndex < 0 || operatorIndex >= numOperators_) {
        return false;
    }
    
    float lastSample = lastSamples_[operatorIndex];
    bool zeroCrossing = (lastSample > 0.0f && currentSample <= 0.0f) ||
                       (lastSample < 0.0f && currentSample >= 0.0f);
    
    return zeroCrossing;
}

void FMAntiClick::updateZeroCrossCounter(int operatorIndex) {
    if (operatorIndex >= 0 && operatorIndex < numOperators_) {
        OperatorState& state = operatorStates_[operatorIndex];
        
        if (state.zeroCrossCountdown > 0) {
            state.zeroCrossCountdown--;
        }
    }
}

void FMAntiClick::calculatePhaseCorrection(int operatorIndex, float oldPhase, float newPhase) {
    if (operatorIndex < 0 || operatorIndex >= numOperators_) {
        return;
    }
    
    OperatorState& state = operatorStates_[operatorIndex];
    
    float phaseJump = newPhase - oldPhase;
    
    // Normalize phase jump to [-π, π]
    while (phaseJump > M_PI) phaseJump -= 2.0f * M_PI;
    while (phaseJump < -M_PI) phaseJump += 2.0f * M_PI;
    
    if (needsPhaseCorrection(phaseJump)) {
        state.phaseCorrection = phaseJump;
        state.phaseCorrectActive = true;
        state.targetPhase = newPhase;
    }
}

float FMAntiClick::applyPhaseCorrection(int operatorIndex, float input, float phase) {
    if (operatorIndex < 0 || operatorIndex >= numOperators_) {
        return input;
    }
    
    OperatorState& state = operatorStates_[operatorIndex];
    
    if (!state.phaseCorrectActive) {
        return input;
    }
    
    // Gradually apply phase correction over multiple samples
    float correctionAmount = state.phaseCorrection * 0.1f; // Apply 10% per sample
    state.phaseCorrection -= correctionAmount;
    
    // Generate corrected phase
    float correctedPhase = phase + correctionAmount;
    float correctedSample = std::sin(correctedPhase) * std::abs(input);
    
    // Check if correction is complete
    if (std::abs(state.phaseCorrection) < 0.01f) {
        state.phaseCorrectActive = false;
        state.phaseCorrection = 0.0f;
    }
    
    return correctedSample;
}

bool FMAntiClick::needsPhaseCorrection(float phaseJump) const {
    return std::abs(phaseJump) > PHASE_JUMP_THRESHOLD;
}

void FMAntiClick::analyzeSignalContent(int operatorIndex, float sample) {
    if (operatorIndex < 0 || operatorIndex >= numOperators_) {
        return;
    }
    
    // Add to analysis buffer
    analysisBuffer_[analysisIndex_] = sample;
    analysisIndex_ = (analysisIndex_ + 1) % analysisBuffer_.size();
    
    // Calculate signal energy (RMS)
    float energy = 0.0f;
    for (float s : analysisBuffer_) {
        energy += s * s;
    }
    energy = std::sqrt(energy / analysisBuffer_.size());
    
    // Update smoothed energy
    signalEnergy_[operatorIndex] = signalEnergy_[operatorIndex] * 0.9f + energy * 0.1f;
    
    // Calculate variance for complexity measure
    float mean = 0.0f;
    for (float s : analysisBuffer_) {
        mean += s;
    }
    mean /= analysisBuffer_.size();
    
    float variance = 0.0f;
    for (float s : analysisBuffer_) {
        float diff = s - mean;
        variance += diff * diff;
    }
    variance /= analysisBuffer_.size();
    
    // Update smoothed variance
    signalVariance_[operatorIndex] = signalVariance_[operatorIndex] * 0.9f + variance * 0.1f;
}

float FMAntiClick::calculateSignalComplexity(int operatorIndex) const {
    if (operatorIndex < 0 || operatorIndex >= numOperators_) {
        return 0.5f;
    }
    
    float energy = signalEnergy_[operatorIndex];
    float variance = signalVariance_[operatorIndex];
    
    // Simple complexity measure based on variance relative to energy
    float complexity = (energy > 0.001f) ? variance / (energy * energy) : 0.0f;
    return clamp(complexity, 0.0f, 1.0f);
}

float FMAntiClick::getAdaptiveRampTime(int operatorIndex, float complexity) const {
    float baseTime = globalConfig_.minRampTimeMs;
    float additionalTime = (globalConfig_.maxRampTimeMs - globalConfig_.minRampTimeMs) * complexity;
    
    return (baseTime + additionalTime) / globalConfig_.adaptiveSpeed;
}

float FMAntiClick::calculateParameterVelocity(int operatorIndex, float change, float timeMs) {
    if (operatorIndex < 0 || operatorIndex >= numOperators_ || timeMs <= 0.0f) {
        return 0.0f;
    }
    
    float velocity = change / timeMs;
    
    // Smooth parameter velocity
    OperatorState& state = operatorStates_[operatorIndex];
    state.parameterVelocity = state.parameterVelocity * PARAMETER_VELOCITY_SMOOTH + 
                             velocity * (1.0f - PARAMETER_VELOCITY_SMOOTH);
    
    return state.parameterVelocity;
}

bool FMAntiClick::isParameterChangeSignificant(float change, float velocity) const {
    return (change > globalConfig_.clickThreshold) || (velocity > globalConfig_.clickThreshold * 10.0f);
}

float FMAntiClick::getClickProbability(int operatorIndex, float parameterChange) const {
    if (operatorIndex < 0 || operatorIndex >= numOperators_) {
        return 0.0f;
    }
    
    const OperatorState& state = operatorStates_[operatorIndex];
    
    // Base probability from parameter change magnitude
    float baseProbability = clamp(parameterChange / globalConfig_.clickThreshold, 0.0f, 1.0f);
    
    // Increase probability based on parameter velocity
    float velocityFactor = clamp(state.parameterVelocity * 0.1f, 0.0f, 0.5f);
    
    // Increase probability based on signal complexity
    float complexity = calculateSignalComplexity(operatorIndex);
    float complexityFactor = complexity * 0.3f;
    
    return clamp(baseProbability + velocityFactor + complexityFactor, 0.0f, 1.0f);
}

bool FMAntiClick::shouldRampParameter(int operatorIndex, float parameterChange) const {
    if (operatorIndex < 0 || operatorIndex >= numOperators_) {
        return false;
    }
    
    const OperatorState& state = operatorStates_[operatorIndex];
    return isParameterChangeSignificant(parameterChange, state.parameterVelocity);
}

float FMAntiClick::calculateOptimalRampTime(int operatorIndex, float parameterChange) const {
    if (operatorIndex < 0 || operatorIndex >= numOperators_) {
        return globalConfig_.minRampTimeMs;
    }
    
    // Base ramp time proportional to parameter change
    float baseTime = globalConfig_.minRampTimeMs + 
                    (globalConfig_.maxRampTimeMs - globalConfig_.minRampTimeMs) * 
                    clamp(parameterChange / globalConfig_.clickThreshold, 0.0f, 1.0f);
    
    // Adjust for signal complexity if content analysis is enabled
    if (globalConfig_.enableContentAnalysis) {
        float complexity = calculateSignalComplexity(operatorIndex);
        baseTime = getAdaptiveRampTime(operatorIndex, complexity);
    }
    
    return baseTime;
}

void FMAntiClick::startRamp(int operatorIndex, float targetLevel, float rampTimeMs) {
    if (operatorIndex < 0 || operatorIndex >= numOperators_) {
        return;
    }
    
    OperatorState& state = operatorStates_[operatorIndex];
    
    state.ramping = true;
    state.rampProgress = 0.0f;
    state.rampTarget = targetLevel;
    
    // Set zero-cross countdown if using zero-crossing ramping
    if (globalConfig_.rampType == RampType::ZERO_CROSS && globalConfig_.enableZeroCrossing) {
        state.zeroCrossCountdown = static_cast<int>((ZERO_CROSS_TIMEOUT_MS / 1000.0f) * sampleRate_);
    }
}

void FMAntiClick::updateRampProgress(int operatorIndex) {
    if (operatorIndex < 0 || operatorIndex >= numOperators_) {
        return;
    }
    
    OperatorState& state = operatorStates_[operatorIndex];
    
    if (!state.ramping) {
        return;
    }
    
    // Calculate progress increment (this would normally be based on ramp time)
    float progressIncrement = 1.0f / (globalConfig_.maxRampTimeMs * sampleRate_ * 0.001f);
    state.rampProgress += progressIncrement;
    
    // Check if ramping is complete
    if (state.rampProgress >= 1.0f) {
        state.rampProgress = 1.0f;
        state.ramping = false;
        state.zeroCrossCountdown = 0;
    }
}

float FMAntiClick::calculateRampValue(int operatorIndex, RampType type) const {
    if (operatorIndex < 0 || operatorIndex >= numOperators_) {
        return 1.0f;
    }
    
    const OperatorState& state = operatorStates_[operatorIndex];
    
    switch (type) {
        case RampType::LINEAR:
            return state.rampProgress;
        case RampType::EXPONENTIAL:
            return 1.0f - std::exp(-EXP_CURVE_FACTOR * state.rampProgress);
        case RampType::ZERO_CROSS:
        case RampType::ADAPTIVE:
            return exponentialInterpolate(0.0f, 1.0f, state.rampProgress);
        default:
            return state.rampProgress;
    }
}

// Utility function implementations

float FMAntiClick::linearInterpolate(float from, float to, float progress) const {
    return from + progress * (to - from);
}

float FMAntiClick::exponentialInterpolate(float from, float to, float progress) const {
    float expProgress = 1.0f - std::exp(-EXP_CURVE_FACTOR * progress);
    return from + expProgress * (to - from);
}

float FMAntiClick::clamp(float value, float min, float max) const {
    return std::max(min, std::min(max, value));
}

float FMAntiClick::dbToLinear(float db) const {
    return std::pow(10.0f, db / 20.0f);
}

float FMAntiClick::linearToDb(float linear) const {
    return 20.0f * std::log10(std::max(linear, 1e-6f));
}