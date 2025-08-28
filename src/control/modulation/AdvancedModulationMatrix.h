#pragma once
#include "../core/Types.h"
#include <vector>
#include <functional>
#include <map>

/**
 * Advanced Modulation Matrix
 * Route any modulation source to any parameter with sophisticated processing
 */
class AdvancedModulationMatrix {
public:
    AdvancedModulationMatrix();
    ~AdvancedModulationMatrix();
    
    // Modulation sources
    enum class ModSource {
        // Hardware sources
        SMART_KNOB = 0,
        TOUCH_X,
        TOUCH_Y,
        AFTERTOUCH,
        VELOCITY,
        
        // Internal sources
        LFO_1,
        LFO_2,
        LFO_3,
        ENVELOPE_1,
        ENVELOPE_2,
        ENVELOPE_3,
        RANDOM,
        
        // Audio-derived sources
        AUDIO_LEVEL,
        AUDIO_PITCH,
        AUDIO_BRIGHTNESS,
        
        // Performance sources
        NOTE_NUMBER,
        NOTE_ON_TIME,
        VOICE_COUNT,
        
        // User-defined
        MACRO_1,
        MACRO_2,
        MACRO_3,
        MACRO_4,
        
        COUNT
    };
    
    // Modulation processing types
    enum class ModProcessing {
        DIRECT = 0,        // Direct mapping
        INVERTED,          // 1 - source
        RECTIFIED,         // |source|
        QUANTIZED,         // Stepped values
        SMOOTHED,          // Smoothing filter
        SAMPLE_HOLD,       // Sample and hold
        CURVE_EXPONENTIAL, // Exponential curve
        CURVE_LOGARITHMIC, // Logarithmic curve
        CURVE_S_SHAPE,     // S-shaped curve
        COUNT
    };
    
    // Modulation slot
    struct ModulationSlot {
        ModSource source = ModSource::SMART_KNOB;
        ParameterID destination = ParameterID::VOLUME;
        float amount = 0.0f;          // -1.0 to 1.0
        float offset = 0.0f;          // Base offset
        ModProcessing processing = ModProcessing::DIRECT;
        bool enabled = false;
        
        // Advanced features
        float rateMultiplier = 1.0f;  // For LFO/envelope sources
        float phaseOffset = 0.0f;     // Phase offset for rhythmic sources
        float threshold = 0.0f;       // Threshold for triggering
        bool bipolar = true;          // Bipolar (-1 to 1) or unipolar (0 to 1)
        
        // Curve shaping
        float curveAmount = 0.0f;     // Curve intensity
        float responseTime = 0.0f;    // Smoothing/lag time
        
        // Conditional modulation
        ModSource condition = ModSource::COUNT; // Optional condition source
        float conditionThreshold = 0.5f;        // Threshold for condition
        bool conditionInvert = false;           // Invert condition logic
        
        uint32_t id = 0;              // Unique identifier
    };
    
    // Core operations
    void addModulation(ModSource source, ParameterID destination, float amount);
    void removeModulation(uint32_t slotId);
    void clearAllModulations();
    
    // Modulation processing
    void processFrame();              // Process one audio frame
    void updateSourceValues();       // Update all source values
    float getModulatedValue(ParameterID param, float baseValue) const;
    
    // Source value management
    void setSourceValue(ModSource source, float value);
    float getSourceValue(ModSource source) const;
    void setSourceEnabled(ModSource source, bool enabled);
    
    // Modulation slot management
    std::vector<ModulationSlot> getModulationsForParameter(ParameterID param) const;
    std::vector<ModulationSlot> getModulationsFromSource(ModSource source) const;
    ModulationSlot* getModulationSlot(uint32_t slotId);
    const ModulationSlot* getModulationSlot(uint32_t slotId) const;
    
    // Advanced features
    void setGlobalModulationAmount(float amount);  // Global mod wheel equivalent
    float getGlobalModulationAmount() const { return globalModAmount_; }
    
    // Macro controls (combine multiple sources)
    void defineMacro(ModSource macro, const std::vector<std::pair<ModSource, float>>& sources);
    void clearMacro(ModSource macro);
    
    // LFO management
    struct LFO {
        enum class Waveform { SINE, TRIANGLE, SAW_UP, SAW_DOWN, SQUARE, RANDOM, COUNT };
        
        Waveform waveform = Waveform::SINE;
        float frequency = 1.0f;       // Hz
        float phase = 0.0f;           // Current phase
        float amplitude = 1.0f;       // Output amplitude
        float offset = 0.0f;          // DC offset
        bool syncToNote = false;      // Sync to note on
        bool enabled = true;
        
        float process(float deltaTime);
        void reset() { phase = 0.0f; }
        void sync() { phase = 0.0f; }
    };
    
    LFO* getLFO(int index);           // Get LFO 1-3
    void syncAllLFOs();               // Sync all LFOs to beat
    
    // Envelope followers for audio-derived modulation
    struct EnvelopeFollower {
        float attack = 0.01f;         // Attack time
        float release = 0.1f;         // Release time
        float level = 0.0f;           // Current level
        
        float process(float input, float deltaTime);
    };
    
    EnvelopeFollower* getEnvelopeFollower(int index);
    void processAudioInput(const EtherAudioBuffer& audioBuffer);
    
    // Modulation visualization and analysis
    struct ModulationInfo {
        ModSource source;
        ParameterID destination;
        float currentValue;
        float amount;
        bool active;
        std::string description;
    };
    
    std::vector<ModulationInfo> getActiveModulations() const;
    float getModulationActivity() const;  // Overall modulation activity level
    
    // Preset management
    void saveMatrix(std::vector<uint8_t>& data) const;
    bool loadMatrix(const std::vector<uint8_t>& data);
    void resetToDefault();
    
    // Performance optimization
    void setUpdateRate(float hz);     // Modulation update rate
    void enableSmartUpdates(bool enable); // Only update when sources change
    
    // Utility functions
    static std::string getSourceName(ModSource source);
    static std::string getProcessingName(ModProcessing processing);
    float applyProcessing(float value, ModProcessing processing, float curveAmount = 0.0f) const;
    
    // Callbacks for parameter changes
    std::function<void(ParameterID, float)> onParameterChange;

private:
    // Storage
    std::vector<ModulationSlot> modSlots_;
    std::array<float, static_cast<size_t>(ModSource::COUNT)> sourceValues_;
    std::array<bool, static_cast<size_t>(ModSource::COUNT)> sourceEnabled_;
    
    // LFOs
    std::array<LFO, 3> lfos_;
    
    // Envelope followers
    std::array<EnvelopeFollower, 3> envelopeFollowers_;
    
    // Macro definitions
    std::map<ModSource, std::vector<std::pair<ModSource, float>>> macros_;
    
    // Global settings
    float globalModAmount_ = 1.0f;
    float updateRate_ = 1000.0f;      // Hz
    bool smartUpdates_ = true;
    
    // Performance tracking
    uint32_t nextSlotId_ = 1;
    float lastUpdateTime_ = 0.0f;
    float updateInterval_ = 0.001f;   // 1ms default
    
    // Audio analysis
    float audioLevel_ = 0.0f;
    float audioPitch_ = 440.0f;
    float audioBrightness_ = 0.5f;
    
    // Helper methods
    uint32_t generateSlotId();
    void updateMacros();
    void updateLFOs(float deltaTime);
    void updateAudioDerivedSources();
    void applyConditionalModulation(ModulationSlot& slot);
    float getCurrentTime() const;
    
    // Curve processing functions
    float applyCurve(float value, ModProcessing processing, float amount) const;
    float exponentialCurve(float value, float amount) const;
    float logarithmicCurve(float value, float amount) const;
    float sCurve(float value, float amount) const;
    
    // Audio analysis helpers
    void analyzeAudioLevel(const EtherAudioBuffer& buffer);
    void analyzeAudioPitch(const EtherAudioBuffer& buffer);
    void analyzeAudioBrightness(const EtherAudioBuffer& buffer);
    
    // Smoothing filters for responsive parameters
    struct SmoothingFilter {
        float value = 0.0f;
        float target = 0.0f;
        float smoothTime = 0.1f;
        
        float process(float input, float deltaTime);
    };
    
    mutable std::map<uint32_t, SmoothingFilter> smoothingFilters_;
};

// Predefined modulation templates
namespace ModulationTemplates {
    // Classic setups
    void setupClassicFilter(AdvancedModulationMatrix& matrix);      // LFO -> Filter
    void setupClassicTremolo(AdvancedModulationMatrix& matrix);     // LFO -> Volume
    void setupClassicVibrato(AdvancedModulationMatrix& matrix);     // LFO -> Pitch
    
    // Performance setups
    void setupPerformanceTouch(AdvancedModulationMatrix& matrix);  // Touch -> Multiple params
    void setupSmartKnobMacro(AdvancedModulationMatrix& matrix);    // Smart knob -> Macro control
    void setupAftertouchExpression(AdvancedModulationMatrix& matrix); // Aftertouch -> Expression
    
    // Audio-reactive setups
    void setupAudioReactive(AdvancedModulationMatrix& matrix);     // Audio level -> Multiple params
    void setupSpectralModulation(AdvancedModulationMatrix& matrix); // Audio analysis -> Parameters
    
    // Experimental setups
    void setupChaoticModulation(AdvancedModulationMatrix& matrix); // Random -> Everything
    void setupRhythmicModulation(AdvancedModulationMatrix& matrix); // Synced LFOs
}