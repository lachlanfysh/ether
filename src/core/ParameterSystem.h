#pragma once
#include "Types.h"
#include "../audio/AdvancedParameterSmoother.h"
#include "../control/modulation/VelocityParameterScaling.h"
#include <unordered_map>
#include <memory>
#include <atomic>
#include <mutex>
#include <functional>
#include <vector>

/**
 * UnifiedParameterSystem - Central parameter management for EtherSynth
 * 
 * This system unifies all parameter handling across the synthesizer:
 * - Real-time safe parameter updates
 * - Integrated velocity scaling and modulation
 * - Parameter smoothing and automation
 * - Preset loading/saving integration  
 * - Thread-safe access patterns
 * - Parameter validation and range checking
 * 
 * Design Goals:
 * - Zero-allocation real-time audio thread access
 * - Lockless reads for audio processing
 * - Unified parameter ID system (using ParameterID from Types.h)
 * - Backward compatibility with existing preset JSON format
 * - Integration with existing velocity and modulation systems
 */
class UnifiedParameterSystem {
public:
    // Parameter value with metadata
    struct ParameterValue {
        float value;                    // Current processed value (0.0-1.0)
        float rawValue;                 // Raw input value before processing
        float targetValue;              // Target value for smoothing
        bool hasBeenSet;                // Whether this parameter has been explicitly set
        uint64_t lastUpdateTime;        // Timestamp of last update
        
        ParameterValue() : 
            value(0.0f), rawValue(0.0f), targetValue(0.0f), 
            hasBeenSet(false), lastUpdateTime(0) {}
            
        ParameterValue(float v) : 
            value(v), rawValue(v), targetValue(v), 
            hasBeenSet(true), lastUpdateTime(0) {}
    };
    
    // Parameter configuration and metadata
    struct ParameterConfig {
        ParameterID id;
        std::string name;
        std::string displayName;
        std::string unit;               // Display unit (Hz, %, dB, etc.)
        
        // Value ranges and mapping
        float minValue;                 // Minimum parameter value
        float maxValue;                 // Maximum parameter value  
        float defaultValue;             // Default/initialization value
        bool isLogarithmic;             // Use logarithmic scaling
        bool isBipolar;                 // Centered around midpoint
        float stepSize;                 // Quantization step (0.0 = continuous)
        
        // Behavioral flags
        bool enableVelocityScaling;     // Allow velocity modulation
        bool enableSmoothing;           // Apply parameter smoothing
        bool enableAutomation;          // Allow automation recording
        bool isGlobalParameter;         // Global vs per-instrument
        bool requiresAudioThreadUpdate; // Needs real-time update
        
        // Smoothing configuration
        AdvancedParameterSmoother::SmoothType smoothType;
        AdvancedParameterSmoother::CurveType curveType;
        float smoothTimeMs;             // Custom smoothing time
        
        // Velocity scaling configuration
        VelocityParameterScaling::ParameterCategory velocityCategory;
        float velocityScale;            // Velocity sensitivity multiplier
        
        // Callbacks for parameter changes
        std::function<void(ParameterID, float, float)> onValueChanged; // (id, oldValue, newValue)
        std::function<bool(ParameterID, float)> onValidateValue;       // (id, value) -> isValid
        
        ParameterConfig() : 
            id(ParameterID::VOLUME), name(""), displayName(""), unit(""),
            minValue(0.0f), maxValue(1.0f), defaultValue(0.5f),
            isLogarithmic(false), isBipolar(false), stepSize(0.0f),
            enableVelocityScaling(true), enableSmoothing(true), enableAutomation(true),
            isGlobalParameter(false), requiresAudioThreadUpdate(true),
            smoothType(AdvancedParameterSmoother::SmoothType::AUDIBLE),
            curveType(AdvancedParameterSmoother::CurveType::EXPONENTIAL),
            smoothTimeMs(20.0f),
            velocityCategory(VelocityParameterScaling::ParameterCategory::CUSTOM),
            velocityScale(1.0f) {}
    };
    
    // Parameter update result
    enum class UpdateResult {
        SUCCESS = 0,
        INVALID_PARAMETER,
        VALUE_OUT_OF_RANGE,
        VALIDATION_FAILED,
        SMOOTHING_ACTIVE,
        SYSTEM_LOCKED
    };
    
    UnifiedParameterSystem();
    ~UnifiedParameterSystem();
    
    // System lifecycle
    bool initialize(float sampleRate);
    void shutdown();
    bool isInitialized() const { return initialized_; }
    void setSampleRate(float sampleRate);
    
    // Parameter registration and configuration
    bool registerParameter(const ParameterConfig& config);
    bool registerParameter(ParameterID id, const std::string& name, 
                          float minValue = 0.0f, float maxValue = 1.0f, float defaultValue = 0.5f);
    bool unregisterParameter(ParameterID id);
    bool isParameterRegistered(ParameterID id) const;
    
    // Parameter configuration access
    const ParameterConfig& getParameterConfig(ParameterID id) const;
    bool setParameterConfig(ParameterID id, const ParameterConfig& config);
    std::vector<ParameterID> getRegisteredParameters() const;
    std::vector<ParameterID> getParametersInCategory(VelocityParameterScaling::ParameterCategory category) const;
    
    // Real-time safe parameter access (for audio thread)
    float getParameterValue(ParameterID id) const;
    float getParameterValue(ParameterID id, size_t instrumentIndex) const;
    bool hasParameter(ParameterID id) const;
    bool isParameterSmoothing(ParameterID id) const;
    
    // Parameter updates (thread-safe)
    UpdateResult setParameterValue(ParameterID id, float value);
    UpdateResult setParameterValue(ParameterID id, size_t instrumentIndex, float value);
    UpdateResult setParameterValueImmediate(ParameterID id, float value); // Skip smoothing
    UpdateResult setParameterTarget(ParameterID id, float targetValue);   // Smooth to target
    
    // Batch parameter updates
    UpdateResult setMultipleParameters(const std::vector<std::pair<ParameterID, float>>& parameters);
    UpdateResult setInstrumentParameters(size_t instrumentIndex, const std::unordered_map<ParameterID, float>& parameters);
    
    // Velocity and modulation integration
    UpdateResult setParameterWithVelocity(ParameterID id, float baseValue, float velocity);
    UpdateResult setParameterWithModulation(ParameterID id, float baseValue, float modAmount);
    float calculateVelocityModulation(ParameterID id, float velocity) const;
    
    // Audio processing integration
    void processAudioBlock();  // Call once per audio buffer
    void updateSmoothers();    // Update parameter smoothing
    
    // Preset system integration
    struct PresetData {
        std::unordered_map<ParameterID, float> globalParameters;
        std::array<std::unordered_map<ParameterID, float>, MAX_INSTRUMENTS> instrumentParameters;
        std::string presetName;
        uint32_t version;
    };
    
    bool savePreset(PresetData& preset) const;
    bool loadPreset(const PresetData& preset);
    bool validatePreset(const PresetData& preset) const;
    
    // JSON serialization (maintains compatibility with existing preset format)
    std::string serializeToJSON() const;
    bool deserializeFromJSON(const std::string& json);
    
    // JSON parsing helpers (private methods made public for separate implementation)
    void parseParameterSection(const std::string& sectionContent);
    void parseVelocityConfig(const std::string& jsonStr);
    
    // Parameter automation and recording
    void enableParameterAutomation(ParameterID id, bool enabled);
    bool isParameterAutomationEnabled(ParameterID id) const;
    void recordParameterChange(ParameterID id, float value, uint64_t timestamp);
    void clearParameterAutomation(ParameterID id);
    
    // System state and diagnostics
    void reset();  // Reset all parameters to defaults
    void resetParameter(ParameterID id);
    size_t getParameterCount() const;
    size_t getActiveSmootherCount() const;
    float getSystemCPUUsage() const; // Estimate of parameter processing CPU usage
    
    // Error handling and validation
    bool validateParameterValue(ParameterID id, float value) const;
    float clampParameterValue(ParameterID id, float value) const;
    float quantizeParameterValue(ParameterID id, float value) const;
    std::string getLastError() const;
    
    // Value conversion utilities
    float normalizedToActual(ParameterID id, float normalizedValue) const;
    float actualToNormalized(ParameterID id, float actualValue) const;
    std::string formatParameterValue(ParameterID id, float value) const;
    
    // Thread safety
    void lockParameterUpdates();   // For bulk updates
    void unlockParameterUpdates();
    bool isSystemLocked() const { return systemLocked_.load(); }

private:
    // System state
    std::atomic<bool> initialized_{false};
    std::atomic<bool> systemLocked_{false};
    float sampleRate_ = 48000.0f;
    mutable std::string lastError_;
    
    // Parameter storage - real-time safe access
    std::array<std::atomic<float>, static_cast<size_t>(ParameterID::COUNT)> globalParameters_;
    std::array<std::array<std::atomic<float>, static_cast<size_t>(ParameterID::COUNT)>, MAX_INSTRUMENTS> instrumentParameters_;
    
    // Parameter metadata and configuration
    mutable std::mutex configMutex_;
    std::unordered_map<ParameterID, ParameterConfig> parameterConfigs_;
    std::unordered_map<ParameterID, ParameterValue> parameterValues_;
    
    // Parameter smoothing - one smoother per parameter per instrument
    std::unordered_map<ParameterID, std::unique_ptr<AdvancedParameterSmoother>> globalSmoothers_;
    std::array<std::unordered_map<ParameterID, std::unique_ptr<AdvancedParameterSmoother>>, MAX_INSTRUMENTS> instrumentSmoothers_;
    
    // Velocity scaling integration
    std::unique_ptr<VelocityParameterScaling> velocityScaling_;
    
    // Automation recording
    struct AutomationData {
        std::vector<std::pair<uint64_t, float>> recordedValues;
        bool isRecording;
        bool isEnabled;
        
        AutomationData() : isRecording(false), isEnabled(false) {}
    };
    std::unordered_map<ParameterID, AutomationData> automationData_;
    
    // Default parameter configurations
    static std::unordered_map<ParameterID, ParameterConfig> createDefaultConfigurations();
    void initializeDefaultParameters();
    void setupVelocityScaling();
    
    // Internal parameter processing
    float processParameterValue(ParameterID id, float rawValue, float velocity = 0.0f) const;
    AdvancedParameterSmoother* getSmoother(ParameterID id, size_t instrumentIndex = 0);
    const AdvancedParameterSmoother* getSmoother(ParameterID id, size_t instrumentIndex = 0) const;
    
    // Validation helpers
    bool isValidParameterID(ParameterID id) const;
    bool isValidInstrumentIndex(size_t index) const;
    void updateLastError(const std::string& error) const;
    
    // Performance monitoring
    mutable std::atomic<uint64_t> processingTimeNs_{0};
    mutable std::atomic<uint64_t> updateCount_{0};
    
    // Constants
    static constexpr float EPSILON = 1e-6f;
    static constexpr size_t MAX_AUTOMATION_HISTORY = 1000;
    static constexpr uint64_t CPU_USAGE_WINDOW_MS = 1000;
};

// Global parameter system instance
extern UnifiedParameterSystem g_parameterSystem;

// Convenience macros for common operations
#define GET_PARAM(id) g_parameterSystem.getParameterValue(id)
#define SET_PARAM(id, value) g_parameterSystem.setParameterValue(id, value)
#define GET_INSTRUMENT_PARAM(id, inst) g_parameterSystem.getParameterValue(id, inst)
#define SET_INSTRUMENT_PARAM(id, inst, value) g_parameterSystem.setParameterValue(id, inst, value)