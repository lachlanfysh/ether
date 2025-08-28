#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "audio/RealtimeAudioBouncer.h"
#include "sequencer/PatternSelection.h"

int main() {
    std::cout << "EtherSynth Realtime Audio Bouncer Test\n";
    std::cout << "=======================================\n";
    
    bool allTestsPassed = true;
    
    // Test audio bouncer creation
    std::cout << "Testing RealtimeAudioBouncer creation... ";
    try {
        RealtimeAudioBouncer bouncer;
        
        if (bouncer.getCaptureState() == RealtimeAudioBouncer::CaptureState::IDLE &&
            !bouncer.isCaptureActive() &&
            !bouncer.hasCapturedAudio()) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization issue)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test audio format configuration
    std::cout << "Testing audio format configuration... ";
    try {
        RealtimeAudioBouncer bouncer;
        
        RealtimeAudioBouncer::AudioFormat format;
        format.sampleRate = 96000;
        format.bitDepth = 32;
        format.channelCount = 1;  // Mono
        format.maxLengthSeconds = 15.0f;
        
        bouncer.setAudioFormat(format);
        const auto& retrievedFormat = bouncer.getAudioFormat();
        
        if (retrievedFormat.sampleRate == 96000 &&
            retrievedFormat.bitDepth == 32 &&
            retrievedFormat.channelCount == 1 &&
            std::abs(retrievedFormat.maxLengthSeconds - 15.0f) < 0.1f) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (format configuration not applied)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test capture configuration
    std::cout << "Testing capture configuration... ";
    try {
        RealtimeAudioBouncer bouncer;
        
        RealtimeAudioBouncer::CaptureConfig config;
        config.selection = PatternSelection::SelectionBounds(1, 4, 2, 8);  // 4×7 selection
        config.capturePostFX = true;
        config.enableAutoGain = true;
        config.autoGainTarget = -18.0f;
        config.preRollBars = 1;
        config.postRollBars = 1;
        
        bouncer.setCaptureConfig(config);
        const auto& retrievedConfig = bouncer.getCaptureConfig();
        
        if (retrievedConfig.selection.getTrackCount() == 4 &&
            retrievedConfig.selection.getStepCount() == 7 &&
            retrievedConfig.capturePostFX &&
            retrievedConfig.enableAutoGain &&
            std::abs(retrievedConfig.autoGainTarget - (-18.0f)) < 0.1f) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (capture configuration not applied)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test memory requirements
    std::cout << "Testing memory requirements... ";
    try {
        RealtimeAudioBouncer bouncer;
        
        RealtimeAudioBouncer::CaptureConfig config;
        config.selection = PatternSelection::SelectionBounds(0, 3, 0, 15);  // 4×16 selection
        
        bouncer.setCaptureConfig(config);
        size_t memoryUsage = bouncer.getEstimatedMemoryUsage();
        bool hasEnoughMemory = bouncer.hasEnoughMemoryForCapture(config);
        
        if (memoryUsage > 0 && hasEnoughMemory) {
            std::cout << "PASS (estimated " << memoryUsage << " bytes)\n";
        } else {
            std::cout << "FAIL (memory calculation issue)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test capture lifecycle
    std::cout << "Testing capture lifecycle... ";
    try {
        RealtimeAudioBouncer bouncer;
        bool progressCallbackCalled = false;
        
        bouncer.setProgressCallback([&progressCallbackCalled](const RealtimeAudioBouncer::CaptureProgress& progress) {
            progressCallbackCalled = true;
        });
        
        RealtimeAudioBouncer::CaptureConfig config;
        config.selection = PatternSelection::SelectionBounds(0, 1, 0, 3);  // Small 2×4 selection
        
        bool startResult = bouncer.startCapture(config);
        
        if (startResult && bouncer.isCaptureActive()) {
            // Simulate capture start
            bouncer.notifyRegionStart();
            
            if (bouncer.getCaptureState() == RealtimeAudioBouncer::CaptureState::CAPTURING) {
                // Cancel capture
                bouncer.cancelCapture();
                
                if (bouncer.getCaptureState() == RealtimeAudioBouncer::CaptureState::CANCELLED &&
                    progressCallbackCalled) {
                    std::cout << "PASS (capture lifecycle working)\n";
                } else {
                    std::cout << "FAIL (cancel or progress callback not working)\n";
                    allTestsPassed = false;
                }
            } else {
                std::cout << "FAIL (region start notification not working)\n";
                allTestsPassed = false;
            }
        } else {
            std::cout << "FAIL (capture start failed)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test audio processing
    std::cout << "Testing audio processing... ";
    try {
        RealtimeAudioBouncer bouncer;
        
        RealtimeAudioBouncer::CaptureConfig config;
        config.selection = PatternSelection::SelectionBounds(0, 0, 0, 1);  // Minimal selection
        
        if (bouncer.startCapture(config)) {
            bouncer.notifyRegionStart();
            
            // Generate test audio buffer
            const uint32_t sampleCount = 64;
            const uint8_t channelCount = 2;
            std::vector<float> testBuffer(sampleCount * channelCount);
            
            // Fill with sine wave
            for (uint32_t i = 0; i < sampleCount; ++i) {
                float sample = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 48000.0f);
                testBuffer[i * channelCount] = sample;      // Left
                testBuffer[i * channelCount + 1] = sample;  // Right
            }
            
            // Process audio block
            bouncer.processAudioBlock(testBuffer.data(), sampleCount, channelCount);
            
            auto progress = bouncer.getCaptureProgress();
            
            if (progress.capturedSamples == sampleCount &&
                progress.currentPeakLevel > -96.0f) {  // Should detect audio
                std::cout << "PASS (audio processing working)\n";
            } else {
                std::cout << "FAIL (audio processing not working)\n";
                allTestsPassed = false;
            }
            
            bouncer.cancelCapture();
        } else {
            std::cout << "FAIL (capture start failed for audio processing test)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test progress monitoring
    std::cout << "Testing progress monitoring... ";
    try {
        RealtimeAudioBouncer bouncer;
        
        auto initialProgress = bouncer.getCaptureProgress();
        
        if (initialProgress.state == RealtimeAudioBouncer::CaptureState::IDLE &&
            initialProgress.progressPercent == 0.0f &&
            initialProgress.capturedSamples == 0) {
            
            // Start capture to test progress updates
            RealtimeAudioBouncer::CaptureConfig config;
            config.selection = PatternSelection::SelectionBounds(0, 0, 0, 7);  // 1×8 selection
            
            if (bouncer.startCapture(config)) {
                auto activeProgress = bouncer.getCaptureProgress();
                
                if (activeProgress.state != RealtimeAudioBouncer::CaptureState::IDLE &&
                    activeProgress.totalExpectedSamples > 0) {
                    std::cout << "PASS (progress monitoring working)\n";
                } else {
                    std::cout << "FAIL (active progress not working)\n";
                    allTestsPassed = false;
                }
                
                bouncer.cancelCapture();
            } else {
                std::cout << "FAIL (capture start failed for progress test)\n";
                allTestsPassed = false;
            }
        } else {
            std::cout << "FAIL (initial progress state incorrect)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test error conditions
    std::cout << "Testing error conditions... ";
    try {
        RealtimeAudioBouncer bouncer;
        bool errorCallbackCalled = false;
        
        bouncer.setErrorCallback([&errorCallbackCalled](const std::string& error) {
            errorCallbackCalled = true;
        });
        
        // Try to start capture with invalid selection
        RealtimeAudioBouncer::CaptureConfig invalidConfig;
        invalidConfig.selection = PatternSelection::SelectionBounds();  // Invalid selection
        
        bool startResult = bouncer.startCapture(invalidConfig);
        
        if (!startResult || errorCallbackCalled) {
            std::cout << "PASS (error handling working)\n";
        } else {
            std::cout << "FAIL (error handling not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL REALTIME AUDIO BOUNCER TESTS PASSED!\n";
        std::cout << "Real-time audio bouncing system for selected regions is working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}