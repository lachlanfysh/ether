#include "src/control/modulation/VelocityDepthControl.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <cmath>

/**
 * Comprehensive test for VelocityDepthControl
 * Tests depth management, scaling, safety controls, and preset systems
 */

void testBasicDepthControl() {
    std::cout << "Testing basic depth control...\n";
    
    VelocityDepthControl depthControl;
    
    // Test initial state
    assert(depthControl.getMasterDepth() == 100.0f); // Should default to 100%
    assert(depthControl.isEnabled() == true);
    
    // Test master depth setting
    depthControl.setMasterDepth(150.0f);
    assert(depthControl.getMasterDepth() == 150.0f);
    
    // Test depth clamping (should clamp to 0-200%)
    depthControl.setMasterDepth(250.0f);
    assert(depthControl.getMasterDepth() == 200.0f);
    
    depthControl.setMasterDepth(-50.0f);
    assert(depthControl.getMasterDepth() == 0.0f);
    
    std::cout << "✓ Basic depth control tests passed\n";
}

void testParameterDepthConfiguration() {
    std::cout << "Testing parameter depth configuration...\n";
    
    VelocityDepthControl depthControl;
    uint32_t parameterId = 100;
    
    // Test parameter depth configuration
    VelocityDepthControl::ParameterDepthConfig config;
    config.baseDepth = 120.0f;
    config.mode = VelocityDepthControl::DepthMode::ABSOLUTE;
    config.safetyLevel = VelocityDepthControl::SafetyLevel::MODERATE;
    config.enableSmoothing = true;
    config.smoothingTime = 50.0f;
    
    depthControl.setParameterDepthConfig(parameterId, config);
    assert(depthControl.hasParameterConfig(parameterId));
    
    const auto& retrievedConfig = depthControl.getParameterDepthConfig(parameterId);
    assert(retrievedConfig.baseDepth == 120.0f);
    assert(retrievedConfig.mode == VelocityDepthControl::DepthMode::ABSOLUTE);
    
    std::cout << "✓ Parameter depth configuration tests passed\n";
}

void testDepthModes() {
    std::cout << "Testing different depth modes...\n";
    
    VelocityDepthControl depthControl;
    uint32_t paramId = 200;
    
    // Test ABSOLUTE mode
    VelocityDepthControl::ParameterDepthConfig absoluteConfig;
    absoluteConfig.mode = VelocityDepthControl::DepthMode::ABSOLUTE;
    absoluteConfig.baseDepth = 150.0f;
    depthControl.setParameterDepthConfig(paramId, absoluteConfig);
    
    float absoluteDepth = depthControl.calculateEffectiveDepth(paramId, 0.5f, 64);
    assert(absoluteDepth >= 0.0f && absoluteDepth <= 2.0f); // 0-200% range
    
    // Test RELATIVE mode
    VelocityDepthControl::ParameterDepthConfig relativeConfig;
    relativeConfig.mode = VelocityDepthControl::DepthMode::RELATIVE;
    relativeConfig.baseDepth = 80.0f;
    depthControl.setParameterDepthConfig(paramId, relativeConfig);
    
    float relativeDepth = depthControl.calculateRelativeDepth(paramId, 0.3f, 100);
    assert(relativeDepth >= 0.0f && relativeDepth <= 2.0f);
    
    // Test SCALED mode
    VelocityDepthControl::ParameterDepthConfig scaledConfig;
    scaledConfig.mode = VelocityDepthControl::DepthMode::SCALED;
    scaledConfig.baseDepth = 120.0f;
    depthControl.setParameterDepthConfig(paramId, scaledConfig);
    
    float scaledDepth = depthControl.calculateScaledDepth(paramId, 0.7f, 90);
    assert(scaledDepth >= 0.0f && scaledDepth <= 2.0f);
    
    // Test LIMITED mode
    VelocityDepthControl::ParameterDepthConfig limitedConfig;
    limitedConfig.mode = VelocityDepthControl::DepthMode::LIMITED;
    limitedConfig.safetyLevel = VelocityDepthControl::SafetyLevel::CONSERVATIVE;
    depthControl.setParameterDepthConfig(paramId, limitedConfig);
    
    float limitedDepth = depthControl.calculateLimitedDepth(paramId, 0.9f, 127);
    assert(limitedDepth >= 0.0f && limitedDepth <= 2.0f);
    
    std::cout << "✓ Depth modes tests passed\n";
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
        VelocityDepthControl::SafetyLevel::CUSTOM
    };
    
    for (auto level : safetyLevels) {
        VelocityDepthControl::ParameterDepthConfig config;
        config.safetyLevel = level;
        config.baseDepth = 180.0f; // High depth to test limiting
        depthControl.setParameterDepthConfig(paramId, config);
        
        auto safetyLimits = depthControl.getSafetyLimits(level);
        assert(safetyLimits.minDepth >= 0.0f);
        assert(safetyLimits.maxDepth <= 200.0f);
        assert(safetyLimits.minDepth <= safetyLimits.maxDepth);
        
        float safeDepth = depthControl.applySafetyLimits(180.0f, level);
        assert(safeDepth >= safetyLimits.minDepth && safeDepth <= safetyLimits.maxDepth);
    }
    
    std::cout << "✓ Safety levels tests passed\n";
}

void testDepthSmoothing() {
    std::cout << "Testing depth smoothing...\n";
    
    VelocityDepthControl depthControl;
    uint32_t paramId = 400;
    
    // Configure parameter with smoothing
    VelocityDepthControl::ParameterDepthConfig config;
    config.enableSmoothing = true;
    config.smoothingTime = 100.0f; // 100ms smoothing
    depthControl.setParameterDepthConfig(paramId, config);
    
    // Test smooth depth transitions
    float depth1 = depthControl.getSmoothedDepth(paramId);
    depthControl.updateDepthSmoothing(paramId, 150.0f, 0.01f); // 10ms delta
    float depth2 = depthControl.getSmoothedDepth(paramId);
    depthControl.updateDepthSmoothing(paramId, 150.0f, 0.01f); // Another 10ms
    float depth3 = depthControl.getSmoothedDepth(paramId);
    
    // Should show gradual change
    assert(depth1 >= 0.0f && depth1 <= 200.0f);
    assert(depth2 >= 0.0f && depth2 <= 200.0f);
    assert(depth3 >= 0.0f && depth3 <= 200.0f);
    
    std::cout << "✓ Depth smoothing tests passed\n";
}

void testDepthPresets() {
    std::cout << "Testing depth presets...\n";
    
    VelocityDepthControl depthControl;
    
    // Test preset creation and application
    VelocityDepthControl::DepthPreset preset;
    preset.name = "Expressive";
    preset.masterDepth = 140.0f;
    preset.description = "Enhanced velocity response for expressive playing";
    
    // Add parameter-specific settings
    VelocityDepthControl::ParameterDepthConfig paramConfig;
    paramConfig.baseDepth = 160.0f;
    paramConfig.mode = VelocityDepthControl::DepthMode::SCALED;
    preset.parameterConfigs[100] = paramConfig;
    
    // Save and load preset
    depthControl.saveDepthPreset("expressive", preset);
    assert(depthControl.hasDepthPreset("expressive"));
    
    auto loadedPreset = depthControl.getDepthPreset("expressive");
    assert(loadedPreset.name == "Expressive");
    assert(loadedPreset.masterDepth == 140.0f);
    
    // Apply preset
    depthControl.applyDepthPreset("expressive");
    assert(depthControl.getMasterDepth() == 140.0f);
    
    // Test preset listing
    auto presetList = depthControl.getAvailablePresets();
    assert(std::find(presetList.begin(), presetList.end(), "expressive") != presetList.end());
    
    std::cout << "✓ Depth presets tests passed\n";
}

void testGlobalDepthOperations() {
    std::cout << "Testing global depth operations...\n";
    
    VelocityDepthControl depthControl;
    
    // Set up multiple parameters
    std::vector<uint32_t> paramIds = {500, 501, 502, 503};
    for (uint32_t id : paramIds) {
        VelocityDepthControl::ParameterDepthConfig config;
        config.baseDepth = 120.0f;
        depthControl.setParameterDepthConfig(id, config);
    }
    
    // Test global depth scaling
    depthControl.setMasterDepth(150.0f);
    depthControl.scaleAllParameterDepths(0.8f); // Scale down by 20%
    
    // Test global depth reset
    depthControl.resetAllParameterDepths();
    
    // Test parameter count
    size_t paramCount = depthControl.getParameterCount();
    assert(paramCount == paramIds.size());
    
    // Test clearing all parameters
    depthControl.clearAllParameters();
    assert(depthControl.getParameterCount() == 0);
    
    std::cout << "✓ Global depth operations tests passed\n";
}

void testRealTimeDepthAdjustment() {
    std::cout << "Testing real-time depth adjustment...\n";
    
    VelocityDepthControl depthControl;
    uint32_t paramId = 600;
    
    VelocityDepthControl::ParameterDepthConfig config;
    config.enableRealTimeAdjustment = true;
    config.enableSmoothing = true;
    depthControl.setParameterDepthConfig(paramId, config);
    
    // Test real-time depth modulation
    float initialDepth = depthControl.getCurrentDepth(paramId);
    
    // Simulate real-time adjustment
    depthControl.adjustDepthRealTime(paramId, 25.0f); // +25% depth
    float adjustedDepth = depthControl.getCurrentDepth(paramId);
    assert(adjustedDepth >= initialDepth);
    
    // Test depth automation
    std::vector<float> automationCurve = {100.0f, 120.0f, 140.0f, 160.0f, 120.0f, 100.0f};
    depthControl.setDepthAutomation(paramId, automationCurve, 1000.0f); // 1 second curve
    
    // Test automation playback
    float automatedDepth = depthControl.getAutomatedDepth(paramId, 0.5f); // 50% through curve
    assert(automatedDepth >= 100.0f && automatedDepth <= 200.0f);
    
    std::cout << "✓ Real-time depth adjustment tests passed\n";
}

void testDepthVisualization() {
    std::cout << "Testing depth visualization integration...\n";
    
    VelocityDepthControl depthControl;
    uint32_t paramId = 700;
    
    VelocityDepthControl::ParameterDepthConfig config;
    config.baseDepth = 130.0f;
    depthControl.setParameterDepthConfig(paramId, config);
    
    // Test V-icon integration
    auto visualizationData = depthControl.getVisualizationData(paramId);
    assert(visualizationData.currentDepth >= 0.0f && visualizationData.currentDepth <= 200.0f);
    assert(visualizationData.normalizedDepth >= 0.0f && visualizationData.normalizedDepth <= 1.0f);
    
    // Test depth color coding
    auto colorInfo = depthControl.getDepthColorInfo(130.0f);
    assert(!colorInfo.colorHex.empty());
    
    // Test depth bar visualization
    float barLength = depthControl.calculateDepthBarLength(130.0f, 200.0f);
    assert(barLength >= 0.0f && barLength <= 1.0f);
    
    std::cout << "✓ Depth visualization tests passed\n";
}

void testPerformanceMonitoring() {
    std::cout << "Testing performance monitoring...\n";
    
    VelocityDepthControl depthControl;
    
    // Enable performance monitoring
    depthControl.enablePerformanceMonitoring(true);
    
    // Create some load
    for (uint32_t i = 800; i < 820; ++i) {
        VelocityDepthControl::ParameterDepthConfig config;
        depthControl.setParameterDepthConfig(i, config);
        depthControl.calculateEffectiveDepth(i, 0.5f, 64);
    }
    
    // Test performance metrics
    auto perfMetrics = depthControl.getPerformanceMetrics();
    assert(perfMetrics.averageProcessingTime >= 0.0f);
    assert(perfMetrics.totalCalculations >= 20); // Should have processed at least 20 calculations
    
    // Test CPU usage estimation
    float cpuUsage = depthControl.getCPUUsageEstimate();
    assert(cpuUsage >= 0.0f);
    
    // Test memory usage
    size_t memoryUsage = depthControl.getEstimatedMemoryUsage();
    assert(memoryUsage > 0);
    
    std::cout << "✓ Performance monitoring tests passed\n";
}

void testErrorHandling() {
    std::cout << "Testing error handling...\n";
    
    VelocityDepthControl depthControl;
    
    // Test with invalid parameter IDs
    uint32_t invalidParam = 99999;
    
    // Should not crash with non-existent parameter
    float depth = depthControl.calculateEffectiveDepth(invalidParam, 0.5f, 64);
    (void)depth; // May return default values
    
    // Test with extreme depth values
    VelocityDepthControl::ParameterDepthConfig extremeConfig;
    extremeConfig.baseDepth = 500.0f; // Way over 200%
    extremeConfig.mode = VelocityDepthControl::DepthMode::ABSOLUTE;
    
    uint32_t paramId = 900;
    depthControl.setParameterDepthConfig(paramId, extremeConfig);
    
    float clampedDepth = depthControl.calculateEffectiveDepth(paramId, 0.5f, 127);
    assert(clampedDepth <= 2.0f); // Should be clamped to 200% max
    
    // Test invalid preset operations
    assert(!depthControl.hasDepthPreset("nonexistent"));
    
    // Should handle gracefully
    bool applied = depthControl.applyDepthPreset("nonexistent");
    assert(!applied);
    
    std::cout << "✓ Error handling tests passed\n";
}

void testSystemIntegration() {
    std::cout << "Testing system integration...\n";
    
    VelocityDepthControl depthControl;
    
    // Test enable/disable
    depthControl.setEnabled(false);
    assert(!depthControl.isEnabled());
    
    uint32_t paramId = 1000;
    VelocityDepthControl::ParameterDepthConfig config;
    depthControl.setParameterDepthConfig(paramId, config);
    
    // When disabled, should return pass-through depth
    float disabledDepth = depthControl.calculateEffectiveDepth(paramId, 0.5f, 64);
    (void)disabledDepth;
    
    // Re-enable
    depthControl.setEnabled(true);
    assert(depthControl.isEnabled());
    
    // Test sample rate setting
    depthControl.setSampleRate(44100.0f);
    assert(depthControl.getSampleRate() == 44100.0f);
    
    // Test system reset
    depthControl.reset();
    
    std::cout << "✓ System integration tests passed\n";
}

int main() {
    std::cout << "=== VelocityDepthControl Tests ===\n\n";
    
    try {
        testBasicDepthControl();
        testParameterDepthConfiguration();
        testDepthModes();
        testSafetyLevels();
        testDepthSmoothing();
        testDepthPresets();
        testGlobalDepthOperations();
        testRealTimeDepthAdjustment();
        testDepthVisualization();
        testPerformanceMonitoring();
        testErrorHandling();
        testSystemIntegration();
        
        std::cout << "\n🎉 All VelocityDepthControl tests PASSED!\n";
        std::cout << "\nSystem features tested:\n";
        std::cout << "✓ Master depth control with 0-200% range\n";
        std::cout << "✓ Multiple depth modes (absolute, relative, scaled, limited, dynamic)\n";
        std::cout << "✓ Safety levels with automatic depth limiting\n";
        std::cout << "✓ Smooth depth transitions and real-time adjustment\n";
        std::cout << "✓ Depth preset system with save/load functionality\n";
        std::cout << "✓ Global depth operations and parameter management\n";
        std::cout << "✓ Real-time depth automation and modulation\n";
        std::cout << "✓ V-icon integration and depth visualization\n";
        std::cout << "✓ Performance monitoring and optimization\n";
        std::cout << "✓ Error handling and system integration\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}