#include <iostream>
#include <cmath>
#include "audio/ResonanceAutoRide.h"

int main() {
    std::cout << "EtherSynth Resonance Auto-Ride Test\n";
    std::cout << "===================================\n";
    
    bool allTestsPassed = true;
    
    // Test basic initialization
    std::cout << "Testing Resonance Auto-Ride initialization... ";
    try {
        ResonanceAutoRide autoRide;
        ResonanceAutoRide::Config config;
        
        if (autoRide.initialize(config)) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test invalid configuration rejection
    std::cout << "Testing invalid configuration rejection... ";
    try {
        ResonanceAutoRide autoRide;
        ResonanceAutoRide::Config badConfig;
        badConfig.minCutoffHz = 1000.0f;
        badConfig.maxCutoffHz = 500.0f; // Invalid: min > max
        
        if (!autoRide.initialize(badConfig)) {
            std::cout << "PASS (correctly rejected invalid config)\n";
        } else {
            std::cout << "FAIL (accepted invalid config)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test basic auto-ride functionality
    std::cout << "Testing basic auto-ride functionality... ";
    try {
        ResonanceAutoRide autoRide;
        ResonanceAutoRide::Config config;
        config.autoRideAmount = 1.0f; // Full auto-ride
        config.minCutoffHz = 100.0f;
        config.maxCutoffHz = 10000.0f;
        config.minResonance = 0.1f;
        config.maxResonance = 10.0f;
        
        autoRide.initialize(config);
        
        // Test at low cutoff (should give high resonance)
        float baseResonance = 1.0f;
        float lowCutoffResonance = autoRide.processResonance(200.0f, baseResonance);
        
        // Test at high cutoff (should give lower resonance)
        float highCutoffResonance = autoRide.processResonance(8000.0f, baseResonance);
        
        if (lowCutoffResonance > highCutoffResonance && 
            lowCutoffResonance >= baseResonance &&
            highCutoffResonance >= config.minResonance) {
            std::cout << "PASS (low cutoff: " << lowCutoffResonance << "Q, high cutoff: " << highCutoffResonance << "Q)\n";
        } else {
            std::cout << "FAIL (auto-ride not working: low=" << lowCutoffResonance << ", high=" << highCutoffResonance << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test cutoff opening compensation
    std::cout << "Testing cutoff opening compensation... ";
    try {
        ResonanceAutoRide autoRide;
        ResonanceAutoRide::Config config;
        config.cutoffOpeningAmount = 1.0f; // Full opening
        config.minCutoffHz = 100.0f;
        config.maxCutoffHz = 10000.0f;
        config.minResonance = 0.1f;
        config.maxResonance = 10.0f;
        
        autoRide.initialize(config);
        
        float baseCutoff = 1000.0f;
        
        // Test with low resonance (should not open much)
        float lowResOpened = autoRide.processCutoffOpening(baseCutoff, 1.0f);
        
        // Test with high resonance (should open significantly)
        float highResOpened = autoRide.processCutoffOpening(baseCutoff, 8.0f);
        
        if (highResOpened > lowResOpened && highResOpened > baseCutoff) {
            std::cout << "PASS (low res: " << lowResOpened << "Hz, high res: " << highResOpened << "Hz)\n";
        } else {
            std::cout << "FAIL (cutoff opening not working: low=" << lowResOpened << ", high=" << highResOpened << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test auto-ride amount scaling
    std::cout << "Testing auto-ride amount scaling... ";
    try {
        ResonanceAutoRide autoRide;
        ResonanceAutoRide::Config config;
        config.minCutoffHz = 100.0f;
        config.maxCutoffHz = 10000.0f;
        config.minResonance = 0.1f;
        config.maxResonance = 10.0f;
        
        float baseCutoff = 300.0f; // Low cutoff
        float baseResonance = 1.0f;
        
        // Test with 25% auto-ride
        config.autoRideAmount = 0.25f;
        autoRide.initialize(config);
        float res25 = autoRide.processResonance(baseCutoff, baseResonance);
        
        // Test with 75% auto-ride
        config.autoRideAmount = 0.75f;
        autoRide.initialize(config);
        float res75 = autoRide.processResonance(baseCutoff, baseResonance);
        
        // Test with 0% auto-ride (disabled)
        config.autoRideAmount = 0.0f;
        autoRide.initialize(config);
        float res0 = autoRide.processResonance(baseCutoff, baseResonance);
        
        if (res75 > res25 && res25 > res0 && std::abs(res0 - baseResonance) < 0.1f) {
            std::cout << "PASS (0%: " << res0 << "Q, 25%: " << res25 << "Q, 75%: " << res75 << "Q)\n";
        } else {
            std::cout << "FAIL (scaling not working properly)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test different curve types
    std::cout << "Testing different curve types... ";
    try {
        ResonanceAutoRide autoRide;
        ResonanceAutoRide::Config config;
        config.autoRideAmount = 1.0f;
        config.minCutoffHz = 100.0f;
        config.maxCutoffHz = 10000.0f;
        config.minResonance = 0.1f;
        config.maxResonance = 10.0f;
        
        float testCutoff = 2000.0f; // Mid-range cutoff for better curve differentiation
        float baseResonance = 1.0f;
        
        // Test exponential curve
        config.curveType = ResonanceAutoRide::CurveType::EXPONENTIAL;
        autoRide.initialize(config);
        float expRes = autoRide.processResonance(testCutoff, baseResonance);
        
        // Test linear curve
        config.curveType = ResonanceAutoRide::CurveType::LINEAR;
        autoRide.initialize(config);
        float linRes = autoRide.processResonance(testCutoff, baseResonance);
        
        // Test S-curve
        config.curveType = ResonanceAutoRide::CurveType::S_CURVE;
        autoRide.initialize(config);
        float sCurveRes = autoRide.processResonance(testCutoff, baseResonance);
        
        // Curves should give different results
        bool curvesDiffer = (std::abs(expRes - linRes) > 0.1f || 
                            std::abs(expRes - sCurveRes) > 0.1f ||
                            std::abs(linRes - sCurveRes) > 0.1f);
        
        if (curvesDiffer) {
            std::cout << "PASS (exp: " << expRes << "Q, lin: " << linRes << "Q, s-curve: " << sCurveRes << "Q)\n";
        } else {
            std::cout << "FAIL (curves too similar: exp=" << expRes << ", lin=" << linRes << ", s=" << sCurveRes << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test enable/disable functionality
    std::cout << "Testing enable/disable functionality... ";
    try {
        ResonanceAutoRide autoRide;
        ResonanceAutoRide::Config config;
        config.autoRideAmount = 1.0f;
        config.minCutoffHz = 100.0f;
        config.maxCutoffHz = 10000.0f;
        config.minResonance = 0.1f;
        config.maxResonance = 10.0f;
        
        float testCutoff = 300.0f; // Low cutoff
        float baseResonance = 2.0f;
        
        // Test with auto-ride enabled
        config.enabled = true;
        autoRide.initialize(config);
        float enabledRes = autoRide.processResonance(testCutoff, baseResonance);
        
        // Test with auto-ride disabled
        config.enabled = false;
        autoRide.initialize(config);
        float disabledRes = autoRide.processResonance(testCutoff, baseResonance);
        
        if (enabledRes > disabledRes && std::abs(disabledRes - baseResonance) < 0.1f) {
            std::cout << "PASS (enabled: " << enabledRes << "Q, disabled: " << disabledRes << "Q)\n";
        } else {
            std::cout << "FAIL (enable/disable not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test static utility functions
    std::cout << "Testing static utility functions... ";
    try {
        ResonanceAutoRide::Config config;
        config.autoRideAmount = 0.8f;
        config.minCutoffHz = 100.0f;
        config.maxCutoffHz = 8000.0f;
        config.minResonance = 0.5f;
        config.maxResonance = 12.0f;
        config.enabled = true;
        
        // Test auto-ride resonance calculation
        float lowCutoffRes = ResonanceAutoRide::calculateAutoRideResonance(200.0f, config);
        float highCutoffRes = ResonanceAutoRide::calculateAutoRideResonance(6000.0f, config);
        
        // Test cutoff opening calculation
        float lowResOpening = ResonanceAutoRide::calculateCutoffOpening(1.0f, config);
        float highResOpening = ResonanceAutoRide::calculateCutoffOpening(8.0f, config);
        
        if (lowCutoffRes > highCutoffRes && highResOpening > lowResOpening) {
            std::cout << "PASS (static functions working correctly)\n";
        } else {
            std::cout << "FAIL (static utility functions not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test boundary conditions
    std::cout << "Testing boundary conditions... ";
    try {
        ResonanceAutoRide autoRide;
        ResonanceAutoRide::Config config;
        config.autoRideAmount = 1.0f;
        config.minCutoffHz = 100.0f;
        config.maxCutoffHz = 8000.0f;
        config.minResonance = 0.1f;
        config.maxResonance = 20.0f;
        
        autoRide.initialize(config);
        
        // Test extreme cutoff values
        float veryLowRes = autoRide.processResonance(50.0f, 1.0f);   // Below min
        float veryHighRes = autoRide.processResonance(12000.0f, 1.0f); // Above max
        
        // Test extreme resonance values
        float opened1 = autoRide.processCutoffOpening(1000.0f, -5.0f);  // Below min
        float opened2 = autoRide.processCutoffOpening(1000.0f, 50.0f);  // Above max
        
        // Should handle gracefully without crashing
        bool boundsOK = (veryLowRes >= config.minResonance && veryLowRes <= config.maxResonance &&
                        veryHighRes >= config.minResonance && veryHighRes <= config.maxResonance &&
                        opened1 >= config.minCutoffHz && opened2 >= config.minCutoffHz);
        
        if (boundsOK) {
            std::cout << "PASS (boundary conditions handled correctly)\n";
        } else {
            std::cout << "FAIL (boundary conditions not handled properly)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test configuration updates
    std::cout << "Testing runtime configuration updates... ";
    try {
        ResonanceAutoRide autoRide;
        ResonanceAutoRide::Config config;
        autoRide.initialize(config);
        
        float testCutoff = 400.0f;
        float baseResonance = 1.5f;
        
        // Get initial result
        float initialRes = autoRide.processResonance(testCutoff, baseResonance);
        
        // Update auto-ride amount
        autoRide.setAutoRideAmount(0.2f);
        float reducedRes = autoRide.processResonance(testCutoff, baseResonance);
        
        // Update curve type
        autoRide.setCurveType(ResonanceAutoRide::CurveType::LINEAR);
        float linearRes = autoRide.processResonance(testCutoff, baseResonance);
        
        if (reducedRes < initialRes) {
            std::cout << "PASS (runtime config updates work)\n";
        } else {
            std::cout << "FAIL (runtime config updates not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL RESONANCE AUTO-RIDE TESTS PASSED!\n";
        std::cout << "Classic analog-style resonance auto-ride system is working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}