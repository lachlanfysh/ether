#pragma once
#include "../core/Types.h"
#include "../core/ErrorHandler.h"
#include "DSPUtils.h"
#include <array>
#include <memory>
#include <atomic>

namespace EtherSynth {

/**
 * Shared components to eliminate code duplication across synthesis engines
 * 
 * This module provides reusable building blocks that are common across
 * multiple engine implementations, reducing code duplication and improving
 * maintainability while ensuring consistent behavior.
 */

//=============================================================================
// Common Parameter Management
//=============================================================================

/**
 * Standardized parameter handling for all engines
 */
class ParameterManager {
public:
    ParameterManager() {
        parameters_.fill(0.0f);
        modulations_.fill(0.0f);
        smoothedParams_.fill(0.0f);
    }
    
    // Parameter control
    void setParameter(ParameterID param, float value) {
        size_t index = static_cast<size_t>(param);
        if (index < parameters_.size()) {
            parameters_[index] = std::clamp(value, 0.0f, 1.0f);
        }
    }
    
    void setModulation(ParameterID target, float amount) {
        size_t index = static_cast<size_t>(target);
        if (index < modulations_.size()) {
            modulations_[index] = std::clamp(amount, -1.0f, 1.0f);
        }
    }
    
    float getParameter(ParameterID param) const {
        size_t index = static_cast<size_t>(param);
        return (index < parameters_.size()) ? parameters_[index] : 0.0f;
    }
    
    float getModulation(ParameterID target) const {
        size_t index = static_cast<size_t>(target);
        return (index < modulations_.size()) ? modulations_[index] : 0.0f;
    }
    
    // Get final parameter value with modulation applied
    float getFinalValue(ParameterID param) const {
        float base = getParameter(param);
        float mod = getModulation(param);
        return std::clamp(base + mod, 0.0f, 1.0f);
    }
    
    // Smooth parameter changes
    void updateSmoothing(float smoothingFactor = 0.99f) {
        for (size_t i = 0; i < smoothedParams_.size(); ++i) {
            float target = parameters_[i] + modulations_[i];
            smoothedParams_[i] = smoothedParams_[i] * smoothingFactor + 
                               target * (1.0f - smoothingFactor);
        }
    }
    
    float getSmoothedValue(ParameterID param) const {
        size_t index = static_cast<size_t>(param);
        return (index < smoothedParams_.size()) ? smoothedParams_[index] : 0.0f;
    }
    
    // Batch operations for performance
    void clearAllModulations() {
        modulations_.fill(0.0f);
    }
    
    void resetAllParameters() {
        parameters_.fill(0.0f);
        modulations_.fill(0.0f);
        smoothedParams_.fill(0.0f);
    }
    
private:
    std::array<float, static_cast<size_t>(ParameterID::COUNT)> parameters_;
    std::array<float, static_cast<size_t>(ParameterID::COUNT)> modulations_;
    std::array<float, static_cast<size_t>(ParameterID::COUNT)> smoothedParams_;
};

//=============================================================================
// Common Voice State Management
//=============================================================================

/**
 * Standardized voice state and lifecycle management
 */
struct VoiceState {
    // Voice identification
    uint32_t id = 0;
    uint32_t noteNumber = 60;
    uint32_t startTime = 0;
    
    // Performance state
    float velocity_ = 0.8f;
    float aftertouch_ = 0.0f;
    float noteFrequency_ = 440.0f;
    bool active_ = false;
    bool releasing_ = false;
    
    // Audio parameters
    float volume_ = 0.8f;
    float pan_ = 0.0f;
    
    // State queries
    bool isActive() const { return active_; }
    bool isReleasing() const { return releasing_; }
    uint32_t getAge(uint32_t currentTime) const { 
        return currentTime - startTime; 
    }
    
    // Voice lifecycle
    void noteOn(uint32_t noteNum, float vel, uint32_t currentTime, uint32_t voiceId) {
        id = voiceId;
        noteNumber = noteNum;
        velocity_ = vel;
        startTime = currentTime;
        noteFrequency_ = 440.0f * std::pow(2.0f, (noteNum - 69.0f) / 12.0f);
        active_ = true;
        releasing_ = false;
    }
    
    void noteOff() {
        releasing_ = true;
    }
    
    void kill() {
        active_ = false;
        releasing_ = false;
    }
    
    void setAftertouch(float at) {
        aftertouch_ = std::clamp(at, 0.0f, 1.0f);
    }
};

//=============================================================================
// Common CPU Usage Tracking
//=============================================================================

/**
 * Consistent CPU usage measurement across all engines
 */
class CPUUsageTracker {
public:
    CPUUsageTracker() : cpuUsage_(0.0f), sampleCount_(0), totalTime_(0.0f) {}
    
    void updateCPUUsage(float processingTime) {
        totalTime_ += processingTime;
        sampleCount_++;
        
        // Update rolling average every 1000 samples
        if (sampleCount_ >= 1000) {
            cpuUsage_ = totalTime_ / sampleCount_;
            totalTime_ = 0.0f;
            sampleCount_ = 0;
        }
    }
    
    float getCPUUsage() const { 
        return cpuUsage_; 
    }
    
    void reset() {
        cpuUsage_ = 0.0f;
        sampleCount_ = 0;
        totalTime_ = 0.0f;
    }
    
private:
    std::atomic<float> cpuUsage_;
    std::atomic<uint32_t> sampleCount_;
    float totalTime_;
};

//=============================================================================
// Common ADSR Envelope
//=============================================================================

/**
 * Reusable ADSR envelope implementation
 */
class StandardADSR {
public:
    enum class Stage { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };
    
    StandardADSR() : stage_(Stage::IDLE), output_(0.0f), attack_(0.01f),
                    decay_(0.3f), sustain_(0.8f), release_(0.1f),
                    sampleRate_(48000.0f) {
        updateRates();
    }
    
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        updateRates();
    }
    
    void setADSR(float attack, float decay, float sustain, float release) {
        attack_ = std::max(0.001f, attack);
        decay_ = std::max(0.001f, decay);
        sustain_ = std::clamp(sustain, 0.0f, 1.0f);
        release_ = std::max(0.001f, release);
        updateRates();
    }
    
    void noteOn() {
        stage_ = Stage::ATTACK;
    }
    
    void noteOff() {
        if (stage_ != Stage::IDLE) {
            stage_ = Stage::RELEASE;
        }
    }
    
    float process() {
        switch (stage_) {
            case Stage::IDLE:
                output_ = 0.0f;
                break;
                
            case Stage::ATTACK:
                output_ += attackRate_;
                if (output_ >= 1.0f) {
                    output_ = 1.0f;
                    stage_ = Stage::DECAY;
                }
                break;
                
            case Stage::DECAY:
                output_ -= decayRate_;
                if (output_ <= sustain_) {
                    output_ = sustain_;
                    stage_ = Stage::SUSTAIN;
                }
                break;
                
            case Stage::SUSTAIN:
                output_ = sustain_;
                break;
                
            case Stage::RELEASE:
                output_ -= releaseRate_;
                if (output_ <= 0.0f) {
                    output_ = 0.0f;
                    stage_ = Stage::IDLE;
                }
                break;
        }
        
        return output_;
    }
    
    bool isActive() const {
        return stage_ != Stage::IDLE && output_ > 0.001f;
    }
    
    Stage getStage() const { return stage_; }
    float getOutput() const { return output_; }
    
private:
    Stage stage_;
    float output_;
    
    // ADSR parameters
    float attack_, decay_, sustain_, release_;
    float sampleRate_;
    
    // Calculated rates
    float attackRate_, decayRate_, releaseRate_;
    
    void updateRates() {
        attackRate_ = 1.0f / (attack_ * sampleRate_);
        decayRate_ = (1.0f - sustain_) / (decay_ * sampleRate_);
        releaseRate_ = sustain_ / (release_ * sampleRate_);
    }
};

//=============================================================================
// Common Filter Implementation
//=============================================================================

/**
 * Standard state variable filter used across multiple engines
 */
class StandardSVF {
public:
    enum class Type { LOWPASS, HIGHPASS, BANDPASS, NOTCH };
    
    StandardSVF() : cutoff_(1000.0f), resonance_(1.0f), sampleRate_(48000.0f),
                   low_(0.0f), band_(0.0f), type_(Type::LOWPASS) {
        updateCoefficients();
    }
    
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        updateCoefficients();
    }
    
    void setParameters(float cutoff, float resonance) {
        cutoff_ = std::clamp(cutoff, 20.0f, sampleRate_ * 0.45f);
        resonance_ = std::clamp(resonance, 0.1f, 30.0f);
        updateCoefficients();
    }
    
    void setType(Type type) {
        type_ = type;
    }
    
    float process(float input) {
        float high = input - low_ - q_ * band_;
        band_ += f_ * high;
        low_ += f_ * band_;
        
        switch (type_) {
            case Type::LOWPASS:  return low_;
            case Type::HIGHPASS: return high;
            case Type::BANDPASS: return band_;
            case Type::NOTCH:    return low_ + high;
        }
        return low_;
    }
    
    void reset() {
        low_ = band_ = 0.0f;
    }
    
private:
    float cutoff_, resonance_, sampleRate_;
    float f_, q_;  // Filter coefficients
    float low_, band_;  // Filter states
    Type type_;
    
    void updateCoefficients() {
        f_ = 2.0f * std::sin(M_PI * cutoff_ / sampleRate_);
        q_ = 1.0f / resonance_;
    }
};

//=============================================================================
// Common LFO Implementation
//=============================================================================

/**
 * Standard LFO implementation for modulation
 */
class StandardLFO {
public:
    enum class Waveform { SINE, TRIANGLE, SAWTOOTH, SQUARE, NOISE };
    
    StandardLFO() : frequency_(1.0f), phase_(0.0f), sampleRate_(48000.0f),
                   waveform_(Waveform::SINE), amplitude_(1.0f) {
        updateIncrement();
    }
    
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        updateIncrement();
    }
    
    void setFrequency(float frequency) {
        frequency_ = std::clamp(frequency, 0.01f, 20.0f);
        updateIncrement();
    }
    
    void setWaveform(Waveform waveform) {
        waveform_ = waveform;
    }
    
    void setAmplitude(float amplitude) {
        amplitude_ = std::clamp(amplitude, 0.0f, 1.0f);
    }
    
    void reset() {
        phase_ = 0.0f;
    }
    
    float process() {
        float output = 0.0f;
        
        switch (waveform_) {
            case Waveform::SINE:
                output = std::sin(2.0f * M_PI * phase_);
                break;
                
            case Waveform::TRIANGLE:
                output = 2.0f * std::abs(2.0f * (phase_ - std::floor(phase_ + 0.5f))) - 1.0f;
                break;
                
            case Waveform::SAWTOOTH:
                output = 2.0f * (phase_ - std::floor(phase_ + 0.5f));
                break;
                
            case Waveform::SQUARE:
                output = (phase_ < 0.5f) ? -1.0f : 1.0f;
                break;
                
            case Waveform::NOISE:
                output = 2.0f * (static_cast<float>(rand()) / RAND_MAX) - 1.0f;
                break;
        }
        
        phase_ += phaseIncrement_;
        if (phase_ >= 1.0f) {
            phase_ -= 1.0f;
        }
        
        return output * amplitude_;
    }
    
private:
    float frequency_, phase_, sampleRate_;
    float phaseIncrement_;
    Waveform waveform_;
    float amplitude_;
    
    void updateIncrement() {
        phaseIncrement_ = frequency_ / sampleRate_;
    }
};

//=============================================================================
// Engine Component Factory
//=============================================================================

/**
 * Factory for creating standardized engine components
 */
class EngineComponentFactory {
public:
    static std::unique_ptr<ParameterManager> createParameterManager() {
        return std::make_unique<ParameterManager>();
    }
    
    static std::unique_ptr<CPUUsageTracker> createCPUTracker() {
        return std::make_unique<CPUUsageTracker>();
    }
    
    static std::unique_ptr<StandardADSR> createADSR() {
        return std::make_unique<StandardADSR>();
    }
    
    static std::unique_ptr<StandardSVF> createFilter() {
        return std::make_unique<StandardSVF>();
    }
    
    static std::unique_ptr<StandardLFO> createLFO() {
        return std::make_unique<StandardLFO>();
    }
    
    // Create complete voice setup
    struct VoiceComponents {
        std::unique_ptr<StandardADSR> ampEnv;
        std::unique_ptr<StandardADSR> filterEnv;
        std::unique_ptr<StandardSVF> filter;
        std::unique_ptr<StandardLFO> lfo;
        VoiceState state;
    };
    
    static VoiceComponents createVoiceComponents() {
        VoiceComponents components;
        components.ampEnv = createADSR();
        components.filterEnv = createADSR();
        components.filter = createFilter();
        components.lfo = createLFO();
        return components;
    }
};

//=============================================================================
// Common Engine Utilities
//=============================================================================

namespace EngineUtils {
    // Common parameter scaling functions
    inline float logScale(float value01, float min, float max) {
        return min * std::pow(max / min, value01);
    }
    
    inline float expScale(float value01, float min, float max) {
        return min + (max - min) * value01 * value01;
    }
    
    inline float linearScale(float value01, float min, float max) {
        return min + (max - min) * value01;
    }
    
    // Common voice management
    inline uint32_t getCurrentTime() {
        static uint32_t time = 0;
        return ++time; // Simplified counter
    }
    
    // Parameter validation
    inline float validateParameter(float value, float min = 0.0f, float max = 1.0f) {
        return std::clamp(value, min, max);
    }
    
    // Common MIDI utilities
    inline float midiToFrequency(uint8_t midiNote) {
        return 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
    }
    
    inline float frequencyToMidi(float frequency) {
        return 69.0f + 12.0f * std::log2(frequency / 440.0f);
    }
}

} // namespace EtherSynth