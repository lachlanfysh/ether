#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <functional>
#include "../modulation/RelativeVelocityModulation.h"
#include "../modulation/VelocityDepthControl.h"
#include "VelocityVolumeControl.h"

/**
 * EngineVelocityMapping - Engine-specific velocity mapping system
 * 
 * Defines how velocity modulates different synthesis parameters for each engine type:
 * - MacroVA: velocity → envelope attack/decay, filter cutoff, amp level
 * - MacroFM: velocity → modulation index, carrier/modulator levels, envelope rates  
 * - MacroHarmonics: velocity → drawbar levels, percussion level, scanner rate
 * - MacroWavetable: velocity → wavetable position, filter, amp envelope
 * 
 * Each engine type has optimized velocity response characteristics:
 * - Musical parameter mappings based on synthesis method
 * - Engine-appropriate velocity curves and scaling
 * - Integration with existing modulation and depth control systems
 * - Real-time parameter updates with smooth transitions
 * 
 * Key Features:
 * - Per-engine velocity mapping templates with musical defaults
 * - Configurable velocity→parameter routing matrix per engine
 * - Integration with RelativeVelocityModulation and VelocityDepthControl
 * - Real-time velocity parameter updates during performance
 * - Preset-based velocity mapping configurations
 * - Performance optimized for embedded real-time processing
 */

// Forward declarations
class MacroVAEngine;
class MacroFMEngine;
class MacroHarmonicsEngine;  
class MacroWavetableEngine;

class EngineVelocityMapping {
public:
    // Synthesis engine types
    enum class EngineType {
        // Main Macro Engines
        MACRO_VA,           // Virtual analog synthesis
        MACRO_FM,           // FM synthesis
        MACRO_HARMONICS,    // Harmonic/drawbar synthesis
        MACRO_WAVETABLE,    // Wavetable synthesis
        MACRO_CHORD,        // Chord synthesis
        MACRO_WAVESHAPER,   // Waveshaping synthesis
        
        // Mutable Instruments-Based Engines
        ELEMENTS_VOICE,     // Physical modeling (Elements)
        RINGS_VOICE,        // Modal synthesis (Rings)
        TIDES_OSC,          // Oscillator/LFO (Tides)
        FORMANT_VOCAL,      // Vocal formant synthesis
        NOISE_PARTICLES,    // Noise and particle synthesis
        CLASSIC_4OP_FM,     // Classic 4-operator FM
        
        // Specialized Engines
        DRUM_KIT,           // Drum synthesis engine
        SAMPLER_KIT,        // Sample playback engine
        SAMPLER_SLICER,     // Sample slicing engine
        SLIDE_ACCENT_BASS,  // Bass synthesis with slide/accent
        
        // Plaits-Based Engines (from Mutable collection)
        PLAITS_VA,          // Plaits virtual analog
        PLAITS_WAVESHAPING, // Plaits waveshaping
        PLAITS_FM,          // Plaits FM
        PLAITS_GRAIN,       // Plaits granular
        PLAITS_ADDITIVE,    // Plaits additive
        PLAITS_WAVETABLE,   // Plaits wavetable
        PLAITS_CHORD,       // Plaits chord
        PLAITS_SPEECH,      // Plaits speech synthesis
        PLAITS_SWARM,       // Plaits swarm/particle
        PLAITS_NOISE,       // Plaits noise engine
        PLAITS_PARTICLE,    // Plaits particle engine
        PLAITS_STRING,      // Plaits string engine
        PLAITS_MODAL,       // Plaits modal engine
        PLAITS_BASS_DRUM,   // Plaits bass drum
        PLAITS_SNARE_DRUM,  // Plaits snare drum
        PLAITS_HI_HAT       // Plaits hi-hat
    };
    
    // Velocity mapping targets for different engine types
    enum class VelocityTarget {
        // Universal targets (all engines)
        VOLUME,             // Overall voice volume
        FILTER_CUTOFF,      // Filter cutoff frequency
        FILTER_RESONANCE,   // Filter resonance/Q
        ENV_ATTACK,         // Envelope attack time (generic)
        ENV_DECAY,          // Envelope decay time (generic)
        ENV_SUSTAIN,        // Envelope sustain level (generic)
        ENV_RELEASE,        // Envelope release time (generic)
        
        // MacroVA specific
        VA_OSC_DETUNE,      // Oscillator detune amount
        VA_OSC_PWM,         // Pulse width modulation
        VA_NOISE_LEVEL,     // Noise oscillator level
        VA_SUB_LEVEL,       // Sub oscillator level
        
        // MacroFM specific
        FM_MOD_INDEX,       // Modulation index (carrier←modulator)
        FM_CARRIER_LEVEL,   // Carrier oscillator level
        FM_MODULATOR_LEVEL, // Modulator oscillator level
        FM_FEEDBACK,        // FM feedback amount
        FM_ALGORITHM,       // FM algorithm selection
        FM_OPERATOR_RATIO,  // Operator frequency ratio
        
        // MacroHarmonics specific
        HARM_DRAWBAR_LEVELS,    // Individual drawbar levels
        HARM_PERCUSSION_LEVEL,  // Percussion level
        HARM_PERCUSSION_DECAY,  // Percussion decay time
        HARM_SCANNER_RATE,      // Scanner vibrato rate
        HARM_SCANNER_DEPTH,     // Scanner vibrato depth
        HARM_KEY_CLICK,         // Key click intensity
        
        // MacroWavetable specific
        WT_POSITION,        // Wavetable position
        WT_SCAN_RATE,       // Wavetable scanning rate
        WT_MORPH_AMOUNT,    // Wavetable morphing amount
        WT_GRAIN_SIZE,      // Granular synthesis grain size
        WT_GRAIN_DENSITY,   // Granular synthesis density
        WT_SPECTRAL_TILT,   // Spectral filter tilt
        
        // MacroChord specific
        CHORD_VOICING,      // Chord voicing/inversion
        CHORD_SPREAD,       // Chord note spread
        CHORD_STRUM_RATE,   // Chord strum/arpeggio rate
        CHORD_HARMONIC_CONTENT, // Harmonic content of chord
        
        // MacroWaveshaper specific
        WS_DRIVE_AMOUNT,    // Waveshaper drive amount
        WS_CURVE_TYPE,      // Waveshaper curve selection
        WS_BIAS_OFFSET,     // DC bias offset
        WS_FOLD_AMOUNT,     // Wavefolding amount
        
        // Elements (Physical Modeling) specific
        ELEM_BOW_PRESSURE,  // Bowing pressure
        ELEM_BOW_POSITION,  // Bowing position
        ELEM_STRIKE_META,   // Strike meta-parameter
        ELEM_DAMPING,       // String damping
        ELEM_BRIGHTNESS,    // Harmonic brightness
        ELEM_POSITION,      // Pickup/exciter position
        
        // Rings (Modal) specific
        RINGS_FREQUENCY,    // Fundamental frequency
        RINGS_STRUCTURE,    // Resonator structure
        RINGS_BRIGHTNESS,   // Modal brightness
        RINGS_DAMPING,      // Modal damping
        RINGS_POSITION,     // Exciter position
        
        // Tides (Oscillator) specific
        TIDES_SLOPE,        // Slope/wave shape
        TIDES_SMOOTH,       // Smoothness/wave folding
        TIDES_SHIFT,        // Phase shift
        TIDES_OUTPUT_MODE,  // Output mode selection
        
        // Formant (Vocal) specific
        FORMANT_VOWEL,      // Vowel selection
        FORMANT_CLOSURE,    // Vocal tract closure
        FORMANT_TONE,       // Voice tone/character
        FORMANT_BREATH,     // Breath noise amount
        
        // Noise/Particles specific
        NOISE_COLOR,        // Noise color/filter
        NOISE_DENSITY,      // Particle density
        NOISE_TEXTURE,      // Texture parameter
        NOISE_SPREAD,       // Stereo spread
        
        // Drum Kit specific
        DRUM_PITCH,         // Drum pitch/tuning
        DRUM_DECAY,         // Drum decay time
        DRUM_SNAP,          // Drum snap/click
        DRUM_TONE,          // Drum tone color
        DRUM_DRIVE,         // Drum drive/saturation
        
        // Sampler specific
        SAMPLE_START,       // Sample start position
        SAMPLE_LOOP,        // Sample loop point
        SAMPLE_REVERSE,     // Reverse playback
        SAMPLE_PITCH,       // Sample pitch/speed
        SAMPLE_FILTER,      // Sample filter cutoff
        
        // Bass synthesis specific
        BASS_SLIDE_TIME,    // Slide/portamento time
        BASS_ACCENT_LEVEL,  // Accent emphasis
        BASS_SUB_HARMONIC,  // Sub-harmonic generation
        BASS_DISTORTION     // Bass distortion amount
    };
    
    // Velocity mapping configuration for a single parameter
    struct VelocityMapping {
        VelocityTarget target;              // Parameter to modulate
        bool enabled;                       // Mapping enabled
        float baseValue;                    // Base parameter value (0-1)
        float velocityAmount;               // Velocity modulation amount (-2.0 to +2.0)
        RelativeVelocityModulation::CurveType curveType;
        float curveAmount;                  // Curve shaping amount
        float minValue;                     // Minimum parameter value
        float maxValue;                     // Maximum parameter value
        bool invertVelocity;               // Invert velocity response
        float smoothingTime;               // Parameter transition time (ms)
        
        VelocityMapping() :
            target(VelocityTarget::VOLUME),
            enabled(true),
            baseValue(0.5f),
            velocityAmount(1.0f),
            curveType(RelativeVelocityModulation::CurveType::LINEAR),
            curveAmount(1.0f),
            minValue(0.0f),
            maxValue(1.0f),
            invertVelocity(false),
            smoothingTime(0.0f) {}
    };
    
    // Complete velocity mapping configuration for an engine
    struct EngineVelocityConfig {
        EngineType engineType;
        std::string configName;             // Configuration preset name
        std::string description;            // Human-readable description
        std::vector<VelocityMapping> mappings;
        bool globalVelocityToVolumeEnabled; // Use velocity→volume system
        float globalVelocityScale;          // Global velocity sensitivity
        float globalVelocityOffset;         // Global velocity offset
        
        EngineVelocityConfig() :
            engineType(EngineType::MACRO_VA),
            configName("Default"),
            description("Default velocity mapping"),
            globalVelocityToVolumeEnabled(true),
            globalVelocityScale(1.0f),
            globalVelocityOffset(0.0f) {}
    };
    
    // Parameter update result
    struct ParameterUpdateResult {
        VelocityTarget target;
        float originalValue;
        float modulatedValue;
        bool wasUpdated;
        float velocityComponent;
        
        ParameterUpdateResult() :
            target(VelocityTarget::VOLUME),
            originalValue(0.0f),
            modulatedValue(0.0f),
            wasUpdated(false),
            velocityComponent(0.0f) {}
    };
    
    EngineVelocityMapping();
    ~EngineVelocityMapping() = default;
    
    // Engine configuration management
    void setEngineConfig(uint32_t engineId, const EngineVelocityConfig& config);
    const EngineVelocityConfig& getEngineConfig(uint32_t engineId) const;
    bool hasEngineConfig(uint32_t engineId) const;
    void removeEngineConfig(uint32_t engineId);
    
    // Velocity mapping updates
    std::vector<ParameterUpdateResult> updateEngineParameters(uint32_t engineId, uint32_t voiceId, uint8_t velocity);
    ParameterUpdateResult updateSingleParameter(uint32_t engineId, VelocityTarget target, float baseValue, uint8_t velocity);
    void updateAllEngineVoices(uint32_t engineId, float deltaTime);
    
    // Engine-specific parameter application
    void applyMacroVAParameters(uint32_t engineId, uint32_t voiceId, const std::vector<ParameterUpdateResult>& updates);
    void applyMacroFMParameters(uint32_t engineId, uint32_t voiceId, const std::vector<ParameterUpdateResult>& updates);
    void applyMacroHarmonicsParameters(uint32_t engineId, uint32_t voiceId, const std::vector<ParameterUpdateResult>& updates);
    void applyMacroWavetableParameters(uint32_t engineId, uint32_t voiceId, const std::vector<ParameterUpdateResult>& updates);
    
    // Preset management
    void loadEnginePreset(uint32_t engineId, const std::string& presetName);
    void saveEnginePreset(uint32_t engineId, const std::string& presetName, const std::string& description);
    std::vector<std::string> getAvailablePresets(EngineType engineType) const;
    void createDefaultPresets();
    
    // Integration with other velocity systems
    void integrateWithVelocityModulation(RelativeVelocityModulation* modSystem);
    void integrateWithDepthControl(VelocityDepthControl* depthControl);
    void integrateWithVolumeControl(VelocityVolumeControl* volumeControl);
    
    // Voice management
    void addEngineVoice(uint32_t engineId, uint32_t voiceId, uint8_t velocity);
    void updateEngineVoiceVelocity(uint32_t engineId, uint32_t voiceId, uint8_t newVelocity);
    void removeEngineVoice(uint32_t engineId, uint32_t voiceId);
    void clearAllEngineVoices(uint32_t engineId);
    
    // System management
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    void setSampleRate(float sampleRate) { sampleRate_ = sampleRate; }
    float getSampleRate() const { return sampleRate_; }
    void reset();
    
    // Performance monitoring
    size_t getActiveEngineCount() const;
    size_t getActiveVoiceCount(uint32_t engineId) const;
    size_t getTotalActiveVoices() const;
    float getAverageProcessingTime() const;
    
    // Parameter mapping utilities
    float mapVelocityToParameter(const VelocityMapping& mapping, uint8_t velocity) const;
    VelocityTarget getParameterTarget(const std::string& parameterName) const;
    std::string getTargetName(VelocityTarget target) const;
    std::vector<VelocityTarget> getEngineTargets(EngineType engineType) const;
    
    // Callbacks for real-time parameter updates
    using ParameterUpdateCallback = std::function<void(uint32_t engineId, uint32_t voiceId, VelocityTarget target, float value)>;
    void setParameterUpdateCallback(ParameterUpdateCallback callback);
    
private:
    // System state
    bool enabled_;
    float sampleRate_;
    
    // Engine configurations
    std::unordered_map<uint32_t, EngineVelocityConfig> engineConfigs_;
    
    // Voice tracking per engine
    struct EngineVoiceState {
        uint32_t voiceId;
        uint32_t engineId;
        uint8_t currentVelocity;
        std::unordered_map<VelocityTarget, float> lastParameterValues;
        uint64_t lastUpdateTime;
    };
    
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, EngineVoiceState>> engineVoices_; // [engineId][voiceId]
    
    // Integration with other systems
    RelativeVelocityModulation* velocityModulation_;
    VelocityDepthControl* depthControl_;
    VelocityVolumeControl* volumeControl_;
    
    // Preset storage
    std::unordered_map<EngineType, std::vector<EngineVelocityConfig>> enginePresets_;
    
    // Callbacks
    ParameterUpdateCallback parameterUpdateCallback_;
    
    // Default configurations
    static EngineVelocityConfig defaultConfig_;
    
    // Internal methods
    void initializeDefaultPresets();
    EngineVelocityConfig createMacroVADefault();
    EngineVelocityConfig createMacroFMDefault();
    EngineVelocityConfig createMacroHarmonicsDefault();
    EngineVelocityConfig createMacroWavetableDefault();
    
    float normalizeVelocity(uint8_t velocity) const;
    float applyCurveToVelocity(float velocity, const VelocityMapping& mapping) const;
    float scaleParameterValue(float value, const VelocityMapping& mapping) const;
    void notifyParameterUpdate(uint32_t engineId, uint32_t voiceId, VelocityTarget target, float value);
    
    // Constants
    static constexpr float DEFAULT_SAMPLE_RATE = 48000.0f;
    static constexpr float MIN_VELOCITY = 0.0f;
    static constexpr float MAX_VELOCITY = 1.0f;
    static constexpr float MIN_PARAMETER_VALUE = 0.0f;
    static constexpr float MAX_PARAMETER_VALUE = 1.0f;
};