#pragma once
#include <array>
#include <cmath>
#include <memory>

/**
 * LUFSNormalizer - EBU R128/ITU-R BS.1770 compliant loudness normalization
 * 
 * Implements real-time LUFS (Loudness Units relative to Full Scale) measurement
 * and automatic gain compensation to maintain consistent perceived loudness
 * across different H/T/M parameter settings and engine types.
 * 
 * Features:
 * - EBU R128 compliant K-weighted loudness measurement
 * - Integrated loudness tracking with configurable time windows
 * - Target loudness: -23 LUFS (broadcast standard) with ±1dB tolerance
 * - Real-time gain compensation with smooth ramping
 * - Low CPU overhead suitable for embedded STM32 H7 deployment
 * - Stereo and mono input support
 */
class LUFSNormalizer {
public:
    LUFSNormalizer();
    ~LUFSNormalizer() = default;
    
    // Initialization
    bool initialize(float sampleRate, bool stereoInput = true);
    void shutdown();
    
    // Processing
    float processSample(float input);
    void processStereoSample(float& left, float& right);
    void processBlock(float* buffer, int numSamples, bool stereo = false);
    void processStereoBlock(float* leftChannel, float* rightChannel, int numSamples);
    
    // Configuration
    void setTargetLUFS(float targetLUFS = -23.0f);
    void setIntegrationTime(float timeSeconds = 3.0f); // Measurement window
    void setMaxGainReduction(float maxReductionDB = 12.0f);
    void setMaxGainBoost(float maxBoostDB = 6.0f);
    void setGainSmoothingTime(float timeMs = 50.0f);
    void setBypass(bool bypass);
    void setSampleRate(float sampleRate);
    
    // Analysis
    float getCurrentLUFS() const;
    float getIntegratedLUFS() const;
    float getCurrentGain() const { return currentGain_; }
    float getTargetGain() const { return targetGain_; }
    bool isBypassed() const { return bypassed_; }
    bool isInitialized() const { return initialized_; }
    
    // Calibration for H/T/M=50% reference
    void calibrateReference(); // Call when H/T/M all at 50%
    void resetCalibration();
    
    // State management
    void reset();
    
    // Performance monitoring
    float getCPUUsage() const { return cpuUsage_; }
    
private:
    float sampleRate_ = 44100.0f;
    bool stereoInput_ = true;
    bool bypassed_ = false;
    bool initialized_ = false;
    
    // LUFS measurement parameters
    float targetLUFS_ = -23.0f;
    float integrationTimeSeconds_ = 3.0f;
    float maxGainReductionDB_ = 12.0f;
    float maxGainBoostDB_ = 6.0f;
    float gainSmoothingTimeMs_ = 50.0f;
    
    // K-weighting filter states (EBU R128 standard)
    struct KWeightingFilter {
        // Pre-filter (high-pass ~38Hz)
        float preX1 = 0.0f, preX2 = 0.0f;
        float preY1 = 0.0f, preY2 = 0.0f;
        
        // High-frequency shelf filter
        float shelfX1 = 0.0f, shelfX2 = 0.0f;
        float shelfY1 = 0.0f, shelfY2 = 0.0f;
    };
    
    KWeightingFilter leftKFilter_;
    KWeightingFilter rightKFilter_;
    
    // K-weighting filter coefficients
    struct FilterCoeffs {
        float b0, b1, b2, a1, a2;
    };
    FilterCoeffs preFilterCoeffs_;
    FilterCoeffs shelfFilterCoeffs_;
    
    // Loudness integration
    static constexpr int INTEGRATION_BUFFER_SIZE = 192000; // ~4.4 seconds at 44.1kHz
    std::array<float, INTEGRATION_BUFFER_SIZE> loudnessBuffer_;
    int bufferIndex_ = 0;
    bool bufferFull_ = false;
    
    // Running statistics
    float instantaneousLoudness_ = -70.0f;
    float integratedLoudness_ = -70.0f;
    float meanSquareSum_ = 0.0f;
    int integrationSamples_ = 0;
    
    // Gain control
    float currentGain_ = 1.0f;
    float targetGain_ = 1.0f;
    float gainSmoothingCoeff_ = 0.99f;
    
    // Reference calibration
    bool hasReference_ = false;
    float referenceLUFS_ = -23.0f;
    
    // Performance monitoring
    mutable float cpuUsage_ = 0.0f;
    
    // Internal methods
    void calculateFilterCoefficients();
    float processKWeighting(float input, KWeightingFilter& filter) const;
    float calculateInstantaneousLoudness(float leftK, float rightK) const;
    void updateIntegratedLoudness(float instantaneous);
    void updateTargetGain();
    float lufsToLinear(float lufs) const;
    float linearToLUFS(float linear) const;
    float clamp(float value, float min, float max) const;
    
    // Constants
    static constexpr float MIN_LUFS = -70.0f;
    static constexpr float MAX_LUFS = 0.0f;
    static constexpr float LUFS_REFERENCE = -23.0f; // EBU R128 reference
    static constexpr float GATE_THRESHOLD = -70.0f; // Gating threshold for integration
    static constexpr float PI = 3.14159265358979323846f;
};