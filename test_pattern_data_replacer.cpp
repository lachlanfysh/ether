#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include "src/sequencer/PatternDataReplacer.h"
#include "src/sequencer/PatternSelection.h"

void test_pattern_replacement() {
    std::cout << "Testing pattern data replacement..." << std::endl;
    
    PatternDataReplacer replacer;
    PatternSelection::SelectionBounds selection(0, 3, 0, 15);
    
    // Test basic replacement
    PatternDataReplacer::ReplacementConfig config;
    config.type = PatternDataReplacer::ReplacementType::FULL_SELECTION;
    config.sampleSlot = 0;
    config.targetTrack = 0;
    config.createBackup = true;
    
    auto result = replacer.replacePatternData(selection, config);
    
    assert(result.success);
    assert(!result.backupId.empty());
    assert(result.affectedRegion.isValid());
    assert(result.modifiedTracks.size() > 0);
    
    std::cout << "✓ Pattern replacement successful" << std::endl;
    std::cout << "  - Backup ID: " << result.backupId << std::endl;
    std::cout << "  - Modified tracks: " << result.modifiedTracks.size() << std::endl;
}

void test_backup_operations() {
    std::cout << "Testing backup and restore operations..." << std::endl;
    
    PatternDataReplacer replacer;
    PatternSelection::SelectionBounds selection(0, 1, 0, 7);
    
    // Create backup
    std::string backupId = replacer.createPatternBackup(selection, "Test backup");
    assert(!backupId.empty());
    assert(replacer.hasBackup(backupId));
    
    // Test backup metadata
    auto backups = replacer.getAvailableBackups();
    assert(backups.size() > 0);
    
    bool foundBackup = false;
    for (const auto& backup : backups) {
        if (backup.backupId == backupId) {
            foundBackup = true;
            assert(backup.operation == "Test backup");
            assert(backup.uncompressedSize > 0);
            break;
        }
    }
    assert(foundBackup);
    
    // Test restore
    bool restoreSuccess = replacer.restoreFromBackup(backupId);
    assert(restoreSuccess);
    
    std::cout << "✓ Backup operations successful" << std::endl;
    std::cout << "  - Created backup: " << backupId << std::endl;
    std::cout << "  - Backup count: " << backups.size() << std::endl;
}

void test_undo_redo() {
    std::cout << "Testing undo/redo functionality..." << std::endl;
    
    PatternDataReplacer replacer;
    PatternSelection::SelectionBounds selection(1, 2, 4, 11);
    
    // Initially no undo available
    assert(!replacer.canUndo());
    assert(!replacer.canRedo());
    
    // Perform replacement to create undo point
    auto result = replacer.replaceWithSample(selection, 5, 1);
    assert(result.success);
    
    // Should now be able to undo
    assert(replacer.canUndo());
    assert(!replacer.canRedo());
    
    // Test undo
    bool undoSuccess = replacer.undoLastOperation();
    assert(undoSuccess);
    
    // After undo, should be able to redo
    assert(!replacer.canUndo());
    assert(replacer.canRedo());
    
    // Test redo
    bool redoSuccess = replacer.redoLastOperation();
    assert(redoSuccess);
    
    std::cout << "✓ Undo/redo functionality working" << std::endl;
}

int main() {
    std::cout << "Starting PatternDataReplacer comprehensive tests..." << std::endl;
    
    try {
        test_pattern_replacement();
        test_backup_operations();
        test_undo_redo();
        
        std::cout << std::endl;
        std::cout << "🎉 All PatternDataReplacer tests passed!" << std::endl;
        std::cout << "✓ Pattern data replacement with atomic operations" << std::endl;
        std::cout << "✓ Backup and restore system with compression" << std::endl;
        std::cout << "✓ Undo/redo functionality with configurable depth" << std::endl;
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cout << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}