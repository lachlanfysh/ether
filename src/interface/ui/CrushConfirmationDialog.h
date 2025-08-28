#pragma once
#include <cstdint>
#include <string>
#include <functional>
#include <memory>
#include "../sequencer/PatternSelection.h"

/**
 * CrushConfirmationDialog - Interactive confirmation dialog for tape crushing operations
 * 
 * Provides comprehensive confirmation and safety features for destructive tape squashing:
 * - Clear presentation of what will be affected by crush operation
 * - Auto-save functionality to protect against data loss
 * - Preview of final sample name and destination slot
 * - Integration with pattern backup system for safety
 * - User-friendly TouchGFX interface for embedded display
 * - Hardware-optimized for STM32 H7 embedded platform
 * 
 * Features:
 * - Visual preview of affected pattern region
 * - Sample destination information and overwrite warnings
 * - Auto-save toggle with customizable save locations
 * - Backup creation options for undo functionality
 * - Pattern analysis and impact assessment
 * - Integration with tape squashing workflow
 * - Keyboard shortcuts for power users
 * - Accessibility features for embedded interface
 */
class CrushConfirmationDialog {
public:
    // Dialog result codes
    enum class DialogResult {
        CANCELLED,          // User cancelled the operation
        CONFIRMED,          // User confirmed the operation
        SAVE_AND_CONFIRM,   // User chose to save first, then confirm
        ERROR               // Error occurred during dialog operation
    };
    
    // Auto-save options
    struct AutoSaveOptions {
        bool enableAutoSave;            // Enable automatic saving before crush
        bool saveCurrentPattern;        // Save entire current pattern
        bool saveAffectedRegionOnly;    // Save only affected region
        bool createBackupCopy;          // Create timestamped backup copy
        std::string saveLocation;       // Save location (empty = default)
        std::string backupPrefix;       // Prefix for backup files
        bool promptForSaveLocation;     // Ask user for save location
        
        AutoSaveOptions() :
            enableAutoSave(true),
            saveCurrentPattern(true),
            saveAffectedRegionOnly(false),
            createBackupCopy(true),
            backupPrefix("Backup_"),
            promptForSaveLocation(false) {}
    };
    
    // Confirmation dialog configuration
    struct DialogConfig {
        PatternSelection::SelectionBounds selection;  // Selected region to be crushed
        std::string sampleName;                      // Name of resulting sample
        uint8_t destinationSlot;                     // Target sampler slot
        bool willOverwriteExistingSample;            // Whether existing sample will be overwritten
        std::string existingSampleName;              // Name of sample being overwritten
        AutoSaveOptions autoSaveOptions;             // Auto-save configuration
        uint32_t affectedSteps;                      // Number of steps that will be affected
        uint32_t affectedTracks;                     // Number of tracks that will be affected
        float estimatedCrushTimeSeconds;             // Estimated time for crush operation
        bool hasComplexPatternData;                  // Whether region contains complex data
        
        DialogConfig() :
            destinationSlot(255),
            willOverwriteExistingSample(false),
            affectedSteps(0),
            affectedTracks(0),
            estimatedCrushTimeSeconds(0.0f),
            hasComplexPatternData(false) {}
    };
    
    // Dialog display information
    struct DialogInfo {
        std::string title;              // Dialog title
        std::string mainMessage;        // Primary message text
        std::string detailMessage;      // Detailed information
        std::string warningMessage;     // Warning text (if any)
        std::vector<std::string> affectedItems;  // List of affected items
        bool showProgressEstimate;      // Whether to show time estimate
        bool requiresUserConfirmation;  // Whether explicit confirmation needed
        
        DialogInfo() :
            showProgressEstimate(true),
            requiresUserConfirmation(true) {}
    };
    
    CrushConfirmationDialog();
    ~CrushConfirmationDialog() = default;
    
    // Dialog Management
    DialogResult showConfirmationDialog(const DialogConfig& config);
    void closeDialog();
    bool isDialogOpen() const { return isOpen_; }
    void setDialogTimeout(uint32_t timeoutMs);  // Auto-close after timeout
    
    // Configuration
    void setAutoSaveOptions(const AutoSaveOptions& options);
    const AutoSaveOptions& getAutoSaveOptions() const { return autoSaveOptions_; }
    void setDialogDefaults(const DialogConfig& defaults);
    
    // Dialog Content Generation
    DialogInfo generateDialogInfo(const DialogConfig& config) const;
    std::string generateMainMessage(const DialogConfig& config) const;
    std::string generateDetailMessage(const DialogConfig& config) const;
    std::string generateWarningMessage(const DialogConfig& config) const;
    
    // Auto-Save Operations
    bool performAutoSave(const DialogConfig& config);
    bool saveCurrentPattern(const std::string& location = "");
    bool saveSelectedRegion(const PatternSelection::SelectionBounds& selection, 
                           const std::string& location = "");
    bool createBackupCopy(const std::string& originalPath, const std::string& backupPrefix);
    
    // User Interaction
    void setConfirmButtonEnabled(bool enabled);
    void setCancelButtonEnabled(bool enabled);
    void setAutoSaveCheckboxState(bool enabled);
    void updateProgressEstimate(float timeSeconds);
    
    // Integration
    void integrateWithPatternDataReplacer(class PatternDataReplacer* patternReplacer);
    void integrateWithAutoSampleLoader(class AutoSampleLoader* sampleLoader);
    void integrateWithSequencer(class SequencerEngine* sequencer);
    
    // Callbacks
    using ConfirmationCallback = std::function<void(DialogResult result, const DialogConfig& config)>;
    using AutoSaveCallback = std::function<bool(const std::string& savePath)>;
    using ValidationCallback = std::function<bool(const DialogConfig& config, std::string& errorMessage)>;
    
    void setConfirmationCallback(ConfirmationCallback callback);
    void setAutoSaveCallback(AutoSaveCallback callback);
    void setValidationCallback(ValidationCallback callback);
    
    // TouchGFX Integration
    void setupTouchGFXElements();
    void handleTouchEvent(int16_t x, int16_t y, bool isPressed);
    void updateTouchGFXDisplay();
    void animateDialogTransition(bool opening);
    
    // Keyboard/Button Support
    void handleKeyPress(uint8_t keyCode);
    void handleConfirmButton();
    void handleCancelButton();
    void handleAutoSaveToggle();
    
    // Accessibility
    void setAccessibilityEnabled(bool enabled);
    std::string generateAccessibilityText() const;
    void announceDialogContent();
    
private:
    // Dialog state
    bool isOpen_;
    DialogConfig currentConfig_;
    AutoSaveOptions autoSaveOptions_;
    DialogResult pendingResult_;
    
    // Timing
    uint32_t dialogOpenTime_;
    uint32_t dialogTimeout_;
    bool hasTimeout_;
    
    // TouchGFX elements (mock structures for embedded GUI)
    struct TouchGFXElements {
        void* mainContainer;        // Main dialog container
        void* titleText;           // Title text element
        void* messageText;         // Message text element
        void* detailsList;         // Details list element
        void* progressBar;         // Progress estimate bar
        void* confirmButton;       // Confirm button
        void* cancelButton;        // Cancel button
        void* autoSaveCheckbox;    // Auto-save checkbox
        void* warningIcon;         // Warning icon
        
        TouchGFXElements() : mainContainer(nullptr), titleText(nullptr), 
                           messageText(nullptr), detailsList(nullptr),
                           progressBar(nullptr), confirmButton(nullptr),
                           cancelButton(nullptr), autoSaveCheckbox(nullptr),
                           warningIcon(nullptr) {}
    } touchElements_;
    
    // Integration
    class PatternDataReplacer* patternReplacer_;
    class AutoSampleLoader* sampleLoader_;
    class SequencerEngine* sequencer_;
    
    // Callbacks
    ConfirmationCallback confirmationCallback_;
    AutoSaveCallback autoSaveCallback_;
    ValidationCallback validationCallback_;
    
    // Internal operations
    bool validateDialogConfig(const DialogConfig& config, std::string& errorMessage);
    void setupDialogContent(const DialogConfig& config);
    void updateDialogDisplay();
    void handleDialogTimeout();
    
    // Content generation helpers
    std::string formatTimeEstimate(float seconds) const;
    std::string formatAffectedItems(uint32_t steps, uint32_t tracks) const;
    std::string formatSlotInformation(uint8_t slot, bool willOverwrite, 
                                     const std::string& existingName) const;
    
    // Auto-save helpers
    std::string generateSavePath(const AutoSaveOptions& options) const;
    std::string generateBackupPath(const std::string& originalPath, 
                                  const std::string& prefix) const;
    bool ensureSaveDirectoryExists(const std::string& path);
    
    // TouchGFX helpers
    void createTouchGFXElements();
    void destroyTouchGFXElements();
    void layoutDialogElements();
    void applyDialogStyling();
    
    // Animation helpers
    void startOpenAnimation();
    void startCloseAnimation();
    void updateAnimationFrame(float progress);
    
    // Validation helpers
    bool isValidSavePath(const std::string& path) const;
    bool hasWritePermission(const std::string& path) const;
    bool checkDiskSpace(const std::string& path, size_t requiredBytes) const;
    
    // Utility methods
    uint32_t getCurrentTimeMs() const;
    std::string getCurrentTimestamp() const;
    
    // Constants
    static constexpr uint32_t DEFAULT_DIALOG_TIMEOUT_MS = 30000;  // 30 seconds
    static constexpr uint16_t DIALOG_WIDTH = 400;
    static constexpr uint16_t DIALOG_HEIGHT = 300;
    static constexpr uint16_t ANIMATION_DURATION_MS = 300;
    static constexpr uint8_t MAX_DETAIL_ITEMS = 10;
    static constexpr size_t MIN_FREE_DISK_SPACE = 10 * 1024 * 1024;  // 10MB
};