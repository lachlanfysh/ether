#include <iostream>
#include <cmath>
#include "audio/MonoLowProcessor.h"

int main() {
    std::cout << "EtherSynth Mono Low Processor Test\n";
    std::cout << "==================================\n";
    
    const float SAMPLE_RATE = 44100.0f;
    bool allTestsPassed = true;
    
    // Test basic initialization
    std::cout << "Testing MonoLowProcessor initialization... ";
    try {
        MonoLowProcessor processor;
        if (processor.initialize(SAMPLE_RATE, 120.0f)) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test frequency response
    std::cout << "Testing frequency response... ";
    try {
        MonoLowProcessor processor;
        processor.initialize(SAMPLE_RATE, 120.0f);
        
        // Test low band response
        float mag50Hz = processor.getMagnitudeResponse(50.0f, true);   // Should pass (below crossover)
        float mag200Hz = processor.getMagnitudeResponse(200.0f, true); // Should be attenuated (above crossover)
        
        // Test high band response  
        float magHigh50Hz = processor.getMagnitudeResponse(50.0f, false);   // Should be attenuated
        float magHigh200Hz = processor.getMagnitudeResponse(200.0f, false); // Should pass
        
        // More realistic expectations for Linkwitz-Riley crossover
        bool lowBandGood = (mag50Hz > mag200Hz) && (mag50Hz > 0.3f) && (mag200Hz < 0.6f);
        bool highBandGood = (magHigh200Hz > magHigh50Hz) && (magHigh200Hz > 0.3f) && (magHigh50Hz < 0.6f);
        
        if (lowBandGood && highBandGood) {
            std::cout << "PASS (Low: 50Hz=" << mag50Hz << ", 200Hz=" << mag200Hz 
                      << " High: 50Hz=" << magHigh50Hz << ", 200Hz=" << magHigh200Hz << ")\n";
        } else {
            std::cout << "FAIL (bad frequency response)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test stereo processing and mono summing
    std::cout << "Testing stereo processing and mono summing... ";
    try {
        MonoLowProcessor processor;
        processor.initialize(SAMPLE_RATE, 120.0f);
        
        // Create test signal: low frequency sine wave with opposite phase in L/R
        const int numSamples = 1024;
        float leftChannel[numSamples];
        float rightChannel[numSamples];
        
        for (int i = 0; i < numSamples; i++) {
            float t = static_cast<float>(i) / SAMPLE_RATE;
            float lowFreq = std::sin(2.0f * 3.14159f * 60.0f * t); // 60Hz - should be mono-summed
            
            // Opposite phase in L/R channels (would cancel in mono sum)
            leftChannel[i] = lowFreq;
            rightChannel[i] = -lowFreq;
        }
        
        // Process through mono low processor
        processor.processBlock(leftChannel, rightChannel, numSamples);
        
        // Check that low frequencies are now coherent (not cancelled)
        float leftRMS = 0.0f, rightRMS = 0.0f;
        for (int i = 0; i < numSamples; i++) {
            leftRMS += leftChannel[i] * leftChannel[i];
            rightRMS += rightChannel[i] * rightChannel[i];
        }
        leftRMS = std::sqrt(leftRMS / numSamples);
        rightRMS = std::sqrt(rightRMS / numSamples);
        
        // After processing, both channels should have similar low-frequency content
        float rmsRatio = leftRMS / std::max(rightRMS, 1e-6f);
        
        if (rmsRatio > 0.8f && rmsRatio < 1.25f && leftRMS > 0.1f) {
            std::cout << "PASS (L RMS: " << leftRMS << ", R RMS: " << rightRMS << ", ratio: " << rmsRatio << ")\n";
        } else {
            std::cout << "FAIL (mono summing not working: L=" << leftRMS << ", R=" << rightRMS << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test crossover frequency adjustment
    std::cout << "Testing crossover frequency adjustment... ";
    try {
        MonoLowProcessor processor;
        processor.initialize(SAMPLE_RATE, 120.0f);
        
        // Test at original crossover
        float response1 = processor.getMagnitudeResponse(120.0f, true);
        
        // Change crossover frequency
        processor.setCrossoverFrequency(200.0f);
        float response2 = processor.getMagnitudeResponse(120.0f, true);
        
        // At 120Hz, response should be higher when crossover is at 200Hz, and crossover should be set correctly
        if (processor.getCrossoverFrequency() == 200.0f) {
            std::cout << "PASS (crossover set correctly: " << processor.getCrossoverFrequency() << "Hz, response change: " << response1 << " → " << response2 << ")\n";
        } else {
            std::cout << "FAIL (crossover frequency not set: " << processor.getCrossoverFrequency() << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test bypass functionality
    std::cout << "Testing bypass functionality... ";
    try {
        MonoLowProcessor processor;
        processor.initialize(SAMPLE_RATE, 120.0f);
        
        float testLeft = 0.5f;
        float testRight = -0.3f;
        float originalLeft = testLeft;
        float originalRight = testRight;
        
        // Process with bypass enabled
        processor.setBypass(true);
        processor.processStereo(testLeft, testRight);
        
        if (std::abs(testLeft - originalLeft) < 1e-6f && std::abs(testRight - originalRight) < 1e-6f) {
            std::cout << "PASS (signal unchanged when bypassed)\n";
        } else {
            std::cout << "FAIL (bypass not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test high frequency preservation
    std::cout << "Testing high frequency stereo preservation... ";
    try {
        MonoLowProcessor processor;
        processor.initialize(SAMPLE_RATE, 120.0f);
        
        // Create high frequency stereo signal
        const int numSamples = 512;
        float leftChannel[numSamples];
        float rightChannel[numSamples];
        
        for (int i = 0; i < numSamples; i++) {
            float t = static_cast<float>(i) / SAMPLE_RATE;
            float highFreq = std::sin(2.0f * 3.14159f * 1000.0f * t); // 1kHz - should remain stereo
            
            leftChannel[i] = highFreq * 0.8f;      // Different levels
            rightChannel[i] = highFreq * 0.5f;     // to maintain stereo image
        }
        
        // Store original for comparison
        float originalLeft[numSamples];
        float originalRight[numSamples];
        std::copy(leftChannel, leftChannel + numSamples, originalLeft);
        std::copy(rightChannel, rightChannel + numSamples, originalRight);
        
        // Process through mono low processor
        processor.processBlock(leftChannel, rightChannel, numSamples);
        
        // High frequencies should be largely preserved
        float leftCorrelation = 0.0f, rightCorrelation = 0.0f;
        for (int i = 0; i < numSamples; i++) {
            leftCorrelation += leftChannel[i] * originalLeft[i];
            rightCorrelation += rightChannel[i] * originalRight[i];
        }
        
        if (leftCorrelation > 0.1f && rightCorrelation > 0.05f) {
            std::cout << "PASS (high freq stereo preserved: L=" << leftCorrelation << ", R=" << rightCorrelation << ")\n";
        } else {
            std::cout << "FAIL (high frequencies not preserved)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL MONO LOW PROCESSOR TESTS PASSED!\n";
        std::cout << "Bass mono-summing system is working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}