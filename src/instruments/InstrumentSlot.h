#pragma once
#include "../core/Types.h"
#include <memory>
#include <vector>
#include <string>

class SynthEngine;
class EffectsChain;
class EuclideanRhythm;

/**
 * Represents one of the 8 color-coded instrument slots
 * Each slot can contain multiple synthesis engines (for layering)
 * and has its own effects chain and sequencer pattern
 */
class InstrumentSlot {
public:
    explicit InstrumentSlot(InstrumentColor color);
    ~InstrumentSlot();
    
    // Identity
    InstrumentColor getColor() const { return color_; }
    void setName(const std::string& name) { name_ = name; }
    const std::string& getName() const { return name_; }
    void setIcon(uint8_t iconIndex) { iconIndex_ = iconIndex; }
    uint8_t getIcon() const { return iconIndex_; }
    
    // Engine management (multi-engine support for layering)
    void addEngine(EngineType type);
    void removeEngine(size_t index);
    void setEngineBalance(size_t index, float balance);  // For layering
    size_t getEngineCount() const { return engines_.size(); }
    SynthEngine* getEngine(size_t index) const;
    SynthEngine* getPrimaryEngine() const;  // First engine, for single-engine operation
    
    // Parameter control
    void setParameter(ParameterID param, float value);
    float getParameter(ParameterID param) const;
    void setEngineParameter(size_t engineIndex, ParameterID param, float value);
    float getEngineParameter(size_t engineIndex, ParameterID param) const;
    
    // Note events
    void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f);
    void noteOff(uint8_t note);
    void setAftertouch(uint8_t note, float aftertouch);
    void allNotesOff();
    
    // Audio processing
    void processAudio(EtherAudioBuffer& outputBuffer);
    
    // Sequencer pattern
    EuclideanRhythm* getPattern() { return pattern_.get(); }
    void setPatternActive(bool active) { patternActive_ = active; }
    bool isPatternActive() const { return patternActive_; }
    
    // Effects chain
    EffectsChain* getEffects() { return effects_.get(); }
    
    // Mix controls
    void setVolume(float volume) { volume_ = volume; }
    float getVolume() const { return volume_; }
    void setPan(float pan) { pan_ = pan; }
    float getPan() const { return pan_; }
    void setMute(bool mute) { muted_ = mute; }
    bool isMuted() const { return muted_; }
    void setSolo(bool solo) { soloed_ = solo; }
    bool isSoloed() const { return soloed_; }
    
    // Performance info
    size_t getActiveVoiceCount() const;
    float getCPUUsage() const;
    
    // Chord mode support
    void setChordRole(bool enabled) { isChordInstrument_ = enabled; }
    bool isChordInstrument() const { return isChordInstrument_; }
    void playChordNotes(const std::vector<uint8_t>& notes, float velocity);
    
private:
    // Identity
    InstrumentColor color_;
    std::string name_;
    uint8_t iconIndex_ = 0;
    
    // Synthesis engines (multi-engine layering support)
    struct EngineLayer {
        std::unique_ptr<SynthEngine> engine;
        float balance = 1.0f;  // Mix level for this layer
        bool enabled = true;
    };
    std::vector<EngineLayer> engines_;
    
    // Effects and processing
    std::unique_ptr<EffectsChain> effects_;
    std::unique_ptr<EuclideanRhythm> pattern_;
    
    // Mix controls
    float volume_ = 0.8f;
    float pan_ = 0.0f;    // -1.0 (left) to 1.0 (right)
    bool muted_ = false;
    bool soloed_ = false;
    
    // State
    bool patternActive_ = false;
    bool isChordInstrument_ = false;
    
    // Parameter storage for each engine
    std::vector<std::array<float, static_cast<size_t>(ParameterID::COUNT)>> engineParameters_;
    
    // Audio processing helpers
    void processEngines(EtherAudioBuffer& buffer);
    void applyEffects(EtherAudioBuffer& buffer);
    void applyMixControls(EtherAudioBuffer& buffer);
    
    // Engine management helpers
    void initializeEngineParameters(size_t engineIndex);
    std::unique_ptr<SynthEngine> createEngine(EngineType type);
    
    // Default names for each color
    static const std::array<std::string, MAX_INSTRUMENTS> DEFAULT_NAMES;
};