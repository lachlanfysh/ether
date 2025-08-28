#pragma once
#include "SynthEngineBase.h"
#include "../audio/VirtualAnalogOscillator.h"
#include "../audio/StateVariableFilter.h"
#include "../audio/ADSREnvelope.h"
#include "../audio/ParameterSmoother.h"
#include "../audio/RingModulator.h"
#include <array>

/**
 * SerialHPLP - Serial HP→LP Mono engine with dual VA oscillators and ring modulation
 * 
 * Features:
 * - Dual virtual analog oscillators with independent tuning
 * - Serial HP12→LP12 filter chain with pre-HP drive for bite
 * - Optional ring modulation between oscillators
 * - Advanced filter character with vintage modeling
 * - Stereo width and chorus effects processing
 * - Mono-sum below 120Hz for tight low end
 * - Optimized for cutting leads and aggressive basses
 * 
 * H/T/M Parameter Mapping:
 * - HARMONICS: HP cutoff + HP drive + ring mod amount
 * - TIMBRE: Oscillator detune + ring mod rate + LP character
 * - MORPH: HP→LP balance + filter envelope depth + stereo width
 */
class SerialHPLPEngine {
public:
    enum class FilterMode {
        SERIAL_HP_LP,   // HP12 → LP12 serial chain
        PARALLEL,       // HP12 + LP12 parallel
        LP_ONLY,        // LP12 only (HP bypassed)
        HP_ONLY,        // HP12 only (LP bypassed)
        BANDPASS        // HP12 → LP12 with resonant peak
    };
    
    enum class RingModMode {
        OFF,            // No ring modulation
        CLASSIC,        // Simple multiplication
        BALANCED,       // Balanced modulation (suppressed carrier)
        FREQUENCY,      // Frequency-based ring modulation
        SYNC            // Hard sync between oscillators
    };
    
    enum class OscillatorWaveform {
        SAWTOOTH,       // Classic saw wave
        SQUARE,         // Square/pulse wave
        TRIANGLE,       // Triangle wave
        SINE,           // Sine wave
        NOISE,          // Filtered noise
        SUPERSAW        // Supersaw with unison
    };
    
    struct FilterConfig {
        FilterMode mode = FilterMode::SERIAL_HP_LP;
        float hpCutoffHz = 80.0f;       // HP cutoff frequency
        float lpCutoffHz = 2000.0f;     // LP cutoff frequency
        float hpResonance = 0.2f;       // HP resonance (for character)
        float lpResonance = 0.4f;       // LP resonance
        float hpDrive = 0.3f;           // Pre-HP drive amount
        float filterBalance = 0.5f;     // HP vs LP balance
        float keyTracking = 0.7f;       // Key tracking amount
        bool vintageModeling = true;    // Vintage filter characteristics
        float saturation = 0.2f;        // Filter saturation amount
    };
    
    struct OscillatorConfig {
        OscillatorWaveform waveform1 = OscillatorWaveform::SAWTOOTH;
        OscillatorWaveform waveform2 = OscillatorWaveform::SAWTOOTH;
        float detune = 0.05f;           // Detune amount in semitones
        float level1 = 0.7f;            // Oscillator 1 level
        float level2 = 0.7f;            // Oscillator 2 level
        float pulseWidth1 = 0.5f;       // PWM for oscillator 1
        float pulseWidth2 = 0.5f;       // PWM for oscillator 2
        float unisonDetune = 0.1f;      // Unison detune for supersaw
        int unisonVoices = 3;           // Number of unison voices
        float noiseLevel = 0.0f;        // Noise oscillator level
        bool hardSync = false;          // Hard sync osc2 to osc1
    };
    
    struct RingModConfig {
        RingModMode mode = RingModMode::OFF;
        float amount = 0.0f;            // Ring modulation amount
        float frequency = 1.0f;         // Ring mod frequency multiplier
        float feedback = 0.0f;          // Ring mod feedback
        bool enableSidechain = false;  // Use external sidechain
        float sidechainLevel = 0.0f;    // Sidechain input level
        float dryWetBalance = 0.5f;     // Dry/wet ring mod balance
    };
    
    struct StereoConfig {
        float width = 0.5f;             // Stereo width (0=mono, 1=full stereo)
        bool chorusEnable = false;      // Enable chorus effect
        float chorusRate = 2.0f;        // Chorus LFO rate
        float chorusDepth = 0.3f;       // Chorus modulation depth
        float chorusDelay = 5.0f;       // Chorus delay time (ms)
        float chorusFeedback = 0.1f;    // Chorus feedback amount
        bool monoLowEnd = true;         // Mono-sum below 120Hz
        float monoFreq = 120.0f;        // Mono-sum frequency
    };
    
    struct VoiceState {
        bool active = false;
        float note = 60.0f;
        float velocity = 100.0f;
        float pitchBend = 0.0f;         // Pitch bend amount (-1 to +1)
        
        // Oscillator phases
        float phase1 = 0.0f;
        float phase2 = 0.0f;
        std::array<float, 7> unisonPhases; // For supersaw unison
        
        // Filter state
        float hpCutoff = 80.0f;
        float lpCutoff = 2000.0f;
        
        // Ring modulation state
        float ringModPhase = 0.0f;
        float ringModAmount = 0.0f;
        
        // Timing
        uint32_t noteOnTime = 0;
        
        // Envelope states
        bool ampEnvActive = false;
        bool filterEnvActive = false;
    };
    
    SerialHPLPEngine();
    ~SerialHPLPEngine();
    
    // Initialization
    bool initialize(float sampleRate);
    void shutdown();
    
    // Configuration
    void setFilterConfig(const FilterConfig& config);
    const FilterConfig& getFilterConfig() const { return filterConfig_; }
    
    void setOscillatorConfig(const OscillatorConfig& config);
    const OscillatorConfig& getOscillatorConfig() const { return oscConfig_; }
    
    void setRingModConfig(const RingModConfig& config);
    const RingModConfig& getRingModConfig() const { return ringModConfig_; }
    
    void setStereoConfig(const StereoConfig& config);
    const StereoConfig& getStereoConfig() const { return stereoConfig_; }
    
    // H/T/M Parameter control (implements engine interface)
    void setHarmonics(float harmonics);    // 0-1: HP cutoff + drive + ring mod
    void setTimbre(float timbre);          // 0-1: Detune + ring rate + LP character
    void setMorph(float morph);            // 0-1: HP/LP balance + filter env + width
    
    void setHTMParameters(float harmonics, float timbre, float morph);
    void getHTMParameters(float& harmonics, float& timbre, float& morph) const;
    
    // Voice control
    void noteOn(float note, float velocity);
    void noteOff(float releaseTime = 0.0f);
    void allNotesOff();
    void setPitchBend(float bendAmount); // -1 to +1 (typically ±2 semitones)
    
    // Real-time parameter control
    void setFilterCutoffs(float hpCutoff, float lpCutoff);
    void setDetune(float detune); // In semitones
    void setRingModAmount(float amount);
    void setStereoWidth(float width);
    
    // Audio processing
    std::pair<float, float> processSampleStereo(); // Returns {left, right}
    float processSample(); // Mono output (sum of stereo)
    void processBlockStereo(float* outputL, float* outputR, int numSamples);
    void processBlock(float* output, int numSamples);
    void processParameters(float deltaTimeMs);
    
    // Analysis
    bool isActive() const { return voiceState_.active; }
    float getCurrentNote() const { return voiceState_.note; }
    float getHPCutoff() const { return voiceState_.hpCutoff; }
    float getLPCutoff() const { return voiceState_.lpCutoff; }
    float getRingModAmount() const { return voiceState_.ringModAmount; }
    float getCPUUsage() const;
    
    // Filter analysis
    float getHPOutput() const { return lastHPOutput_; }
    float getLPOutput() const { return lastLPOutput_; }
    float getFilterDrive() const;
    
    // Preset management
    void reset();
    void setPreset(const std::string& presetName);
    
private:
    // Core audio components
    VirtualAnalogOscillator osc1_;
    VirtualAnalogOscillator osc2_;
    VirtualAnalogOscillator noiseOsc_;
    std::array<VirtualAnalogOscillator, 7> unisonOscs_; // For supersaw
    
    StateVariableFilter hpFilter_;
    StateVariableFilter lpFilter_;
    
    RingModulator ringMod_;
    
    ADSREnvelope ampEnvelope_;
    ADSREnvelope filterEnvelope_;
    
    // Parameter smoothing
    ParameterSmoother hpCutoffSmoother_;
    ParameterSmoother lpCutoffSmoother_;
    ParameterSmoother hpDriveSmoother_;
    ParameterSmoother detuneSmoother_;
    ParameterSmoother ringModSmoother_;
    ParameterSmoother widthSmoother_;
    
    // Configuration
    FilterConfig filterConfig_;
    OscillatorConfig oscConfig_;
    RingModConfig ringModConfig_;
    StereoConfig stereoConfig_;
    
    // State
    VoiceState voiceState_;
    float sampleRate_;
    bool initialized_;
    
    // Current parameter values
    float harmonics_ = 0.5f;
    float timbre_ = 0.5f;
    float morph_ = 0.5f;
    
    // Internal processing state
    float currentHPCutoff_ = 80.0f;
    float currentLPCutoff_ = 2000.0f;
    float currentHPDrive_ = 0.3f;
    float currentDetune_ = 0.05f;
    float currentRingMod_ = 0.0f;
    float currentWidth_ = 0.5f;
    
    // Filter analysis
    float lastHPOutput_ = 0.0f;
    float lastLPOutput_ = 0.0f;
    
    // Chorus/delay processing
    std::array<float, 4410> chorusDelayLineL_; // 100ms at 44.1kHz
    std::array<float, 4410> chorusDelayLineR_;
    int chorusDelayIndexL_ = 0;
    int chorusDelayIndexR_ = 0;
    float chorusLFOPhase_ = 0.0f;
    
    // Mono low-end processing
    StateVariableFilter monoHPFilter_;
    float monoLowSignal_ = 0.0f;
    
    // Performance monitoring
    float cpuUsage_ = 0.0f;
    uint32_t processingStartTime_ = 0;
    
    // Private methods
    void updateOscillatorParameters();
    void updateFilterParameters();
    void updateRingModParameters();
    void updateStereoParameters();
    
    // Oscillator processing
    float processOscillators();
    float processSupersaw();
    float processUnisonOscillator(int index, float baseFreq, float detune);
    
    // Filter processing
    std::pair<float, float> processFilters(float input);
    float applyFilterDrive(float input, float drive);
    float applyVintageCharacteristics(float input, float cutoff);
    
    // Ring modulation processing
    float processRingModulation(float carrier, float modulator);
    float generateRingModulator();
    
    // Stereo processing
    std::pair<float, float> processStereoEffects(float input);
    std::pair<float, float> processChorus(float inputL, float inputR);
    std::pair<float, float> processMonoLowEnd(float inputL, float inputR);
    std::pair<float, float> applyStereoWidth(float inputL, float inputR, float width);
    
    // Parameter mapping
    float mapHarmonicsToHP(float harmonics) const;
    float mapTimbreToDetune(float timbre) const;
    float mapMorphToBalance(float morph) const;
    
    // Filter helpers
    float calculateKeyTrackedCutoff(float note, float baseCutoff) const;
    float applyFilterSaturation(float input, float amount) const;
    
    // Utility functions
    float noteToFrequency(float note) const;
    float semitonesToRatio(float semitones) const;
    float dbToLinear(float db) const;
    uint32_t getTimeMs() const;
    float lerp(float a, float b, float t) const;
    float clamp(float value, float min, float max) const;
    
    // Constants
    static constexpr float MIN_HP_CUTOFF_HZ = 20.0f;
    static constexpr float MAX_HP_CUTOFF_HZ = 2000.0f;
    static constexpr float MIN_LP_CUTOFF_HZ = 100.0f;
    static constexpr float MAX_LP_CUTOFF_HZ = 12000.0f;
    static constexpr float MAX_DETUNE_SEMITONES = 12.0f;
    static constexpr float MAX_DRIVE_GAIN = 6.0f;
    static constexpr int MAX_UNISON_VOICES = 7;
    static constexpr float CHORUS_MAX_DELAY_MS = 50.0f;
};