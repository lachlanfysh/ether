#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include "../sequencer/PatternSelection.h"

/**
 * TapeSquashingUI - Tape squashing interface with 'Crush to Tape' action button
 * 
 * Provides comprehensive UI for the tape squashing workflow:
 * - Visual confirmation of selected region for tape squashing
 * - 'Crush to Tape' action button with progress indication
 * - Settings panel for tape squashing parameters and options
 * - Integration with pattern selection and audio bouncing systems
 * - Real-time progress feedback during audio capture and processing
 * - Confirmation dialogs and error handling for tape squashing operations
 * 
 * Features:
 * - Interactive 'Crush to Tape' button with visual feedback
 * - Selection overview showing tracks, steps, and estimated duration
 * - Progress bar with detailed status messages during processing
 * - Settings for sample quality, auto-naming, and destination slot
 * - Confirmation dialog with auto-save option before destructive operations
 * - Integration with TouchGFX for embedded GUI rendering
 * - Error handling and recovery for failed tape squashing operations
 */
class TapeSquashingUI {
public:
    // Tape squashing operation states
    enum class SquashState {
        IDLE,               // No operation in progress
        PREPARING,          // Preparing for tape squashing
        CAPTURING,          // Capturing audio from selection
        PROCESSING,         // Processing captured audio
        LOADING_SAMPLE,     // Loading sample into sampler slot
        COMPLETED,          // Operation completed successfully
        ERROR,              // Error occurred during operation
        CANCELLED           // Operation was cancelled by user
    };
    
    // Tape squashing settings
    struct SquashSettings {
        float sampleRate;           // Sample rate for captured audio (Hz)
        uint8_t bitDepth;           // Bit depth (16, 24, 32)
        bool enableAutoNormalize;   // Auto-normalize captured audio
        bool enableAutoFade;        // Add fade-in/out to avoid clicks
        bool enableAutoName;        // Auto-generate sample names
        std::string namePrefix;     // Prefix for auto-generated names
        uint8_t targetSlot;         // Target sampler slot (0-15, 255=auto)
        bool confirmDestructive;    // Show confirmation for destructive operations
        bool enableAutoSave;        // Auto-save pattern before squashing
        uint16_t maxDurationMs;     // Maximum capture duration (ms)
        
        SquashSettings() :
            sampleRate(48000.0f),
            bitDepth(24),
            enableAutoNormalize(true),
            enableAutoFade(true),
            enableAutoName(true),
            namePrefix("Crush"),
            targetSlot(255),  // Auto-select
            confirmDestructive(true),
            enableAutoSave(true),
            maxDurationMs(10000) {}  // 10 second max
    };
    
    // Progress information for UI feedback
    struct ProgressInfo {
        SquashState currentState;
        float progressPercent;      // 0.0-100.0
        std::string statusMessage;
        uint32_t elapsedTimeMs;
        uint32_t estimatedTotalMs;
        bool canCancel;
        
        ProgressInfo() :
            currentState(SquashState::IDLE),
            progressPercent(0.0f),
            statusMessage("Ready"),
            elapsedTimeMs(0),
            estimatedTotalMs(0),
            canCancel(false) {}
    };
    
    // Selection overview information
    struct SelectionOverview {
        uint16_t trackCount;
        uint16_t stepCount;
        uint32_t totalCells;
        float estimatedDurationMs;
        bool hasAudio;              // Whether selection contains audio content
        std::vector<std::string> affectedTracks;
        
        SelectionOverview() :
            trackCount(0), stepCount(0), totalCells(0),
            estimatedDurationMs(0.0f), hasAudio(false) {}
    };
    
    TapeSquashingUI();
    ~TapeSquashingUI() = default;
    
    // UI State Management
    void show();
    void hide();
    bool isVisible() const { return visible_; }
    void update(uint32_t currentTimeMs);
    
    // Settings Configuration
    void setSquashSettings(const SquashSettings& settings);
    const SquashSettings& getSquashSettings() const { return settings_; }
    void showSettingsPanel();
    void hideSettingsPanel();
    
    // Selection Integration
    void setCurrentSelection(const PatternSelection::SelectionBounds& selection);
    void clearSelection();
    bool hasValidSelection() const;
    const SelectionOverview& getSelectionOverview() const { return selectionOverview_; }
    
    // Tape Squashing Operations
    bool canStartSquashing() const;
    void startTapeSquashing();
    void cancelTapeSquashing();
    bool isSquashingActive() const;
    
    // Progress Monitoring
    const ProgressInfo& getProgressInfo() const { return progressInfo_; }
    void updateProgress(SquashState state, float percent, const std::string& message);
    
    // UI Event Handlers
    void onCrushButtonPressed();
    void onCancelButtonPressed();
    void onSettingsButtonPressed();
    void onConfirmDialogYes();
    void onConfirmDialogNo();
    void onSlotSelectionChanged(uint8_t slot);
    void onQualitySettingChanged(float sampleRate, uint8_t bitDepth);
    void onAutoNormalizeToggled(bool enabled);
    void onAutoFadeToggled(bool enabled);
    void onAutoNameToggled(bool enabled);
    void onNamePrefixChanged(const std::string& prefix);
    
    // Visual Components (TouchGFX integration)
    void drawMainPanel(void* graphics);
    void drawCrushButton(void* graphics);
    void drawSelectionOverview(void* graphics);
    void drawProgressBar(void* graphics);
    void drawSettingsPanel(void* graphics);
    void drawConfirmationDialog(void* graphics);
    
    // Callbacks
    using TapeSquashCallback = std::function<void(const PatternSelection::SelectionBounds&, const SquashSettings&)>;
    using ProgressUpdateCallback = std::function<void(const ProgressInfo&)>;
    using CompletionCallback = std::function<void(bool success, const std::string& sampleName, uint8_t slot)>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    
    void setTapeSquashCallback(TapeSquashCallback callback);
    void setProgressUpdateCallback(ProgressUpdateCallback callback);
    void setCompletionCallback(CompletionCallback callback);
    void setErrorCallback(ErrorCallback callback);
    
    // Error Handling
    void handleError(const std::string& errorMessage);
    void showErrorDialog(const std::string& error);
    void hideErrorDialog();
    
private:
    // UI State
    bool visible_;
    bool settingsPanelVisible_;
    bool confirmDialogVisible_;
    bool errorDialogVisible_;
    
    // Current data
    SquashSettings settings_;
    PatternSelection::SelectionBounds currentSelection_;
    SelectionOverview selectionOverview_;
    ProgressInfo progressInfo_;
    
    // Error state
    std::string lastError_;
    
    // Timing
    uint32_t operationStartTime_;
    uint32_t lastUpdateTime_;
    
    // Callbacks
    TapeSquashCallback tapeSquashCallback_;
    ProgressUpdateCallback progressUpdateCallback_;
    CompletionCallback completionCallback_;
    ErrorCallback errorCallback_;
    
    // Helper methods
    void updateSelectionOverview();
    void calculateEstimatedDuration();
    bool validateSelection() const;
    bool validateSettings() const;
    std::string generateSampleName() const;
    uint8_t findNextAvailableSlot() const;
    void resetProgress();
    void notifyProgressUpdate();
    void notifyCompletion(bool success, const std::string& sampleName, uint8_t slot);
    void notifyError(const std::string& error);
    
    // UI Drawing Helpers
    void drawButton(void* graphics, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                   const std::string& text, uint32_t color, bool enabled);
    void drawProgressBar(void* graphics, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                        float percent, uint32_t fillColor, uint32_t bgColor);
    void drawText(void* graphics, uint16_t x, uint16_t y, const std::string& text, 
                  uint32_t color, uint8_t size);
    void drawPanel(void* graphics, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                   uint32_t bgColor, uint32_t borderColor);
    
    // Constants
    static constexpr uint16_t MAIN_PANEL_WIDTH = 400;
    static constexpr uint16_t MAIN_PANEL_HEIGHT = 300;
    static constexpr uint16_t CRUSH_BUTTON_WIDTH = 120;
    static constexpr uint16_t CRUSH_BUTTON_HEIGHT = 40;
    static constexpr uint16_t PROGRESS_BAR_HEIGHT = 20;
    
    // Colors
    static constexpr uint32_t COLOR_CRUSH_BUTTON_ENABLED = 0xFF3333;
    static constexpr uint32_t COLOR_CRUSH_BUTTON_DISABLED = 0x666666;
    static constexpr uint32_t COLOR_PROGRESS_FILL = 0x33FF33;
    static constexpr uint32_t COLOR_PROGRESS_BG = 0x333333;
    static constexpr uint32_t COLOR_PANEL_BG = 0x222222;
    static constexpr uint32_t COLOR_PANEL_BORDER = 0x888888;
    static constexpr uint32_t COLOR_TEXT_NORMAL = 0xFFFFFF;
    static constexpr uint32_t COLOR_TEXT_ERROR = 0xFF3333;
};