#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <map>
#include "AutoSampleLoader.h"

/**
 * SampleNamingSystem - Comprehensive auto-naming and user-definable sample naming
 * 
 * Provides intelligent sample naming capabilities:
 * - Auto-generated names based on content analysis and source
 * - User-customizable naming templates and rules
 * - Intelligent name collision handling and uniqueness
 * - Integration with tape squashing workflow for contextual naming
 * - Support for hierarchical naming schemes and categories
 * - Hardware-optimized for STM32 H7 embedded platform
 * 
 * Features:
 * - Smart content-aware name generation (e.g., "Kick", "Snare", "Lead")
 * - Template-based naming with variables and formatting
 * - Name collision detection and automatic numbering
 * - User preference learning for consistent naming patterns
 * - Integration with metadata tagging and search systems
 * - Support for multi-language naming schemes
 * - Real-time name validation and suggestion
 * - Name history and favorite patterns
 */
class SampleNamingSystem {
public:
    // Naming strategies
    enum class NamingStrategy {
        AUTO_ANALYZE,           // Analyze content and generate appropriate name
        TEMPLATE_BASED,         // Use template with variables
        USER_DEFINED,           // User manually defines name
        SOURCE_BASED,           // Based on source location/pattern
        TIMESTAMP_BASED,        // Include timestamp information
        SEQUENTIAL,             // Sequential numbering (Sample_001, Sample_002, etc.)
        DESCRIPTIVE,            // Generate descriptive names based on parameters
        HYBRID                  // Combine multiple strategies
    };
    
    // Content analysis categories for auto-naming
    enum class ContentCategory {
        UNKNOWN,                // Unable to categorize
        KICK_DRUM,              // Kick drum sample
        SNARE_DRUM,             // Snare drum sample
        HI_HAT,                 // Hi-hat sample
        CYMBAL,                 // Cymbal sample
        TOM_DRUM,               // Tom drum sample
        PERCUSSION,             // Generic percussion
        BASS_SOUND,             // Bass frequency content
        LEAD_SOUND,             // Lead melody content
        PAD_SOUND,              // Pad/ambient sound
        CHORD_SOUND,            // Harmonic/chord content
        NOISE_SOUND,            // Noise-based content
        VOCAL_SOUND,            // Vocal content
        MELODIC,                // Melodic content
        RHYTHMIC,               // Rhythmic content
        AMBIENT,                // Ambient/atmospheric
        EFFECT_SOUND,           // Sound effect
        LOOP,                   // Musical loop
        ONE_SHOT               // One-shot sample
    };
    
    // Naming template configuration
    struct NamingTemplate {
        std::string templateString;     // Template with variables (e.g., "{category}_{slot:02d}_{timestamp}")
        NamingStrategy strategy;        // Primary strategy for this template
        bool enableContentAnalysis;    // Whether to analyze content for naming
        bool enableNumbering;          // Whether to add numbers for uniqueness
        bool preserveUserNames;        // Whether to keep user-defined names
        uint8_t maxNameLength;         // Maximum name length
        std::string categoryPrefix;    // Prefix based on category
        std::string categorySuffix;    // Suffix based on category
        
        NamingTemplate() :
            templateString("{category}_{slot:02d}"),
            strategy(NamingStrategy::TEMPLATE_BASED),
            enableContentAnalysis(true),
            enableNumbering(true),
            preserveUserNames(true),
            maxNameLength(32),
            categoryPrefix(""),
            categorySuffix("") {}
    };
    
    // Sample analysis result for naming
    struct SampleAnalysis {
        ContentCategory primaryCategory;    // Primary content category
        ContentCategory secondaryCategory;  // Secondary category (if applicable)
        float confidence;                  // Confidence in categorization (0.0-1.0)
        float peakFrequency;              // Dominant frequency (Hz)
        float dynamicRange;               // Dynamic range (dB)
        float brightness;                 // Spectral brightness (0.0-1.0)
        float rhythmicity;                // Rhythmic content (0.0-1.0)
        bool isPercussive;                // Whether sample is percussive
        bool isHarmonic;                  // Whether sample has harmonic content
        bool isTonal;                     // Whether sample is tonal
        uint32_t durationMs;              // Sample duration in milliseconds
        std::vector<std::string> tags;    // Descriptive tags
        
        SampleAnalysis() :
            primaryCategory(ContentCategory::UNKNOWN),
            secondaryCategory(ContentCategory::UNKNOWN),
            confidence(0.0f),
            peakFrequency(0.0f),
            dynamicRange(0.0f),
            brightness(0.5f),
            rhythmicity(0.0f),
            isPercussive(false),
            isHarmonic(false),
            isTonal(false),
            durationMs(0) {}
    };
    
    // Name generation result
    struct NameGenerationResult {
        std::string suggestedName;         // Generated name suggestion
        std::vector<std::string> alternatives;  // Alternative name suggestions
        NamingStrategy usedStrategy;       // Strategy that was used
        SampleAnalysis analysis;           // Content analysis result
        bool hasCollision;                 // Whether name collides with existing
        std::string collisionResolution;   // How collision was resolved
        float confidence;                  // Confidence in suggested name
        
        NameGenerationResult() :
            usedStrategy(NamingStrategy::AUTO_ANALYZE),
            hasCollision(false),
            confidence(0.0f) {}
    };
    
    // User naming preferences
    struct NamingPreferences {
        NamingStrategy preferredStrategy;   // User's preferred naming strategy
        std::vector<NamingTemplate> templates;  // User's custom templates
        bool enableAutoSuggestions;        // Whether to show auto-suggestions
        bool enableRealTimeValidation;     // Validate names as user types
        uint8_t maxSuggestions;            // Max number of suggestions to show
        std::map<ContentCategory, std::string> categoryNames;  // Custom category names
        std::vector<std::string> favoritePatterns;  // User's favorite naming patterns
        bool enableIntelligentNumbering;   // Smart numbering (01, 02 vs 1, 2)
        
        NamingPreferences() :
            preferredStrategy(NamingStrategy::HYBRID),
            enableAutoSuggestions(true),
            enableRealTimeValidation(true),
            maxSuggestions(5),
            enableIntelligentNumbering(true) {}
    };
    
    SampleNamingSystem();
    ~SampleNamingSystem() = default;
    
    // Configuration
    void setNamingPreferences(const NamingPreferences& preferences);
    const NamingPreferences& getNamingPreferences() const { return preferences_; }
    void addNamingTemplate(const NamingTemplate& nameTemplate);
    void removeNamingTemplate(const std::string& templateString);
    
    // Name Generation
    NameGenerationResult generateName(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData,
                                     const std::string& sourceContext = "",
                                     uint8_t slotId = 255);
    NameGenerationResult generateNameFromTemplate(const NamingTemplate& nameTemplate,
                                                  const SampleAnalysis& analysis,
                                                  uint8_t slotId = 255);
    std::vector<std::string> generateNameSuggestions(const SampleAnalysis& analysis,
                                                     uint8_t count = 5);
    
    // Content Analysis
    SampleAnalysis analyzeSampleContent(std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData);
    ContentCategory categorizeContent(const SampleAnalysis& analysis);
    std::string getCategoryName(ContentCategory category) const;
    std::vector<std::string> generateContentTags(const SampleAnalysis& analysis);
    
    // Name Validation and Collision Handling
    bool isValidSampleName(const std::string& name) const;
    bool hasNameCollision(const std::string& name) const;
    std::string resolveNameCollision(const std::string& baseName);
    std::string sanitizeName(const std::string& name) const;
    
    // Template Processing
    std::string expandTemplate(const std::string& templateString, 
                             const SampleAnalysis& analysis,
                             uint8_t slotId,
                             const std::string& sourceContext = "");
    std::vector<std::string> getAvailableTemplateVariables() const;
    bool validateTemplate(const std::string& templateString) const;
    
    // User Name Management
    void setUserDefinedName(uint8_t slotId, const std::string& name);
    std::string getUserDefinedName(uint8_t slotId) const;
    bool hasUserDefinedName(uint8_t slotId) const;
    void clearUserDefinedName(uint8_t slotId);
    
    // Name History and Favorites
    void addToNameHistory(const std::string& name);
    std::vector<std::string> getNameHistory(uint8_t count = 10) const;
    void addToFavoritePatterns(const std::string& pattern);
    std::vector<std::string> getFavoritePatterns() const;
    void clearNameHistory();
    
    // Integration
    void integrateWithAutoSampleLoader(AutoSampleLoader* sampleLoader);
    void setSampleAccessCallback(std::function<const AutoSampleLoader::SamplerSlot&(uint8_t)> callback);
    void setNameCollisionCheckCallback(std::function<bool(const std::string&)> callback);
    
    // Real-time Validation
    struct ValidationResult {
        bool isValid;                      // Whether name is valid
        std::vector<std::string> errors;   // Validation errors
        std::vector<std::string> warnings; // Validation warnings
        std::vector<std::string> suggestions; // Improvement suggestions
        
        ValidationResult() : isValid(false) {}
    };
    
    ValidationResult validateNameRealTime(const std::string& name);
    std::vector<std::string> getNameCompletions(const std::string& partial);
    
    // Batch Operations
    void renameAllSamples(const NamingTemplate& nameTemplate);
    void autoNameUnnamedSamples();
    std::map<uint8_t, std::string> generateNamesForMultipleSamples(
        const std::vector<std::pair<uint8_t, std::shared_ptr<RealtimeAudioBouncer::CapturedAudio>>>& samples);
    
    // Import/Export
    bool exportNamingPreferences(const std::string& filePath) const;
    bool importNamingPreferences(const std::string& filePath);
    std::string serializePreferences() const;
    bool deserializePreferences(const std::string& data);
    
    // Callbacks
    using NameGeneratedCallback = std::function<void(uint8_t slotId, const std::string& name)>;
    using NameValidationCallback = std::function<void(const std::string& name, const ValidationResult& result)>;
    using ContentAnalysisCallback = std::function<void(const SampleAnalysis& analysis)>;
    
    void setNameGeneratedCallback(NameGeneratedCallback callback);
    void setNameValidationCallback(NameValidationCallback callback);
    void setContentAnalysisCallback(ContentAnalysisCallback callback);
    
private:
    // Configuration
    NamingPreferences preferences_;
    std::vector<NamingTemplate> customTemplates_;
    
    // Name tracking
    std::map<uint8_t, std::string> userDefinedNames_;
    std::vector<std::string> nameHistory_;
    std::vector<std::string> favoritePatterns_;
    
    // Integration
    AutoSampleLoader* sampleLoader_;
    std::function<const AutoSampleLoader::SamplerSlot&(uint8_t)> sampleAccessCallback_;
    std::function<bool(const std::string&)> nameCollisionCheckCallback_;
    
    // Callbacks
    NameGeneratedCallback nameGeneratedCallback_;
    NameValidationCallback nameValidationCallback_;
    ContentAnalysisCallback contentAnalysisCallback_;
    
    // Content analysis helpers
    float analyzeSpectralCentroid(const std::vector<float>& audioData) const;
    float analyzeSpectralBrightness(const std::vector<float>& audioData) const;
    float analyzeRhythmicity(const std::vector<float>& audioData) const;
    bool detectPercussiveContent(const std::vector<float>& audioData) const;
    bool detectHarmonicContent(const std::vector<float>& audioData) const;
    bool detectTonalContent(const std::vector<float>& audioData) const;
    float findDominantFrequency(const std::vector<float>& audioData) const;
    
    // Category classification
    ContentCategory classifyByFrequencyContent(float dominantFreq, float brightness) const;
    ContentCategory classifyByTemporalFeatures(bool isPercussive, float rhythmicity) const;
    ContentCategory classifyBySpectralFeatures(float brightness, bool isHarmonic) const;
    
    // Template processing
    std::string replaceTemplateVariables(const std::string& templateString,
                                        const std::map<std::string, std::string>& variables) const;
    std::map<std::string, std::string> buildVariableMap(const SampleAnalysis& analysis,
                                                        uint8_t slotId,
                                                        const std::string& sourceContext) const;
    
    // Name generation strategies
    std::string generateAutoAnalyzeName(const SampleAnalysis& analysis) const;
    std::string generateSequentialName(uint8_t slotId) const;
    std::string generateDescriptiveName(const SampleAnalysis& analysis) const;
    std::string generateSourceBasedName(const std::string& sourceContext, uint8_t slotId) const;
    std::string generateTimestampName() const;
    
    // Collision resolution strategies
    std::string addNumberSuffix(const std::string& baseName, uint16_t number) const;
    std::string addIntelligentSuffix(const std::string& baseName) const;
    uint16_t findNextAvailableNumber(const std::string& baseName) const;
    
    // Validation helpers
    bool containsValidCharacters(const std::string& name) const;
    bool isWithinLengthLimits(const std::string& name) const;
    bool isReservedName(const std::string& name) const;
    
    // Category name mappings
    void initializeDefaultCategoryNames();
    std::string getDefaultCategoryName(ContentCategory category) const;
    
    // History management
    void pruneNameHistory();
    void addUniqueToHistory(const std::string& name);
    
    // Utility methods
    std::string getCurrentTimestamp() const;
    std::string formatSlotNumber(uint8_t slotId, bool zeroPadded = true) const;
    std::string capitalizeFirst(const std::string& str) const;
    std::string removeInvalidCharacters(const std::string& str) const;
    float calculateConfidence(const SampleAnalysis& analysis) const;
    
    // Constants
    static constexpr uint8_t MAX_NAME_LENGTH = 64;
    static constexpr uint8_t MAX_HISTORY_SIZE = 50;
    static constexpr uint8_t MAX_FAVORITE_PATTERNS = 20;
    static constexpr float MIN_CONFIDENCE_THRESHOLD = 0.3f;
    
    // Default category names
    static const std::map<ContentCategory, std::string> DEFAULT_CATEGORY_NAMES;
    
    // Reserved names that cannot be used
    static const std::vector<std::string> RESERVED_NAMES;
};