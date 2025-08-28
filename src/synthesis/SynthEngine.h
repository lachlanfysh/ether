#pragma once
#include "../core/Types.h"
#include <memory>
#include <vector>

/**
 * Abstract base class for all synthesis engines
 * Provides common interface for polyphonic synthesis
 */
class SynthEngine {
public:
    virtual ~SynthEngine() = default;
    
    // Engine identification
    virtual EngineType getType() const = 0;
    virtual const char* getName() const = 0;
    virtual const char* getDescription() const = 0;
    
    // Note events
    virtual void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f) = 0;
    virtual void noteOff(uint8_t note) = 0;
    virtual void setAftertouch(uint8_t note, float aftertouch) = 0;
    virtual void allNotesOff() = 0;
    
    // Parameter control
    virtual void setParameter(ParameterID param, float value) = 0;
    virtual float getParameter(ParameterID param) const = 0;
    virtual bool hasParameter(ParameterID param) const = 0;
    
    // Audio processing (must be real-time safe)
    virtual void processAudio(EtherAudioBuffer& outputBuffer) = 0;
    
    // Voice management
    virtual size_t getActiveVoiceCount() const = 0;
    virtual size_t getMaxVoiceCount() const = 0;
    virtual void setVoiceCount(size_t maxVoices) = 0;
    
    // Performance monitoring
    virtual float getCPUUsage() const = 0;
    
    // Preset management
    virtual void savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const = 0;
    virtual bool loadPreset(const uint8_t* data, size_t size) = 0;
    
    // Engine-specific configuration
    virtual void setSampleRate(float sampleRate) = 0;
    virtual void setBufferSize(size_t bufferSize) = 0;
    
    // Polyphonic aftertouch support
    virtual bool supportsPolyAftertouch() const { return false; }
    
    // Modulation support
    virtual void setModulation(ParameterID target, float amount) {}
    virtual bool supportsModulation(ParameterID target) const { return false; }
    
protected:
    // Common utilities for derived classes
    float sampleRate_ = SAMPLE_RATE;
    size_t bufferSize_ = BUFFER_SIZE;
    size_t maxVoices_ = MAX_VOICES;
    
    // Parameter validation
    float validateParameter(ParameterID param, float value) const;
    
    // Common parameter ranges
    static constexpr float getParameterMin(ParameterID param);
    static constexpr float getParameterMax(ParameterID param);
    static constexpr float getParameterDefault(ParameterID param);
};

/**
 * Voice base class for polyphonic engines
 */
class Voice {
public:
    virtual ~Voice() = default;
    
    // Voice lifecycle
    virtual void noteOn(uint8_t note, float velocity, float aftertouch) = 0;
    virtual void noteOff() = 0;
    virtual void kill() = 0;  // Immediate stop
    
    // Processing
    virtual AudioFrame processSample() = 0;
    virtual void processBuffer(AudioFrame* buffer, size_t frames) = 0;
    
    // State
    virtual bool isActive() const = 0;
    virtual bool isReleasing() const = 0;
    virtual uint8_t getNote() const = 0;
    virtual float getVelocity() const = 0;
    virtual float getAftertouch() const = 0;
    virtual void setAftertouch(float aftertouch) = 0;
    
    // Age for voice stealing
    virtual uint32_t getAge() const = 0;
    
protected:
    uint8_t note_ = 60;
    float velocity_ = 0.8f;
    float aftertouch_ = 0.0f;
    uint32_t startTime_ = 0;
    bool active_ = false;
    bool releasing_ = false;
};

/**
 * Polyphonic engine base class
 * Manages voice allocation and provides common polyphonic functionality
 */
template<typename VoiceType>
class PolyphonicEngine : public SynthEngine {
public:
    explicit PolyphonicEngine(size_t maxVoices = MAX_VOICES);
    virtual ~PolyphonicEngine() = default;
    
    // SynthEngine interface implementation
    void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f) override;
    void noteOff(uint8_t note) override;
    void setAftertouch(uint8_t note, float aftertouch) override;
    void allNotesOff() override;
    
    void processAudio(EtherAudioBuffer& outputBuffer) override;
    
    size_t getActiveVoiceCount() const override;
    size_t getMaxVoiceCount() const override { return voices_.size(); }
    void setVoiceCount(size_t maxVoices) override;
    
    float getCPUUsage() const override { return cpuUsage_; }
    
    void setSampleRate(float sampleRate) override;
    void setBufferSize(size_t bufferSize) override;
    
    bool supportsPolyAftertouch() const override { return true; }
    
protected:
    // Voice management
    VoiceType* allocateVoice(uint8_t note, float velocity, float aftertouch);
    VoiceType* findVoice(uint8_t note);
    void deallocateVoice(VoiceType* voice);
    
    // Voice stealing strategy
    VoiceType* stealVoice();
    
    // Parameter application to all voices
    virtual void updateAllVoices() {}
    
    std::vector<std::unique_ptr<VoiceType>> voices_;
    
private:
    uint32_t voiceCounter_ = 0;
    float cpuUsage_ = 0.0f;
    
    void updateCPUUsage(float processingTime);
};

/**
 * Factory function to create engines
 */
std::unique_ptr<SynthEngine> createSynthEngine(EngineType type);

// Note: Template implementations would go in SynthEngine.inl if needed