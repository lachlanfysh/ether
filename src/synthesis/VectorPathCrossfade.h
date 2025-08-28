#pragma once
#include "VectorPath.h"
#include <array>
#include <functional>
#include <memory>

/**
 * VectorPathCrossfade - Corner type crossfading for seamless engine transitions
 * 
 * Features:
 * - Independent corner source management with engine assignment
 * - Real-time engine crossfading with equal-power blending
 * - Corner transition morphing with configurable curves
 * - Engine-specific parameter mapping and voice allocation
 * - Seamless transitions without audio dropouts or clicks
 * - Dynamic voice management during engine changes
 * - Preset morphing between different engine configurations
 */
class VectorPathCrossfade {
public:
    // Forward declarations for engine types
    class SynthEngineBase;
    
    enum class EngineType {
        MACRO_VA,           // Virtual Analog engine
        MACRO_FM,           // FM synthesis engine
        MACRO_WAVESHAPER,   // Waveshaper engine
        MACRO_WAVETABLE,    // Wavetable engine
        MACRO_CHORD,        // Chord engine
        MACRO_HARMONICS,    // Harmonics engine
        FORMANT,            // Formant engine
        NOISE,              // Noise engine
        TIDES_OSC,          // Tides oscillator
        RINGS_VOICE,        // Rings voice
        ELEMENTS_VOICE,     // Elements voice
        SLIDE_ACCENT_BASS,  // Slide+Accent Mono Bass
        SERIAL_HP_LP,       // Serial HP→LP Mono
        CLASSIC_4OP_FM      // Classic 4-Op FM
    };
    
    enum class CrossfadeMode {
        INSTANT,            // Immediate engine switch
        LINEAR,             // Linear crossfade
        EQUAL_POWER,        // Equal-power crossfade
        S_CURVE,            // S-curve (smooth) crossfade
        EXPONENTIAL         // Exponential crossfade
    };
    
    struct CornerEngine {
        EngineType engineType = EngineType::MACRO_VA;
        std::shared_ptr<SynthEngineBase> engine = nullptr;
        bool active = false;
        float level = 1.0f;             // Engine output level
        float crossfadeAmount = 0.0f;   // Current crossfade amount (0-1)
        
        // H/T/M parameter mapping for this corner
        float harmonics = 0.5f;
        float timbre = 0.5f;
        float morph = 0.5f;
        
        // Engine-specific parameters
        std::array<float, 16> engineParams; // Extended parameters
        
        // Voice management
        int voiceCount = 0;             // Current active voices
        int maxVoices = 16;             // Maximum voices for this engine
        bool voiceSteal = true;         // Allow voice stealing
    };
    
    struct CrossfadeConfig {
        CrossfadeMode mode = CrossfadeMode::EQUAL_POWER;
        float transitionTime = 0.05f;   // Crossfade time in seconds
        bool preserveVoices = true;     // Keep voices during transition
        bool snapToCorners = false;     // Snap to pure corners when close
        float cornerSnapThreshold = 0.05f; // Distance threshold for snapping
        
        // Morphing curves
        float morphCurve = 0.5f;        // Morph curve shape (0=linear, 1=exponential)
        bool usePerceptualBlending = true; // Use perceptually-tuned blending
        
        // Performance
        bool enableInterpolation = true; // Interpolate between engines
        int interpolationQuality = 2;   // 1=basic, 2=good, 3=best
        bool realTimeUpdate = true;     // Update engines in real-time
    };
    
    struct EngineTransition {
        VectorPath::Corner corner;
        EngineType fromEngine;
        EngineType toEngine;
        float progress = 0.0f;          // Transition progress (0-1)
        float startTime = 0.0f;         // Transition start time
        bool active = false;
        
        // Voice management during transition
        std::vector<int> fadingVoiceIds;
        std::vector<int> newVoiceIds;
    };
    
    using EngineChangeCallback = std::function<void(VectorPath::Corner corner, EngineType oldEngine, EngineType newEngine)>;
    using VoiceAllocationCallback = std::function<bool(EngineType engine, int voiceId)>;
    using CrossfadeCompleteCallback = std::function<void(const std::array<CornerEngine, 4>& corners)>;
    
    VectorPathCrossfade();
    ~VectorPathCrossfade();
    
    // Initialization
    bool initialize(VectorPath* vectorPath);
    void shutdown();
    
    // Corner engine management
    void setCornerEngine(VectorPath::Corner corner, EngineType engineType);
    EngineType getCornerEngine(VectorPath::Corner corner) const;
    void setCornerEngineParams(VectorPath::Corner corner, float harmonics, float timbre, float morph);
    void getCornerEngineParams(VectorPath::Corner corner, float& harmonics, float& timbre, float& morph) const;
    
    // Engine instances
    std::shared_ptr<SynthEngineBase> getEngine(VectorPath::Corner corner) const;
    void setEngine(VectorPath::Corner corner, std::shared_ptr<SynthEngineBase> engine);
    
    // Crossfade configuration
    void setCrossfadeConfig(const CrossfadeConfig& config);
    const CrossfadeConfig& getCrossfadeConfig() const { return config_; }
    
    // Engine transitions
    void transitionCornerEngine(VectorPath::Corner corner, EngineType newEngine, float transitionTime = 0.0f);
    bool isTransitionActive(VectorPath::Corner corner) const;
    float getTransitionProgress(VectorPath::Corner corner) const;
    void cancelTransition(VectorPath::Corner corner);
    void cancelAllTransitions();
    
    // Preset morphing
    void morphToPreset(const std::array<EngineType, 4>& targetEngines, 
                      const std::array<std::array<float, 3>, 4>& targetParams,
                      float morphTime = 1.0f);
    void saveCurrentAsPreset(const std::string& name);
    bool loadPreset(const std::string& name);
    
    // Voice management
    void setMaxVoices(VectorPath::Corner corner, int maxVoices);
    int getActiveVoices(VectorPath::Corner corner) const;
    int getTotalActiveVoices() const;
    void setVoiceStealingEnabled(VectorPath::Corner corner, bool enabled);
    
    // Real-time processing
    void updateCrossfade(const VectorPath::Position& position, const VectorPath::CornerBlend& blend);
    void processAudio(float* outputL, float* outputR, int numSamples);
    void processParameters(float deltaTimeMs);
    
    // Analysis
    float getCornerActivity(VectorPath::Corner corner) const;
    std::array<float, 4> getEngineBlendWeights() const;
    EngineType getDominantEngine() const;
    float getCrossfadeComplexity() const; // Measure of blend complexity
    
    // Callbacks
    void setEngineChangeCallback(EngineChangeCallback callback) { engineChangeCallback_ = callback; }
    void setVoiceAllocationCallback(VoiceAllocationCallback callback) { voiceAllocationCallback_ = callback; }
    void setCrossfadeCompleteCallback(CrossfadeCompleteCallback callback) { crossfadeCompleteCallback_ = callback; }
    
    // Utility
    static std::string getEngineTypeName(EngineType type);
    static std::array<std::string, 3> getEngineParameterNames(EngineType type);
    static bool isEngineCompatible(EngineType engine1, EngineType engine2);
    
private:
    // Core state
    VectorPath* vectorPath_ = nullptr;
    bool initialized_ = false;
    
    // Corner engines
    std::array<CornerEngine, 4> cornerEngines_;
    std::array<EngineTransition, 4> activeTransitions_;
    
    // Configuration
    CrossfadeConfig config_;
    
    // Current state
    VectorPath::Position lastPosition_;
    VectorPath::CornerBlend lastBlend_;
    std::array<float, 4> engineLevels_;
    float totalEngineLevel_ = 1.0f;
    
    // Voice management
    int totalVoiceCount_ = 0;
    int globalMaxVoices_ = 64;
    bool globalVoiceSteal_ = true;
    
    // Transition state
    bool anyTransitionActive_ = false;
    uint32_t lastUpdateTime_ = 0;
    
    // Morphing state
    struct PresetMorph {
        bool active = false;
        float progress = 0.0f;
        float duration = 1.0f;
        uint32_t startTime = 0;
        
        std::array<EngineType, 4> startEngines;
        std::array<EngineType, 4> targetEngines;
        std::array<std::array<float, 3>, 4> startParams;
        std::array<std::array<float, 3>, 4> targetParams;
    } presetMorph_;
    
    // Callbacks
    EngineChangeCallback engineChangeCallback_;
    VoiceAllocationCallback voiceAllocationCallback_;
    CrossfadeCompleteCallback crossfadeCompleteCallback_;
    
    // Private methods
    void initializeCornerEngines();
    void updateEngineTransitions(float deltaTimeMs);
    void updatePresetMorph(float deltaTimeMs);
    void updateEngineBlending();
    void updateVoiceAllocation();
    
    // Crossfade calculations
    float calculateCrossfadeAmount(float blendWeight, CrossfadeMode mode) const;
    std::array<float, 4> calculateEqualPowerBlend(const VectorPath::CornerBlend& blend) const;
    std::array<float, 4> calculatePerceptualBlend(const VectorPath::CornerBlend& blend) const;
    float applyCrossfadeCurve(float t, CrossfadeMode mode) const;
    
    // Engine management
    void activateEngine(VectorPath::Corner corner);
    void deactivateEngine(VectorPath::Corner corner);
    void reallocateVoices();
    bool requestVoice(VectorPath::Corner corner, int voiceId);
    void releaseVoice(VectorPath::Corner corner, int voiceId);
    
    // Transition helpers
    void startEngineTransition(VectorPath::Corner corner, EngineType newEngine, float duration);
    void updateEngineTransition(EngineTransition& transition, float deltaTimeMs);
    void completeEngineTransition(VectorPath::Corner corner);
    
    // Parameter interpolation
    void interpolateEngineParameters(VectorPath::Corner corner, float amount);
    void updateEngineParametersFromHTM(VectorPath::Corner corner);
    void mapHTMToEngineParams(EngineType engineType, float h, float t, float m, std::array<float, 16>& params);
    
    // Corner snapping
    bool shouldSnapToCorner(const VectorPath::CornerBlend& blend, VectorPath::Corner& dominantCorner) const;
    void applyCornerSnapping(VectorPath::CornerBlend& blend, VectorPath::Corner dominantCorner) const;
    
    // Audio processing helpers
    void processEngineAudio(VectorPath::Corner corner, float* outputL, float* outputR, int numSamples, float level);
    void mixEngineOutputs(float* outputL, float* outputR, int numSamples);
    void applyGlobalNormalization(float* outputL, float* outputR, int numSamples);
    
    // Utility functions
    uint32_t getTimeMs() const;
    float lerp(float a, float b, float t) const;
    float smoothStep(float edge0, float edge1, float x) const;
    float exponentialCurve(float t, float shape) const;
    float equalPowerGain(float position) const;
    
    // Engine factory
    std::shared_ptr<SynthEngineBase> createEngine(EngineType type);
    void destroyEngine(std::shared_ptr<SynthEngineBase> engine);
    
    // Constants
    static constexpr float MIN_CROSSFADE_TIME = 0.001f;    // 1ms minimum
    static constexpr float MAX_CROSSFADE_TIME = 10.0f;     // 10s maximum
    static constexpr float CORNER_SNAP_HYSTERESIS = 0.02f; // Hysteresis for corner snapping
    static constexpr int MAX_TOTAL_VOICES = 128;           // Maximum total voices
    static constexpr float VOICE_STEAL_THRESHOLD = 0.1f;   // Minimum level for voice stealing
};

/**
 * Base class for synthesis engines used in Vector Path crossfading
 */
class VectorPathCrossfade::SynthEngineBase {
public:
    virtual ~SynthEngineBase() = default;
    
    // Engine identification
    virtual EngineType getEngineType() const = 0;
    virtual std::string getEngineName() const = 0;
    
    // Parameter control
    virtual void setHTMParameters(float harmonics, float timbre, float morph) = 0;
    virtual void getHTMParameters(float& harmonics, float& timbre, float& morph) const = 0;
    virtual void setEngineParameter(int paramIndex, float value) = 0;
    virtual float getEngineParameter(int paramIndex) const = 0;
    
    // Voice management
    virtual int startVoice(int voiceId, float note, float velocity) = 0;
    virtual void stopVoice(int voiceId, float releaseTime = 0.0f) = 0;
    virtual void updateVoice(int voiceId, float note, float velocity) = 0;
    virtual bool isVoiceActive(int voiceId) const = 0;
    virtual int getActiveVoiceCount() const = 0;
    
    // Audio processing
    virtual void processAudio(float* outputL, float* outputR, int numSamples) = 0;
    virtual void processParameters(float deltaTimeMs) = 0;
    
    // Engine state
    virtual void activate() = 0;
    virtual void deactivate() = 0;
    virtual bool isActive() const = 0;
    virtual void reset() = 0;
    
    // Performance
    virtual float getCPUUsage() const = 0;
    virtual void setQuality(int quality) = 0; // 1=draft, 2=good, 3=best
    
protected:
    // Engine state
    bool active_ = false;
    float harmonics_ = 0.5f;
    float timbre_ = 0.5f;
    float morph_ = 0.5f;
    std::array<float, 16> engineParams_;
    int voiceCount_ = 0;
    int maxVoices_ = 16;
};