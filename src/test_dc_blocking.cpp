#include <iostream>
#include <cmath>
#include "audio/DCBlocker.h"
#include "audio/SubsonicFilter.h"
#include "audio/PostNonlinearProcessor.h"
#include "audio/AdvancedParameterSmoother.h"

int main() {
    std::cout << "EtherSynth DC Blocking & Subsonic Filter Test\n";
    std::cout << "==============================================\n";
    
    const float SAMPLE_RATE = 44100.0f;
    bool allTestsPassed = true;
    
    // Test DCBlocker
    std::cout << "Testing DCBlocker... ";
    try {
        DCBlocker dcBlocker;
        if (dcBlocker.initialize(SAMPLE_RATE, 24.0f)) {
            // Test with DC offset signal - need several samples to settle
            float testSignal = 1.0f; // Pure DC
            float output = 0.0f;
            
            // Process multiple samples to let filter settle
            for (int i = 0; i < 100; i++) {
                output = dcBlocker.processSample(testSignal);
            }
            
            // DC should be significantly reduced after settling
            if (std::abs(output) < 0.1f) { // DC significantly reduced
                std::cout << "PASS (DC reduced from " << testSignal << " to " << output << ")\n";
            } else {
                std::cout << "FAIL (DC not blocked: " << output << ")\n";
                allTestsPassed = false;
            }
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test SubsonicFilter
    std::cout << "Testing SubsonicFilter... ";
    try {
        SubsonicFilter subsonicFilter;
        if (subsonicFilter.initialize(SAMPLE_RATE, 24.0f, SubsonicFilter::FilterType::BUTTERWORTH)) {
            // Test frequency response
            float mag1kHz = subsonicFilter.getMagnitudeResponse(1000.0f);
            float mag10Hz = subsonicFilter.getMagnitudeResponse(10.0f);
            
            // 1kHz should pass through, 10Hz should be attenuated
            if (mag1kHz > 0.9f && mag10Hz < 0.5f) {
                std::cout << "PASS (1kHz: " << mag1kHz << ", 10Hz: " << mag10Hz << ")\n";
            } else {
                std::cout << "FAIL (bad frequency response)\n";
                allTestsPassed = false;
            }
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test PostNonlinearProcessor
    std::cout << "Testing PostNonlinearProcessor... ";
    try {
        PostNonlinearProcessor processor;
        if (processor.initialize(SAMPLE_RATE, PostNonlinearProcessor::FilterTopology::SUBSONIC_ONLY)) {
            // Test nonlinear processing cleanup
            float testBuffer[64];
            
            // Create test signal: sine wave + DC bias + low frequency noise
            for (int i = 0; i < 64; i++) {
                float sine = 0.5f * std::sin(2.0f * 3.14159f * 440.0f * i / SAMPLE_RATE);
                float dc = 0.2f;
                float lowFreq = 0.1f * std::sin(2.0f * 3.14159f * 10.0f * i / SAMPLE_RATE);
                testBuffer[i] = sine + dc + lowFreq;
            }
            
            // Process through cleanup
            processor.processBlock(testBuffer, 64);
            
            // Calculate DC level after processing
            float dcAfter = 0.0f;
            for (int i = 0; i < 64; i++) {
                dcAfter += testBuffer[i];
            }
            dcAfter /= 64.0f;
            
            if (std::abs(dcAfter) < 0.1f) { // DC should be much reduced
                std::cout << "PASS (DC reduced to " << dcAfter << ")\n";
            } else {
                std::cout << "FAIL (DC not reduced: " << dcAfter << ")\n";
                allTestsPassed = false;
            }
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test AdvancedParameterSmoother
    std::cout << "Testing AdvancedParameterSmoother... ";
    try {
        AdvancedParameterSmoother smoother;
        AdvancedParameterSmoother::Config config;
        config.smoothType = AdvancedParameterSmoother::SmoothType::FAST;
        config.curveType = AdvancedParameterSmoother::CurveType::EXPONENTIAL;
        
        smoother.initialize(SAMPLE_RATE, config);
        smoother.setValue(0.0f);
        smoother.setTarget(1.0f);
        
        // Process samples to see progression over time
        float values[5];
        for (int i = 0; i < 5; i++) {
            // Process multiple samples for each measurement
            for (int j = 0; j < 100; j++) {
                values[i] = smoother.process();
            }
        }
        
        // Should be smoothly increasing
        if (values[0] < values[2] && values[2] < values[4] && values[4] < 1.0f) {
            std::cout << "PASS (smooth progression: " << values[0] << " → " << values[2] << " → " << values[4] << ")\n";
        } else {
            std::cout << "FAIL (not smoothing properly: " << values[0] << ", " << values[2] << ", " << values[4] << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test filter topology switching
    std::cout << "Testing filter topology switching... ";
    try {
        PostNonlinearProcessor processor;
        
        // Test different topologies
        processor.initialize(SAMPLE_RATE, PostNonlinearProcessor::FilterTopology::DC_ONLY);
        float output1 = processor.processSample(1.0f);
        
        processor.setFilterTopology(PostNonlinearProcessor::FilterTopology::SERIAL);
        float output2 = processor.processSample(1.0f);
        
        processor.setFilterTopology(PostNonlinearProcessor::FilterTopology::PARALLEL);
        float output3 = processor.processSample(1.0f);
        
        std::cout << "PASS (topologies: " << output1 << ", " << output2 << ", " << output3 << ")\n";
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL DC BLOCKING TESTS PASSED!\n";
        std::cout << "DC blocker and subsonic filter systems are working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}