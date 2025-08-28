#include "src/control/velocity/EngineVelocityMapping.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <cmath>

/**
 * Comprehensive test for EngineVelocityMapping
 * Tests engine-specific velocity parameter mapping
 */

void testBasicEngineConfiguration() {
    std::cout << "Testing basic engine configuration...\n";
    
    EngineVelocityMapping mapper;
    
    // Test initial state
    assert(mapper.isEnabled() == true);
    assert(mapper.getActiveEngineCount() == 0);
    
    // Create and set engine configuration
    uint32_t engineId = 1000;
    EngineVelocityMapping::EngineVelocityConfig config;
    config.engineType = EngineVelocityMapping::EngineType::MACRO_VA;
    config.configName = "Test VA Config";
    config.description = "Test configuration for VA engine";
    
    mapper.setEngineConfig(engineId, config);
    assert(mapper.hasEngineConfig(engineId));
    assert(mapper.getActiveEngineCount() == 1);
    
    const auto& retrievedConfig = mapper.getEngineConfig(engineId);
    assert(retrievedConfig.engineType == EngineVelocityMapping::EngineType::MACRO_VA);
    assert(retrievedConfig.configName == "Test VA Config");
    
    // Test engine removal
    mapper.removeEngineConfig(engineId);
    assert(!mapper.hasEngineConfig(engineId));
    assert(mapper.getActiveEngineCount() == 0);
    
    std::cout << "✓ Basic engine configuration tests passed\n";
}

void testVelocityMappingConfiguration() {
    std::cout << "Testing velocity mapping configuration...\n";
    
    EngineVelocityMapping mapper;
    uint32_t engineId = 2000;
    
    // Create configuration with velocity mappings
    EngineVelocityMapping::EngineVelocityConfig config;
    config.engineType = EngineVelocityMapping::EngineType::MACRO_VA;
    
    // Add volume mapping
    EngineVelocityMapping::VelocityMapping volumeMapping;
    volumeMapping.target = EngineVelocityMapping::VelocityTarget::VOLUME;
    volumeMapping.enabled = true;
    volumeMapping.baseValue = 0.2f;
    volumeMapping.velocityAmount = 0.8f;
    volumeMapping.curveType = RelativeVelocityModulation::CurveType::EXPONENTIAL;
    volumeMapping.curveAmount = 1.5f;
    config.mappings.push_back(volumeMapping);
    
    // Add filter cutoff mapping
    EngineVelocityMapping::VelocityMapping filterMapping;
    filterMapping.target = EngineVelocityMapping::VelocityTarget::FILTER_CUTOFF;
    filterMapping.enabled = true;
    filterMapping.baseValue = 0.5f;
    filterMapping.velocityAmount = 0.4f;
    filterMapping.curveType = RelativeVelocityModulation::CurveType::LINEAR;
    config.mappings.push_back(filterMapping);
    
    mapper.setEngineConfig(engineId, config);
    
    const auto& retrievedConfig = mapper.getEngineConfig(engineId);
    assert(retrievedConfig.mappings.size() == 2);
    assert(retrievedConfig.mappings[0].target == EngineVelocityMapping::VelocityTarget::VOLUME);
    assert(retrievedConfig.mappings[1].target == EngineVelocityMapping::VelocityTarget::FILTER_CUTOFF);
    
    std::cout << "✓ Velocity mapping configuration tests passed\n";
}

void testParameterMapping() {
    std::cout << "Testing parameter mapping...\n";
    
    EngineVelocityMapping mapper;
    uint32_t engineId = 3000;
    
    // Set up engine with mappings
    EngineVelocityMapping::EngineVelocityConfig config;
    config.engineType = EngineVelocityMapping::EngineType::MACRO_VA;
    
    EngineVelocityMapping::VelocityMapping mapping;
    mapping.target = EngineVelocityMapping::VelocityTarget::VOLUME;
    mapping.enabled = true;
    mapping.baseValue = 0.0f;
    mapping.velocityAmount = 1.0f;
    mapping.curveType = RelativeVelocityModulation::CurveType::LINEAR;
    mapping.minValue = 0.0f;
    mapping.maxValue = 1.0f;
    config.mappings.push_back(mapping);
    
    mapper.setEngineConfig(engineId, config);
    
    // Test velocity mapping calculations
    float lowVelResult = mapper.mapVelocityToParameter(mapping, 32);   // Low velocity
    float midVelResult = mapper.mapVelocityToParameter(mapping, 64);   // Mid velocity  
    float highVelResult = mapper.mapVelocityToParameter(mapping, 127); // High velocity
    
    assert(lowVelResult < midVelResult);
    assert(midVelResult < highVelResult);
    assert(lowVelResult >= 0.0f && lowVelResult <= 1.0f);
    assert(highVelResult >= 0.0f && highVelResult <= 1.0f);
    
    // Test single parameter update
    auto result = mapper.updateSingleParameter(engineId, EngineVelocityMapping::VelocityTarget::VOLUME, 0.5f, 100);
    assert(result.target == EngineVelocityMapping::VelocityTarget::VOLUME);
    assert(result.wasUpdated == true);
    assert(result.modulatedValue >= 0.0f && result.modulatedValue <= 1.0f);
    
    std::cout << "✓ Parameter mapping tests passed\n";
}

void testEngineSpecificMappings() {
    std::cout << "Testing engine-specific mappings...\n";
    
    EngineVelocityMapping mapper;
    
    // Test different engine types have different available targets
    auto vaTargets = mapper.getEngineTargets(EngineVelocityMapping::EngineType::MACRO_VA);
    auto fmTargets = mapper.getEngineTargets(EngineVelocityMapping::EngineType::MACRO_FM);
    auto harmTargets = mapper.getEngineTargets(EngineVelocityMapping::EngineType::MACRO_HARMONICS);
    auto wtTargets = mapper.getEngineTargets(EngineVelocityMapping::EngineType::MACRO_WAVETABLE);
    
    // All should have universal targets
    assert(std::find(vaTargets.begin(), vaTargets.end(), EngineVelocityMapping::VelocityTarget::VOLUME) != vaTargets.end());
    assert(std::find(fmTargets.begin(), fmTargets.end(), EngineVelocityMapping::VelocityTarget::FILTER_CUTOFF) != fmTargets.end());
    
    // VA should have envelope targets
    assert(std::find(vaTargets.begin(), vaTargets.end(), EngineVelocityMapping::VelocityTarget::ENV_ATTACK) != vaTargets.end());
    
    // FM should have modulation targets
    assert(std::find(fmTargets.begin(), fmTargets.end(), EngineVelocityMapping::VelocityTarget::FM_MOD_INDEX) != fmTargets.end());
    
    // Harmonics should have organ targets
    assert(std::find(harmTargets.begin(), harmTargets.end(), EngineVelocityMapping::VelocityTarget::HARM_PERCUSSION_LEVEL) != harmTargets.end());
    
    // Wavetable should have WT targets
    assert(std::find(wtTargets.begin(), wtTargets.end(), EngineVelocityMapping::VelocityTarget::WT_POSITION) != wtTargets.end());
    
    std::cout << "✓ Engine-specific mappings tests passed\n";
}

void testVoiceManagement() {
    std::cout << "Testing voice management...\n";
    
    EngineVelocityMapping mapper;
    uint32_t engineId = 4000;
    
    // Set up engine configuration
    EngineVelocityMapping::EngineVelocityConfig config;
    config.engineType = EngineVelocityMapping::EngineType::MACRO_VA;
    
    EngineVelocityMapping::VelocityMapping mapping;
    mapping.target = EngineVelocityMapping::VelocityTarget::VOLUME;
    mapping.enabled = true;
    mapping.baseValue = 0.0f;
    mapping.velocityAmount = 1.0f;
    config.mappings.push_back(mapping);
    
    mapper.setEngineConfig(engineId, config);
    
    // Test voice addition
    uint32_t voice1 = 5000;
    uint32_t voice2 = 5001;
    uint32_t voice3 = 5002;
    
    mapper.addEngineVoice(engineId, voice1, 64);
    mapper.addEngineVoice(engineId, voice2, 80);
    mapper.addEngineVoice(engineId, voice3, 100);
    
    assert(mapper.getActiveVoiceCount(engineId) == 3);
    assert(mapper.getTotalActiveVoices() == 3);
    
    // Test voice velocity update
    mapper.updateEngineVoiceVelocity(engineId, voice1, 120);
    
    // Test voice removal
    mapper.removeEngineVoice(engineId, voice2);
    assert(mapper.getActiveVoiceCount(engineId) == 2);
    
    // Test clear all voices
    mapper.clearAllEngineVoices(engineId);
    assert(mapper.getActiveVoiceCount(engineId) == 0);
    
    std::cout << "✓ Voice management tests passed\n";
}

void testEngineParameterUpdates() {
    std::cout << "Testing engine parameter updates...\n";
    
    EngineVelocityMapping mapper;
    uint32_t engineId = 6000;
    uint32_t voiceId = 7000;
    
    // Set up VA engine with multiple parameters
    EngineVelocityMapping::EngineVelocityConfig config;
    config.engineType = EngineVelocityMapping::EngineType::MACRO_VA;
    
    // Volume mapping
    EngineVelocityMapping::VelocityMapping volumeMapping;
    volumeMapping.target = EngineVelocityMapping::VelocityTarget::VOLUME;
    volumeMapping.enabled = true;
    volumeMapping.baseValue = 0.0f;
    volumeMapping.velocityAmount = 1.0f;
    config.mappings.push_back(volumeMapping);
    
    // Filter mapping
    EngineVelocityMapping::VelocityMapping filterMapping;
    filterMapping.target = EngineVelocityMapping::VelocityTarget::FILTER_CUTOFF;
    filterMapping.enabled = true;
    filterMapping.baseValue = 0.3f;
    filterMapping.velocityAmount = 0.6f;
    config.mappings.push_back(filterMapping);
    
    // Envelope mapping
    EngineVelocityMapping::VelocityMapping envMapping;
    envMapping.target = EngineVelocityMapping::VelocityTarget::ENV_ATTACK;
    envMapping.enabled = true;
    envMapping.baseValue = 0.5f;
    envMapping.velocityAmount = -0.3f; // Negative for faster attack
    config.mappings.push_back(envMapping);
    
    mapper.setEngineConfig(engineId, config);
    mapper.addEngineVoice(engineId, voiceId, 90);
    
    // Test parameter updates
    auto results = mapper.updateEngineParameters(engineId, voiceId, 90);
    
    assert(results.size() == 3); // Should have 3 parameter updates
    
    // Check that all parameters were updated
    bool foundVolume = false, foundFilter = false, foundEnv = false;
    for (const auto& result : results) {
        assert(result.wasUpdated == true);
        assert(result.modulatedValue >= 0.0f && result.modulatedValue <= 1.0f);
        
        if (result.target == EngineVelocityMapping::VelocityTarget::VOLUME) foundVolume = true;
        if (result.target == EngineVelocityMapping::VelocityTarget::FILTER_CUTOFF) foundFilter = true;
        if (result.target == EngineVelocityMapping::VelocityTarget::ENV_ATTACK) foundEnv = true;
    }
    
    assert(foundVolume && foundFilter && foundEnv);
    
    std::cout << "✓ Engine parameter updates tests passed\n";
}

void testPresetManagement() {
    std::cout << "Testing preset management...\n";
    
    EngineVelocityMapping mapper;
    
    // Create default presets
    mapper.createDefaultPresets();
    
    // Test available presets for different engine types
    auto vaPresets = mapper.getAvailablePresets(EngineVelocityMapping::EngineType::MACRO_VA);
    auto fmPresets = mapper.getAvailablePresets(EngineVelocityMapping::EngineType::MACRO_FM);
    auto harmPresets = mapper.getAvailablePresets(EngineVelocityMapping::EngineType::MACRO_HARMONICS);
    auto wtPresets = mapper.getAvailablePresets(EngineVelocityMapping::EngineType::MACRO_WAVETABLE);
    
    assert(!vaPresets.empty());
    assert(!fmPresets.empty());
    assert(!harmPresets.empty());
    assert(!wtPresets.empty());
    
    // Test preset loading
    uint32_t engineId = 8000;
    if (!vaPresets.empty()) {
        mapper.loadEnginePreset(engineId, vaPresets[0]);
        assert(mapper.hasEngineConfig(engineId));
        
        const auto& config = mapper.getEngineConfig(engineId);
        assert(config.engineType == EngineVelocityMapping::EngineType::MACRO_VA);
        assert(config.configName == vaPresets[0]);
    }
    
    // Test preset saving
    mapper.saveEnginePreset(engineId, "Custom Test Preset", "Test description");
    auto updatedPresets = mapper.getAvailablePresets(EngineVelocityMapping::EngineType::MACRO_VA);
    assert(updatedPresets.size() == vaPresets.size() + 1);
    
    std::cout << "✓ Preset management tests passed\n";
}

void testUtilityFunctions() {
    std::cout << "Testing utility functions...\n";
    
    EngineVelocityMapping mapper;
    
    // Test parameter name to target mapping
    auto volumeTarget = mapper.getParameterTarget("volume");
    assert(volumeTarget == EngineVelocityMapping::VelocityTarget::VOLUME);
    
    auto filterTarget = mapper.getParameterTarget("filter_cutoff");
    assert(filterTarget == EngineVelocityMapping::VelocityTarget::FILTER_CUTOFF);
    
    // Test target to name mapping
    std::string volumeName = mapper.getTargetName(EngineVelocityMapping::VelocityTarget::VOLUME);
    assert(volumeName == "Volume");
    
    std::string filterName = mapper.getTargetName(EngineVelocityMapping::VelocityTarget::FILTER_CUTOFF);
    assert(filterName == "Filter Cutoff");
    
    // Test generic envelope target names
    std::string envName = mapper.getTargetName(EngineVelocityMapping::VelocityTarget::ENV_ATTACK);
    assert(envName == "Envelope Attack");
    
    // Test FM-specific target names
    std::string modIndexName = mapper.getTargetName(EngineVelocityMapping::VelocityTarget::FM_MOD_INDEX);
    assert(modIndexName == "FM Modulation Index");
    
    std::cout << "✓ Utility functions tests passed\n";
}

void testSystemManagement() {
    std::cout << "Testing system management...\n";
    
    EngineVelocityMapping mapper;
    
    // Test enable/disable
    mapper.setEnabled(false);
    assert(!mapper.isEnabled());
    
    uint32_t engineId = 9000;
    uint32_t voiceId = 10000;
    
    // Set up engine
    EngineVelocityMapping::EngineVelocityConfig config;
    EngineVelocityMapping::VelocityMapping mapping;
    mapping.target = EngineVelocityMapping::VelocityTarget::VOLUME;
    mapping.enabled = true;
    mapping.velocityAmount = 1.0f;
    config.mappings.push_back(mapping);
    
    mapper.setEngineConfig(engineId, config);
    mapper.addEngineVoice(engineId, voiceId, 100);
    
    // When disabled, should return empty results
    auto results = mapper.updateEngineParameters(engineId, voiceId, 100);
    assert(results.empty());
    
    // Re-enable
    mapper.setEnabled(true);
    assert(mapper.isEnabled());
    
    // Should now return results
    results = mapper.updateEngineParameters(engineId, voiceId, 100);
    assert(!results.empty());
    
    // Test sample rate
    mapper.setSampleRate(44100.0f);
    assert(mapper.getSampleRate() == 44100.0f);
    
    // Test performance monitoring
    float avgTime = mapper.getAverageProcessingTime();
    assert(avgTime >= 0.0f);
    
    // Test system reset
    mapper.reset();
    assert(mapper.getActiveEngineCount() == 0);
    assert(mapper.getTotalActiveVoices() == 0);
    
    std::cout << "✓ System management tests passed\n";
}

void testCallbackIntegration() {
    std::cout << "Testing callback integration...\n";
    
    EngineVelocityMapping mapper;
    
    // Set up callback to track parameter updates
    uint32_t callbackCount = 0;
    mapper.setParameterUpdateCallback([&callbackCount](uint32_t engineId, uint32_t voiceId, EngineVelocityMapping::VelocityTarget target, float value) {
        (void)engineId; (void)voiceId; (void)target; (void)value;
        callbackCount++;
    });
    
    // Set up engine with mapping
    uint32_t engineId = 11000;
    uint32_t voiceId = 12000;
    
    EngineVelocityMapping::EngineVelocityConfig config;
    EngineVelocityMapping::VelocityMapping mapping;
    mapping.target = EngineVelocityMapping::VelocityTarget::VOLUME;
    mapping.enabled = true;
    mapping.velocityAmount = 1.0f;
    config.mappings.push_back(mapping);
    
    mapper.setEngineConfig(engineId, config);
    mapper.addEngineVoice(engineId, voiceId, 80);
    
    // Should trigger callback
    uint32_t initialCount = callbackCount;
    mapper.updateEngineParameters(engineId, voiceId, 100);
    assert(callbackCount > initialCount);
    
    std::cout << "✓ Callback integration tests passed\n";
}

int main() {
    std::cout << "=== EngineVelocityMapping Tests ===\n\n";
    
    try {
        testBasicEngineConfiguration();
        testVelocityMappingConfiguration();
        testParameterMapping();
        testEngineSpecificMappings();
        testVoiceManagement();
        testEngineParameterUpdates();
        testPresetManagement();
        testUtilityFunctions();
        testSystemManagement();
        testCallbackIntegration();
        
        std::cout << "\n🎉 All EngineVelocityMapping tests PASSED!\n";
        std::cout << "\nEngine-Specific Velocity Mapping System Features Verified:\n";
        std::cout << "✓ Per-engine velocity mapping configuration and management\n";
        std::cout << "✓ Engine-specific parameter targets (VA, FM, Harmonics, Wavetable)\n";
        std::cout << "✓ Real-time parameter mapping with velocity curve processing\n";
        std::cout << "✓ Voice management with per-voice velocity tracking\n";
        std::cout << "✓ Multi-parameter engine updates with performance optimization\n";
        std::cout << "✓ Comprehensive preset system with engine-specific defaults\n";
        std::cout << "✓ Parameter name/target utilities and engine integration\n";
        std::cout << "✓ System management with enable/disable and monitoring\n";
        std::cout << "✓ Callback integration for real-time parameter updates\n";
        std::cout << "✓ Default presets for all engine types (VA, FM, Organ, Wavetable)\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}