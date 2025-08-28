#include "TapeSquashingUI.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstdio>

TapeSquashingUI::TapeSquashingUI() {
    visible_ = false;
    settingsPanelVisible_ = false;
    confirmDialogVisible_ = false;
    errorDialogVisible_ = false;
    
    settings_ = SquashSettings();
    currentSelection_ = PatternSelection::SelectionBounds();
    selectionOverview_ = SelectionOverview();
    progressInfo_ = ProgressInfo();
    
    operationStartTime_ = 0;
    lastUpdateTime_ = 0;
}

// UI State Management
void TapeSquashingUI::show() {
    visible_ = true;
    updateSelectionOverview();
}

void TapeSquashingUI::hide() {
    visible_ = false;
    settingsPanelVisible_ = false;
    confirmDialogVisible_ = false;
    errorDialogVisible_ = false;
}

void TapeSquashingUI::update(uint32_t currentTimeMs) {
    if (!visible_) return;
    
    lastUpdateTime_ = currentTimeMs;
    
    if (isSquashingActive()) {
        // Update elapsed time for progress display
        progressInfo_.elapsedTimeMs = currentTimeMs - operationStartTime_;
        
        // Update estimated total time based on current progress
        if (progressInfo_.progressPercent > 5.0f) {
            float estimatedTotal = (progressInfo_.elapsedTimeMs / progressInfo_.progressPercent) * 100.0f;
            progressInfo_.estimatedTotalMs = static_cast<uint32_t>(estimatedTotal);
        }
        
        notifyProgressUpdate();
    }
}

// Settings Configuration
void TapeSquashingUI::setSquashSettings(const SquashSettings& settings) {
    settings_ = settings;
    
    // Validate and clamp settings
    settings_.sampleRate = std::max(8000.0f, std::min(settings_.sampleRate, 192000.0f));
    settings_.bitDepth = (settings_.bitDepth == 16 || settings_.bitDepth == 24 || settings_.bitDepth == 32) ? 
                        settings_.bitDepth : 24;
    settings_.maxDurationMs = std::max(static_cast<uint16_t>(1000), 
                                      std::min(settings_.maxDurationMs, static_cast<uint16_t>(60000)));
}

void TapeSquashingUI::showSettingsPanel() {
    settingsPanelVisible_ = true;
}

void TapeSquashingUI::hideSettingsPanel() {
    settingsPanelVisible_ = false;
}

// Selection Integration
void TapeSquashingUI::setCurrentSelection(const PatternSelection::SelectionBounds& selection) {
    currentSelection_ = selection;
    updateSelectionOverview();
}

void TapeSquashingUI::clearSelection() {
    currentSelection_ = PatternSelection::SelectionBounds();
    selectionOverview_ = SelectionOverview();
}

bool TapeSquashingUI::hasValidSelection() const {
    return currentSelection_.isValid() && 
           currentSelection_.getTotalCells() > 0 &&
           validateSelection() &&
           selectionOverview_.hasAudio;
}

// Tape Squashing Operations
bool TapeSquashingUI::isSquashingActive() const {
    return progressInfo_.currentState != SquashState::IDLE &&
           progressInfo_.currentState != SquashState::COMPLETED &&
           progressInfo_.currentState != SquashState::ERROR &&
           progressInfo_.currentState != SquashState::CANCELLED;
}

bool TapeSquashingUI::canStartSquashing() const {
    return hasValidSelection() && 
           !isSquashingActive() && 
           validateSettings() &&
           selectionOverview_.hasAudio;
}

void TapeSquashingUI::startTapeSquashing() {
    if (!canStartSquashing()) {
        handleError("Cannot start tape squashing: invalid selection or settings");
        return;
    }
    
    if (settings_.confirmDestructive) {
        confirmDialogVisible_ = true;
    } else {
        onConfirmDialogYes();
    }
}

void TapeSquashingUI::cancelTapeSquashing() {
    if (!isSquashingActive()) return;
    
    progressInfo_.currentState = SquashState::CANCELLED;
    progressInfo_.statusMessage = "Cancelling...";
    resetProgress();
    
    notifyProgressUpdate();
}

// Progress Monitoring
void TapeSquashingUI::updateProgress(SquashState state, float percent, const std::string& message) {
    progressInfo_.currentState = state;
    progressInfo_.progressPercent = std::max(0.0f, std::min(percent, 100.0f));
    progressInfo_.statusMessage = message;
    progressInfo_.canCancel = (state == SquashState::CAPTURING || state == SquashState::PROCESSING);
    
    notifyProgressUpdate();
}

// UI Event Handlers
void TapeSquashingUI::onCrushButtonPressed() {
    if (canStartSquashing()) {
        startTapeSquashing();
    }
}

void TapeSquashingUI::onCancelButtonPressed() {
    if (isSquashingActive()) {
        cancelTapeSquashing();
    } else {
        hide();
    }
}

void TapeSquashingUI::onSettingsButtonPressed() {
    settingsPanelVisible_ = !settingsPanelVisible_;
}

void TapeSquashingUI::onConfirmDialogYes() {
    confirmDialogVisible_ = false;
    
    // Start the actual tape squashing operation
    operationStartTime_ = lastUpdateTime_;
    resetProgress();
    
    progressInfo_.currentState = SquashState::PREPARING;
    progressInfo_.statusMessage = "Preparing to capture audio...";
    notifyProgressUpdate();
    
    if (tapeSquashCallback_) {
        tapeSquashCallback_(currentSelection_, settings_);
    }
}

void TapeSquashingUI::onConfirmDialogNo() {
    confirmDialogVisible_ = false;
}

void TapeSquashingUI::onSlotSelectionChanged(uint8_t slot) {
    settings_.targetSlot = slot;
}

void TapeSquashingUI::onQualitySettingChanged(float sampleRate, uint8_t bitDepth) {
    settings_.sampleRate = sampleRate;
    settings_.bitDepth = bitDepth;
}

void TapeSquashingUI::onAutoNormalizeToggled(bool enabled) {
    settings_.enableAutoNormalize = enabled;
}

void TapeSquashingUI::onAutoFadeToggled(bool enabled) {
    settings_.enableAutoFade = enabled;
}

void TapeSquashingUI::onAutoNameToggled(bool enabled) {
    settings_.enableAutoName = enabled;
}

void TapeSquashingUI::onNamePrefixChanged(const std::string& prefix) {
    settings_.namePrefix = prefix.substr(0, 16);  // Limit length
}

// Visual Components (mock implementations for compilation)
void TapeSquashingUI::drawMainPanel(void* graphics) {
    if (!visible_) return;
    
    // Draw main panel background
    drawPanel(graphics, 50, 50, MAIN_PANEL_WIDTH, MAIN_PANEL_HEIGHT, 
              COLOR_PANEL_BG, COLOR_PANEL_BORDER);
    
    // Draw selection overview
    drawSelectionOverview(graphics);
    
    // Draw crush button
    drawCrushButton(graphics);
    
    // Draw progress bar if active
    if (isSquashingActive()) {
        drawProgressBar(graphics);
    }
    
    // Draw settings panel if visible
    if (settingsPanelVisible_) {
        drawSettingsPanel(graphics);
    }
    
    // Draw confirmation dialog if visible
    if (confirmDialogVisible_) {
        drawConfirmationDialog(graphics);
    }
}

void TapeSquashingUI::drawCrushButton(void* graphics) {
    uint16_t buttonX = 50 + (MAIN_PANEL_WIDTH - CRUSH_BUTTON_WIDTH) / 2;
    uint16_t buttonY = 250;
    
    bool enabled = canStartSquashing();
    uint32_t buttonColor = enabled ? COLOR_CRUSH_BUTTON_ENABLED : COLOR_CRUSH_BUTTON_DISABLED;
    
    std::string buttonText;
    if (isSquashingActive()) {
        buttonText = "CRUSHING...";
        buttonColor = COLOR_CRUSH_BUTTON_DISABLED;
    } else {
        buttonText = "CRUSH TO TAPE";
    }
    
    drawButton(graphics, buttonX, buttonY, CRUSH_BUTTON_WIDTH, CRUSH_BUTTON_HEIGHT,
               buttonText, buttonColor, enabled);
}

void TapeSquashingUI::drawSelectionOverview(void* graphics) {
    uint16_t textX = 70;
    uint16_t textY = 80;
    uint16_t lineHeight = 20;
    
    if (hasValidSelection()) {
        char buffer[64];
        
        snprintf(buffer, sizeof(buffer), "Selection: %d tracks × %d steps", 
                selectionOverview_.trackCount, selectionOverview_.stepCount);
        drawText(graphics, textX, textY, buffer, COLOR_TEXT_NORMAL, 12);
        textY += lineHeight;
        
        snprintf(buffer, sizeof(buffer), "Duration: %.1f seconds", 
                selectionOverview_.estimatedDurationMs / 1000.0f);
        drawText(graphics, textX, textY, buffer, COLOR_TEXT_NORMAL, 12);
        textY += lineHeight;
        
        const char* audioStatus = selectionOverview_.hasAudio ? "Contains audio" : "No audio detected";
        uint32_t audioColor = selectionOverview_.hasAudio ? COLOR_TEXT_NORMAL : COLOR_TEXT_ERROR;
        drawText(graphics, textX, textY, audioStatus, audioColor, 12);
    } else {
        drawText(graphics, textX, textY, "No valid selection", COLOR_TEXT_ERROR, 12);
    }
}

void TapeSquashingUI::drawProgressBar(void* graphics) {
    uint16_t barX = 70;
    uint16_t barY = 200;
    uint16_t barWidth = MAIN_PANEL_WIDTH - 40;
    
    drawProgressBar(graphics, barX, barY, barWidth, PROGRESS_BAR_HEIGHT,
                   progressInfo_.progressPercent, COLOR_PROGRESS_FILL, COLOR_PROGRESS_BG);
    
    // Draw status text
    drawText(graphics, barX, barY + PROGRESS_BAR_HEIGHT + 5, 
             progressInfo_.statusMessage, COLOR_TEXT_NORMAL, 10);
}

void TapeSquashingUI::drawSettingsPanel(void* graphics) {
    uint16_t panelX = 470;
    uint16_t panelY = 50;
    uint16_t panelWidth = 300;
    uint16_t panelHeight = 400;
    
    drawPanel(graphics, panelX, panelY, panelWidth, panelHeight,
              COLOR_PANEL_BG, COLOR_PANEL_BORDER);
    
    drawText(graphics, panelX + 10, panelY + 10, "Tape Squashing Settings", COLOR_TEXT_NORMAL, 14);
    
    // Quality settings
    char qualityText[64];
    snprintf(qualityText, sizeof(qualityText), "Quality: %.0fHz %d-bit", 
             settings_.sampleRate, settings_.bitDepth);
    drawText(graphics, panelX + 10, panelY + 40, qualityText, COLOR_TEXT_NORMAL, 12);
    
    // Auto-features
    uint16_t y = panelY + 70;
    drawText(graphics, panelX + 10, y, settings_.enableAutoNormalize ? "☑ Auto Normalize" : "☐ Auto Normalize", COLOR_TEXT_NORMAL, 12);
    y += 25;
    drawText(graphics, panelX + 10, y, settings_.enableAutoFade ? "☑ Auto Fade" : "☐ Auto Fade", COLOR_TEXT_NORMAL, 12);
    y += 25;
    drawText(graphics, panelX + 10, y, settings_.enableAutoName ? "☑ Auto Name" : "☐ Auto Name", COLOR_TEXT_NORMAL, 12);
    
    // Name prefix
    if (settings_.enableAutoName) {
        y += 30;
        std::string prefixText = "Prefix: " + settings_.namePrefix;
        drawText(graphics, panelX + 10, y, prefixText, COLOR_TEXT_NORMAL, 12);
    }
    
    // Target slot
    y += 30;
    std::string slotText = (settings_.targetSlot == 255) ? "Slot: Auto" : 
                          ("Slot: " + std::to_string(settings_.targetSlot + 1));
    drawText(graphics, panelX + 10, y, slotText, COLOR_TEXT_NORMAL, 12);
}

void TapeSquashingUI::drawConfirmationDialog(void* graphics) {
    uint16_t dialogX = 150;
    uint16_t dialogY = 150;
    uint16_t dialogWidth = 300;
    uint16_t dialogHeight = 150;
    
    drawPanel(graphics, dialogX, dialogY, dialogWidth, dialogHeight,
              COLOR_PANEL_BG, COLOR_PANEL_BORDER);
    
    drawText(graphics, dialogX + 10, dialogY + 20, "Confirm Tape Squashing", COLOR_TEXT_NORMAL, 14);
    drawText(graphics, dialogX + 10, dialogY + 50, "This will replace the selected", COLOR_TEXT_NORMAL, 12);
    drawText(graphics, dialogX + 10, dialogY + 70, "pattern data with a single sample.", COLOR_TEXT_NORMAL, 12);
    drawText(graphics, dialogX + 10, dialogY + 90, "This operation cannot be undone.", COLOR_TEXT_ERROR, 12);
    
    // Yes/No buttons
    drawButton(graphics, dialogX + 50, dialogY + 110, 80, 30, "YES", COLOR_CRUSH_BUTTON_ENABLED, true);
    drawButton(graphics, dialogX + 170, dialogY + 110, 80, 30, "NO", COLOR_CRUSH_BUTTON_DISABLED, true);
}

// Callbacks
void TapeSquashingUI::setTapeSquashCallback(TapeSquashCallback callback) {
    tapeSquashCallback_ = callback;
}

void TapeSquashingUI::setProgressUpdateCallback(ProgressUpdateCallback callback) {
    progressUpdateCallback_ = callback;
}

void TapeSquashingUI::setCompletionCallback(CompletionCallback callback) {
    completionCallback_ = callback;
}

void TapeSquashingUI::setErrorCallback(ErrorCallback callback) {
    errorCallback_ = callback;
}

// Error Handling
void TapeSquashingUI::handleError(const std::string& errorMessage) {
    lastError_ = errorMessage;
    progressInfo_.currentState = SquashState::ERROR;
    progressInfo_.statusMessage = "Error: " + errorMessage;
    showErrorDialog(errorMessage);
    notifyError(errorMessage);
}

void TapeSquashingUI::showErrorDialog(const std::string& error) {
    errorDialogVisible_ = true;
}

void TapeSquashingUI::hideErrorDialog() {
    errorDialogVisible_ = false;
}

// Helper methods
void TapeSquashingUI::updateSelectionOverview() {
    if (!currentSelection_.isValid()) {
        selectionOverview_ = SelectionOverview();
        return;
    }
    
    selectionOverview_.trackCount = currentSelection_.getTrackCount();
    selectionOverview_.stepCount = currentSelection_.getStepCount();
    selectionOverview_.totalCells = currentSelection_.getTotalCells();
    
    calculateEstimatedDuration();
    
    // Mock audio detection - in real implementation would check pattern data
    selectionOverview_.hasAudio = (selectionOverview_.totalCells > 0);
    
    // Mock track names - in real implementation would get actual track names
    selectionOverview_.affectedTracks.clear();
    for (uint16_t i = currentSelection_.startTrack; i <= currentSelection_.endTrack; ++i) {
        selectionOverview_.affectedTracks.push_back("Track " + std::to_string(i + 1));
    }
}

void TapeSquashingUI::calculateEstimatedDuration() {
    // Simple estimation: assume 120 BPM, 16 steps per bar
    float bpm = 120.0f;
    float stepsPerSecond = (bpm / 60.0f) * (16.0f / 4.0f);  // 16th notes at 120 BPM
    selectionOverview_.estimatedDurationMs = (selectionOverview_.stepCount / stepsPerSecond) * 1000.0f;
}

bool TapeSquashingUI::validateSelection() const {
    return currentSelection_.isValid() && currentSelection_.getTotalCells() >= 1;
}

bool TapeSquashingUI::validateSettings() const {
    return settings_.sampleRate >= 8000.0f && 
           settings_.sampleRate <= 192000.0f &&
           (settings_.bitDepth == 16 || settings_.bitDepth == 24 || settings_.bitDepth == 32) &&
           settings_.maxDurationMs >= 1000 &&
           settings_.maxDurationMs <= 60000;
}


std::string TapeSquashingUI::generateSampleName() const {
    if (!settings_.enableAutoName) {
        return "Sample";
    }
    
    // Generate name like "Crush_T2-5_S4-7"
    std::ostringstream oss;
    oss << settings_.namePrefix 
        << "_T" << (currentSelection_.startTrack + 1) << "-" << (currentSelection_.endTrack + 1)
        << "_S" << (currentSelection_.startStep + 1) << "-" << (currentSelection_.endStep + 1);
    
    return oss.str();
}

uint8_t TapeSquashingUI::findNextAvailableSlot() const {
    // Mock implementation - in real system would check sampler slots
    return 0;  // Return first slot for now
}

void TapeSquashingUI::resetProgress() {
    progressInfo_.progressPercent = 0.0f;
    progressInfo_.elapsedTimeMs = 0;
    progressInfo_.estimatedTotalMs = 0;
}

void TapeSquashingUI::notifyProgressUpdate() {
    if (progressUpdateCallback_) {
        progressUpdateCallback_(progressInfo_);
    }
}

void TapeSquashingUI::notifyCompletion(bool success, const std::string& sampleName, uint8_t slot) {
    if (completionCallback_) {
        completionCallback_(success, sampleName, slot);
    }
}

void TapeSquashingUI::notifyError(const std::string& error) {
    if (errorCallback_) {
        errorCallback_(error);
    }
}

// UI Drawing Helpers (mock implementations for compilation)
void TapeSquashingUI::drawButton(void* graphics, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                                const std::string& text, uint32_t color, bool enabled) {
    // In real implementation, would use TouchGFX to draw button with text
}

void TapeSquashingUI::drawProgressBar(void* graphics, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                                     float percent, uint32_t fillColor, uint32_t bgColor) {
    // In real implementation, would use TouchGFX to draw progress bar
}

void TapeSquashingUI::drawText(void* graphics, uint16_t x, uint16_t y, const std::string& text, 
                               uint32_t color, uint8_t size) {
    // In real implementation, would use TouchGFX to draw text
}

void TapeSquashingUI::drawPanel(void* graphics, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                                uint32_t bgColor, uint32_t borderColor) {
    // In real implementation, would use TouchGFX to draw panel with background and border
}