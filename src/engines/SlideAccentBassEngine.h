#pragma once
#include "../synthesis/SynthEngine.h"
#include "../audio/ZDFLadderFilter.h"
#include "../audio/VirtualAnalogOscillator.h"
#include "../audio/ADSREnvelope.h"
#include "../audio/ParameterSmoother.h"
#include <array>

/**
 * SlideAccentBass - Specialized mono bass engine with exponential slide and accent system
 * 
 * Features:
 * - Exponential legato slide system (5-120ms per note interval)
 * - Per-note accent system (+4-8dB VCA, +10-25% cutoff boost, +Q boost)
 * - ZDF ladder filter with internal soft-clip saturation
 * - Phase reset policy (reset non-legato, preserve legato)
 * - Sub-oscillator with independent level control
 * - Drive/saturation stage before filter for character
 * - Optimized for punchy bass lines and acid-style sequences
 * 
 * H/T/M Parameter Mapping:
 * - HARMONICS: Filter cutoff + resonance auto-ride
 * - TIMBRE: Oscillator shape + sub oscillator blend + drive
 * - MORPH: Slide time + accent amount + filter envelope depth
 */
class SlideAccentBassEngine : public SynthEngine {
public:
    enum class SlideMode {
        OFF,            // No slide
        LEGATO_ONLY,    // Slide only on legato notes
        ALWAYS,         // Always slide between notes
        ACCENT_ONLY     // Slide only on accented notes
    };
    
    enum class AccentMode {
        OFF,            // No accent processing
        VELOCITY,       // Accent based on velocity threshold
        PATTERN,        // Accent based on pattern flags
        COMBINED        // Both velocity and pattern flags
    };
    
    enum class PhaseResetPolicy {
        ALWAYS,         // Reset phase on every note
        NON_LEGATO,     // Reset only on non-legato notes
        NEVER           // Never reset phase
    };
    
    struct SlideConfig {
        SlideMode mode = SlideMode::LEGATO_ONLY;
        float minTimeMs = 5.0f;         // Minimum slide time
        float maxTimeMs = 120.0f;       // Maximum slide time
        float curve = 0.5f;             // Slide curve shape (0=linear, 1=exponential)
        bool quantizeTime = false;      // Quantize slide time to musical divisions
        float portamentoAmount = 1.0f;  // Global portamento scaling
    };
    
    struct AccentConfig {
        AccentMode mode = AccentMode::VELOCITY;
        float velocityThreshold = 100.0f;  // Velocity threshold for accent (0-127)
        float volumeBoost = 6.0f;          // Volume boost in dB (0-12dB)
        float cutoffBoost = 20.0f;         // Cutoff boost percentage (0-50%)
        float resonanceBoost = 15.0f;      // Resonance boost percentage (0-30%)
        float driveBoost = 25.0f;          // Drive boost percentage (0-50%)
        float decayTime = 0.05f;           // Accent decay time in seconds
        bool accentEnvelope = true;        // Use envelope for accent decay
    };
    
    struct FilterConfig {
        float cutoffHz = 1000.0f;       // Base cutoff frequency
        float resonance = 0.3f;         // Resonance amount (0-1)
        float keyTracking = 0.5f;       // Key tracking amount (0-1)
        float envelopeDepth = 0.6f;     // Filter envelope depth
        float velocitySensitivity = 0.4f; // Velocity to cutoff sensitivity
        bool autoResonanceRide = true;  // Auto-ride resonance with cutoff
        float saturationDrive = 0.2f;   // Internal saturation drive
    };
    
    struct OscillatorConfig {
        float shape = 0.5f;             // Oscillator shape (saw-square-tri)
        float pulseWidth = 0.5f;        // Pulse width for square wave
        float subLevel = 0.3f;          // Sub-oscillator level
        float subShape = 0.0f;          // Sub-oscillator shape
        int subOctave = -1;             // Sub-oscillator octave offset
        float drive = 0.2f;             // Pre-filter drive amount
        float noiseLevel = 0.05f;       // Noise level for character
    };
    
    struct VoiceState {
        bool active = false;
        bool legato = false;
        bool accented = false;
        
        float note = 60.0f;             // Current note
        float targetNote = 60.0f;       // Target note (for sliding)
        float velocity = 100.0f;        // Note velocity
        float slideProgress = 1.0f;     // Slide progress (0-1)
        float slideTime = 0.0f;         // Current slide time
        
        // Accent state
        float accentAmount = 0.0f;      // Current accent amount
        float accentPhase = 0.0f;       // Accent envelope phase
        
        // Timing
        uint32_t noteOnTime = 0;        // Note on timestamp
        uint32_t lastNoteTime = 0;      // Previous note timestamp
        
        // Phase state
        float phase = 0.0f;             // Oscillator phase
        float subPhase = 0.0f;          // Sub-oscillator phase
        bool phaseReset = false;        // Flag for phase reset
    };
    
    SlideAccentBassEngine();
    ~SlideAccentBassEngine();
    
    // Initialization
    bool initialize(float sampleRate);
    void shutdown();
    
    // Configuration
    void setSlideConfig(const SlideConfig& config);
    const SlideConfig& getSlideConfig() const { return slideConfig_; }
    
    void setAccentConfig(const AccentConfig& config);
    const AccentConfig& getAccentConfig() const { return accentConfig_; }
    
    void setFilterConfig(const FilterConfig& config);
    const FilterConfig& getFilterConfig() const { return filterConfig_; }
    
    void setOscillatorConfig(const OscillatorConfig& config);
    const OscillatorConfig& getOscillatorConfig() const { return oscConfig_; }
    
    void setPhaseResetPolicy(PhaseResetPolicy policy) { phaseResetPolicy_ = policy; }
    PhaseResetPolicy getPhaseResetPolicy() const { return phaseResetPolicy_; }
    
    // H/T/M Parameter control (implements engine interface)
    void setHarmonics(float harmonics);    // 0-1: Filter cutoff + auto-resonance
    void setTimbre(float timbre);          // 0-1: Osc shape + sub blend + drive
    void setMorph(float morph);            // 0-1: Slide time + accent + filter env
    
    void setHTMParameters(float harmonics, float timbre, float morph);
    void getHTMParameters(float& harmonics, float& timbre, float& morph) const;
    
    // Voice control
    void noteOn(float note, float velocity, bool accent = false, float slideTimeMs = 0.0f);
    void noteOff(float releaseTime = 0.0f);
    // Note: allNotesOff() now implemented as SynthEngine override
    
    // Legato and slide control
    void setLegato(bool legato) { voiceState_.legato = legato; }
    bool isLegato() const { return voiceState_.legato; }
    void setSlideTime(float timeMs);
    float getSlideTime() const { return voiceState_.slideTime; }
    
    // Accent control
    void setAccent(bool accented);
    bool isAccented() const { return voiceState_.accented; }
    void triggerAccent(float amount = 1.0f);
    
    // Audio processing
    float processSample();
    void processBlock(float* output, int numSamples);
    void processParameters(float deltaTimeMs);
    
    // Analysis
    bool isActive() const { return voiceState_.active; }
    float getCurrentNote() const { return voiceState_.note; }
    float getSlideProgress() const { return voiceState_.slideProgress; }
    float getAccentAmount() const { return voiceState_.accentAmount; }
    float getFilterCutoff() const;
    float getCPUUsage() const override;
    
    // Preset management
    void reset();
    void setPreset(const std::string& presetName);
    
    // SynthEngine interface implementation
    EngineType getType() const override { return EngineType::SLIDE_ACCENT_BASS; }
    const char* getName() const override { return "SlideAccentBass"; }
    const char* getDescription() const override { return "Specialized mono bass engine with slide and accent"; }
    
    // SynthEngine note methods (adapt existing methods)
    void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f) override;
    void noteOff(uint8_t note) override;
    void setAftertouch(uint8_t note, float aftertouch) override;
    void allNotesOff() override;
    
    // SynthEngine parameter methods
    void setParameter(ParameterID param, float value) override;
    float getParameter(ParameterID param) const override;
    bool hasParameter(ParameterID param) const override;
    
    // SynthEngine audio processing
    void processAudio(EtherAudioBuffer& outputBuffer) override;
    
    // SynthEngine voice management
    size_t getActiveVoiceCount() const override { return isActive() ? 1 : 0; }
    size_t getMaxVoiceCount() const override { return 1; } // Mono engine
    void setVoiceCount(size_t maxVoices) override {} // Mono engine ignores this
    
    // SynthEngine preset methods
    void savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const override;
    bool loadPreset(const uint8_t* data, size_t size) override;
    
    // SynthEngine configuration
    void setSampleRate(float sampleRate) override;
    void setBufferSize(size_t bufferSize) override;
    
private:
    // Core audio components
    VirtualAnalogOscillator mainOsc_;
    VirtualAnalogOscillator subOsc_;
    ZDFLadderFilter filter_;
    ADSREnvelope ampEnvelope_;
    ADSREnvelope filterEnvelope_;
    
    // Parameter smoothing
    ParameterSmoother cutoffSmoother_;
    ParameterSmoother resonanceSmoother_;
    ParameterSmoother driveSmoother_;
    ParameterSmoother volumeSmoother_;
    
    // Configuration
    SlideConfig slideConfig_;
    AccentConfig accentConfig_;
    FilterConfig filterConfig_;
    OscillatorConfig oscConfig_;
    PhaseResetPolicy phaseResetPolicy_;
    
    // State
    VoiceState voiceState_;
    float sampleRate_;
    bool initialized_;
    
    // Current parameter values
    float harmonics_ = 0.5f;
    float timbre_ = 0.5f;
    float morph_ = 0.5f;
    
    // Internal processing state
    float currentCutoff_ = 1000.0f;
    float currentResonance_ = 0.3f;
    float currentDrive_ = 0.2f;
    float currentVolume_ = 1.0f;
    
    // Slide processing
    float slideStartNote_ = 60.0f;
    float slideEndNote_ = 60.0f;
    uint32_t slideStartTime_ = 0;
    
    // Accent processing
    float baseVolume_ = 1.0f;
    float baseCutoff_ = 1000.0f;
    float baseResonance_ = 0.3f;
    float baseDrive_ = 0.2f;
    
    // Performance monitoring
    uint32_t processingStartTime_ = 0;
    float cpuUsage_ = 0.0f;
    
    // Private methods
    void updateSlideParameters();
    void updateAccentParameters();
    void updateFilterParameters();
    void updateOscillatorParameters();
    
    // Slide calculation
    float calculateSlideTime(float fromNote, float toNote) const;
    float calculateSlideProgress(uint32_t currentTime) const;
    float applySlideEasing(float progress) const;
    
    // Accent calculation
    bool shouldAccent(float velocity, bool patternAccent) const;
    float calculateAccentAmount(float velocity, bool patternAccent) const;
    void applyAccentBoosts(float accentAmount);
    void updateAccentEnvelope(float deltaTimeMs);
    
    // Filter helpers
    float calculateKeyTrackedCutoff(float note, float baseCutoff) const;
    float calculateAutoResonance(float cutoff, float baseResonance) const;
    float applySaturation(float input, float drive) const;
    
    // Phase management
    bool shouldResetPhase(bool legato) const;
    void resetOscillatorPhases();
    void updateOscillatorPhases(float note);
    
    // Parameter mapping
    float mapHarmonicsToFilter(float harmonics) const;
    float mapTimbreToOscillator(float timbre) const;
    float mapMorphToSlideAccent(float morph) const;
    
    // Utility functions
    float noteToFrequency(float note) const;
    float dbToLinear(float db) const;
    float linearToDb(float linear) const;
    uint32_t getTimeMs() const;
    float lerp(float a, float b, float t) const;
    float clamp(float value, float min, float max) const;
    
    // Constants
    static constexpr float MIN_CUTOFF_HZ = 20.0f;
    static constexpr float MAX_CUTOFF_HZ = 12000.0f;
    static constexpr float MIN_SLIDE_TIME_MS = 1.0f;
    static constexpr float MAX_SLIDE_TIME_MS = 500.0f;
    static constexpr float MAX_ACCENT_BOOST_DB = 12.0f;
    static constexpr float MAX_DRIVE_GAIN = 4.0f;
    static constexpr float PHASE_RESET_THRESHOLD = 0.1f; // For phase continuity
};