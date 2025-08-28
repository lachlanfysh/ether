#include "CrushConfirmationDialog.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <chrono>

CrushConfirmationDialog::CrushConfirmationDialog() {
    isOpen_ = false;
    dialogOpenTime_ = 0;
    dialogTimeout_ = DEFAULT_DIALOG_TIMEOUT_MS;
    hasTimeout_ = false;
    pendingResult_ = DialogResult::CANCELLED;
    
    patternReplacer_ = nullptr;
    sampleLoader_ = nullptr;
    sequencer_ = nullptr;
    
    autoSaveOptions_ = AutoSaveOptions();
    currentConfig_ = DialogConfig();
    touchElements_ = TouchGFXElements();
}

// Dialog Management
CrushConfirmationDialog::DialogResult CrushConfirmationDialog::showConfirmationDialog(const DialogConfig& config) {
    if (isOpen_) {
        return DialogResult::ERROR;  // Dialog already open
    }
    
    // Validate configuration
    std::string validationError;
    if (!validateDialogConfig(config, validationError)) {
        return DialogResult::ERROR;
    }
    
    currentConfig_ = config;
    isOpen_ = true;
    dialogOpenTime_ = getCurrentTimeMs();
    pendingResult_ = DialogResult::CANCELLED;
    
    // Setup dialog content
    setupDialogContent(config);
    
    // Create TouchGFX elements
    createTouchGFXElements();
    
    // Start opening animation
    startOpenAnimation();
    
    // Update display
    updateDialogDisplay();
    
    // Set timeout if configured
    if (hasTimeout_ && dialogTimeout_ > 0) {
        // In real implementation, would set up timer for handleDialogTimeout()
    }
    
    return DialogResult::CONFIRMED;  // Mock - in real implementation would wait for user input
}

void CrushConfirmationDialog::closeDialog() {
    if (!isOpen_) return;
    
    // Start closing animation
    startCloseAnimation();
    
    // Cleanup TouchGFX elements
    destroyTouchGFXElements();
    
    isOpen_ = false;
    
    // Notify completion if callback is set
    if (confirmationCallback_) {
        confirmationCallback_(pendingResult_, currentConfig_);
    }
}

void CrushConfirmationDialog::setDialogTimeout(uint32_t timeoutMs) {
    dialogTimeout_ = timeoutMs;
    hasTimeout_ = (timeoutMs > 0);
}

// Configuration
void CrushConfirmationDialog::setAutoSaveOptions(const AutoSaveOptions& options) {
    autoSaveOptions_ = options;
    
    // Validate save location if specified
    if (!options.saveLocation.empty() && !isValidSavePath(options.saveLocation)) {
        // Reset to default if invalid
        autoSaveOptions_.saveLocation = "";
    }
}

void CrushConfirmationDialog::setDialogDefaults(const DialogConfig& defaults) {
    // Store default values for future dialogs
    // In real implementation would maintain default state
}

// Dialog Content Generation
CrushConfirmationDialog::DialogInfo CrushConfirmationDialog::generateDialogInfo(const DialogConfig& config) const {
    DialogInfo info;
    
    info.title = "Confirm Tape Crush Operation";
    info.mainMessage = generateMainMessage(config);
    info.detailMessage = generateDetailMessage(config);
    info.warningMessage = generateWarningMessage(config);
    
    // Generate affected items list
    info.affectedItems.clear();
    info.affectedItems.push_back("Tracks: " + std::to_string(config.affectedTracks));
    info.affectedItems.push_back("Steps: " + std::to_string(config.affectedSteps));
    info.affectedItems.push_back("Sample slot: " + std::to_string(config.destinationSlot + 1));
    
    if (config.willOverwriteExistingSample) {
        info.affectedItems.push_back("Existing sample: " + config.existingSampleName);
    }
    
    info.showProgressEstimate = config.estimatedCrushTimeSeconds > 0.1f;
    info.requiresUserConfirmation = config.hasComplexPatternData || 
                                   config.willOverwriteExistingSample;
    
    return info;
}

std::string CrushConfirmationDialog::generateMainMessage(const DialogConfig& config) const {
    std::ostringstream oss;
    
    oss << "This will crush the selected pattern region to audio and replace it with ";
    oss << "a single sample trigger.";
    
    if (config.willOverwriteExistingSample) {
        oss << " This will overwrite the existing sample \"" << config.existingSampleName << "\".";
    }
    
    return oss.str();
}

std::string CrushConfirmationDialog::generateDetailMessage(const DialogConfig& config) const {
    std::ostringstream oss;
    
    oss << "Selected region: ";
    oss << formatAffectedItems(config.affectedSteps, config.affectedTracks);
    oss << "\\n";
    
    oss << "Sample destination: ";
    oss << formatSlotInformation(config.destinationSlot, config.willOverwriteExistingSample, 
                                config.existingSampleName);
    oss << "\\n";
    
    if (config.estimatedCrushTimeSeconds > 0.1f) {
        oss << "Estimated processing time: ";
        oss << formatTimeEstimate(config.estimatedCrushTimeSeconds);
        oss << "\\n";
    }
    
    if (autoSaveOptions_.enableAutoSave) {
        oss << "Pattern will be automatically saved before crushing.";
    }
    
    return oss.str();
}

std::string CrushConfirmationDialog::generateWarningMessage(const DialogConfig& config) const {
    std::ostringstream oss;
    
    if (config.hasComplexPatternData) {
        oss << "⚠️ This region contains complex pattern data that cannot be undone. ";
    }
    
    if (config.willOverwriteExistingSample) {
        oss << "⚠️ Existing sample will be permanently overwritten. ";
    }
    
    if (!autoSaveOptions_.enableAutoSave) {
        oss << "⚠️ Auto-save is disabled - consider saving manually first. ";
    }
    
    return oss.str();
}

// Auto-Save Operations
bool CrushConfirmationDialog::performAutoSave(const DialogConfig& config) {
    if (!autoSaveOptions_.enableAutoSave) {
        return true;  // No auto-save requested
    }
    
    bool success = true;
    
    // Save current pattern if requested
    if (autoSaveOptions_.saveCurrentPattern) {
        std::string savePath = generateSavePath(autoSaveOptions_);
        success &= saveCurrentPattern(savePath);
        
        // Create backup copy if requested
        if (success && autoSaveOptions_.createBackupCopy) {
            success &= createBackupCopy(savePath, autoSaveOptions_.backupPrefix);
        }
    }
    
    // Save selected region only if requested instead
    if (!autoSaveOptions_.saveCurrentPattern && autoSaveOptions_.saveAffectedRegionOnly) {
        std::string savePath = generateSavePath(autoSaveOptions_);
        success &= saveSelectedRegion(config.selection, savePath);
    }
    
    // Notify via callback if set
    if (autoSaveCallback_) {
        std::string savePath = generateSavePath(autoSaveOptions_);
        success &= autoSaveCallback_(savePath);
    }
    
    return success;
}

bool CrushConfirmationDialog::saveCurrentPattern(const std::string& location) {
    // Mock implementation - in real system would save actual pattern
    std::string savePath = location.empty() ? generateSavePath(autoSaveOptions_) : location;
    
    // Ensure directory exists
    if (!ensureSaveDirectoryExists(savePath)) {
        return false;
    }
    
    // Check disk space
    if (!checkDiskSpace(savePath, MIN_FREE_DISK_SPACE)) {
        return false;
    }
    
    // In real implementation, would serialize pattern data and save to file
    return true;
}

bool CrushConfirmationDialog::saveSelectedRegion(const PatternSelection::SelectionBounds& selection, 
                                               const std::string& location) {
    // Mock implementation - in real system would save selected region only
    std::string savePath = location.empty() ? generateSavePath(autoSaveOptions_) : location;
    
    // Ensure directory exists
    if (!ensureSaveDirectoryExists(savePath)) {
        return false;
    }
    
    // Check available space
    size_t estimatedSize = selection.getTotalCells() * 16;  // Estimate 16 bytes per cell
    if (!checkDiskSpace(savePath, estimatedSize)) {
        return false;
    }
    
    // In real implementation, would extract and save selected region data
    return true;
}

bool CrushConfirmationDialog::createBackupCopy(const std::string& originalPath, const std::string& backupPrefix) {
    std::string backupPath = generateBackupPath(originalPath, backupPrefix);
    
    // In real implementation, would copy original file to backup location
    return true;
}

// User Interaction
void CrushConfirmationDialog::setConfirmButtonEnabled(bool enabled) {
    // In real implementation, would enable/disable TouchGFX confirm button
}

void CrushConfirmationDialog::setCancelButtonEnabled(bool enabled) {
    // In real implementation, would enable/disable TouchGFX cancel button
}

void CrushConfirmationDialog::setAutoSaveCheckboxState(bool enabled) {
    autoSaveOptions_.enableAutoSave = enabled;
    // In real implementation, would update TouchGFX checkbox state
}

void CrushConfirmationDialog::updateProgressEstimate(float timeSeconds) {
    currentConfig_.estimatedCrushTimeSeconds = timeSeconds;
    
    // Update progress display if dialog is open
    if (isOpen_) {
        updateDialogDisplay();
    }
}

// Integration
void CrushConfirmationDialog::integrateWithPatternDataReplacer(PatternDataReplacer* patternReplacer) {
    patternReplacer_ = patternReplacer;
}

void CrushConfirmationDialog::integrateWithAutoSampleLoader(AutoSampleLoader* sampleLoader) {
    sampleLoader_ = sampleLoader;
}

void CrushConfirmationDialog::integrateWithSequencer(SequencerEngine* sequencer) {
    sequencer_ = sequencer;
}

// Callbacks
void CrushConfirmationDialog::setConfirmationCallback(ConfirmationCallback callback) {
    confirmationCallback_ = callback;
}

void CrushConfirmationDialog::setAutoSaveCallback(AutoSaveCallback callback) {
    autoSaveCallback_ = callback;
}

void CrushConfirmationDialog::setValidationCallback(ValidationCallback callback) {
    validationCallback_ = callback;
}

// TouchGFX Integration
void CrushConfirmationDialog::setupTouchGFXElements() {
    if (!isOpen_) return;
    
    // In real implementation, would configure TouchGFX elements
    layoutDialogElements();
    applyDialogStyling();
}

void CrushConfirmationDialog::handleTouchEvent(int16_t x, int16_t y, bool isPressed) {
    if (!isOpen_ || !isPressed) return;
    
    // Mock touch handling - in real implementation would check touch coordinates
    // against button bounds and trigger appropriate actions
    
    // Example button areas (mock coordinates)
    if (x >= 250 && x <= 350 && y >= 220 && y <= 250) {  // Confirm button
        handleConfirmButton();
    } else if (x >= 150 && x <= 220 && y >= 220 && y <= 250) {  // Cancel button
        handleCancelButton();
    } else if (x >= 50 && x <= 200 && y >= 180 && y <= 200) {  // Auto-save checkbox
        handleAutoSaveToggle();
    }
}

void CrushConfirmationDialog::updateTouchGFXDisplay() {
    if (!isOpen_) return;
    
    // Update dialog content based on current configuration
    DialogInfo info = generateDialogInfo(currentConfig_);
    
    // In real implementation, would update TouchGFX text elements
    // touchElements_.titleText->setText(info.title);
    // touchElements_.messageText->setText(info.mainMessage);
    // etc.
}

void CrushConfirmationDialog::animateDialogTransition(bool opening) {
    // Mock animation - in real implementation would animate TouchGFX elements
    float animationProgress = opening ? 1.0f : 0.0f;
    updateAnimationFrame(animationProgress);
}

// Keyboard/Button Support
void CrushConfirmationDialog::handleKeyPress(uint8_t keyCode) {
    if (!isOpen_) return;
    
    switch (keyCode) {
        case 13:  // Enter key
            handleConfirmButton();
            break;
        case 27:  // Escape key
            handleCancelButton();
            break;
        case 32:  // Spacebar - toggle auto-save
            handleAutoSaveToggle();
            break;
        default:
            // Ignore other keys
            break;
    }
}

void CrushConfirmationDialog::handleConfirmButton() {
    if (!isOpen_) return;
    
    // Perform auto-save if enabled
    if (autoSaveOptions_.enableAutoSave) {
        if (!performAutoSave(currentConfig_)) {
            // Auto-save failed - show error or continue based on user preference
            return;
        }
    }
    
    // Validate one final time
    std::string validationError;
    if (validationCallback_ && !validationCallback_(currentConfig_, validationError)) {
        // Validation failed - show error message
        return;
    }
    
    pendingResult_ = autoSaveOptions_.enableAutoSave ? 
                    DialogResult::SAVE_AND_CONFIRM : DialogResult::CONFIRMED;
    closeDialog();
}

void CrushConfirmationDialog::handleCancelButton() {
    if (!isOpen_) return;
    
    pendingResult_ = DialogResult::CANCELLED;
    closeDialog();
}

void CrushConfirmationDialog::handleAutoSaveToggle() {
    if (!isOpen_) return;
    
    autoSaveOptions_.enableAutoSave = !autoSaveOptions_.enableAutoSave;
    updateDialogDisplay();
}

// Accessibility
void CrushConfirmationDialog::setAccessibilityEnabled(bool enabled) {
    // In real implementation, would configure accessibility features
}

std::string CrushConfirmationDialog::generateAccessibilityText() const {
    if (!isOpen_) return "";
    
    DialogInfo info = generateDialogInfo(currentConfig_);
    std::ostringstream oss;
    
    oss << info.title << ". ";
    oss << info.mainMessage << " ";
    oss << info.detailMessage << " ";
    
    if (!info.warningMessage.empty()) {
        oss << "Warning: " << info.warningMessage << " ";
    }
    
    return oss.str();
}

void CrushConfirmationDialog::announceDialogContent() {
    // In real implementation, would use text-to-speech or screen reader API
    std::string accessibilityText = generateAccessibilityText();
}

// Internal operations
bool CrushConfirmationDialog::validateDialogConfig(const DialogConfig& config, std::string& errorMessage) {
    if (!config.selection.isValid()) {
        errorMessage = "Invalid selection bounds";
        return false;
    }
    
    if (config.destinationSlot >= 16) {  // Assuming 16 max slots
        errorMessage = "Invalid destination slot";
        return false;
    }
    
    if (config.affectedSteps == 0 || config.affectedTracks == 0) {
        errorMessage = "Selection must affect at least one step and track";
        return false;
    }
    
    // Use validation callback if available
    if (validationCallback_) {
        return validationCallback_(config, errorMessage);
    }
    
    return true;
}

void CrushConfirmationDialog::setupDialogContent(const DialogConfig& config) {
    // Generate dialog information
    DialogInfo info = generateDialogInfo(config);
    
    // In real implementation, would prepare TouchGFX elements with content
}

void CrushConfirmationDialog::updateDialogDisplay() {
    if (!isOpen_) return;
    
    updateTouchGFXDisplay();
}

void CrushConfirmationDialog::handleDialogTimeout() {
    if (!isOpen_ || !hasTimeout_) return;
    
    uint32_t elapsed = getCurrentTimeMs() - dialogOpenTime_;
    if (elapsed >= dialogTimeout_) {
        pendingResult_ = DialogResult::CANCELLED;
        closeDialog();
    }
}

// Content generation helpers
std::string CrushConfirmationDialog::formatTimeEstimate(float seconds) const {
    if (seconds < 1.0f) {
        return "< 1 second";
    } else if (seconds < 60.0f) {
        return std::to_string(static_cast<int>(seconds + 0.5f)) + " seconds";
    } else {
        int minutes = static_cast<int>(seconds / 60.0f);
        int remainingSeconds = static_cast<int>(seconds) % 60;
        return std::to_string(minutes) + "m " + std::to_string(remainingSeconds) + "s";
    }
}

std::string CrushConfirmationDialog::formatAffectedItems(uint32_t steps, uint32_t tracks) const {
    std::ostringstream oss;
    oss << steps << " step" << (steps != 1 ? "s" : "") << " × ";
    oss << tracks << " track" << (tracks != 1 ? "s" : "");
    return oss.str();
}

std::string CrushConfirmationDialog::formatSlotInformation(uint8_t slot, bool willOverwrite, 
                                                         const std::string& existingName) const {
    std::ostringstream oss;
    oss << "Slot " << (slot + 1);
    
    if (willOverwrite) {
        oss << " (will overwrite \"" << existingName << "\")";
    } else {
        oss << " (empty)";
    }
    
    return oss.str();
}

// Auto-save helpers
std::string CrushConfirmationDialog::generateSavePath(const AutoSaveOptions& options) const {
    if (!options.saveLocation.empty()) {
        return options.saveLocation;
    }
    
    // Generate default save path
    std::ostringstream oss;
    oss << "patterns/autosave_" << getCurrentTimestamp() << ".pattern";
    return oss.str();
}

std::string CrushConfirmationDialog::generateBackupPath(const std::string& originalPath, 
                                                       const std::string& prefix) const {
    std::ostringstream oss;
    oss << prefix << getCurrentTimestamp() << "_" << originalPath;
    return oss.str();
}

bool CrushConfirmationDialog::ensureSaveDirectoryExists(const std::string& path) {
    // Mock implementation - in real system would ensure directory structure exists
    return true;
}

// TouchGFX helpers
void CrushConfirmationDialog::createTouchGFXElements() {
    // Mock implementation - in real system would create TouchGFX UI elements
    touchElements_ = TouchGFXElements();
}

void CrushConfirmationDialog::destroyTouchGFXElements() {
    // Mock implementation - in real system would cleanup TouchGFX elements
    touchElements_ = TouchGFXElements();
}

void CrushConfirmationDialog::layoutDialogElements() {
    // Mock implementation - in real system would position TouchGFX elements
}

void CrushConfirmationDialog::applyDialogStyling() {
    // Mock implementation - in real system would apply visual styling
}

// Animation helpers
void CrushConfirmationDialog::startOpenAnimation() {
    // Mock implementation - in real system would start opening animation
}

void CrushConfirmationDialog::startCloseAnimation() {
    // Mock implementation - in real system would start closing animation
}

void CrushConfirmationDialog::updateAnimationFrame(float progress) {
    // Mock implementation - in real system would update animation frame
}

// Validation helpers
bool CrushConfirmationDialog::isValidSavePath(const std::string& path) const {
    return !path.empty() && path.find("..") == std::string::npos;
}

bool CrushConfirmationDialog::hasWritePermission(const std::string& path) const {
    // Mock implementation - in real system would check filesystem permissions
    return true;
}

bool CrushConfirmationDialog::checkDiskSpace(const std::string& path, size_t requiredBytes) const {
    // Mock implementation - in real system would check available disk space
    return true;
}

// Utility methods
uint32_t CrushConfirmationDialog::getCurrentTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

std::string CrushConfirmationDialog::getCurrentTimestamp() const {
    uint32_t timeMs = getCurrentTimeMs();
    std::ostringstream oss;
    oss << timeMs;
    return oss.str();
}