#pragma once
#include "../core/Types.h"
#include <array>
#include <cstdint>

/**
 * GlobalLFOSystem - Centralized LFO management for EtherSynth
 * 
 * Provides 8 LFOs per slot with comprehensive waveform generation,
 * sync modes, and parameter assignment system. Optimized for 
 * block-based processing with minimal CPU overhead.
 */
class GlobalLFOSystem {
public:
    static constexpr int MAX_SLOTS = 16;
    static constexpr int MAX_LFOS = 8;
    static constexpr int MAX_PARAMS = static_cast<int>(ParameterID::COUNT);
    
    // Waveform types
    enum class Waveform : uint8_t {
        Sine = 0,
        Tri,
        SawUp,
        SawDown,
        Square,
        Pulse,
        SampleHold,
        Noise,
        ExpUp,
        ExpDown,
        COUNT
    };
    
    // Sync modes
    enum class SyncMode : uint8_t {
        Free = 0,     // Free running at rateHz
        Tempo,        // Synced to BPM with clock division
        Key,          // Restart on key trigger
        OneShot,      // Single cycle on trigger
        Envelope,     // ADSR envelope mode
        COUNT
    };
    
    // LFO configuration
    struct LFOSettings {
        Waveform wave = Waveform::Sine;
        SyncMode sync = SyncMode::Free;
        float rateHz = 1.0f;        // Free mode rate (0.01-50 Hz)
        int clockDiv = 0;           // Tempo division: 0=OFF, 1=1/1, 2=1/2, 3=1/4, 4=1/8, 5=1/16
        float depth = 0.0f;         // Output depth (0-1)
        float pulseWidth = 0.5f;    // Pulse/Square duty cycle (0.1-0.9)
        
        // Envelope mode parameters
        float envA = 0.01f;         // Attack time (s)
        float envD = 0.2f;          // Decay time (s)
        float envS = 0.7f;          // Sustain level (0-1)
        float envR = 0.2f;          // Release time (s)
    };
    
    GlobalLFOSystem();
    ~GlobalLFOSystem() = default;
    
    // System configuration
    void init(float sampleRate, float bpm);
    void setBPM(float bpm);
    void setSampleRate(float sampleRate);
    
    // LFO control
    void setLFO(int slot, int idx, const LFOSettings& settings);
    void getLFO(int slot, int idx, LFOSettings& settings) const;
    
    // Parameter assignment
    void assign(int slot, int paramId, int idx, float depth);
    void unassign(int slot, int paramId, int idx);
    void clearAssignments(int slot, int paramId);
    
    // Triggers
    void retrigger(int slot);                    // Trigger all KEY/ONESHOT/ENV LFOs
    void retriggerLFO(int slot, int idx);       // Trigger specific LFO
    void releaseEnvelopes(int slot);            // Release all envelope LFOs
    
    // Block processing
    void stepBlock(int slot, int frames);       // Update LFO values for block
    
    // Value queries
    float combinedValue(int slot, int paramId) const;  // Sum of assigned LFO contributions
    uint8_t mask(int slot, int paramId) const;         // Assignment mask
    float getLFOValue(int slot, int idx) const;        // Individual LFO value
    bool isActive(int slot, int idx) const;            // LFO activity status
    
private:
    // Internal LFO state
    struct LFOState {
        LFOSettings settings;
        float phase = 0.0f;                     // Current phase (0-2π or 0-1)
        float lastValue = 0.0f;                 // Current output (-1 to +1)
        bool active = true;                     // LFO enabled
        
        // Envelope state
        enum EnvPhase { ENV_IDLE, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE };
        EnvPhase envPhase = ENV_IDLE;
        float envLevel = 0.0f;                  // Current envelope level
        float envRate = 0.0f;                   // Current envelope rate
        
        // Sample & Hold state
        float holdValue = 0.0f;                 // Current S&H value
        float lastPhase = 0.0f;                 // Previous phase for S&H trigger
        
        // Noise smoothing
        float noiseTarget = 0.0f;               // Target noise value
        float noiseSmooth = 0.0f;               // Smoothed noise output
    };
    
    // Parameter assignment
    struct ParamAssignment {
        uint8_t mask = 0;                       // Bit mask of assigned LFOs
        std::array<float, MAX_LFOS> depths{};   // Assignment depths
    };
    
    // System state
    float sampleRate_ = 48000.0f;
    float bpm_ = 120.0f;
    
    // Per-slot data
    std::array<std::array<LFOState, MAX_LFOS>, MAX_SLOTS> lfoStates_;
    std::array<std::array<ParamAssignment, MAX_PARAMS>, MAX_SLOTS> assignments_;
    
    // Clock division mapping
    static constexpr float CLOCK_DIVISIONS[] = {
        0.0f,   // 0: OFF
        1.0f,   // 1: 1/1 (whole note)
        2.0f,   // 2: 1/2 (half note)
        4.0f,   // 3: 1/4 (quarter note)
        8.0f,   // 4: 1/8 (eighth note)
        16.0f   // 5: 1/16 (sixteenth note)
    };
    static constexpr int MAX_CLOCK_DIVS = 6;
    
    // Internal methods
    void updateLFO(LFOState& lfo, int frames);
    float generateWaveform(Waveform wave, float phase, float pulseWidth);
    void updateEnvelope(LFOState& lfo, int frames);
    float calculatePhaseIncrement(const LFOState& lfo, int frames);
    
    // Waveform generators
    float generateSine(float phase);
    float generateTriangle(float phase);
    float generateSawtooth(float phase, bool rising);
    float generateSquare(float phase, float pulseWidth);
    float generateSampleHold(LFOState& lfo);
    float generateNoise(LFOState& lfo);
    float generateExponential(float phase, bool rising);
    
    // Utility functions
    float clamp(float value, float min = -1.0f, float max = 1.0f);
    bool isValidSlot(int slot) const;
    bool isValidLFO(int idx) const;
    bool isValidParam(int paramId) const;
};