#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>
#include "control/modulation/RelativeVelocityModulation.h"

int main() {
    std::cout << "EtherSynth Relative Velocity Modulation Test\n";
    std::cout << "============================================\n";
    
    bool allTestsPassed = true;
    
    // Test relative velocity modulation system creation
    std::cout << "Testing RelativeVelocityModulation creation... ";
    try {
        RelativeVelocityModulation relativeVel;
        
        if (relativeVel.isEnabled() && relativeVel.getSampleRate() == 48000.0f) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization issue)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test absolute modulation mode (basic)
    std::cout << "Testing absolute modulation mode... ";
    try {
        RelativeVelocityModulation relativeVel;
        const uint32_t PARAM_ID = 1001;
        
        RelativeVelocityModulation::VelocityModulationConfig config;
        config.mode = RelativeVelocityModulation::ModulationMode::ABSOLUTE;
        config.curveType = RelativeVelocityModulation::CurveType::LINEAR;
        config.modulationDepth = 1.0f;
        config.smoothingType = RelativeVelocityModulation::SmoothingType::NONE;
        
        relativeVel.setParameterConfig(PARAM_ID, config);
        
        // Test different velocities
        auto result64 = relativeVel.calculateModulation(PARAM_ID, 0.5f, 64);   // Mid velocity
        auto result127 = relativeVel.calculateModulation(PARAM_ID, 0.5f, 127); // Max velocity
        auto result1 = relativeVel.calculateModulation(PARAM_ID, 0.5f, 1);     // Min velocity
        
        // Expected: baseValue + (normalizedVelocity * modulationDepth)
        // result64: 0.5 + (0.5 * 1.0) = 1.0
        // result127: 0.5 + (1.0 * 1.0) = 1.5 → clamped to 1.0
        // result1: 0.5 + (0.008 * 1.0) ≈ 0.508
        
        if (std::abs(result64.modulatedValue - 1.0f) < 0.01f &&
            std::abs(result127.modulatedValue - 1.0f) < 0.01f &&  // Clamped
            std::abs(result1.modulatedValue - 0.508f) < 0.01f &&
            result64.isActive && result127.isActive && result1.isActive) {
            std::cout << "PASS (values: " << result64.modulatedValue << ", " 
                      << result127.modulatedValue << ", " << result1.modulatedValue << ")\n";
        } else {
            std::cout << "FAIL (absolute modulation incorrect)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test relative modulation mode
    std::cout << "Testing relative modulation mode... ";
    try {
        RelativeVelocityModulation relativeVel;
        const uint32_t PARAM_ID = 2001;
        
        RelativeVelocityModulation::VelocityModulationConfig config;
        config.mode = RelativeVelocityModulation::ModulationMode::RELATIVE;
        config.modulationDepth = 0.5f;  // 50% depth
        
        relativeVel.setParameterConfig(PARAM_ID, config);
        
        // Test relative modulation: velocity affects change from current to target
        float currentValue = 0.3f;
        float targetValue = 0.8f;  // Want to move from 0.3 to 0.8
        
        auto result64 = relativeVel.calculateModulation(PARAM_ID, currentValue, 64);   // Mid velocity
        auto result127 = relativeVel.calculateModulation(PARAM_ID, currentValue, 127); // Max velocity
        auto result1 = relativeVel.calculateModulation(PARAM_ID, currentValue, 1);     // Min velocity
        
        // Relative modulation should interpolate between current and target based on velocity
        if (result64.modulatedValue > currentValue && result64.modulatedValue < targetValue &&
            result127.modulatedValue > result64.modulatedValue &&
            result1.modulatedValue > currentValue && result1.modulatedValue < result64.modulatedValue) {
            std::cout << "PASS (progression: " << result1.modulatedValue << " < " 
                      << result64.modulatedValue << " < " << result127.modulatedValue << ")\n";
        } else {
            std::cout << "FAIL (relative modulation not working correctly)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test additive modulation mode with polarity
    std::cout << "Testing additive modulation with different polarities... ";
    try {
        RelativeVelocityModulation relativeVel;
        const uint32_t PARAM_POS = 3001;
        const uint32_t PARAM_NEG = 3002;
        const uint32_t PARAM_BI = 3003;
        
        // Positive modulation
        RelativeVelocityModulation::VelocityModulationConfig configPos;
        configPos.mode = RelativeVelocityModulation::ModulationMode::ADDITIVE;
        configPos.polarity = VelocityModulationUI::ModulationPolarity::POSITIVE;
        configPos.modulationDepth = 0.3f;
        
        // Negative modulation
        RelativeVelocityModulation::VelocityModulationConfig configNeg;
        configNeg.mode = RelativeVelocityModulation::ModulationMode::ADDITIVE;
        configNeg.polarity = VelocityModulationUI::ModulationPolarity::NEGATIVE;
        configNeg.modulationDepth = 0.3f;
        
        // Bipolar modulation
        RelativeVelocityModulation::VelocityModulationConfig configBi;
        configBi.mode = RelativeVelocityModulation::ModulationMode::ADDITIVE;
        configBi.polarity = VelocityModulationUI::ModulationPolarity::BIPOLAR;
        configBi.modulationDepth = 0.3f;
        
        relativeVel.setParameterConfig(PARAM_POS, configPos);
        relativeVel.setParameterConfig(PARAM_NEG, configNeg);
        relativeVel.setParameterConfig(PARAM_BI, configBi);
        
        float baseValue = 0.5f;
        uint8_t testVelocity = 100;
        
        auto resultPos = relativeVel.calculateModulation(PARAM_POS, baseValue, testVelocity);
        auto resultNeg = relativeVel.calculateModulation(PARAM_NEG, baseValue, testVelocity);
        auto resultBi = relativeVel.calculateModulation(PARAM_BI, baseValue, testVelocity);
        
        // Positive should increase, negative should decrease, bipolar should depend on velocity
        if (resultPos.modulatedValue > baseValue &&
            resultNeg.modulatedValue < baseValue &&
            resultBi.modulatedValue != baseValue) {  // Bipolar should be different from base
            std::cout << "PASS (pos: " << resultPos.modulatedValue << ", neg: " 
                      << resultNeg.modulatedValue << ", bi: " << resultBi.modulatedValue << ")\n";
        } else {
            std::cout << "FAIL (polarity modulation not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test multiplicative modulation mode
    std::cout << "Testing multiplicative modulation mode... ";
    try {
        RelativeVelocityModulation relativeVel;
        const uint32_t PARAM_ID = 4001;
        
        RelativeVelocityModulation::VelocityModulationConfig config;
        config.mode = RelativeVelocityModulation::ModulationMode::MULTIPLICATIVE;
        config.modulationDepth = 1.0f;  // 100% modulation depth
        
        relativeVel.setParameterConfig(PARAM_ID, config);
        
        float baseValue = 0.5f;
        
        auto result64 = relativeVel.calculateModulation(PARAM_ID, baseValue, 64);   // Mid velocity
        auto result127 = relativeVel.calculateModulation(PARAM_ID, baseValue, 127); // Max velocity
        auto result1 = relativeVel.calculateModulation(PARAM_ID, baseValue, 1);     // Min velocity
        
        // Multiplicative should scale the base value
        // Mid velocity (0.5) should leave value roughly unchanged
        // High velocity should increase, low velocity should decrease
        if (std::abs(result64.modulatedValue - baseValue) < 0.1f &&  // Should be close to base
            result127.modulatedValue > baseValue &&                   // Should be higher
            result1.modulatedValue < baseValue) {                     // Should be lower
            std::cout << "PASS (scaling: " << result1.modulatedValue << " < " 
                      << result64.modulatedValue << " < " << result127.modulatedValue << ")\n";
        } else {
            std::cout << "FAIL (multiplicative modulation not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test velocity curve processing
    std::cout << "Testing velocity curve processing... ";
    try {
        RelativeVelocityModulation relativeVel;
        
        float testVelocity = 0.5f;  // Mid-range velocity
        
        // Test different curve types
        float linear = relativeVel.applyCurve(testVelocity, RelativeVelocityModulation::CurveType::LINEAR, 1.0f);
        float exponential = relativeVel.applyCurve(testVelocity, RelativeVelocityModulation::CurveType::EXPONENTIAL, 2.0f);
        float logarithmic = relativeVel.applyCurve(testVelocity, RelativeVelocityModulation::CurveType::LOGARITHMIC, 2.0f);
        float sCurve = relativeVel.applyCurve(testVelocity, RelativeVelocityModulation::CurveType::S_CURVE, 2.0f);
        float stepped = relativeVel.applyCurve(testVelocity, RelativeVelocityModulation::CurveType::STEPPED, 4.0f);
        
        if (std::abs(linear - 0.5f) < 0.01f &&        // Linear should be unchanged
            exponential > 0.6f &&                     // Exponential should be higher (0.707)
            logarithmic < 0.3f &&                     // Logarithmic should be lower (0.25)
            std::abs(sCurve - 0.5f) < 0.01f &&        // S-curve at midpoint should be ~0.5
            std::abs(stepped - 0.333f) < 0.01f) {     // Stepped should be quantized to 0.333
            std::cout << "PASS (curves: lin=" << linear << ", exp=" << exponential 
                      << ", log=" << logarithmic << ", s=" << sCurve << ", step=" << stepped << ")\n";
        } else {
            std::cout << "FAIL (curve processing not working correctly)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test velocity smoothing
    std::cout << "Testing velocity smoothing... ";
    try {
        RelativeVelocityModulation relativeVel;
        const uint32_t PARAM_ID = 5001;
        
        RelativeVelocityModulation::VelocityModulationConfig config;
        config.mode = RelativeVelocityModulation::ModulationMode::ABSOLUTE;
        config.smoothingType = RelativeVelocityModulation::SmoothingType::LOW_PASS;
        config.smoothingAmount = 0.3f;  // Moderate smoothing
        config.modulationDepth = 1.0f;
        
        relativeVel.setParameterConfig(PARAM_ID, config);
        
        // Apply series of velocity values to test smoothing
        float baseValue = 0.0f;
        auto result1 = relativeVel.calculateModulation(PARAM_ID, baseValue, 127); // High velocity
        auto result2 = relativeVel.calculateModulation(PARAM_ID, baseValue, 127); // Same high velocity
        auto result3 = relativeVel.calculateModulation(PARAM_ID, baseValue, 1);   // Sudden low velocity
        auto result4 = relativeVel.calculateModulation(PARAM_ID, baseValue, 1);   // Same low velocity
        
        // With smoothing, the sudden change should be gradual
        if (result2.smoothedVelocity >= result1.smoothedVelocity &&  // Should maintain or increase
            result3.smoothedVelocity > result4.smoothedVelocity &&   // Should be smoothly decreasing
            result3.smoothedVelocity < result2.smoothedVelocity) {   // Should be lower than previous high
            std::cout << "PASS (smoothing working: " << result1.smoothedVelocity 
                      << " → " << result2.smoothedVelocity << " → " << result3.smoothedVelocity 
                      << " → " << result4.smoothedVelocity << ")\n";
        } else {
            std::cout << "FAIL (smoothing not working correctly)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test velocity quantization
    std::cout << "Testing velocity quantization... ";
    try {
        RelativeVelocityModulation relativeVel;
        const uint32_t PARAM_ID = 6001;
        
        RelativeVelocityModulation::VelocityModulationConfig config;
        config.mode = RelativeVelocityModulation::ModulationMode::ABSOLUTE;
        config.enableQuantization = true;
        config.quantizationSteps = 4;  // 4 steps: 0, 0.33, 0.67, 1.0
        config.modulationDepth = 1.0f;
        config.smoothingType = RelativeVelocityModulation::SmoothingType::NONE;
        
        relativeVel.setParameterConfig(PARAM_ID, config);
        
        // Test velocities that should be quantized to specific steps
        auto result25 = relativeVel.calculateModulation(PARAM_ID, 0.0f, 32);   // ~25% velocity
        auto result50 = relativeVel.calculateModulation(PARAM_ID, 0.0f, 64);   // ~50% velocity
        auto result75 = relativeVel.calculateModulation(PARAM_ID, 0.0f, 96);   // ~75% velocity
        auto result100 = relativeVel.calculateModulation(PARAM_ID, 0.0f, 127); // 100% velocity
        
        // Should be quantized to discrete steps
        std::vector<float> expectedSteps = {0.0f, 0.333f, 0.667f, 1.0f};
        
        // Find closest quantized values
        bool quantized = true;
        for (auto result : {result25, result50, result75, result100}) {
            bool found = false;
            for (float step : expectedSteps) {
                if (std::abs(result.modulatedValue - step) < 0.1f) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                quantized = false;
                break;
            }
        }
        
        if (quantized) {
            std::cout << "PASS (quantized values: " << result25.modulatedValue 
                      << ", " << result50.modulatedValue << ", " << result75.modulatedValue 
                      << ", " << result100.modulatedValue << ")\n";
        } else {
            std::cout << "FAIL (quantization not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test performance monitoring
    std::cout << "Testing performance monitoring... ";
    try {
        RelativeVelocityModulation relativeVel;
        relativeVel.enableProfiling(true);
        
        // Add multiple parameters
        for (uint32_t i = 7001; i <= 7010; i++) {
            RelativeVelocityModulation::VelocityModulationConfig config;
            relativeVel.setParameterConfig(i, config);
        }
        
        // Perform some calculations
        for (uint32_t i = 7001; i <= 7010; i++) {
            relativeVel.calculateModulation(i, 0.5f, 64);
        }
        
        size_t activeCount = relativeVel.getActiveParameterCount();
        float cpuUsage = relativeVel.getCPUUsageEstimate();
        
        if (activeCount == 10 && cpuUsage >= 0.0f) {  // Should have valid CPU usage estimate
            std::cout << "PASS (active params: " << activeCount << ", CPU: " << cpuUsage << "%)\n";
        } else {
            std::cout << "FAIL (performance monitoring not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL RELATIVE VELOCITY MODULATION TESTS PASSED!\n";
        std::cout << "Advanced velocity modulation calculation system is working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}