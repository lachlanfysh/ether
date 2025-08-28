#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

/**
 * EnginePresetLibrary - Comprehensive preset management system
 * 
 * Provides a complete library of presets for all synthesis engines:
 * - 3 fundamental presets per engine: Clean, Classic, Extreme
 * - Specific named presets: Detuned Stack Pad, 2-Op Punch, Drawbar Keys
 * - JSON-based preset schema with full parameter serialization
 * - Preset validation, import/export, and backup functionality
 * - Integration with synthesis engines and velocity mapping systems
 * 
 * Preset Categories:
 * - Clean: Pristine, minimal processing, pure synthesis tones
 * - Classic: Vintage-inspired, moderately processed, musical character  
 * - Extreme: Heavy processing, dramatic effects, modern synthesis
 * 
 * Engine Coverage:
 * - Main Macro: VA, FM, Harmonics, Wavetable, Chord, Waveshaper (6 engines)
 * - Mutable-based: Elements, Rings, Tides, Formant, Noise, Classic4OpFM (6 engines)  
 * - Specialized: DrumKit, SamplerKit, SamplerSlicer, SlideAccentBass (4 engines)
 * - Plaits Collection: 16 synthesis engines from Mutable Instruments Plaits
 * - Total: 32 synthesis engines with comprehensive preset coverage
 * 
 * Features:
 * - Complete parameter serialization (H/T/M params, macro assignments, FX)
 * - Preset validation and compatibility checking
 * - Batch preset operations and library management
 * - User preset storage with categorization and tagging
 * - Performance-optimized preset switching with parameter interpolation
 */

// Forward declarations
class MacroVAEngine;
class MacroFMEngine; 
class MacroHarmonicsEngine;
class MacroWavetableEngine;
class EngineVelocityMapping;

class EnginePresetLibrary {
public:
    // Synthesis engine types - expanded to match EngineVelocityMapping
    enum class EngineType {
        // Main Macro Engines
        MACRO_VA,           // Virtual analog synthesis
        MACRO_FM,           // FM synthesis
        MACRO_HARMONICS,    // Harmonic/drawbar synthesis  
        MACRO_WAVETABLE,    // Wavetable synthesis
        MACRO_CHORD,        // Chord synthesis
        MACRO_WAVESHAPER,   // Waveshaping synthesis
        
        // Advanced Synthesis Engines
        PHYSICAL_MODELING,  // Physical modeling synthesis
        MODAL_RESONATOR,    // Modal synthesis and resonators
        SLOPE_GENERATOR,    // Slope/envelope generator synthesis
        FORMANT_VOCAL,      // Vocal formant synthesis
        NOISE_PARTICLES,    // Noise and particle synthesis
        CLASSIC_4OP_FM,     // Classic 4-operator FM
        
        // Specialized Engines
        DRUM_KIT,           // Drum synthesis engine
        SAMPLER_KIT,        // Sample playback engine
        SAMPLER_SLICER,     // Sample slicing engine
        SLIDE_ACCENT_BASS,  // Bass synthesis with slide/accent
        
        // Multi-Algorithm Synthesis Engines
        MULTI_VA,           // Multi-algorithm virtual analog
        MULTI_WAVESHAPING,  // Multi-algorithm waveshaping
        MULTI_FM,           // Multi-algorithm FM
        GRANULAR_ENGINE,    // Granular synthesis
        ADDITIVE_ENGINE,    // Additive synthesis
        MULTI_WAVETABLE,    // Multi-algorithm wavetable
        CHORD_ENGINE,       // Chord generation engine
        SPEECH_SYNTHESIS,   // Speech synthesis
        PARTICLE_SWARM,     // Particle swarm synthesis
        NOISE_ENGINE,       // Advanced noise synthesis
        PARTICLE_ENGINE,    // Particle synthesis
        STRING_PHYSICAL,    // String physical modeling
        MODAL_SYNTHESIS,    // Modal synthesis
        SYNTH_KICK,         // Synthesized kick drum
        SYNTH_SNARE,        // Synthesized snare drum
        SYNTH_HIHAT         // Synthesized hi-hat
    };
    
    // Preset categories
    enum class PresetCategory {
        CLEAN,              // Clean, minimal processing
        CLASSIC,            // Classic, vintage character
        EXTREME,            // Extreme, heavy processing
        USER_CUSTOM,        // User-created presets
        FACTORY_SIGNATURE   // Signature factory presets
    };
    
    // Parameter types in preset schema
    enum class ParameterType {
        HOLD_PARAM,         // H parameter (hold/sustain controls)
        TWIST_PARAM,        // T parameter (twist/modulation controls)  
        MORPH_PARAM,        // M parameter (morph/blend controls)
        MACRO_ASSIGNMENT,   // Macro knob parameter assignment
        FX_PARAM,          // Effects parameter
        VELOCITY_MAPPING,   // Velocity modulation mapping
        SYSTEM_SETTING     // Global system setting
    };
    
    // Complete preset data structure
    struct EnginePreset {
        // Metadata
        std::string name;                           // Preset display name
        std::string description;                    // Detailed description
        EngineType engineType;                      // Target synthesis engine
        PresetCategory category;                    // Preset category
        std::string author;                         // Preset author/creator
        std::string version;                        // Preset format version
        uint64_t creationTime;                      // Unix timestamp
        uint64_t modificationTime;                  // Last modification time
        std::vector<std::string> tags;              // Search tags
        
        // Core synthesis parameters
        std::unordered_map<std::string, float> holdParams;      // H parameters
        std::unordered_map<std::string, float> twistParams;     // T parameters  
        std::unordered_map<std::string, float> morphParams;     // M parameters
        
        // Macro assignments
        struct MacroAssignment {
            std::string parameterName;              // Target parameter
            float amount;                           // Assignment amount (-1.0 to +1.0)
            bool enabled;                           // Assignment enabled
            
            MacroAssignment() : amount(0.0f), enabled(false) {}
        };
        
        std::unordered_map<uint8_t, MacroAssignment> macroAssignments; // [macroId] -> assignment
        
        // Effects parameters
        std::unordered_map<std::string, float> fxParams;        // Effects settings
        
        // Velocity mapping configuration
        struct VelocityConfig {
            bool enableVelocityToVolume;            // Master velocity→volume enable
            std::unordered_map<std::string, float> velocityMappings; // Parameter→velocity mappings
            
            VelocityConfig() : enableVelocityToVolume(true) {}
        };
        
        VelocityConfig velocityConfig;
        
        // Performance settings
        float morphTransitionTime;                  // Morph transition time (ms)
        bool enableParameterSmoothing;              // Smooth parameter changes
        float parameterSmoothingTime;               // Parameter smoothing time (ms)
        
        EnginePreset() :
            engineType(EngineType::MACRO_VA),
            category(PresetCategory::CLEAN),
            author("Factory"),
            version("1.0"),
            creationTime(0),
            modificationTime(0),
            morphTransitionTime(100.0f),
            enableParameterSmoothing(true),
            parameterSmoothingTime(10.0f) {}
    };
    
    // Preset validation result
    struct PresetValidationResult {
        bool isValid;                               // Overall validation result
        std::vector<std::string> errors;            // Validation error messages
        std::vector<std::string> warnings;          // Non-critical warnings
        float compatibilityScore;                   // Compatibility score (0.0-1.0)
        
        PresetValidationResult() : isValid(false), compatibilityScore(0.0f) {}
    };
    
    // Preset search/filter criteria
    struct PresetSearchCriteria {
        EngineType engineType;
        PresetCategory category;
        std::vector<std::string> tags;
        std::string nameFilter;                     // Name substring search
        std::string authorFilter;                   // Author filter
        bool includeUserPresets;                    // Include user-created presets
        
        PresetSearchCriteria() :
            engineType(EngineType::MACRO_VA),
            category(PresetCategory::CLEAN),
            includeUserPresets(true) {}
    };
    
    EnginePresetLibrary();
    ~EnginePresetLibrary() = default;
    
    // Preset library initialization
    void initializeFactoryPresets();
    void loadUserPresets();
    void createDefaultPresets();
    
    // Core preset operations
    bool addPreset(const EnginePreset& preset);
    bool removePreset(const std::string& presetName, EngineType engineType);
    bool hasPreset(const std::string& presetName, EngineType engineType) const;
    const EnginePreset* getPreset(const std::string& presetName, EngineType engineType) const;
    EnginePreset* getPresetMutable(const std::string& presetName, EngineType engineType);
    
    // Preset searching and filtering
    std::vector<const EnginePreset*> searchPresets(const PresetSearchCriteria& criteria) const;
    std::vector<std::string> getPresetNames(EngineType engineType, PresetCategory category = PresetCategory::CLEAN) const;
    std::vector<const EnginePreset*> getPresetsByCategory(EngineType engineType, PresetCategory category) const;
    std::vector<const EnginePreset*> getPresetsByTag(const std::string& tag) const;
    
    // Preset validation
    PresetValidationResult validatePreset(const EnginePreset& preset) const;
    bool isPresetCompatible(const EnginePreset& preset, EngineType targetEngine) const;
    void repairPreset(EnginePreset& preset) const;
    
    // JSON serialization
    std::string serializePreset(const EnginePreset& preset) const;
    bool deserializePreset(const std::string& json, EnginePreset& preset) const;
    std::string exportPresetLibrary(EngineType engineType) const;
    bool importPresetLibrary(const std::string& json, EngineType engineType);
    
    // File I/O operations
    bool savePresetToFile(const EnginePreset& preset, const std::string& filename) const;
    bool loadPresetFromFile(const std::string& filename, EnginePreset& preset) const;
    bool saveLibraryToDirectory(const std::string& directory) const;
    bool loadLibraryFromDirectory(const std::string& directory);
    
    // Factory preset creation
    EnginePreset createCleanPreset(EngineType engineType, const std::string& name) const;
    EnginePreset createClassicPreset(EngineType engineType, const std::string& name) const;
    EnginePreset createExtremePreset(EngineType engineType, const std::string& name) const;
    
    // Signature preset creation
    EnginePreset createDetunedStackPad() const;         // MacroVA signature preset
    EnginePreset create2OpPunch() const;                // MacroFM signature preset  
    EnginePreset createDrawbarKeys() const;             // MacroHarmonics signature preset
    void createSignaturePresets();                      // Create all signature presets
    
    // Preset application to engines
    bool applyPresetToEngine(const EnginePreset& preset, uint32_t engineId);
    bool morphBetweenPresets(const EnginePreset& presetA, const EnginePreset& presetB, float morphAmount, uint32_t engineId);
    
    // Batch operations
    void createAllFactoryPresets();
    void validateAllPresets();
    void rebuildPresetCache();
    void cleanupOrphanedPresets();
    
    // Integration with engine systems
    void integrateWithVelocityMapping(EngineVelocityMapping* velocityMapping);
    void setEngineParameterCallback(std::function<void(uint32_t engineId, const std::string& paramName, float value)> callback);
    
    // Statistics and monitoring
    size_t getTotalPresetCount() const;
    size_t getPresetCount(EngineType engineType) const;
    size_t getPresetCount(PresetCategory category) const;
    std::vector<std::string> getMostUsedTags() const;
    float getAveragePresetSize() const;
    
    // User preset management
    bool saveUserPreset(const EnginePreset& preset);
    bool deleteUserPreset(const std::string& presetName, EngineType engineType);
    std::vector<const EnginePreset*> getUserPresets(EngineType engineType) const;
    void backupUserPresets(const std::string& backupPath) const;
    bool restoreUserPresets(const std::string& backupPath);
    
    // System management
    void setPresetDirectory(const std::string& directory) { presetDirectory_ = directory; }
    const std::string& getPresetDirectory() const { return presetDirectory_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    void reset();
    
private:
    // System state
    bool enabled_;
    std::string presetDirectory_;
    
    // Preset storage
    std::unordered_map<EngineType, std::vector<std::unique_ptr<EnginePreset>>> factoryPresets_;
    std::unordered_map<EngineType, std::vector<std::unique_ptr<EnginePreset>>> userPresets_;
    
    // Integration
    EngineVelocityMapping* velocityMapping_;
    std::function<void(uint32_t, const std::string&, float)> engineParameterCallback_;
    
    // Cache for performance
    mutable std::unordered_map<std::string, const EnginePreset*> presetCache_;
    mutable bool cacheValid_;
    
    // Internal preset creation methods
    void initializeMacroVAPresets();
    void initializeMacroFMPresets();
    void initializeMacroHarmonicsPresets();
    void initializeMacroWavetablePresets();
    void initializeMacroChordPresets();
    void initializeMacroWaveshaperPresets();
    void initializeElementsPresets();
    void initializeRingsPresets();
    void initializeTidesPresets();
    void initializeFormantPresets();
    void initializeNoiseParticlesPresets();
    void initializeClassic4OpFMPresets();
    void initializeDrumKitPresets();
    void initializeSamplerKitPresets();
    void initializeSamplerSlicerPresets();
    void initializeSlideAccentBassPresets();
    void initializePlaitsVAPresets();
    void initializePlaitsWaveshapingPresets();
    void initializePlaitsFMPresets();
    void initializePlaitsGrainPresets();
    void initializePlaitsAdditivePresets();
    void initializePlaitsWavetablePresets();
    void initializePlaitsChordPresets();
    void initializePlaitsSpeechPresets();
    void initializePlaitsSwarmPresets();
    void initializePlaitsNoisePresets();
    void initializePlaitsParticlePresets();
    void initializePlaitsStringPresets();
    void initializePlaitsModalPresets();
    void initializePlaitsBassPresets();
    void initializePlaitsSnarePresets();
    void initializePlaitsHiHatPresets();
    
    // JSON helpers
    std::string serializeParameterMap(const std::unordered_map<std::string, float>& params) const;
    bool deserializeParameterMap(const std::string& json, std::unordered_map<std::string, float>& params) const;
    
    // Validation helpers
    bool validateParameterRanges(const EnginePreset& preset) const;
    bool validateMacroAssignments(const EnginePreset& preset) const;
    bool validateEngineCompatibility(const EnginePreset& preset) const;
    
    // Utility methods
    std::string generatePresetId(const EnginePreset& preset) const;
    void updatePresetCache() const;
    void invalidateCache() { cacheValid_ = false; }
    uint64_t getCurrentTimestamp() const;
    
    // Constants
    static constexpr float MIN_PARAMETER_VALUE = 0.0f;
    static constexpr float MAX_PARAMETER_VALUE = 1.0f;
    static constexpr size_t MAX_PRESET_NAME_LENGTH = 64;
    static constexpr size_t MAX_DESCRIPTION_LENGTH = 256;
    static const std::string PRESET_FILE_EXTENSION;
    static const std::string PRESET_SCHEMA_VERSION;
};