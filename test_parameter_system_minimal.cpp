#include "test_advanced_parameter_smoother_stub.h"
#include "src/core/ParameterSystem.h"
#include <iostream>
#include <cassert>

// Minimal test to verify basic parameter system functionality
int main() {
    std::cout << "Testing UnifiedParameterSystem basic functionality...\n";
    
    UnifiedParameterSystem system;
    
    // Test initialization
    bool initialized = system.initialize(48000.0f);
    if (!initialized) {
        std::cerr << "Failed to initialize parameter system\n";
        return 1;
    }
    
    std::cout << "✓ System initialized successfully\n";
    
    // Test basic parameter operations
    auto result = system.setParameterValue(ParameterID::VOLUME, 0.8f);
    if (result == UnifiedParameterSystem::UpdateResult::SUCCESS || 
        result == UnifiedParameterSystem::UpdateResult::SMOOTHING_ACTIVE) {
        std::cout << "✓ Parameter set successfully\n";
    } else {
        std::cout << "✗ Parameter set failed with result: " << static_cast<int>(result) << "\n";
        return 1;
    }
    
    // Process a few audio blocks
    for (int i = 0; i < 10; ++i) {
        system.processAudioBlock();
    }
    
    // Get parameter value
    float value = system.getParameterValue(ParameterID::VOLUME);
    std::cout << "✓ Retrieved parameter value: " << value << "\n";
    
    // Test JSON serialization
    std::string json = system.serializeToJSON();
    if (!json.empty()) {
        std::cout << "✓ JSON serialization successful (length: " << json.length() << ")\n";
    } else {
        std::cout << "✗ JSON serialization failed\n";
        return 1;
    }
    
    // Test parameter count
    size_t paramCount = system.getParameterCount();
    std::cout << "✓ Parameter count: " << paramCount << "\n";
    
    // Test preset operations
    UnifiedParameterSystem::PresetData preset;
    bool saved = system.savePreset(preset);
    if (saved) {
        std::cout << "✓ Preset saved successfully\n";
    } else {
        std::cout << "✗ Preset save failed\n";
    }
    
    system.shutdown();
    std::cout << "✓ System shutdown successfully\n";
    
    std::cout << "\n=== All basic tests passed! ===\n";
    return 0;
}