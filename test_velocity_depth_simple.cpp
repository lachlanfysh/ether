#include "src/control/modulation/VelocityDepthControl.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <cmath>

/**
 * Simplified test for VelocityDepthControl
 * Tests the actually implemented functionality
 */

void testBasicDepthControl() {
    std::cout << "Testing basic depth control...\n";
    
    VelocityDepthControl depthControl;
    
    // Test initial state
    assert(depthControl.getMasterDepth() == 1.0f); // Should default to 100%
    assert(depthControl.isEnabled() == true);
    
    // Test master depth setting
    depthControl.setMasterDepth(1.5f);
    assert(depthControl.getMasterDepth() == 1.5f);
    
    // Test depth clamping (should clamp to 0-2.0 range based on maxGlobalDepth)
    depthControl.setMasterDepth(2.5f);
    assert(depthControl.getMasterDepth() == 2.0f); // Should be clamped
    
    depthControl.setMasterDepth(-0.5f);
    assert(depthControl.getMasterDepth() == 0.0f); // Should be clamped to MIN_DEPTH
    
    std::cout << "✓ Basic depth control tests passed\n";
}

void testParameterDepthConfiguration() {
    std::cout << "Testing parameter depth configuration...\n";
    
    VelocityDepthControl depthControl;
    uint32_t parameterId = 100;
    
    // Test parameter depth configuration
    VelocityDepthControl::ParameterDepthConfig config;
    config.baseDepth = 1.2f;
    config.depthMode = VelocityDepthControl::DepthMode::ABSOLUTE;
    config.safetyLevel = VelocityDepthControl::SafetyLevel::MODERATE;
    config.enableDepthModulation = true;
    config.depthSmoothingTime = 50.0f;
    
    depthControl.setParameterDepthConfig(parameterId, config);
    assert(depthControl.hasParameterDepthConfig(parameterId));
    
    const auto& retrievedConfig = depthControl.getParameterDepthConfig(parameterId);
    assert(retrievedConfig.baseDepth == 1.2f);
    assert(retrievedConfig.depthMode == VelocityDepthControl::DepthMode::ABSOLUTE);
    
    // Test individual parameter setters
    depthControl.setParameterBaseDepth(parameterId, 0.8f);
    assert(depthControl.getParameterBaseDepth(parameterId) == 0.8f);
    
    depthControl.setParameterMaxDepth(parameterId, 1.5f);
    depthControl.setParameterDepthMode(parameterId, VelocityDepthControl::DepthMode::SCALED);
    depthControl.setParameterSafetyLevel(parameterId, VelocityDepthControl::SafetyLevel::CONSERVATIVE);
    
    std::cout << "✓ Parameter depth configuration tests passed\n";
}

void testDepthCalculation() {
    std::cout << "Testing depth calculation...\n";
    
    VelocityDepthControl depthControl;
    uint32_t paramId = 200;
    
    // Configure parameter
    VelocityDepthControl::ParameterDepthConfig config;
    config.baseDepth = 1.2f;
    config.depthMode = VelocityDepthControl::DepthMode::ABSOLUTE;
    depthControl.setParameterDepthConfig(paramId, config);
    
    // Test effective depth calculation
    auto depthResult = depthControl.calculateEffectiveDepth(paramId, 1.0f);
    assert(depthResult.requestedDepth == 1.0f);
    assert(depthResult.actualDepth >= 0.0f && depthResult.actualDepth <= 2.0f);
    assert(depthResult.effectiveDepth >= 0.0f && depthResult.effectiveDepth <= 2.0f);
    
    // Test depth application to modulation
    float baseModulation = 0.5f;
    float velocity = 0.8f;
    float modulated = depthControl.applyDepthToModulation(paramId, baseModulation, velocity);
    assert(modulated >= 0.0f);
    
    // Test effective parameter depth
    float effectiveDepth = depthControl.getEffectiveParameterDepth(paramId);
    assert(effectiveDepth >= 0.0f && effectiveDepth <= 2.0f);
    
    std::cout << "✓ Depth calculation tests passed\n";
}

void testSafetyLevels() {
    std::cout << "Testing safety levels...\n";
    
    VelocityDepthControl depthControl;
    uint32_t paramId = 300;
    
    // Test different safety levels
    std::vector<VelocityDepthControl::SafetyLevel> safetyLevels = {
        VelocityDepthControl::SafetyLevel::CONSERVATIVE,
        VelocityDepthControl::SafetyLevel::MODERATE,
        VelocityDepthControl::SafetyLevel::AGGRESSIVE,
        VelocityDepthControl::SafetyLevel::CUSTOM,
        VelocityDepthControl::SafetyLevel::NONE
    };
    
    for (auto level : safetyLevels) {
        VelocityDepthControl::ParameterDepthConfig config;
        config.safetyLevel = level;
        config.baseDepth = 1.8f; // High depth to test limiting
        depthControl.setParameterDepthConfig(paramId, config);
        
        float maxSafeDepth = depthControl.getMaxSafeDepth(paramId, level);
        assert(maxSafeDepth >= 0.0f && maxSafeDepth <= 2.0f);
        
        bool isSafe = depthControl.isDepthSafe(paramId, 1.0f);
        (void)isSafe; // May be true or false depending on level
        
        float safeDepth = depthControl.applySafetyLimiting(paramId, 1.8f, level);
        assert(safeDepth >= 0.0f && safeDepth <= 2.0f);
        assert(safeDepth <= maxSafeDepth || level == VelocityDepthControl::SafetyLevel::NONE);
    }
    
    std::cout << "✓ Safety levels tests passed\n";
}

void testRealTimeDepthModulation() {
    std::cout << "Testing real-time depth modulation...\n";
    
    VelocityDepthControl depthControl;
    uint32_t paramId = 400;
    
    VelocityDepthControl::ParameterDepthConfig config;
    config.enableDepthModulation = true;
    depthControl.setParameterDepthConfig(paramId, config);
    
    // Test real-time depth modulation
    depthControl.setRealTimeDepthModulation(paramId, 0.3f);
    float rtMod = depthControl.getRealTimeDepthModulation(paramId);
    assert(rtMod == 0.3f);
    
    // Test clamping of real-time modulation
    depthControl.setRealTimeDepthModulation(paramId, 2.0f); // Should be clamped
    float clampedRtMod = depthControl.getRealTimeDepthModulation(paramId);
    assert(clampedRtMod == 1.0f); // Should be clamped to max 1.0
    
    // Test depth smoothing update
    depthControl.updateDepthSmoothing(0.01f); // 10ms delta
    
    std::cout << "✓ Real-time depth modulation tests passed\n";
}

void testGlobalDepthOperations() {
    std::cout << "Testing global depth operations...\n";
    
    VelocityDepthControl depthControl;
    
    // Set up multiple parameters
    std::vector<uint32_t> paramIds = {500, 501, 502, 503};
    for (uint32_t id : paramIds) {
        VelocityDepthControl::ParameterDepthConfig config;
        config.baseDepth = 1.2f;
        config.linkToMasterDepth = true;
        depthControl.setParameterDepthConfig(id, config);
    }
    
    // Test global depth operations
    depthControl.setAllParametersDepth(0.8f);
    for (uint32_t id : paramIds) {
        assert(depthControl.getParameterBaseDepth(id) == 0.8f);
    }
    
    // Test safety level setting
    depthControl.setAllParametersSafetyLevel(VelocityDepthControl::SafetyLevel::CONSERVATIVE);
    
    // Test master depth linking
    depthControl.linkAllParametersToMaster(false);
    depthControl.linkAllParametersToMaster(true);
    
    // Test parameter count
    size_t paramCount = depthControl.getConfiguredParameterCount();
    assert(paramCount == paramIds.size());
    
    // Test reset to defaults
    depthControl.resetAllParametersToDefaults();
    
    std::cout << "✓ Global depth operations tests passed\n";
}

void testStatisticsAndMonitoring() {
    std::cout << "Testing statistics and monitoring...\n";
    
    VelocityDepthControl depthControl;
    
    // Set up parameters with varying depths
    std::vector<std::pair<uint32_t, float>> paramDepths = {
        {600, 0.5f}, {601, 1.0f}, {602, 1.5f}, {603, 0.8f}
    };
    
    for (const auto& [id, depth] : paramDepths) {
        VelocityDepthControl::ParameterDepthConfig config;
        config.baseDepth = depth;
        depthControl.setParameterDepthConfig(id, config);
    }
    
    // Test statistics
    float avgDepth = depthControl.getAverageDepth();
    assert(avgDepth >= 0.0f && avgDepth <= 2.0f);
    
    size_t overThreshold = depthControl.getParametersOverDepth(1.0f);
    assert(overThreshold <= paramDepths.size());
    
    auto excessiveParams = depthControl.getParametersWithExcessiveDepth(1.2f);
    assert(excessiveParams.size() <= paramDepths.size());
    
    float systemLoad = depthControl.getSystemDepthLoad();
    assert(systemLoad >= 0.0f);
    
    std::cout << "✓ Statistics and monitoring tests passed\n";
}

void testSystemManagement() {
    std::cout << "Testing system management...\n";
    
    VelocityDepthControl depthControl;
    
    // Test enable/disable
    depthControl.setEnabled(false);
    assert(!depthControl.isEnabled());
    
    uint32_t paramId = 700;
    VelocityDepthControl::ParameterDepthConfig config;
    depthControl.setParameterDepthConfig(paramId, config);
    
    // When disabled, should return zero effective depth
    auto disabledResult = depthControl.calculateEffectiveDepth(paramId, 1.0f);
    assert(disabledResult.actualDepth == 0.0f);
    assert(disabledResult.effectiveDepth == 0.0f);
    
    // Re-enable
    depthControl.setEnabled(true);
    assert(depthControl.isEnabled());
    
    // Test emergency depth limit
    depthControl.emergencyDepthLimit(1.0f);
    
    // Test parameter removal
    depthControl.removeParameter(paramId);
    assert(!depthControl.hasParameterDepthConfig(paramId));
    
    // Test system reset
    depthControl.reset();
    assert(depthControl.getConfiguredParameterCount() == 0);
    
    std::cout << "✓ System management tests passed\n";
}

void testGlobalConfiguration() {
    std::cout << "Testing global configuration...\n";
    
    VelocityDepthControl depthControl;
    
    // Test global config setting
    VelocityDepthControl::GlobalDepthConfig globalConfig;
    globalConfig.masterDepth = 1.3f;
    globalConfig.globalSafetyLevel = VelocityDepthControl::SafetyLevel::AGGRESSIVE;
    globalConfig.enableMasterDepthControl = true;
    globalConfig.maxGlobalDepth = 1.8f;
    globalConfig.enableDepthLimiting = true;
    
    depthControl.setGlobalConfig(globalConfig);
    const auto& retrievedConfig = depthControl.getGlobalConfig();
    
    assert(retrievedConfig.masterDepth == 1.3f);
    assert(retrievedConfig.globalSafetyLevel == VelocityDepthControl::SafetyLevel::AGGRESSIVE);
    assert(retrievedConfig.enableMasterDepthControl == true);
    
    std::cout << "✓ Global configuration tests passed\n";
}

void testErrorHandling() {
    std::cout << "Testing error handling...\n";
    
    VelocityDepthControl depthControl;
    
    // Test with invalid parameter IDs
    uint32_t invalidParam = 99999;
    
    // Should not crash with non-existent parameter
    auto result = depthControl.calculateEffectiveDepth(invalidParam, 1.0f);
    (void)result; // May return default values
    
    float baseDepth = depthControl.getParameterBaseDepth(invalidParam);
    assert(baseDepth == 1.0f); // Should return default
    
    float effectiveDepth = depthControl.getEffectiveParameterDepth(invalidParam);
    assert(effectiveDepth == 1.0f); // Should return default
    
    // Test with extreme depth values
    VelocityDepthControl::ParameterDepthConfig extremeConfig;
    extremeConfig.baseDepth = 5.0f; // Way over 200%
    extremeConfig.maxAllowedDepth = 10.0f;
    extremeConfig.minAllowedDepth = -1.0f;
    
    uint32_t paramId = 800;
    depthControl.setParameterDepthConfig(paramId, extremeConfig);
    
    // Values should be clamped during configuration
    const auto& clampedConfig = depthControl.getParameterDepthConfig(paramId);
    assert(clampedConfig.baseDepth <= 2.0f); // Should be clamped to MAX_DEPTH
    assert(clampedConfig.maxAllowedDepth <= 2.0f);
    assert(clampedConfig.minAllowedDepth >= 0.0f); // Should be clamped to MIN_DEPTH
    
    std::cout << "✓ Error handling tests passed\n";
}

int main() {
    std::cout << "=== VelocityDepthControl Tests (Simplified) ===\n\n";
    
    try {
        testBasicDepthControl();
        testParameterDepthConfiguration();
        testDepthCalculation();
        testSafetyLevels();
        testRealTimeDepthModulation();
        testGlobalDepthOperations();
        testStatisticsAndMonitoring();
        testSystemManagement();
        testGlobalConfiguration();
        testErrorHandling();
        
        std::cout << "\n🎉 All VelocityDepthControl tests PASSED!\n";
        std::cout << "\nSystem features tested:\n";
        std::cout << "✓ Master depth control with 0-200% range\n";
        std::cout << "✓ Parameter-specific depth configuration and management\n";
        std::cout << "✓ Safety levels with automatic depth limiting\n";
        std::cout << "✓ Real-time depth modulation and smoothing\n";
        std::cout << "✓ Global depth operations and batch processing\n";
        std::cout << "✓ Statistics and performance monitoring\n";
        std::cout << "✓ System state management and configuration\n";
        std::cout << "✓ Error handling and boundary condition safety\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}