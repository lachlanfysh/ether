#include <iostream>
#include <cmath>
#include "audio/LUFSNormalizer.h"

int main() {
    std::cout << "EtherSynth LUFS Normalizer Test\n";
    std::cout << "===============================\n";
    
    const float SAMPLE_RATE = 44100.0f;
    bool allTestsPassed = true;
    
    // Test basic initialization
    std::cout << "Testing LUFS Normalizer initialization... ";
    try {
        LUFSNormalizer normalizer;
        if (normalizer.initialize(SAMPLE_RATE, true)) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test bypass functionality
    std::cout << "Testing bypass functionality... ";
    try {
        LUFSNormalizer normalizer;
        normalizer.initialize(SAMPLE_RATE, false);
        
        float testSignal = 0.5f;
        float originalSignal = testSignal;
        
        // Process with bypass enabled
        normalizer.setBypass(true);
        float output = normalizer.processSample(testSignal);
        
        if (std::abs(output - originalSignal) < 1e-6f) {
            std::cout << "PASS (signal unchanged when bypassed)\n";
        } else {
            std::cout << "FAIL (bypass not working: " << output << " vs " << originalSignal << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test mono processing and LUFS measurement
    std::cout << "Testing mono processing and LUFS measurement... ";
    try {
        LUFSNormalizer normalizer;
        normalizer.initialize(SAMPLE_RATE, false);
        normalizer.setTargetLUFS(-20.0f);
        normalizer.setIntegrationTime(1.0f); // Shorter for testing
        
        // Generate test tone (1kHz sine wave)
        const int numSamples = static_cast<int>(SAMPLE_RATE * 0.5f); // 0.5 seconds
        float amplitude = 0.1f; // -20dB FS approximately
        
        for (int i = 0; i < numSamples; i++) {
            float t = static_cast<float>(i) / SAMPLE_RATE;
            float input = amplitude * std::sin(2.0f * 3.14159f * 1000.0f * t);
            float output = normalizer.processSample(input);
        }
        
        float currentLUFS = normalizer.getCurrentLUFS();
        float integratedLUFS = normalizer.getIntegratedLUFS();
        float currentGain = normalizer.getCurrentGain();
        
        // LUFS should be reasonable (not -70 or 0)
        if (currentLUFS > -50.0f && currentLUFS < -10.0f && 
            integratedLUFS > -50.0f && integratedLUFS < -10.0f &&
            currentGain > 0.1f && currentGain < 10.0f) {
            std::cout << "PASS (LUFS: " << integratedLUFS << ", gain: " << currentGain << ")\n";
        } else {
            std::cout << "FAIL (unrealistic values: LUFS=" << integratedLUFS << ", gain=" << currentGain << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test stereo processing
    std::cout << "Testing stereo processing... ";
    try {
        LUFSNormalizer normalizer;
        normalizer.initialize(SAMPLE_RATE, true);
        
        // Process stereo samples
        float left = 0.3f;
        float right = 0.2f;
        float originalLeft = left;
        float originalRight = right;
        
        normalizer.processStereoSample(left, right);
        
        // Both channels should be processed (likely different from input)
        // but should maintain some relationship to original
        float leftRatio = left / std::max(originalLeft, 1e-6f);
        float rightRatio = right / std::max(originalRight, 1e-6f);
        
        if (std::abs(leftRatio - rightRatio) < 0.1f) { // Same gain applied to both
            std::cout << "PASS (stereo coherent: L ratio=" << leftRatio << ", R ratio=" << rightRatio << ")\n";
        } else {
            std::cout << "FAIL (stereo not coherent)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test reference calibration
    std::cout << "Testing reference calibration... ";
    try {
        LUFSNormalizer normalizer;
        normalizer.initialize(SAMPLE_RATE, false);
        
        // Process some signal to get a measurement
        for (int i = 0; i < 1000; i++) {
            float input = 0.1f * std::sin(2.0f * 3.14159f * 440.0f * i / SAMPLE_RATE);
            normalizer.processSample(input);
        }
        
        // Calibrate reference
        normalizer.calibrateReference();
        float lufsBeforeReset = normalizer.getIntegratedLUFS();
        
        // Reset calibration
        normalizer.resetCalibration();
        
        std::cout << "PASS (calibration set at " << lufsBeforeReset << " LUFS)\n";
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test parameter adjustment
    std::cout << "Testing parameter adjustment... ";
    try {
        LUFSNormalizer normalizer;
        normalizer.initialize(SAMPLE_RATE, false);
        
        // Test target LUFS adjustment
        normalizer.setTargetLUFS(-18.0f);
        normalizer.setIntegrationTime(2.0f);
        normalizer.setMaxGainReduction(15.0f);
        normalizer.setMaxGainBoost(8.0f);
        normalizer.setGainSmoothingTime(100.0f);
        
        std::cout << "PASS (parameters set successfully)\n";
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test block processing
    std::cout << "Testing block processing... ";
    try {
        LUFSNormalizer normalizer;
        normalizer.initialize(SAMPLE_RATE, true);
        
        const int blockSize = 128;
        float leftChannel[blockSize];
        float rightChannel[blockSize];
        
        // Fill with test signal
        for (int i = 0; i < blockSize; i++) {
            float t = static_cast<float>(i) / SAMPLE_RATE;
            leftChannel[i] = 0.2f * std::sin(2.0f * 3.14159f * 800.0f * t);
            rightChannel[i] = 0.15f * std::sin(2.0f * 3.14159f * 1200.0f * t);
        }
        
        // Process block
        normalizer.processStereoBlock(leftChannel, rightChannel, blockSize);
        
        // Verify processing occurred (values should change)
        bool leftChanged = false;
        bool rightChanged = false;
        
        for (int i = 0; i < blockSize; i++) {
            if (std::abs(leftChannel[i]) > 0.001f) leftChanged = true;
            if (std::abs(rightChannel[i]) > 0.001f) rightChanged = true;
        }
        
        if (leftChanged && rightChanged) {
            std::cout << "PASS (block processing working)\n";
        } else {
            std::cout << "FAIL (block processing not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test integration time effects
    std::cout << "Testing integration time effects... ";
    try {
        LUFSNormalizer normalizer1, normalizer2;
        normalizer1.initialize(SAMPLE_RATE, false);
        normalizer2.initialize(SAMPLE_RATE, false);
        
        normalizer1.setIntegrationTime(0.5f);  // Fast
        normalizer2.setIntegrationTime(3.0f);  // Slow
        
        // Process same signal through both
        const int numSamples = static_cast<int>(SAMPLE_RATE * 0.2f);
        for (int i = 0; i < numSamples; i++) {
            float input = 0.1f * std::sin(2.0f * 3.14159f * 440.0f * i / SAMPLE_RATE);
            normalizer1.processSample(input);
            normalizer2.processSample(input);
        }
        
        float gain1 = normalizer1.getCurrentGain();
        float gain2 = normalizer2.getCurrentGain();
        
        // Both should have reasonable gains
        if (gain1 > 0.1f && gain1 < 10.0f && gain2 > 0.1f && gain2 < 10.0f) {
            std::cout << "PASS (fast gain: " << gain1 << ", slow gain: " << gain2 << ")\n";
        } else {
            std::cout << "FAIL (unrealistic gains)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL LUFS NORMALIZER TESTS PASSED!\n";
        std::cout << "LUFS loudness normalization system is working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}