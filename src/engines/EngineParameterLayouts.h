#pragma once
#include "../core/Types.h"
#include <array>
#include <string>
#include <unordered_map>

/**
 * EngineParameterLayouts - 16-Key Parameter Mapping System
 * 
 * Maps all 14 synthesis engines to exactly 16 parameters each for 
 * the 960×320 + 2×16 hardware interface. When INST is held, the 
 * bottom 16 keys select parameters, SmartKnob controls values.
 * 
 * Design Principles:
 * - Each engine has exactly 16 parameters mapped to keys 1-16
 * - Parameters grouped logically: OSC(1-4), FILTER(5-8), ENV(9-12), FX(13-16)
 * - Most expressive parameters get prime key positions (1,2,3,4)
 * - Compatible with existing ParameterID system
 * - Designed for immediate hardware control workflow
 */

struct EngineParameterLayout {
    std::array<ParameterID, 16> parameters;
    std::array<const char*, 16> displayNames;
    std::array<std::pair<float, float>, 16> ranges;  // min, max
    std::array<const char*, 16> units;
    
    // Parameter grouping for visual feedback (matches hardware quads)
    enum class ParameterGroup : uint8_t {
        OSC = 0,     // Keys 1-4: Core synthesis
        FILTER = 1,  // Keys 5-8: Filter & tone shaping  
        ENV = 2,     // Keys 9-12: Envelope & dynamics
        FX = 3       // Keys 13-16: Effects & mix
    };
    
    std::array<ParameterGroup, 16> groups;
};

/**
 * EngineParameterMappings - Complete 16-key layouts for all engines
 */
class EngineParameterMappings {
public:
    // Get parameter layout for specific engine
    static const EngineParameterLayout& getLayout(EngineType engineType);
    
    // Get parameter at key index for engine
    static ParameterID getParameterAt(EngineType engineType, int keyIndex);
    
    // Get display name for parameter at key index
    static const char* getParameterName(EngineType engineType, int keyIndex);
    
    // Get parameter range for value scaling
    static std::pair<float, float> getParameterRange(EngineType engineType, int keyIndex);
    
    // Get parameter group for color coding
    static EngineParameterLayout::ParameterGroup getParameterGroup(EngineType engineType, int keyIndex);
    
private:
    static void initializeLayouts();
    static std::unordered_map<EngineType, EngineParameterLayout> layouts_;
    static bool initialized_;
    
    // Individual engine layout creators
    static EngineParameterLayout createMacroVALayout();
    static EngineParameterLayout createMacroFMLayout();
    static EngineParameterLayout createMacroWTLayout();
    static EngineParameterLayout createMacroWSLayout();
    static EngineParameterLayout createMacroChordLayout();
    static EngineParameterLayout createMacroHarmonicsLayout();
    static EngineParameterLayout createFormantVocalLayout();
    static EngineParameterLayout createNoiseParticlesLayout();
    static EngineParameterLayout createTidesOscLayout();
    static EngineParameterLayout createRingsVoiceLayout();
    static EngineParameterLayout createElementsVoiceLayout();
    static EngineParameterLayout createDrumKitLayout();
    static EngineParameterLayout createSamplerKitLayout();
    static EngineParameterLayout createSamplerSlicerLayout();
};

// Utility functions for hardware interface
namespace EngineParameterUtils {
    // Convert 0-1 knob value to actual parameter value
    float scaleKnobToParameter(EngineType engineType, int keyIndex, float knobValue);
    
    // Convert parameter value to 0-1 knob value
    float scaleParameterToKnob(EngineType engineType, int keyIndex, float paramValue);
    
    // Get color for parameter group (matches UI color system)
    uint32_t getGroupColor(EngineParameterLayout::ParameterGroup group);
    
    // Validate key index (0-15)
    bool isValidKeyIndex(int keyIndex);
}