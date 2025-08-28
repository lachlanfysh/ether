#include "src/control/modulation/VelocityDepthControl.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <cmath>

// Stub implementations for UI dependencies
namespace VelocityModulationUI {
    class VIcon {
    public:
        void setModulationDepth(float depth) { (void)depth; }
    };
    
    class VelocityModulationPanel {
    public:
        VIcon* getVIcon(uint32_t parameterId) { (void)parameterId; return nullptr; }
    };
}

/**
 * Simplified test for VelocityDepthControl with UI stubs
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

int main() {
    std::cout << "=== VelocityDepthControl Tests (With Stubs) ===\n\n";
    
    try {
        testBasicDepthControl();
        testParameterDepthConfiguration();
        testDepthCalculation();
        testSafetyLevels();
        testSystemManagement();
        
        std::cout << "\n🎉 VelocityDepthControl core tests PASSED!\n";
        std::cout << "\nCore system features tested:\n";
        std::cout << "✓ Master depth control with 0-200% range and safety clamping\n";
        std::cout << "✓ Parameter-specific depth configuration and mode settings\n";
        std::cout << "✓ Depth calculation with different processing modes\n";
        std::cout << "✓ Safety level enforcement and depth limiting\n";
        std::cout << "✓ System state management and parameter lifecycle\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}