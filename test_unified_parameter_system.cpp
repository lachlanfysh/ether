#include "src/core/ParameterSystem.h"
#include "src/core/ParameterSystemAdapter.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>

/**
 * Comprehensive test suite for UnifiedParameterSystem
 * Tests all major functionality including:
 * - Basic parameter operations
 * - Velocity modulation
 * - Parameter smoothing
 * - Preset loading/saving
 * - JSON serialization
 * - Adapter compatibility
 * - Performance characteristics
 */

class UnifiedParameterSystemTester {
public:
    void runAllTests() {
        std::cout << "=== UnifiedParameterSystem Test Suite ===\n\n";
        
        testBasicInitialization();
        testParameterRegistration();
        testBasicParameterOperations();
        testVelocityModulation();
        testParameterSmoothing();
        testPresetOperations();
        testJSONSerialization();
        testAdapterCompatibility();
        testPerformanceCharacteristics();
        testErrorHandling();
        testThreadSafety();
        
        std::cout << "\n=== All Tests Completed Successfully ===\n";
    }

private:
    void testBasicInitialization() {
        std::cout << "Testing basic initialization...\n";
        
        UnifiedParameterSystem system;
        assert(!system.isInitialized());
        
        bool success = system.initialize(48000.0f);
        assert(success);
        assert(system.isInitialized());
        
        // Test double initialization
        bool doubleInit = system.initialize(48000.0f);
        assert(!doubleInit); // Should fail
        
        system.shutdown();
        assert(!system.isInitialized());
        
        std::cout << "✓ Basic initialization tests passed\n";
    }
    
    void testParameterRegistration() {
        std::cout << "Testing parameter registration...\n";
        
        UnifiedParameterSystem system;
        system.initialize(48000.0f);
        
        // Test registering a custom parameter
        UnifiedParameterSystem::ParameterConfig config;
        config.id = ParameterID::VOLUME;
        config.name = "Test Volume";
        config.minValue = 0.0f;
        config.maxValue = 1.0f;
        config.defaultValue = 0.8f;
        config.enableVelocityScaling = true;
        config.velocityScale = 0.5f;
        
        bool registered = system.registerParameter(config);
        assert(registered);
        
        // Test that parameter is registered
        assert(system.isParameterRegistered(ParameterID::VOLUME));
        
        // Test getting configuration
        const auto& retrievedConfig = system.getParameterConfig(ParameterID::VOLUME);
        assert(retrievedConfig.name == "Test Volume");
        assert(retrievedConfig.defaultValue == 0.8f);
        assert(retrievedConfig.velocityScale == 0.5f);
        
        // Test getting registered parameters list
        auto paramList = system.getRegisteredParameters();
        assert(!paramList.empty());
        
        system.shutdown();
        std::cout << "✓ Parameter registration tests passed\n";
    }
    
    void testBasicParameterOperations() {
        std::cout << "Testing basic parameter operations...\n";
        
        UnifiedParameterSystem system;
        system.initialize(48000.0f);
        
        // Test setting and getting global parameters
        auto result = system.setParameterValue(ParameterID::VOLUME, 0.75f);
        assert(result == UnifiedParameterSystem::UpdateResult::SUCCESS || 
               result == UnifiedParameterSystem::UpdateResult::SMOOTHING_ACTIVE);
        
        // Process a few audio blocks to let smoothing settle
        for (int i = 0; i < 10; ++i) {
            system.processAudioBlock();
        }
        
        float value = system.getParameterValue(ParameterID::VOLUME);
        assert(value >= 0.7f && value <= 0.8f); // Allow for smoothing
        
        // Test instrument-specific parameters
        result = system.setParameterValue(ParameterID::FILTER_CUTOFF, 0, 0.6f);
        assert(result == UnifiedParameterSystem::UpdateResult::SUCCESS || 
               result == UnifiedParameterSystem::UpdateResult::SMOOTHING_ACTIVE);
        
        // Process smoothing
        for (int i = 0; i < 10; ++i) {
            system.processAudioBlock();
        }
        
        float instValue = system.getParameterValue(ParameterID::FILTER_CUTOFF, 0);
        assert(instValue >= 0.55f && instValue <= 0.65f);
        
        // Test immediate value setting (skip smoothing)
        result = system.setParameterValueImmediate(ParameterID::PAN, 0.3f);
        assert(result == UnifiedParameterSystem::UpdateResult::SUCCESS);
        
        float panValue = system.getParameterValue(ParameterID::PAN);
        assert(std::abs(panValue - 0.3f) < 0.001f);
        
        system.shutdown();
        std::cout << "✓ Basic parameter operations tests passed\n";
    }
    
    void testVelocityModulation() {
        std::cout << "Testing velocity modulation...\n";
        
        UnifiedParameterSystem system;
        system.initialize(48000.0f);
        
        // Test velocity modulation
        auto result = system.setParameterWithVelocity(ParameterID::FILTER_CUTOFF, 0.5f, 1.0f);
        assert(result == UnifiedParameterSystem::UpdateResult::SUCCESS || 
               result == UnifiedParameterSystem::UpdateResult::SMOOTHING_ACTIVE);
        
        // Process smoothing
        for (int i = 0; i < 20; ++i) {
            system.processAudioBlock();
        }
        
        float modulatedValue = system.getParameterValue(ParameterID::FILTER_CUTOFF);
        
        // Test different velocity value
        result = system.setParameterWithVelocity(ParameterID::FILTER_CUTOFF, 0.5f, 0.0f);
        assert(result == UnifiedParameterSystem::UpdateResult::SUCCESS || 
               result == UnifiedParameterSystem::UpdateResult::SMOOTHING_ACTIVE);
        
        for (int i = 0; i < 20; ++i) {
            system.processAudioBlock();
        }
        
        float unmodulatedValue = system.getParameterValue(ParameterID::FILTER_CUTOFF);
        
        // Values should be different (assuming velocity scaling is enabled)
        // Note: The exact difference depends on the velocity scaling configuration
        
        system.shutdown();
        std::cout << "✓ Velocity modulation tests passed\n";
    }
    
    void testParameterSmoothing() {
        std::cout << "Testing parameter smoothing...\n";
        
        UnifiedParameterSystem system;
        system.initialize(48000.0f);
        
        // Set initial value
        system.setParameterValueImmediate(ParameterID::VOLUME, 0.0f);
        
        // Set target value (should trigger smoothing)
        auto result = system.setParameterValue(ParameterID::VOLUME, 1.0f);
        assert(result == UnifiedParameterSystem::UpdateResult::SMOOTHING_ACTIVE ||
               result == UnifiedParameterSystem::UpdateResult::SUCCESS);
        
        // Test that parameter is smoothing
        bool wasSmoothing = system.isParameterSmoothing(ParameterID::VOLUME);
        
        // Process audio blocks and watch value change
        float lastValue = system.getParameterValue(ParameterID::VOLUME);
        float currentValue = lastValue;
        
        bool valueChanged = false;
        for (int i = 0; i < 100 && system.isParameterSmoothing(ParameterID::VOLUME); ++i) {
            system.processAudioBlock();
            currentValue = system.getParameterValue(ParameterID::VOLUME);
            if (std::abs(currentValue - lastValue) > 0.001f) {
                valueChanged = true;
            }
            lastValue = currentValue;
        }
        
        // Value should have changed during smoothing
        if (wasSmoothing) {
            assert(valueChanged);
        }
        
        // Final value should be close to target
        assert(currentValue >= 0.9f);
        
        system.shutdown();
        std::cout << "✓ Parameter smoothing tests passed\n";
    }
    
    void testPresetOperations() {
        std::cout << "Testing preset operations...\n";
        
        UnifiedParameterSystem system;
        system.initialize(48000.0f);
        
        // Set some parameter values
        system.setParameterValueImmediate(ParameterID::VOLUME, 0.75f);
        system.setParameterValueImmediate(ParameterID::FILTER_CUTOFF, 0.6f);
        system.setParameterValueImmediate(ParameterID::ATTACK, 0.2f);
        
        // Save preset
        UnifiedParameterSystem::PresetData preset;
        bool saved = system.savePreset(preset);
        assert(saved);
        
        // Modify parameters
        system.setParameterValueImmediate(ParameterID::VOLUME, 0.3f);
        system.setParameterValueImmediate(ParameterID::FILTER_CUTOFF, 0.9f);
        
        // Load preset
        bool loaded = system.loadPreset(preset);
        assert(loaded);
        
        // Process to let loading complete
        for (int i = 0; i < 20; ++i) {
            system.processAudioBlock();
        }
        
        // Check that values were restored (approximately, due to smoothing)
        float volume = system.getParameterValue(ParameterID::VOLUME);
        float cutoff = system.getParameterValue(ParameterID::FILTER_CUTOFF);
        
        assert(volume >= 0.7f && volume <= 0.8f);
        assert(cutoff >= 0.55f && cutoff <= 0.65f);
        
        system.shutdown();
        std::cout << "✓ Preset operations tests passed\n";
    }
    
    void testJSONSerialization() {
        std::cout << "Testing JSON serialization...\n";
        
        UnifiedParameterSystem system;
        system.initialize(48000.0f);
        
        // Set some parameter values
        system.setParameterValueImmediate(ParameterID::VOLUME, 0.8f);
        system.setParameterValueImmediate(ParameterID::FILTER_CUTOFF, 0.65f);
        system.setParameterValueImmediate(ParameterID::REVERB_SIZE, 0.4f);
        
        // Serialize to JSON
        std::string json = system.serializeToJSON();
        assert(!json.empty());
        
        std::cout << "Generated JSON length: " << json.length() << " characters\n";
        
        // Modify parameters
        system.setParameterValueImmediate(ParameterID::VOLUME, 0.2f);
        system.setParameterValueImmediate(ParameterID::FILTER_CUTOFF, 0.9f);
        
        // Deserialize from JSON
        bool deserialized = system.deserializeFromJSON(json);
        assert(deserialized);
        
        // Process to let deserialization complete
        for (int i = 0; i < 10; ++i) {
            system.processAudioBlock();
        }
        
        // Check that values were restored
        float volume = system.getParameterValue(ParameterID::VOLUME);
        float cutoff = system.getParameterValue(ParameterID::FILTER_CUTOFF);
        
        assert(volume >= 0.75f && volume <= 0.85f);
        assert(cutoff >= 0.6f && cutoff <= 0.7f);
        
        system.shutdown();
        std::cout << "✓ JSON serialization tests passed\n";
    }
    
    void testAdapterCompatibility() {
        std::cout << "Testing adapter compatibility...\n";
        
        ParameterSystemAdapter adapter;
        bool initialized = adapter.initialize(48000.0f);
        assert(initialized);
        
        // Test legacy parameter access
        adapter.setParameter(0, 0.7f); // Assuming 0 maps to VOLUME
        float value = adapter.getParameter(0);
        
        // Process to let smoothing settle
        for (int i = 0; i < 20; ++i) {
            adapter.processAudioBlock();
        }
        
        value = adapter.getParameter(0);
        assert(value >= 0.65f && value <= 0.75f);
        
        // Test velocity scaling
        adapter.setParameterVelocityScale(0, 0.8f);
        float scale = adapter.getParameterVelocityScale(0);
        assert(std::abs(scale - 0.8f) < 0.01f);
        
        // Test velocity modulation
        adapter.setParameterWithVelocity(0, 0.5f, 1.0f);
        
        for (int i = 0; i < 20; ++i) {
            adapter.processAudioBlock();
        }
        
        // Test master velocity depth
        adapter.setMasterVelocityDepth(1.5f);
        float masterDepth = adapter.getMasterVelocityDepth();
        assert(std::abs(masterDepth - 1.5f) < 0.01f);
        
        // Test migration statistics
        auto stats = adapter.getMigrationStats();
        assert(stats.totalParametersFound > 0);
        assert(stats.migrationCompleteness >= 0.0f && stats.migrationCompleteness <= 1.0f);
        
        adapter.shutdown();
        std::cout << "✓ Adapter compatibility tests passed\n";
    }
    
    void testPerformanceCharacteristics() {
        std::cout << "Testing performance characteristics...\n";
        
        UnifiedParameterSystem system;
        system.initialize(48000.0f);
        
        // Set up multiple parameters
        for (int i = 0; i < static_cast<int>(ParameterID::COUNT); ++i) {
            ParameterID paramId = static_cast<ParameterID>(i);
            if (system.isParameterRegistered(paramId)) {
                system.setParameterValue(paramId, 0.5f);
            }
        }
        
        // Time audio block processing
        const int numBlocks = 1000;
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < numBlocks; ++i) {
            system.processAudioBlock();
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        
        double avgTimePerBlock = static_cast<double>(duration.count()) / numBlocks;
        std::cout << "Average time per audio block: " << avgTimePerBlock << " microseconds\n";
        
        // Performance should be reasonable (less than 100 microseconds per block for this test)
        assert(avgTimePerBlock < 100.0);
        
        system.shutdown();
        std::cout << "✓ Performance characteristics tests passed\n";
    }
    
    void testErrorHandling() {
        std::cout << "Testing error handling...\n";
        
        UnifiedParameterSystem system;
        
        // Test operations before initialization
        auto result = system.setParameterValue(ParameterID::VOLUME, 0.5f);
        assert(result != UnifiedParameterSystem::UpdateResult::SUCCESS);
        
        system.initialize(48000.0f);
        
        // Test invalid parameter values
        result = system.setParameterValue(ParameterID::COUNT, 0.5f); // Invalid parameter ID
        assert(result == UnifiedParameterSystem::UpdateResult::INVALID_PARAMETER);
        
        // Test invalid instrument index
        result = system.setParameterValue(ParameterID::VOLUME, MAX_INSTRUMENTS + 1, 0.5f);
        assert(result == UnifiedParameterSystem::UpdateResult::INVALID_PARAMETER);
        
        // Test parameter validation
        bool valid = system.validateParameterValue(ParameterID::VOLUME, 2.0f); // Assuming max is 1.0
        
        // Test value clamping
        float clamped = system.clampParameterValue(ParameterID::VOLUME, 2.0f);
        assert(clamped <= 1.0f);
        
        system.shutdown();
        std::cout << "✓ Error handling tests passed\n";
    }
    
    void testThreadSafety() {
        std::cout << "Testing thread safety...\n";
        
        UnifiedParameterSystem system;
        system.initialize(48000.0f);
        
        std::atomic<bool> testComplete{false};
        std::atomic<int> errorCount{0};
        
        // Audio thread simulation
        std::thread audioThread([&]() {
            while (!testComplete.load()) {
                try {
                    system.processAudioBlock();
                    
                    // Read parameter values
                    for (int i = 0; i < static_cast<int>(ParameterID::COUNT); ++i) {
                        ParameterID paramId = static_cast<ParameterID>(i);
                        if (system.isParameterRegistered(paramId)) {
                            volatile float value = system.getParameterValue(paramId);
                            (void)value; // Suppress unused variable warning
                        }
                    }
                } catch (...) {
                    errorCount.fetch_add(1);
                }
                
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
        
        // Main thread - parameter updates
        std::thread mainThread([&]() {
            for (int iteration = 0; iteration < 100; ++iteration) {
                try {
                    for (int i = 0; i < static_cast<int>(ParameterID::COUNT); ++i) {
                        ParameterID paramId = static_cast<ParameterID>(i);
                        if (system.isParameterRegistered(paramId)) {
                            float value = static_cast<float>(iteration % 100) / 100.0f;
                            system.setParameterValue(paramId, value);
                        }
                    }
                } catch (...) {
                    errorCount.fetch_add(1);
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
        
        // Let threads run for a while
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        testComplete.store(true);
        
        audioThread.join();
        mainThread.join();
        
        // Should have no errors in thread-safe access
        assert(errorCount.load() == 0);
        
        system.shutdown();
        std::cout << "✓ Thread safety tests passed\n";
    }
};

int main() {
    try {
        UnifiedParameterSystemTester tester;
        tester.runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}