#pragma once
#include "../core/Types.h"
#include "../audio/SIMDOptimizations.h"
#include <array>
#include <memory>
#include <cstdint>
#include <cmath>

namespace EtherSynthModulation {

/**
 * Advanced LFO System for EtherSynth V1.0
 * 
 * Features:
 * - 12 high-quality waveforms including custom shapes
 * - Sample-accurate sync to host/internal tempo
 * - Phase offset and randomization
 * - Bipolar/unipolar output modes
 * - LFO-to-LFO modulation (FM/AM)
 * - Envelope-style one-shot modes
 * - Smooth parameter interpolation
 * - MIDI sync with clock division
 * - Real-time waveform morphing
 * - Low-CPU optimized processing
 */

class AdvancedLFO {
public:
    // Waveform types
    enum class Waveform : uint8_t {
        SINE = 0,           // Classic sine wave
        TRIANGLE,           // Linear triangle
        SAWTOOTH_UP,        // Rising sawtooth
        SAWTOOTH_DOWN,      // Falling sawtooth
        SQUARE,             // Square wave with PWM
        PULSE,              // Pulse wave (narrow square)
        NOISE,              // Smooth noise
        SAMPLE_HOLD,        // Sample & hold (stepped)
        EXPONENTIAL_UP,     // Exponential rise
        EXPONENTIAL_DOWN,   // Exponential fall
        LOGARITHMIC,        // Logarithmic curve
        CUSTOM,             // Custom wavetable
        COUNT
    };
    
    // Sync modes
    enum class SyncMode : uint8_t {
        FREE_RUNNING = 0,   // Internal rate
        TEMPO_SYNC,         // Sync to host tempo
        KEY_SYNC,           // Restart on key press
        ONE_SHOT,           // Single cycle on trigger
        ENVELOPE,           // Envelope-style (ADSR)
        COUNT
    };
    
    // Clock divisions for tempo sync
    enum class ClockDivision : uint8_t {
        FOUR_BARS = 0,      // 4 bars
        TWO_BARS,           // 2 bars
        ONE_BAR,            // 1 bar
        HALF_NOTE,          // 1/2
        QUARTER_NOTE,       // 1/4
        EIGHTH_NOTE,        // 1/8
        SIXTEENTH_NOTE,     // 1/16
        THIRTY_SECOND,      // 1/32
        DOTTED_QUARTER,     // 1/4.
        DOTTED_EIGHTH,      // 1/8.
        QUARTER_TRIPLET,    // 1/4T
        EIGHTH_TRIPLET,     // 1/8T
        SIXTEENTH_TRIPLET,  // 1/16T
        COUNT
    };
    
    struct LFOSettings {
        Waveform waveform = Waveform::SINE;
        SyncMode syncMode = SyncMode::FREE_RUNNING;
        ClockDivision clockDiv = ClockDivision::QUARTER_NOTE;
        
        float rate = 1.0f;              // 0.01 - 100 Hz (free running)
        float depth = 1.0f;             // 0.0 - 1.0 modulation depth
        float offset = 0.0f;            // -1.0 to 1.0 DC offset
        float phase = 0.0f;             // 0.0 - 1.0 phase offset
        
        bool bipolar = true;            // True: -1 to 1, False: 0 to 1
        bool invert = false;            // Invert output
        float pulseWidth = 0.5f;        // 0.1 - 0.9 for pulse/square waves
        float smooth = 0.0f;            // 0.0 - 1.0 smoothing amount
        
        // Advanced modulation
        float fmAmount = 0.0f;          // 0.0 - 1.0 frequency modulation
        float amAmount = 0.0f;          // 0.0 - 1.0 amplitude modulation
        uint8_t fmSource = 255;         // LFO source for FM (255 = none)
        uint8_t amSource = 255;         // LFO source for AM (255 = none)
        
        // Envelope mode (when syncMode == ENVELOPE)
        float attack = 0.1f;            // Attack time in seconds
        float decay = 0.3f;             // Decay time in seconds  
        float sustain = 0.7f;           // Sustain level
        float release = 0.5f;           // Release time in seconds
        
        // Randomization
        float phaseRandom = 0.0f;       // 0.0 - 1.0 random phase offset
        float rateRandom = 0.0f;        // 0.0 - 1.0 random rate variation
        
        bool enabled = true;            // LFO enabled
        bool retrigger = true;          // Restart on note-on
    };
    
    AdvancedLFO();
    ~AdvancedLFO() = default;
    
    // Configuration
    void setSettings(const LFOSettings& settings);
    const LFOSettings& getSettings() const { return settings_; }
    void setSampleRate(float sampleRate);
    void setTempo(float bpm);
    
    // Waveform control
    void setWaveform(Waveform waveform);
    void setCustomWavetable(const float* wavetable, size_t size);
    void morphBetweenWaveforms(Waveform waveA, Waveform waveB, float morph);
    
    // Processing
    float process();                    // Generate next sample
    void processBlock(float* output, int blockSize); // Process block for efficiency
    
    // Control
    void trigger();                     // Manual trigger
    void reset();                       // Reset phase to 0
    void sync();                        // Sync to external clock
    
    // State
    bool isActive() const;
    float getCurrentPhase() const { return phase_; }
    float getCurrentValue() const { return currentValue_; }
    Waveform getCurrentWaveform() const { return settings_.waveform; }
    
    // Modulation inputs (for LFO-to-LFO modulation)
    void setFrequencyModulation(float fmValue);
    void setAmplitudeModulation(float amValue);
    
    // MIDI and automation
    void noteOn(uint8_t velocity = 100);
    void noteOff();
    void setMIDILearnParameter(uint8_t parameter);
    void processMIDICC(uint8_t ccNumber, uint8_t value);
    
private:
    LFOSettings settings_;
    
    // Core state
    float sampleRate_ = 48000.0f;
    float tempo_ = 120.0f;
    float phase_ = 0.0f;
    float phaseIncrement_ = 0.0f;
    float currentValue_ = 0.0f;
    
    // Envelope state (for envelope mode)
    enum class EnvStage { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };
    EnvStage envStage_ = EnvStage::IDLE;
    float envValue_ = 0.0f;
    float envTarget_ = 0.0f;
    float envRate_ = 0.0f;
    
    // Modulation inputs
    float fmInput_ = 0.0f;
    float amInput_ = 0.0f;
    
    // Waveform generation
    std::array<float, 2048> customWavetable_;
    size_t wavetableSize_ = 0;
    
    // Smoothing and interpolation
    float smoothedValue_ = 0.0f;
    float smoothingCoeff_ = 0.999f;
    
    // Random generation
    uint32_t randomSeed_ = 54321;
    float phaseRandomOffset_ = 0.0f;
    float rateRandomMultiplier_ = 1.0f;
    
    // Performance optimization
    static constexpr size_t SINE_TABLE_SIZE = 2048;
    static std::array<float, SINE_TABLE_SIZE> sineTable_;
    static bool sineTableInitialized_;
    
    // Clock sync
    float samplesPerCycle_ = 0.0f;
    uint32_t sampleCounter_ = 0;
    bool clockSynced_ = false;
    
    // Internal methods
    void updatePhaseIncrement();
    void updateEnvelope();
    void initializeSineTable();
    float generateWaveform(Waveform waveform, float phase);
    float interpolateWavetable(const float* table, size_t size, float phase);
    float applySmoothingAndModulation(float rawValue);
    float fastRandom();
    void updateRandomization();
    float getClockDivisionMultiplier() const;
    
    // Waveform generators (optimized)
    inline float generateSine(float phase) const;
    inline float generateTriangle(float phase) const;
    inline float generateSawtooth(float phase) const;
    inline float generateSquare(float phase) const;
    inline float generateNoise();
    inline float generateExponential(float phase, bool rising) const;
    inline float generateLogarithmic(float phase) const;
    
    // Utility
    inline float wrap(float value) const {
        return value - std::floor(value);
    }
    
    inline float bipolarToUnipolar(float bipolar) const {
        return (bipolar + 1.0f) * 0.5f;
    }
    
    inline float unipolarToBipolar(float unipolar) const {
        return unipolar * 2.0f - 1.0f;
    }
};

/**
 * Advanced LFO Manager - handles multiple LFOs with cross-modulation
 */
class AdvancedLFOManager {
public:
    static constexpr int MAX_LFOS = 8;
    
    AdvancedLFOManager();
    ~AdvancedLFOManager() = default;
    
    // LFO management
    AdvancedLFO* getLFO(int index);
    void setLFOSettings(int index, const AdvancedLFO::LFOSettings& settings);
    
    // Global settings
    void setSampleRate(float sampleRate);
    void setTempo(float bpm);
    void setGlobalSync(bool enabled);
    
    // Processing
    void processAll();
    void processBlock(int blockSize);
    
    // Cross-modulation matrix
    void setModulationMatrix(int sourceLFO, int destLFO, float amount);
    float getModulationMatrix(int sourceLFO, int destLFO) const;
    void clearModulationMatrix();
    
    // Preset management
    struct LFOPreset {
        std::array<AdvancedLFO::LFOSettings, MAX_LFOS> lfoSettings;
        std::array<std::array<float, MAX_LFOS>, MAX_LFOS> modMatrix;
        char name[32];
    };
    
    void savePreset(int slot, const char* name);
    bool loadPreset(int slot);
    const char* getPresetName(int slot) const;
    
private:
    std::array<std::unique_ptr<AdvancedLFO>, MAX_LFOS> lfos_;
    std::array<std::array<float, MAX_LFOS>, MAX_LFOS> modMatrix_;
    std::array<LFOPreset, 16> presets_;
    
    bool globalSync_ = false;
    
    void updateCrossModulation();
};

// Factory functions
inline std::unique_ptr<AdvancedLFO> createAdvancedLFO() {
    return std::make_unique<AdvancedLFO>();
}

inline std::unique_ptr<AdvancedLFOManager> createLFOManager() {
    return std::make_unique<AdvancedLFOManager>();
}

// Utility functions for UI
const char* getWaveformName(AdvancedLFO::Waveform waveform);
const char* getSyncModeName(AdvancedLFO::SyncMode mode);
const char* getClockDivisionName(AdvancedLFO::ClockDivision division);
float getClockDivisionValue(AdvancedLFO::ClockDivision division);

} // namespace EtherSynthModulation