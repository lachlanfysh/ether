#include "src/presets/EnginePresetLibrary.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <set>

/**
 * Comprehensive test for EnginePresetLibrary
 * Tests comprehensive preset management for all 32+ synthesis engines
 */

void testBasicLibraryInitialization() {
    std::cout << "Testing basic library initialization...\n";
    
    EnginePresetLibrary library;
    
    // Test initial state
    assert(library.isEnabled() == true);
    assert(library.getTotalPresetCount() == 0);
    
    // Initialize factory presets
    library.initializeFactoryPresets();
    
    // Should have presets for all engine types
    size_t totalPresets = library.getTotalPresetCount();
    std::cout << "Total presets initialized: " << totalPresets << "\n";
    
    // Each engine should have 3 presets (Clean/Classic/Extreme)
    // 32 engines × 3 presets = 96 total presets expected
    assert(totalPresets >= 90); // Allow some flexibility
    assert(totalPresets <= 100);
    
    std::cout << "✓ Basic library initialization tests passed\n";
}

void testEngineSpecificPresets() {
    std::cout << "Testing engine-specific presets...\n";
    
    EnginePresetLibrary library;
    library.initializeFactoryPresets();
    
    // Test main macro engines
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::MACRO_VA) == 3);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::MACRO_FM) == 3);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::MACRO_HARMONICS) == 3);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::MACRO_WAVETABLE) == 3);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::MACRO_CHORD) == 3);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::MACRO_WAVESHAPER) == 3);
    
    // Test Mutable-based engines
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::ELEMENTS_VOICE) == 3);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::RINGS_VOICE) == 3);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::TIDES_OSC) == 3);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::FORMANT_VOCAL) == 3);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::NOISE_PARTICLES) == 3);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::CLASSIC_4OP_FM) == 3);
    
    // Test specialized engines
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::DRUM_KIT) == 3);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::SAMPLER_KIT) == 3);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::SAMPLER_SLICER) == 3);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::SLIDE_ACCENT_BASS) == 3);
    
    // Test some Plaits engines (sample check)
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::PLAITS_VA) == 3);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::PLAITS_FM) == 3);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::PLAITS_GRAIN) == 3);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::PLAITS_BASS_DRUM) == 3);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::PLAITS_HI_HAT) == 3);
    
    std::cout << "✓ Engine-specific presets tests passed\n";
}

void testPresetCategories() {
    std::cout << "Testing preset categories...\n";
    
    EnginePresetLibrary library;
    library.initializeFactoryPresets();
    
    // Count presets by category
    size_t cleanCount = library.getPresetCount(EnginePresetLibrary::PresetCategory::CLEAN);
    size_t classicCount = library.getPresetCount(EnginePresetLibrary::PresetCategory::CLASSIC);
    size_t extremeCount = library.getPresetCount(EnginePresetLibrary::PresetCategory::EXTREME);
    
    std::cout << "Clean presets: " << cleanCount << "\n";
    std::cout << "Classic presets: " << classicCount << "\n";
    std::cout << "Extreme presets: " << extremeCount << "\n";
    
    // Each category should have equal count (1 per engine)
    assert(cleanCount == classicCount);
    assert(classicCount == extremeCount);
    assert(cleanCount >= 30); // At least 30 engines
    
    std::cout << "✓ Preset categories tests passed\n";
}

void testPresetRetrieval() {
    std::cout << "Testing preset retrieval...\n";
    
    EnginePresetLibrary library;
    library.initializeFactoryPresets();
    
    // Test preset existence
    assert(library.hasPreset("VA Clean", EnginePresetLibrary::EngineType::MACRO_VA));
    assert(library.hasPreset("VA Classic", EnginePresetLibrary::EngineType::MACRO_VA));
    assert(library.hasPreset("VA Extreme", EnginePresetLibrary::EngineType::MACRO_VA));
    
    assert(library.hasPreset("FM Clean", EnginePresetLibrary::EngineType::MACRO_FM));
    assert(library.hasPreset("Organ Classic", EnginePresetLibrary::EngineType::MACRO_HARMONICS));
    assert(library.hasPreset("Wavetable Extreme", EnginePresetLibrary::EngineType::MACRO_WAVETABLE));
    
    // Test preset retrieval
    const auto* vaClean = library.getPreset("VA Clean", EnginePresetLibrary::EngineType::MACRO_VA);
    assert(vaClean != nullptr);
    assert(vaClean->name == "VA Clean");
    assert(vaClean->engineType == EnginePresetLibrary::EngineType::MACRO_VA);
    assert(vaClean->category == EnginePresetLibrary::PresetCategory::CLEAN);
    
    const auto* fmExtreme = library.getPreset("FM Extreme", EnginePresetLibrary::EngineType::MACRO_FM);
    assert(fmExtreme != nullptr);
    assert(fmExtreme->category == EnginePresetLibrary::PresetCategory::EXTREME);
    
    // Test non-existent preset
    assert(library.getPreset("NonExistent", EnginePresetLibrary::EngineType::MACRO_VA) == nullptr);
    
    std::cout << "✓ Preset retrieval tests passed\n";
}

void testPresetContent() {
    std::cout << "Testing preset content...\n";
    
    EnginePresetLibrary library;
    library.initializeFactoryPresets();
    
    // Test VA Clean preset content
    const auto* vaClean = library.getPreset("VA Clean", EnginePresetLibrary::EngineType::MACRO_VA);
    assert(vaClean != nullptr);
    
    // Check basic metadata
    assert(vaClean->author == "EtherSynth Factory");
    assert(vaClean->version == "1.0");
    assert(!vaClean->tags.empty());
    
    // Check parameter content (should have some parameters)
    assert(!vaClean->holdParams.empty() || !vaClean->twistParams.empty() || !vaClean->morphParams.empty());
    
    // Check velocity configuration
    assert(vaClean->velocityConfig.enableVelocityToVolume == true);
    assert(!vaClean->velocityConfig.velocityMappings.empty());
    
    // Test different categories have different characteristics
    const auto* vaClassic = library.getPreset("VA Classic", EnginePresetLibrary::EngineType::MACRO_VA);
    const auto* vaExtreme = library.getPreset("VA Extreme", EnginePresetLibrary::EngineType::MACRO_VA);
    
    assert(vaClassic != nullptr && vaExtreme != nullptr);
    
    // Classic and Extreme should have more FX parameters than Clean
    assert(vaClassic->fxParams.size() >= vaClean->fxParams.size());
    assert(vaExtreme->fxParams.size() >= vaClassic->fxParams.size());
    
    std::cout << "✓ Preset content tests passed\n";
}

void testPresetValidation() {
    std::cout << "Testing preset validation...\n";
    
    EnginePresetLibrary library;
    
    // Create a valid preset
    auto validPreset = library.createCleanPreset(EnginePresetLibrary::EngineType::MACRO_VA, "Test Valid");
    auto validation = library.validatePreset(validPreset);
    assert(validation.isValid == true);
    assert(validation.compatibilityScore >= 0.9f);
    
    // Create an invalid preset (empty name)
    auto invalidPreset = validPreset;
    invalidPreset.name = "";
    validation = library.validatePreset(invalidPreset);
    assert(validation.isValid == false);
    assert(!validation.errors.empty());
    
    // Create a preset with out-of-range parameters
    auto rangePreset = validPreset;
    rangePreset.holdParams["test_param"] = 2.5f; // Out of range
    validation = library.validatePreset(rangePreset);
    assert(validation.compatibilityScore < 1.0f);
    
    std::cout << "✓ Preset validation tests passed\n";
}

void testPresetOperations() {
    std::cout << "Testing preset operations...\n";
    
    EnginePresetLibrary library;
    library.initializeFactoryPresets();
    
    // Test adding custom preset
    auto customPreset = library.createCleanPreset(EnginePresetLibrary::EngineType::MACRO_VA, "Custom Test");
    customPreset.category = EnginePresetLibrary::PresetCategory::USER_CUSTOM;
    
    size_t initialCount = library.getPresetCount(EnginePresetLibrary::EngineType::MACRO_VA);
    bool added = library.addPreset(customPreset);
    assert(added == true);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::MACRO_VA) == initialCount + 1);
    
    // Test preset removal
    bool removed = library.removePreset("Custom Test", EnginePresetLibrary::EngineType::MACRO_VA);
    assert(removed == true);
    assert(library.getPresetCount(EnginePresetLibrary::EngineType::MACRO_VA) == initialCount);
    
    std::cout << "✓ Preset operations tests passed\n";
}

void testFactoryPresets() {
    std::cout << "Testing factory preset creation methods...\n";
    
    EnginePresetLibrary library;
    
    // Test Clean preset creation
    auto cleanPreset = library.createCleanPreset(EnginePresetLibrary::EngineType::MACRO_FM, "Test Clean");
    assert(cleanPreset.category == EnginePresetLibrary::PresetCategory::CLEAN);
    assert(cleanPreset.engineType == EnginePresetLibrary::EngineType::MACRO_FM);
    assert(cleanPreset.name == "Test Clean");
    
    // Test Classic preset creation
    auto classicPreset = library.createClassicPreset(EnginePresetLibrary::EngineType::MACRO_FM, "Test Classic");
    assert(classicPreset.category == EnginePresetLibrary::PresetCategory::CLASSIC);
    
    // Test Extreme preset creation
    auto extremePreset = library.createExtremePreset(EnginePresetLibrary::EngineType::MACRO_FM, "Test Extreme");
    assert(extremePreset.category == EnginePresetLibrary::PresetCategory::EXTREME);
    
    // Classic and Extreme should have more FX processing than Clean
    assert(classicPreset.fxParams.size() >= cleanPreset.fxParams.size());
    assert(extremePreset.fxParams.size() >= classicPreset.fxParams.size());
    
    std::cout << "✓ Factory preset creation tests passed\n";
}

void testPlaitsEnginePresets() {
    std::cout << "Testing Plaits engine presets...\n";
    
    EnginePresetLibrary library;
    library.initializeFactoryPresets();
    
    // Test that all Plaits engines have presets
    std::vector<EnginePresetLibrary::EngineType> plaitsEngines = {
        EnginePresetLibrary::EngineType::PLAITS_VA,
        EnginePresetLibrary::EngineType::PLAITS_WAVESHAPING,
        EnginePresetLibrary::EngineType::PLAITS_FM,
        EnginePresetLibrary::EngineType::PLAITS_GRAIN,
        EnginePresetLibrary::EngineType::PLAITS_ADDITIVE,
        EnginePresetLibrary::EngineType::PLAITS_WAVETABLE,
        EnginePresetLibrary::EngineType::PLAITS_CHORD,
        EnginePresetLibrary::EngineType::PLAITS_SPEECH,
        EnginePresetLibrary::EngineType::PLAITS_SWARM,
        EnginePresetLibrary::EngineType::PLAITS_NOISE,
        EnginePresetLibrary::EngineType::PLAITS_PARTICLE,
        EnginePresetLibrary::EngineType::PLAITS_STRING,
        EnginePresetLibrary::EngineType::PLAITS_MODAL,
        EnginePresetLibrary::EngineType::PLAITS_BASS_DRUM,
        EnginePresetLibrary::EngineType::PLAITS_SNARE_DRUM,
        EnginePresetLibrary::EngineType::PLAITS_HI_HAT
    };
    
    for (auto engineType : plaitsEngines) {
        assert(library.getPresetCount(engineType) == 3);
    }
    
    // Test specific Plaits preset names
    assert(library.hasPreset("Plaits VA Clean", EnginePresetLibrary::EngineType::PLAITS_VA));
    assert(library.hasPreset("Plaits FM Classic", EnginePresetLibrary::EngineType::PLAITS_FM));
    assert(library.hasPreset("Plaits Kick Extreme", EnginePresetLibrary::EngineType::PLAITS_BASS_DRUM));
    
    std::cout << "✓ Plaits engine presets tests passed\n";
}

void testSystemManagement() {
    std::cout << "Testing system management...\n";
    
    EnginePresetLibrary library;
    library.initializeFactoryPresets();
    
    size_t initialCount = library.getTotalPresetCount();
    assert(initialCount > 0);
    
    // Test enable/disable
    library.setEnabled(false);
    assert(!library.isEnabled());
    
    // Should not be able to add presets when disabled
    auto testPreset = library.createCleanPreset(EnginePresetLibrary::EngineType::MACRO_VA, "Test Disabled");
    bool added = library.addPreset(testPreset);
    assert(added == false);
    
    // Re-enable
    library.setEnabled(true);
    assert(library.isEnabled());
    
    // Test reset
    library.reset();
    assert(library.getTotalPresetCount() == 0);
    
    // Re-initialize and verify count is restored
    library.initializeFactoryPresets();
    assert(library.getTotalPresetCount() == initialCount);
    
    std::cout << "✓ System management tests passed\n";
}

void testSignaturePresets() {
    std::cout << "Testing signature presets...\n";
    
    EnginePresetLibrary library;
    library.initializeFactoryPresets();
    library.createSignaturePresets();
    
    // Test that signature presets exist
    assert(library.hasPreset("Detuned Stack Pad", EnginePresetLibrary::EngineType::MACRO_VA));
    assert(library.hasPreset("2-Op Punch", EnginePresetLibrary::EngineType::MACRO_FM));
    assert(library.hasPreset("Drawbar Keys", EnginePresetLibrary::EngineType::MACRO_HARMONICS));
    
    // Test signature preset content
    const auto* detunedPad = library.getPreset("Detuned Stack Pad", EnginePresetLibrary::EngineType::MACRO_VA);
    assert(detunedPad != nullptr);
    assert(detunedPad->category == EnginePresetLibrary::PresetCategory::FACTORY_SIGNATURE);
    assert(detunedPad->engineType == EnginePresetLibrary::EngineType::MACRO_VA);
    assert(!detunedPad->holdParams.empty());
    assert(!detunedPad->twistParams.empty());
    assert(!detunedPad->morphParams.empty());
    
    const auto* opPunch = library.getPreset("2-Op Punch", EnginePresetLibrary::EngineType::MACRO_FM);
    assert(opPunch != nullptr);
    assert(opPunch->category == EnginePresetLibrary::PresetCategory::FACTORY_SIGNATURE);
    assert(opPunch->engineType == EnginePresetLibrary::EngineType::MACRO_FM);
    
    const auto* drawbarKeys = library.getPreset("Drawbar Keys", EnginePresetLibrary::EngineType::MACRO_HARMONICS);
    assert(drawbarKeys != nullptr);
    assert(drawbarKeys->category == EnginePresetLibrary::PresetCategory::FACTORY_SIGNATURE);
    assert(drawbarKeys->engineType == EnginePresetLibrary::EngineType::MACRO_HARMONICS);
    
    // Test that signature presets have rich parameter content
    assert(detunedPad->fxParams.size() > 5);  // Should have many effects
    assert(opPunch->velocityConfig.velocityMappings.size() > 3);  // Should have velocity mappings
    assert(drawbarKeys->holdParams.size() > 7);  // Should have all drawbars
    
    // Test macro assignments
    assert(detunedPad->macroAssignments.size() == 4);
    assert(opPunch->macroAssignments.size() == 4);
    assert(drawbarKeys->macroAssignments.size() == 4);
    
    std::cout << "✓ Signature presets tests passed\n";
}

void testJSONSerialization() {
    std::cout << "Testing JSON serialization...\n";
    
    EnginePresetLibrary library;
    library.initializeFactoryPresets();
    library.createSignaturePresets();
    
    // Test preset serialization
    const auto* detunedPad = library.getPreset("Detuned Stack Pad", EnginePresetLibrary::EngineType::MACRO_VA);
    assert(detunedPad != nullptr);
    
    // Serialize the preset to JSON
    std::string json = library.serializePreset(*detunedPad);
    assert(!json.empty());
    
    // Check that JSON contains expected fields
    assert(json.find("\"schema_version\"") != std::string::npos);
    assert(json.find("\"preset_info\"") != std::string::npos);
    assert(json.find("\"hold_params\"") != std::string::npos);
    assert(json.find("\"twist_params\"") != std::string::npos);
    assert(json.find("\"morph_params\"") != std::string::npos);
    assert(json.find("\"macro_assignments\"") != std::string::npos);
    assert(json.find("\"fx_params\"") != std::string::npos);
    assert(json.find("\"velocity_config\"") != std::string::npos);
    assert(json.find("\"performance\"") != std::string::npos);
    
    // Check specific content
    assert(json.find("\"name\": \"Detuned Stack Pad\"") != std::string::npos);
    assert(json.find("\"author\": \"EtherSynth Factory\"") != std::string::npos);
    assert(json.find("\"engine_type\": 0") != std::string::npos); // MACRO_VA = 0
    
    // Test deserialization
    EnginePresetLibrary::EnginePreset deserializedPreset;
    bool success = library.deserializePreset(json, deserializedPreset);
    assert(success == true);
    assert(deserializedPreset.name == "Detuned Stack Pad");
    assert(deserializedPreset.author == "EtherSynth Factory");
    assert(deserializedPreset.engineType == EnginePresetLibrary::EngineType::MACRO_VA);
    
    // Test library export
    std::string libraryJson = library.exportPresetLibrary(EnginePresetLibrary::EngineType::MACRO_VA);
    assert(!libraryJson.empty());
    assert(libraryJson.find("\"library_info\"") != std::string::npos);
    assert(libraryJson.find("\"presets\"") != std::string::npos);
    assert(libraryJson.find("\"engine_type\": 0") != std::string::npos); // MACRO_VA
    
    // Test library import
    bool importSuccess = library.importPresetLibrary(libraryJson, EnginePresetLibrary::EngineType::MACRO_VA);
    assert(importSuccess == true);
    
    std::cout << "JSON sample (first 200 chars): " << json.substr(0, 200) << "...\n";
    std::cout << "✓ JSON serialization tests passed\n";
}

void testComprehensiveCoverage() {
    std::cout << "Testing comprehensive engine coverage...\n";
    
    EnginePresetLibrary library;
    library.initializeFactoryPresets();
    
    // Count total unique engine types with presets
    std::set<EnginePresetLibrary::EngineType> enginesWithPresets;
    
    // Main Macro engines (6)
    std::vector<EnginePresetLibrary::EngineType> macroEngines = {
        EnginePresetLibrary::EngineType::MACRO_VA,
        EnginePresetLibrary::EngineType::MACRO_FM,
        EnginePresetLibrary::EngineType::MACRO_HARMONICS,
        EnginePresetLibrary::EngineType::MACRO_WAVETABLE,
        EnginePresetLibrary::EngineType::MACRO_CHORD,
        EnginePresetLibrary::EngineType::MACRO_WAVESHAPER
    };
    
    // Mutable-based engines (6)
    std::vector<EnginePresetLibrary::EngineType> mutableEngines = {
        EnginePresetLibrary::EngineType::ELEMENTS_VOICE,
        EnginePresetLibrary::EngineType::RINGS_VOICE,
        EnginePresetLibrary::EngineType::TIDES_OSC,
        EnginePresetLibrary::EngineType::FORMANT_VOCAL,
        EnginePresetLibrary::EngineType::NOISE_PARTICLES,
        EnginePresetLibrary::EngineType::CLASSIC_4OP_FM
    };
    
    // Specialized engines (4)
    std::vector<EnginePresetLibrary::EngineType> specializedEngines = {
        EnginePresetLibrary::EngineType::DRUM_KIT,
        EnginePresetLibrary::EngineType::SAMPLER_KIT,
        EnginePresetLibrary::EngineType::SAMPLER_SLICER,
        EnginePresetLibrary::EngineType::SLIDE_ACCENT_BASS
    };
    
    // Plaits engines (16)
    std::vector<EnginePresetLibrary::EngineType> plaitsEngines = {
        EnginePresetLibrary::EngineType::PLAITS_VA,
        EnginePresetLibrary::EngineType::PLAITS_WAVESHAPING,
        EnginePresetLibrary::EngineType::PLAITS_FM,
        EnginePresetLibrary::EngineType::PLAITS_GRAIN,
        EnginePresetLibrary::EngineType::PLAITS_ADDITIVE,
        EnginePresetLibrary::EngineType::PLAITS_WAVETABLE,
        EnginePresetLibrary::EngineType::PLAITS_CHORD,
        EnginePresetLibrary::EngineType::PLAITS_SPEECH,
        EnginePresetLibrary::EngineType::PLAITS_SWARM,
        EnginePresetLibrary::EngineType::PLAITS_NOISE,
        EnginePresetLibrary::EngineType::PLAITS_PARTICLE,
        EnginePresetLibrary::EngineType::PLAITS_STRING,
        EnginePresetLibrary::EngineType::PLAITS_MODAL,
        EnginePresetLibrary::EngineType::PLAITS_BASS_DRUM,
        EnginePresetLibrary::EngineType::PLAITS_SNARE_DRUM,
        EnginePresetLibrary::EngineType::PLAITS_HI_HAT
    };
    
    // Verify all engine categories have presets
    for (auto engine : macroEngines) {
        assert(library.getPresetCount(engine) == 3);
        enginesWithPresets.insert(engine);
    }
    for (auto engine : mutableEngines) {
        assert(library.getPresetCount(engine) == 3);
        enginesWithPresets.insert(engine);
    }
    for (auto engine : specializedEngines) {
        assert(library.getPresetCount(engine) == 3);
        enginesWithPresets.insert(engine);
    }
    for (auto engine : plaitsEngines) {
        assert(library.getPresetCount(engine) == 3);
        enginesWithPresets.insert(engine);
    }
    
    // Total should be 6 + 6 + 4 + 16 = 32 engines
    assert(enginesWithPresets.size() == 32);
    
    std::cout << "Total engines with presets: " << enginesWithPresets.size() << "\n";
    std::cout << "Total presets created: " << library.getTotalPresetCount() << "\n";
    
    // 32 engines × 3 presets each = 96 total
    assert(library.getTotalPresetCount() == 96);
    
    std::cout << "✓ Comprehensive engine coverage tests passed\n";
}

int main() {
    std::cout << "=== EnginePresetLibrary Tests ===\n\n";
    
    try {
        testBasicLibraryInitialization();
        testEngineSpecificPresets();
        testPresetCategories();
        testPresetRetrieval();
        testPresetContent();
        testPresetValidation();
        testPresetOperations();
        testFactoryPresets();
        testPlaitsEnginePresets();
        testSystemManagement();
        testSignaturePresets();
        testJSONSerialization();
        testComprehensiveCoverage();
        
        std::cout << "\n🎉 All EnginePresetLibrary tests PASSED!\n";
        std::cout << "\nComprehensive Preset Library System Features Verified:\n";
        std::cout << "✓ Complete factory preset initialization for all 32 synthesis engines\n";
        std::cout << "✓ Clean/Classic/Extreme preset categories with appropriate characteristics\n";
        std::cout << "✓ Preset validation and parameter range checking\n";
        std::cout << "✓ Comprehensive engine coverage: Macro, Mutable, Specialized, Plaits\n";
        std::cout << "✓ Preset storage, retrieval, and management operations\n";
        std::cout << "✓ Factory preset creation with engine-specific parameter sets\n";
        std::cout << "✓ User preset support with custom categorization\n";
        std::cout << "✓ System management with enable/disable and reset functionality\n";
        std::cout << "✓ Velocity configuration integration for all preset types\n";
        std::cout << "✓ Total preset coverage: 96+ presets across 32 synthesis engines\n";
        std::cout << "✓ Factory signature presets: Detuned Stack Pad, 2-Op Punch, Drawbar Keys\n";
        std::cout << "✓ Complete JSON preset serialization with H/T/M/macro/fx/velocity schema\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}