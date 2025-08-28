#include "VelocityUIStrings.h"
#include <algorithm>
#include <sstream>

VelocityUIStrings::VelocityUIStrings() {
    // Initialize all string categories
    initializeButtonLabels();
    initializeOverlayHints();
    initializeStatusMessages();
    initializeErrorMessages();
    initializeAccessibilityStrings();
    initializeTooltips();
    initializeTemplates();
    
    // Organize strings into categories
    categorizeAllStrings();
}

void VelocityUIStrings::initializeButtonLabels() {
    using namespace VelocityStrings::Buttons;
    
    // Main velocity control buttons (optimized for 12-char UI limit)
    addString(LocalizedString(
        VELOCITY_ENABLE, "Velocity", "VEL", 
        "Enable or disable velocity modulation for all parameters",
        12, UIContext::HUD_MAIN
    ), StringCategory::BUTTON_LABELS);
    
    addString(LocalizedString(
        DEPTH_CONTROL, "Depth", "DEPTH", 
        "Adjust global velocity modulation depth from 0 to 200 percent",
        12, UIContext::VELOCITY_PANEL
    ), StringCategory::BUTTON_LABELS);
    
    addString(LocalizedString(
        CURVE_TYPE, "Curve", "CURVE", 
        "Select velocity response curve type: linear, exponential, or custom",
        12, UIContext::VELOCITY_PANEL
    ), StringCategory::BUTTON_LABELS);
    
    addString(LocalizedString(
        ENGINE_MAP, "Engine Map", "ENG MAP", 
        "Configure velocity mapping for specific synthesis engines",
        12, UIContext::ENGINE_CONFIG
    ), StringCategory::BUTTON_LABELS);
    
    addString(LocalizedString(
        PRESET_LOAD, "Load Preset", "PRESET", 
        "Load predefined velocity configuration preset",
        12, UIContext::PRESET_BROWSER
    ), StringCategory::BUTTON_LABELS);
    
    addString(LocalizedString(
        VELOCITY_RESET, "Reset", "RESET", 
        "Reset all velocity settings to default values",
        12, UIContext::VELOCITY_PANEL
    ), StringCategory::BUTTON_LABELS);
    
    addString(LocalizedString(
        MAPPING_EDIT, "Edit Map", "EDIT", 
        "Edit parameter-to-velocity mappings for current engine",
        12, UIContext::ENGINE_CONFIG
    ), StringCategory::BUTTON_LABELS);
    
    addString(LocalizedString(
        CURVE_ADJUST, "Adjust", "ADJUST", 
        "Fine-tune velocity curve response characteristics",
        12, UIContext::VELOCITY_PANEL
    ), StringCategory::BUTTON_LABELS);
    
    addString(LocalizedString(
        DEPTH_UP, "Depth+", "D+", 
        "Increase velocity modulation depth by 10 percent",
        6, UIContext::HUD_MAIN
    ), StringCategory::BUTTON_LABELS);
    
    addString(LocalizedString(
        DEPTH_DOWN, "Depth-", "D-", 
        "Decrease velocity modulation depth by 10 percent",
        6, UIContext::HUD_MAIN
    ), StringCategory::BUTTON_LABELS);
    
    addString(LocalizedString(
        VELOCITY_HELP, "Help", "HELP", 
        "Show velocity modulation help and documentation",
        12, UIContext::HELP_SYSTEM
    ), StringCategory::BUTTON_LABELS);
    
    addString(LocalizedString(
        ENGINE_SELECT, "Engine", "ENGINE", 
        "Select synthesis engine for velocity mapping configuration",
        12, UIContext::ENGINE_CONFIG
    ), StringCategory::BUTTON_LABELS);
}

void VelocityUIStrings::initializeOverlayHints() {
    using namespace VelocityStrings::Overlays;
    
    // Comprehensive overlay descriptions for complex features
    addString(LocalizedString(
        VELOCITY_SYSTEM_OVERVIEW,
        "EtherSynth's velocity modulation system provides musical expression through key velocity. "
        "Velocity affects multiple synthesis parameters simultaneously, creating dynamic and expressive performances. "
        "The system supports 0-200% modulation depth, multiple curve types, and engine-specific parameter mapping. "
        "Use the depth control to set global sensitivity, curve selection for response character, "
        "and engine mapping to customize which parameters respond to velocity.",
        "Velocity system overview",
        "Complete velocity modulation system with depth control, curve shaping, and engine-specific mapping",
        0, UIContext::HUD_MAIN
    ), StringCategory::OVERLAY_HINTS);
    
    addString(LocalizedString(
        DEPTH_CONTROL_DETAIL,
        "Velocity depth controls the intensity of velocity modulation across all parameters. "
        "0% = no velocity effect, 100% = normal MIDI velocity response, 200% = double sensitivity. "
        "Individual parameters can override global depth with their own scaling factors. "
        "Depth changes are smoothed to prevent audio artifacts during performance. "
        "Use lower depths (25-75%) for subtle expression, higher depths (125-200%) for dramatic effects.",
        "Depth control details",
        "Global velocity modulation depth from 0 to 200 percent with smooth transitions",
        0, UIContext::VELOCITY_PANEL
    ), StringCategory::OVERLAY_HINTS);
    
    addString(LocalizedString(
        CURVE_TYPES_EXPLANATION,
        "Velocity curves shape how key velocity translates to modulation amount:\\n"
        "• LINEAR: Direct 1:1 relationship (default MIDI behavior)\\n"
        "• EXPONENTIAL: More sensitivity at low velocities, gradual at high\\n"
        "• LOGARITHMIC: Gradual at low velocities, more sensitivity at high\\n"
        "• S-CURVE: Gentle at extremes, steep response in middle range\\n"
        "• POWER: Configurable exponential curve with adjustable steepness\\n"
        "• STEPPED: Quantized velocity levels for rhythmic effects\\n"
        "Curve selection affects the musical character and playability of velocity response.",
        "Curve types explained",
        "Six velocity response curves: linear, exponential, logarithmic, S-curve, power law, and stepped",
        0, UIContext::VELOCITY_PANEL
    ), StringCategory::OVERLAY_HINTS);
    
    addString(LocalizedString(
        ENGINE_MAPPING_GUIDE,
        "Each synthesis engine has optimized velocity mappings for musical expression:\\n"
        "• VA Engine: Attack time, filter cutoff, oscillator levels\\n"
        "• FM Engine: Modulation index, carrier/modulator balance\\n"
        "• Harmonics: Drawbar levels, percussion intensity\\n"
        "• Wavetable: Table position, filter tracking\\n"
        "• Physical Models: Bow pressure, excitation intensity\\n"
        "• Granular: Grain density, texture parameters\\n"
        "Mappings can be customized per engine or use factory presets for instant musical results.",
        "Engine mapping guide",
        "Engine-specific velocity parameter mappings optimized for each synthesis method",
        0, UIContext::ENGINE_CONFIG
    ), StringCategory::OVERLAY_HINTS);
    
    addString(LocalizedString(
        PRESET_SYSTEM_HELP,
        "Velocity presets provide instant access to musically useful configurations:\\n"
        "• CLEAN: Minimal velocity response, pure synthesis tones\\n"
        "• CLASSIC: Traditional MIDI velocity behavior, balanced response\\n"
        "• EXTREME: Maximum velocity sensitivity, dramatic expression\\n"
        "• SIGNATURE: Hand-crafted presets for specific musical styles\\n"
        "Presets include engine mappings, curve settings, and depth configurations. "
        "Custom presets can be created and saved for personal playing styles.",
        "Preset system help",
        "Velocity configuration presets with clean, classic, extreme, and signature categories",
        0, UIContext::PRESET_BROWSER
    ), StringCategory::OVERLAY_HINTS);
    
    addString(LocalizedString(
        PERFORMANCE_TIPS,
        "Performance tips for expressive velocity playing:\\n"
        "• Start with 75-100% depth for natural response\\n"
        "• Use exponential curves for piano-like expression\\n"
        "• Try S-curve for balanced touch sensitivity\\n"
        "• Lower depth (25-50%) for subtle ambient textures\\n"
        "• Higher depth (150-200%) for dramatic dynamic range\\n"
        "• Map velocity to multiple parameters for rich expression\\n"
        "• Use engine-specific presets as starting points",
        "Performance tips",
        "Playing technique recommendations for expressive velocity modulation",
        0, UIContext::PERFORMANCE_VIEW
    ), StringCategory::OVERLAY_HINTS);
    
    addString(LocalizedString(
        TROUBLESHOOTING,
        "Common velocity modulation issues and solutions:\\n"
        "• No velocity response: Check global enable, verify depth > 0%\\n"
        "• Too sensitive: Reduce depth or try logarithmic curve\\n"
        "• Not sensitive enough: Increase depth or use exponential curve\\n"
        "• Choppy response: Enable velocity smoothing, check CPU load\\n"
        "• Wrong parameters affected: Review engine mapping configuration\\n"
        "• Preset won't load: Verify preset compatibility with current engine\\n"
        "Reset to defaults if settings become unstable.",
        "Troubleshooting guide",
        "Solutions for common velocity modulation problems and configuration issues",
        0, UIContext::HELP_SYSTEM
    ), StringCategory::OVERLAY_HINTS);
    
    addString(LocalizedString(
        ADVANCED_FEATURES,
        "Advanced velocity modulation features:\\n"
        "• Per-parameter depth scaling with individual curves\\n"
        "• Velocity history smoothing for consistent response\\n"
        "• Bidirectional modulation (positive and negative)\\n"
        "• Envelope-driven velocity effects over time\\n"
        "• Custom curve tables for unique response characteristics\\n"
        "• Real-time velocity morphing between settings\\n"
        "• Integration with macro controls and performance features\\n"
        "Access through engine-specific configuration panels.",
        "Advanced features",
        "Professional velocity modulation features for complex synthesis programming",
        0, UIContext::VELOCITY_PANEL
    ), StringCategory::OVERLAY_HINTS);
}

void VelocityUIStrings::initializeStatusMessages() {
    using namespace VelocityStrings::Status;
    
    // Real-time status feedback messages
    addString(LocalizedString(
        VELOCITY_ENABLED, "Velocity ON", "VEL ON", 
        "Velocity modulation is now active for all mapped parameters",
        20, UIContext::HUD_MAIN
    ), StringCategory::STATUS_MESSAGES);
    
    addString(LocalizedString(
        VELOCITY_DISABLED, "Velocity OFF", "VEL OFF", 
        "Velocity modulation is disabled - all parameters use base values",
        20, UIContext::HUD_MAIN
    ), StringCategory::STATUS_MESSAGES);
    
    addString(LocalizedString(
        DEPTH_CHANGED, "Depth: {0}%", "D:{0}%", 
        "Global velocity modulation depth changed to {0} percent",
        15, UIContext::VELOCITY_PANEL
    ), StringCategory::STATUS_MESSAGES);
    
    addString(LocalizedString(
        CURVE_CHANGED, "Curve: {0}", "C:{0}", 
        "Velocity response curve changed to {0} type",
        20, UIContext::VELOCITY_PANEL
    ), StringCategory::STATUS_MESSAGES);
    
    addString(LocalizedString(
        MAPPING_UPDATED, "Mapping Updated", "MAP OK", 
        "Engine velocity parameter mapping has been updated successfully",
        25, UIContext::ENGINE_CONFIG
    ), StringCategory::STATUS_MESSAGES);
    
    addString(LocalizedString(
        PRESET_LOADED, "Preset: {0}", "P:{0}", 
        "Velocity preset {0} loaded successfully with all parameters",
        30, UIContext::PRESET_BROWSER
    ), StringCategory::STATUS_MESSAGES);
    
    addString(LocalizedString(
        SYSTEM_READY, "Velocity Ready", "VEL RDY", 
        "Velocity modulation system initialized and ready for performance",
        25, UIContext::HUD_MAIN
    ), StringCategory::STATUS_MESSAGES);
    
    addString(LocalizedString(
        PROCESSING_VOICES, "Voices: {0}", "V:{0}", 
        "Processing velocity modulation for {0} active voices",
        15, UIContext::PERFORMANCE_VIEW
    ), StringCategory::STATUS_MESSAGES);
}

void VelocityUIStrings::initializeErrorMessages() {
    using namespace VelocityStrings::Errors;
    
    // Error and warning messages with helpful guidance
    addString(LocalizedString(
        VELOCITY_INIT_FAILED, "Velocity Init Failed", "INIT ERR", 
        "Velocity modulation system failed to initialize - check system resources",
        30, UIContext::HUD_MAIN
    ), StringCategory::ERROR_MESSAGES);
    
    addString(LocalizedString(
        INVALID_DEPTH_RANGE, "Invalid Depth", "DEPTH ERR", 
        "Depth value must be between 0% and 200% - value has been clamped",
        25, UIContext::VELOCITY_PANEL
    ), StringCategory::ERROR_MESSAGES);
    
    addString(LocalizedString(
        CURVE_LOAD_FAILED, "Curve Load Failed", "CURVE ERR", 
        "Unable to load velocity curve - using linear curve as fallback",
        30, UIContext::VELOCITY_PANEL
    ), StringCategory::ERROR_MESSAGES);
    
    addString(LocalizedString(
        ENGINE_MAP_ERROR, "Mapping Error", "MAP ERR", 
        "Engine velocity mapping configuration error - check parameter assignments",
        25, UIContext::ENGINE_CONFIG
    ), StringCategory::ERROR_MESSAGES);
    
    addString(LocalizedString(
        PRESET_NOT_FOUND, "Preset Not Found", "PSET ERR", 
        "Requested velocity preset could not be found - using default configuration",
        30, UIContext::PRESET_BROWSER
    ), StringCategory::ERROR_MESSAGES);
    
    addString(LocalizedString(
        SYSTEM_OVERLOAD, "System Overload", "OVERLOAD", 
        "Velocity processing overload detected - reduce voice count or complexity",
        25, UIContext::PERFORMANCE_VIEW
    ), StringCategory::ERROR_MESSAGES);
    
    addString(LocalizedString(
        MEMORY_ERROR, "Memory Error", "MEM ERR", 
        "Insufficient memory for velocity processing - reduce active parameters",
        25, UIContext::HUD_MAIN
    ), StringCategory::ERROR_MESSAGES);
}

void VelocityUIStrings::initializeAccessibilityStrings() {
    // Screen reader friendly descriptions for all controls
    addString(LocalizedString(
        "vel_enable_aria", "Velocity Modulation Enable", "", 
        "Toggle button to enable or disable velocity modulation for all synthesis parameters. "
        "When enabled, key velocity affects multiple sound characteristics for expressive playing.",
        0, UIContext::HUD_MAIN
    ), StringCategory::ACCESSIBILITY);
    
    addString(LocalizedString(
        "depth_slider_aria", "Velocity Modulation Depth", "", 
        "Slider control for global velocity modulation depth. "
        "Range from 0 percent for no velocity effect to 200 percent for double sensitivity. "
        "Current value is {0} percent.",
        0, UIContext::VELOCITY_PANEL
    ), StringCategory::ACCESSIBILITY);
    
    addString(LocalizedString(
        "curve_selector_aria", "Velocity Response Curve", "", 
        "Dropdown menu to select velocity response curve type. "
        "Options include linear, exponential, logarithmic, S-curve, power law, and stepped curves. "
        "Current selection is {0} curve.",
        0, UIContext::VELOCITY_PANEL
    ), StringCategory::ACCESSIBILITY);
    
    addString(LocalizedString(
        "engine_map_aria", "Engine Velocity Mapping", "", 
        "Configuration panel for engine-specific velocity parameter mappings. "
        "Shows which synthesis parameters respond to velocity for the current engine type. "
        "Use arrow keys to navigate parameter list.",
        0, UIContext::ENGINE_CONFIG
    ), StringCategory::ACCESSIBILITY);
}

void VelocityUIStrings::initializeTooltips() {
    // Concise tooltip text for hover/long-press interactions
    addString(LocalizedString(
        "vel_enable_tip", "Enable/disable velocity modulation", "", 
        "Toggle velocity modulation for all parameters",
        0, UIContext::HUD_MAIN
    ), StringCategory::TOOLTIPS);
    
    addString(LocalizedString(
        "depth_control_tip", "Adjust velocity sensitivity", "", 
        "Control how strongly velocity affects sound parameters",
        0, UIContext::VELOCITY_PANEL
    ), StringCategory::TOOLTIPS);
    
    addString(LocalizedString(
        "curve_type_tip", "Select velocity response curve", "", 
        "Choose how velocity translates to parameter changes",
        0, UIContext::VELOCITY_PANEL
    ), StringCategory::TOOLTIPS);
    
    addString(LocalizedString(
        "preset_load_tip", "Load velocity preset", "", 
        "Quick access to predefined velocity configurations",
        0, UIContext::PRESET_BROWSER
    ), StringCategory::TOOLTIPS);
}

void VelocityUIStrings::initializeTemplates() {
    // String templates for dynamic content generation
    templates_["depth_value"] = "Depth: {0}%";
    templates_["curve_name"] = "Curve: {0}";
    templates_["preset_name"] = "Preset: {0}";
    templates_["voice_count"] = "Voices: {0}";
    templates_["engine_type"] = "Engine: {0}";
    templates_["parameter_value"] = "{0}: {1}";
    templates_["mapping_status"] = "{0} → {1} ({2}%)";
    templates_["error_context"] = "Error in {0}: {1}";
}

// String retrieval methods
std::string VelocityUIStrings::getString(const std::string& id) const {
    auto it = strings_.find(id);
    return (it != strings_.end()) ? it->second.english : "[MISSING: " + id + "]";
}

std::string VelocityUIStrings::getBriefString(const std::string& id) const {
    auto it = strings_.find(id);
    return (it != strings_.end()) ? it->second.brief : getString(id);
}

std::string VelocityUIStrings::getAccessibleString(const std::string& id) const {
    auto it = strings_.find(id);
    return (it != strings_.end()) ? it->second.accessible : getString(id);
}

std::string VelocityUIStrings::getTooltip(const std::string& id) const {
    std::string tooltipId = id + "_tip";
    return getString(tooltipId);
}

// Context-aware retrieval
std::string VelocityUIStrings::getStringForContext(const std::string& id, UIContext context) const {
    auto it = strings_.find(id);
    if (it != strings_.end() && it->second.context == context) {
        return it->second.english;
    }
    return getString(id); // Fallback to default
}

// Dynamic string formatting
std::string VelocityUIStrings::formatStatusMessage(const std::string& templateId, 
                                                  const std::vector<std::string>& params) const {
    auto it = templates_.find(templateId);
    if (it != templates_.end()) {
        return formatTemplate(it->second, params);
    }
    return getString(templateId);
}

std::string VelocityUIStrings::formatTemplate(const std::string& templateStr, 
                                             const std::vector<std::string>& params) const {
    std::string result = templateStr;
    for (size_t i = 0; i < params.size(); ++i) {
        std::string placeholder = "{" + std::to_string(i) + "}";
        size_t pos = result.find(placeholder);
        if (pos != std::string::npos) {
            result.replace(pos, placeholder.length(), params[i]);
        }
    }
    return result;
}

// String validation and utility
bool VelocityUIStrings::validateStringLength(const std::string& id, const std::string& text) const {
    auto it = strings_.find(id);
    if (it != strings_.end() && it->second.maxLength > 0) {
        return text.length() <= it->second.maxLength;
    }
    return true; // No constraint
}

std::string VelocityUIStrings::truncateForUI(const std::string& text, size_t maxLength) const {
    if (text.length() <= maxLength) {
        return text;
    }
    if (maxLength <= 3) {
        return text.substr(0, maxLength);
    }
    return text.substr(0, maxLength - 3) + "...";
}

// Category and context organization
void VelocityUIStrings::addString(const LocalizedString& str, StringCategory category) {
    strings_[str.id] = str;
    categories_[std::to_string(static_cast<int>(category))].push_back(str.id);
    contextStrings_[str.context].push_back(str.id);
}

void VelocityUIStrings::categorizeAllStrings() {
    // Additional categorization logic if needed
    // This could sort strings by length, frequency, or other criteria
}

// Engine-specific string generation
std::string VelocityUIStrings::getEngineSpecificString(const std::string& baseId, 
                                                      const std::string& engineType) const {
    std::string engineSpecificId = baseId + "_" + engineType;
    auto it = strings_.find(engineSpecificId);
    if (it != strings_.end()) {
        return it->second.english;
    }
    return getString(baseId); // Fallback to generic version
}

std::string VelocityUIStrings::getParameterString(const std::string& parameterId, float value) const {
    std::ostringstream oss;
    oss.precision(1);
    oss << std::fixed;
    
    if (parameterId.find("depth") != std::string::npos) {
        oss << (value * 100.0f) << "%";
    } else if (parameterId.find("time") != std::string::npos) {
        oss << (value * 1000.0f) << "ms";
    } else {
        oss << value;
    }
    
    return oss.str();
}