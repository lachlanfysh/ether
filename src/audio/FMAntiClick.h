#pragma once
#include <array>
#include <vector>

/**
 * FMAntiClick - Advanced anti-click system for FM synthesis
 * 
 * Features:
 * - Phase continuity preservation during parameter changes
 * - Zero-crossing detection and ramping
 * - Per-operator click suppression
 * - Intelligent ramping based on parameter change magnitude
 * - Real-time safe operation with minimal CPU overhead
 * - Adaptive ramp times based on content analysis
 */
class FMAntiClick {
public:
    enum class RampType {
        LINEAR,         // Linear ramping
        EXPONENTIAL,    // Exponential ramping (more natural)
        ZERO_CROSS,     // Wait for zero crossing
        ADAPTIVE        // Adaptive based on signal content
    };
    
    struct OperatorState {
        float lastPhase = 0.0f;         // Last known phase
        float targetPhase = 0.0f;       // Target phase after parameter change
        float phaseCorrection = 0.0f;   // Phase correction amount
        float lastOutput = 0.0f;        // Last output sample
        float rampProgress = 1.0f;      // Ramp progress (0-1)
        float rampTarget = 1.0f;        // Ramp target level
        bool ramping = false;           // Currently ramping
        bool phaseCorrectActive = false; // Phase correction active
        int zeroCrossCountdown = 0;     // Samples until zero crossing ramp
        float parameterVelocity = 0.0f; // Rate of parameter change
    };
    
    struct GlobalConfig {
        RampType rampType = RampType::ADAPTIVE;
        float minRampTimeMs = 0.5f;     // Minimum ramp time
        float maxRampTimeMs = 5.0f;     // Maximum ramp time
        float clickThreshold = 0.1f;    // Click detection threshold
        bool enablePhaseCorrection = true; // Enable phase correction
        bool enableZeroCrossing = true;    // Enable zero crossing detection
        float adaptiveSpeed = 1.0f;     // Adaptive algorithm speed
        bool enableContentAnalysis = true; // Enable content-based ramping
    };
    
    FMAntiClick();
    ~FMAntiClick();
    
    // Initialization
    bool initialize(float sampleRate, int numOperators = 4);
    void shutdown();
    
    // Configuration
    void setGlobalConfig(const GlobalConfig& config);
    const GlobalConfig& getGlobalConfig() const { return globalConfig_; }
    
    void setOperatorEnabled(int operatorIndex, bool enabled);
    bool isOperatorEnabled(int operatorIndex) const;
    
    // Parameter change detection
    void onParameterChange(int operatorIndex, float oldValue, float newValue, float changeRate = 1.0f);
    void onFrequencyChange(int operatorIndex, float oldFreq, float newFreq);
    void onLevelChange(int operatorIndex, float oldLevel, float newLevel);
    void onPhaseReset(int operatorIndex, float newPhase = 0.0f);
    
    // Audio processing
    float processOperatorSample(int operatorIndex, float input, float currentPhase);
    void processOperatorBlock(int operatorIndex, const float* input, float* output, 
                             const float* phases, int numSamples);
    
    // Analysis
    bool isRamping(int operatorIndex) const;
    float getRampProgress(int operatorIndex) const;
    bool hasClickPotential(int operatorIndex, float newValue, float oldValue) const;
    float getCPUUsage() const { return cpuUsage_; }
    
    // Advanced features
    void updatePhaseCorrection(int operatorIndex, float currentPhase, float targetPhase);
    bool shouldRampParameter(int operatorIndex, float parameterChange) const;
    float calculateOptimalRampTime(int operatorIndex, float parameterChange) const;
    
private:
    // Configuration
    GlobalConfig globalConfig_;
    float sampleRate_ = 44100.0f;
    int numOperators_ = 4;
    bool initialized_ = false;
    
    // Per-operator state
    std::vector<OperatorState> operatorStates_;
    std::vector<bool> operatorEnabled_;
    
    // Zero-crossing detection
    std::vector<float> lastSamples_;
    std::vector<int> zeroCrossCounters_;
    
    // Content analysis
    std::vector<float> signalEnergy_;
    std::vector<float> signalVariance_;
    std::array<float, 32> analysisBuffer_; // Small buffer for content analysis
    int analysisIndex_ = 0;
    
    // Performance monitoring
    float cpuUsage_ = 0.0f;
    
    // Processing methods
    float processLinearRamp(int operatorIndex, float input);
    float processExponentialRamp(int operatorIndex, float input);
    float processZeroCrossRamp(int operatorIndex, float input);
    float processAdaptiveRamp(int operatorIndex, float input);
    
    // Zero-crossing detection
    bool detectZeroCrossing(int operatorIndex, float currentSample);
    void updateZeroCrossCounter(int operatorIndex);
    
    // Phase correction
    void calculatePhaseCorrection(int operatorIndex, float oldPhase, float newPhase);
    float applyPhaseCorrection(int operatorIndex, float input, float phase);
    bool needsPhaseCorrection(float phaseJump) const;
    
    // Content analysis
    void analyzeSignalContent(int operatorIndex, float sample);
    float calculateSignalComplexity(int operatorIndex) const;
    float getAdaptiveRampTime(int operatorIndex, float complexity) const;
    
    // Parameter change analysis
    float calculateParameterVelocity(int operatorIndex, float change, float timeMs);
    bool isParameterChangeSignificant(float change, float velocity) const;
    float getClickProbability(int operatorIndex, float parameterChange) const;
    
    // Ramping algorithms
    void startRamp(int operatorIndex, float targetLevel, float rampTimeMs);
    void updateRampProgress(int operatorIndex);
    float calculateRampValue(int operatorIndex, RampType type) const;
    
    // Utility functions
    float linearInterpolate(float from, float to, float progress) const;
    float exponentialInterpolate(float from, float to, float progress) const;
    float clamp(float value, float min, float max) const;
    float dbToLinear(float db) const;
    float linearToDb(float linear) const;
    
    // Constants
    static constexpr float PHASE_JUMP_THRESHOLD = 0.5f;  // Radians
    static constexpr float CLICK_DETECTION_TIME_MS = 1.0f;
    static constexpr float MIN_RAMP_SAMPLES = 8;
    static constexpr float MAX_RAMP_SAMPLES = 512;
    static constexpr float ZERO_CROSS_TIMEOUT_MS = 10.0f;
    static constexpr float PARAMETER_VELOCITY_SMOOTH = 0.95f;
    static constexpr float CPU_USAGE_SMOOTH = 0.99f;
    
    // Exponential curve coefficients
    static constexpr float EXP_CURVE_FACTOR = 3.0f;
    static constexpr float ADAPTIVE_RESPONSE = 0.1f;
};