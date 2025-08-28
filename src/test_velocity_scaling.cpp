#include <iostream>
#include <cmath>
#include "control/modulation/VelocityParameterScaling.h"

int main() {
    std::cout << "EtherSynth Velocity Parameter Scaling Test\n";
    std::cout << "===========================================\n";
    
    bool allTestsPassed = true;
    
    // Test velocity parameter scaling system creation
    std::cout << "Testing VelocityParameterScaling creation... ";
    try {
        VelocityParameterScaling scaling;
        
        if (scaling.isEnabled() && scaling.getConfiguredParameterCount() == 0) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization issue)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test parameter category configuration
    std::cout << "Testing parameter category configuration... ";
    try {
        VelocityParameterScaling scaling;
        const uint32_t CUTOFF_PARAM = 1001;
        const uint32_t RESONANCE_PARAM = 1002;
        
        // Set categories which should apply default scaling
        scaling.setParameterCategory(CUTOFF_PARAM, VelocityParameterScaling::ParameterCategory::FILTER_CUTOFF);
        scaling.setParameterCategory(RESONANCE_PARAM, VelocityParameterScaling::ParameterCategory::FILTER_RESONANCE);
        
        // Check that categories were set and defaults applied
        auto cutoffCategory = scaling.getParameterCategory(CUTOFF_PARAM);
        auto resonanceCategory = scaling.getParameterCategory(RESONANCE_PARAM);
        float cutoffScale = scaling.getParameterVelocityScale(CUTOFF_PARAM);
        float resonanceScale = scaling.getParameterVelocityScale(RESONANCE_PARAM);
        
        if (cutoffCategory == VelocityParameterScaling::ParameterCategory::FILTER_CUTOFF &&
            resonanceCategory == VelocityParameterScaling::ParameterCategory::FILTER_RESONANCE &&
            scaling.hasParameterScaling(CUTOFF_PARAM) &&
            scaling.hasParameterScaling(RESONANCE_PARAM) &&
            cutoffScale != resonanceScale) {  // Different defaults for different categories
            std::cout << "PASS (cutoff scale: " << cutoffScale << ", resonance scale: " << resonanceScale << ")\n";
        } else {
            std::cout << "FAIL (category configuration not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test velocity scaling calculation
    std::cout << "Testing velocity scaling calculation... ";
    try {
        VelocityParameterScaling scaling;
        const uint32_t PARAM_ID = 2001;
        
        VelocityParameterScaling::ParameterScalingConfig config;
        config.velocityScale = 2.0f;  // Double sensitivity
        config.polarity = VelocityModulationUI::ModulationPolarity::POSITIVE;
        config.deadzone = 0.0f;  // No deadzone
        config.threshold = 0.0f;  // No threshold
        
        scaling.setParameterScaling(PARAM_ID, config);
        
        // Test different velocity values
        auto result25 = scaling.calculateParameterScaling(PARAM_ID, 0.25f, 0.5f);
        auto result50 = scaling.calculateParameterScaling(PARAM_ID, 0.50f, 0.5f);
        auto result75 = scaling.calculateParameterScaling(PARAM_ID, 0.75f, 0.5f);
        
        // Expected: baseValue + (velocity * scale)
        // result25: 0.5 + (0.25 * 2.0) = 1.0
        // result50: 0.5 + (0.50 * 2.0) = 1.5 → clamped to 1.0
        // result75: 0.5 + (0.75 * 2.0) = 2.0 → clamped to 1.0
        
        if (std::abs(result25.finalValue - 1.0f) < 0.01f &&
            std::abs(result50.finalValue - 1.0f) < 0.01f &&  // Clamped
            std::abs(result75.finalValue - 1.0f) < 0.01f &&  // Clamped
            result25.scaledVelocity < result50.scaledVelocity &&
            result50.scaledVelocity < result75.scaledVelocity &&
            !result25.inDeadzone && result25.thresholdPassed) {
            std::cout << "PASS (scaled velocities: " << result25.scaledVelocity 
                      << ", " << result50.scaledVelocity << ", " << result75.scaledVelocity << ")\n";
        } else {
            std::cout << "FAIL (velocity scaling calculation incorrect)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test polarity configuration
    std::cout << "Testing polarity configuration... ";
    try {
        VelocityParameterScaling scaling;
        const uint32_t PARAM_POS = 3001;
        const uint32_t PARAM_NEG = 3002;
        const uint32_t PARAM_BI = 3003;
        
        // Positive polarity
        VelocityParameterScaling::ParameterScalingConfig configPos;
        configPos.velocityScale = 1.0f;
        configPos.polarity = VelocityModulationUI::ModulationPolarity::POSITIVE;
        
        // Negative polarity
        VelocityParameterScaling::ParameterScalingConfig configNeg;
        configNeg.velocityScale = 1.0f;
        configNeg.polarity = VelocityModulationUI::ModulationPolarity::NEGATIVE;
        
        // Bipolar polarity
        VelocityParameterScaling::ParameterScalingConfig configBi;
        configBi.velocityScale = 1.0f;
        configBi.polarity = VelocityModulationUI::ModulationPolarity::BIPOLAR;
        configBi.centerPoint = 0.5f;
        
        scaling.setParameterScaling(PARAM_POS, configPos);
        scaling.setParameterScaling(PARAM_NEG, configNeg);
        scaling.setParameterScaling(PARAM_BI, configBi);
        
        float baseValue = 0.5f;
        float velocity = 0.8f;
        
        auto resultPos = scaling.calculateParameterScaling(PARAM_POS, velocity, baseValue);
        auto resultNeg = scaling.calculateParameterScaling(PARAM_NEG, velocity, baseValue);
        auto resultBi = scaling.calculateParameterScaling(PARAM_BI, velocity, baseValue);
        
        // Positive should increase, negative should decrease, bipolar should be different
        if (resultPos.finalValue > baseValue &&
            resultNeg.finalValue < baseValue &&
            resultBi.finalValue != baseValue) {
            std::cout << "PASS (pos: " << resultPos.finalValue << ", neg: " 
                      << resultNeg.finalValue << ", bi: " << resultBi.finalValue << ")\n";
        } else {
            std::cout << "FAIL (polarity configuration not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test velocity range mapping
    std::cout << "Testing velocity range mapping... ";
    try {
        VelocityParameterScaling scaling;
        const uint32_t PARAM_ID = 4001;
        
        VelocityParameterScaling::ParameterScalingConfig config;
        config.velocityScale = 1.0f;
        config.polarity = VelocityModulationUI::ModulationPolarity::POSITIVE;
        
        // Map input range 0.2-0.8 to output range 0.0-1.0
        config.velocityRange = VelocityParameterScaling::VelocityRange(0.2f, 0.8f, 0.0f, 1.0f);
        
        scaling.setParameterScaling(PARAM_ID, config);
        
        // Test range mapping
        auto result10 = scaling.calculateParameterScaling(PARAM_ID, 0.1f, 0.0f);  // Below input range
        auto result50 = scaling.calculateParameterScaling(PARAM_ID, 0.5f, 0.0f);  // Mid input range
        auto result90 = scaling.calculateParameterScaling(PARAM_ID, 0.9f, 0.0f);  // Above input range
        
        // Should map: 0.1→0.2 (clamped), 0.5→0.5 (middle), 0.9→0.8 (clamped)
        // Then scale to 0-1 output range and add to base (0.0)
        if (result10.finalValue < result50.finalValue &&
            result50.finalValue < result90.finalValue &&
            result50.finalValue > 0.3f && result50.finalValue < 0.7f) {  // Should be roughly in middle
            std::cout << "PASS (range mapping: " << result10.finalValue 
                      << " < " << result50.finalValue << " < " << result90.finalValue << ")\n";
        } else {
            std::cout << "FAIL (velocity range mapping not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test deadzone and threshold
    std::cout << "Testing deadzone and threshold... ";
    try {
        VelocityParameterScaling scaling;
        const uint32_t PARAM_ID = 5001;
        
        VelocityParameterScaling::ParameterScalingConfig config;
        config.velocityScale = 1.0f;
        config.polarity = VelocityModulationUI::ModulationPolarity::POSITIVE;
        config.deadzone = 0.1f;    // 10% deadzone
        config.threshold = 0.2f;   // 20% threshold
        config.hysteresis = 0.05f; // 5% hysteresis
        
        scaling.setParameterScaling(PARAM_ID, config);
        
        float baseValue = 0.5f;
        
        // Test velocities in different zones
        auto resultDeadzone = scaling.calculateParameterScaling(PARAM_ID, 0.05f, baseValue);  // In deadzone
        auto resultBelowThreshold = scaling.calculateParameterScaling(PARAM_ID, 0.15f, baseValue);  // Below threshold
        auto resultAboveThreshold = scaling.calculateParameterScaling(PARAM_ID, 0.3f, baseValue);   // Above threshold
        
        if (resultDeadzone.inDeadzone &&
            resultDeadzone.finalValue == baseValue &&  // No modulation in deadzone
            !resultBelowThreshold.thresholdPassed &&
            resultBelowThreshold.finalValue == baseValue &&  // No modulation below threshold
            resultAboveThreshold.thresholdPassed &&
            resultAboveThreshold.finalValue > baseValue) {   // Modulation above threshold
            std::cout << "PASS (deadzone/threshold working correctly)\n";
        } else {
            std::cout << "FAIL (deadzone/threshold not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test velocity compression
    std::cout << "Testing velocity compression... ";
    try {
        VelocityParameterScaling scaling;
        const uint32_t PARAM_ID = 6001;
        
        VelocityParameterScaling::ParameterScalingConfig config;
        config.velocityScale = 1.0f;
        config.polarity = VelocityModulationUI::ModulationPolarity::POSITIVE;
        config.compressionRatio = 2.0f;  // 2:1 compression
        config.softKnee = 0.1f;
        
        scaling.setParameterScaling(PARAM_ID, config);
        
        // Test compression effect
        auto resultLow = scaling.calculateParameterScaling(PARAM_ID, 0.3f, 0.0f);   // Below threshold
        auto resultHigh = scaling.calculateParameterScaling(PARAM_ID, 0.9f, 0.0f);  // Above threshold
        
        // With compression, high velocities should be reduced more than low velocities
        float lowRatio = resultLow.finalValue / 0.3f;
        float highRatio = resultHigh.finalValue / 0.9f;
        
        if (resultHigh.compressionAmount > 0.0f &&  // Compression was applied
            highRatio < lowRatio) {                 // High velocities compressed more
            std::cout << "PASS (compression working: compression amount = " 
                      << resultHigh.compressionAmount << ")\n";
        } else {
            std::cout << "FAIL (velocity compression not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test scaling presets
    std::cout << "Testing scaling presets... ";
    try {
        VelocityParameterScaling scaling;
        const uint32_t PARAM_ID = 7001;
        
        // Get available presets
        auto presets = scaling.getAvailablePresets();
        
        if (!presets.empty()) {
            // Apply first preset
            scaling.applyScalingPreset(PARAM_ID, presets[0].name);
            
            // Check that parameter now has scaling configuration
            bool hasScaling = scaling.hasParameterScaling(PARAM_ID);
            float scale = scaling.getParameterVelocityScale(PARAM_ID);
            
            if (hasScaling && scale > 0.0f) {
                std::cout << "PASS (preset '" << presets[0].name << "' applied, scale: " << scale << ")\n";
            } else {
                std::cout << "FAIL (preset application not working)\n";
                allTestsPassed = false;
            }
        } else {
            std::cout << "FAIL (no presets available)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test batch operations
    std::cout << "Testing batch operations... ";
    try {
        VelocityParameterScaling scaling;
        
        // Add multiple parameters
        for (uint32_t i = 8001; i <= 8005; i++) {
            scaling.setParameterCategory(i, VelocityParameterScaling::ParameterCategory::FILTER_CUTOFF);
        }
        
        size_t initialCount = scaling.getConfiguredParameterCount();
        
        // Apply batch scaling
        scaling.setAllParametersScale(1.5f);
        
        // Check that all parameters have the new scale
        bool allUpdated = true;
        for (uint32_t i = 8001; i <= 8005; i++) {
            float scale = scaling.getParameterVelocityScale(i);
            if (std::abs(scale - 1.5f) > 0.01f) {
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
    
    // Test category statistics
    std::cout << "Testing category statistics... ";
    try {
        VelocityParameterScaling scaling;
        
        // Add parameters in different categories
        scaling.setParameterCategory(9001, VelocityParameterScaling::ParameterCategory::FILTER_CUTOFF);
        scaling.setParameterCategory(9002, VelocityParameterScaling::ParameterCategory::FILTER_CUTOFF);
        scaling.setParameterCategory(9003, VelocityParameterScaling::ParameterCategory::FILTER_RESONANCE);
        scaling.setParameterCategory(9004, VelocityParameterScaling::ParameterCategory::VOLUME);
        
        auto categoryCounts = scaling.getCategoryCounts();
        auto cutoffParams = scaling.getParametersInCategory(VelocityParameterScaling::ParameterCategory::FILTER_CUTOFF);
        
        if (categoryCounts[VelocityParameterScaling::ParameterCategory::FILTER_CUTOFF] == 2 &&
            categoryCounts[VelocityParameterScaling::ParameterCategory::FILTER_RESONANCE] == 1 &&
            categoryCounts[VelocityParameterScaling::ParameterCategory::VOLUME] == 1 &&
            cutoffParams.size() == 2) {
            std::cout << "PASS (statistics: " << categoryCounts.size() << " categories, " 
                      << cutoffParams.size() << " cutoff params)\n";
        } else {
            std::cout << "FAIL (category statistics not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL VELOCITY PARAMETER SCALING TESTS PASSED!\n";
        std::cout << "Per-parameter velocity scaling and polarity configuration is working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}