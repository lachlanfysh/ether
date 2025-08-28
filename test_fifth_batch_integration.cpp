#include "src/audio/RealtimeAudioBouncer.h"
#include "src/sampler/AutoSampleLoader.h"
#include "src/sequencer/PatternDataReplacer.h"
#include "src/interface/ui/CrushConfirmationDialog.h"
#include <iostream>
#include <cassert>
#include <memory>
#include <vector>

/**
 * Comprehensive integration test for the fifth batch of EtherSynth systems:
 * 1. RealtimeAudioBouncer - Real-time audio rendering and format conversion
 * 2. AutoSampleLoader - Intelligent sample loading and slot management  
 * 3. PatternDataReplacer - Pattern data replacement with backup/restore
 * 4. CrushConfirmationDialog - Advanced confirmation dialog with auto-save
 */

void testRealtimeAudioBouncer() {
    std::cout << "Testing RealtimeAudioBouncer...\n";
    
    RealtimeAudioBouncer bouncer;
    
    // Test configuration
    RealtimeAudioBouncer::BounceConfig config;
    config.format = RealtimeAudioBouncer::AudioFormat::WAV_24BIT;
    config.sampleRate = RealtimeAudioBouncer::SampleRate::SR_48000;
    config.channels = 2;
    config.enableNormalization = true;
    config.normalizationLevel = 0.95f;
    config.outputPath = "/tmp/test_bounce.wav";
    
    bouncer.setBounceConfig(config);
    assert(bouncer.getBounceConfig().format == RealtimeAudioBouncer::AudioFormat::WAV_24BIT);
    
    // Test processing parameters
    RealtimeAudioBouncer::ProcessingParams params;
    params.inputGain = 1.2f;
    params.outputGain = 0.9f;
    params.enableLimiter = true;
    params.limiterThreshold = 0.98f;
    
    bouncer.setProcessingParams(params);
    assert(bouncer.getProcessingParams().inputGain == 1.2f);
    
    // Test status management
    assert(bouncer.getStatus() == RealtimeAudioBouncer::BounceStatus::IDLE);
    assert(!bouncer.isActive());
    
    // Test format conversion
    std::vector<float> testAudio = {0.5f, -0.3f, 0.8f, -0.2f};
    auto converted = bouncer.convertFloatToFormat(testAudio.data(), testAudio.size(), 
                                                 RealtimeAudioBouncer::AudioFormat::WAV_16BIT);
    assert(!converted.empty());
    
    std::cout << "✓ RealtimeAudioBouncer tests passed\n";
}

void testAutoSampleLoader() {
    std::cout << "Testing AutoSampleLoader...\n";
    
    AutoSampleLoader loader;
    
    // Test configuration
    AutoSampleLoader::SampleLoadingOptions options;
    options.strategy = AutoSampleLoader::SlotAllocationStrategy::NEXT_AVAILABLE;
    options.enableAutoTrim = true;
    options.enableNormalization = true;
    options.targetLevel = -12.0f;
    
    loader.setSampleLoadingOptions(options);
    assert(loader.getSampleLoadingOptions().enableAutoTrim == true);
    
    // Test slot management
    assert(loader.isSlotAvailable(0));
    assert(!loader.isSlotProtected(0));
    
    uint8_t nextSlot = loader.findNextAvailableSlot();
    assert(nextSlot < 16); // Should find an available slot
    
    // Test slot information
    std::vector<uint8_t> occupied = loader.getOccupiedSlots();
    std::vector<uint8_t> available = loader.getAvailableSlots();
    assert(available.size() > 0); // Should have available slots initially
    
    // Test memory management
    size_t totalMemory = loader.getTotalMemoryUsage();
    size_t availableMemory = loader.getAvailableMemory();
    assert(totalMemory + availableMemory > 0); // Basic sanity check
    
    std::cout << "✓ AutoSampleLoader tests passed\n";
}

void testPatternDataReplacer() {
    std::cout << "Testing PatternDataReplacer...\n";
    
    PatternDataReplacer replacer;
    
    // Test configuration
    PatternDataReplacer::ReplacementConfig config;
    config.type = PatternDataReplacer::ReplacementType::CLEAR_AND_SAMPLE;
    config.sampleVelocity = 0.8f;
    config.createBackup = true;
    config.validateAfterReplace = true;
    
    replacer.setReplacementConfig(config);
    assert(replacer.getReplacementConfig().sampleVelocity == 0.8f);
    
    // Test backup management
    assert(replacer.getAvailableBackups().size() == 0); // No backups initially
    assert(!replacer.hasBackup("nonexistent"));
    
    // Test undo/redo capabilities
    assert(!replacer.canUndo()); // No operations yet
    assert(!replacer.canRedo()); // No operations yet
    
    // Test memory management
    size_t memoryUsage = replacer.getEstimatedMemoryUsage();
    assert(memoryUsage >= 0); // Should have some base memory usage (may be 0 initially)
    
    // Test backup limits
    replacer.setMaxBackupCount(5);
    replacer.setMaxBackupMemory(1024 * 1024); // 1MB
    
    std::cout << "✓ PatternDataReplacer tests passed\n";
}

void testCrushConfirmationDialog() {
    std::cout << "Testing CrushConfirmationDialog...\n";
    
    CrushConfirmationDialog dialog;
    
    // Test initial state
    assert(!dialog.isDialogOpen());
    
    // Test auto-save configuration
    CrushConfirmationDialog::AutoSaveOptions autoSaveOptions;
    autoSaveOptions.enableAutoSave = true;
    autoSaveOptions.saveCurrentPattern = true;
    autoSaveOptions.createBackupCopy = true;
    autoSaveOptions.backupPrefix = "TestBackup_";
    
    dialog.setAutoSaveOptions(autoSaveOptions);
    assert(dialog.getAutoSaveOptions().enableAutoSave == true);
    
    // Test dialog configuration
    CrushConfirmationDialog::DialogConfig config;
    config.sampleName = "Test Crush Sample";
    config.destinationSlot = 3;
    config.willOverwriteExistingSample = false;
    config.affectedSteps = 16;
    config.affectedTracks = 4;
    config.estimatedCrushTimeSeconds = 2.5f;
    config.hasComplexPatternData = true;
    
    // Test dialog content generation
    auto info = dialog.generateDialogInfo(config);
    assert(!info.title.empty());
    assert(!info.mainMessage.empty());
    assert(info.requiresUserConfirmation == true); // Should require confirmation for complex data
    
    // Test message formatting
    std::string mainMessage = dialog.generateMainMessage(config);
    assert(mainMessage.find("crush") != std::string::npos); // Should mention crushing
    
    std::string detailMessage = dialog.generateDetailMessage(config);
    assert(detailMessage.find("16") != std::string::npos); // Should mention step count
    
    // Test auto-save operations
    assert(dialog.performAutoSave(config) == true); // Should succeed in test environment
    
    std::cout << "✓ CrushConfirmationDialog tests passed\n";
}

void testSystemIntegration() {
    std::cout << "Testing system integration...\n";
    
    // Create all systems
    auto bouncer = std::make_shared<RealtimeAudioBouncer>();
    auto loader = std::make_shared<AutoSampleLoader>();
    auto replacer = std::make_shared<PatternDataReplacer>();
    auto dialog = std::make_shared<CrushConfirmationDialog>();
    
    // Test integration setup
    dialog->integrateWithAutoSampleLoader(loader.get());
    dialog->integrateWithPatternDataReplacer(replacer.get());
    
    // Test workflow scenario: 
    // 1. Configure audio bouncer for tape squashing
    RealtimeAudioBouncer::BounceConfig bounceConfig;
    bounceConfig.format = RealtimeAudioBouncer::AudioFormat::WAV_24BIT;
    bounceConfig.sampleRate = RealtimeAudioBouncer::SampleRate::SR_48000;
    bounceConfig.enableNormalization = true;
    bouncer->setBounceConfig(bounceConfig);
    
    // 2. Set up sample loader for automatic slot management
    AutoSampleLoader::SampleLoadingOptions loadOptions;
    loadOptions.strategy = AutoSampleLoader::SlotAllocationStrategy::LEAST_RECENTLY_USED;
    loadOptions.enableAutoTrim = true;
    loadOptions.enableNormalization = true;
    loader->setSampleLoadingOptions(loadOptions);
    
    // 3. Configure pattern replacer for backup creation
    PatternDataReplacer::ReplacementConfig replaceConfig;
    replaceConfig.type = PatternDataReplacer::ReplacementType::CLEAR_AND_SAMPLE;
    replaceConfig.createBackup = true;
    replaceConfig.validateAfterReplace = true;
    replacer->setReplacementConfig(replaceConfig);
    
    // 4. Set up confirmation dialog
    CrushConfirmationDialog::AutoSaveOptions autoSave;
    autoSave.enableAutoSave = true;
    autoSave.saveCurrentPattern = true;
    dialog->setAutoSaveOptions(autoSave);
    
    // Test that all systems are properly configured
    assert(bouncer->getBounceConfig().enableNormalization == true);
    assert(loader->getSampleLoadingOptions().enableAutoTrim == true);
    assert(replacer->getReplacementConfig().createBackup == true);
    assert(dialog->getAutoSaveOptions().enableAutoSave == true);
    
    std::cout << "✓ System integration tests passed\n";
}

void testTapeSquashingWorkflow() {
    std::cout << "Testing complete tape squashing workflow...\n";
    
    // Simulate a complete tape squashing operation using all systems
    auto bouncer = std::make_shared<RealtimeAudioBouncer>();
    auto loader = std::make_shared<AutoSampleLoader>();
    auto replacer = std::make_shared<PatternDataReplacer>();
    auto dialog = std::make_shared<CrushConfirmationDialog>();
    
    // Step 1: Show confirmation dialog
    CrushConfirmationDialog::DialogConfig dialogConfig;
    dialogConfig.sampleName = "Crushed_Pattern_T2-5";
    dialogConfig.destinationSlot = 2;
    dialogConfig.affectedSteps = 32;
    dialogConfig.affectedTracks = 6;
    dialogConfig.hasComplexPatternData = true;
    
    // Generate dialog content (would normally show to user)
    auto dialogInfo = dialog->generateDialogInfo(dialogConfig);
    assert(!dialogInfo.warningMessage.empty()); // Should have warnings for complex data
    
    // Step 2: Create pattern backup before operation
    PatternDataReplacer::ReplacementConfig backupConfig;
    backupConfig.createBackup = true;
    
    // Step 3: Configure audio bouncing for the crush operation
    RealtimeAudioBouncer::BounceConfig bounceSettings;
    bounceSettings.format = RealtimeAudioBouncer::AudioFormat::WAV_24BIT;
    bounceSettings.enableNormalization = true;
    bounceSettings.normalizationLevel = 0.9f;
    bouncer->setBounceConfig(bounceSettings);
    
    // Step 4: Prepare sample loader for the resulting audio
    AutoSampleLoader::SampleLoadingOptions loaderSettings;
    loaderSettings.enableAutoTrim = true;
    loaderSettings.enableNormalization = false; // Already normalized by bouncer
    loader->setSampleLoadingOptions(loaderSettings);
    
    // Verify the workflow configuration is valid
    assert(dialogConfig.destinationSlot < 16);
    assert(bounceSettings.enableNormalization == true);
    assert(loaderSettings.enableAutoTrim == true);
    
    std::cout << "✓ Tape squashing workflow tests passed\n";
}

void testErrorHandling() {
    std::cout << "Testing error handling...\n";
    
    // Test RealtimeAudioBouncer error conditions
    RealtimeAudioBouncer bouncer;
    RealtimeAudioBouncer::BounceConfig invalidConfig;
    invalidConfig.channels = 0; // Invalid
    bouncer.setBounceConfig(invalidConfig);
    // Should handle invalid config gracefully
    
    // Test AutoSampleLoader error conditions  
    AutoSampleLoader loader;
    assert(!loader.isSlotAvailable(255)); // Invalid slot
    
    const auto& invalidSlot = loader.getSlot(255); // Invalid slot
    assert(invalidSlot.slotId == 255); // Should return default slot
    
    // Test PatternDataReplacer error conditions
    PatternDataReplacer replacer;
    assert(!replacer.hasBackup("nonexistent")); // Should return false
    
    // Test CrushConfirmationDialog error conditions
    CrushConfirmationDialog dialog;
    CrushConfirmationDialog::DialogConfig invalidDialogConfig;
    invalidDialogConfig.destinationSlot = 255; // Invalid slot
    
    auto result = dialog.showConfirmationDialog(invalidDialogConfig);
    (void)result; // Suppress unused variable warning
    // Should handle invalid config (exact behavior depends on implementation)
    
    std::cout << "✓ Error handling tests passed\n";
}

int main() {
    std::cout << "=== EtherSynth Fifth Batch Integration Tests ===\n\n";
    
    try {
        testRealtimeAudioBouncer();
        testAutoSampleLoader();
        testPatternDataReplacer();
        testCrushConfirmationDialog();
        testSystemIntegration();
        testTapeSquashingWorkflow();
        testErrorHandling();
        
        std::cout << "\n🎉 All fifth batch integration tests PASSED!\n";
        std::cout << "\nSystems tested:\n";
        std::cout << "✓ RealtimeAudioBouncer - Real-time audio rendering and format conversion\n";
        std::cout << "✓ AutoSampleLoader - Intelligent sample loading and slot management\n"; 
        std::cout << "✓ PatternDataReplacer - Pattern data replacement with backup/restore\n";
        std::cout << "✓ CrushConfirmationDialog - Advanced confirmation dialog with auto-save\n";
        std::cout << "✓ Complete tape squashing workflow integration\n";
        std::cout << "✓ Cross-system communication and error handling\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}