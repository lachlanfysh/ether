#pragma once
#include <vector>
#include <array>
#include <random>
#include <cmath>
#include <algorithm>

/**
 * GranularFX - Clouds-like Granular Effects Processor
 * 
 * Features:
 * - Stereo circular capture buffer (2-4 seconds configurable)
 * - Real-time grain generation with Hann↔Tukey windowing
 * - Position control with jitter randomization  
 * - Pitch shifting via resampling (-24 to +24 semitones)
 * - Stereo spread with per-grain equal-power panning
 * - Freeze mode stops capture, renders from frozen buffer
 * - Feedback/smear reinjection with LPF
 * - Return tone shaping (HPF/LPF)
 * - Block-based scheduling for CPU efficiency
 */
class GranularFX {
public:
    // Parameter indices for external API
    enum ParamIndex {
        PARAM_SIZE = 0,         // Grain size 10-500ms (log)
        PARAM_DENSITY,          // Grain density 2-50 grains/s (log) 
        PARAM_POSITION,         // Position 0-1 in capture buffer
        PARAM_JITTER,           // Position/timing jitter 0-1
        PARAM_PITCH,            // Pitch shift -24 to +24 semitones
        PARAM_SPREAD,           // Stereo spread 0-1
        PARAM_TEXTURE,          // Hann↔Tukey blend 0-1
        PARAM_FEEDBACK,         // Feedback amount 0-1
        PARAM_FREEZE,           // Freeze capture 0/1
        PARAM_WET,              // Wet level 0-1
        PARAM_RETURN_HPF,       // Return HPF 20-600Hz (exp)
        PARAM_RETURN_LPF,       // Return LPF 1k-18kHz (exp)
        PARAM_SYNC_DIVISION,    // Sync division 0-5 (0=off)
        PARAM_RAND_PITCH,       // Random pitch ±3 semitones
        PARAM_RAND_TIME,        // Random time ±20%
        PARAM_QUALITY,          // Grain cap 16-128 per block
        PARAM_COUNT
    };

    GranularFX();
    ~GranularFX() = default;

    // Configuration
    void setSampleRate(float sampleRate);
    void setBufferSize(size_t bufferSize);
    void setParameter(ParamIndex param, float value); // 0-1 normalized
    float getParameter(ParamIndex param) const;
    
    // Processing
    void process(const float* inputL, const float* inputR, 
                float* outputL, float* outputR, size_t blockSize);
    
    // Utility
    void reset();
    const char* getParameterName(ParamIndex param) const;
    
private:
    // Grain structure
    struct Grain {
        bool active = false;
        float bufferPosL = 0.0f;        // Read position in capture buffer (left)
        float bufferPosR = 0.0f;        // Read position in capture buffer (right)
        float phaseInc = 1.0f;          // Pitch shift rate
        float windowPhase = 0.0f;       // Window envelope position 0-1
        float windowInc = 0.0f;         // Window phase increment
        float panL = 1.0f;              // Left pan gain
        float panR = 1.0f;              // Right pan gain
        float amplitude = 1.0f;         // Grain amplitude
    };
    
    // Parameters (0-1 normalized)
    std::array<float, PARAM_COUNT> params_;
    
    // Audio buffers
    std::vector<float> captureBufferL_;
    std::vector<float> captureBufferR_;
    size_t captureIndex_ = 0;
    size_t captureSize_ = 0;
    bool captureActive_ = true;
    
    // Grain management
    static constexpr size_t MAX_GRAINS = 128;
    std::array<Grain, MAX_GRAINS> grains_;
    size_t activeGrains_ = 0;
    float grainTimer_ = 0.0f;           // Time accumulator for scheduling
    
    // Return filters (simple one-pole)
    struct SimpleFilter {
        float state = 0.0f;
        float coeff = 0.0f;
        void setCoeff(float c) { coeff = std::clamp(c, 0.0f, 0.999f); }
        float process(float input) { 
            state += (input - state) * coeff;
            return state;
        }
    };
    SimpleFilter returnHPF_L_, returnHPF_R_;
    SimpleFilter returnLPF_L_, returnLPF_R_;
    
    // Feedback state
    float feedbackGain_ = 0.0f;
    SimpleFilter feedbackLPF_L_, feedbackLPF_R_;
    
    // Random generation
    std::mt19937 rng_;
    std::uniform_real_distribution<float> uniform_{-1.0f, 1.0f};
    
    // System state
    float sampleRate_ = 48000.0f;
    size_t bufferSize_ = 256;
    
    // Internal methods
    void updateCaptureBuffer(const float* inputL, const float* inputR, size_t blockSize);
    void scheduleGrains(size_t blockSize);
    void processGrains(float* outputL, float* outputR, size_t blockSize);
    void applyReturnFiltering(float* outputL, float* outputR, size_t blockSize);
    void applyFeedback(const float* wetL, const float* wetR, size_t blockSize);
    
    // Parameter mapping utilities
    float mapSize(float norm) const;        // 10-500ms log mapping
    float mapDensity(float norm) const;     // 2-50 grains/s log mapping  
    float mapPitch(float norm) const;       // -24 to +24 semitones
    float mapReturnHPF(float norm) const;   // 20-600Hz exp mapping
    float mapReturnLPF(float norm) const;   // 1k-18kHz exp mapping
    int mapQuality(float norm) const;       // 16-128 grain cap
    
    // Grain generation helpers
    float generateWindow(float phase, float texture) const;
    std::pair<float, float> generatePanGains(float spread) const;
    float getInterpolatedSample(const std::vector<float>& buffer, float position) const;
    
    // Timing and sync
    float calculateGrainInterval() const;
    void initializeDefaultParams();
};