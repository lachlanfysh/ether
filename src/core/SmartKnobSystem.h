#pragma once
#include "Types.h"
#include <vector>
#include <string>
#include <functional>

/**
 * Smart Knob Parameter Assignment System
 * Intelligently maps the single smart knob to different parameters
 * based on context, instrument, and user interaction patterns
 */
class SmartKnobSystem {
public:
    SmartKnobSystem();
    ~SmartKnobSystem();
    
    // Core knob interaction
    void setKnobValue(float value);           // 0.0 - 1.0
    float getKnobValue() const { return currentValue_; }
    
    // Context management
    void setActiveInstrument(InstrumentColor color);
    void setMode(SmartKnobMode mode);
    SmartKnobMode getCurrentMode() const { return currentMode_; }
    
    // Parameter assignment
    void assignParameter(ParameterID param, float weight = 1.0f);
    void clearAssignments();
    void setParameterRange(ParameterID param, float min, float max);
    
    // Quick access patterns
    void enableMacroMode(const std::string& macroName);
    void createMacro(const std::string& name, const std::vector<ParameterAssignment>& assignments);
    
    // User interaction tracking
    void onKnobTouch(bool touched);           // Physical touch detection
    void onKnobTurn(float delta);             // Turn direction and speed
    void onDoubleClick();                     // Reset to center or switch mode
    void onLongPress();                       // Context menu or mode switch
    
    // Visual feedback
    std::string getCurrentParameterName() const;
    float getCurrentParameterValue() const;
    std::vector<std::string> getActiveParameterNames() const;
    Color getKnobColor() const;               // Color based on current context
    
    // Auto-assignment intelligence
    void enableAutoAssignment(bool enable);
    void learnFromUserBehavior();             // ML-inspired parameter relevance
    
    // Preset system
    void savePreset(const std::string& name);
    void loadPreset(const std::string& name);
    std::vector<std::string> getPresetNames() const;
    
    // Advanced features
    void enableCrossfadeMode(ParameterID param1, ParameterID param2);
    void enableMultiParameterMode(const std::vector<ParameterID>& params);
    void setParameterCurve(ParameterID param, CurveType curve);
    
private:
    // Current state
    float currentValue_ = 0.5f;
    float lastValue_ = 0.5f;
    SmartKnobMode currentMode_ = SmartKnobMode::SINGLE_PARAMETER;
    InstrumentColor activeInstrument_ = InstrumentColor::RED;
    
    // Parameter assignments
    struct ParameterAssignment {
        ParameterID parameter;
        float weight = 1.0f;         // How much this parameter is affected
        float minValue = 0.0f;       // Minimum mapped value
        float maxValue = 1.0f;       // Maximum mapped value
        CurveType curve = CurveType::LINEAR;
        bool enabled = true;
        
        // Contextual relevance
        float relevanceScore = 1.0f; // Auto-calculated based on usage
        uint32_t lastUsed = 0;       // Timestamp
        uint32_t useCount = 0;       // Usage frequency
    };
    
    std::vector<ParameterAssignment> currentAssignments_;
    
    // Mode-specific data
    struct MacroDefinition {
        std::string name;
        std::vector<ParameterAssignment> assignments;
        Color visualColor;
        std::string description;
    };
    
    std::vector<MacroDefinition> macros_;
    std::string activeMacro_;
    
    // Per-instrument parameter preferences
    struct InstrumentContext {
        std::vector<ParameterID> preferredParameters;
        std::vector<MacroDefinition> customMacros;
        SmartKnobMode preferredMode = SmartKnobMode::SINGLE_PARAMETER;
        float lastKnobValue = 0.5f;
    };
    
    std::array<InstrumentContext, MAX_INSTRUMENTS> instrumentContexts_;
    
    // User interaction tracking
    struct InteractionMetrics {
        uint32_t totalTurns = 0;
        uint32_t totalTouches = 0;
        uint32_t doubleClicks = 0;
        uint32_t longPresses = 0;
        float avgTurnSpeed = 0.0f;
        std::vector<ParameterID> mostUsedParams;
        uint32_t sessionStartTime = 0;
    };
    
    InteractionMetrics metrics_;
    
    // Auto-assignment intelligence
    bool autoAssignmentEnabled_ = true;
    std::vector<ParameterID> suggestedParameters_;
    
    // Touch and gesture state
    bool knobTouched_ = false;
    uint32_t touchStartTime_ = 0;
    uint32_t lastClickTime_ = 0;
    float totalTurnDistance_ = 0.0f;
    
    // Callbacks for parameter changes
    std::function<void(ParameterID, float)> parameterCallback_;
    
    // Internal methods
    void updateParameterAssignments();
    void calculateParameterRelevance();
    void updateSuggestedParameters();
    void applyParameterCurve(float& value, CurveType curve);
    float mapValueToRange(float input, float min, float max, CurveType curve);
    
    // Auto-assignment algorithms
    void analyzeParameterUsage();
    void updateContextualSuggestions();
    std::vector<ParameterID> getContextualParameters() const;
    
    // Preset management
    void saveContextToPreset(const std::string& name);
    void loadContextFromPreset(const std::string& name);
    
    // Default configurations
    void initializeDefaultMacros();
    void initializeInstrumentDefaults();
    
    // Utility methods
    uint32_t getCurrentTime() const;
    Color getParameterColor(ParameterID param) const;
    std::string getParameterDisplayName(ParameterID param) const;
    
public:
    // Callback registration
    void setParameterCallback(std::function<void(ParameterID, float)> callback) {
        parameterCallback_ = callback;
    }
    
    // Static utility methods
    static std::string getModeDescription(SmartKnobMode mode);
    static std::vector<ParameterID> getRecommendedParameters(InstrumentColor instrument);
    static MacroDefinition createPerformanceMacro();
    static MacroDefinition createFilterMacro();
    static MacroDefinition createEnvelopeMacro();
};

// Smart knob modes
enum class SmartKnobMode {
    SINGLE_PARAMETER,    // Controls one parameter at a time
    MACRO,              // Controls multiple parameters simultaneously
    CROSSFADE,          // Crossfades between two parameter states
    MULTI_PARAMETER,    // Controls multiple parameters with different curves
    AUTO_ASSIGN,        // Automatically assigns based on context
    PERFORMANCE,        // Optimized for live performance
    LEARN               // Learning mode for recording parameter changes
};

// Parameter curve types for smart mapping
enum class CurveType {
    LINEAR,             // y = x
    EXPONENTIAL,        // y = x²
    LOGARITHMIC,        // y = √x
    S_CURVE,           // Smooth S-shaped curve
    REVERSE_EXP,       // y = 1 - (1-x)²
    CUSTOM             // User-defined curve
};

// Color system for visual feedback
struct Color {
    float r, g, b, a = 1.0f;
    
    Color(float red = 0.0f, float green = 0.0f, float blue = 0.0f, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}
    
    static Color fromInstrument(InstrumentColor instrument);
    static Color fromParameter(ParameterID param);
    static Color blend(const Color& a, const Color& b, float mix);
};