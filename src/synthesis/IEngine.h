#pragma once
#include "../core/Types.h"
#include "../core/ErrorHandler.h"
#include <cstdint>

/**
 * Common rendering context for all engines
 * Contains per-block parameter values and voice state
 */
struct RenderContext {
    // Voice parameters
    float pitch_semitones;   // MIDI note as semitones
    float fine_cents;        // +/-100c fine tuning
    float gate;              // 0/1 gate signal
    float aftertouch;        // 0..1 polyphonic aftertouch
    float modWheel;          // 0..1 mod wheel value
    uint32_t randomSeed;     // per-voice or per-block seed
    
    // Smoothed, per-block macros (0..1 unless noted)
    float HARMONICS, TIMBRE, MORPH, LEVEL;
    float EXTRA1, EXTRA2;    // engine-specific optional
    
    // Sample rate and block info
    float sampleRate;
    int blockSize;
};

/**
 * New synthesis engine interface following requirements specification
 * All engines must implement this interface for consistency
 */
class IEngine {
public:
    virtual ~IEngine() = default;
    
    // Engine initialization and cleanup
    virtual void prepare(double sampleRate, int maxBlockSize) = 0;
    virtual void reset() = 0;               // silent within 1 block
    
    // Error-aware initialization (optional - for new implementations)
    virtual EtherSynth::Result<void> initializeEngine(double sampleRate, int maxBlockSize) {
        prepare(sampleRate, maxBlockSize);
        return EtherSynth::Result<void>(); // Success by default
    }
    
    // Voice management  
    virtual void noteOn(float note, float velocity, uint32_t id) = 0;
    virtual void noteOff(uint32_t id) = 0;  // may tail
    
    // Parameter control (normalized 0..1)
    virtual void setParam(int paramID, float v01) = 0;           
    virtual void setMod(int paramID, float value, float depth) = 0; // per-block modulation
    
    // Audio rendering
    virtual void render(const RenderContext& ctx, float* out, int n) = 0; // mono unless stereo
    virtual bool isStereo() const = 0;
    
    // Engine metadata
    virtual const char* getName() const = 0;
    virtual const char* getShortName() const = 0;
    virtual int getEngineID() const = 0;
    
    // CPU classification for UI hints
    enum class CPUClass { Light, Medium, Heavy, VeryHeavy };
    virtual CPUClass getCPUClass() const = 0;
    
    // Parameter metadata
    struct ParameterInfo {
        int id;
        const char* name;
        const char* unit;
        float defaultValue;
        float minValue;
        float maxValue;
        bool isDiscrete;
        int steps; // for discrete params
        const char* group;
    };
    
    virtual int getParameterCount() const = 0;
    virtual const ParameterInfo* getParameterInfo(int index) const = 0;
    
    // Modulation destinations (bitmask)
    virtual uint32_t getModDestinations() const = 0;
    
    // Haptics hints
    enum class HapticPolicy { Uniform, Curve, Guard, Landmarks, CenterNotch };
    struct HapticInfo {
        HapticPolicy policy;
        float arcDegrees; // 180/270/360
        const float* detents; // array of detent positions in engineering units
        int detentCount;
    };
    virtual const HapticInfo* getHapticInfo(int paramID) const = 0;
};

/**
 * Common parameter IDs as defined in requirements
 */
enum class EngineParamID : int {
    // Common macros (all engines)
    PITCH = 0, FINE, LEVEL,
    HARMONICS, TIMBRE, MORPH, EXTRA1, EXTRA2,
    
    // Channel Strip parameters  
    HPF_CUTOFF, LPF_CUTOFF, LPF_RES, FLT_KEYTRACK,
    FLT_ENV_AMT, FLT_ATTACK, FLT_DECAY, FLT_SUSTAIN, FLT_RELEASE,
    COMP_AMOUNT, PUNCH, DRIVE, DRIVE_TONE, BODY, AIR,
    STRIP_ENABLE, STRIP_MODE,
    SEND_A, SEND_B, SEND_C, PAN,
    
    // Engine-specific ranges start here
    ENGINE_SPECIFIC_START = 100,
    
    // MacroVA specific
    UNISON_COUNT = ENGINE_SPECIFIC_START, UNISON_SPREAD, SUB_LEVEL, NOISE_MIX, SYNC_DEPTH,
    
    // MacroFM specific  
    ALGO = ENGINE_SPECIFIC_START + 10, BRIGHT_TILT, FIXED_MOD,
    
    // MacroWaveshaper specific
    FOLD_MODE = ENGINE_SPECIFIC_START + 20, POST_LP,
    
    // MacroWavetable specific
    UNISON_3V = ENGINE_SPECIFIC_START + 30, INTERP,
    
    // MacroChord specific
    VOICES = ENGINE_SPECIFIC_START + 40, ENGINE_1, ENGINE_2, ENGINE_3, ENGINE_4, ENGINE_5, STRUM_MS,
    
    // MacroHarmonics specific
    PARTIAL_COUNT = ENGINE_SPECIFIC_START + 50, DECAY_EXP, INHARMONICITY, 
    EVEN_ODD_BIAS, BANDLIMIT_MODE,
    
    // Formant/Vocal specific
    VOWEL = ENGINE_SPECIFIC_START + 60, BANDWIDTH, BREATH, 
    FORMANT_SHIFT, GLOTTAL_SHAPE,
    
    // Noise/Particles specific
    DENSITY_HZ = ENGINE_SPECIFIC_START + 70, GRAIN_MS, BP_CENTER, BP_Q, SPRAY,
    
    // TidesOsc specific
    CONTOUR = ENGINE_SPECIFIC_START + 80, SLOPE, UNISON, CHAOS, LFO_MODE,
    
    // RingsVoice specific
    STRUCTURE = ENGINE_SPECIFIC_START + 90, BRIGHTNESS, POSITION, 
    EXCITER, DAMPING, SPACE_MIX, STEREO,
    
    // ElementsVoice specific
    GEOMETRY = ENGINE_SPECIFIC_START + 100, ENERGY, 
    EXCITER_BAL, SPACE, NOISE_COLOR
};

/**
 * Engine factory for creating instances of each engine type
 */
class EngineFactory {
public:
    enum class EngineType {
        MACRO_VA = 0,
        MACRO_FM,
        MACRO_WAVESHAPER, 
        MACRO_WAVETABLE,
        MACRO_CHORD,
        MACRO_HARMONICS,
        FORMANT_VOCAL,
        NOISE_PARTICLES,
        TIDES_OSC,
        RINGS_VOICE,
        ELEMENTS_VOICE,
        DRUM_KIT,
        SAMPLER_KIT,
        SAMPLER_SLICER,
        COUNT
    };
    
    static std::unique_ptr<IEngine> createEngine(EngineType type);
    static const char* getEngineName(EngineType type);
    static int getEngineCount() { return static_cast<int>(EngineType::COUNT); }
};