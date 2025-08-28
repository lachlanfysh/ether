#include "TapeEffectsProcessor.h"
#include <algorithm>
#include <cstring>

TapeEffectsProcessor::TapeEffectsProcessor() {
    // Initialize default configuration
    config_ = TapeConfig();
    
    // Generate lookup tables
    generateSaturationLUT();
    generateHarmonicLUT();
    
    // Initialize presets
    initializePresets();
    
    // Initialize filters
    state_.lowShelfFilter.setSampleRate(sampleRate_);
    state_.highShelfFilter.setSampleRate(sampleRate_);
    state_.presenceFilter.setSampleRate(sampleRate_);
    
    // Set up initial filter parameters
    updateFrequencyFilters();
    
    // Initialize harmonic generators
    for (int i = 0; i < 8; i++) {
        state_.harmonicGains[i] = 1.0f / (i + 2);  // Decreasing harmonic levels
    }
    
    reset();
}

void TapeEffectsProcessor::setTapeConfig(const TapeConfig& config) {
    config_ = config;
    updateFrequencyFilters();
    
    // Update smoothing parameters
    state_.saturationSmoothing.setSmoothing(1.0f);
    state_.attackSmoothing.setSmoothing(config_.attackTime);
    state_.releaseSmoothing.setSmoothing(config_.releaseTime);
}

void TapeEffectsProcessor::setTapeType(TapeType type) {
    config_.machineType = type;
    updateFrequencyFilters();
}

void TapeEffectsProcessor::setTapeMaterial(TapeMaterial material) {
    config_.material = material;
    updateFrequencyFilters();
}

void TapeEffectsProcessor::setTapeSpeed(TapeSpeed speed) {
    config_.speed = speed;
    updateFrequencyFilters();
}

void TapeEffectsProcessor::setSaturationAmount(float amount) {
    config_.saturationAmount = std::max(0.0f, std::min(amount, 1.0f));
}

void TapeEffectsProcessor::setCompressionAmount(float amount) {
    config_.compressionAmount = std::max(0.0f, std::min(amount, 1.0f));
}

void TapeEffectsProcessor::setWetDryMix(float mix) {
    config_.wetDryMix = std::max(0.0f, std::min(mix, 1.0f));
}

void TapeEffectsProcessor::setBypassed(bool bypassed) {
    bypassed_ = bypassed;
}

float TapeEffectsProcessor::processSample(float input) {
    if (bypassed_ || !config_.bypassable) {
        return input;
    }
    
    float processed = input;
    
    // 1. Input gain and bias
    processed = calculateBiasEffect(processed);
    
    // 2. Saturation processing
    processed = processSaturation(processed);
    
    // 3. Compression processing
    processed = processCompression(processed, state_.compressorEnvelope);
    
    // 4. Frequency response modeling
    processed = processFrequencyResponse(processed);
    
    // 5. Harmonic generation
    processed = processHarmonicGeneration(processed, input);
    
    // 6. Modulation (wow/flutter)
    processed = processModulation(processed);
    
    // 7. Advanced modeling (hysteresis, print-through)
    processed = processAdvancedModeling(processed);
    
    // 8. DC blocking
    processed = state_.dcBlocker.process(processed);
    
    // 9. Wet/dry mix
    float output = input * (1.0f - config_.wetDryMix) + processed * config_.wetDryMix;
    
    return output;
}

void TapeEffectsProcessor::processBlock(const float* input, float* output, int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        output[i] = processSample(input[i]);
    }
}

void TapeEffectsProcessor::processStereo(const float* inputL, const float* inputR, 
                                       float* outputL, float* outputR, int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        outputL[i] = processSample(inputL[i]);
        outputR[i] = processSample(inputR[i]);
    }
}

float TapeEffectsProcessor::processSaturation(float input) {
    float amount = config_.saturationAmount;
    if (amount <= 0.0f) {
        return input;
    }
    
    // Apply appropriate saturation algorithm based on tape type
    float saturated = input;
    
    switch (config_.machineType) {
        case TapeType::VINTAGE_TUBE:
            saturated = vintageeTubeSaturation(input, amount);
            break;
            
        case TapeType::MODERN_SOLID:
            saturated = modernSolidStateSaturation(input, amount);
            break;
            
        case TapeType::VINTAGE_TRANSISTOR:
            saturated = transistorSaturation(input, amount);
            break;
            
        case TapeType::EXOTIC_DIGITAL:
            saturated = digitalTapeSaturation(input, amount);
            break;
            
        case TapeType::CUSTOM:
            // Use lookup table for custom saturation
            float index = (input + 1.0f) * 0.5f * (SATURATION_TABLE_SIZE - 1);
            saturated = interpolateTable(saturationLUT_, index) * amount + input * (1.0f - amount);
            break;
    }
    
    // Apply asymmetry
    if (config_.saturationAsymmetry > 0.0f) {
        if (saturated > 0.0f) {
            saturated *= (1.0f + config_.saturationAsymmetry);
        } else {
            saturated *= (1.0f - config_.saturationAsymmetry * 0.5f);
        }
    }
    
    // Smooth the saturation to avoid artifacts
    state_.saturationSmoothing.setTarget(saturated);
    return state_.saturationSmoothing.process();
}

float TapeEffectsProcessor::vintageeTubeSaturation(float input, float amount) {
    // Model vintage tube-based tape machine saturation
    float drive = 1.0f + amount * 3.0f;
    float driven = input * drive;
    
    // Tube-like asymmetric saturation using tanh with bias
    float bias = 0.1f * amount;
    float saturated = std::tanh(driven + bias) - std::tanh(bias);
    
    // Add even harmonic content typical of tubes
    float even_harmonic = std::sin(driven * PI) * amount * 0.1f;
    
    return saturated + even_harmonic;
}

float TapeEffectsProcessor::modernSolidStateSaturation(float input, float amount) {
    // Model modern solid-state tape machine saturation
    float drive = 1.0f + amount * 2.0f;
    float driven = input * drive;
    
    // Cleaner, more symmetric saturation
    float saturated;
    if (std::abs(driven) < 0.7f) {
        saturated = driven;  // Linear region
    } else {
        // Soft clipping region
        saturated = std::copysign(0.7f + (std::abs(driven) - 0.7f) * 0.3f, driven);
    }
    
    return saturated * (1.0f / drive);  // Compensate for drive gain
}

float TapeEffectsProcessor::transistorSaturation(float input, float amount) {
    // Model 70s/80s transistor-based tape machine saturation
    float drive = 1.0f + amount * 2.5f;
    float driven = input * drive;
    
    // Transistor-like clipping with some asymmetry
    float saturated;
    if (driven > 0.8f) {
        saturated = 0.8f + (driven - 0.8f) * 0.2f;  // Soft clip positive
    } else if (driven < -0.7f) {
        saturated = -0.7f + (driven + 0.7f) * 0.3f;  // Slightly softer clip negative
    } else {
        saturated = driven;
    }
    
    // Add some odd harmonic content
    float odd_harmonic = std::sin(driven * PI * 3.0f) * amount * 0.05f;
    
    return (saturated + odd_harmonic) * (1.0f / drive);
}

float TapeEffectsProcessor::digitalTapeSaturation(float input, float amount) {
    // Model digital tape simulation with artifacts
    float drive = 1.0f + amount * 4.0f;
    float driven = input * drive;
    
    // Digital-style hard clipping with aliasing simulation
    float saturated = std::max(-1.0f, std::min(driven, 1.0f));
    
    // Add some aliasing-like artifacts
    if (std::abs(driven) > 0.9f) {
        float alias = std::sin(driven * PI * 7.0f) * amount * 0.1f;
        saturated += alias;
    }
    
    return saturated * (1.0f / drive);
}

float TapeEffectsProcessor::processCompression(float input, float& envelope) {
    if (config_.compressionAmount <= 0.0f) {
        return input;
    }
    
    // Calculate compression gain
    float gain = calculateCompressionGain(input, envelope);
    
    // Update envelope
    updateCompressionEnvelope(input, envelope);
    
    // Apply gain reduction
    float compressed = input * gain;
    
    // Store gain reduction for metering
    state_.gainReduction = linearToDb(gain);
    
    return compressed;
}

float TapeEffectsProcessor::calculateCompressionGain(float input, float envelope) {
    float threshold = 0.7f - config_.compressionAmount * 0.4f;  // Adaptive threshold
    float ratio = config_.compressionRatio;
    
    float inputLevel = std::abs(input);
    
    if (inputLevel <= threshold) {
        return 1.0f;  // No compression
    }
    
    // Calculate compression
    float excess = inputLevel - threshold;
    float compressedExcess = excess / ratio;
    float targetLevel = threshold + compressedExcess;
    
    return inputLevel > 0.0f ? targetLevel / inputLevel : 1.0f;
}

void TapeEffectsProcessor::updateCompressionEnvelope(float input, float& envelope) {
    float inputLevel = std::abs(input);
    
    // Program-dependent timing
    float attackTime = config_.attackTime;
    float releaseTime = config_.releaseTime;
    
    if (config_.programDependentTiming) {
        // Faster attack for transients, slower for sustained material
        float transient = std::abs(inputLevel - envelope);
        attackTime *= (1.0f - transient * 0.8f);
        releaseTime *= (1.0f + transient * 0.5f);
    }
    
    // Update envelope with adaptive timing
    if (inputLevel > envelope) {
        // Attack
        float attackCoeff = std::exp(-1.0f / (attackTime * sampleRate_ * 0.001f));
        envelope = inputLevel + (envelope - inputLevel) * attackCoeff;
    } else {
        // Release
        float releaseCoeff = std::exp(-1.0f / (releaseTime * sampleRate_ * 0.001f));
        envelope = inputLevel + (envelope - inputLevel) * releaseCoeff;
    }
}

float TapeEffectsProcessor::processFrequencyResponse(float input) {
    // Apply tape-characteristic frequency response
    float processed = input;
    
    // Low-frequency boost (tape warmth)
    state_.lowShelfFilter.setCutoff(100.0f);
    state_.lowShelfFilter.setMode(DSP::SVF::LP);
    processed = state_.lowShelfFilter.process(processed);
    processed = input + (processed - input) * config_.lowFreqBoost;
    
    // High-frequency rolloff (tape head characteristics)
    state_.highShelfFilter.setCutoff(8000.0f - config_.highFreqRolloff * 3000.0f);
    state_.highShelfFilter.setMode(DSP::SVF::LP);
    processed = state_.highShelfFilter.process(processed);
    
    // Mid-frequency presence (tape machine character)
    state_.presenceFilter.setCutoff(2000.0f);
    state_.presenceFilter.setMode(DSP::SVF::BP);
    state_.presenceFilter.setResonance(0.3f);
    float presence = state_.presenceFilter.process(processed);
    processed += presence * config_.midFreqColoration;
    
    return processed;
}

float TapeEffectsProcessor::processModulation(float input) {
    if (config_.wowAmount <= 0.0f && config_.flutterAmount <= 0.0f) {
        return input;
    }
    
    updateModulationOscillators();
    
    // Calculate pitch modulation
    float wowMod = std::sin(state_.wowPhase) * config_.wowAmount * 0.01f;
    float flutterMod = std::sin(state_.flutterPhase) * config_.flutterAmount * 0.005f;
    float totalMod = wowMod + flutterMod;
    
    // Apply modulation using delay line
    float delaySamples = totalMod * sampleRate_ * 0.001f;  // Convert to samples
    
    // Write to delay buffer
    state_.delayBuffer[state_.delayWritePtr] = input;
    state_.delayWritePtr = (state_.delayWritePtr + 1) % state_.delayBuffer.size();
    
    // Read from delay buffer with interpolation
    float readPos = state_.delayWritePtr - delaySamples - 1.0f;
    while (readPos < 0) readPos += state_.delayBuffer.size();
    
    int readIdx = static_cast<int>(readPos) % state_.delayBuffer.size();
    int nextIdx = (readIdx + 1) % state_.delayBuffer.size();
    float frac = readPos - std::floor(readPos);
    
    return DSP::Interp::linear(state_.delayBuffer[readIdx], state_.delayBuffer[nextIdx], frac);
}

float TapeEffectsProcessor::processHarmonicGeneration(float input, float fundamental) {
    if (config_.harmonicContent <= 0.0f) {
        return input;
    }
    
    updateHarmonicGenerators(fundamental);
    
    float harmonics = 0.0f;
    
    // Generate even and odd harmonics
    harmonics += calculateEvenHarmonics(input);
    harmonics += calculateOddHarmonics(input);
    
    return input + harmonics * config_.harmonicContent;
}

float TapeEffectsProcessor::calculateEvenHarmonics(float input) {
    float evenHarmonics = 0.0f;
    
    // 2nd, 4th, 6th harmonics (typical of tube saturation)
    for (int i = 0; i < 3; i++) {
        int harmonic = (i + 1) * 2;  // 2, 4, 6
        float phase = state_.harmonicPhases[i];
        float gain = state_.harmonicGains[i];
        
        evenHarmonics += std::sin(phase * harmonic) * gain * 0.1f;
    }
    
    return evenHarmonics;
}

float TapeEffectsProcessor::calculateOddHarmonics(float input) {
    float oddHarmonics = 0.0f;
    
    // 3rd, 5th, 7th harmonics (typical of transistor saturation)
    for (int i = 0; i < 3; i++) {
        int harmonic = i * 2 + 3;  // 3, 5, 7
        float phase = state_.harmonicPhases[i + 3];
        float gain = state_.harmonicGains[i + 3];
        
        oddHarmonics += std::sin(phase * harmonic) * gain * 0.05f;
    }
    
    return oddHarmonics;
}

float TapeEffectsProcessor::processAdvancedModeling(float input) {
    float processed = input;
    
    // Hysteresis modeling
    processed = processHysteresis(processed);
    
    // Print-through modeling
    processed = processPrintThrough(processed);
    
    // Tape noise
    if (config_.noiseFloor > -80.0f) {
        float noise = generateTapeNoise();
        processed += noise * dbToLinear(config_.noiseFloor);
    }
    
    // Dropout simulation
    if (shouldGenerateDropout()) {
        processed *= 0.1f;  // Severe level drop during dropout
    }
    
    return processed;
}

float TapeEffectsProcessor::processHysteresis(float input) {
    // Simple hysteresis model
    float hysteresis = config_.hysteresis;
    if (hysteresis <= 0.0f) {
        return input;
    }
    
    float delta = input - state_.hysteresisHistory;
    float processed = input - delta * hysteresis * 0.1f;
    
    state_.hysteresisHistory = processed;
    return processed;
}

float TapeEffectsProcessor::processPrintThrough(float input) {
    // Simple print-through model (echo from adjacent tape layers)
    if (config_.printThrough <= 0.0f) {
        return input;
    }
    
    float delayedSignal = state_.printThroughDelay * 0.9f + input * 0.1f;
    state_.printThroughDelay = delayedSignal;
    
    return input + delayedSignal * config_.printThrough * 0.1f;
}

void TapeEffectsProcessor::updateModulationOscillators() {
    float wowIncrement = WOW_FREQUENCY * TWO_PI / sampleRate_;
    float flutterIncrement = FLUTTER_FREQUENCY * TWO_PI / sampleRate_;
    
    state_.wowPhase += wowIncrement;
    state_.flutterPhase += flutterIncrement;
    
    if (state_.wowPhase > TWO_PI) state_.wowPhase -= TWO_PI;
    if (state_.flutterPhase > TWO_PI) state_.flutterPhase -= TWO_PI;
}

void TapeEffectsProcessor::updateHarmonicGenerators(float fundamental) {
    // Update harmonic oscillator phases based on fundamental
    float fundamentalPhase = fundamental * TWO_PI;
    
    for (int i = 0; i < 8; i++) {
        state_.harmonicPhases[i] = fundamentalPhase;
    }
}

float TapeEffectsProcessor::generateTapeNoise() {
    // Generate tape-characteristic noise (pink-ish noise)
    float noise = state_.noiseGenerator.normal(0.0f, 1.0f);
    
    // Simple pink noise approximation
    static float pinkState = 0.0f;
    pinkState = pinkState * 0.95f + noise * 0.05f;
    
    return pinkState * 0.1f;
}

bool TapeEffectsProcessor::shouldGenerateDropout() {
    // Simple dropout simulation
    state_.dropoutTimer++;
    
    if (!state_.inDropout) {
        float dropoutProbability = config_.dropoutRate / sampleRate_;
        if (state_.noiseGenerator.uniform() < dropoutProbability) {
            state_.inDropout = true;
            state_.dropoutTimer = 0;
            return true;
        }
    } else {
        // End dropout after minimum duration
        float dropoutDuration = DROPOUT_MIN_DURATION * sampleRate_;
        if (state_.dropoutTimer > dropoutDuration) {
            state_.inDropout = false;
        }
        return true;
    }
    
    return false;
}

float TapeEffectsProcessor::calculateBiasEffect(float input) {
    // Tape bias affects saturation characteristics
    float bias = config_.biasLevel;
    float biasedInput = input + bias * 0.1f;  // Small DC bias
    
    // Bias affects the saturation curve
    return biasedInput;
}

void TapeEffectsProcessor::updateFrequencyFilters() {
    // Update filters based on tape type and material
    state_.lowShelfFilter.setSampleRate(sampleRate_);
    state_.highShelfFilter.setSampleRate(sampleRate_);
    state_.presenceFilter.setSampleRate(sampleRate_);
}

float TapeEffectsProcessor::getSaturationAmount() const {
    return std::abs(state_.lastSaturationOutput - state_.lastSaturationInput);
}

float TapeEffectsProcessor::getCompressionReduction() const {
    return state_.gainReduction;
}

void TapeEffectsProcessor::reset() {
    state_ = TapeState();
    state_.saturationSmoothing.setSampleRate(sampleRate_);
    state_.attackSmoothing.setSampleRate(sampleRate_);
    state_.releaseSmoothing.setSampleRate(sampleRate_);
}

void TapeEffectsProcessor::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    state_.lowShelfFilter.setSampleRate(sampleRate);
    state_.highShelfFilter.setSampleRate(sampleRate);
    state_.presenceFilter.setSampleRate(sampleRate);
    state_.saturationSmoothing.setSampleRate(sampleRate);
    state_.attackSmoothing.setSampleRate(sampleRate);
    state_.releaseSmoothing.setSampleRate(sampleRate);
}

void TapeEffectsProcessor::generateSaturationLUT() {
    for (int i = 0; i < SATURATION_TABLE_SIZE; i++) {
        float x = (static_cast<float>(i) / (SATURATION_TABLE_SIZE - 1)) * 2.0f - 1.0f;
        
        // Custom saturation curve (sigmoid-like)
        saturationLUT_[i] = std::tanh(x * 2.0f) * 0.8f;
    }
}

void TapeEffectsProcessor::generateHarmonicLUT() {
    for (int i = 0; i < SATURATION_TABLE_SIZE; i++) {
        float x = (static_cast<float>(i) / (SATURATION_TABLE_SIZE - 1)) * 2.0f - 1.0f;
        
        // Harmonic generation curve
        harmonicLUT_[i] = std::sin(x * PI) * 0.1f + std::sin(x * PI * 2.0f) * 0.05f;
    }
}

float TapeEffectsProcessor::interpolateTable(const std::array<float, SATURATION_TABLE_SIZE>& table, float index) const {
    if (index <= 0.0f) return table[0];
    if (index >= SATURATION_TABLE_SIZE - 1) return table[SATURATION_TABLE_SIZE - 1];
    
    int idx = static_cast<int>(index);
    float frac = index - idx;
    
    return DSP::Interp::linear(table[idx], table[idx + 1], frac);
}

void TapeEffectsProcessor::initializePresets() {
    TapeConfig config;
    
    // Vintage Tube Warmth
    config = TapeConfig();
    config.machineType = TapeType::VINTAGE_TUBE;
    config.material = TapeMaterial::TYPE_I_NORMAL;
    config.speed = TapeSpeed::IPS_7_5;
    config.saturationAmount = 0.3f;
    config.compressionAmount = 0.4f;
    config.harmonicContent = 0.25f;
    config.lowFreqBoost = 0.3f;
    presets_["Vintage Tube Warmth"] = config;
    
    // Modern Clean
    config = TapeConfig();
    config.machineType = TapeType::MODERN_SOLID;
    config.material = TapeMaterial::TYPE_II_CHROME;
    config.speed = TapeSpeed::IPS_15;
    config.saturationAmount = 0.1f;
    config.compressionAmount = 0.2f;
    config.harmonicContent = 0.1f;
    presets_["Modern Clean"] = config;
    
    // Lo-Fi Character
    config = TapeConfig();
    config.machineType = TapeType::VINTAGE_TRANSISTOR;
    config.material = TapeMaterial::VINTAGE_ACETATE;
    config.speed = TapeSpeed::IPS_1_875;
    config.saturationAmount = 0.6f;
    config.compressionAmount = 0.7f;
    config.harmonicContent = 0.4f;
    config.wowAmount = 0.1f;
    config.flutterAmount = 0.08f;
    config.noiseFloor = -45.0f;
    presets_["Lo-Fi Character"] = config;
}

void TapeEffectsProcessor::loadPreset(const std::string& presetName) {
    auto it = presets_.find(presetName);
    if (it != presets_.end()) {
        setTapeConfig(it->second);
    }
}

std::vector<std::string> TapeEffectsProcessor::getAvailablePresets() const {
    std::vector<std::string> presetNames;
    for (const auto& [name, config] : presets_) {
        presetNames.push_back(name);
    }
    return presetNames;
}

void TapeEffectsProcessor::savePreset(const std::string& name, const TapeConfig& config) {
    presets_[name] = config;
}

float TapeEffectsProcessor::getHarmonicContent() const {
    return config_.harmonicContent;
}

float TapeEffectsProcessor::getOutputLevel() const {
    return 1.0f;  // Simplified for now
}

// Utility functions
std::string TapeEffectsProcessor::tapeTypeToString(TapeType type) {
    switch (type) {
        case TapeType::VINTAGE_TUBE: return "Vintage Tube";
        case TapeType::MODERN_SOLID: return "Modern Solid State";
        case TapeType::VINTAGE_TRANSISTOR: return "Vintage Transistor";
        case TapeType::EXOTIC_DIGITAL: return "Digital Simulation";
        case TapeType::CUSTOM: return "Custom";
    }
    return "Unknown";
}

std::string TapeEffectsProcessor::tapeMaterialToString(TapeMaterial material) {
    switch (material) {
        case TapeMaterial::TYPE_I_NORMAL: return "Type I (Normal)";
        case TapeMaterial::TYPE_II_CHROME: return "Type II (Chrome)";
        case TapeMaterial::TYPE_III_FERRICHROME: return "Type III (Ferrichrome)";
        case TapeMaterial::TYPE_IV_METAL: return "Type IV (Metal)";
        case TapeMaterial::VINTAGE_ACETATE: return "Vintage Acetate";
    }
    return "Unknown";
}

std::string TapeEffectsProcessor::tapeSpeedToString(TapeSpeed speed) {
    switch (speed) {
        case TapeSpeed::IPS_1_875: return "1⅞ ips";
        case TapeSpeed::IPS_3_75: return "3¾ ips";
        case TapeSpeed::IPS_7_5: return "7½ ips";
        case TapeSpeed::IPS_15: return "15 ips";
        case TapeSpeed::IPS_30: return "30 ips";
    }
    return "Unknown";
}