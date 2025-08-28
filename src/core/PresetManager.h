#pragma once
#include "Types.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

/**
 * Comprehensive Preset Management System
 * Handles saving, loading, and organizing synthesizer presets
 */
class PresetManager {
public:
    PresetManager();
    ~PresetManager();
    
    // Preset structure
    struct Preset {
        std::string name;
        std::string description;
        std::string category;
        std::string author;
        uint32_t version = 1;
        uint64_t createdTime = 0;
        uint64_t modifiedTime = 0;
        
        // Engine configuration
        EngineType engineType = EngineType::SUBTRACTIVE;
        std::vector<uint8_t> engineData;
        
        // Global parameters
        std::map<ParameterID, float> globalParameters;
        
        // Per-instrument configurations
        struct InstrumentConfig {
            EngineType engineType = EngineType::SUBTRACTIVE;
            std::vector<uint8_t> enginePreset;
            std::map<ParameterID, float> parameters;
            std::string name;
            bool muted = false;
            bool soloed = false;
            float volume = 0.8f;
            float pan = 0.0f;
        };
        
        std::array<InstrumentConfig, MAX_INSTRUMENTS> instruments;
        
        // Smart knob configuration
        struct SmartKnobConfig {
            ParameterID assignedParameter = ParameterID::VOLUME;
            float currentValue = 0.5f;
            std::string macroName;
            std::vector<ParameterID> multiParameters;
        };
        
        SmartKnobConfig smartKnob;
        
        // Effects settings
        struct EffectsConfig {
            bool reverbEnabled = false;
            float reverbSize = 0.5f;
            float reverbMix = 0.3f;
            
            bool delayEnabled = false;
            float delayTime = 0.25f;
            float delayFeedback = 0.3f;
            float delayMix = 0.2f;
            
            bool chorusEnabled = false;
            float chorusRate = 0.5f;
            float chorusDepth = 0.3f;
            float chorusMix = 0.4f;
        };
        
        EffectsConfig effects;
        
        // Performance settings
        float masterVolume = 0.8f;
        float bpm = 120.0f;
        bool isPlaying = false;
    };
    
    // Preset operations
    bool savePreset(const Preset& preset);
    bool loadPreset(const std::string& name, Preset& preset);
    bool deletePreset(const std::string& name);
    bool renamePreset(const std::string& oldName, const std::string& newName);
    bool duplicatePreset(const std::string& sourceName, const std::string& newName);
    
    // Preset discovery
    std::vector<std::string> getPresetNames() const;
    std::vector<std::string> getPresetsByCategory(const std::string& category) const;
    std::vector<std::string> getCategories() const;
    std::vector<Preset> searchPresets(const std::string& query) const;
    
    // Factory presets
    void loadFactoryPresets();
    bool isFactoryPreset(const std::string& name) const;
    
    // Preset validation
    bool validatePreset(const Preset& preset) const;
    std::string getPresetPath(const std::string& name) const;
    
    // Import/Export
    bool exportPreset(const std::string& name, const std::string& filePath) const;
    bool importPreset(const std::string& filePath);
    bool exportPresetBank(const std::vector<std::string>& presetNames, const std::string& filePath) const;
    bool importPresetBank(const std::string& filePath);
    
    // Preset organization
    bool createCategory(const std::string& category);
    bool deleteCategory(const std::string& category);
    bool movePresetToCategory(const std::string& presetName, const std::string& category);
    
    // Quick access
    void setFavoritePreset(const std::string& name, bool favorite);
    std::vector<std::string> getFavoritePresets() const;
    void setLastUsedPreset(const std::string& name);
    std::string getLastUsedPreset() const;
    
    // Auto-save functionality
    void enableAutoSave(bool enable, float intervalSeconds = 30.0f);
    void setAutoSavePresetName(const std::string& name);
    
    // Preset comparison and morphing
    float comparePresets(const std::string& preset1, const std::string& preset2) const;
    Preset morphPresets(const std::string& preset1, const std::string& preset2, float amount) const;
    
    // Statistics
    size_t getPresetCount() const;
    size_t getCategoryCount() const;
    std::map<std::string, size_t> getPresetCountByCategory() const;
    
    // Error handling
    enum class PresetError {
        NONE = 0,
        FILE_NOT_FOUND,
        INVALID_FORMAT,
        WRITE_FAILED,
        READ_FAILED,
        PRESET_EXISTS,
        INVALID_NAME,
        CATEGORY_NOT_FOUND,
        DISK_FULL,
        PERMISSION_DENIED
    };
    
    PresetError getLastError() const { return lastError_; }
    std::string getErrorMessage() const;

private:
    // Internal storage
    std::map<std::string, Preset> presets_;
    std::map<std::string, std::vector<std::string>> categories_;
    std::vector<std::string> factoryPresetNames_;
    std::vector<std::string> favoritePresets_;
    std::string lastUsedPreset_;
    
    // Auto-save
    bool autoSaveEnabled_ = false;
    float autoSaveInterval_ = 30.0f;
    std::string autoSavePresetName_ = "AutoSave";
    float lastAutoSaveTime_ = 0.0f;
    
    // Error tracking
    mutable PresetError lastError_ = PresetError::NONE;
    
    // File system operations
    std::string presetDirectory_;
    std::string factoryDirectory_;
    std::string userDirectory_;
    
    // Helper methods
    bool initializeDirectories();
    bool savePresetToFile(const Preset& preset, const std::string& filePath) const;
    bool loadPresetFromFile(const std::string& filePath, Preset& preset) const;
    std::string sanitizePresetName(const std::string& name) const;
    std::string generateUniquePresetName(const std::string& baseName) const;
    bool isValidPresetName(const std::string& name) const;
    
    // Factory preset generation
    void createFactoryPreset(const std::string& name, const std::string& description, 
                           EngineType engineType, const std::string& category);
    
    // Serialization helpers
    std::vector<uint8_t> serializePreset(const Preset& preset) const;
    bool deserializePreset(const std::vector<uint8_t>& data, Preset& preset) const;
    
    // File format constants
    static constexpr uint32_t PRESET_FILE_MAGIC = 0x45544852; // "ETHR"
    static constexpr uint32_t PRESET_FILE_VERSION = 1;
    static constexpr size_t MAX_PRESET_NAME_LENGTH = 64;
    static constexpr size_t MAX_DESCRIPTION_LENGTH = 256;
    static constexpr size_t MAX_CATEGORY_LENGTH = 32;
    static constexpr size_t MAX_AUTHOR_LENGTH = 64;
};

// Factory preset definitions
namespace FactoryPresets {
    // Categories
    extern const std::string BASS;
    extern const std::string LEAD;
    extern const std::string PAD;
    extern const std::string PLUCK;
    extern const std::string FX;
    extern const std::string PERCUSSION;
    extern const std::string EXPERIMENTAL;
    extern const std::string TEMPLATE;
    
    // Classic synthesizer emulations
    extern const std::string TB303_BASS;
    extern const std::string MOOG_LEAD;
    extern const std::string DX7_BELL;
    extern const std::string JUNO_PAD;
    extern const std::string SH101_ACID;
    
    // Modern sounds
    extern const std::string FUTURE_BASS;
    extern const std::string DUBSTEP_WOBBLE;
    extern const std::string AMBIENT_TEXTURE;
    extern const std::string GRANULAR_CLOUD;
    extern const std::string FM_BELL;
    
    // Color-themed presets (matching instrument colors)
    extern const std::string RED_FIRE;      // Aggressive bass
    extern const std::string ORANGE_WARM;   // Warm lead
    extern const std::string YELLOW_BRIGHT; // Bright pluck
    extern const std::string GREEN_ORGANIC;  // Natural pad
    extern const std::string BLUE_DEEP;     // Deep strings
    extern const std::string INDIGO_MYSTIC; // Mysterious FX
    extern const std::string VIOLET_ETHEREAL; // Ethereal lead
    extern const std::string GREY_UTILITY;  // Utility sounds
}