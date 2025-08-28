#include "PresetManager.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <sstream>

PresetManager::PresetManager() {
    std::cout << "PresetManager created" << std::endl;
    
    // Set up directory structure
    presetDirectory_ = "./presets";
    factoryDirectory_ = presetDirectory_ + "/factory";
    userDirectory_ = presetDirectory_ + "/user";
    
    if (!initializeDirectories()) {
        std::cout << "Warning: Could not create preset directories" << std::endl;
    }
    
    // Load factory presets
    loadFactoryPresets();
    
    std::cout << "PresetManager: Initialized with " << presets_.size() << " presets" << std::endl;
}

PresetManager::~PresetManager() {
    // Auto-save if enabled
    if (autoSaveEnabled_ && !autoSavePresetName_.empty()) {
        // Would save current state to auto-save preset
    }
    
    std::cout << "PresetManager destroyed" << std::endl;
}

bool PresetManager::savePreset(const Preset& preset) {
    if (!isValidPresetName(preset.name)) {
        lastError_ = PresetError::INVALID_NAME;
        return false;
    }
    
    // Check if preset already exists
    if (presets_.find(preset.name) != presets_.end() && isFactoryPreset(preset.name)) {
        lastError_ = PresetError::PRESET_EXISTS;
        return false;
    }
    
    // Validate preset
    if (!validatePreset(preset)) {
        lastError_ = PresetError::INVALID_FORMAT;
        return false;
    }
    
    // Create a copy with updated timestamps
    Preset presetToSave = preset;
    auto now = std::chrono::system_clock::now();
    presetToSave.modifiedTime = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    
    if (presets_.find(preset.name) == presets_.end()) {
        presetToSave.createdTime = presetToSave.modifiedTime;
    }
    
    // Save to file
    std::string filePath = getPresetPath(preset.name);
    if (!savePresetToFile(presetToSave, filePath)) {
        lastError_ = PresetError::WRITE_FAILED;
        return false;
    }
    
    // Store in memory
    presets_[preset.name] = presetToSave;
    
    // Add to category if specified
    if (!preset.category.empty()) {
        categories_[preset.category].push_back(preset.name);
    }
    
    lastError_ = PresetError::NONE;
    std::cout << "Saved preset: " << preset.name << std::endl;
    return true;
}

bool PresetManager::loadPreset(const std::string& name, Preset& preset) {
    auto it = presets_.find(name);
    if (it != presets_.end()) {
        preset = it->second;
        setLastUsedPreset(name);
        lastError_ = PresetError::NONE;
        std::cout << "Loaded preset: " << name << std::endl;
        return true;
    }
    
    // Try loading from file
    std::string filePath = getPresetPath(name);
    if (loadPresetFromFile(filePath, preset)) {
        presets_[name] = preset;
        setLastUsedPreset(name);
        lastError_ = PresetError::NONE;
        std::cout << "Loaded preset from file: " << name << std::endl;
        return true;
    }
    
    lastError_ = PresetError::FILE_NOT_FOUND;
    return false;
}

bool PresetManager::deletePreset(const std::string& name) {
    if (isFactoryPreset(name)) {
        lastError_ = PresetError::PERMISSION_DENIED;
        return false;
    }
    
    auto it = presets_.find(name);
    if (it == presets_.end()) {
        lastError_ = PresetError::FILE_NOT_FOUND;
        return false;
    }
    
    // Remove from file system
    std::string filePath = getPresetPath(name);
    try {
        std::filesystem::remove(filePath);
    } catch (const std::exception&) {
        lastError_ = PresetError::PERMISSION_DENIED;
        return false;
    }
    
    // Remove from memory
    presets_.erase(it);
    
    // Remove from categories
    for (auto& category : categories_) {
        auto& presetList = category.second;
        presetList.erase(std::remove(presetList.begin(), presetList.end(), name), presetList.end());
    }
    
    // Remove from favorites
    favoritePresets_.erase(std::remove(favoritePresets_.begin(), favoritePresets_.end(), name), 
                          favoritePresets_.end());
    
    lastError_ = PresetError::NONE;
    std::cout << "Deleted preset: " << name << std::endl;
    return true;
}

bool PresetManager::renamePreset(const std::string& oldName, const std::string& newName) {
    if (isFactoryPreset(oldName)) {
        lastError_ = PresetError::PERMISSION_DENIED;
        return false;
    }
    
    if (!isValidPresetName(newName)) {
        lastError_ = PresetError::INVALID_NAME;
        return false;
    }
    
    auto it = presets_.find(oldName);
    if (it == presets_.end()) {
        lastError_ = PresetError::FILE_NOT_FOUND;
        return false;
    }
    
    if (presets_.find(newName) != presets_.end()) {
        lastError_ = PresetError::PRESET_EXISTS;
        return false;
    }
    
    // Create new preset with new name
    Preset preset = it->second;
    preset.name = newName;
    
    // Save with new name
    if (!savePreset(preset)) {
        return false;
    }
    
    // Delete old preset
    deletePreset(oldName);
    
    std::cout << "Renamed preset: " << oldName << " -> " << newName << std::endl;
    return true;
}

bool PresetManager::duplicatePreset(const std::string& sourceName, const std::string& newName) {
    if (!isValidPresetName(newName)) {
        lastError_ = PresetError::INVALID_NAME;
        return false;
    }
    
    auto it = presets_.find(sourceName);
    if (it == presets_.end()) {
        lastError_ = PresetError::FILE_NOT_FOUND;
        return false;
    }
    
    if (presets_.find(newName) != presets_.end()) {
        lastError_ = PresetError::PRESET_EXISTS;
        return false;
    }
    
    // Create duplicate
    Preset preset = it->second;
    preset.name = newName;
    preset.description = "Copy of " + sourceName;
    
    return savePreset(preset);
}

std::vector<std::string> PresetManager::getPresetNames() const {
    std::vector<std::string> names;
    names.reserve(presets_.size());
    
    for (const auto& preset : presets_) {
        names.push_back(preset.first);
    }
    
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::string> PresetManager::getPresetsByCategory(const std::string& category) const {
    auto it = categories_.find(category);
    if (it != categories_.end()) {
        return it->second;
    }
    return {};
}

std::vector<std::string> PresetManager::getCategories() const {
    std::vector<std::string> cats;
    cats.reserve(categories_.size());
    
    for (const auto& category : categories_) {
        cats.push_back(category.first);
    }
    
    std::sort(cats.begin(), cats.end());
    return cats;
}

std::vector<PresetManager::Preset> PresetManager::searchPresets(const std::string& query) const {
    std::vector<Preset> results;
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    for (const auto& presetPair : presets_) {
        const Preset& preset = presetPair.second;
        
        // Search in name, description, category, and author
        std::string searchText = preset.name + " " + preset.description + " " + 
                                preset.category + " " + preset.author;
        std::transform(searchText.begin(), searchText.end(), searchText.begin(), ::tolower);
        
        if (searchText.find(lowerQuery) != std::string::npos) {
            results.push_back(preset);
        }
    }
    
    return results;
}

void PresetManager::loadFactoryPresets() {
    std::cout << "Loading factory presets..." << std::endl;
    
    // Create classic synthesizer presets
    createFactoryPreset("TB-303 Bass", "Classic acid bass sound", EngineType::SUBTRACTIVE, FactoryPresets::BASS);
    createFactoryPreset("Moog Lead", "Fat analog lead synth", EngineType::SUBTRACTIVE, FactoryPresets::LEAD);
    createFactoryPreset("DX7 E.Piano", "Classic FM electric piano", EngineType::FM, FactoryPresets::PLUCK);
    createFactoryPreset("Juno Strings", "Lush analog strings", EngineType::SUBTRACTIVE, FactoryPresets::PAD);
    createFactoryPreset("SH-101 Acid", "Roland SH-101 style acid", EngineType::SUBTRACTIVE, FactoryPresets::BASS);
    
    // Modern sounds
    createFactoryPreset("Future Bass", "Modern future bass lead", EngineType::WAVETABLE, FactoryPresets::LEAD);
    createFactoryPreset("Dubstep Wobble", "Aggressive dubstep bass", EngineType::FM, FactoryPresets::BASS);
    createFactoryPreset("Ambient Texture", "Evolving ambient pad", EngineType::GRANULAR, FactoryPresets::PAD);
    createFactoryPreset("Granular Cloud", "Atmospheric granular", EngineType::GRANULAR, FactoryPresets::FX);
    createFactoryPreset("FM Bell", "Bright FM bell sound", EngineType::FM, FactoryPresets::PLUCK);
    
    // Color-themed presets
    createFactoryPreset("Red Fire", "Aggressive bass synth", EngineType::SUBTRACTIVE, FactoryPresets::BASS);
    createFactoryPreset("Orange Warm", "Warm analog lead", EngineType::SUBTRACTIVE, FactoryPresets::LEAD);
    createFactoryPreset("Yellow Bright", "Bright plucked synth", EngineType::WAVETABLE, FactoryPresets::PLUCK);
    createFactoryPreset("Green Organic", "Natural evolving pad", EngineType::GRANULAR, FactoryPresets::PAD);
    createFactoryPreset("Blue Deep", "Deep string ensemble", EngineType::SUBTRACTIVE, FactoryPresets::PAD);
    createFactoryPreset("Indigo Mystic", "Mysterious atmospheric FX", EngineType::GRANULAR, FactoryPresets::FX);
    createFactoryPreset("Violet Ethereal", "Ethereal lead synth", EngineType::WAVETABLE, FactoryPresets::LEAD);
    createFactoryPreset("Grey Utility", "Basic utility sound", EngineType::SUBTRACTIVE, FactoryPresets::TEMPLATE);
    
    std::cout << "Loaded " << factoryPresetNames_.size() << " factory presets" << std::endl;
}

bool PresetManager::isFactoryPreset(const std::string& name) const {
    return std::find(factoryPresetNames_.begin(), factoryPresetNames_.end(), name) != factoryPresetNames_.end();
}

bool PresetManager::validatePreset(const Preset& preset) const {
    // Check required fields
    if (preset.name.empty() || preset.name.length() > MAX_PRESET_NAME_LENGTH) {
        return false;
    }
    
    if (preset.description.length() > MAX_DESCRIPTION_LENGTH) {
        return false;
    }
    
    if (preset.category.length() > MAX_CATEGORY_LENGTH) {
        return false;
    }
    
    if (preset.author.length() > MAX_AUTHOR_LENGTH) {
        return false;
    }
    
    // Validate engine type
    if (preset.engineType >= EngineType::COUNT) {
        return false;
    }
    
    return true;
}

std::string PresetManager::getPresetPath(const std::string& name) const {
    std::string sanitized = sanitizePresetName(name);
    
    if (isFactoryPreset(name)) {
        return factoryDirectory_ + "/" + sanitized + ".epr";
    } else {
        return userDirectory_ + "/" + sanitized + ".epr";
    }
}

bool PresetManager::exportPreset(const std::string& name, const std::string& filePath) const {
    auto it = presets_.find(name);
    if (it == presets_.end()) {
        return false;
    }
    
    return savePresetToFile(it->second, filePath);
}

bool PresetManager::importPreset(const std::string& filePath) {
    Preset preset;
    if (!loadPresetFromFile(filePath, preset)) {
        return false;
    }
    
    // Generate unique name if needed
    if (presets_.find(preset.name) != presets_.end()) {
        preset.name = generateUniquePresetName(preset.name);
    }
    
    return savePreset(preset);
}

bool PresetManager::exportPresetBank(const std::vector<std::string>& presetNames, const std::string& filePath) const {
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    try {
        // Write bank header
        uint32_t magic = PRESET_FILE_MAGIC;
        uint32_t version = PRESET_FILE_VERSION;
        uint32_t count = static_cast<uint32_t>(presetNames.size());
        
        file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));
        
        // Write each preset
        for (const std::string& name : presetNames) {
            auto it = presets_.find(name);
            if (it != presets_.end()) {
                std::vector<uint8_t> presetData = serializePreset(it->second);
                uint32_t dataSize = static_cast<uint32_t>(presetData.size());
                
                file.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
                file.write(reinterpret_cast<const char*>(presetData.data()), dataSize);
            }
        }
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool PresetManager::importPresetBank(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    try {
        // Read bank header
        uint32_t magic, version, count;
        file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        
        if (magic != PRESET_FILE_MAGIC || version != PRESET_FILE_VERSION) {
            return false;
        }
        
        // Read each preset
        for (uint32_t i = 0; i < count; i++) {
            uint32_t dataSize;
            file.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
            
            std::vector<uint8_t> presetData(dataSize);
            file.read(reinterpret_cast<char*>(presetData.data()), dataSize);
            
            Preset preset;
            if (deserializePreset(presetData, preset)) {
                // Generate unique name if needed
                if (presets_.find(preset.name) != presets_.end()) {
                    preset.name = generateUniquePresetName(preset.name);
                }
                savePreset(preset);
            }
        }
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool PresetManager::createCategory(const std::string& category) {
    if (categories_.find(category) != categories_.end()) {
        return false; // Already exists
    }
    
    categories_[category] = std::vector<std::string>();
    std::cout << "Created category: " << category << std::endl;
    return true;
}

bool PresetManager::deleteCategory(const std::string& category) {
    auto it = categories_.find(category);
    if (it == categories_.end()) {
        return false;
    }
    
    // Move presets to uncategorized
    for (const std::string& presetName : it->second) {
        auto presetIt = presets_.find(presetName);
        if (presetIt != presets_.end()) {
            presetIt->second.category = "";
        }
    }
    
    categories_.erase(it);
    std::cout << "Deleted category: " << category << std::endl;
    return true;
}

bool PresetManager::movePresetToCategory(const std::string& presetName, const std::string& category) {
    auto presetIt = presets_.find(presetName);
    if (presetIt == presets_.end()) {
        return false;
    }
    
    // Remove from old category
    std::string oldCategory = presetIt->second.category;
    if (!oldCategory.empty()) {
        auto categoryIt = categories_.find(oldCategory);
        if (categoryIt != categories_.end()) {
            auto& presetList = categoryIt->second;
            presetList.erase(std::remove(presetList.begin(), presetList.end(), presetName), 
                           presetList.end());
        }
    }
    
    // Add to new category
    presetIt->second.category = category;
    if (!category.empty()) {
        categories_[category].push_back(presetName);
    }
    
    return true;
}

void PresetManager::setFavoritePreset(const std::string& name, bool favorite) {
    auto it = std::find(favoritePresets_.begin(), favoritePresets_.end(), name);
    
    if (favorite && it == favoritePresets_.end()) {
        favoritePresets_.push_back(name);
        std::cout << "Added to favorites: " << name << std::endl;
    } else if (!favorite && it != favoritePresets_.end()) {
        favoritePresets_.erase(it);
        std::cout << "Removed from favorites: " << name << std::endl;
    }
}

std::vector<std::string> PresetManager::getFavoritePresets() const {
    return favoritePresets_;
}

void PresetManager::setLastUsedPreset(const std::string& name) {
    lastUsedPreset_ = name;
}

std::string PresetManager::getLastUsedPreset() const {
    return lastUsedPreset_;
}

void PresetManager::enableAutoSave(bool enable, float intervalSeconds) {
    autoSaveEnabled_ = enable;
    autoSaveInterval_ = intervalSeconds;
    std::cout << "Auto-save " << (enable ? "enabled" : "disabled") 
              << " (interval: " << intervalSeconds << "s)" << std::endl;
}

void PresetManager::setAutoSavePresetName(const std::string& name) {
    autoSavePresetName_ = name;
}

float PresetManager::comparePresets(const std::string& preset1, const std::string& preset2) const {
    auto it1 = presets_.find(preset1);
    auto it2 = presets_.find(preset2);
    
    if (it1 == presets_.end() || it2 == presets_.end()) {
        return 0.0f;
    }
    
    const Preset& p1 = it1->second;
    const Preset& p2 = it2->second;
    
    // Simple similarity metric based on engine type and parameters
    float similarity = 0.0f;
    
    // Engine type match
    if (p1.engineType == p2.engineType) {
        similarity += 0.3f;
    }
    
    // Parameter similarity (simplified)
    float paramSimilarity = 0.0f;
    int paramCount = 0;
    
    for (const auto& param1 : p1.globalParameters) {
        auto it = p2.globalParameters.find(param1.first);
        if (it != p2.globalParameters.end()) {
            float diff = std::abs(param1.second - it->second);
            paramSimilarity += (1.0f - diff);
            paramCount++;
        }
    }
    
    if (paramCount > 0) {
        similarity += (paramSimilarity / paramCount) * 0.7f;
    }
    
    return std::clamp(similarity, 0.0f, 1.0f);
}

PresetManager::Preset PresetManager::morphPresets(const std::string& preset1, const std::string& preset2, float amount) const {
    auto it1 = presets_.find(preset1);
    auto it2 = presets_.find(preset2);
    
    if (it1 == presets_.end()) {
        return (it2 != presets_.end()) ? it2->second : Preset();
    }
    if (it2 == presets_.end()) {
        return it1->second;
    }
    
    const Preset& p1 = it1->second;
    const Preset& p2 = it2->second;
    
    Preset morphed = p1; // Start with preset1
    morphed.name = "Morph " + p1.name + " -> " + p2.name;
    morphed.description = "Morphed preset";
    
    // Morph global parameters
    for (auto& param : morphed.globalParameters) {
        auto it = p2.globalParameters.find(param.first);
        if (it != p2.globalParameters.end()) {
            param.second = param.second * (1.0f - amount) + it->second * amount;
        }
    }
    
    // Morph instrument configurations
    for (size_t i = 0; i < morphed.instruments.size(); i++) {
        auto& inst1 = morphed.instruments[i];
        const auto& inst2 = p2.instruments[i];
        
        // Morph parameters
        for (auto& param : inst1.parameters) {
            auto it = inst2.parameters.find(param.first);
            if (it != inst2.parameters.end()) {
                param.second = param.second * (1.0f - amount) + it->second * amount;
            }
        }
        
        // Morph mix levels
        inst1.volume = inst1.volume * (1.0f - amount) + inst2.volume * amount;
        inst1.pan = inst1.pan * (1.0f - amount) + inst2.pan * amount;
    }
    
    return morphed;
}

size_t PresetManager::getPresetCount() const {
    return presets_.size();
}

size_t PresetManager::getCategoryCount() const {
    return categories_.size();
}

std::map<std::string, size_t> PresetManager::getPresetCountByCategory() const {
    std::map<std::string, size_t> counts;
    
    for (const auto& category : categories_) {
        counts[category.first] = category.second.size();
    }
    
    return counts;
}

std::string PresetManager::getErrorMessage() const {
    switch (lastError_) {
        case PresetError::NONE: return "No error";
        case PresetError::FILE_NOT_FOUND: return "Preset file not found";
        case PresetError::INVALID_FORMAT: return "Invalid preset format";
        case PresetError::WRITE_FAILED: return "Failed to write preset";
        case PresetError::READ_FAILED: return "Failed to read preset";
        case PresetError::PRESET_EXISTS: return "Preset already exists";
        case PresetError::INVALID_NAME: return "Invalid preset name";
        case PresetError::CATEGORY_NOT_FOUND: return "Category not found";
        case PresetError::DISK_FULL: return "Disk full";
        case PresetError::PERMISSION_DENIED: return "Permission denied";
        default: return "Unknown error";
    }
}

// Private helper methods
bool PresetManager::initializeDirectories() {
    try {
        std::filesystem::create_directories(factoryDirectory_);
        std::filesystem::create_directories(userDirectory_);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool PresetManager::savePresetToFile(const Preset& preset, const std::string& filePath) const {
    try {
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        std::vector<uint8_t> data = serializePreset(preset);
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        
        return file.good();
    } catch (const std::exception&) {
        return false;
    }
}

bool PresetManager::loadPresetFromFile(const std::string& filePath, Preset& preset) const {
    try {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        // Get file size
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        
        // Read data
        std::vector<uint8_t> data(fileSize);
        file.read(reinterpret_cast<char*>(data.data()), fileSize);
        
        return deserializePreset(data, preset);
    } catch (const std::exception&) {
        return false;
    }
}

std::string PresetManager::sanitizePresetName(const std::string& name) const {
    std::string sanitized = name;
    
    // Replace invalid characters
    for (char& c : sanitized) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || 
            c == '"' || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
    }
    
    return sanitized;
}

std::string PresetManager::generateUniquePresetName(const std::string& baseName) const {
    std::string name = baseName;
    int counter = 1;
    
    while (presets_.find(name) != presets_.end()) {
        name = baseName + " (" + std::to_string(counter) + ")";
        counter++;
    }
    
    return name;
}

bool PresetManager::isValidPresetName(const std::string& name) const {
    if (name.empty() || name.length() > MAX_PRESET_NAME_LENGTH) {
        return false;
    }
    
    // Check for invalid characters
    const std::string invalidChars = "/\\:*?\"<>|";
    return name.find_first_of(invalidChars) == std::string::npos;
}

void PresetManager::createFactoryPreset(const std::string& name, const std::string& description, 
                                      EngineType engineType, const std::string& category) {
    Preset preset;
    preset.name = name;
    preset.description = description;
    preset.category = category;
    preset.author = "ether Factory";
    preset.engineType = engineType;
    
    auto now = std::chrono::system_clock::now();
    preset.createdTime = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    preset.modifiedTime = preset.createdTime;
    
    // Set up default parameters based on engine type
    switch (engineType) {
        case EngineType::SUBTRACTIVE:
            preset.globalParameters[ParameterID::ATTACK] = 0.01f;
            preset.globalParameters[ParameterID::DECAY] = 0.3f;
            preset.globalParameters[ParameterID::SUSTAIN] = 0.7f;
            preset.globalParameters[ParameterID::RELEASE] = 0.5f;
            preset.globalParameters[ParameterID::FILTER_CUTOFF] = 0.6f;
            preset.globalParameters[ParameterID::FILTER_RESONANCE] = 0.2f;
            break;
            
        case EngineType::FM:
            preset.globalParameters[ParameterID::ATTACK] = 0.01f;
            preset.globalParameters[ParameterID::DECAY] = 0.5f;
            preset.globalParameters[ParameterID::SUSTAIN] = 0.3f;
            preset.globalParameters[ParameterID::RELEASE] = 1.0f;
            preset.globalParameters[ParameterID::LFO_RATE] = 0.3f;
            preset.globalParameters[ParameterID::LFO_DEPTH] = 0.5f;
            break;
            
        case EngineType::WAVETABLE:
            preset.globalParameters[ParameterID::ATTACK] = 0.1f;
            preset.globalParameters[ParameterID::DECAY] = 0.4f;
            preset.globalParameters[ParameterID::SUSTAIN] = 0.8f;
            preset.globalParameters[ParameterID::RELEASE] = 0.7f;
            preset.globalParameters[ParameterID::MORPH] = 0.5f;
            break;
            
        case EngineType::GRANULAR:
            preset.globalParameters[ParameterID::ATTACK] = 0.2f;
            preset.globalParameters[ParameterID::DECAY] = 0.6f;
            preset.globalParameters[ParameterID::SUSTAIN] = 0.9f;
            preset.globalParameters[ParameterID::RELEASE] = 1.5f;
            preset.globalParameters[ParameterID::LFO_RATE] = 0.1f; // Grain density
            preset.globalParameters[ParameterID::LFO_DEPTH] = 0.3f; // Grain size
            break;
            
        default:
            break;
    }
    
    // Store preset
    presets_[name] = preset;
    factoryPresetNames_.push_back(name);
    
    // Add to category
    if (!category.empty()) {
        categories_[category].push_back(name);
    }
}

std::vector<uint8_t> PresetManager::serializePreset(const Preset& preset) const {
    std::vector<uint8_t> data;
    
    // Write header
    uint32_t magic = PRESET_FILE_MAGIC;
    uint32_t version = PRESET_FILE_VERSION;
    
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&magic), 
                reinterpret_cast<const uint8_t*>(&magic) + sizeof(magic));
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&version), 
                reinterpret_cast<const uint8_t*>(&version) + sizeof(version));
    
    // Write preset data (simplified serialization)
    // In a real implementation, this would use a proper serialization format
    std::stringstream ss;
    ss << preset.name << "|" << preset.description << "|" << preset.category << "|" 
       << preset.author << "|" << static_cast<int>(preset.engineType) << "|"
       << preset.masterVolume << "|" << preset.bpm;
    
    std::string serialized = ss.str();
    data.insert(data.end(), serialized.begin(), serialized.end());
    
    return data;
}

bool PresetManager::deserializePreset(const std::vector<uint8_t>& data, Preset& preset) const {
    if (data.size() < sizeof(uint32_t) * 2) {
        return false;
    }
    
    // Read header
    uint32_t magic, version;
    std::memcpy(&magic, data.data(), sizeof(magic));
    std::memcpy(&version, data.data() + sizeof(magic), sizeof(version));
    
    if (magic != PRESET_FILE_MAGIC) {
        return false;
    }
    
    // Read preset data (simplified deserialization)
    std::string serialized(data.begin() + sizeof(uint32_t) * 2, data.end());
    std::stringstream ss(serialized);
    
    std::string token;
    int field = 0;
    while (std::getline(ss, token, '|') && field < 7) {
        switch (field) {
            case 0: preset.name = token; break;
            case 1: preset.description = token; break;
            case 2: preset.category = token; break;
            case 3: preset.author = token; break;
            case 4: preset.engineType = static_cast<EngineType>(std::stoi(token)); break;
            case 5: preset.masterVolume = std::stof(token); break;
            case 6: preset.bpm = std::stof(token); break;
        }
        field++;
    }
    
    return field >= 7;
}

// Factory preset constants
namespace FactoryPresets {
    const std::string BASS = "Bass";
    const std::string LEAD = "Lead";
    const std::string PAD = "Pad";
    const std::string PLUCK = "Pluck";
    const std::string FX = "FX";
    const std::string PERCUSSION = "Percussion";
    const std::string EXPERIMENTAL = "Experimental";
    const std::string TEMPLATE = "Template";
    
    const std::string TB303_BASS = "TB-303 Bass";
    const std::string MOOG_LEAD = "Moog Lead";
    const std::string DX7_BELL = "DX7 E.Piano";
    const std::string JUNO_PAD = "Juno Strings";
    const std::string SH101_ACID = "SH-101 Acid";
    
    const std::string FUTURE_BASS = "Future Bass";
    const std::string DUBSTEP_WOBBLE = "Dubstep Wobble";
    const std::string AMBIENT_TEXTURE = "Ambient Texture";
    const std::string GRANULAR_CLOUD = "Granular Cloud";
    const std::string FM_BELL = "FM Bell";
    
    const std::string RED_FIRE = "Red Fire";
    const std::string ORANGE_WARM = "Orange Warm";
    const std::string YELLOW_BRIGHT = "Yellow Bright";
    const std::string GREEN_ORGANIC = "Green Organic";
    const std::string BLUE_DEEP = "Blue Deep";
    const std::string INDIGO_MYSTIC = "Indigo Mystic";
    const std::string VIOLET_ETHEREAL = "Violet Ethereal";
    const std::string GREY_UTILITY = "Grey Utility";
}