#pragma once
#include "TapeEffectsProcessor.h"
#include "../storage/presets/EnginePresetLibrary.h"
#include <unordered_map>
#include <memory>

/**
 * TapeEffectsIntegration - Integrates tape effects with EtherSynth preset system
 * 
 * Provides seamless integration between the comprehensive tape effects processor
 * and the existing preset parameter system. Maps preset parameters to tape
 * effects controls and manages per-engine tape processing.
 * 
 * Features:
 * - Automatic mapping of preset fx parameters to tape controls
 * - Per-engine tape effect instances for different characteristics
 * - Real-time parameter updates with smooth transitions
 * - Preset-driven tape effect configuration
 * - Master tape processing chain bypass
 */
class TapeEffectsIntegration {
public:
    struct TapeParameterMapping {
        // Core tape effect parameters
        std::string saturationParam = "tape_saturation";
        std::string compressionParam = "tape_compression";  
        std::string warmthParam = "tape_warmth";
        std::string tubeParam = "tube_warmth";
        std::string vintageParam = "vintage_warmth";
        std::string driveParam = "tube_drive";
        std::string tubeSatParam = "tube_saturation";
        
        // Modulation parameters
        std::string wowParam = "tape_wow";
        std::string flutterParam = "tape_flutter";
        std::string noiseParam = "tape_noise";
        
        // Advanced parameters  
        std::string biasParam = "tape_bias";
        std::string hysteresisParam = "tape_hysteresis";
        std::string speedParam = "tape_speed";
        std::string materialParam = "tape_material";
        
        // Mix parameters
        std::string wetDryParam = "tape_mix";
        std::string bypassParam = "tape_bypass";
    };
    
    TapeEffectsIntegration();
    ~TapeEffectsIntegration() = default;
    
    // System control
    void initialize(float sampleRate);
    void shutdown();
    void setSampleRate(float sampleRate);
    
    // Preset integration
    void applyPresetParameters(const EnginePresetLibrary::EnginePreset& preset);
    void updateFromPresetFxParams(const std::unordered_map<std::string, float>& fxParams);
    void updateFromPresetTwistParams(const std::unordered_map<std::string, float>& twistParams);
    
    // Engine-specific processing
    float processEngineOutput(uint32_t engineId, float input);
    void processEngineBlock(uint32_t engineId, const float* input, float* output, int numSamples);
    void processEngineStereo(uint32_t engineId, const float* inputL, const float* inputR,
                           float* outputL, float* outputR, int numSamples);
    
    // Global tape processing (master output)
    float processMasterOutput(float input);
    void processMasterBlock(const float* input, float* output, int numSamples);
    void processMasterStereo(const float* inputL, const float* inputR,
                           float* outputL, float* outputR, int numSamples);
    
    // Parameter mapping and updates
    void mapPresetParameterToTape(const std::string& presetParam, float value);
    void setTapeParameter(const std::string& tapeParam, float value);
    float getTapeParameter(const std::string& tapeParam) const;
    
    // Engine configuration
    void setEngineConfiguration(uint32_t engineId, const TapeEffectsProcessor::TapeConfig& config);
    const TapeEffectsProcessor::TapeConfig& getEngineConfiguration(uint32_t engineId) const;
    void setEngineTapeType(uint32_t engineId, TapeEffectsProcessor::TapeType type);
    void setEngineTapeMaterial(uint32_t engineId, TapeEffectsProcessor::TapeMaterial material);
    
    // Global controls
    void setMasterTapeConfig(const TapeEffectsProcessor::TapeConfig& config);
    const TapeEffectsProcessor::TapeConfig& getMasterTapeConfig() const;
    void setGlobalBypass(bool bypassed);
    bool isGloballyBypassed() const { return globallyBypassed_; }
    void setMasterWetDryMix(float mix);
    
    // Engine management
    void enableEngineProcessing(uint32_t engineId, bool enabled);
    bool isEngineProcessingEnabled(uint32_t engineId) const;
    void resetEngineState(uint32_t engineId);
    void resetAllEngines();
    
    // Preset management
    void saveEnginePreset(uint32_t engineId, const std::string& presetName);
    void loadEnginePreset(uint32_t engineId, const std::string& presetName);
    std::vector<std::string> getAvailableEnginePresets(uint32_t engineId) const;
    
    // Metering and analysis
    float getEngineSaturation(uint32_t engineId) const;
    float getEngineCompression(uint32_t engineId) const;
    float getMasterSaturation() const;
    float getMasterCompression() const;
    
    // Utility
    void setParameterMapping(const TapeParameterMapping& mapping) { paramMapping_ = mapping; }
    const TapeParameterMapping& getParameterMapping() const { return paramMapping_; }
    
private:
    // System state
    bool initialized_ = false;
    bool globallyBypassed_ = false;
    float sampleRate_ = 48000.0f;
    
    // Tape processors
    std::unique_ptr<TapeEffectsProcessor> masterProcessor_;
    std::unordered_map<uint32_t, std::unique_ptr<TapeEffectsProcessor>> engineProcessors_;
    std::unordered_map<uint32_t, bool> engineProcessingEnabled_;
    
    // Parameter mapping
    TapeParameterMapping paramMapping_;
    std::unordered_map<std::string, float> currentTapeParams_;
    
    // Default configurations for different engine types
    std::unordered_map<std::string, TapeEffectsProcessor::TapeConfig> engineTypeDefaults_;
    
    // Internal methods
    void initializeEngineDefaults();
    void createEngineProcessor(uint32_t engineId);
    TapeEffectsProcessor* getOrCreateEngineProcessor(uint32_t engineId);
    TapeEffectsProcessor::TapeType mapEngineToTapeType(uint32_t engineId) const;
    TapeEffectsProcessor::TapeMaterial mapEngineToTapeMaterial(uint32_t engineId) const;
    void updateProcessorFromParameters(TapeEffectsProcessor* processor);
    float mapPresetValueToTapeRange(const std::string& param, float presetValue) const;
    
    // Parameter update helpers
    void updateSaturationParameters(TapeEffectsProcessor* processor);
    void updateCompressionParameters(TapeEffectsProcessor* processor);
    void updateFrequencyParameters(TapeEffectsProcessor* processor);
    void updateModulationParameters(TapeEffectsProcessor* processor);
    void updateAdvancedParameters(TapeEffectsProcessor* processor);
    
    // Engine type mapping
    std::string getEngineTypeName(uint32_t engineId) const;
    
    // Constants for known engine IDs (from your synthesis engines)
    static constexpr uint32_t ENGINE_MACRO_VA = 0;
    static constexpr uint32_t ENGINE_MACRO_FM = 1;
    static constexpr uint32_t ENGINE_MACRO_HARMONICS = 2;
    static constexpr uint32_t ENGINE_MACRO_WAVETABLE = 3;
    static constexpr uint32_t ENGINE_MACRO_WAVESHAPING = 4;
    static constexpr uint32_t ENGINE_MACRO_TWIN_T = 5;
    static constexpr uint32_t ENGINE_MACRO_STRING = 6;
    static constexpr uint32_t ENGINE_MODAL_RESONATOR = 7;
    static constexpr uint32_t ENGINE_ANALOG_PERCUSSION = 8;
    static constexpr uint32_t ENGINE_ANALOG_SNARE = 9;
    static constexpr uint32_t ENGINE_ANALOG_KICK = 10;
    static constexpr uint32_t ENGINE_DIGITAL_PERCUSSION = 11;
    static constexpr uint32_t ENGINE_SPEECH_SYNTHESIS = 12;
    static constexpr uint32_t ENGINE_GRANULAR_PROCESSOR = 13;
};