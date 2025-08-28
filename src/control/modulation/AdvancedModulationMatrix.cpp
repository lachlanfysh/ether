#include "AdvancedModulationMatrix.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iostream>

AdvancedModulationMatrix::AdvancedModulationMatrix() {
    std::cout << "AdvancedModulationMatrix created" << std::endl;
    
    // Initialize source values to neutral positions
    sourceValues_.fill(0.0f);
    sourceEnabled_.fill(true);
    
    // Initialize LFOs with different frequencies
    lfos_[0].frequency = 1.0f;   // 1 Hz
    lfos_[1].frequency = 0.5f;   // 0.5 Hz
    lfos_[2].frequency = 2.0f;   // 2 Hz
    
    // Set up envelope followers
    for (auto& follower : envelopeFollowers_) {
        follower.attack = 0.01f;
        follower.release = 0.1f;
        follower.level = 0.0f;
    }
    
    // Calculate update interval
    updateInterval_ = 1.0f / updateRate_;
    
    std::cout << "AdvancedModulationMatrix: Initialized with " << static_cast<int>(ModSource::COUNT) 
              << " modulation sources" << std::endl;
}

AdvancedModulationMatrix::~AdvancedModulationMatrix() {
    std::cout << "AdvancedModulationMatrix destroyed" << std::endl;
}

void AdvancedModulationMatrix::addModulation(ModSource source, ParameterID destination, float amount) {
    ModulationSlot slot;
    slot.source = source;
    slot.destination = destination;
    slot.amount = std::clamp(amount, -1.0f, 1.0f);
    slot.enabled = true;
    slot.id = generateSlotId();
    
    modSlots_.push_back(slot);
    
    std::cout << "Added modulation: " << getSourceName(source) 
              << " -> Parameter " << static_cast<int>(destination) 
              << " (amount: " << amount << ")" << std::endl;
}

void AdvancedModulationMatrix::removeModulation(uint32_t slotId) {
    auto it = std::remove_if(modSlots_.begin(), modSlots_.end(),
        [slotId](const ModulationSlot& slot) { return slot.id == slotId; });
    
    if (it != modSlots_.end()) {
        modSlots_.erase(it, modSlots_.end());
        std::cout << "Removed modulation slot " << slotId << std::endl;
    }
}

void AdvancedModulationMatrix::clearAllModulations() {
    modSlots_.clear();
    std::cout << "Cleared all modulations" << std::endl;
}

void AdvancedModulationMatrix::processFrame() {
    float currentTime = getCurrentTime();
    float deltaTime = currentTime - lastUpdateTime_;
    
    if (deltaTime >= updateInterval_ || !smartUpdates_) {
        updateSourceValues();
        updateLFOs(deltaTime);
        updateMacros();
        updateAudioDerivedSources();
        
        lastUpdateTime_ = currentTime;
    }
}

void AdvancedModulationMatrix::updateSourceValues() {
    // Hardware sources are updated externally via setSourceValue()
    // Internal sources are updated here
    
    // Random source
    static float randomTimer = 0.0f;
    randomTimer += updateInterval_;
    if (randomTimer >= 0.1f) { // Update random every 100ms
        sourceValues_[static_cast<size_t>(ModSource::RANDOM)] = 
            static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f;
        randomTimer = 0.0f;
    }
    
    // Note-based sources (would be set by voice manager)
    // These are placeholders for now
    sourceValues_[static_cast<size_t>(ModSource::NOTE_NUMBER)] = 0.5f; // Middle C
    sourceValues_[static_cast<size_t>(ModSource::NOTE_ON_TIME)] = 0.0f;
    sourceValues_[static_cast<size_t>(ModSource::VOICE_COUNT)] = 0.0f;
}

float AdvancedModulationMatrix::getModulatedValue(ParameterID param, float baseValue) const {
    float result = baseValue;
    
    for (const auto& slot : modSlots_) {
        if (!slot.enabled || slot.destination != param) continue;
        
        float sourceValue = getSourceValue(slot.source);
        
        // Apply conditional modulation if specified
        if (slot.condition != ModSource::COUNT) {
            float conditionValue = getSourceValue(slot.condition);
            bool conditionMet = slot.conditionInvert ? 
                (conditionValue < slot.conditionThreshold) : 
                (conditionValue >= slot.conditionThreshold);
            
            if (!conditionMet) continue;
        }
        
        // Apply processing to source value
        float processedValue = applyProcessing(sourceValue, slot.processing, slot.curveAmount);
        
        // Apply rate multiplier for time-based sources
        if (slot.source >= ModSource::LFO_1 && slot.source <= ModSource::LFO_3) {
            processedValue *= slot.rateMultiplier;
        }
        
        // Add phase offset
        if (slot.phaseOffset != 0.0f) {
            processedValue = std::sin((std::asin(processedValue) + slot.phaseOffset * 2.0f * M_PI));
        }
        
        // Convert to unipolar if needed
        if (!slot.bipolar) {
            processedValue = (processedValue + 1.0f) * 0.5f;
        }
        
        // Apply amount and offset
        float modulation = processedValue * slot.amount + slot.offset;
        
        // Apply global modulation amount
        modulation *= globalModAmount_;
        
        // Apply smoothing if specified
        if (slot.responseTime > 0.0f) {
            auto& filter = smoothingFilters_[slot.id];
            filter.target = baseValue + modulation;
            filter.smoothTime = slot.responseTime;
            result = filter.process(filter.target, updateInterval_);
        } else {
            result += modulation;
        }
    }
    
    return result;
}

void AdvancedModulationMatrix::setSourceValue(ModSource source, float value) {
    size_t index = static_cast<size_t>(source);
    if (index < sourceValues_.size()) {
        sourceValues_[index] = std::clamp(value, -1.0f, 1.0f);
    }
}

float AdvancedModulationMatrix::getSourceValue(ModSource source) const {
    size_t index = static_cast<size_t>(source);
    if (index < sourceValues_.size() && sourceEnabled_[index]) {
        return sourceValues_[index];
    }
    return 0.0f;
}

void AdvancedModulationMatrix::setSourceEnabled(ModSource source, bool enabled) {
    size_t index = static_cast<size_t>(source);
    if (index < sourceEnabled_.size()) {
        sourceEnabled_[index] = enabled;
    }
}

std::vector<AdvancedModulationMatrix::ModulationSlot> AdvancedModulationMatrix::getModulationsForParameter(ParameterID param) const {
    std::vector<ModulationSlot> result;
    
    for (const auto& slot : modSlots_) {
        if (slot.destination == param) {
            result.push_back(slot);
        }
    }
    
    return result;
}

std::vector<AdvancedModulationMatrix::ModulationSlot> AdvancedModulationMatrix::getModulationsFromSource(ModSource source) const {
    std::vector<ModulationSlot> result;
    
    for (const auto& slot : modSlots_) {
        if (slot.source == source) {
            result.push_back(slot);
        }
    }
    
    return result;
}

AdvancedModulationMatrix::ModulationSlot* AdvancedModulationMatrix::getModulationSlot(uint32_t slotId) {
    auto it = std::find_if(modSlots_.begin(), modSlots_.end(),
        [slotId](const ModulationSlot& slot) { return slot.id == slotId; });
    
    return (it != modSlots_.end()) ? &(*it) : nullptr;
}

const AdvancedModulationMatrix::ModulationSlot* AdvancedModulationMatrix::getModulationSlot(uint32_t slotId) const {
    auto it = std::find_if(modSlots_.begin(), modSlots_.end(),
        [slotId](const ModulationSlot& slot) { return slot.id == slotId; });
    
    return (it != modSlots_.end()) ? &(*it) : nullptr;
}

void AdvancedModulationMatrix::setGlobalModulationAmount(float amount) {
    globalModAmount_ = std::clamp(amount, 0.0f, 2.0f);
}

void AdvancedModulationMatrix::defineMacro(ModSource macro, const std::vector<std::pair<ModSource, float>>& sources) {
    if (macro >= ModSource::MACRO_1 && macro <= ModSource::MACRO_4) {
        macros_[macro] = sources;
        std::cout << "Defined macro " << getSourceName(macro) 
                  << " with " << sources.size() << " sources" << std::endl;
    }
}

void AdvancedModulationMatrix::clearMacro(ModSource macro) {
    auto it = macros_.find(macro);
    if (it != macros_.end()) {
        macros_.erase(it);
        std::cout << "Cleared macro " << getSourceName(macro) << std::endl;
    }
}

AdvancedModulationMatrix::LFO* AdvancedModulationMatrix::getLFO(int index) {
    if (index >= 0 && index < 3) {
        return &lfos_[index];
    }
    return nullptr;
}

void AdvancedModulationMatrix::syncAllLFOs() {
    for (auto& lfo : lfos_) {
        lfo.sync();
    }
    std::cout << "Synced all LFOs" << std::endl;
}

AdvancedModulationMatrix::EnvelopeFollower* AdvancedModulationMatrix::getEnvelopeFollower(int index) {
    if (index >= 0 && index < 3) {
        return &envelopeFollowers_[index];
    }
    return nullptr;
}

void AdvancedModulationMatrix::processAudioInput(const EtherAudioBuffer& audioBuffer) {
    // Update envelope followers and audio analysis
    for (size_t i = 0; i < audioBuffer.size(); i++) {
        float sample = (audioBuffer[i].left + audioBuffer[i].right) * 0.5f;
        
        for (auto& follower : envelopeFollowers_) {
            follower.process(std::abs(sample), 1.0f / SAMPLE_RATE);
        }
    }
    
    // Update audio-derived sources
    analyzeAudioLevel(audioBuffer);
    analyzeAudioPitch(audioBuffer);
    analyzeAudioBrightness(audioBuffer);
}

std::vector<AdvancedModulationMatrix::ModulationInfo> AdvancedModulationMatrix::getActiveModulations() const {
    std::vector<ModulationInfo> result;
    
    for (const auto& slot : modSlots_) {
        if (!slot.enabled) continue;
        
        ModulationInfo info;
        info.source = slot.source;
        info.destination = slot.destination;
        info.currentValue = getSourceValue(slot.source);
        info.amount = slot.amount;
        info.active = std::abs(info.currentValue) > 0.001f;
        info.description = getSourceName(slot.source) + " -> Param " + std::to_string(static_cast<int>(slot.destination));
        
        result.push_back(info);
    }
    
    return result;
}

float AdvancedModulationMatrix::getModulationActivity() const {
    float activity = 0.0f;
    int activeCount = 0;
    
    for (const auto& slot : modSlots_) {
        if (slot.enabled) {
            float sourceValue = getSourceValue(slot.source);
            activity += std::abs(sourceValue * slot.amount);
            activeCount++;
        }
    }
    
    return activeCount > 0 ? activity / activeCount : 0.0f;
}

void AdvancedModulationMatrix::saveMatrix(std::vector<uint8_t>& data) const {
    // Simple binary serialization
    data.clear();
    
    // Write number of slots
    uint32_t numSlots = static_cast<uint32_t>(modSlots_.size());
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&numSlots), 
                reinterpret_cast<const uint8_t*>(&numSlots) + sizeof(numSlots));
    
    // Write each slot
    for (const auto& slot : modSlots_) {
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&slot), 
                    reinterpret_cast<const uint8_t*>(&slot) + sizeof(slot));
    }
    
    // Write global settings
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&globalModAmount_), 
                reinterpret_cast<const uint8_t*>(&globalModAmount_) + sizeof(globalModAmount_));
}

bool AdvancedModulationMatrix::loadMatrix(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(uint32_t)) return false;
    
    size_t offset = 0;
    
    // Read number of slots
    uint32_t numSlots;
    std::memcpy(&numSlots, data.data() + offset, sizeof(numSlots));
    offset += sizeof(numSlots);
    
    if (data.size() < offset + numSlots * sizeof(ModulationSlot) + sizeof(float)) {
        return false;
    }
    
    // Clear existing slots
    modSlots_.clear();
    modSlots_.reserve(numSlots);
    
    // Read each slot
    for (uint32_t i = 0; i < numSlots; i++) {
        ModulationSlot slot;
        std::memcpy(&slot, data.data() + offset, sizeof(slot));
        offset += sizeof(slot);
        modSlots_.push_back(slot);
    }
    
    // Read global settings
    std::memcpy(&globalModAmount_, data.data() + offset, sizeof(globalModAmount_));
    
    std::cout << "Loaded modulation matrix with " << numSlots << " slots" << std::endl;
    return true;
}

void AdvancedModulationMatrix::resetToDefault() {
    clearAllModulations();
    globalModAmount_ = 1.0f;
    macros_.clear();
    
    // Reset LFOs
    for (auto& lfo : lfos_) {
        lfo.reset();
    }
    
    std::cout << "Reset modulation matrix to defaults" << std::endl;
}

void AdvancedModulationMatrix::setUpdateRate(float hz) {
    updateRate_ = std::clamp(hz, 10.0f, 10000.0f);
    updateInterval_ = 1.0f / updateRate_;
}

void AdvancedModulationMatrix::enableSmartUpdates(bool enable) {
    smartUpdates_ = enable;
}

std::string AdvancedModulationMatrix::getSourceName(ModSource source) {
    switch (source) {
        case ModSource::SMART_KNOB: return "Smart Knob";
        case ModSource::TOUCH_X: return "Touch X";
        case ModSource::TOUCH_Y: return "Touch Y";
        case ModSource::AFTERTOUCH: return "Aftertouch";
        case ModSource::VELOCITY: return "Velocity";
        case ModSource::LFO_1: return "LFO 1";
        case ModSource::LFO_2: return "LFO 2";
        case ModSource::LFO_3: return "LFO 3";
        case ModSource::ENVELOPE_1: return "Envelope 1";
        case ModSource::ENVELOPE_2: return "Envelope 2";
        case ModSource::ENVELOPE_3: return "Envelope 3";
        case ModSource::RANDOM: return "Random";
        case ModSource::AUDIO_LEVEL: return "Audio Level";
        case ModSource::AUDIO_PITCH: return "Audio Pitch";
        case ModSource::AUDIO_BRIGHTNESS: return "Audio Brightness";
        case ModSource::NOTE_NUMBER: return "Note Number";
        case ModSource::NOTE_ON_TIME: return "Note On Time";
        case ModSource::VOICE_COUNT: return "Voice Count";
        case ModSource::MACRO_1: return "Macro 1";
        case ModSource::MACRO_2: return "Macro 2";
        case ModSource::MACRO_3: return "Macro 3";
        case ModSource::MACRO_4: return "Macro 4";
        default: return "Unknown";
    }
}

std::string AdvancedModulationMatrix::getProcessingName(ModProcessing processing) {
    switch (processing) {
        case ModProcessing::DIRECT: return "Direct";
        case ModProcessing::INVERTED: return "Inverted";
        case ModProcessing::RECTIFIED: return "Rectified";
        case ModProcessing::QUANTIZED: return "Quantized";
        case ModProcessing::SMOOTHED: return "Smoothed";
        case ModProcessing::SAMPLE_HOLD: return "Sample & Hold";
        case ModProcessing::CURVE_EXPONENTIAL: return "Exponential";
        case ModProcessing::CURVE_LOGARITHMIC: return "Logarithmic";
        case ModProcessing::CURVE_S_SHAPE: return "S-Curve";
        default: return "Unknown";
    }
}

float AdvancedModulationMatrix::applyProcessing(float value, ModProcessing processing, float curveAmount) const {
    switch (processing) {
        case ModProcessing::DIRECT:
            return value;
        case ModProcessing::INVERTED:
            return -value;
        case ModProcessing::RECTIFIED:
            return std::abs(value);
        case ModProcessing::QUANTIZED:
            return std::round(value * 8.0f) / 8.0f; // 8 steps
        case ModProcessing::SMOOTHED:
            // This would use a smoothing filter in practice
            return value;
        case ModProcessing::SAMPLE_HOLD:
            // This would sample and hold at regular intervals
            return value;
        case ModProcessing::CURVE_EXPONENTIAL:
            return exponentialCurve(value, curveAmount);
        case ModProcessing::CURVE_LOGARITHMIC:
            return logarithmicCurve(value, curveAmount);
        case ModProcessing::CURVE_S_SHAPE:
            return sCurve(value, curveAmount);
        default:
            return value;
    }
}

// LFO implementation
float AdvancedModulationMatrix::LFO::process(float deltaTime) {
    if (!enabled) return 0.0f;
    
    float output = 0.0f;
    
    switch (waveform) {
        case Waveform::SINE:
            output = std::sin(phase * 2.0f * M_PI);
            break;
        case Waveform::TRIANGLE:
            output = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
            break;
        case Waveform::SAW_UP:
            output = 2.0f * phase - 1.0f;
            break;
        case Waveform::SAW_DOWN:
            output = 1.0f - 2.0f * phase;
            break;
        case Waveform::SQUARE:
            output = (phase < 0.5f) ? 1.0f : -1.0f;
            break;
        case Waveform::RANDOM:
            if (phase < deltaTime * frequency) {
                output = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f;
            }
            break;
        default:
            output = 0.0f;
            break;
    }
    
    // Update phase
    phase += deltaTime * frequency;
    if (phase >= 1.0f) phase -= 1.0f;
    
    return output * amplitude + offset;
}

// EnvelopeFollower implementation
float AdvancedModulationMatrix::EnvelopeFollower::process(float input, float deltaTime) {
    float target = std::abs(input);
    
    if (target > level) {
        // Attack
        float rate = 1.0f / (attack + 0.001f);
        level += (target - level) * rate * deltaTime;
    } else {
        // Release
        float rate = 1.0f / (release + 0.001f);
        level += (target - level) * rate * deltaTime;
    }
    
    return std::clamp(level, 0.0f, 1.0f);
}

// SmoothingFilter implementation
float AdvancedModulationMatrix::SmoothingFilter::process(float input, float deltaTime) {
    if (smoothTime <= 0.0f) {
        value = input;
        return value;
    }
    
    float rate = 1.0f / smoothTime;
    value += (input - value) * rate * deltaTime;
    return value;
}

// Private helper methods
uint32_t AdvancedModulationMatrix::generateSlotId() {
    return nextSlotId_++;
}

void AdvancedModulationMatrix::updateMacros() {
    for (const auto& macro : macros_) {
        float value = 0.0f;
        
        for (const auto& source : macro.second) {
            value += getSourceValue(source.first) * source.second;
        }
        
        setSourceValue(macro.first, std::clamp(value, -1.0f, 1.0f));
    }
}

void AdvancedModulationMatrix::updateLFOs(float deltaTime) {
    for (size_t i = 0; i < lfos_.size(); i++) {
        float lfoValue = lfos_[i].process(deltaTime);
        ModSource lfoSource = static_cast<ModSource>(static_cast<size_t>(ModSource::LFO_1) + i);
        setSourceValue(lfoSource, lfoValue);
    }
}

void AdvancedModulationMatrix::updateAudioDerivedSources() {
    setSourceValue(ModSource::AUDIO_LEVEL, audioLevel_);
    setSourceValue(ModSource::AUDIO_PITCH, (audioPitch_ - 440.0f) / 440.0f); // Normalized around A4
    setSourceValue(ModSource::AUDIO_BRIGHTNESS, audioBrightness_);
}

void AdvancedModulationMatrix::applyConditionalModulation(ModulationSlot& slot) {
    // This would be called during modulation processing
    // Implementation already in getModulatedValue
}

float AdvancedModulationMatrix::getCurrentTime() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / 1000000.0f;
}

float AdvancedModulationMatrix::exponentialCurve(float value, float amount) const {
    if (amount == 0.0f) return value;
    
    float sign = (value >= 0.0f) ? 1.0f : -1.0f;
    float abs_val = std::abs(value);
    
    if (amount > 0.0f) {
        return sign * std::pow(abs_val, 1.0f + amount * 3.0f);
    } else {
        return sign * std::pow(abs_val, 1.0f / (1.0f - amount * 0.8f));
    }
}

float AdvancedModulationMatrix::logarithmicCurve(float value, float amount) const {
    if (amount == 0.0f) return value;
    
    float sign = (value >= 0.0f) ? 1.0f : -1.0f;
    float abs_val = std::abs(value) + 0.001f; // Avoid log(0)
    
    if (amount > 0.0f) {
        return sign * std::log(abs_val * (std::exp(1.0f) - 1.0f) + 1.0f) * (1.0f + amount);
    } else {
        return sign * (std::exp(abs_val * (1.0f - amount)) - 1.0f) / (std::exp(1.0f) - 1.0f);
    }
}

float AdvancedModulationMatrix::sCurve(float value, float amount) const {
    if (amount == 0.0f) return value;
    
    float t = (value + 1.0f) * 0.5f; // Convert from -1,1 to 0,1
    float curve = t * t * (3.0f - 2.0f * t); // Basic S-curve
    
    // Blend with original based on amount
    float result = value * (1.0f - std::abs(amount)) + (curve * 2.0f - 1.0f) * std::abs(amount);
    
    return std::clamp(result, -1.0f, 1.0f);
}

void AdvancedModulationMatrix::analyzeAudioLevel(const EtherAudioBuffer& buffer) {
    float sum = 0.0f;
    for (const auto& frame : buffer) {
        sum += std::abs(frame.left) + std::abs(frame.right);
    }
    audioLevel_ = sum / (buffer.size() * 2.0f);
}

void AdvancedModulationMatrix::analyzeAudioPitch(const EtherAudioBuffer& buffer) {
    // Simple pitch detection would go here
    // For now, just maintain the current value
    (void)buffer; // Suppress unused parameter warning
}

void AdvancedModulationMatrix::analyzeAudioBrightness(const EtherAudioBuffer& buffer) {
    // Simple brightness analysis (high frequency content)
    float brightness = 0.0f;
    for (size_t i = 1; i < buffer.size(); i++) {
        float diff = std::abs(buffer[i].left - buffer[i-1].left) + 
                     std::abs(buffer[i].right - buffer[i-1].right);
        brightness += diff;
    }
    audioBrightness_ = brightness / (buffer.size() * 2.0f);
}

// Factory modulation templates
namespace ModulationTemplates {
    void setupClassicFilter(AdvancedModulationMatrix& matrix) {
        matrix.addModulation(AdvancedModulationMatrix::ModSource::LFO_1, 
                           ParameterID::FILTER_CUTOFF, 0.3f);
        std::cout << "Setup classic filter modulation" << std::endl;
    }
    
    void setupClassicTremolo(AdvancedModulationMatrix& matrix) {
        matrix.addModulation(AdvancedModulationMatrix::ModSource::LFO_2, 
                           ParameterID::VOLUME, 0.2f);
        std::cout << "Setup classic tremolo modulation" << std::endl;
    }
    
    void setupClassicVibrato(AdvancedModulationMatrix& matrix) {
        matrix.addModulation(AdvancedModulationMatrix::ModSource::LFO_3, 
                           ParameterID::DETUNE, 0.05f);
        std::cout << "Setup classic vibrato modulation" << std::endl;
    }
    
    void setupPerformanceTouch(AdvancedModulationMatrix& matrix) {
        matrix.addModulation(AdvancedModulationMatrix::ModSource::TOUCH_X, 
                           ParameterID::FILTER_CUTOFF, 0.5f);
        matrix.addModulation(AdvancedModulationMatrix::ModSource::TOUCH_Y, 
                           ParameterID::FILTER_RESONANCE, 0.3f);
        matrix.addModulation(AdvancedModulationMatrix::ModSource::AFTERTOUCH, 
                           ParameterID::VOLUME, 0.2f);
        std::cout << "Setup performance touch modulation" << std::endl;
    }
    
    void setupSmartKnobMacro(AdvancedModulationMatrix& matrix) {
        matrix.addModulation(AdvancedModulationMatrix::ModSource::SMART_KNOB, 
                           ParameterID::FILTER_CUTOFF, 0.6f);
        matrix.addModulation(AdvancedModulationMatrix::ModSource::SMART_KNOB, 
                           ParameterID::FILTER_RESONANCE, 0.3f);
        std::cout << "Setup smart knob macro modulation" << std::endl;
    }
    
    void setupAftertouchExpression(AdvancedModulationMatrix& matrix) {
        matrix.addModulation(AdvancedModulationMatrix::ModSource::AFTERTOUCH, 
                           ParameterID::VOLUME, 0.3f);
        matrix.addModulation(AdvancedModulationMatrix::ModSource::AFTERTOUCH, 
                           ParameterID::FILTER_CUTOFF, 0.4f);
        std::cout << "Setup aftertouch expression modulation" << std::endl;
    }
    
    void setupAudioReactive(AdvancedModulationMatrix& matrix) {
        matrix.addModulation(AdvancedModulationMatrix::ModSource::AUDIO_LEVEL, 
                           ParameterID::FILTER_CUTOFF, 0.5f);
        matrix.addModulation(AdvancedModulationMatrix::ModSource::AUDIO_BRIGHTNESS, 
                           ParameterID::FILTER_RESONANCE, 0.3f);
        std::cout << "Setup audio reactive modulation" << std::endl;
    }
    
    void setupSpectralModulation(AdvancedModulationMatrix& matrix) {
        matrix.addModulation(AdvancedModulationMatrix::ModSource::AUDIO_PITCH, 
                           ParameterID::LFO_RATE, 0.4f);
        matrix.addModulation(AdvancedModulationMatrix::ModSource::AUDIO_BRIGHTNESS, 
                           ParameterID::HARMONICS, 0.3f);
        std::cout << "Setup spectral modulation" << std::endl;
    }
    
    void setupChaoticModulation(AdvancedModulationMatrix& matrix) {
        matrix.addModulation(AdvancedModulationMatrix::ModSource::RANDOM, 
                           ParameterID::FILTER_CUTOFF, 0.2f);
        matrix.addModulation(AdvancedModulationMatrix::ModSource::RANDOM, 
                           ParameterID::LFO_RATE, 0.3f);
        matrix.addModulation(AdvancedModulationMatrix::ModSource::RANDOM, 
                           ParameterID::PAN, 0.1f);
        std::cout << "Setup chaotic modulation" << std::endl;
    }
    
    void setupRhythmicModulation(AdvancedModulationMatrix& matrix) {
        auto* lfo1 = matrix.getLFO(0);
        auto* lfo2 = matrix.getLFO(1);
        auto* lfo3 = matrix.getLFO(2);
        
        if (lfo1) lfo1->frequency = 2.0f; // 2 Hz
        if (lfo2) lfo2->frequency = 1.0f; // 1 Hz
        if (lfo3) lfo3->frequency = 4.0f; // 4 Hz
        
        matrix.syncAllLFOs();
        
        matrix.addModulation(AdvancedModulationMatrix::ModSource::LFO_1, 
                           ParameterID::VOLUME, 0.1f);
        matrix.addModulation(AdvancedModulationMatrix::ModSource::LFO_2, 
                           ParameterID::FILTER_CUTOFF, 0.3f);
        matrix.addModulation(AdvancedModulationMatrix::ModSource::LFO_3, 
                           ParameterID::PAN, 0.2f);
        
        std::cout << "Setup rhythmic modulation" << std::endl;
    }
}