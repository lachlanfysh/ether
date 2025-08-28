#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <functional>
#include "PatternSelection.h"

/**
 * PatternDataReplacer - Destructive pattern data replacement after successful crush
 * 
 * Provides safe and comprehensive pattern data replacement functionality:
 * - Destructive replacement of selected pattern regions with crushed samples
 * - Backup and restore capabilities for undo operations
 * - Pattern data validation and consistency checking
 * - Integration with tape squashing workflow for seamless sample replacement
 * - Multi-track pattern manipulation with atomic operations
 * - Hardware-optimized for STM32 H7 embedded platform
 * 
 * Features:
 * - Atomic pattern replacement operations (all-or-nothing)
 * - Pattern backup system with compression for memory efficiency
 * - Validation of pattern data before and after replacement
 * - Integration with sequencer engine for real-time safe operations
 * - Support for partial pattern replacement within selections
 * - Automatic pattern length adjustment and optimization
 * - Undo/redo capability with configurable history depth
 * - Pattern consistency verification and repair
 */
class PatternDataReplacer {
public:
    // Replacement operation types
    enum class ReplacementType {
        FULL_SELECTION,     // Replace entire selected region
        SAMPLE_ONLY,        // Replace with sample data only
        CLEAR_AND_SAMPLE,   // Clear selection then add sample
        MERGE_WITH_SAMPLE,  // Merge sample with existing data
        OVERLAY_SAMPLE      // Overlay sample on existing pattern
    };
    
    // Pattern backup information
    struct PatternBackup {
        std::string backupId;           // Unique backup identifier
        PatternSelection::SelectionBounds originalBounds;  // Original selection
        std::vector<uint8_t> compressedData;  // Compressed pattern data
        uint32_t backupTime;            // Timestamp of backup
        std::string operation;          // Description of operation
        size_t uncompressedSize;        // Original data size
        
        PatternBackup() : backupTime(0), uncompressedSize(0) {}
    };
    
    // Replacement configuration
    struct ReplacementConfig {
        ReplacementType type;           // Type of replacement operation
        uint8_t targetTrack;            // Target track for sample (255=auto)
        bool preserveVelocity;          // Preserve original velocity data
        bool preserveTiming;            // Preserve original timing/swing
        bool adjustPatternLength;       // Auto-adjust pattern length if needed
        bool validateAfterReplace;      // Validate pattern after replacement
        bool createBackup;              // Create backup before replacement
        float sampleVelocity;           // Velocity for new sample notes (0.0-1.0)
        uint8_t sampleSlot;            // Sampler slot containing crushed audio
        
        ReplacementConfig() :
            type(ReplacementType::FULL_SELECTION),
            targetTrack(255),  // Auto-select
            preserveVelocity(false),
            preserveTiming(false),
            adjustPatternLength(true),
            validateAfterReplace(true),
            createBackup(true),
            sampleVelocity(1.0f),
            sampleSlot(0) {}
    };
    
    // Replacement result information
    struct ReplacementResult {
        bool success;                   // Whether replacement succeeded
        std::string backupId;           // ID of created backup (if any)
        PatternSelection::SelectionBounds affectedRegion;  // Actually affected region
        uint16_t originalStepCount;     // Original pattern length
        uint16_t newStepCount;          // New pattern length
        std::vector<uint8_t> modifiedTracks;  // Tracks that were modified
        size_t dataSize;                // Size of replaced data
        std::string errorMessage;       // Error message if failed
        
        ReplacementResult() :
            success(false),
            originalStepCount(0),
            newStepCount(0),
            dataSize(0) {}
    };
    
    // Pattern data validation result
    struct ValidationResult {
        bool isValid;                   // Whether pattern data is valid
        std::vector<std::string> errors;  // List of validation errors
        std::vector<std::string> warnings; // List of validation warnings
        uint32_t totalSteps;            // Total steps in pattern
        uint32_t totalTracks;           // Total tracks in pattern
        bool hasEmptyTracks;            // Whether pattern has empty tracks
        bool hasLongPattern;            // Whether pattern exceeds normal length
        
        ValidationResult() :
            isValid(false),
            totalSteps(0),
            totalTracks(0),
            hasEmptyTracks(false),
            hasLongPattern(false) {}
    };
    
    PatternDataReplacer();
    ~PatternDataReplacer() = default;
    
    // Main replacement operations
    ReplacementResult replacePatternData(const PatternSelection::SelectionBounds& selection,
                                        const ReplacementConfig& config);
    ReplacementResult replaceWithSample(const PatternSelection::SelectionBounds& selection,
                                       uint8_t sampleSlot, uint8_t targetTrack = 255);
    ReplacementResult clearAndReplacewithSample(const PatternSelection::SelectionBounds& selection,
                                               uint8_t sampleSlot, uint8_t targetTrack = 255);
    
    // Backup and restore operations
    std::string createPatternBackup(const PatternSelection::SelectionBounds& selection,
                                   const std::string& operationDescription = "");
    bool restoreFromBackup(const std::string& backupId);
    bool removeBackup(const std::string& backupId);
    void clearAllBackups();
    
    // Backup management
    std::vector<PatternBackup> getAvailableBackups() const;
    bool hasBackup(const std::string& backupId) const;
    size_t getTotalBackupMemoryUsage() const;
    void setMaxBackupCount(size_t maxCount);
    void setMaxBackupMemory(size_t maxMemoryBytes);
    
    // Pattern validation
    ValidationResult validatePattern(const PatternSelection::SelectionBounds& selection) const;
    ValidationResult validateEntirePattern() const;
    bool repairPatternInconsistencies(const PatternSelection::SelectionBounds& selection);
    
    // Undo/Redo functionality
    bool canUndo() const;
    bool canRedo() const;
    bool undoLastOperation();
    bool redoLastOperation();
    void clearUndoHistory();
    
    // Integration with sequencer
    void integrateWithSequencer(class SequencerEngine* sequencer);
    void integrateWithSampler(class AutoSampleLoader* sampler);
    void integrateWithTapeSquashing(class TapeSquashingUI* tapeSquashing);
    
    // Configuration
    void setReplacementConfig(const ReplacementConfig& config);
    const ReplacementConfig& getReplacementConfig() const { return defaultConfig_; }
    
    // Callbacks
    using ReplacementCompleteCallback = std::function<void(const ReplacementResult&)>;
    using BackupCreatedCallback = std::function<void(const std::string& backupId)>;
    using ValidationErrorCallback = std::function<void(const ValidationResult&)>;
    using PatternModifiedCallback = std::function<void(const PatternSelection::SelectionBounds&)>;
    
    void setReplacementCompleteCallback(ReplacementCompleteCallback callback);
    void setBackupCreatedCallback(BackupCreatedCallback callback);
    void setValidationErrorCallback(ValidationErrorCallback callback);
    void setPatternModifiedCallback(PatternModifiedCallback callback);
    
    // Memory management
    void optimizeBackupMemory();
    size_t getEstimatedMemoryUsage() const;
    
private:
    // Configuration
    ReplacementConfig defaultConfig_;
    size_t maxBackupCount_;
    size_t maxBackupMemory_;
    
    // Backup storage
    std::vector<PatternBackup> patternBackups_;
    std::vector<std::string> undoStack_;
    std::vector<std::string> redoStack_;
    size_t maxUndoDepth_;
    
    // Integration
    class SequencerEngine* sequencer_;
    class AutoSampleLoader* sampler_;
    class TapeSquashingUI* tapeSquashing_;
    
    // Callbacks
    ReplacementCompleteCallback replacementCompleteCallback_;
    BackupCreatedCallback backupCreatedCallback_;
    ValidationErrorCallback validationErrorCallback_;
    PatternModifiedCallback patternModifiedCallback_;
    
    // Internal operations
    bool performReplacement(const PatternSelection::SelectionBounds& selection,
                           const ReplacementConfig& config,
                           ReplacementResult& result);
    bool extractPatternData(const PatternSelection::SelectionBounds& selection,
                           std::vector<uint8_t>& patternData);
    bool insertPatternData(const PatternSelection::SelectionBounds& selection,
                          const std::vector<uint8_t>& patternData);
    
    // Sample integration
    bool createSampleTriggers(const PatternSelection::SelectionBounds& selection,
                             uint8_t sampleSlot, uint8_t targetTrack, float velocity);
    bool clearRegionData(const PatternSelection::SelectionBounds& selection);
    
    // Backup operations
    bool compressPatternData(const std::vector<uint8_t>& input, std::vector<uint8_t>& compressed);
    bool decompressPatternData(const std::vector<uint8_t>& compressed, std::vector<uint8_t>& output);
    std::string generateBackupId() const;
    void pruneOldBackups();
    
    // Validation helpers
    bool validateSelectionBounds(const PatternSelection::SelectionBounds& selection) const;
    bool validatePatternDataIntegrity(const std::vector<uint8_t>& patternData) const;
    void addValidationError(ValidationResult& result, const std::string& error) const;
    void addValidationWarning(ValidationResult& result, const std::string& warning) const;
    
    // Undo/Redo management
    void addToUndoStack(const std::string& backupId);
    void pruneUndoStack();
    
    // Notification helpers
    void notifyReplacementComplete(const ReplacementResult& result);
    void notifyBackupCreated(const std::string& backupId);
    void notifyValidationError(const ValidationResult& result);
    void notifyPatternModified(const PatternSelection::SelectionBounds& bounds);
    
    // Utility methods
    uint32_t getCurrentTimeMs() const;
    
    // Constants
    static constexpr size_t DEFAULT_MAX_BACKUPS = 10;
    static constexpr size_t DEFAULT_MAX_BACKUP_MEMORY = 1024 * 1024;  // 1MB
    static constexpr size_t DEFAULT_MAX_UNDO_DEPTH = 20;
    static constexpr uint16_t MAX_PATTERN_LENGTH = 256;  // Maximum steps per pattern
    static constexpr uint8_t MAX_TRACKS = 16;           // Maximum tracks
};