#pragma once
#include "../platform/hardware/SmartKnob.h"
#include "MacroHUD.h"
#include <array>
#include <vector>
#include <functional>
#include <string>

/**
 * MacroOverlay - Advanced parameter editing with encoder repurposing
 * 
 * Features:
 * - Dynamic encoder repurposing for different parameter groups
 * - Real-time curve visualization and editing
 * - Multi-parameter mapping with visual feedback
 * - Context-sensitive parameter grouping
 * - TouchGFX optimized 3D curve rendering
 * - Professional-grade curve editing tools
 */
class MacroOverlay {
public:
    enum class OverlayMode {
        HIDDEN,         // Overlay not visible
        PARAMETER,      // Standard parameter editing
        CURVE_EDIT,     // Curve shape editing
        MAPPING,        // Parameter mapping configuration
        ANALYZE         // Real-time analysis view
    };
    
    enum class EncoderMode {
        MACRO_HTM,      // Standard H/T/M control
        CURVE_SHAPE,    // Curve shape parameters
        MAPPING_SRC,    // Mapping source selection
        MAPPING_DST,    // Mapping destination selection
        FINE_TUNE       // Fine parameter adjustment
    };
    
    enum class CurveType {
        LINEAR,         // y = x
        EXPONENTIAL,    // y = x^n
        LOGARITHMIC,    // y = log(x)
        S_CURVE,        // Sigmoid curve
        STEPPED,        // Quantized steps
        CUSTOM          // User-defined curve
    };
    
    struct CurveParameters {
        CurveType type = CurveType::LINEAR;
        float shape = 0.5f;         // Shape parameter (0-1)
        float bias = 0.5f;          // Bias/offset (0-1)
        float scale = 1.0f;         // Scale factor
        bool bipolar = false;       // Bipolar vs unipolar
        bool inverted = false;      // Invert curve
        int steps = 0;              // Number of steps (0 = smooth)
        
        // Custom curve points (for CUSTOM type)
        static constexpr int MAX_POINTS = 16;
        std::array<float, MAX_POINTS> customPoints;
        int numCustomPoints = 4;
    };
    
    struct ParameterMapping {
        std::string sourceName;
        std::string destinationName;
        CurveParameters curve;
        float depth = 1.0f;         // Modulation depth
        bool enabled = true;
        bool bipolar = false;
    };
    
    struct EncoderAssignment {
        EncoderMode mode;
        std::string parameterName;
        std::string displayName;
        float minValue = 0.0f;
        float maxValue = 1.0f;
        float currentValue = 0.5f;
        bool logarithmic = false;
        std::string units;
    };
    
    using ParameterChangeCallback = std::function<void(const std::string& param, float value)>;
    using CurveChangeCallback = std::function<void(MacroHUD::MacroParameter param, const CurveParameters& curve)>;
    using MappingChangeCallback = std::function<void(const ParameterMapping& mapping)>;
    
    MacroOverlay();
    ~MacroOverlay();
    
    // Initialization
    bool initialize(SmartKnob* smartKnob, MacroHUD* macroHUD);
    void shutdown();
    
    // Overlay control
    void setMode(OverlayMode mode);
    OverlayMode getMode() const { return currentMode_; }
    
    void show();
    void hide();
    bool isVisible() const { return currentMode_ != OverlayMode::HIDDEN; }
    
    // Encoder management
    void setEncoderMode(EncoderMode mode);
    EncoderMode getEncoderMode() const { return encoderMode_; }
    
    void setEncoderAssignment(int encoderIndex, const EncoderAssignment& assignment);
    const EncoderAssignment& getEncoderAssignment(int encoderIndex) const;
    
    // Curve editing
    void setCurveParameters(MacroHUD::MacroParameter param, const CurveParameters& curve);
    const CurveParameters& getCurveParameters(MacroHUD::MacroParameter param) const;
    
    void startCurveEdit(MacroHUD::MacroParameter param);
    void endCurveEdit();
    bool isCurveEditing() const { return curveEditActive_; }
    
    // Parameter mapping
    void addParameterMapping(const ParameterMapping& mapping);
    void removeParameterMapping(const std::string& sourceName);
    const std::vector<ParameterMapping>& getParameterMappings() const { return parameterMappings_; }
    
    // Callbacks
    void setParameterChangeCallback(ParameterChangeCallback callback) { paramChangeCallback_ = callback; }
    void setCurveChangeCallback(CurveChangeCallback callback) { curveChangeCallback_ = callback; }
    void setMappingChangeCallback(MappingChangeCallback callback) { mappingChangeCallback_ = callback; }
    
    // Input handling
    void handleRotation(int32_t delta, float velocity, bool inDetent);
    void handleGesture(SmartKnob::GestureType gesture, float parameter);
    void handleTouch(int x, int y, bool pressed);
    
    // Update and rendering
    void update();
    void render();
    
    // Analysis
    void setAnalysisData(const std::array<float, 1024>& spectrum, 
                        const std::array<float, 3>& macroValues,
                        float cpuUsage);
    
private:
    // Rendering contexts
    struct RenderContext {
        int screenWidth = 800;
        int screenHeight = 480;
        int overlayX = 100;
        int overlayY = 80;
        int overlayWidth = 600;
        int overlayHeight = 320;
        
        // Colors
        uint16_t backgroundColor = 0x0841;    // Dark blue
        uint16_t panelColor = 0x18C3;         // Medium blue
        uint16_t gridColor = 0x39E7;          // Light gray
        uint16_t curveColor = 0x07FF;         // Cyan
        uint16_t activeColor = 0xFD20;        // Orange
        uint16_t textColor = 0xFFFF;          // White
        uint16_t highlightColor = 0xFFE0;     // Yellow
    };
    
    // Curve editing state
    struct CurveEditState {
        MacroHUD::MacroParameter activeParam = MacroHUD::MacroParameter::HARMONICS;
        bool editing = false;
        int selectedPoint = -1;
        bool dragging = false;
        int dragStartX = 0;
        int dragStartY = 0;
        float dragStartValue = 0.0f;
    };
    
    // Analysis display
    struct AnalysisData {
        std::array<float, 1024> spectrum;
        std::array<float, 3> macroValues;
        float cpuUsage = 0.0f;
        uint32_t lastUpdate = 0;
        bool dataValid = false;
    };
    
    // Animation system
    struct Animation {
        float progress = 0.0f;
        float duration = 300.0f; // ms
        bool active = false;
        uint32_t startTime = 0;
        
        void start(float durationMs = 300.0f);
        float update(uint32_t currentTime);
        bool isComplete() const { return !active; }
    };
    
    // State
    OverlayMode currentMode_ = OverlayMode::HIDDEN;
    OverlayMode previousMode_ = OverlayMode::HIDDEN;
    EncoderMode encoderMode_ = EncoderMode::MACRO_HTM;
    
    // Hardware integration
    SmartKnob* smartKnob_ = nullptr;
    MacroHUD* macroHUD_ = nullptr;
    bool initialized_ = false;
    
    // Rendering
    RenderContext render_;
    CurveEditState curveEdit_;
    AnalysisData analysis_;
    Animation showAnimation_;
    Animation hideAnimation_;
    
    // Parameter management
    std::array<EncoderAssignment, 4> encoderAssignments_;
    std::array<CurveParameters, 3> curveParameters_;
    std::vector<ParameterMapping> parameterMappings_;
    
    // Callbacks
    ParameterChangeCallback paramChangeCallback_;
    CurveChangeCallback curveChangeCallback_;
    MappingChangeCallback mappingChangeCallback_;
    
    // State flags
    bool curveEditActive_ = false;
    MacroHUD::MacroParameter activeCurveParam_ = MacroHUD::MacroParameter::HARMONICS;
    
    // Private methods
    void initializeEncoderAssignments();
    void initializeCurveParameters();
    
    void updateEncoderAssignments();
    void updateCurveEditing();
    void updateAnimations();
    
    void renderOverlayBackground();
    void renderParameterMode();
    void renderCurveEditMode();
    void renderMappingMode();
    void renderAnalyzeMode();
    
    void renderCurve(const CurveParameters& curve, int x, int y, int width, int height, 
                    uint16_t color, bool interactive = false);
    void renderCurveEditor(MacroHUD::MacroParameter param);
    void renderParameterMappings();
    void renderEncoderAssignments();
    void renderAnalysisDisplay();
    
    void renderSpectrumAnalyzer(const std::array<float, 1024>& spectrum, 
                               int x, int y, int width, int height);
    void renderMacroValues(const std::array<float, 3>& values,
                          int x, int y, int width, int height);
    void renderCPUUsage(float usage, int x, int y, int width, int height);
    
    // Curve calculation
    std::array<float, 64> calculateCurve(const CurveParameters& params) const;
    float evaluateCurve(const CurveParameters& params, float input) const;
    
    float calculateLinear(float x) const;
    float calculateExponential(float x, float shape) const;
    float calculateLogarithmic(float x, float shape) const;
    float calculateSCurve(float x, float shape) const;
    float calculateStepped(float x, int steps) const;
    float calculateCustom(float x, const CurveParameters& params) const;
    
    // Touch handling for curve editing
    void handleCurveEditTouch(int x, int y, bool pressed);
    int findNearestCurvePoint(int x, int y, const CurveParameters& params) const;
    void updateCurvePoint(int pointIndex, float newValue);
    
    // Encoder mapping
    void mapEncoderToParameter(int encoderIndex, float delta);
    void mapEncoderToCurve(float delta);
    void mapEncoderToMapping(float delta);
    
    // Utility functions
    uint32_t getTimeMs() const;
    float smoothStep(float edge0, float edge1, float x) const;
    uint16_t blendColors(uint16_t color1, uint16_t color2, float blend) const;
    
    // Drawing helpers
    void drawPanel(int x, int y, int width, int height, uint16_t color);
    void drawGrid(int x, int y, int width, int height, int divisions);
    void drawCurvePath(const std::array<float, 64>& points, 
                      int x, int y, int width, int height, uint16_t color, int thickness = 2);
    void drawControlPoint(int x, int y, bool selected, uint16_t color);
    void drawParameterBox(const std::string& name, float value, const std::string& units,
                         int x, int y, int width, int height, bool active);
    
    // Text rendering
    void drawText(const std::string& text, int x, int y, uint16_t color, int size = 12);
    void drawCenteredText(const std::string& text, int x, int y, int width, uint16_t color, int size = 12);
    
    // Preset curve types
    CurveParameters createLinearCurve() const;
    CurveParameters createExponentialCurve(float exponent) const;
    CurveParameters createSCurve(float steepness) const;
    CurveParameters createSteppedCurve(int steps) const;
};