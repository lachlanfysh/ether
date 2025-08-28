#include <iostream>
#include <cmath>
#include "control/modulation/VelocityDepthControl.h"
#include "interface/ui/VelocityModulationUI.h"

int main() {
    std::cout << "EtherSynth Velocity Depth Control Test\n";
    std::cout << "=======================================\n";
    
    bool allTestsPassed = true;
    
    // Test velocity depth control system creation
    std::cout << "Testing VelocityDepthControl creation... ";
    try {
        VelocityDepthControl depthControl;
        
        if (depthControl.isEnabled() && 
            std::abs(depthControl.getMasterDepth() - 1.0f) < 0.01f &&
            depthControl.getConfiguredParameterCount() == 0) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization issue)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test master depth control
    std::cout << "Testing master depth control... ";
    try {
        VelocityDepthControl depthControl;
        
        // Test setting master depth
        depthControl.setMasterDepth(1.5f);  // 150%
        float masterDepth = depthControl.getMasterDepth();
        
        // Test clamping
        depthControl.setMasterDepth(3.0f);  // Should be clamped to 2.0
        float clampedDepth = depthControl.getMasterDepth();
        
        if (std::abs(masterDepth - 1.5f) < 0.01f &&
            std::abs(clampedDepth - 2.0f) < 0.01f) {
            std::cout << "PASS (depths: " << masterDepth << ", " << clampedDepth << ")\n";
        } else {
            std::cout << "FAIL (master depth control not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test parameter depth configuration
    std::cout << "Testing parameter depth configuration... ";
    try {
        VelocityDepthControl depthControl;
        const uint32_t PARAM_ID = 1001;
        
        VelocityDepthControl::ParameterDepthConfig config;
        config.baseDepth = 1.3f;      // 130%
        config.maxAllowedDepth = 1.8f; // 180% max
        config.minAllowedDepth = 0.2f; // 20% min
        config.depthMode = VelocityDepthControl::DepthMode::ABSOLUTE;
        config.safetyLevel = VelocityDepthControl::SafetyLevel::MODERATE;
        
        depthControl.setParameterDepthConfig(PARAM_ID, config);
        
        // Check configuration was applied
        const auto& storedConfig = depthControl.getParameterDepthConfig(PARAM_ID);
        float baseDepth = depthControl.getParameterBaseDepth(PARAM_ID);
        bool hasConfig = depthControl.hasParameterDepthConfig(PARAM_ID);
        
        if (std::abs(storedConfig.baseDepth - 1.3f) < 0.01f &&
            std::abs(baseDepth - 1.3f) < 0.01f &&
            hasConfig &&
            storedConfig.depthMode == VelocityDepthControl::DepthMode::ABSOLUTE) {
            std::cout << "PASS (config applied correctly)\n";
        } else {
            std::cout << "FAIL (parameter configuration not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test effective depth calculation
    std::cout << "Testing effective depth calculation... ";
    try {
        VelocityDepthControl depthControl;
        const uint32_t PARAM_ID = 2001;
        
        // Configure parameter
        VelocityDepthControl::ParameterDepthConfig config;
        config.baseDepth = 1.2f;
        config.depthMode = VelocityDepthControl::DepthMode::ABSOLUTE;
        config.linkToMasterDepth = false;  // Don't link to master for predictable results
        config.enableDepthModulation = false;  // No smoothing for predictable results
        
        depthControl.setParameterDepthConfig(PARAM_ID, config);
        
        // Test depth calculation
        auto result1 = depthControl.calculateEffectiveDepth(PARAM_ID, 1.0f);
        auto result2 = depthControl.calculateEffectiveDepth(PARAM_ID, 1.5f);
        auto result3 = depthControl.calculateEffectiveDepth(PARAM_ID, 0.5f);
        
        // With ABSOLUTE mode, effective depth should match requested (clamped to base config)
        if (result1.effectiveDepth > 0.0f &&
            result2.effectiveDepth >= result1.effectiveDepth &&
            result3.effectiveDepth <= result1.effectiveDepth &&
            !result1.wasLimited) {  // Should not be limited at moderate levels
            std::cout << "PASS (effective depths: " << result3.effectiveDepth 
                      << " ≤ " << result1.effectiveDepth << " ≤ " << result2.effectiveDepth << ")\n";
        } else {
            std::cout << "FAIL (effective depth calculation incorrect)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test safety limiting
    std::cout << "Testing safety limiting... ";
    try {
        VelocityDepthControl depthControl;
        const uint32_t PARAM_ID = 3001;
        
        // Configure with conservative safety
        VelocityDepthControl::ParameterDepthConfig config;
        config.baseDepth = 2.0f;  // High depth
        config.safetyLevel = VelocityDepthControl::SafetyLevel::CONSERVATIVE;  // Should limit to ~80%
        
        depthControl.setParameterDepthConfig(PARAM_ID, config);
        
        // Test safety limiting
        auto result = depthControl.calculateEffectiveDepth(PARAM_ID, 2.0f);
        bool isSafe = depthControl.isDepthSafe(PARAM_ID, 0.5f);  // Low depth should be safe
        bool isUnsafe = !depthControl.isDepthSafe(PARAM_ID, 1.5f);  // High depth should be unsafe with conservative
        
        if (result.wasLimited &&
            result.effectiveDepth < 1.0f &&  // Should be limited to conservative level
            isSafe && isUnsafe) {
            std::cout << "PASS (safety limiting working, limited to: " << result.effectiveDepth << ")\n";
        } else {
            std::cout << "FAIL (safety limiting not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test master depth linking
    std::cout << "Testing master depth linking... ";
    try {
        VelocityDepthControl depthControl;
        const uint32_t PARAM1 = 4001;
        const uint32_t PARAM2 = 4002;
        
        // Configure linked parameter
        VelocityDepthControl::ParameterDepthConfig configLinked;
        configLinked.baseDepth = 1.0f;
        configLinked.linkToMasterDepth = true;
        configLinked.masterDepthScale = 1.5f;  // 1.5x master scaling
        
        // Configure unlinked parameter
        VelocityDepthControl::ParameterDepthConfig configUnlinked;
        configUnlinked.baseDepth = 1.0f;
        configUnlinked.linkToMasterDepth = false;
        
        depthControl.setParameterDepthConfig(PARAM1, configLinked);
        depthControl.setParameterDepthConfig(PARAM2, configUnlinked);
        
        // Change master depth
        depthControl.setMasterDepth(0.5f);  // 50%
        
        // Check effective depths
        float linkedDepth = depthControl.getEffectiveParameterDepth(PARAM1);
        float unlinkedDepth = depthControl.getEffectiveParameterDepth(PARAM2);
        
        // Linked should be affected: 0.5 * 1.5 = 0.75
        // Unlinked should remain at 1.0
        if (std::abs(linkedDepth - 0.75f) < 0.1f &&  // Allow some tolerance for processing
            std::abs(unlinkedDepth - 1.0f) < 0.01f) {
            std::cout << "PASS (linked: " << linkedDepth << ", unlinked: " << unlinkedDepth << ")\n";
        } else {
            std::cout << "FAIL (master depth linking not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test real-time depth modulation
    std::cout << "Testing real-time depth modulation... ";
    try {
        VelocityDepthControl depthControl;
        const uint32_t PARAM_ID = 5001;
        
        VelocityDepthControl::ParameterDepthConfig config;
        config.baseDepth = 1.0f;
        config.enableDepthModulation = true;
        
        depthControl.setParameterDepthConfig(PARAM_ID, config);
        
        // Set real-time depth modulation
        depthControl.setRealTimeDepthModulation(PARAM_ID, 0.3f);  // +30%
        float rtMod = depthControl.getRealTimeDepthModulation(PARAM_ID);
        
        // Calculate depth with RT modulation
        auto result = depthControl.calculateEffectiveDepth(PARAM_ID, 1.0f);
        
        if (std::abs(rtMod - 0.3f) < 0.01f &&
            result.effectiveDepth > 1.0f) {  // Should be higher due to RT modulation
            std::cout << "PASS (RT modulation: " << rtMod << ", effective depth: " << result.effectiveDepth << ")\n";
        } else {
            std::cout << "FAIL (real-time depth modulation not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test batch operations
    std::cout << "Testing batch operations... ";
    try {
        VelocityDepthControl depthControl;
        
        // Add multiple parameters
        for (uint32_t i = 6001; i <= 6005; i++) {
            VelocityDepthControl::ParameterDepthConfig config;
            config.baseDepth = 1.0f;
            depthControl.setParameterDepthConfig(i, config);
        }
        
        size_t initialCount = depthControl.getConfiguredParameterCount();
        
        // Set all parameters to new depth
        depthControl.setAllParametersDepth(1.5f);
        
        // Check that all parameters have the new depth
        bool allUpdated = true;
        for (uint32_t i = 6001; i <= 6005; i++) {
            float depth = depthControl.getParameterBaseDepth(i);
            if (std::abs(depth - 1.5f) > 0.01f) {
                allUpdated = false;
                break;
            }
        }
        
        if (initialCount == 5 && allUpdated) {
            std::cout << "PASS (batch operations working on " << initialCount << " parameters)\n";
        } else {
            std::cout << "FAIL (batch operations not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test emergency depth limiting
    std::cout << "Testing emergency depth limiting... ";
    try {
        VelocityDepthControl depthControl;
        
        // Set up parameters with high depths
        for (uint32_t i = 7001; i <= 7003; i++) {
            VelocityDepthControl::ParameterDepthConfig config;
            config.baseDepth = 1.8f;  // High depth
            depthControl.setParameterDepthConfig(i, config);
        }
        
        // Trigger emergency limiting
        depthControl.emergencyDepthLimit(1.0f);  // Limit to 100%
        
        // Check that all parameters were limited
        bool allLimited = true;
        for (uint32_t i = 7001; i <= 7003; i++) {
            float depth = depthControl.getParameterBaseDepth(i);
            if (depth > 1.01f) {  // Allow small tolerance
                allLimited = false;
                break;
            }
        }
        
        if (allLimited) {
            std::cout << "PASS (emergency limiting applied to all parameters)\n";
        } else {
            std::cout << "FAIL (emergency depth limiting not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test system statistics
    std::cout << "Testing system statistics... ";
    try {
        VelocityDepthControl depthControl;
        
        // Add parameters with different depths
        VelocityDepthControl::ParameterDepthConfig config1;
        config1.baseDepth = 0.5f;
        depthControl.setParameterDepthConfig(8001, config1);
        
        VelocityDepthControl::ParameterDepthConfig config2;
        config2.baseDepth = 1.0f;
        depthControl.setParameterDepthConfig(8002, config2);
        
        VelocityDepthControl::ParameterDepthConfig config3;
        config3.baseDepth = 1.5f;
        depthControl.setParameterDepthConfig(8003, config3);
        
        // Test statistics
        size_t totalCount = depthControl.getConfiguredParameterCount();
        float avgDepth = depthControl.getAverageDepth();
        size_t overThreshold = depthControl.getParametersOverDepth(1.2f);
        auto excessiveParams = depthControl.getParametersWithExcessiveDepth(1.4f);
        float systemLoad = depthControl.getSystemDepthLoad();
        
        // Expected: 3 params, avg = 1.0, 1 over 1.2, 1 excessive over 1.4
        if (totalCount == 3 &&
            std::abs(avgDepth - 1.0f) < 0.01f &&
            overThreshold == 1 &&
            excessiveParams.size() == 1 &&
            systemLoad >= 0.0f) {
            std::cout << "PASS (stats: " << totalCount << " params, avg: " << avgDepth 
                      << ", over threshold: " << overThreshold << ", load: " << systemLoad << ")\n";
        } else {
            std::cout << "FAIL (system statistics not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL VELOCITY DEPTH CONTROL TESTS PASSED!\n";
        std::cout << "Unified velocity modulation depth management (0-200%) is working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}