#pragma once
#include "../core/Types.h"
#include "../engines/EngineParameterLayouts.h"
#include "AdvancedLFO.h"
#include "../control/modulation/AdvancedModulationMatrix.h"
#include <array>
#include <memory>
#include <vector>

/**
 * LFO Parameter Mapping System - connects LFOs to 16-key parameter system
 * 
 * This system provides:
 * - Multiple LFO destinations per engine (up to 4 LFO → 16 parameters)
 * - Real-time LFO rate/waveform selection via UI
 * - Visual LFO feedback in parameter mode
 * - Context-sensitive LFO assignments per engine type
 * - Professional groovebox-style LFO workflow
 * 
 * Integration with 960×320 + 2×16 interface:
 * - INST + key selects parameter for LFO assignment
 * - SmartKnob controls LFO depth
 * - Visual feedback shows active LFO connections
 * - LFO rate/shape controlled via dedicated interface
 */

namespace EtherSynthModulation {

class LFOParameterMapping {
public:
    static constexpr int MAX_INSTRUMENT_LFOS = 8;  // Final: LFO1–LFO8 per instrument
    static constexpr int MAX_INSTRUMENTS = 8;      // I1-I8
    static constexpr int MAX_PARAMETERS = 16;      // 16-key parameter system
    
    struct LFOAssignment {
        bool enabled = false;
        int lfoIndex = 0;               // 0-3 (LFO1-4)
        ParameterID parameterID = ParameterID::VOLUME;
        int keyIndex = 0;               // 0-15 (key position)
        float depth = 0.0f;             // -1.0 to 1.0 modulation depth
        float offset = 0.0f;            // Base parameter offset
        bool bipolar = true;            // Bipolar vs unipolar modulation
        AdvancedModulationMatrix::ModProcessing processing = 
            AdvancedModulationMatrix::ModProcessing::DIRECT;
    };
    
    struct InstrumentLFOState {
        std::array<std::unique_ptr<EtherSynthModulation::AdvancedLFO>, MAX_INSTRUMENT_LFOS> lfos;
        std::vector<LFOAssignment> assignments;
        EngineType engineType = EngineType::MACRO_VA;
        bool globalSync = false;
        float masterDepth = 1.0f;       // Global LFO depth multiplier
    };
    
    LFOParameterMapping();
    ~LFOParameterMapping() = default;
    
    // Initialization
    void initialize(float sampleRate, float tempo);
    void setInstrumentEngine(int instrumentIndex, EngineType engineType);
    
    // LFO Assignment Management
    void assignLFOToParameter(int instrumentIndex, int lfoIndex, int keyIndex, float depth);
    void removeLFOAssignment(int instrumentIndex, int lfoIndex, int keyIndex);
    void clearAllAssignments(int instrumentIndex);
    LFOAssignment* getAssignment(int instrumentIndex, int lfoIndex, int keyIndex);
    std::vector<LFOAssignment> getParameterAssignments(int instrumentIndex, int keyIndex);
    
    // LFO Control
    EtherSynthModulation::AdvancedLFO* getLFO(int instrumentIndex, int lfoIndex);
    void setLFOWaveform(int instrumentIndex, int lfoIndex, EtherSynthModulation::AdvancedLFO::Waveform waveform);
    void setLFORate(int instrumentIndex, int lfoIndex, float rate);
    void setLFODepth(int instrumentIndex, int lfoIndex, float depth);
    void setLFOSync(int instrumentIndex, int lfoIndex, bool sync);
    
    // Parameter Modulation Processing
    void processFrame();
    float getModulatedParameterValue(int instrumentIndex, int keyIndex, float baseValue);
    
    // UI Integration
    struct LFODisplayInfo {
        bool hasLFO = false;
        int activeLFOs = 0;             // Bitmask of active LFOs (1-4)
        float currentValue = 0.0f;      // Current LFO output for visual feedback
        EtherSynthModulation::AdvancedLFO::Waveform waveform = EtherSynthModulation::AdvancedLFO::Waveform::SINE;
        float rate = 1.0f;
        bool synced = false;
    };
    
    LFODisplayInfo getParameterLFOInfo(int instrumentIndex, int keyIndex);
    std::vector<int> getParametersWithLFOs(int instrumentIndex); // Returns key indices
    
    // Engine-Specific LFO Templates
    void applyEngineTemplate(int instrumentIndex, EngineType engineType);
    void createBasicTemplate(int instrumentIndex);      // Basic LFO→Filter/Pitch template
    void createPerformanceTemplate(int instrumentIndex); // Performance-oriented template
    void createExperimentalTemplate(int instrumentIndex); // Creative modulation template
    
    // Preset Management
    struct LFOPreset {
        char name[32];
        EngineType engineType;
        std::array<EtherSynthModulation::AdvancedLFO::LFOSettings, MAX_INSTRUMENT_LFOS> lfoSettings;
        std::vector<LFOAssignment> assignments;
        float masterDepth;
        bool globalSync;
    };
    
    void savePreset(int instrumentIndex, int slot, const char* name);
    bool loadPreset(int instrumentIndex, int slot);
    const LFOPreset* getPreset(int slot) const;
    
    // Global Controls
    void setGlobalTempo(float bpm);
    void setGlobalSync(bool enabled);
    void setMasterDepth(int instrumentIndex, float depth);
    
    // Real-time Performance
    void triggerLFOs(int instrumentIndex);              // Trigger on note-on
    void syncLFOsToTempo(int instrumentIndex);         // Sync to DAW/internal tempo
    
private:
    std::array<InstrumentLFOState, MAX_INSTRUMENTS> instruments_;
    std::array<LFOPreset, 16> presets_;                // 16 preset slots
    
    float sampleRate_ = 48000.0f;
    float tempo_ = 120.0f;
    bool globalSync_ = false;
    
    // Template creation helpers
    void createMacroVATemplate(int instrumentIndex);
    void createMacroFMTemplate(int instrumentIndex);
    void createMacroWTTemplate(int instrumentIndex);
    void createDrumKitTemplate(int instrumentIndex);
    void createSamplerTemplate(int instrumentIndex);
    
    // Utility
    void updateLFOSettings(int instrumentIndex);
    bool isValidInstrument(int index) const;
    bool isValidLFO(int index) const;
    bool isValidKey(int index) const;
};

/**
 * LFO UI Manager - handles visual feedback and control interfaces
 */
class LFOUIManager {
public:
    struct LFOVisualState {
        float phase = 0.0f;             // Current phase for waveform display
        float value = 0.0f;             // Current output value
        bool active = false;            // LFO currently running
        uint32_t color = 0xFFFFFF;      // Color for visual feedback
        float intensity = 0.0f;         // Visual intensity (0-1)
    };
    
    LFOUIManager(LFOParameterMapping* mapping);
    
    // Visual state updates
    void updateVisualState();
    LFOVisualState getLFOState(int instrumentIndex, int lfoIndex);
    uint32_t getParameterLFOColor(int instrumentIndex, int keyIndex);
    
    // Rate/waveform selection helpers
    const char* getWaveformName(EtherSynthModulation::AdvancedLFO::Waveform waveform);
    const char* getRateDisplayString(float rate, bool synced);
    std::vector<EtherSynthModulation::AdvancedLFO::Waveform> getRecommendedWaveforms(EngineType engineType);
    
    // Template suggestions
    struct TemplateSuggestion {
        const char* name;
        const char* description;
        void (LFOParameterMapping::*applyFunction)(int);
    };
    
    std::vector<TemplateSuggestion> getTemplatesForEngine(EngineType engineType);
    
private:
    LFOParameterMapping* mapping_;
    std::array<std::array<LFOVisualState, LFOParameterMapping::MAX_INSTRUMENT_LFOS>, 
               LFOParameterMapping::MAX_INSTRUMENTS> visualState_;
};

} // namespace EtherSynthModulation
