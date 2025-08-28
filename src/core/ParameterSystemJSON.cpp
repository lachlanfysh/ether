#include "ParameterSystem.h"
#include <sstream>
#include <iomanip>
#include <ctime>

/**
 * JSON serialization implementation for UnifiedParameterSystem
 * Maintains backward compatibility with existing preset format
 */

namespace {
    // Helper function to escape JSON strings
    std::string escapeJSON(const std::string& str) {
        std::stringstream escaped;
        for (char c : str) {
            switch (c) {
                case '"': escaped << "\\\""; break;
                case '\\': escaped << "\\\\"; break;
                case '\b': escaped << "\\b"; break;
                case '\f': escaped << "\\f"; break;
                case '\n': escaped << "\\n"; break;
                case '\r': escaped << "\\r"; break;
                case '\t': escaped << "\\t"; break;
                default:
                    if (c < 32) {
                        escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                    } else {
                        escaped << c;
                    }
                    break;
            }
        }
        return escaped.str();
    }
    
    // Map ParameterID to legacy JSON parameter names
    std::unordered_map<ParameterID, std::string> parameterToJSONName = {
        {ParameterID::HARMONICS, "harmonics"},
        {ParameterID::TIMBRE, "timbre"},
        {ParameterID::MORPH, "morph"},
        {ParameterID::OSC_MIX, "osc_mix"},
        {ParameterID::DETUNE, "detune"},
        {ParameterID::FILTER_CUTOFF, "filter_cutoff"},
        {ParameterID::FILTER_RESONANCE, "filter_resonance"},
        {ParameterID::FILTER_TYPE, "filter_type"},
        {ParameterID::ATTACK, "env_attack"},
        {ParameterID::DECAY, "env_decay"},
        {ParameterID::SUSTAIN, "amp_sustain"},
        {ParameterID::RELEASE, "env_release"},
        {ParameterID::LFO_RATE, "lfo_rate"},
        {ParameterID::LFO_DEPTH, "lfo_depth"},
        {ParameterID::LFO_SHAPE, "lfo_shape"},
        {ParameterID::REVERB_SIZE, "reverb_size"},
        {ParameterID::REVERB_DAMPING, "reverb_damping"},
        {ParameterID::REVERB_MIX, "reverb_mix"},
        {ParameterID::DELAY_TIME, "delay_time"},
        {ParameterID::DELAY_FEEDBACK, "delay_feedback"},
        {ParameterID::VOLUME, "volume"},
        {ParameterID::PAN, "pan"}
    };
    
    // Reverse mapping for deserialization
    std::unordered_map<std::string, ParameterID> jsonNameToParameter;
    
    void initializeJSONMapping() {
        static bool initialized = false;
        if (initialized) return;
        
        for (const auto& [paramId, jsonName] : parameterToJSONName) {
            jsonNameToParameter[jsonName] = paramId;
        }
        initialized = true;
    }
    
    std::string parameterIDToJSON(ParameterID id) {
        initializeJSONMapping();
        auto it = parameterToJSONName.find(id);
        return (it != parameterToJSONName.end()) ? it->second : "unknown";
    }
    
    ParameterID jsonToParameterID(const std::string& jsonName) {
        initializeJSONMapping();
        auto it = jsonNameToParameter.find(jsonName);
        return (it != jsonNameToParameter.end()) ? it->second : ParameterID::COUNT;
    }
    
    // Get current timestamp
    uint64_t getCurrentTimestamp() {
        return static_cast<uint64_t>(std::time(nullptr));
    }
}

std::string UnifiedParameterSystem::serializeToJSON() const {
    if (!initialized_.load()) {
        updateLastError("Parameter system not initialized");
        return "";
    }
    
    std::stringstream json;
    std::lock_guard<std::mutex> lock(configMutex_);
    
    json << std::fixed << std::setprecision(3);
    json << "{\n";
    
    // Schema version for compatibility
    json << "  \"schema_version\": \"2.0\",\n";
    
    // Preset info section
    json << "  \"preset_info\": {\n";
    json << "    \"name\": \"Generated Preset\",\n";
    json << "    \"description\": \"Generated from UnifiedParameterSystem\",\n";
    json << "    \"author\": \"EtherSynth\",\n";
    json << "    \"engine_type\": 0,\n";
    json << "    \"category\": 0,\n";
    json << "    \"creation_time\": " << getCurrentTimestamp() << ",\n";
    json << "    \"modification_time\": " << getCurrentTimestamp() << ",\n";
    json << "    \"tags\": [\"generated\", \"unified\"]\n";
    json << "  },\n";
    
    // Hold params (main synthesis parameters)
    json << "  \"hold_params\": {\n";
    bool first = true;
    for (const auto& [paramId, config] : parameterConfigs_) {
        if (config.isGlobalParameter) continue; // Skip global params for hold_params
        
        std::string jsonName = parameterIDToJSON(paramId);
        if (jsonName == "unknown") continue;
        
        float value = globalParameters_[static_cast<size_t>(paramId)].load(std::memory_order_relaxed);
        
        if (!first) json << ",\n";
        json << "    \"" << jsonName << "\": " << value;
        first = false;
    }
    json << "\n  },\n";
    
    // Twist params (modulation and envelope parameters)
    json << "  \"twist_params\": {\n";
    first = true;
    std::vector<ParameterID> twistParams = {
        ParameterID::ATTACK, ParameterID::DECAY, ParameterID::RELEASE,
        ParameterID::LFO_RATE, ParameterID::LFO_DEPTH, ParameterID::DETUNE
    };
    
    for (ParameterID paramId : twistParams) {
        if (parameterConfigs_.find(paramId) == parameterConfigs_.end()) continue;
        
        std::string jsonName = parameterIDToJSON(paramId);
        if (jsonName == "unknown") continue;
        
        float value = globalParameters_[static_cast<size_t>(paramId)].load(std::memory_order_relaxed);
        
        if (!first) json << ",\n";
        json << "    \"" << jsonName << "\": " << value;
        first = false;
    }
    json << "\n  },\n";
    
    // Morph params (advanced parameters)
    json << "  \"morph_params\": {\n";
    json << "    \"stereo_spread\": 0.0,\n";
    json << "    \"chorus_depth\": 0.0,\n";
    json << "    \"unison_voices\": 0.0,\n";
    json << "    \"unison_detune\": 0.0,\n";
    json << "    \"analog_drift\": 0.0,\n";
    json << "    \"filter_tracking\": 1.0\n";
    json << "  },\n";
    
    // Macro assignments (placeholder - could be extended)
    json << "  \"macro_assignments\": {\n";
    json << "    \"macro_1\": { \"parameter\": \"filter_cutoff\", \"amount\": 0.8, \"enabled\": true },\n";
    json << "    \"macro_2\": { \"parameter\": \"reverb_size\", \"amount\": 0.6, \"enabled\": true },\n";
    json << "    \"macro_3\": { \"parameter\": \"lfo_depth\", \"amount\": 0.7, \"enabled\": true },\n";
    json << "    \"macro_4\": { \"parameter\": \"env_attack\", \"amount\": 0.5, \"enabled\": true }\n";
    json << "  },\n";
    
    // FX params
    json << "  \"fx_params\": {\n";
    first = true;
    std::vector<ParameterID> fxParams = {
        ParameterID::REVERB_SIZE, ParameterID::REVERB_DAMPING, ParameterID::REVERB_MIX,
        ParameterID::DELAY_TIME, ParameterID::DELAY_FEEDBACK
    };
    
    for (ParameterID paramId : fxParams) {
        if (parameterConfigs_.find(paramId) == parameterConfigs_.end()) continue;
        
        std::string jsonName = parameterIDToJSON(paramId);
        if (jsonName == "unknown") continue;
        
        float value = globalParameters_[static_cast<size_t>(paramId)].load(std::memory_order_relaxed);
        
        if (!first) json << ",\n";
        json << "    \"" << jsonName << "\": " << value;
        first = false;
    }
    // Add some default FX params
    if (!first) json << ",\n";
    json << "    \"chorus_rate\": 0.3,\n";
    json << "    \"chorus_feedback\": 0.2,\n";
    json << "    \"tape_saturation\": 0.1\n";
    json << "  },\n";
    
    // Velocity configuration
    json << "  \"velocity_config\": {\n";
    json << "    \"enable_velocity_to_volume\": true,\n";
    json << "    \"velocity_mappings\": {\n";
    first = true;
    
    // Include parameters that have velocity scaling enabled
    for (const auto& [paramId, config] : parameterConfigs_) {
        if (!config.enableVelocityScaling || config.velocityScale <= 0.0f) continue;
        
        std::string jsonName = parameterIDToJSON(paramId);
        if (jsonName == "unknown") continue;
        
        if (!first) json << ",\n";
        json << "      \"" << jsonName << "\": " << config.velocityScale;
        first = false;
    }
    
    json << "\n    }\n";
    json << "  },\n";
    
    // Performance settings
    json << "  \"performance\": {\n";
    json << "    \"morph_transition_time\": 200,\n";
    json << "    \"enable_parameter_smoothing\": true,\n";
    json << "    \"parameter_smoothing_time\": 50\n";
    json << "  },\n";
    
    // Extended unified parameter system info
    json << "  \"unified_system_info\": {\n";
    json << "    \"parameter_count\": " << parameterConfigs_.size() << ",\n";
    json << "    \"active_smoothers\": " << getActiveSmootherCount() << ",\n";
    json << "    \"system_version\": \"1.0\"\n";
    json << "  }\n";
    
    json << "}";
    
    return json.str();
}

bool UnifiedParameterSystem::deserializeFromJSON(const std::string& jsonStr) {
    if (!initialized_.load()) {
        updateLastError("Parameter system not initialized");
        return false;
    }
    
    // For now, implement a simple parser for the key sections
    // In a production system, you'd want to use a proper JSON library like nlohmann/json
    
    try {
        std::lock_guard<std::mutex> lock(configMutex_);
        
        // Simple JSON parsing - look for parameter sections
        size_t pos = 0;
        
        // Parse hold_params
        pos = jsonStr.find("\"hold_params\"");
        if (pos != std::string::npos) {
            size_t startBrace = jsonStr.find('{', pos);
            size_t endBrace = jsonStr.find('}', startBrace);
            if (startBrace != std::string::npos && endBrace != std::string::npos) {
                parseParameterSection(jsonStr.substr(startBrace + 1, endBrace - startBrace - 1));
            }
        }
        
        // Parse twist_params
        pos = jsonStr.find("\"twist_params\"");
        if (pos != std::string::npos) {
            size_t startBrace = jsonStr.find('{', pos);
            size_t endBrace = jsonStr.find('}', startBrace);
            if (startBrace != std::string::npos && endBrace != std::string::npos) {
                parseParameterSection(jsonStr.substr(startBrace + 1, endBrace - startBrace - 1));
            }
        }
        
        // Parse fx_params
        pos = jsonStr.find("\"fx_params\"");
        if (pos != std::string::npos) {
            size_t startBrace = jsonStr.find('{', pos);
            size_t endBrace = jsonStr.find('}', startBrace);
            if (startBrace != std::string::npos && endBrace != std::string::npos) {
                parseParameterSection(jsonStr.substr(startBrace + 1, endBrace - startBrace - 1));
            }
        }
        
        // Parse velocity_config
        parseVelocityConfig(jsonStr);
        
        return true;
        
    } catch (const std::exception& e) {
        updateLastError("JSON parsing error: " + std::string(e.what()));
        return false;
    }
}

void UnifiedParameterSystem::parseParameterSection(const std::string& sectionContent) {
    // Simple parameter parsing
    std::istringstream stream(sectionContent);
    std::string line;
    
    while (std::getline(stream, line)) {
        // Remove whitespace
        line.erase(0, line.find_first_not_of(" \t\n\r"));
        line.erase(line.find_last_not_of(" \t\n\r") + 1);
        
        if (line.empty() || line[0] == ',' || line[0] == '}') continue;
        
        // Look for "parameter_name": value pattern
        size_t colonPos = line.find(':');
        size_t firstQuote = line.find('"');
        size_t secondQuote = line.find('"', firstQuote + 1);
        
        if (colonPos != std::string::npos && firstQuote != std::string::npos && secondQuote != std::string::npos) {
            std::string paramName = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
            
            // Extract value after colon
            std::string valueStr = line.substr(colonPos + 1);
            valueStr.erase(0, valueStr.find_first_not_of(" \t"));
            
            // Remove trailing comma if present
            if (!valueStr.empty() && valueStr.back() == ',') {
                valueStr.pop_back();
            }
            
            try {
                float value = std::stof(valueStr);
                ParameterID paramId = jsonToParameterID(paramName);
                
                if (paramId != ParameterID::COUNT) {
                    // Set parameter value directly in atomic storage
                    globalParameters_[static_cast<size_t>(paramId)].store(value, std::memory_order_relaxed);
                    
                    // Update parameter value record
                    if (parameterValues_.find(paramId) != parameterValues_.end()) {
                        parameterValues_[paramId].value = value;
                        parameterValues_[paramId].rawValue = value;
                        parameterValues_[paramId].targetValue = value;
                        parameterValues_[paramId].hasBeenSet = true;
                        parameterValues_[paramId].lastUpdateTime = std::chrono::steady_clock::now().time_since_epoch().count();
                    }
                    
                    // Update smoother target if smoothing is enabled
                    if (globalSmoothers_.find(paramId) != globalSmoothers_.end()) {
                        globalSmoothers_[paramId]->setValue(value);
                    }
                }
            } catch (const std::exception&) {
                // Skip invalid values
                continue;
            }
        }
    }
}

void UnifiedParameterSystem::parseVelocityConfig(const std::string& jsonStr) {
    // Parse velocity_mappings section
    size_t velocityPos = jsonStr.find("\"velocity_mappings\"");
    if (velocityPos == std::string::npos) return;
    
    size_t startBrace = jsonStr.find('{', velocityPos);
    size_t endBrace = jsonStr.find('}', startBrace);
    if (startBrace == std::string::npos || endBrace == std::string::npos) return;
    
    std::string mappingsContent = jsonStr.substr(startBrace + 1, endBrace - startBrace - 1);
    
    // Parse velocity mappings
    std::istringstream stream(mappingsContent);
    std::string line;
    
    while (std::getline(stream, line)) {
        // Remove whitespace
        line.erase(0, line.find_first_not_of(" \t\n\r"));
        line.erase(line.find_last_not_of(" \t\n\r") + 1);
        
        if (line.empty() || line[0] == ',' || line[0] == '}') continue;
        
        size_t colonPos = line.find(':');
        size_t firstQuote = line.find('"');
        size_t secondQuote = line.find('"', firstQuote + 1);
        
        if (colonPos != std::string::npos && firstQuote != std::string::npos && secondQuote != std::string::npos) {
            std::string paramName = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
            
            std::string valueStr = line.substr(colonPos + 1);
            valueStr.erase(0, valueStr.find_first_not_of(" \t"));
            if (!valueStr.empty() && valueStr.back() == ',') {
                valueStr.pop_back();
            }
            
            try {
                float velocityScale = std::stof(valueStr);
                ParameterID paramId = jsonToParameterID(paramName);
                
                if (paramId != ParameterID::COUNT && parameterConfigs_.find(paramId) != parameterConfigs_.end()) {
                    // Update velocity scaling configuration
                    auto& config = parameterConfigs_[paramId];
                    config.velocityScale = std::abs(velocityScale); // Use absolute value
                    config.enableVelocityScaling = (config.velocityScale > 0.0f);
                    
                    // If velocity scaling integration is available, update it too
                    if (velocityScaling_) {
                        VelocityParameterScaling::ParameterScalingConfig velConfig;
                        velConfig.category = config.velocityCategory;
                        velConfig.velocityScale = config.velocityScale;
                        velocityScaling_->setParameterScaling(static_cast<uint32_t>(paramId), velConfig);
                    }
                }
            } catch (const std::exception&) {
                continue;
            }
        }
    }
}

size_t UnifiedParameterSystem::getActiveSmootherCount() const {
    size_t count = 0;
    
    // Count global smoothers that are active
    for (const auto& [paramId, smoother] : globalSmoothers_) {
        if (smoother && smoother->isSmoothing()) {
            count++;
        }
    }
    
    // Count instrument smoothers that are active
    for (const auto& instrumentSmoothers : instrumentSmoothers_) {
        for (const auto& [paramId, smoother] : instrumentSmoothers) {
            if (smoother && smoother->isSmoothing()) {
                count++;
            }
        }
    }
    
    return count;
}