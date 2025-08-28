#include <iostream>
#include <memory>
#include "storage/sampling/AutoSampleLoader.h"
#include "audio/RealtimeAudioBouncer.h"

int main() {
    std::cout << "EtherSynth Auto Sample Loader Test\n";
    std::cout << "===================================\n";
    
    bool allTestsPassed = true;
    
    // Test auto sample loader creation
    std::cout << "Testing AutoSampleLoader creation... ";
    try {
        AutoSampleLoader loader;
        
        auto availableSlots = loader.getAvailableSlots();
        auto occupiedSlots = loader.getOccupiedSlots();
        
        if (availableSlots.size() == 16 &&  // All 16 slots should be available initially
            occupiedSlots.size() == 0 &&    // No slots should be occupied
            loader.getTotalMemoryUsage() == 0) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization issue)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test loading options configuration
    std::cout << "Testing loading options configuration... ";
    try {
        AutoSampleLoader loader;
        
        AutoSampleLoader::SampleLoadingOptions options;
        options.strategy = AutoSampleLoader::SlotAllocationStrategy::ROUND_ROBIN;
        options.enableAutoTrim = false;
        options.targetLevel = -6.0f;
        options.nameTemplate = "Test_{slot}_{timestamp}";
        options.preferredSlot = 3;
        
        loader.setSampleLoadingOptions(options);
        const auto& retrievedOptions = loader.getSampleLoadingOptions();
        
        if (retrievedOptions.strategy == AutoSampleLoader::SlotAllocationStrategy::ROUND_ROBIN &&
            !retrievedOptions.enableAutoTrim &&
            std::abs(retrievedOptions.targetLevel - (-6.0f)) < 0.1f &&
            retrievedOptions.nameTemplate == "Test_{slot}_{timestamp}" &&
            retrievedOptions.preferredSlot == 3) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (options configuration not applied)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test slot management
    std::cout << "Testing slot management... ";
    try {
        AutoSampleLoader loader;
        
        uint8_t nextSlot = loader.findNextAvailableSlot();
        bool slot0Available = loader.isSlotAvailable(0);
        bool slot15Available = loader.isSlotAvailable(15);
        bool invalidSlot = loader.isSlotAvailable(16);  // Invalid slot
        
        if (nextSlot == 0 &&     // First available should be slot 0
            slot0Available &&
            slot15Available &&
            !invalidSlot) {      // Invalid slot should return false
            std::cout << "PASS (next slot: " << static_cast<int>(nextSlot) << ")\n";
        } else {
            std::cout << "FAIL (slot management not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test sample loading
    std::cout << "Testing sample loading... ";
    try {
        AutoSampleLoader loader;
        
        // Create mock captured audio
        auto capturedAudio = std::make_shared<RealtimeAudioBouncer::CapturedAudio>();
        capturedAudio->format.sampleRate = 48000;
        capturedAudio->format.bitDepth = 24;
        capturedAudio->format.channelCount = 2;
        capturedAudio->sampleCount = 1000;
        capturedAudio->audioData.resize(2000);  // 1000 samples × 2 channels
        
        // Fill with test data
        for (size_t i = 0; i < capturedAudio->audioData.size(); ++i) {
            capturedAudio->audioData[i] = 0.5f * std::sin(2.0f * M_PI * i / 100.0f);
        }
        
        capturedAudio->peakLevel = -6.0f;
        capturedAudio->rmsLevel = -12.0f;
        
        auto loadResult = loader.loadSample(capturedAudio, "Test Source");
        
        if (loadResult.success &&
            loadResult.assignedSlot == 0 &&  // Should get first available slot
            !loadResult.sampleName.empty() &&
            loadResult.memoryUsed > 0 &&
            !loadResult.replacedExistingSample) {
            std::cout << "PASS (loaded to slot " << static_cast<int>(loadResult.assignedSlot) << ")\n";
        } else {
            std::cout << "FAIL (sample loading failed)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test slot occupation after loading
    std::cout << "Testing slot occupation after loading... ";
    try {
        AutoSampleLoader loader;
        
        // Load a sample first
        auto capturedAudio = std::make_shared<RealtimeAudioBouncer::CapturedAudio>();
        capturedAudio->format.sampleRate = 48000;
        capturedAudio->format.channelCount = 1;
        capturedAudio->sampleCount = 500;
        capturedAudio->audioData.resize(500);
        
        auto loadResult = loader.loadSample(capturedAudio, "Test");
        
        if (loadResult.success) {
            auto availableSlots = loader.getAvailableSlots();
            auto occupiedSlots = loader.getOccupiedSlots();
            bool slot0Occupied = !loader.isSlotAvailable(0);
            
            if (availableSlots.size() == 15 &&  // 15 slots should be available
                occupiedSlots.size() == 1 &&    // 1 slot should be occupied
                slot0Occupied) {                 // Slot 0 should be occupied
                std::cout << "PASS (15 available, 1 occupied)\n";
            } else {
                std::cout << "FAIL (slot occupation tracking incorrect)\n";
                allTestsPassed = false;
            }
        } else {
            std::cout << "FAIL (sample loading failed in occupation test)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test memory usage tracking
    std::cout << "Testing memory usage tracking... ";
    try {
        AutoSampleLoader loader;
        
        size_t initialMemoryUsage = loader.getTotalMemoryUsage();
        
        // Load a sample
        auto capturedAudio = std::make_shared<RealtimeAudioBouncer::CapturedAudio>();
        capturedAudio->sampleCount = 2000;
        capturedAudio->audioData.resize(4000);  // 2000 samples × 2 channels
        
        bool hasMemoryBefore = loader.hasEnoughMemoryForSample(capturedAudio);
        auto loadResult = loader.loadSample(capturedAudio, "Memory Test");
        size_t memoryAfterLoad = loader.getTotalMemoryUsage();
        
        if (initialMemoryUsage == 0 &&
            hasMemoryBefore &&
            loadResult.success &&
            memoryAfterLoad > initialMemoryUsage &&
            memoryAfterLoad == loadResult.memoryUsed) {
            std::cout << "PASS (memory tracking: " << memoryAfterLoad << " bytes)\n";
        } else {
            std::cout << "FAIL (memory usage tracking not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test slot protection
    std::cout << "Testing slot protection... ";
    try {
        AutoSampleLoader loader;
        
        // Load a sample to slot 0
        auto capturedAudio = std::make_shared<RealtimeAudioBouncer::CapturedAudio>();
        capturedAudio->sampleCount = 100;
        capturedAudio->audioData.resize(100);
        
        auto loadResult = loader.loadSample(capturedAudio, "Protected Sample");
        
        if (loadResult.success && loadResult.assignedSlot == 0) {
            // Protect the slot
            loader.setSlotProtected(0, true);
            bool isProtected = loader.isSlotProtected(0);
            
            // Try to load to the same slot (should fail or find another slot)
            auto loadResult2 = loader.loadSampleToSlot(0, capturedAudio, "Overwrite Attempt");
            
            if (isProtected && (!loadResult2.success || loadResult2.assignedSlot != 0)) {
                std::cout << "PASS (slot protection working)\n";
            } else {
                std::cout << "FAIL (slot protection not preventing overwrite)\n";
                allTestsPassed = false;
            }
        } else {
            std::cout << "FAIL (initial sample loading failed for protection test)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test sample removal
    std::cout << "Testing sample removal... ";
    try {
        AutoSampleLoader loader;
        
        // Load samples to multiple slots
        auto capturedAudio = std::make_shared<RealtimeAudioBouncer::CapturedAudio>();
        capturedAudio->sampleCount = 100;
        capturedAudio->audioData.resize(200);  // Stereo
        
        loader.loadSample(capturedAudio, "Sample 1");
        loader.loadSample(capturedAudio, "Sample 2");
        loader.loadSample(capturedAudio, "Sample 3");
        
        auto occupiedBefore = loader.getOccupiedSlots();
        
        // Remove sample from slot 1
        bool removeResult = loader.removeSample(1);
        
        auto occupiedAfter = loader.getOccupiedSlots();
        bool slot1Available = loader.isSlotAvailable(1);
        
        if (occupiedBefore.size() == 3 &&
            removeResult &&
            occupiedAfter.size() == 2 &&
            slot1Available) {
            std::cout << "PASS (sample removal working)\n";
        } else {
            std::cout << "FAIL (sample removal not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test callback system
    std::cout << "Testing callback system... ";
    try {
        AutoSampleLoader loader;
        bool loadingCompleteCallbackCalled = false;
        
        loader.setLoadingCompleteCallback([&loadingCompleteCallbackCalled](const AutoSampleLoader::LoadingResult& result) {
            loadingCompleteCallbackCalled = true;
        });
        
        auto capturedAudio = std::make_shared<RealtimeAudioBouncer::CapturedAudio>();
        capturedAudio->sampleCount = 50;
        capturedAudio->audioData.resize(50);
        
        auto loadResult = loader.loadSample(capturedAudio, "Callback Test");
        
        if (loadResult.success && loadingCompleteCallbackCalled) {
            std::cout << "PASS (callback system working)\n";
        } else {
            std::cout << "FAIL (callback not triggered)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL AUTO SAMPLE LOADER TESTS PASSED!\n";
        std::cout << "Automatic sample loading into next available sampler slot is working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}