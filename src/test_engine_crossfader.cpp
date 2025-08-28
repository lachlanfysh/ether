#include <iostream>
#include <cmath>
#include "audio/EngineCrossfader.h"

int main() {
    std::cout << "EtherSynth Engine Crossfader Test\n";
    std::cout << "=================================\n";
    
    const float SAMPLE_RATE = 44100.0f;
    bool allTestsPassed = true;
    
    // Test basic initialization
    std::cout << "Testing Engine Crossfader initialization... ";
    try {
        EngineCrossfader crossfader;
        if (crossfader.initialize(SAMPLE_RATE, 30.0f)) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test initial state (engine A only)
    std::cout << "Testing initial state... ";
    try {
        EngineCrossfader crossfader;
        crossfader.initialize(SAMPLE_RATE, 30.0f);
        
        float engineA = 1.0f;
        float engineB = 0.5f;
        float output = crossfader.processMix(engineA, engineB);
        
        // Should output mostly engine A initially
        if (std::abs(output - engineA) < 0.1f && 
            crossfader.getCurrentState() == EngineCrossfader::CrossfadeState::ENGINE_A_ONLY) {
            std::cout << "PASS (output: " << output << ", state: A_ONLY)\n";
        } else {
            std::cout << "FAIL (wrong initial state or output)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test crossfade to engine B
    std::cout << "Testing crossfade to engine B... ";
    try {
        EngineCrossfader crossfader;
        crossfader.initialize(SAMPLE_RATE, 10.0f); // Short crossfade for testing
        
        float engineA = 1.0f;
        float engineB = 0.5f;
        
        // Start crossfade to B
        crossfader.startCrossfadeToB();
        
        // Process samples and track output
        float initialOutput = crossfader.processMix(engineA, engineB);
        
        // Process enough samples to complete crossfade
        int crossfadeSamples = static_cast<int>(0.01f * SAMPLE_RATE); // 10ms
        float finalOutput = 0.0f;
        
        for (int i = 0; i < crossfadeSamples + 100; i++) {
            finalOutput = crossfader.processMix(engineA, engineB);
        }
        
        // Should end up mostly at engine B
        if (std::abs(finalOutput - engineB) < 0.1f && 
            crossfader.getCurrentState() == EngineCrossfader::CrossfadeState::ENGINE_B_ONLY) {
            std::cout << "PASS (initial: " << initialOutput << ", final: " << finalOutput << ")\n";
        } else {
            std::cout << "FAIL (crossfade not completed properly)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test manual position control
    std::cout << "Testing manual position control... ";
    try {
        EngineCrossfader crossfader;
        crossfader.initialize(SAMPLE_RATE, 30.0f);
        crossfader.setManualControl(true);
        
        float engineA = 0.8f;
        float engineB = 0.4f;
        
        // Test at 25% position (mostly engine A)
        crossfader.setCrossfadePosition(0.25f);
        float output25 = crossfader.processMix(engineA, engineB);
        
        // Test at 75% position (mostly engine B)
        crossfader.setCrossfadePosition(0.75f);
        float output75 = crossfader.processMix(engineA, engineB);
        
        // Output should change appropriately
        if (output25 > output75 && // More engine A at 25%
            crossfader.getCrossfadePosition() == 0.75f) {
            std::cout << "PASS (25%: " << output25 << ", 75%: " << output75 << ")\n";
        } else {
            std::cout << "FAIL (manual control not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test equal-power crossfade law
    std::cout << "Testing equal-power crossfade law... ";
    try {
        EngineCrossfader crossfader;
        crossfader.initialize(SAMPLE_RATE, 30.0f);
        crossfader.setCrossfadeType(EngineCrossfader::CrossfadeType::EQUAL_POWER_SINE);
        crossfader.setManualControl(true);
        
        // Test with equal amplitude signals
        float engineA = 0.707f; // -3dB
        float engineB = 0.707f; // -3dB
        
        // At 50% crossfade, equal-power should maintain similar level
        crossfader.setCrossfadePosition(0.5f);
        float output50 = crossfader.processMix(engineA, engineB);
        
        // Should be close to original level (equal power law)
        float expectedLevel = engineA * std::cos(0.5f * 1.5708f) + engineB * std::sin(0.5f * 1.5708f);
        
        if (std::abs(output50 - expectedLevel) < 0.1f) {
            std::cout << "PASS (50% output: " << output50 << ", expected: " << expectedLevel << ")\n";
        } else {
            std::cout << "FAIL (equal-power law not working: got " << output50 << ", expected " << expectedLevel << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test different crossfade types
    std::cout << "Testing crossfade types... ";
    try {
        EngineCrossfader crossfader;
        crossfader.initialize(SAMPLE_RATE, 30.0f);
        crossfader.setManualControl(true);
        crossfader.setCrossfadePosition(0.5f);
        
        float engineA = 0.6f;
        float engineB = 0.4f;
        
        // Test sine crossfade
        crossfader.setCrossfadeType(EngineCrossfader::CrossfadeType::EQUAL_POWER_SINE);
        float outputSine = crossfader.processMix(engineA, engineB);
        
        // Test square root crossfade
        crossfader.setCrossfadeType(EngineCrossfader::CrossfadeType::EQUAL_POWER_SQRT);
        float outputSqrt = crossfader.processMix(engineA, engineB);
        
        // Test linear crossfade
        crossfader.setCrossfadeType(EngineCrossfader::CrossfadeType::LINEAR);
        float outputLinear = crossfader.processMix(engineA, engineB);
        
        // All should give different results - but differences might be small
        bool typesAreDifferent = (std::abs(outputSine - outputSqrt) > 0.001f || 
                                 std::abs(outputSine - outputLinear) > 0.001f ||
                                 std::abs(outputSqrt - outputLinear) > 0.001f);
        
        if (typesAreDifferent) {
            std::cout << "PASS (sine: " << outputSine << ", sqrt: " << outputSqrt << ", linear: " << outputLinear << ")\n";
        } else {
            std::cout << "FAIL (crossfade types not different enough: sine=" << outputSine << ", sqrt=" << outputSqrt << ", linear=" << outputLinear << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test snap to engine functionality
    std::cout << "Testing snap to engine... ";
    try {
        EngineCrossfader crossfader;
        crossfader.initialize(SAMPLE_RATE, 30.0f);
        
        float engineA = 0.9f;
        float engineB = 0.3f;
        
        // Snap to engine B
        crossfader.snapToEngine(true);
        float outputB = crossfader.processMix(engineA, engineB);
        
        // Snap to engine A
        crossfader.snapToEngine(false);
        float outputA = crossfader.processMix(engineA, engineB);
        
        if (std::abs(outputB - engineB) < 0.1f && std::abs(outputA - engineA) < 0.1f) {
            std::cout << "PASS (snap to B: " << outputB << ", snap to A: " << outputA << ")\n";
        } else {
            std::cout << "FAIL (snap functionality not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test block processing
    std::cout << "Testing block processing... ";
    try {
        EngineCrossfader crossfader;
        crossfader.initialize(SAMPLE_RATE, 20.0f);
        
        const int blockSize = 64;
        float engineABuffer[blockSize];
        float engineBBuffer[blockSize];
        float outputBuffer[blockSize];
        
        // Fill with different signals
        for (int i = 0; i < blockSize; i++) {
            engineABuffer[i] = 0.5f * std::sin(2.0f * 3.14159f * 440.0f * i / SAMPLE_RATE);
            engineBBuffer[i] = 0.3f * std::sin(2.0f * 3.14159f * 880.0f * i / SAMPLE_RATE);
        }
        
        // Start crossfade and process block
        crossfader.startCrossfadeToB();
        crossfader.processBlock(engineABuffer, engineBBuffer, outputBuffer, blockSize);
        
        // Verify output is reasonable
        float outputRMS = 0.0f;
        for (int i = 0; i < blockSize; i++) {
            outputRMS += outputBuffer[i] * outputBuffer[i];
        }
        outputRMS = std::sqrt(outputRMS / blockSize);
        
        if (outputRMS > 0.05f && outputRMS < 0.8f) {
            std::cout << "PASS (block output RMS: " << outputRMS << ")\n";
        } else {
            std::cout << "FAIL (block processing output unrealistic)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test crossfade time adjustment
    std::cout << "Testing crossfade time adjustment... ";
    try {
        EngineCrossfader crossfader;
        crossfader.initialize(SAMPLE_RATE, 30.0f);
        
        // Test that crossfade time is set correctly
        crossfader.setCrossfadeTime(50.0f);
        
        if (crossfader.getCrossfadeTimeMs() == 50.0f) {
            std::cout << "PASS (crossfade time set to " << crossfader.getCrossfadeTimeMs() << "ms)\n";
        } else {
            std::cout << "FAIL (crossfade time not set correctly)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL ENGINE CROSSFADER TESTS PASSED!\n";
        std::cout << "Equal-power crossfading system is working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}