#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include "control/modulation/VelocityLatchSystem.h"
#include "sequencer/VelocityCapture.h"
#include "interface/ui/VelocityModulationUI.h"

int main() {
    std::cout << "EtherSynth Velocity Latch System Test\n";
    std::cout << "=====================================\n";
    
    bool allTestsPassed = true;
    
    // Test velocity latch system creation and initialization
    std::cout << "Testing VelocityLatchSystem creation and initialization... ";
    try {
        auto velocityCapture = std::make_shared<VelocityCapture>();
        auto modulationPanel = std::make_shared<VelocityModulationUI::VelocityModulationPanel>();
        
        VelocityLatchSystem latchSystem;
        latchSystem.initialize(velocityCapture);
        latchSystem.setVelocityModulationPanel(modulationPanel);
        
        if (latchSystem.isEnabled() && latchSystem.getActiveVelocityLatchCount() == 0) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization issue)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test parameter registration and velocity latch toggle
    std::cout << "Testing parameter registration and velocity latch toggle... ";
    try {
        auto velocityCapture = std::make_shared<VelocityCapture>();
        auto modulationPanel = std::make_shared<VelocityModulationUI::VelocityModulationPanel>();
        
        VelocityLatchSystem latchSystem;
        latchSystem.initialize(velocityCapture);
        latchSystem.setVelocityModulationPanel(modulationPanel);
        
        // Register parameters
        const uint32_t PARAM_CUTOFF = 1001;
        const uint32_t PARAM_RESONANCE = 1002;
        
        VelocityLatchSystem::ParameterVelocityConfig config;
        config.enabled = false;
        config.modulationDepth = 1.0f;
        config.baseValue = 0.5f;
        
        latchSystem.registerParameter(PARAM_CUTOFF, config);
        latchSystem.registerParameter(PARAM_RESONANCE, config);
        
        // Test initial state
        if (!latchSystem.isVelocityLatchEnabled(PARAM_CUTOFF) &&
            !latchSystem.isVelocityLatchEnabled(PARAM_RESONANCE) &&
            latchSystem.getActiveVelocityLatchCount() == 0) {
            
            // Test velocity latch toggle
            latchSystem.toggleVelocityLatch(PARAM_CUTOFF);
            
            if (latchSystem.isVelocityLatchEnabled(PARAM_CUTOFF) &&
                !latchSystem.isVelocityLatchEnabled(PARAM_RESONANCE) &&
                latchSystem.getActiveVelocityLatchCount() == 1) {
                
                std::cout << "PASS\n";
            } else {
                std::cout << "FAIL (toggle not working)\n";
                allTestsPassed = false;
            }
        } else {
            std::cout << "FAIL (initial state incorrect)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test velocity modulation calculation
    std::cout << "Testing velocity modulation calculation... ";
    try {
        auto velocityCapture = std::make_shared<VelocityCapture>();
        VelocityLatchSystem latchSystem;
        latchSystem.initialize(velocityCapture);
        
        const uint32_t PARAM_ID = 2001;
        VelocityLatchSystem::ParameterVelocityConfig config;
        config.enabled = true;
        config.modulationDepth = 1.0f;
        config.polarity = VelocityModulationUI::ModulationPolarity::POSITIVE;
        config.baseValue = 0.5f;
        
        latchSystem.registerParameter(PARAM_ID, config);
        
        // Test positive modulation
        float modulation64 = latchSystem.calculateVelocityModulation(PARAM_ID, 64);   // Mid velocity
        float modulation127 = latchSystem.calculateVelocityModulation(PARAM_ID, 127); // Max velocity
        float modulation1 = latchSystem.calculateVelocityModulation(PARAM_ID, 1);     // Min velocity
        
        if (std::abs(modulation64 - 0.5f) < 0.01f &&      // 64/127 ≈ 0.5
            std::abs(modulation127 - 1.0f) < 0.01f &&     // 127/127 = 1.0
            std::abs(modulation1 - 0.008f) < 0.01f) {      // 1/127 ≈ 0.008
            std::cout << "PASS (mod64: " << modulation64 << ", mod127: " << modulation127 << ", mod1: " << modulation1 << ")\n";
        } else {
            std::cout << "FAIL (modulation calculation incorrect)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test parameter value application
    std::cout << "Testing parameter value application... ";
    try {
        auto velocityCapture = std::make_shared<VelocityCapture>();
        VelocityLatchSystem latchSystem;
        latchSystem.initialize(velocityCapture);
        
        const uint32_t PARAM_ID = 3001;
        VelocityLatchSystem::ParameterVelocityConfig config;
        config.enabled = true;
        config.modulationDepth = 0.5f;  // 50% modulation depth
        config.polarity = VelocityModulationUI::ModulationPolarity::POSITIVE;
        config.baseValue = 0.4f;        // 40% base value
        
        latchSystem.registerParameter(PARAM_ID, config);
        
        // Test parameter modulation application
        float result64 = latchSystem.applyVelocityToParameter(PARAM_ID, 0.4f, 64);   // Mid velocity
        float result127 = latchSystem.applyVelocityToParameter(PARAM_ID, 0.4f, 127); // Max velocity
        float result1 = latchSystem.applyVelocityToParameter(PARAM_ID, 0.4f, 1);     // Min velocity
        
        // Expected: base + (depth * normalizedVelocity)
        // result64: 0.4 + (0.5 * 0.5) = 0.65
        // result127: 0.4 + (0.5 * 1.0) = 0.9
        // result1: 0.4 + (0.5 * 0.008) = ~0.404
        
        if (std::abs(result64 - 0.65f) < 0.01f &&
            std::abs(result127 - 0.9f) < 0.01f &&
            std::abs(result1 - 0.404f) < 0.01f) {
            std::cout << "PASS (values: " << result64 << ", " << result127 << ", " << result1 << ")\n";
        } else {
            std::cout << "FAIL (parameter application incorrect)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test negative and bipolar modulation
    std::cout << "Testing negative and bipolar modulation polarities... ";
    try {
        auto velocityCapture = std::make_shared<VelocityCapture>();
        VelocityLatchSystem latchSystem;
        latchSystem.initialize(velocityCapture);
        
        const uint32_t PARAM_NEG = 4001;
        const uint32_t PARAM_BIPOLAR = 4002;
        
        // Negative modulation config
        VelocityLatchSystem::ParameterVelocityConfig negConfig;
        negConfig.enabled = true;
        negConfig.modulationDepth = 1.0f;
        negConfig.polarity = VelocityModulationUI::ModulationPolarity::NEGATIVE;
        negConfig.baseValue = 0.5f;
        
        // Bipolar modulation config
        VelocityLatchSystem::ParameterVelocityConfig bipolarConfig;
        bipolarConfig.enabled = true;
        bipolarConfig.modulationDepth = 1.0f;
        bipolarConfig.polarity = VelocityModulationUI::ModulationPolarity::BIPOLAR;
        bipolarConfig.baseValue = 0.5f;
        
        latchSystem.registerParameter(PARAM_NEG, negConfig);
        latchSystem.registerParameter(PARAM_BIPOLAR, bipolarConfig);
        
        // Test negative modulation (should reduce parameter value)
        float negResult127 = latchSystem.applyVelocityToParameter(PARAM_NEG, 0.5f, 127);
        float negResult1 = latchSystem.applyVelocityToParameter(PARAM_NEG, 0.5f, 1);
        
        // Test bipolar modulation
        float bipolarResult32 = latchSystem.applyVelocityToParameter(PARAM_BIPOLAR, 0.5f, 32);   // Low velocity (negative)
        float bipolarResult127 = latchSystem.applyVelocityToParameter(PARAM_BIPOLAR, 0.5f, 127); // High velocity (positive)
        
        // Negative: base - (depth * normalizedVelocity)
        // negResult127: 0.5 - (1.0 * 1.0) = -0.5 → clamped to 0.0
        // negResult1: 0.5 - (1.0 * 0.008) = ~0.492
        
        // Bipolar: base + (depth * (normalizedVelocity * 2 - 1))
        // bipolarResult32: 0.5 + (1.0 * (0.25 * 2 - 1)) = 0.5 + (-0.5) = 0.0
        // bipolarResult127: 0.5 + (1.0 * (1.0 * 2 - 1)) = 0.5 + 1.0 = 1.5 → clamped to 1.0
        
        if (negResult127 < 0.01f &&                    // Should be clamped to ~0
            std::abs(negResult1 - 0.492f) < 0.01f &&   // Should be slightly reduced
            bipolarResult32 < 0.01f &&                 // Should be clamped to ~0
            std::abs(bipolarResult127 - 1.0f) < 0.01f) { // Should be clamped to 1.0
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (polarity calculations incorrect)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test real-time velocity modulation update
    std::cout << "Testing real-time velocity modulation update... ";
    try {
        auto velocityCapture = std::make_shared<VelocityCapture>();
        VelocityLatchSystem latchSystem;
        latchSystem.initialize(velocityCapture);
        
        const uint32_t PARAM_ID = 5001;
        VelocityLatchSystem::ParameterVelocityConfig config;
        config.enabled = true;
        config.modulationDepth = 1.0f;
        config.polarity = VelocityModulationUI::ModulationPolarity::POSITIVE;
        config.baseValue = 0.1f;        // 10% base value to avoid clamping
        
        latchSystem.registerParameter(PARAM_ID, config);
        
        // Set up parameter update callback to track changes
        bool parameterUpdated = false;
        uint32_t updatedParameterId = 0;
        float updatedValue = 0.0f;
        
        latchSystem.setParameterUpdateCallback([&](uint32_t parameterId, float modulatedValue) {
            parameterUpdated = true;
            updatedParameterId = parameterId;
            updatedValue = modulatedValue;
        });
        
        // Simulate velocity input with recent activity
        velocityCapture->updateMidiVelocity(100);  // Set velocity to 100
        velocityCapture->startVelocityCapture();   // Start capturing to make source active
        
        // Update velocity modulation system
        latchSystem.updateVelocityModulation();
        
        // Check if parameter was updated
        float expectedValue = 0.1f + (1.0f * (100.0f / 127.0f)); // base + (depth * normalizedVel)
        
        if (parameterUpdated &&
            updatedParameterId == PARAM_ID &&
            std::abs(updatedValue - expectedValue) < 0.01f) {
            std::cout << "PASS (updated to " << updatedValue << ")\n";
        } else {
            std::cout << "FAIL (real-time update not working: ";
            std::cout << "updated=" << parameterUpdated << ", ";
            std::cout << "paramId=" << updatedParameterId << " (expected " << PARAM_ID << "), ";
            std::cout << "value=" << updatedValue << " (expected " << expectedValue << "))\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test batch operations
    std::cout << "Testing batch velocity latch operations... ";
    try {
        auto velocityCapture = std::make_shared<VelocityCapture>();
        VelocityLatchSystem latchSystem;
        latchSystem.initialize(velocityCapture);
        
        // Register multiple parameters
        for (uint32_t i = 6001; i <= 6005; i++) {
            VelocityLatchSystem::ParameterVelocityConfig config;
            config.enabled = false;  // Start disabled
            config.modulationDepth = 0.5f;
            latchSystem.registerParameter(i, config);
        }
        
        // Test enable all
        latchSystem.enableAllVelocityLatches();
        size_t enabledCount = latchSystem.getActiveVelocityLatchCount();
        
        // Test disable all
        latchSystem.disableAllVelocityLatches();
        size_t disabledCount = latchSystem.getActiveVelocityLatchCount();
        
        // Test set all depths
        latchSystem.enableAllVelocityLatches();
        latchSystem.setAllModulationDepths(1.5f);
        
        if (enabledCount == 5 && disabledCount == 0) {
            std::cout << "PASS (batch operations working)\n";
        } else {
            std::cout << "FAIL (batch operations not working: enabled=" << enabledCount << ", disabled=" << disabledCount << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test system load calculation
    std::cout << "Testing system load calculation... ";
    try {
        VelocityLatchSystem latchSystem;
        
        // Add various numbers of parameters and check load calculation
        for (uint32_t i = 7001; i <= 7010; i++) {  // 10 parameters
            VelocityLatchSystem::ParameterVelocityConfig config;
            config.enabled = true;
            latchSystem.registerParameter(i, config);
        }
        
        float systemLoad = latchSystem.getSystemVelocityModulationLoad();
        size_t activeCount = latchSystem.getActiveVelocityLatchCount();
        
        // Should be approximately 0.01 (10 * 0.001)
        if (activeCount == 10 && std::abs(systemLoad - 0.01f) < 0.001f) {
            std::cout << "PASS (load: " << systemLoad << " for " << activeCount << " parameters)\n";
        } else {
            std::cout << "FAIL (load calculation incorrect)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL VELOCITY LATCH SYSTEM TESTS PASSED!\n";
        std::cout << "Velocity latch toggle functionality is working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}