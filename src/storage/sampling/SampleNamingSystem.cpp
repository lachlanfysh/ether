#include "SampleNamingSystem.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <random>

// Static member definitions
const std::map<SampleNamingSystem::ContentCategory, std::string> SampleNamingSystem::DEFAULT_CATEGORY_NAMES = {
    {ContentCategory::UNKNOWN, "Unknown"},
    {ContentCategory::KICK_DRUM, "Kick"},
    {ContentCategory::SNARE_DRUM, "Snare"},
    {ContentCategory::HI_HAT, "HiHat"},
    {ContentCategory::CYMBAL, "Cymbal"},
    {ContentCategory::TOM_DRUM, "Tom"},
    {ContentCategory::PERCUSSION, "Perc"},
    {ContentCategory::BASS_SOUND, "Bass"},
    {ContentCategory::LEAD_SOUND, "Lead"},
    {ContentCategory::PAD_SOUND, "Pad"},
    {ContentCategory::CHORD_SOUND, "Chord"},
    {ContentCategory::NOISE_SOUND, "Noise"},
    {ContentCategory::VOCAL_SOUND, "Vocal"},
    {ContentCategory::MELODIC, "Melody"},
    {ContentCategory::RHYTHMIC, "Rhythm"},
    {ContentCategory::AMBIENT, "Ambient"},
    {ContentCategory::EFFECT_SOUND, "FX"},
    {ContentCategory::LOOP, "Loop"},
    {ContentCategory::ONE_SHOT, "OneShot"}
};

const std::vector<std::string> SampleNamingSystem::RESERVED_NAMES = {
    "System", "Default", "Empty", "Null", "Error", "Unknown", "Temp", "Test"
};

SampleNamingSystem::SampleNamingSystem() {
    preferences_ = NamingPreferences();
    
    sampleLoader_ = nullptr;
    
    // Initialize default category names
    initializeDefaultCategoryNames();
    
    // Add some default templates
    NamingTemplate defaultTemplate;
    defaultTemplate.templateString = "{category}_{slot:02d}";
    defaultTemplate.strategy = NamingStrategy::TEMPLATE_BASED;
    customTemplates_.push_back(defaultTemplate);
    
    NamingTemplate descriptiveTemplate;
    descriptiveTemplate.templateString = "{category}_{brightness}_{timestamp}";
    descriptiveTemplate.strategy = NamingStrategy::DESCRIPTIVE;
    customTemplates_.push_back(descriptiveTemplate);
}

// Configuration
void SampleNamingSystem::setNamingPreferences(const NamingPreferences& preferences) {
    preferences_ = preferences;
    
    // Validate preferences
    preferences_.maxSuggestions = std::min(preferences_.maxSuggestions, static_cast<uint8_t>(10));
    
    // Update category names if user provided custom ones
    if (preferences_.categoryNames.empty()) {
        initializeDefaultCategoryNames();
    }
}

void SampleNamingSystem::addNamingTemplate(const NamingTemplate& nameTemplate) {
    if (validateTemplate(nameTemplate.templateString)) {
        customTemplates_.push_back(nameTemplate);
        
        // Limit number of templates
        if (customTemplates_.size() > 20) {
            customTemplates_.erase(customTemplates_.begin());
        }
    }
}

void SampleNamingSystem::removeNamingTemplate(const std::string& templateString) {
    customTemplates_.erase(
        std::remove_if(customTemplates_.begin(), customTemplates_.end(),
                      [&templateString](const NamingTemplate& t) {
                          return t.templateString == templateString;
                      }),
        customTemplates_.end());
}

// Name Generation
SampleNamingSystem::NameGenerationResult SampleNamingSystem::generateName(
    std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData,
    const std::string& sourceContext,
    uint8_t slotId) {
    
    NameGenerationResult result;
    
    if (!audioData) {
        result.suggestedName = "Empty";
        result.usedStrategy = NamingStrategy::AUTO_ANALYZE;
        result.confidence = 0.0f;
        return result;
    }
    
    // Analyze sample content
    result.analysis = analyzeSampleContent(audioData);
    
    // Generate name based on preferred strategy
    switch (preferences_.preferredStrategy) {
        case NamingStrategy::AUTO_ANALYZE:
            result.suggestedName = generateAutoAnalyzeName(result.analysis);
            break;
            
        case NamingStrategy::TEMPLATE_BASED:
            if (!customTemplates_.empty()) {
                result = generateNameFromTemplate(customTemplates_[0], result.analysis, slotId);
            } else {
                result.suggestedName = generateAutoAnalyzeName(result.analysis);
            }
            break;
            
        case NamingStrategy::SOURCE_BASED:
            result.suggestedName = generateSourceBasedName(sourceContext, slotId);
            break;
            
        case NamingStrategy::SEQUENTIAL:
            result.suggestedName = generateSequentialName(slotId);
            break;
            
        case NamingStrategy::DESCRIPTIVE:
            result.suggestedName = generateDescriptiveName(result.analysis);
            break;
            
        case NamingStrategy::TIMESTAMP_BASED:
            result.suggestedName = generateTimestampName();
            break;
            
        case NamingStrategy::HYBRID:
        default:
            // Combine multiple strategies
            std::string baseName = generateAutoAnalyzeName(result.analysis);
            if (slotId != 255) {
                baseName += "_" + formatSlotNumber(slotId);
            }
            result.suggestedName = baseName;
            break;
    }
    
    result.usedStrategy = preferences_.preferredStrategy;
    
    // Resolve name collisions
    if (hasNameCollision(result.suggestedName)) {
        result.hasCollision = true;
        result.collisionResolution = "Added number suffix";
        result.suggestedName = resolveNameCollision(result.suggestedName);
    }
    
    // Generate alternatives
    if (preferences_.enableAutoSuggestions) {
        result.alternatives = generateNameSuggestions(result.analysis, preferences_.maxSuggestions);
    }
    
    result.confidence = calculateConfidence(result.analysis);
    
    // Notify callback
    if (contentAnalysisCallback_) {
        contentAnalysisCallback_(result.analysis);
    }
    
    return result;
}

SampleNamingSystem::NameGenerationResult SampleNamingSystem::generateNameFromTemplate(
    const NamingTemplate& nameTemplate,
    const SampleAnalysis& analysis,
    uint8_t slotId) {
    
    NameGenerationResult result;
    result.analysis = analysis;
    result.usedStrategy = nameTemplate.strategy;
    
    // Expand template
    result.suggestedName = expandTemplate(nameTemplate.templateString, analysis, slotId);
    
    // Apply template-specific processing
    if (nameTemplate.enableContentAnalysis && analysis.confidence < MIN_CONFIDENCE_THRESHOLD) {
        // Low confidence in analysis - fall back to simpler name
        result.suggestedName = "Sample_" + formatSlotNumber(slotId);
    }
    
    // Resolve collisions if enabled
    if (nameTemplate.enableNumbering && hasNameCollision(result.suggestedName)) {
        result.hasCollision = true;
        result.suggestedName = resolveNameCollision(result.suggestedName);
    }
    
    result.confidence = calculateConfidence(analysis);
    
    return result;
}

std::vector<std::string> SampleNamingSystem::generateNameSuggestions(const SampleAnalysis& analysis, uint8_t count) {
    std::vector<std::string> suggestions;
    
    // Primary category-based name
    suggestions.push_back(getCategoryName(analysis.primaryCategory));
    
    // Secondary category if available
    if (analysis.secondaryCategory != ContentCategory::UNKNOWN && 
        analysis.secondaryCategory != analysis.primaryCategory) {
        suggestions.push_back(getCategoryName(analysis.secondaryCategory));
    }
    
    // Descriptive names based on characteristics
    if (analysis.isPercussive) {
        suggestions.push_back(getCategoryName(analysis.primaryCategory) + "_Hit");
    }
    
    if (analysis.isTonal) {
        suggestions.push_back(getCategoryName(analysis.primaryCategory) + "_Tonal");
    }
    
    if (analysis.brightness > 0.7f) {
        suggestions.push_back("Bright_" + getCategoryName(analysis.primaryCategory));
    } else if (analysis.brightness < 0.3f) {
        suggestions.push_back("Dark_" + getCategoryName(analysis.primaryCategory));
    }
    
    // Duration-based suggestions
    if (analysis.durationMs < 200) {
        suggestions.push_back(getCategoryName(analysis.primaryCategory) + "_Short");
    } else if (analysis.durationMs > 2000) {
        suggestions.push_back(getCategoryName(analysis.primaryCategory) + "_Long");
    }
    
    // Add tag-based suggestions
    for (const auto& tag : analysis.tags) {
        if (suggestions.size() >= count) break;
        suggestions.push_back(capitalizeFirst(tag) + "_" + getCategoryName(analysis.primaryCategory));
    }
    
    // Remove duplicates and truncate to requested count
    std::sort(suggestions.begin(), suggestions.end());
    suggestions.erase(std::unique(suggestions.begin(), suggestions.end()), suggestions.end());
    
    if (suggestions.size() > count) {
        suggestions.resize(count);
    }
    
    return suggestions;
}

// Content Analysis
SampleNamingSystem::SampleAnalysis SampleNamingSystem::analyzeSampleContent(
    std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData) {
    
    SampleAnalysis analysis;
    
    if (!audioData || audioData->audioData.empty()) {
        return analysis;
    }
    
    const auto& audio = audioData->audioData;
    
    // Basic analysis
    analysis.durationMs = (audioData->sampleCount * 1000) / audioData->format.sampleRate;
    analysis.peakFrequency = findDominantFrequency(audio);
    analysis.brightness = analyzeSpectralBrightness(audio);
    analysis.rhythmicity = analyzeRhythmicity(audio);
    analysis.dynamicRange = audioData->peakLevel - audioData->rmsLevel;
    
    // Feature detection
    analysis.isPercussive = detectPercussiveContent(audio);
    analysis.isHarmonic = detectHarmonicContent(audio);
    analysis.isTonal = detectTonalContent(audio);
    
    // Categorization
    analysis.primaryCategory = categorizeContent(analysis);
    analysis.confidence = std::min(1.0f, analysis.brightness * 0.5f + 
                                          (analysis.isPercussive ? 0.3f : 0.0f) +
                                          (analysis.isTonal ? 0.2f : 0.0f));
    
    // Generate descriptive tags
    analysis.tags = generateContentTags(analysis);
    
    return analysis;
}

SampleNamingSystem::ContentCategory SampleNamingSystem::categorizeContent(const SampleAnalysis& analysis) {
    // Frequency-based classification
    if (analysis.peakFrequency < 100.0f && analysis.isPercussive) {
        return ContentCategory::KICK_DRUM;
    }
    
    if (analysis.peakFrequency >= 100.0f && analysis.peakFrequency < 300.0f && analysis.isPercussive) {
        if (analysis.dynamicRange > 20.0f) {
            return ContentCategory::SNARE_DRUM;
        } else {
            return ContentCategory::TOM_DRUM;
        }
    }
    
    if (analysis.peakFrequency >= 300.0f && analysis.brightness > 0.7f && analysis.isPercussive) {
        if (analysis.durationMs < 100) {
            return ContentCategory::HI_HAT;
        } else {
            return ContentCategory::CYMBAL;
        }
    }
    
    if (analysis.peakFrequency < 200.0f && analysis.isTonal) {
        return ContentCategory::BASS_SOUND;
    }
    
    if (analysis.peakFrequency >= 200.0f && analysis.isTonal && analysis.brightness > 0.5f) {
        return ContentCategory::LEAD_SOUND;
    }
    
    if (analysis.isHarmonic && analysis.durationMs > 500) {
        return ContentCategory::PAD_SOUND;
    }
    
    if (analysis.rhythmicity > 0.7f) {
        return ContentCategory::RHYTHMIC;
    }
    
    if (analysis.isTonal && analysis.isHarmonic) {
        return ContentCategory::MELODIC;
    }
    
    if (analysis.brightness < 0.3f && analysis.durationMs > 1000) {
        return ContentCategory::AMBIENT;
    }
    
    if (analysis.isPercussive) {
        return ContentCategory::PERCUSSION;
    }
    
    return ContentCategory::UNKNOWN;
}

std::string SampleNamingSystem::getCategoryName(ContentCategory category) const {
    auto it = preferences_.categoryNames.find(category);
    if (it != preferences_.categoryNames.end()) {
        return it->second;
    }
    
    auto defaultIt = DEFAULT_CATEGORY_NAMES.find(category);
    if (defaultIt != DEFAULT_CATEGORY_NAMES.end()) {
        return defaultIt->second;
    }
    
    return "Unknown";
}

std::vector<std::string> SampleNamingSystem::generateContentTags(const SampleAnalysis& analysis) {
    std::vector<std::string> tags;
    
    // Brightness tags
    if (analysis.brightness > 0.8f) {
        tags.push_back("bright");
    } else if (analysis.brightness < 0.2f) {
        tags.push_back("dark");
    }
    
    // Dynamic range tags
    if (analysis.dynamicRange > 30.0f) {
        tags.push_back("punchy");
    } else if (analysis.dynamicRange < 10.0f) {
        tags.push_back("compressed");
    }
    
    // Duration tags
    if (analysis.durationMs < 100) {
        tags.push_back("short");
    } else if (analysis.durationMs > 2000) {
        tags.push_back("long");
    }
    
    // Character tags
    if (analysis.isPercussive) {
        tags.push_back("percussive");
    }
    
    if (analysis.isTonal) {
        tags.push_back("tonal");
    }
    
    if (analysis.isHarmonic) {
        tags.push_back("harmonic");
    }
    
    if (analysis.rhythmicity > 0.5f) {
        tags.push_back("rhythmic");
    }
    
    return tags;
}

// Name Validation and Collision Handling
bool SampleNamingSystem::isValidSampleName(const std::string& name) const {
    return containsValidCharacters(name) && 
           isWithinLengthLimits(name) && 
           !isReservedName(name) &&
           !name.empty();
}

bool SampleNamingSystem::hasNameCollision(const std::string& name) const {
    if (nameCollisionCheckCallback_) {
        return nameCollisionCheckCallback_(name);
    }
    
    // Check against user-defined names
    for (const auto& pair : userDefinedNames_) {
        if (pair.second == name) {
            return true;
        }
    }
    
    return false;
}

std::string SampleNamingSystem::resolveNameCollision(const std::string& baseName) {
    if (!hasNameCollision(baseName)) {
        return baseName;
    }
    
    if (preferences_.enableIntelligentNumbering) {
        return addIntelligentSuffix(baseName);
    } else {
        uint16_t nextNumber = findNextAvailableNumber(baseName);
        return addNumberSuffix(baseName, nextNumber);
    }
}

std::string SampleNamingSystem::sanitizeName(const std::string& name) const {
    std::string sanitized = removeInvalidCharacters(name);
    
    // Trim to maximum length
    if (sanitized.length() > MAX_NAME_LENGTH) {
        sanitized = sanitized.substr(0, MAX_NAME_LENGTH);
    }
    
    // Ensure name is not empty
    if (sanitized.empty()) {
        sanitized = "Sample";
    }
    
    return sanitized;
}

// Template Processing
std::string SampleNamingSystem::expandTemplate(const std::string& templateString,
                                              const SampleAnalysis& analysis,
                                              uint8_t slotId,
                                              const std::string& sourceContext) {
    
    std::map<std::string, std::string> variables = buildVariableMap(analysis, slotId, sourceContext);
    return replaceTemplateVariables(templateString, variables);
}

std::vector<std::string> SampleNamingSystem::getAvailableTemplateVariables() const {
    return {
        "category", "slot", "timestamp", "brightness", "duration", 
        "pitch", "peak_freq", "dynamic_range", "tags", "source"
    };
}

bool SampleNamingSystem::validateTemplate(const std::string& templateString) const {
    // Check for valid variable syntax
    size_t pos = 0;
    while ((pos = templateString.find("{", pos)) != std::string::npos) {
        size_t endPos = templateString.find("}", pos);
        if (endPos == std::string::npos) {
            return false;  // Unclosed variable
        }
        
        std::string variable = templateString.substr(pos + 1, endPos - pos - 1);
        
        // Check if variable contains format specifier
        size_t colonPos = variable.find(":");
        if (colonPos != std::string::npos) {
            variable = variable.substr(0, colonPos);
        }
        
        // Validate variable name
        auto availableVars = getAvailableTemplateVariables();
        if (std::find(availableVars.begin(), availableVars.end(), variable) == availableVars.end()) {
            return false;  // Unknown variable
        }
        
        pos = endPos + 1;
    }
    
    return true;
}

// User Name Management
void SampleNamingSystem::setUserDefinedName(uint8_t slotId, const std::string& name) {
    std::string sanitizedName = sanitizeName(name);
    
    if (isValidSampleName(sanitizedName)) {
        userDefinedNames_[slotId] = sanitizedName;
        addToNameHistory(sanitizedName);
        
        if (nameGeneratedCallback_) {
            nameGeneratedCallback_(slotId, sanitizedName);
        }
    }
}

std::string SampleNamingSystem::getUserDefinedName(uint8_t slotId) const {
    auto it = userDefinedNames_.find(slotId);
    return (it != userDefinedNames_.end()) ? it->second : "";
}

bool SampleNamingSystem::hasUserDefinedName(uint8_t slotId) const {
    return userDefinedNames_.find(slotId) != userDefinedNames_.end();
}

void SampleNamingSystem::clearUserDefinedName(uint8_t slotId) {
    userDefinedNames_.erase(slotId);
}

// Name History and Favorites
void SampleNamingSystem::addToNameHistory(const std::string& name) {
    addUniqueToHistory(name);
    pruneNameHistory();
}

std::vector<std::string> SampleNamingSystem::getNameHistory(uint8_t count) const {
    count = std::min(count, static_cast<uint8_t>(nameHistory_.size()));
    return std::vector<std::string>(nameHistory_.begin(), nameHistory_.begin() + count);
}

void SampleNamingSystem::addToFavoritePatterns(const std::string& pattern) {
    auto it = std::find(favoritePatterns_.begin(), favoritePatterns_.end(), pattern);
    if (it == favoritePatterns_.end()) {
        favoritePatterns_.push_back(pattern);
        
        // Limit favorites
        if (favoritePatterns_.size() > MAX_FAVORITE_PATTERNS) {
            favoritePatterns_.erase(favoritePatterns_.begin());
        }
    }
}

std::vector<std::string> SampleNamingSystem::getFavoritePatterns() const {
    return favoritePatterns_;
}

void SampleNamingSystem::clearNameHistory() {
    nameHistory_.clear();
}

// Integration
void SampleNamingSystem::integrateWithAutoSampleLoader(AutoSampleLoader* sampleLoader) {
    sampleLoader_ = sampleLoader;
}

void SampleNamingSystem::setSampleAccessCallback(std::function<const AutoSampleLoader::SamplerSlot&(uint8_t)> callback) {
    sampleAccessCallback_ = callback;
}

void SampleNamingSystem::setNameCollisionCheckCallback(std::function<bool(const std::string&)> callback) {
    nameCollisionCheckCallback_ = callback;
}

// Real-time Validation
SampleNamingSystem::ValidationResult SampleNamingSystem::validateNameRealTime(const std::string& name) {
    ValidationResult result;
    
    result.isValid = isValidSampleName(name);
    
    if (name.empty()) {
        result.errors.push_back("Name cannot be empty");
    }
    
    if (!containsValidCharacters(name)) {
        result.errors.push_back("Name contains invalid characters");
        result.suggestions.push_back("Use only letters, numbers, and underscores");
    }
    
    if (!isWithinLengthLimits(name)) {
        result.errors.push_back("Name exceeds maximum length");
        result.suggestions.push_back("Shorten name to " + std::to_string(MAX_NAME_LENGTH) + " characters");
    }
    
    if (isReservedName(name)) {
        result.errors.push_back("Name is reserved");
        result.suggestions.push_back("Choose a different name");
    }
    
    if (hasNameCollision(name)) {
        result.warnings.push_back("Name already exists");
        result.suggestions.push_back("Add number suffix or choose unique name");
    }
    
    if (nameValidationCallback_) {
        nameValidationCallback_(name, result);
    }
    
    return result;
}

std::vector<std::string> SampleNamingSystem::getNameCompletions(const std::string& partial) {
    std::vector<std::string> completions;
    
    // Check name history
    for (const auto& historyName : nameHistory_) {
        if (historyName.find(partial) == 0) {  // Starts with partial
            completions.push_back(historyName);
        }
    }
    
    // Check category names
    for (const auto& pair : DEFAULT_CATEGORY_NAMES) {
        if (pair.second.find(partial) == 0) {
            completions.push_back(pair.second);
        }
    }
    
    // Remove duplicates and sort
    std::sort(completions.begin(), completions.end());
    completions.erase(std::unique(completions.begin(), completions.end()), completions.end());
    
    // Limit results
    if (completions.size() > 10) {
        completions.resize(10);
    }
    
    return completions;
}

// Batch Operations
void SampleNamingSystem::renameAllSamples(const NamingTemplate& nameTemplate) {
    if (!sampleAccessCallback_) return;
    
    for (uint8_t i = 0; i < 16; ++i) {  // Assuming 16 sample slots
        const auto& slot = sampleAccessCallback_(i);
        if (slot.isOccupied && slot.audioData) {
            SampleAnalysis analysis = analyzeSampleContent(slot.audioData);
            std::string newName = expandTemplate(nameTemplate.templateString, analysis, i);
            
            if (hasNameCollision(newName)) {
                newName = resolveNameCollision(newName);
            }
            
            setUserDefinedName(i, newName);
        }
    }
}

void SampleNamingSystem::autoNameUnnamedSamples() {
    if (!sampleAccessCallback_) return;
    
    for (uint8_t i = 0; i < 16; ++i) {
        if (!hasUserDefinedName(i)) {
            const auto& slot = sampleAccessCallback_(i);
            if (slot.isOccupied && slot.audioData) {
                auto result = generateName(slot.audioData, "", i);
                setUserDefinedName(i, result.suggestedName);
            }
        }
    }
}

std::map<uint8_t, std::string> SampleNamingSystem::generateNamesForMultipleSamples(
    const std::vector<std::pair<uint8_t, std::shared_ptr<RealtimeAudioBouncer::CapturedAudio>>>& samples) {
    
    std::map<uint8_t, std::string> names;
    
    for (const auto& pair : samples) {
        uint8_t slotId = pair.first;
        auto audioData = pair.second;
        
        auto result = generateName(audioData, "", slotId);
        names[slotId] = result.suggestedName;
    }
    
    return names;
}

// Callbacks
void SampleNamingSystem::setNameGeneratedCallback(NameGeneratedCallback callback) {
    nameGeneratedCallback_ = callback;
}

void SampleNamingSystem::setNameValidationCallback(NameValidationCallback callback) {
    nameValidationCallback_ = callback;
}

void SampleNamingSystem::setContentAnalysisCallback(ContentAnalysisCallback callback) {
    contentAnalysisCallback_ = callback;
}

// Content analysis helpers
float SampleNamingSystem::analyzeSpectralCentroid(const std::vector<float>& audioData) const {
    // Mock implementation - in real system would perform FFT and calculate spectral centroid
    float sum = 0.0f;
    float weightedSum = 0.0f;
    
    for (size_t i = 0; i < audioData.size(); ++i) {
        float magnitude = std::abs(audioData[i]);
        sum += magnitude;
        weightedSum += magnitude * i;
    }
    
    return (sum > 0.0f) ? weightedSum / sum : 0.0f;
}

float SampleNamingSystem::analyzeSpectralBrightness(const std::vector<float>& audioData) const {
    // Estimate brightness based on high-frequency content
    float highFreqEnergy = 0.0f;
    float totalEnergy = 0.0f;
    
    size_t midPoint = audioData.size() / 2;
    
    for (size_t i = 0; i < audioData.size(); ++i) {
        float energy = audioData[i] * audioData[i];
        totalEnergy += energy;
        
        if (i > midPoint) {
            highFreqEnergy += energy;
        }
    }
    
    return (totalEnergy > 0.0f) ? highFreqEnergy / totalEnergy : 0.0f;
}

float SampleNamingSystem::analyzeRhythmicity(const std::vector<float>& audioData) const {
    // Simple rhythm detection based on amplitude variation
    if (audioData.size() < 100) return 0.0f;
    
    float variation = 0.0f;
    for (size_t i = 1; i < audioData.size(); ++i) {
        variation += std::abs(audioData[i] - audioData[i-1]);
    }
    
    return std::min(1.0f, variation / audioData.size());
}

bool SampleNamingSystem::detectPercussiveContent(const std::vector<float>& audioData) const {
    if (audioData.empty()) return false;
    
    // Check for sharp attack
    float maxValue = *std::max_element(audioData.begin(), 
                                      audioData.begin() + std::min(size_t(100), audioData.size()));
    
    // If peak occurs early and is significant, likely percussive
    return maxValue > 0.5f;
}

bool SampleNamingSystem::detectHarmonicContent(const std::vector<float>& audioData) const {
    // Mock implementation - in real system would analyze harmonic content
    return analyzeSpectralBrightness(audioData) > 0.3f && analyzeSpectralBrightness(audioData) < 0.8f;
}

bool SampleNamingSystem::detectTonalContent(const std::vector<float>& audioData) const {
    // Mock implementation - in real system would detect tonal vs. noise content
    float spectralCentroid = analyzeSpectralCentroid(audioData);
    return spectralCentroid > 0.1f && spectralCentroid < 0.9f;
}

float SampleNamingSystem::findDominantFrequency(const std::vector<float>& audioData) const {
    // Mock implementation - in real system would use FFT to find dominant frequency
    // Return a frequency estimate based on spectral centroid
    float centroid = analyzeSpectralCentroid(audioData);
    return centroid * 12000.0f;  // Scale to reasonable frequency range
}

// Name generation strategies
std::string SampleNamingSystem::generateAutoAnalyzeName(const SampleAnalysis& analysis) const {
    std::string baseName = getCategoryName(analysis.primaryCategory);
    
    // Add descriptive suffixes based on analysis
    if (analysis.brightness > 0.8f) {
        baseName += "_Bright";
    } else if (analysis.brightness < 0.2f) {
        baseName += "_Dark";
    }
    
    if (analysis.durationMs < 200) {
        baseName += "_Short";
    } else if (analysis.durationMs > 2000) {
        baseName += "_Long";
    }
    
    return baseName;
}

std::string SampleNamingSystem::generateSequentialName(uint8_t slotId) const {
    return "Sample_" + formatSlotNumber(slotId, true);
}

std::string SampleNamingSystem::generateDescriptiveName(const SampleAnalysis& analysis) const {
    std::ostringstream oss;
    
    oss << getCategoryName(analysis.primaryCategory);
    
    if (analysis.brightness > 0.7f) {
        oss << "_Bright";
    } else if (analysis.brightness < 0.3f) {
        oss << "_Dark";
    }
    
    if (analysis.dynamicRange > 20.0f) {
        oss << "_Punchy";
    }
    
    if (analysis.isPercussive) {
        oss << "_Hit";
    }
    
    return oss.str();
}

std::string SampleNamingSystem::generateSourceBasedName(const std::string& sourceContext, uint8_t slotId) const {
    if (!sourceContext.empty()) {
        return sourceContext + "_" + formatSlotNumber(slotId);
    }
    return "Sample_" + formatSlotNumber(slotId);
}

std::string SampleNamingSystem::generateTimestampName() const {
    return "Sample_" + getCurrentTimestamp();
}

// Template processing helpers
std::string SampleNamingSystem::replaceTemplateVariables(const std::string& templateString,
                                                        const std::map<std::string, std::string>& variables) const {
    std::string result = templateString;
    
    for (const auto& pair : variables) {
        std::string variable = "{" + pair.first + "}";
        size_t pos = 0;
        
        while ((pos = result.find(variable, pos)) != std::string::npos) {
            result.replace(pos, variable.length(), pair.second);
            pos += pair.second.length();
        }
        
        // Handle format specifiers like {slot:02d}
        std::string formatVariable = "{" + pair.first + ":";
        pos = 0;
        while ((pos = result.find(formatVariable, pos)) != std::string::npos) {
            size_t endPos = result.find("}", pos);
            if (endPos != std::string::npos) {
                std::string formatSpec = result.substr(pos + formatVariable.length(), 
                                                      endPos - pos - formatVariable.length());
                
                // Apply format specification (simplified)
                std::string formattedValue = pair.second;
                if (formatSpec == "02d" && pair.first == "slot") {
                    int slotNum = std::stoi(pair.second);
                    std::ostringstream oss;
                    oss << std::setw(2) << std::setfill('0') << slotNum;
                    formattedValue = oss.str();
                }
                
                result.replace(pos, endPos - pos + 1, formattedValue);
                pos += formattedValue.length();
            } else {
                break;
            }
        }
    }
    
    return result;
}

std::map<std::string, std::string> SampleNamingSystem::buildVariableMap(const SampleAnalysis& analysis,
                                                                       uint8_t slotId,
                                                                       const std::string& sourceContext) const {
    std::map<std::string, std::string> variables;
    
    variables["category"] = getCategoryName(analysis.primaryCategory);
    variables["slot"] = std::to_string(slotId + 1);  // 1-based for display
    variables["timestamp"] = getCurrentTimestamp();
    variables["brightness"] = (analysis.brightness > 0.5f) ? "Bright" : "Dark";
    variables["duration"] = (analysis.durationMs < 500) ? "Short" : "Long";
    variables["pitch"] = (analysis.peakFrequency < 200) ? "Low" : "High";
    variables["peak_freq"] = std::to_string(static_cast<int>(analysis.peakFrequency));
    variables["dynamic_range"] = std::to_string(static_cast<int>(analysis.dynamicRange));
    variables["source"] = sourceContext;
    
    // Tags
    if (!analysis.tags.empty()) {
        variables["tags"] = analysis.tags[0];  // Use first tag
    } else {
        variables["tags"] = "";
    }
    
    return variables;
}

// Collision resolution strategies
std::string SampleNamingSystem::addNumberSuffix(const std::string& baseName, uint16_t number) const {
    if (preferences_.enableIntelligentNumbering && number < 100) {
        return baseName + "_" + (number < 10 ? "0" : "") + std::to_string(number);
    } else {
        return baseName + "_" + std::to_string(number);
    }
}

std::string SampleNamingSystem::addIntelligentSuffix(const std::string& baseName) const {
    // Try different suffix strategies
    std::vector<std::string> suffixes = {"_Alt", "_B", "_2", "_New", "_v2"};
    
    for (const auto& suffix : suffixes) {
        std::string candidate = baseName + suffix;
        if (!hasNameCollision(candidate)) {
            return candidate;
        }
    }
    
    // Fall back to number suffix
    return addNumberSuffix(baseName, findNextAvailableNumber(baseName));
}

uint16_t SampleNamingSystem::findNextAvailableNumber(const std::string& baseName) const {
    uint16_t number = 1;
    
    while (number < 1000) {  // Reasonable limit
        std::string candidate = addNumberSuffix(baseName, number);
        if (!hasNameCollision(candidate)) {
            return number;
        }
        ++number;
    }
    
    return number;
}

// Validation helpers
bool SampleNamingSystem::containsValidCharacters(const std::string& name) const {
    for (char c : name) {
        if (!std::isalnum(c) && c != '_' && c != '-' && c != ' ') {
            return false;
        }
    }
    return true;
}

bool SampleNamingSystem::isWithinLengthLimits(const std::string& name) const {
    return name.length() > 0 && name.length() <= MAX_NAME_LENGTH;
}

bool SampleNamingSystem::isReservedName(const std::string& name) const {
    return std::find(RESERVED_NAMES.begin(), RESERVED_NAMES.end(), name) != RESERVED_NAMES.end();
}

// Category name management
void SampleNamingSystem::initializeDefaultCategoryNames() {
    if (preferences_.categoryNames.empty()) {
        preferences_.categoryNames = DEFAULT_CATEGORY_NAMES;
    }
}

std::string SampleNamingSystem::getDefaultCategoryName(ContentCategory category) const {
    auto it = DEFAULT_CATEGORY_NAMES.find(category);
    return (it != DEFAULT_CATEGORY_NAMES.end()) ? it->second : "Unknown";
}

// History management
void SampleNamingSystem::pruneNameHistory() {
    if (nameHistory_.size() > MAX_HISTORY_SIZE) {
        nameHistory_.resize(MAX_HISTORY_SIZE);
    }
}

void SampleNamingSystem::addUniqueToHistory(const std::string& name) {
    // Remove existing entry if present
    auto it = std::find(nameHistory_.begin(), nameHistory_.end(), name);
    if (it != nameHistory_.end()) {
        nameHistory_.erase(it);
    }
    
    // Add to front
    nameHistory_.insert(nameHistory_.begin(), name);
}

// Utility methods
std::string SampleNamingSystem::getCurrentTimestamp() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return std::to_string(millis);
}

std::string SampleNamingSystem::formatSlotNumber(uint8_t slotId, bool zeroPadded) const {
    if (zeroPadded) {
        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << (slotId + 1);
        return oss.str();
    } else {
        return std::to_string(slotId + 1);
    }
}

std::string SampleNamingSystem::capitalizeFirst(const std::string& str) const {
    if (str.empty()) return str;
    
    std::string result = str;
    result[0] = std::toupper(result[0]);
    return result;
}

std::string SampleNamingSystem::removeInvalidCharacters(const std::string& str) const {
    std::string result;
    
    for (char c : str) {
        if (std::isalnum(c) || c == '_' || c == '-' || c == ' ') {
            result += c;
        }
    }
    
    return result;
}

float SampleNamingSystem::calculateConfidence(const SampleAnalysis& analysis) const {
    float confidence = 0.0f;
    
    // Confidence based on analysis results
    if (analysis.primaryCategory != ContentCategory::UNKNOWN) {
        confidence += 0.4f;
    }
    
    if (analysis.peakFrequency > 0.0f) {
        confidence += 0.2f;
    }
    
    if (analysis.isPercussive || analysis.isTonal || analysis.isHarmonic) {
        confidence += 0.2f;
    }
    
    if (analysis.brightness > 0.1f && analysis.brightness < 0.9f) {
        confidence += 0.1f;
    }
    
    if (!analysis.tags.empty()) {
        confidence += 0.1f;
    }
    
    return std::min(1.0f, confidence);
}