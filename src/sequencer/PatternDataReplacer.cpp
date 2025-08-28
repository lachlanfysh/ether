#include "PatternDataReplacer.h"
#include <algorithm>
#include <sstream>
#include <cstring>
#include <chrono>

PatternDataReplacer::PatternDataReplacer() {
    defaultConfig_ = ReplacementConfig();
    maxBackupCount_ = DEFAULT_MAX_BACKUPS;
    maxBackupMemory_ = DEFAULT_MAX_BACKUP_MEMORY;
    maxUndoDepth_ = DEFAULT_MAX_UNDO_DEPTH;
    
    sequencer_ = nullptr;
    sampler_ = nullptr;
    tapeSquashing_ = nullptr;
}

// Main replacement operations
PatternDataReplacer::ReplacementResult PatternDataReplacer::replacePatternData(const PatternSelection::SelectionBounds& selection,
                                                                               const ReplacementConfig& config) {
    ReplacementResult result;
    result.affectedRegion = selection;
    
    // Validate selection bounds
    if (!validateSelectionBounds(selection)) {
        result.success = false;
        result.errorMessage = "Invalid selection bounds";
        notifyReplacementComplete(result);
        return result;
    }
    
    // Validate pattern before replacement
    if (config.validateAfterReplace) {
        auto validationResult = validatePattern(selection);
        if (!validationResult.isValid) {
            result.success = false;
            result.errorMessage = "Pattern validation failed before replacement";
            notifyValidationError(validationResult);
            notifyReplacementComplete(result);
            return result;
        }
    }
    
    // Create backup if requested
    std::string backupId;
    if (config.createBackup) {
        backupId = createPatternBackup(selection, "Pattern replacement");
        if (backupId.empty()) {
            result.success = false;
            result.errorMessage = "Failed to create backup";
            notifyReplacementComplete(result);
            return result;
        }
        result.backupId = backupId;
    }
    
    // Perform the actual replacement
    bool success = performReplacement(selection, config, result);
    
    if (success) {
        // Add to undo stack
        if (!backupId.empty()) {
            addToUndoStack(backupId);
        }
        
        // Notify pattern modification
        notifyPatternModified(selection);
        
        // Validate after replacement if requested
        if (config.validateAfterReplace) {
            auto postValidation = validatePattern(result.affectedRegion);
            if (!postValidation.isValid) {
                // Attempt to restore from backup
                if (!backupId.empty()) {
                    restoreFromBackup(backupId);
                }
                result.success = false;
                result.errorMessage = "Pattern validation failed after replacement";
                notifyValidationError(postValidation);
            }
        }
    } else {
        // Restore from backup on failure
        if (!backupId.empty()) {
            restoreFromBackup(backupId);
            removeBackup(backupId);  // Remove failed backup
        }
    }
    
    notifyReplacementComplete(result);
    return result;
}

PatternDataReplacer::ReplacementResult PatternDataReplacer::replaceWithSample(const PatternSelection::SelectionBounds& selection,
                                                                             uint8_t sampleSlot, uint8_t targetTrack) {
    ReplacementConfig config = defaultConfig_;
    config.type = ReplacementType::SAMPLE_ONLY;
    config.sampleSlot = sampleSlot;
    config.targetTrack = targetTrack;
    
    return replacePatternData(selection, config);
}

PatternDataReplacer::ReplacementResult PatternDataReplacer::clearAndReplacewithSample(const PatternSelection::SelectionBounds& selection,
                                                                                      uint8_t sampleSlot, uint8_t targetTrack) {
    ReplacementConfig config = defaultConfig_;
    config.type = ReplacementType::CLEAR_AND_SAMPLE;
    config.sampleSlot = sampleSlot;
    config.targetTrack = targetTrack;
    
    return replacePatternData(selection, config);
}

// Backup and restore operations
std::string PatternDataReplacer::createPatternBackup(const PatternSelection::SelectionBounds& selection,
                                                     const std::string& operationDescription) {
    // Extract pattern data from selection
    std::vector<uint8_t> patternData;
    if (!extractPatternData(selection, patternData)) {
        return "";  // Failed to extract data
    }
    
    // Create backup entry
    PatternBackup backup;
    backup.backupId = generateBackupId();
    backup.originalBounds = selection;
    backup.backupTime = getCurrentTimeMs();
    backup.operation = operationDescription;
    backup.uncompressedSize = patternData.size();
    
    // Compress data for storage
    if (!compressPatternData(patternData, backup.compressedData)) {
        return "";  // Failed to compress
    }
    
    // Add to backup storage
    patternBackups_.push_back(backup);
    
    // Prune old backups if necessary
    pruneOldBackups();
    
    notifyBackupCreated(backup.backupId);
    return backup.backupId;
}

bool PatternDataReplacer::restoreFromBackup(const std::string& backupId) {
    // Find backup
    auto it = std::find_if(patternBackups_.begin(), patternBackups_.end(),
                          [&backupId](const PatternBackup& backup) {
                              return backup.backupId == backupId;
                          });
    
    if (it == patternBackups_.end()) {
        return false;  // Backup not found
    }
    
    // Decompress pattern data
    std::vector<uint8_t> patternData;
    if (!decompressPatternData(it->compressedData, patternData)) {
        return false;  // Failed to decompress
    }
    
    // Restore pattern data
    bool success = insertPatternData(it->originalBounds, patternData);
    
    if (success) {
        notifyPatternModified(it->originalBounds);
    }
    
    return success;
}

bool PatternDataReplacer::removeBackup(const std::string& backupId) {
    auto it = std::find_if(patternBackups_.begin(), patternBackups_.end(),
                          [&backupId](const PatternBackup& backup) {
                              return backup.backupId == backupId;
                          });
    
    if (it != patternBackups_.end()) {
        patternBackups_.erase(it);
        return true;
    }
    
    return false;
}

void PatternDataReplacer::clearAllBackups() {
    patternBackups_.clear();
    undoStack_.clear();
    redoStack_.clear();
}

// Backup management
std::vector<PatternDataReplacer::PatternBackup> PatternDataReplacer::getAvailableBackups() const {
    return patternBackups_;
}

bool PatternDataReplacer::hasBackup(const std::string& backupId) const {
    return std::find_if(patternBackups_.begin(), patternBackups_.end(),
                       [&backupId](const PatternBackup& backup) {
                           return backup.backupId == backupId;
                       }) != patternBackups_.end();
}

size_t PatternDataReplacer::getTotalBackupMemoryUsage() const {
    size_t total = 0;
    for (const auto& backup : patternBackups_) {
        total += backup.compressedData.size();
    }
    return total;
}

void PatternDataReplacer::setMaxBackupCount(size_t maxCount) {
    maxBackupCount_ = maxCount;
    pruneOldBackups();
}

void PatternDataReplacer::setMaxBackupMemory(size_t maxMemoryBytes) {
    maxBackupMemory_ = maxMemoryBytes;
    pruneOldBackups();
}

// Pattern validation
PatternDataReplacer::ValidationResult PatternDataReplacer::validatePattern(const PatternSelection::SelectionBounds& selection) const {
    ValidationResult result;
    result.isValid = true;
    
    // Validate selection bounds
    if (!validateSelectionBounds(selection)) {
        addValidationError(result, "Invalid selection bounds");
        result.isValid = false;
    }
    
    // Check pattern dimensions
    result.totalTracks = selection.getTrackCount();
    result.totalSteps = selection.getStepCount();
    
    if (result.totalTracks > MAX_TRACKS) {
        addValidationError(result, "Too many tracks selected");
        result.isValid = false;
    }
    
    if (result.totalSteps > MAX_PATTERN_LENGTH) {
        addValidationWarning(result, "Pattern length exceeds recommended maximum");
        result.hasLongPattern = true;
    }
    
    // Check for empty tracks (warning only)
    // In real implementation, would check actual pattern data
    if (result.totalTracks > 8) {
        addValidationWarning(result, "Large selection may contain empty tracks");
        result.hasEmptyTracks = true;
    }
    
    return result;
}

PatternDataReplacer::ValidationResult PatternDataReplacer::validateEntirePattern() const {
    PatternSelection::SelectionBounds fullPattern(0, MAX_TRACKS - 1, 0, MAX_PATTERN_LENGTH - 1);
    return validatePattern(fullPattern);
}

// Undo/Redo functionality
bool PatternDataReplacer::canUndo() const {
    return !undoStack_.empty();
}

bool PatternDataReplacer::canRedo() const {
    return !redoStack_.empty();
}

bool PatternDataReplacer::undoLastOperation() {
    if (undoStack_.empty()) {
        return false;
    }
    
    std::string backupId = undoStack_.back();
    undoStack_.pop_back();
    
    // Create a backup of current state before undoing
    if (!patternBackups_.empty()) {
        auto& lastBackup = patternBackups_.back();
        std::string currentBackupId = createPatternBackup(lastBackup.originalBounds, "Redo point");
        if (!currentBackupId.empty()) {
            redoStack_.push_back(currentBackupId);
        }
    }
    
    return restoreFromBackup(backupId);
}

bool PatternDataReplacer::redoLastOperation() {
    if (redoStack_.empty()) {
        return false;
    }
    
    std::string backupId = redoStack_.back();
    redoStack_.pop_back();
    
    return restoreFromBackup(backupId);
}

void PatternDataReplacer::clearUndoHistory() {
    undoStack_.clear();
    redoStack_.clear();
}

// Integration
void PatternDataReplacer::integrateWithSequencer(SequencerEngine* sequencer) {
    sequencer_ = sequencer;
}

void PatternDataReplacer::integrateWithSampler(AutoSampleLoader* sampler) {
    sampler_ = sampler;
}

void PatternDataReplacer::integrateWithTapeSquashing(TapeSquashingUI* tapeSquashing) {
    tapeSquashing_ = tapeSquashing;
}

// Configuration
void PatternDataReplacer::setReplacementConfig(const ReplacementConfig& config) {
    defaultConfig_ = config;
    
    // Validate configuration
    defaultConfig_.sampleVelocity = std::max(0.0f, std::min(defaultConfig_.sampleVelocity, 1.0f));
}

// Callbacks
void PatternDataReplacer::setReplacementCompleteCallback(ReplacementCompleteCallback callback) {
    replacementCompleteCallback_ = callback;
}

void PatternDataReplacer::setBackupCreatedCallback(BackupCreatedCallback callback) {
    backupCreatedCallback_ = callback;
}

void PatternDataReplacer::setValidationErrorCallback(ValidationErrorCallback callback) {
    validationErrorCallback_ = callback;
}

void PatternDataReplacer::setPatternModifiedCallback(PatternModifiedCallback callback) {
    patternModifiedCallback_ = callback;
}

// Memory management
void PatternDataReplacer::optimizeBackupMemory() {
    // Sort backups by access time and remove oldest if needed
    std::sort(patternBackups_.begin(), patternBackups_.end(),
              [](const PatternBackup& a, const PatternBackup& b) {
                  return a.backupTime < b.backupTime;
              });
    
    while (getTotalBackupMemoryUsage() > maxBackupMemory_ && !patternBackups_.empty()) {
        patternBackups_.erase(patternBackups_.begin());
    }
}

size_t PatternDataReplacer::getEstimatedMemoryUsage() const {
    return getTotalBackupMemoryUsage() + 
           (undoStack_.size() + redoStack_.size()) * sizeof(std::string);
}

// Internal operations
bool PatternDataReplacer::performReplacement(const PatternSelection::SelectionBounds& selection,
                                            const ReplacementConfig& config,
                                            ReplacementResult& result) {
    result.originalStepCount = MAX_PATTERN_LENGTH;  // Mock - would get from sequencer
    
    switch (config.type) {
        case ReplacementType::FULL_SELECTION:
        case ReplacementType::CLEAR_AND_SAMPLE:
            // Clear the region first
            if (!clearRegionData(selection)) {
                result.errorMessage = "Failed to clear region data";
                return false;
            }
            // Fall through to add sample
            
        case ReplacementType::SAMPLE_ONLY:
            {
                // Add sample triggers
                uint8_t targetTrack = (config.targetTrack == 255) ? selection.startTrack : config.targetTrack;
                if (!createSampleTriggers(selection, config.sampleSlot, targetTrack, config.sampleVelocity)) {
                    result.errorMessage = "Failed to create sample triggers";
                    return false;
                }
                break;
            }
            
        case ReplacementType::MERGE_WITH_SAMPLE:
        case ReplacementType::OVERLAY_SAMPLE:
            {
                // More complex operations - simplified for now
                uint8_t targetTrack = (config.targetTrack == 255) ? selection.startTrack : config.targetTrack;
                if (!createSampleTriggers(selection, config.sampleSlot, targetTrack, config.sampleVelocity)) {
                    result.errorMessage = "Failed to overlay sample";
                    return false;
                }
                break;
            }
    }
    
    // Update result
    result.success = true;
    result.newStepCount = result.originalStepCount;  // No length change for now
    result.dataSize = selection.getTotalCells() * 4;  // Estimate 4 bytes per cell
    
    // Add modified tracks
    for (uint16_t track = selection.startTrack; track <= selection.endTrack; ++track) {
        result.modifiedTracks.push_back(static_cast<uint8_t>(track));
    }
    
    return true;
}

bool PatternDataReplacer::extractPatternData(const PatternSelection::SelectionBounds& selection,
                                           std::vector<uint8_t>& patternData) {
    // Mock implementation - in real system would extract actual pattern data
    size_t dataSize = selection.getTotalCells() * 4;  // 4 bytes per cell estimate
    patternData.resize(dataSize);
    
    // Fill with mock pattern data
    for (size_t i = 0; i < dataSize; ++i) {
        patternData[i] = static_cast<uint8_t>(i % 256);
    }
    
    return true;
}

bool PatternDataReplacer::insertPatternData(const PatternSelection::SelectionBounds& selection,
                                          const std::vector<uint8_t>& patternData) {
    // Mock implementation - in real system would insert actual pattern data
    if (patternData.empty()) {
        return false;
    }
    
    // Validate data size matches selection
    size_t expectedSize = selection.getTotalCells() * 4;
    if (patternData.size() != expectedSize) {
        return false;
    }
    
    return true;
}

bool PatternDataReplacer::createSampleTriggers(const PatternSelection::SelectionBounds& selection,
                                             uint8_t sampleSlot, uint8_t targetTrack, float velocity) {
    // Mock implementation - in real system would create actual sample triggers
    if (targetTrack >= MAX_TRACKS) {
        return false;
    }
    
    // Create a sample trigger at the start of the selection
    // In real implementation, would add note data to sequencer pattern
    
    return true;
}

bool PatternDataReplacer::clearRegionData(const PatternSelection::SelectionBounds& selection) {
    // Mock implementation - in real system would clear actual pattern data
    return validateSelectionBounds(selection);
}

// Backup operations
bool PatternDataReplacer::compressPatternData(const std::vector<uint8_t>& input, std::vector<uint8_t>& compressed) {
    // Simple mock compression - just copy data
    // In real implementation would use actual compression algorithm
    compressed = input;
    return true;
}

bool PatternDataReplacer::decompressPatternData(const std::vector<uint8_t>& compressed, std::vector<uint8_t>& output) {
    // Simple mock decompression - just copy data
    output = compressed;
    return true;
}

std::string PatternDataReplacer::generateBackupId() const {
    std::ostringstream oss;
    oss << "backup_" << getCurrentTimeMs() << "_" << (patternBackups_.size() + 1);
    return oss.str();
}

void PatternDataReplacer::pruneOldBackups() {
    // Remove excess backups by count
    while (patternBackups_.size() > maxBackupCount_) {
        patternBackups_.erase(patternBackups_.begin());
    }
    
    // Remove excess backups by memory usage
    optimizeBackupMemory();
}

// Validation helpers
bool PatternDataReplacer::validateSelectionBounds(const PatternSelection::SelectionBounds& selection) const {
    return selection.isValid() &&
           selection.endTrack < MAX_TRACKS &&
           selection.endStep < MAX_PATTERN_LENGTH;
}

void PatternDataReplacer::addValidationError(ValidationResult& result, const std::string& error) const {
    result.errors.push_back(error);
}

void PatternDataReplacer::addValidationWarning(ValidationResult& result, const std::string& warning) const {
    result.warnings.push_back(warning);
}

// Undo/Redo management
void PatternDataReplacer::addToUndoStack(const std::string& backupId) {
    undoStack_.push_back(backupId);
    
    // Clear redo stack when new operation is performed
    redoStack_.clear();
    
    // Prune undo stack if too deep
    pruneUndoStack();
}

void PatternDataReplacer::pruneUndoStack() {
    while (undoStack_.size() > maxUndoDepth_) {
        undoStack_.erase(undoStack_.begin());
    }
}

// Notification helpers
void PatternDataReplacer::notifyReplacementComplete(const ReplacementResult& result) {
    if (replacementCompleteCallback_) {
        replacementCompleteCallback_(result);
    }
}

void PatternDataReplacer::notifyBackupCreated(const std::string& backupId) {
    if (backupCreatedCallback_) {
        backupCreatedCallback_(backupId);
    }
}

void PatternDataReplacer::notifyValidationError(const ValidationResult& result) {
    if (validationErrorCallback_) {
        validationErrorCallback_(result);
    }
}

void PatternDataReplacer::notifyPatternModified(const PatternSelection::SelectionBounds& bounds) {
    if (patternModifiedCallback_) {
        patternModifiedCallback_(bounds);
    }
}

// Utility methods
uint32_t PatternDataReplacer::getCurrentTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}