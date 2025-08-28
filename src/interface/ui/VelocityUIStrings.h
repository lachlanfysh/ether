#pragma once
#include <string>
#include <unordered_map>
#include <vector>

/**
 * VelocityUIStrings - Comprehensive UI strings for velocity modulation interface
 * 
 * Provides all text strings used in the velocity modulation HUD and overlay system:
 * - Button labels and descriptions for all velocity controls
 * - Contextual hints and help text for complex velocity features
 * - Status messages and feedback for velocity operations
 * - Error messages and troubleshooting guidance
 * - Accessibility descriptions for screen readers and assistive technology
 * 
 * Features:
 * - Consistent terminology across all velocity-related UI elements
 * - Clear, concise descriptions that fit within UI space constraints
 * - Progressive disclosure: brief labels with detailed overlay hints
 * - Context-sensitive help text based on current velocity configuration
 * - Localization-ready string structure with unique identifiers
 * - Accessibility support with ARIA labels and descriptions
 */
class VelocityUIStrings {
public:
    // String categories for organization
    enum class StringCategory {
        BUTTON_LABELS,      // Short button text (max 12 chars)
        OVERLAY_HINTS,      // Detailed overlay descriptions
        STATUS_MESSAGES,    // Real-time status feedback
        ERROR_MESSAGES,     // Error and warning text
        ACCESSIBILITY,      // Screen reader descriptions
        TOOLTIPS,          // Hover/long-press tooltip text
        CONTEXT_HELP       // Context-sensitive help
    };
    
    // UI context for dynamic string selection
    enum class UIContext {
        HUD_MAIN,          // Main HUD overlay
        VELOCITY_PANEL,    // Velocity modulation panel
        ENGINE_CONFIG,     // Engine-specific velocity config
        PRESET_BROWSER,    // Preset selection interface
        PERFORMANCE_VIEW,  // Live performance overlay
        SETTINGS_MENU,     // Settings and configuration
        HELP_SYSTEM       // Help and documentation
    };
    
    // String localization support
    struct LocalizedString {
        std::string id;             // Unique identifier
        std::string english;        // English text
        std::string brief;          // Abbreviated version
        std::string accessible;     // Screen reader version
        size_t maxLength;           // UI space constraint
        UIContext context;          // Where this string appears
        
        LocalizedString(const std::string& id, const std::string& en, 
                       const std::string& br = "", const std::string& acc = "",
                       size_t max = 0, UIContext ctx = UIContext::HUD_MAIN) :
            id(id), english(en), brief(br.empty() ? en : br), 
            accessible(acc.empty() ? en : acc), maxLength(max), context(ctx) {}
    };
    
    VelocityUIStrings();
    ~VelocityUIStrings() = default;
    
    // Core string retrieval
    std::string getString(const std::string& id) const;
    std::string getBriefString(const std::string& id) const;
    std::string getAccessibleString(const std::string& id) const;
    std::string getTooltip(const std::string& id) const;
    
    // Context-aware string selection
    std::string getStringForContext(const std::string& id, UIContext context) const;
    std::vector<std::string> getStringsForCategory(StringCategory category) const;
    std::vector<std::string> getStringsForContext(UIContext context) const;
    
    // Dynamic string generation
    std::string formatStatusMessage(const std::string& templateId, const std::vector<std::string>& params) const;
    std::string getEngineSpecificString(const std::string& baseId, const std::string& engineType) const;
    std::string getParameterString(const std::string& parameterId, float value) const;
    
    // String validation and constraints
    bool validateStringLength(const std::string& id, const std::string& text) const;
    std::string truncateForUI(const std::string& text, size_t maxLength) const;
    std::vector<std::string> getOverlongStrings() const;
    
    // Accessibility support
    std::string getARIALabel(const std::string& id) const;
    std::string getARIADescription(const std::string& id) const;
    std::string getKeyboardShortcutDescription(const std::string& actionId) const;
    
private:
    // String storage
    std::unordered_map<std::string, LocalizedString> strings_;
    std::unordered_map<std::string, std::vector<std::string>> categories_;
    std::unordered_map<UIContext, std::vector<std::string>> contextStrings_;
    
    // String templates for dynamic content
    std::unordered_map<std::string, std::string> templates_;
    
    // Initialization methods
    void initializeButtonLabels();
    void initializeOverlayHints();
    void initializeStatusMessages();
    void initializeErrorMessages();
    void initializeAccessibilityStrings();
    void initializeTooltips();
    void initializeTemplates();
    
    // String categories
    void addString(const LocalizedString& str, StringCategory category);
    void categorizeAllStrings();
    
    // Utility methods
    std::string formatTemplate(const std::string& templateStr, const std::vector<std::string>& params) const;
    std::string ellipsize(const std::string& text, size_t maxLength) const;
};

// Global string definitions organized by functional area
namespace VelocityStrings {
    // Button Labels (max 12 characters for UI space)
    namespace Buttons {
        constexpr const char* VELOCITY_ENABLE = "VEL_ENABLE";
        constexpr const char* DEPTH_CONTROL = "DEPTH_CTL";
        constexpr const char* CURVE_TYPE = "CURVE_TYPE";
        constexpr const char* ENGINE_MAP = "ENGINE_MAP";
        constexpr const char* PRESET_LOAD = "LOAD_PSET";
        constexpr const char* VELOCITY_RESET = "VEL_RESET";
        constexpr const char* MAPPING_EDIT = "EDIT_MAP";
        constexpr const char* CURVE_ADJUST = "ADJ_CURVE";
        constexpr const char* DEPTH_UP = "DEPTH_UP";
        constexpr const char* DEPTH_DOWN = "DEPTH_DN";
        constexpr const char* VELOCITY_HELP = "VEL_HELP";
        constexpr const char* ENGINE_SELECT = "ENG_SEL";
    }
    
    // Overlay Hints (detailed descriptions for context overlay)
    namespace Overlays {
        constexpr const char* VELOCITY_SYSTEM_OVERVIEW = "VEL_OVERVIEW";
        constexpr const char* DEPTH_CONTROL_DETAIL = "DEPTH_DETAIL";
        constexpr const char* CURVE_TYPES_EXPLANATION = "CURVE_EXPLAIN";
        constexpr const char* ENGINE_MAPPING_GUIDE = "MAP_GUIDE";
        constexpr const char* PRESET_SYSTEM_HELP = "PRESET_HELP";
        constexpr const char* PERFORMANCE_TIPS = "PERF_TIPS";
        constexpr const char* TROUBLESHOOTING = "TROUBLE";
        constexpr const char* ADVANCED_FEATURES = "ADVANCED";
    }
    
    // Status Messages (real-time feedback)
    namespace Status {
        constexpr const char* VELOCITY_ENABLED = "VEL_ON";
        constexpr const char* VELOCITY_DISABLED = "VEL_OFF";
        constexpr const char* DEPTH_CHANGED = "DEPTH_CHG";
        constexpr const char* CURVE_CHANGED = "CURVE_CHG";
        constexpr const char* MAPPING_UPDATED = "MAP_UPD";
        constexpr const char* PRESET_LOADED = "PSET_LOAD";
        constexpr const char* SYSTEM_READY = "SYS_RDY";
        constexpr const char* PROCESSING_VOICES = "PROC_VOICE";
    }
    
    // Error Messages
    namespace Errors {
        constexpr const char* VELOCITY_INIT_FAILED = "VEL_INIT_ERR";
        constexpr const char* INVALID_DEPTH_RANGE = "DEPTH_ERR";
        constexpr const char* CURVE_LOAD_FAILED = "CURVE_ERR";
        constexpr const char* ENGINE_MAP_ERROR = "MAP_ERR";
        constexpr const char* PRESET_NOT_FOUND = "PSET_ERR";
        constexpr const char* SYSTEM_OVERLOAD = "OVERLOAD";
        constexpr const char* MEMORY_ERROR = "MEM_ERR";
    }
    
    // Parameter Names (for dynamic display)
    namespace Parameters {
        constexpr const char* MASTER_DEPTH = "MASTER_DEPTH";
        constexpr const char* CURVE_AMOUNT = "CURVE_AMT";
        constexpr const char* VELOCITY_SCALE = "VEL_SCALE";
        constexpr const char* ENGINE_TARGET = "ENG_TARGET";
        constexpr const char* MAPPING_AMOUNT = "MAP_AMT";
        constexpr const char* RESPONSE_TIME = "RESP_TIME";
    }
}