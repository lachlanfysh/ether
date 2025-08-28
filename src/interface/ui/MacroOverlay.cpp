#include "MacroOverlay.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

MacroOverlay::MacroOverlay() {
    // Initialize default state
    currentMode_ = OverlayMode::HIDDEN;
    encoderMode_ = EncoderMode::MACRO_HTM;
    
    // Initialize encoder assignments
    initializeEncoderAssignments();
    
    // Initialize curve parameters
    initializeCurveParameters();
    
    // Initialize analysis data
    analysis_.spectrum.fill(0.0f);
    analysis_.macroValues.fill(0.5f);
    analysis_.cpuUsage = 0.0f;
    analysis_.dataValid = false;
    
    // Initialize animations
    showAnimation_.active = false;
    hideAnimation_.active = false;
}

MacroOverlay::~MacroOverlay() {
    shutdown();
}

bool MacroOverlay::initialize(SmartKnob* smartKnob, MacroHUD* macroHUD) {
    if (initialized_) {
        return true;
    }
    
    smartKnob_ = smartKnob;
    macroHUD_ = macroHUD;
    
    if (!smartKnob_ || !macroHUD_) {
        return false;
    }
    
    initialized_ = true;
    return true;
}

void MacroOverlay::shutdown() {
    if (!initialized_) {
        return;
    }
    
    smartKnob_ = nullptr;
    macroHUD_ = nullptr;
    initialized_ = false;
}

void MacroOverlay::setMode(OverlayMode mode) {
    if (mode == currentMode_) {
        return;
    }
    
    previousMode_ = currentMode_;
    currentMode_ = mode;
    
    // Configure SmartKnob for new mode
    if (smartKnob_) {
        SmartKnob::DetentConfig detentConfig;
        SmartKnob::HapticConfig hapticConfig;
        
        switch (mode) {
            case OverlayMode::PARAMETER:
                encoderMode_ = EncoderMode::MACRO_HTM;
                detentConfig.mode = SmartKnob::DetentMode::MEDIUM;
                hapticConfig.pattern = SmartKnob::HapticPattern::TICK;
                break;
                
            case OverlayMode::CURVE_EDIT:
                encoderMode_ = EncoderMode::CURVE_SHAPE;
                detentConfig.mode = SmartKnob::DetentMode::LIGHT;
                hapticConfig.pattern = SmartKnob::HapticPattern::SPRING;
                break;
                
            case OverlayMode::MAPPING:
                encoderMode_ = EncoderMode::MAPPING_SRC;
                detentConfig.mode = SmartKnob::DetentMode::HEAVY;
                hapticConfig.pattern = SmartKnob::HapticPattern::BUMP;
                break;
                
            case OverlayMode::ANALYZE:
                encoderMode_ = EncoderMode::FINE_TUNE;
                detentConfig.mode = SmartKnob::DetentMode::NONE;
                hapticConfig.pattern = SmartKnob::HapticPattern::NONE;
                break;
                
            default:
                break;
        }
        
        smartKnob_->setDetentConfig(detentConfig);
        smartKnob_->setHapticConfig(hapticConfig);
    }
    
    // Update encoder assignments for new mode
    updateEncoderAssignments();
}

void MacroOverlay::show() {
    if (currentMode_ == OverlayMode::HIDDEN) {
        setMode(OverlayMode::PARAMETER);
    }
    showAnimation_.start(300.0f);
}

void MacroOverlay::hide() {
    hideAnimation_.start(200.0f);
    if (hideAnimation_.isComplete()) {
        setMode(OverlayMode::HIDDEN);
    }
}

void MacroOverlay::setEncoderMode(EncoderMode mode) {
    encoderMode_ = mode;
    updateEncoderAssignments();
}

void MacroOverlay::setEncoderAssignment(int encoderIndex, const EncoderAssignment& assignment) {
    if (encoderIndex >= 0 && encoderIndex < 4) {
        encoderAssignments_[encoderIndex] = assignment;
    }
}

const MacroOverlay::EncoderAssignment& MacroOverlay::getEncoderAssignment(int encoderIndex) const {
    if (encoderIndex >= 0 && encoderIndex < 4) {
        return encoderAssignments_[encoderIndex];
    }
    return encoderAssignments_[0]; // Fallback
}

void MacroOverlay::setCurveParameters(MacroHUD::MacroParameter param, const CurveParameters& curve) {
    int index = static_cast<int>(param);
    if (index >= 0 && index < 3) {
        curveParameters_[index] = curve;
        
        if (curveChangeCallback_) {
            curveChangeCallback_(param, curve);
        }
    }
}

const MacroOverlay::CurveParameters& MacroOverlay::getCurveParameters(MacroHUD::MacroParameter param) const {
    int index = static_cast<int>(param);
    if (index >= 0 && index < 3) {
        return curveParameters_[index];
    }
    return curveParameters_[0]; // Fallback
}

void MacroOverlay::startCurveEdit(MacroHUD::MacroParameter param) {
    curveEditActive_ = true;
    activeCurveParam_ = param;
    curveEdit_.activeParam = param;
    curveEdit_.editing = true;
    curveEdit_.selectedPoint = -1;
    
    setMode(OverlayMode::CURVE_EDIT);
}

void MacroOverlay::endCurveEdit() {
    curveEditActive_ = false;
    curveEdit_.editing = false;
    curveEdit_.selectedPoint = -1;
    curveEdit_.dragging = false;
    
    setMode(OverlayMode::PARAMETER);
}

void MacroOverlay::addParameterMapping(const ParameterMapping& mapping) {
    // Check if mapping already exists
    for (auto& existing : parameterMappings_) {
        if (existing.sourceName == mapping.sourceName && 
            existing.destinationName == mapping.destinationName) {
            existing = mapping; // Update existing
            return;
        }
    }
    
    // Add new mapping
    parameterMappings_.push_back(mapping);
    
    if (mappingChangeCallback_) {
        mappingChangeCallback_(mapping);
    }
}

void MacroOverlay::removeParameterMapping(const std::string& sourceName) {
    parameterMappings_.erase(
        std::remove_if(parameterMappings_.begin(), parameterMappings_.end(),
                      [&sourceName](const ParameterMapping& mapping) {
                          return mapping.sourceName == sourceName;
                      }),
        parameterMappings_.end());
}

void MacroOverlay::handleRotation(int32_t delta, float velocity, bool inDetent) {
    if (currentMode_ == OverlayMode::HIDDEN) {
        return;
    }
    
    // Convert encoder delta to normalized value
    float normalizedDelta = static_cast<float>(delta) / 16384.0f;
    
    // Apply velocity scaling
    if (std::abs(velocity) > 2.0f) {
        normalizedDelta *= 2.0f; // Coarse mode
    } else if (std::abs(velocity) < 0.5f) {
        normalizedDelta *= 0.1f; // Fine mode
    }
    
    switch (encoderMode_) {
        case EncoderMode::MACRO_HTM:
            // Forward to MacroHUD
            if (macroHUD_) {
                macroHUD_->handleRotation(delta, velocity, inDetent);
            }
            break;
            
        case EncoderMode::CURVE_SHAPE:
            mapEncoderToCurve(normalizedDelta);
            break;
            
        case EncoderMode::MAPPING_SRC:
        case EncoderMode::MAPPING_DST:
            mapEncoderToMapping(normalizedDelta);
            break;
            
        case EncoderMode::FINE_TUNE:
            mapEncoderToParameter(0, normalizedDelta); // Use encoder 0 for fine tuning
            break;
    }
}

void MacroOverlay::handleGesture(SmartKnob::GestureType gesture, float parameter) {
    switch (gesture) {
        case SmartKnob::GestureType::DOUBLE_FLICK:
            if (currentMode_ == OverlayMode::PARAMETER) {
                setMode(OverlayMode::CURVE_EDIT);
            } else if (currentMode_ == OverlayMode::CURVE_EDIT) {
                setMode(OverlayMode::PARAMETER);
            }
            break;
            
        case SmartKnob::GestureType::DETENT_DWELL:
            if (currentMode_ == OverlayMode::PARAMETER) {
                setMode(OverlayMode::MAPPING);
            } else {
                setMode(OverlayMode::PARAMETER);
            }
            break;
            
        default:
            break;
    }
}

void MacroOverlay::handleTouch(int x, int y, bool pressed) {
    if (currentMode_ == OverlayMode::HIDDEN) {
        return;
    }
    
    // Convert to overlay-relative coordinates
    int overlayX = x - render_.overlayX;
    int overlayY = y - render_.overlayY;
    
    if (overlayX < 0 || overlayX >= render_.overlayWidth ||
        overlayY < 0 || overlayY >= render_.overlayHeight) {
        return; // Outside overlay
    }
    
    if (currentMode_ == OverlayMode::CURVE_EDIT) {
        handleCurveEditTouch(overlayX, overlayY, pressed);
    }
}

void MacroOverlay::update() {
    if (!initialized_) {
        return;
    }
    
    updateAnimations();
    updateEncoderAssignments();
    updateCurveEditing();
}

void MacroOverlay::render() {
    if (currentMode_ == OverlayMode::HIDDEN && !showAnimation_.active) {
        return;
    }
    
    renderOverlayBackground();
    
    switch (currentMode_) {
        case OverlayMode::PARAMETER:
            renderParameterMode();
            break;
            
        case OverlayMode::CURVE_EDIT:
            renderCurveEditMode();
            break;
            
        case OverlayMode::MAPPING:
            renderMappingMode();
            break;
            
        case OverlayMode::ANALYZE:
            renderAnalyzeMode();
            break;
            
        default:
            break;
    }
    
    renderEncoderAssignments();
}

void MacroOverlay::setAnalysisData(const std::array<float, 1024>& spectrum, 
                                  const std::array<float, 3>& macroValues,
                                  float cpuUsage) {
    analysis_.spectrum = spectrum;
    analysis_.macroValues = macroValues;
    analysis_.cpuUsage = cpuUsage;
    analysis_.lastUpdate = getTimeMs();
    analysis_.dataValid = true;
}

// Private method implementations

void MacroOverlay::initializeEncoderAssignments() {
    // Default H/T/M assignments
    encoderAssignments_[0] = {"HARMONICS", "Harmonics", 0.0f, 1.0f, 0.5f, false, "%"};
    encoderAssignments_[1] = {"TIMBRE", "Timbre", 0.0f, 1.0f, 0.5f, false, "%"};
    encoderAssignments_[2] = {"MORPH", "Morph", 0.0f, 1.0f, 0.5f, false, "%"};
    encoderAssignments_[3] = {"VOLUME", "Volume", 0.0f, 1.0f, 0.8f, false, "%"};
    
    for (auto& assignment : encoderAssignments_) {
        assignment.mode = EncoderMode::MACRO_HTM;
    }
}

void MacroOverlay::initializeCurveParameters() {
    // Initialize all parameters to linear curves
    for (auto& curve : curveParameters_) {
        curve = createLinearCurve();
    }
}

void MacroOverlay::updateEncoderAssignments() {
    switch (encoderMode_) {
        case EncoderMode::MACRO_HTM:
            encoderAssignments_[0] = {"HARMONICS", "Harmonics", 0.0f, 1.0f, 0.5f, false, "%"};
            encoderAssignments_[1] = {"TIMBRE", "Timbre", 0.0f, 1.0f, 0.5f, false, "%"};
            encoderAssignments_[2] = {"MORPH", "Morph", 0.0f, 1.0f, 0.5f, false, "%"};
            encoderAssignments_[3] = {"VOLUME", "Volume", 0.0f, 1.0f, 0.8f, false, "%"};
            break;
            
        case EncoderMode::CURVE_SHAPE:
            {
                const CurveParameters& curve = getCurveParameters(activeCurveParam_);
                encoderAssignments_[0] = {"SHAPE", "Shape", 0.0f, 1.0f, curve.shape, false, ""};
                encoderAssignments_[1] = {"BIAS", "Bias", 0.0f, 1.0f, curve.bias, false, ""};
                encoderAssignments_[2] = {"SCALE", "Scale", 0.1f, 2.0f, curve.scale, false, "x"};
                encoderAssignments_[3] = {"TYPE", "Type", 0.0f, 5.0f, static_cast<float>(curve.type), false, ""};
            }
            break;
            
        case EncoderMode::MAPPING_SRC:
            encoderAssignments_[0] = {"SRC_SEL", "Source", 0.0f, 10.0f, 0.0f, false, ""};
            encoderAssignments_[1] = {"SRC_DEPTH", "Depth", -1.0f, 1.0f, 1.0f, false, ""};
            encoderAssignments_[2] = {"SRC_CURVE", "Curve", 0.0f, 5.0f, 0.0f, false, ""};
            encoderAssignments_[3] = {"SRC_ENABLE", "Enable", 0.0f, 1.0f, 1.0f, false, ""};
            break;
            
        case EncoderMode::MAPPING_DST:
            encoderAssignments_[0] = {"DST_SEL", "Destination", 0.0f, 20.0f, 0.0f, false, ""};
            encoderAssignments_[1] = {"DST_AMOUNT", "Amount", 0.0f, 2.0f, 1.0f, false, ""};
            encoderAssignments_[2] = {"DST_OFFSET", "Offset", -1.0f, 1.0f, 0.0f, false, ""};
            encoderAssignments_[3] = {"DST_LIMIT", "Limit", 0.0f, 1.0f, 1.0f, false, ""};
            break;
            
        case EncoderMode::FINE_TUNE:
            if (macroHUD_) {
                MacroHUD::MacroParameter activeParam = macroHUD_->getActiveParameter();
                std::string paramName = (activeParam == MacroHUD::MacroParameter::HARMONICS) ? "Harmonics" :
                                       (activeParam == MacroHUD::MacroParameter::TIMBRE) ? "Timbre" : "Morph";
                float paramValue = macroHUD_->getParameter(activeParam);
                
                encoderAssignments_[0] = {"FINE", paramName + " Fine", 0.0f, 1.0f, paramValue, false, "%"};
                encoderAssignments_[1] = {"OFFSET", "Offset", -0.1f, 0.1f, 0.0f, false, "%"};
                encoderAssignments_[2] = {"SCALE", "Scale", 0.5f, 2.0f, 1.0f, false, "x"};
                encoderAssignments_[3] = {"SMOOTH", "Smooth", 0.0f, 1.0f, 0.0f, false, ""};
            }
            break;
    }
    
    // Update mode for all assignments
    for (auto& assignment : encoderAssignments_) {
        assignment.mode = encoderMode_;
    }
}

void MacroOverlay::updateCurveEditing() {
    if (!curveEditActive_) {
        return;
    }
    
    // Update curve visualization
    // This would be called during rendering
}

void MacroOverlay::updateAnimations() {
    uint32_t currentTime = getTimeMs();
    
    if (showAnimation_.active) {
        showAnimation_.update(currentTime);
    }
    
    if (hideAnimation_.active) {
        hideAnimation_.update(currentTime);
        if (hideAnimation_.isComplete()) {
            setMode(OverlayMode::HIDDEN);
        }
    }
}

void MacroOverlay::renderOverlayBackground() {
    float alpha = 1.0f;
    
    if (showAnimation_.active) {
        alpha = showAnimation_.progress;
    } else if (hideAnimation_.active) {
        alpha = 1.0f - hideAnimation_.progress;
    }
    
    // Draw semi-transparent background
    uint16_t bgColor = blendColors(0x0000, render_.backgroundColor, alpha * 0.9f);
    drawPanel(0, 0, render_.screenWidth, render_.screenHeight, bgColor);
    
    // Draw main overlay panel
    uint16_t panelColor = blendColors(0x0000, render_.panelColor, alpha);
    drawPanel(render_.overlayX, render_.overlayY, render_.overlayWidth, render_.overlayHeight, panelColor);
    
    // Draw title bar
    std::string title;
    switch (currentMode_) {
        case OverlayMode::PARAMETER: title = "Parameter Control"; break;
        case OverlayMode::CURVE_EDIT: title = "Curve Editor"; break;
        case OverlayMode::MAPPING: title = "Parameter Mapping"; break;
        case OverlayMode::ANALYZE: title = "Real-time Analysis"; break;
        default: title = "Macro Overlay"; break;
    }
    
    drawCenteredText(title, render_.overlayX, render_.overlayY - 25, render_.overlayWidth, 
                    render_.textColor, 16);
}

void MacroOverlay::renderParameterMode() {
    int x = render_.overlayX + 20;
    int y = render_.overlayY + 20;
    int width = render_.overlayWidth - 40;
    int height = render_.overlayHeight - 40;
    
    // Draw parameter visualization for each macro
    int paramWidth = width / 3;
    int paramHeight = height - 60;
    
    for (int i = 0; i < 3; i++) {
        MacroHUD::MacroParameter param = static_cast<MacroHUD::MacroParameter>(i);
        int paramX = x + i * paramWidth;
        int paramY = y;
        
        // Get current parameter value
        float value = macroHUD_ ? macroHUD_->getParameter(param) : 0.5f;
        bool isActive = macroHUD_ ? (macroHUD_->getActiveParameter() == param) : false;
        
        // Draw parameter curve
        const CurveParameters& curve = getCurveParameters(param);
        renderCurve(curve, paramX + 10, paramY, paramWidth - 20, paramHeight, 
                   isActive ? render_.activeColor : render_.curveColor);
        
        // Draw current position
        float curveValue = evaluateCurve(curve, value);
        int posX = paramX + 10 + static_cast<int>(value * (paramWidth - 20));
        int posY = paramY + paramHeight - static_cast<int>(curveValue * paramHeight);
        drawControlPoint(posX, posY, isActive, render_.highlightColor);
        
        // Draw parameter name
        std::string paramName = (i == 0) ? "HARMONICS" : (i == 1) ? "TIMBRE" : "MORPH";
        drawCenteredText(paramName, paramX, paramY + paramHeight + 10, paramWidth, 
                        render_.textColor, 12);
    }
}

void MacroOverlay::renderCurveEditMode() {
    renderCurveEditor(activeCurveParam_);
}

void MacroOverlay::renderMappingMode() {
    renderParameterMappings();
}

void MacroOverlay::renderAnalyzeMode() {
    renderAnalysisDisplay();
}

void MacroOverlay::renderCurve(const CurveParameters& curve, int x, int y, int width, int height, 
                              uint16_t color, bool interactive) {
    // Calculate curve points
    std::array<float, 64> points = calculateCurve(curve);
    
    // Draw grid
    drawGrid(x, y, width, height, 8);
    
    // Draw curve
    drawCurvePath(points, x, y, width, height, color, interactive ? 3 : 2);
    
    if (interactive && curve.type == CurveType::CUSTOM) {
        // Draw control points for custom curves
        for (int i = 0; i < curve.numCustomPoints; i++) {
            float pointX = static_cast<float>(i) / (curve.numCustomPoints - 1);
            float pointY = curve.customPoints[i];
            
            int screenX = x + static_cast<int>(pointX * width);
            int screenY = y + height - static_cast<int>(pointY * height);
            
            bool selected = (curveEdit_.selectedPoint == i);
            drawControlPoint(screenX, screenY, selected, color);
        }
    }
}

// Curve calculation methods
std::array<float, 64> MacroOverlay::calculateCurve(const CurveParameters& params) const {
    std::array<float, 64> points;
    
    for (int i = 0; i < 64; i++) {
        float x = static_cast<float>(i) / 63.0f;
        points[i] = evaluateCurve(params, x);
    }
    
    return points;
}

float MacroOverlay::evaluateCurve(const CurveParameters& params, float input) const {
    input = std::clamp(input, 0.0f, 1.0f);
    
    float output = 0.0f;
    
    switch (params.type) {
        case CurveType::LINEAR:
            output = calculateLinear(input);
            break;
            
        case CurveType::EXPONENTIAL:
            output = calculateExponential(input, params.shape);
            break;
            
        case CurveType::LOGARITHMIC:
            output = calculateLogarithmic(input, params.shape);
            break;
            
        case CurveType::S_CURVE:
            output = calculateSCurve(input, params.shape);
            break;
            
        case CurveType::STEPPED:
            output = calculateStepped(input, static_cast<int>(params.shape * 15 + 1));
            break;
            
        case CurveType::CUSTOM:
            output = calculateCustom(input, params);
            break;
    }
    
    // Apply bias
    output += (params.bias - 0.5f) * 0.5f;
    
    // Apply scale
    output = (output - 0.5f) * params.scale + 0.5f;
    
    // Apply inversion
    if (params.inverted) {
        output = 1.0f - output;
    }
    
    return std::clamp(output, 0.0f, 1.0f);
}

float MacroOverlay::calculateLinear(float x) const {
    return x;
}

float MacroOverlay::calculateExponential(float x, float shape) const {
    float exponent = 0.1f + shape * 9.9f; // 0.1 to 10.0
    return std::pow(x, exponent);
}

float MacroOverlay::calculateLogarithmic(float x, float shape) const {
    if (x <= 0.0f) return 0.0f;
    float base = 0.1f + shape * 9.9f; // 0.1 to 10.0
    return std::log(x * (base - 1.0f) + 1.0f) / std::log(base);
}

float MacroOverlay::calculateSCurve(float x, float shape) const {
    float steepness = 1.0f + shape * 19.0f; // 1 to 20
    float center = 0.5f;
    return 1.0f / (1.0f + std::exp(-steepness * (x - center)));
}

float MacroOverlay::calculateStepped(float x, int steps) const {
    if (steps <= 1) return x;
    
    float stepSize = 1.0f / (steps - 1);
    int stepIndex = static_cast<int>(x / stepSize);
    return std::min(stepIndex * stepSize, 1.0f);
}

float MacroOverlay::calculateCustom(float x, const CurveParameters& params) const {
    if (params.numCustomPoints < 2) return x;
    
    // Linear interpolation between custom points
    float scaledX = x * (params.numCustomPoints - 1);
    int index = static_cast<int>(scaledX);
    float frac = scaledX - index;
    
    if (index >= params.numCustomPoints - 1) {
        return params.customPoints[params.numCustomPoints - 1];
    }
    
    return params.customPoints[index] * (1.0f - frac) + 
           params.customPoints[index + 1] * frac;
}

// Animation implementation
void MacroOverlay::Animation::start(float durationMs) {
    duration = durationMs;
    progress = 0.0f;
    active = true;
    startTime = 0; // Will be set on first update
}

float MacroOverlay::Animation::update(uint32_t currentTime) {
    if (!active) return progress;
    
    if (startTime == 0) {
        startTime = currentTime;
    }
    
    uint32_t elapsed = currentTime - startTime;
    
    if (elapsed >= static_cast<uint32_t>(duration)) {
        progress = 1.0f;
        active = false;
    } else {
        progress = static_cast<float>(elapsed) / duration;
        // Apply easing
        progress = smoothStep(0.0f, 1.0f, progress);
    }
    
    return progress;
}

// Preset curve creators
MacroOverlay::CurveParameters MacroOverlay::createLinearCurve() const {
    CurveParameters curve;
    curve.type = CurveType::LINEAR;
    curve.shape = 0.5f;
    curve.bias = 0.5f;
    curve.scale = 1.0f;
    curve.bipolar = false;
    curve.inverted = false;
    curve.steps = 0;
    
    // Initialize custom points as linear
    for (int i = 0; i < curve.numCustomPoints; i++) {
        curve.customPoints[i] = static_cast<float>(i) / (curve.numCustomPoints - 1);
    }
    
    return curve;
}

MacroOverlay::CurveParameters MacroOverlay::createExponentialCurve(float exponent) const {
    CurveParameters curve = createLinearCurve();
    curve.type = CurveType::EXPONENTIAL;
    curve.shape = (exponent - 0.1f) / 9.9f; // Map to 0-1 range
    return curve;
}

MacroOverlay::CurveParameters MacroOverlay::createSCurve(float steepness) const {
    CurveParameters curve = createLinearCurve();
    curve.type = CurveType::S_CURVE;
    curve.shape = (steepness - 1.0f) / 19.0f; // Map to 0-1 range
    return curve;
}

MacroOverlay::CurveParameters MacroOverlay::createSteppedCurve(int steps) const {
    CurveParameters curve = createLinearCurve();
    curve.type = CurveType::STEPPED;
    curve.shape = static_cast<float>(steps - 1) / 15.0f; // Map to 0-1 range
    curve.steps = steps;
    return curve;
}

// Utility functions
uint32_t MacroOverlay::getTimeMs() const {
#ifdef STM32H7
    return HAL_GetTick();
#else
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
#endif
}

float MacroOverlay::smoothStep(float edge0, float edge1, float x) const {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

uint16_t MacroOverlay::blendColors(uint16_t color1, uint16_t color2, float blend) const {
    blend = std::clamp(blend, 0.0f, 1.0f);
    
    uint8_t r1 = (color1 >> 11) & 0x1F;
    uint8_t g1 = (color1 >> 5) & 0x3F;
    uint8_t b1 = color1 & 0x1F;
    
    uint8_t r2 = (color2 >> 11) & 0x1F;
    uint8_t g2 = (color2 >> 5) & 0x3F;
    uint8_t b2 = color2 & 0x1F;
    
    uint8_t r = static_cast<uint8_t>(r1 + blend * (r2 - r1));
    uint8_t g = static_cast<uint8_t>(g1 + blend * (g2 - g1));
    uint8_t b = static_cast<uint8_t>(b1 + blend * (b2 - b1));
    
    return (r << 11) | (g << 5) | b;
}

// Placeholder implementations for drawing functions
void MacroOverlay::drawPanel(int x, int y, int width, int height, uint16_t color) {
    // TouchGFX implementation would go here
}

void MacroOverlay::drawGrid(int x, int y, int width, int height, int divisions) {
    // TouchGFX implementation would go here
}

void MacroOverlay::drawCurvePath(const std::array<float, 64>& points, 
                                int x, int y, int width, int height, uint16_t color, int thickness) {
    // TouchGFX implementation would go here
}

void MacroOverlay::drawControlPoint(int x, int y, bool selected, uint16_t color) {
    // TouchGFX implementation would go here
}

void MacroOverlay::drawText(const std::string& text, int x, int y, uint16_t color, int size) {
    // TouchGFX implementation would go here
}

void MacroOverlay::drawCenteredText(const std::string& text, int x, int y, int width, uint16_t color, int size) {
    // TouchGFX implementation would go here
}

// Stub implementations for methods that need more space
void MacroOverlay::mapEncoderToParameter(int encoderIndex, float delta) {
    // Implementation would handle encoder to parameter mapping
}

void MacroOverlay::mapEncoderToCurve(float delta) {
    // Implementation would handle curve parameter adjustment
}

void MacroOverlay::mapEncoderToMapping(float delta) {
    // Implementation would handle parameter mapping configuration
}

void MacroOverlay::handleCurveEditTouch(int x, int y, bool pressed) {
    // Implementation would handle touch input for curve editing
}

void MacroOverlay::renderCurveEditor(MacroHUD::MacroParameter param) {
    // Implementation would render the curve editor interface
}

void MacroOverlay::renderParameterMappings() {
    // Implementation would render parameter mapping interface
}

void MacroOverlay::renderEncoderAssignments() {
    // Implementation would render encoder assignment display
}

void MacroOverlay::renderAnalysisDisplay() {
    // Implementation would render real-time analysis display
}