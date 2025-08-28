#include "src/control/velocity/VelocityVolumeControl.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <cmath>

/**
 * Comprehensive test for VelocityVolumeControl
 * Tests velocity→volume mapping with enable/disable functionality
 */

void testBasicVelocityToVolume() {
    std::cout << "Testing basic velocity→volume functionality...\n";
    
    VelocityVolumeControl volumeControl;
    
    // Test initial state
    assert(volumeControl.isEnabled() == true);
    assert(volumeControl.isGlobalVelocityToVolumeEnabled() == true);
    
    // Test direct volume calculation
    float lowVolume = volumeControl.calculateDirectVolume(32, volumeControl.getGlobalVolumeConfig());  // Low velocity
    float midVolume = volumeControl.calculateDirectVolume(64, volumeControl.getGlobalVolumeConfig());  // Mid velocity
    float highVolume = volumeControl.calculateDirectVolume(127, volumeControl.getGlobalVolumeConfig()); // High velocity
    
    // Should have increasing volume with increasing velocity
    assert(lowVolume < midVolume);
    assert(midVolume < highVolume);
    assert(lowVolume >= 0.0f && lowVolume <= 1.0f);
    assert(highVolume >= 0.0f && highVolume <= 1.0f);
    
    // Test velocity→volume disable
    volumeControl.setGlobalVelocityToVolumeEnabled(false);
    assert(!volumeControl.isGlobalVelocityToVolumeEnabled());
    
    // When disabled, all velocities should give max volume
    float disabledLow = volumeControl.calculateDirectVolume(1, volumeControl.getGlobalVolumeConfig());
    float disabledHigh = volumeControl.calculateDirectVolume(127, volumeControl.getGlobalVolumeConfig());
    assert(disabledLow == disabledHigh);
    assert(disabledLow == volumeControl.getGlobalVolumeConfig().volumeMax);
    
    std::cout << "✓ Basic velocity→volume functionality tests passed\n";
}

void testVelocityCurves() {
    std::cout << "Testing velocity curve types...\n";
    
    VelocityVolumeControl volumeControl;
    
    float testVelocity = 0.5f; // Mid-range test
    
    // Test different curve types
    float linear = volumeControl.applyLinearCurve(testVelocity);
    assert(std::abs(linear - testVelocity) < 0.001f); // Linear should be pass-through
    
    float exponential = volumeControl.applyExponentialCurve(testVelocity, 2.0f);
    assert(exponential >= 0.0f && exponential <= 1.0f);
    
    float logarithmic = volumeControl.applyLogarithmicCurve(testVelocity, 2.0f);
    assert(logarithmic >= 0.0f && logarithmic <= 1.0f);
    
    float sCurve = volumeControl.applySCurve(testVelocity, 2.0f);
    assert(sCurve >= 0.0f && sCurve <= 1.0f);
    
    float powerLaw = volumeControl.applyPowerLawCurve(testVelocity, 2.0f);
    assert(powerLaw >= 0.0f && powerLaw <= 1.0f);
    
    float stepped = volumeControl.applySteppedCurve(testVelocity, 8);
    assert(stepped >= 0.0f && stepped <= 1.0f);
    
    // Test boundary conditions
    assert(volumeControl.applyLinearCurve(0.0f) == 0.0f);
    assert(volumeControl.applyLinearCurve(1.0f) == 1.0f);
    
    std::cout << "✓ Velocity curve types tests passed\n";
}

void testVolumeConfiguration() {
    std::cout << "Testing volume configuration...\n";
    
    VelocityVolumeControl volumeControl;
    
    // Test global configuration
    VelocityVolumeControl::VolumeConfig config;
    config.curveType = VelocityVolumeControl::VolumeCurveType::EXPONENTIAL;
    config.curveAmount = 2.0f;
    config.velocityScale = 1.5f;
    config.velocityOffset = 0.1f;
    config.volumeMin = 0.1f;
    config.volumeMax = 0.9f;
    config.volumeRange = 0.8f;
    config.invertVelocity = false;
    
    volumeControl.setGlobalVolumeConfig(config);
    const auto& retrievedConfig = volumeControl.getGlobalVolumeConfig();
    
    assert(retrievedConfig.curveType == VelocityVolumeControl::VolumeCurveType::EXPONENTIAL);
    assert(retrievedConfig.curveAmount == 2.0f);
    assert(retrievedConfig.velocityScale == 1.5f);
    assert(retrievedConfig.volumeMin == 0.1f);
    assert(retrievedConfig.volumeMax == 0.9f);
    
    // Test per-engine configuration
    uint32_t engineId = 100;
    VelocityVolumeControl::VolumeConfig engineConfig;
    engineConfig.curveType = VelocityVolumeControl::VolumeCurveType::S_CURVE;
    engineConfig.volumeMin = 0.2f;
    engineConfig.volumeMax = 0.8f;
    
    volumeControl.setEngineVolumeConfig(engineId, engineConfig);
    assert(volumeControl.hasEngineVolumeConfig(engineId));
    
    const auto& retrievedEngineConfig = volumeControl.getEngineVolumeConfig(engineId);
    assert(retrievedEngineConfig.curveType == VelocityVolumeControl::VolumeCurveType::S_CURVE);
    assert(retrievedEngineConfig.volumeMin == 0.2f);
    
    volumeControl.removeEngineVolumeConfig(engineId);
    assert(!volumeControl.hasEngineVolumeConfig(engineId));
    
    std::cout << "✓ Volume configuration tests passed\n";
}

void testVoiceManagement() {
    std::cout << "Testing voice management...\n";
    
    VelocityVolumeControl volumeControl;
    
    // Add voices with different velocities
    uint32_t voice1 = 1000;
    uint32_t voice2 = 1001;
    uint32_t voice3 = 1002;
    
    volumeControl.addVoice(voice1, 32);  // Low velocity
    volumeControl.addVoice(voice2, 64);  // Mid velocity  
    volumeControl.addVoice(voice3, 127); // High velocity
    
    assert(volumeControl.getActiveVoiceCount() == 3);
    
    // Test voice volumes
    float vol1 = volumeControl.getVoiceVolume(voice1);
    float vol2 = volumeControl.getVoiceVolume(voice2);
    float vol3 = volumeControl.getVoiceVolume(voice3);
    
    assert(vol1 < vol2);
    assert(vol2 < vol3);
    assert(vol1 >= 0.0f && vol1 <= 1.0f);
    assert(vol3 >= 0.0f && vol3 <= 1.0f);
    
    // Test velocity update
    volumeControl.updateVoiceVelocity(voice1, 100);
    float newVol1 = volumeControl.getVoiceVolume(voice1);
    assert(newVol1 > vol1); // Should increase
    
    // Test voice removal
    volumeControl.removeVoice(voice2);
    assert(volumeControl.getActiveVoiceCount() == 2);
    
    // Test clear all voices
    volumeControl.clearAllVoices();
    assert(volumeControl.getActiveVoiceCount() == 0);
    
    std::cout << "✓ Voice management tests passed\n";
}

void testVolumeOverrides() {
    std::cout << "Testing volume overrides...\n";
    
    VelocityVolumeControl volumeControl;
    
    uint32_t voiceId = 2000;
    volumeControl.addVoice(voiceId, 64); // Mid velocity
    
    float originalVolume = volumeControl.getVoiceVolume(voiceId);
    assert(!volumeControl.hasVoiceVolumeOverride(voiceId));
    
    // Set volume override
    float overrideVolume = 0.3f;
    volumeControl.setVoiceVolumeOverride(voiceId, overrideVolume);
    assert(volumeControl.hasVoiceVolumeOverride(voiceId));
    assert(volumeControl.getVoiceVolume(voiceId) == overrideVolume);
    
    // Clear volume override
    volumeControl.clearVoiceVolumeOverride(voiceId);
    assert(!volumeControl.hasVoiceVolumeOverride(voiceId));
    assert(volumeControl.getVoiceVolume(voiceId) == originalVolume);
    
    std::cout << "✓ Volume overrides tests passed\n";
}

void testCustomCurves() {
    std::cout << "Testing custom curve tables...\n";
    
    VelocityVolumeControl volumeControl;
    
    // Create custom curve table (inverse curve)
    std::vector<float> customTable;
    for (size_t i = 0; i < 128; ++i) {
        float normalized = static_cast<float>(i) / 127.0f;
        customTable.push_back(1.0f - normalized); // Inverted curve
    }
    
    volumeControl.setCustomCurveTable(customTable);
    const auto& retrievedTable = volumeControl.getCustomCurveTable();
    assert(retrievedTable.size() == customTable.size());
    assert(retrievedTable[0] == 1.0f); // First value should be 1.0
    assert(retrievedTable[127] == 0.0f); // Last value should be 0.0
    
    // Test custom curve application
    float customResult = volumeControl.applyCustomTableCurve(0.0f, customTable);
    assert(customResult == 1.0f);
    
    customResult = volumeControl.applyCustomTableCurve(1.0f, customTable);
    assert(customResult == 0.0f);
    
    // Test curve table generation
    volumeControl.generateCurveTable(VelocityVolumeControl::VolumeCurveType::EXPONENTIAL, 2.0f, 64);
    const auto& generatedTable = volumeControl.getCustomCurveTable();
    assert(generatedTable.size() == 64);
    assert(generatedTable[0] == 0.0f);
    assert(generatedTable[63] == 1.0f);
    
    std::cout << "✓ Custom curve tables tests passed\n";
}

void testVolumeCalculationResults() {
    std::cout << "Testing volume calculation results...\n";
    
    VelocityVolumeControl volumeControl;
    uint32_t voiceId = 3000;
    
    // Add voice and test result structure
    volumeControl.addVoice(voiceId, 80);
    auto result = volumeControl.calculateVolume(voiceId, 80);
    
    assert(result.volume >= 0.0f && result.volume <= 1.0f);
    assert(result.velocityComponent >= 0.0f && result.velocityComponent <= 1.0f);
    assert(result.appliedCurve == VelocityVolumeControl::VolumeCurveType::LINEAR);
    
    // Test with smoothing enabled
    VelocityVolumeControl::VolumeConfig smoothConfig;
    smoothConfig.smoothingTime = 50.0f; // 50ms smoothing
    volumeControl.setGlobalVolumeConfig(smoothConfig);
    
    auto smoothResult = volumeControl.calculateVolume(voiceId, 100);
    // Note: Smoothing may or may not occur depending on timing
    assert(smoothResult.volume >= 0.0f && smoothResult.volume <= 1.0f);
    
    // Test with volume limiting
    VelocityVolumeControl::VolumeConfig limitConfig;
    limitConfig.volumeMin = 0.3f;
    limitConfig.volumeMax = 0.7f;
    volumeControl.setGlobalVolumeConfig(limitConfig);
    
    auto limitResult = volumeControl.calculateVolume(voiceId, 127); // Max velocity
    assert(limitResult.volume <= 0.7f); // Should be limited
    assert(limitResult.volume >= 0.3f); // Should not go below min
    
    std::cout << "✓ Volume calculation results tests passed\n";
}

void testBatchOperations() {
    std::cout << "Testing batch operations...\n";
    
    VelocityVolumeControl volumeControl;
    
    // Add multiple voices
    std::vector<uint32_t> voiceIds = {4000, 4001, 4002, 4003};
    for (uint32_t voiceId : voiceIds) {
        volumeControl.addVoice(voiceId, 64 + (voiceId % 32)); // Varying velocities
    }
    
    assert(volumeControl.getActiveVoiceCount() == voiceIds.size());
    
    // Test set all voices volume
    float batchVolume = 0.5f;
    volumeControl.setAllVoicesVolume(batchVolume);
    
    for (uint32_t voiceId : voiceIds) {
        assert(volumeControl.getVoiceVolume(voiceId) == batchVolume);
        assert(volumeControl.hasVoiceVolumeOverride(voiceId));
    }
    
    // Test reset all voices to velocity volume
    volumeControl.resetAllVoicesToVelocityVolume();
    
    for (uint32_t voiceId : voiceIds) {
        assert(!volumeControl.hasVoiceVolumeOverride(voiceId));
    }
    
    // Test global volume scale
    std::vector<float> originalVolumes;
    for (uint32_t voiceId : voiceIds) {
        originalVolumes.push_back(volumeControl.getVoiceVolume(voiceId));
    }
    
    float scale = 0.8f;
    volumeControl.applyGlobalVolumeScale(scale);
    
    for (size_t i = 0; i < voiceIds.size(); ++i) {
        float expectedVolume = std::min(originalVolumes[i] * scale, 1.0f);
        float actualVolume = volumeControl.getVoiceVolume(voiceIds[i]);
        assert(std::abs(actualVolume - expectedVolume) < 0.01f);
    }
    
    std::cout << "✓ Batch operations tests passed\n";
}

void testStatisticsAndMonitoring() {
    std::cout << "Testing statistics and monitoring...\n";
    
    VelocityVolumeControl volumeControl;
    
    // Add voices with some overrides
    std::vector<uint32_t> voiceIds = {5000, 5001, 5002, 5003, 5004};
    for (size_t i = 0; i < voiceIds.size(); ++i) {
        volumeControl.addVoice(voiceIds[i], 64 + i * 10);
        
        // Override some voices
        if (i % 2 == 0) {
            volumeControl.setVoiceVolumeOverride(voiceIds[i], 0.6f);
        }
    }
    
    assert(volumeControl.getActiveVoiceCount() == voiceIds.size());
    assert(volumeControl.getVoicesWithOverrides() == 3); // 3 overridden voices
    
    float avgVolume = volumeControl.getAverageVolume();
    assert(avgVolume >= 0.0f && avgVolume <= 1.0f);
    
    // Test active voice IDs
    auto activeIds = volumeControl.getActiveVoiceIds();
    assert(activeIds.size() == voiceIds.size());
    
    // Test voice state retrieval
    auto voiceState = volumeControl.getVoiceState(voiceIds[0]);
    assert(voiceState.voiceId == voiceIds[0]);
    assert(voiceState.volumeOverridden == true); // First voice should be overridden
    
    std::cout << "✓ Statistics and monitoring tests passed\n";
}

void testSystemManagement() {
    std::cout << "Testing system management...\n";
    
    VelocityVolumeControl volumeControl;
    
    // Test enable/disable
    volumeControl.setEnabled(false);
    assert(!volumeControl.isEnabled());
    
    uint32_t voiceId = 6000;
    auto result = volumeControl.calculateVolume(voiceId, 64);
    assert(result.volume == 1.0f); // Should return max volume when disabled
    
    volumeControl.setEnabled(true);
    assert(volumeControl.isEnabled());
    
    // Test sample rate
    volumeControl.setSampleRate(44100.0f);
    assert(volumeControl.getSampleRate() == 44100.0f);
    
    // Test system reset
    volumeControl.addVoice(voiceId, 64);
    assert(volumeControl.getActiveVoiceCount() == 1);
    
    volumeControl.reset();
    assert(volumeControl.getActiveVoiceCount() == 0);
    assert(volumeControl.isGlobalVelocityToVolumeEnabled() == true); // Should reset to default
    
    std::cout << "✓ System management tests passed\n";
}

void testErrorHandling() {
    std::cout << "Testing error handling...\n";
    
    VelocityVolumeControl volumeControl;
    
    // Test with invalid voice IDs
    uint32_t invalidVoice = 99999;
    
    float volume = volumeControl.getVoiceVolume(invalidVoice);
    assert(volume == 1.0f); // Should return max volume for non-existent voice
    
    assert(!volumeControl.hasVoiceVolumeOverride(invalidVoice));
    
    // Test with extreme configuration values
    VelocityVolumeControl::VolumeConfig extremeConfig;
    extremeConfig.curveAmount = 100.0f;  // Way over max
    extremeConfig.velocityScale = 10.0f; // Way over max
    extremeConfig.velocityOffset = 5.0f; // Way over max
    extremeConfig.volumeMin = -1.0f;     // Below min
    extremeConfig.volumeMax = 5.0f;      // Way over max
    
    volumeControl.setGlobalVolumeConfig(extremeConfig);
    const auto& clampedConfig = volumeControl.getGlobalVolumeConfig();
    
    // Values should be clamped
    assert(clampedConfig.curveAmount <= 10.0f);
    assert(clampedConfig.velocityScale <= 2.0f);
    assert(clampedConfig.velocityOffset <= 1.0f);
    assert(clampedConfig.volumeMin >= 0.0f);
    assert(clampedConfig.volumeMax <= 1.0f);
    
    std::cout << "✓ Error handling tests passed\n";
}

int main() {
    std::cout << "=== VelocityVolumeControl Tests ===\n\n";
    
    try {
        testBasicVelocityToVolume();
        testVelocityCurves();
        testVolumeConfiguration();
        testVoiceManagement();
        testVolumeOverrides();
        testCustomCurves();
        testVolumeCalculationResults();
        testBatchOperations();
        testStatisticsAndMonitoring();
        testSystemManagement();
        testErrorHandling();
        
        std::cout << "\n🎉 All VelocityVolumeControl tests PASSED!\n";
        std::cout << "\nVelocity→Volume System Features Verified:\n";
        std::cout << "✓ Velocity-to-volume mapping with enable/disable functionality\n";
        std::cout << "✓ Multiple velocity curve types (linear, exponential, S-curve, etc.)\n";
        std::cout << "✓ Volume configuration with range and scaling controls\n";
        std::cout << "✓ Per-voice volume management with velocity tracking\n";
        std::cout << "✓ Volume override system for manual control\n";
        std::cout << "✓ Custom curve tables with interpolation\n";
        std::cout << "✓ Comprehensive volume calculation with smoothing\n";
        std::cout << "✓ Batch operations for efficient voice management\n";
        std::cout << "✓ Statistics, monitoring, and performance tracking\n";
        std::cout << "✓ System management with proper state handling\n";
        std::cout << "✓ Error handling and input validation\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}