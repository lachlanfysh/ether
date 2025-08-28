#include "InstrumentSlot.h"
#include "../synthesis/SynthEngine.h"
#include "../processing/effects/EffectsChain.h"
#include "../sequencer/EuclideanRhythm.h"
#include <iostream>

const std::array<std::string, MAX_INSTRUMENTS> InstrumentSlot::DEFAULT_NAMES = {
    "Red Bass",
    "Orange Lead", 
    "Yellow Pad",
    "Green Arp",
    "Blue Strings",
    "Indigo FX",
    "Violet Perc",
    "Grey Util"
};

InstrumentSlot::InstrumentSlot(InstrumentColor color) 
    : color_(color), name_(DEFAULT_NAMES[static_cast<size_t>(color)]) {
    
    // Create components
    effects_ = std::make_unique<EffectsChain>();
    pattern_ = std::make_unique<EuclideanRhythm>();
    
    std::cout << "InstrumentSlot created: " << name_ << std::endl;
}

InstrumentSlot::~InstrumentSlot() {
    std::cout << "InstrumentSlot destroyed: " << name_ << std::endl;
}

void InstrumentSlot::addEngine(EngineType type) {
    try {
        EngineLayer layer;
        layer.engine = createEngine(type);
        if (layer.engine) {
            engines_.push_back(std::move(layer));
            
            // Initialize parameters for this engine
            initializeEngineParameters(engines_.size() - 1);
            
            std::cout << name_ << ": Added engine " << layer.engine->getName() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to add engine to " << name_ << ": " << e.what() << std::endl;
    }
}

void InstrumentSlot::removeEngine(size_t index) {
    if (index < engines_.size()) {
        std::cout << name_ << ": Removed engine " << engines_[index].engine->getName() << std::endl;
        engines_.erase(engines_.begin() + index);
        
        // Remove corresponding parameter array
        if (index < engineParameters_.size()) {
            engineParameters_.erase(engineParameters_.begin() + index);
        }
    }
}

void InstrumentSlot::setEngineBalance(size_t index, float balance) {
    if (index < engines_.size()) {
        engines_[index].balance = std::clamp(balance, 0.0f, 1.0f);
    }
}

SynthEngine* InstrumentSlot::getEngine(size_t index) const {
    if (index < engines_.size()) {
        return engines_[index].engine.get();
    }
    return nullptr;
}

SynthEngine* InstrumentSlot::getPrimaryEngine() const {
    return getEngine(0);
}

void InstrumentSlot::setParameter(ParameterID param, float value) {
    // Apply to all engines
    for (size_t i = 0; i < engines_.size(); i++) {
        setEngineParameter(i, param, value);
    }
}

float InstrumentSlot::getParameter(ParameterID param) const {
    if (!engines_.empty()) {
        return getEngineParameter(0, param);
    }
    return 0.0f;
}

void InstrumentSlot::setEngineParameter(size_t engineIndex, ParameterID param, float value) {
    if (engineIndex < engines_.size() && engineIndex < engineParameters_.size()) {
        size_t paramIndex = static_cast<size_t>(param);
        if (paramIndex < engineParameters_[engineIndex].size()) {
            engineParameters_[engineIndex][paramIndex] = value;
            
            // Apply to engine if it supports this parameter
            if (engines_[engineIndex].engine->hasParameter(param)) {
                engines_[engineIndex].engine->setParameter(param, value);
            }
        }
    }
}

float InstrumentSlot::getEngineParameter(size_t engineIndex, ParameterID param) const {
    if (engineIndex < engineParameters_.size()) {
        size_t paramIndex = static_cast<size_t>(param);
        if (paramIndex < engineParameters_[engineIndex].size()) {
            return engineParameters_[engineIndex][paramIndex];
        }
    }
    return 0.0f;
}

void InstrumentSlot::noteOn(uint8_t note, float velocity, float aftertouch) {
    if (muted_) return;
    
    for (auto& layer : engines_) {
        if (layer.enabled && layer.engine) {
            layer.engine->noteOn(note, velocity, aftertouch);
        }
    }
    
    std::cout << name_ << ": Note ON " << static_cast<int>(note) 
              << " vel=" << velocity << std::endl;
}

void InstrumentSlot::noteOff(uint8_t note) {
    for (auto& layer : engines_) {
        if (layer.engine) {
            layer.engine->noteOff(note);
        }
    }
    
    std::cout << name_ << ": Note OFF " << static_cast<int>(note) << std::endl;
}

void InstrumentSlot::setAftertouch(uint8_t note, float aftertouch) {
    for (auto& layer : engines_) {
        if (layer.engine) {
            layer.engine->setAftertouch(note, aftertouch);
        }
    }
}

void InstrumentSlot::allNotesOff() {
    for (auto& layer : engines_) {
        if (layer.engine) {
            layer.engine->allNotesOff();
        }
    }
}

void InstrumentSlot::processAudio(EtherAudioBuffer& outputBuffer) {
    if (muted_) {
        for (auto& frame : outputBuffer) {
            frame.left = 0.0f;
            frame.right = 0.0f;
        }
        return;
    }
    
    processEngines(outputBuffer);
    applyEffects(outputBuffer);
    applyMixControls(outputBuffer);
}

size_t InstrumentSlot::getActiveVoiceCount() const {
    size_t totalVoices = 0;
    for (const auto& layer : engines_) {
        if (layer.engine) {
            totalVoices += layer.engine->getActiveVoiceCount();
        }
    }
    return totalVoices;
}

float InstrumentSlot::getCPUUsage() const {
    float totalCPU = 0.0f;
    for (const auto& layer : engines_) {
        if (layer.engine) {
            totalCPU += layer.engine->getCPUUsage();
        }
    }
    return totalCPU;
}

void InstrumentSlot::playChordNotes(const std::vector<uint8_t>& notes, float velocity) {
    for (uint8_t note : notes) {
        noteOn(note, velocity);
    }
}

void InstrumentSlot::processEngines(EtherAudioBuffer& buffer) {
    // Clear buffer first
    for (auto& frame : buffer) {
        frame.left = 0.0f;
        frame.right = 0.0f;
    }
    
    // Process and mix all engine layers
    EtherAudioBuffer layerBuffer;
    for (auto& layer : engines_) {
        if (layer.enabled && layer.engine) {
            // Clear layer buffer
            for (auto& frame : layerBuffer) {
                frame.left = 0.0f;
                frame.right = 0.0f;
            }
            
            // Process this layer
            layer.engine->processAudio(layerBuffer);
            
            // Mix into main buffer with layer balance
            for (size_t i = 0; i < BUFFER_SIZE; i++) {
                buffer[i] += layerBuffer[i] * layer.balance;
            }
        }
    }
}

void InstrumentSlot::applyEffects(EtherAudioBuffer& buffer) {
    if (effects_) {
        effects_->process(buffer);
    }
}

void InstrumentSlot::applyMixControls(EtherAudioBuffer& buffer) {
    // Apply volume and pan
    for (auto& frame : buffer) {
        frame = frame * volume_;
        
        // Apply pan (simple equal-power panning)
        if (pan_ < 0.0f) {
            // Pan left
            frame.right *= (1.0f + pan_);
        } else if (pan_ > 0.0f) {
            // Pan right  
            frame.left *= (1.0f - pan_);
        }
    }
}

void InstrumentSlot::initializeEngineParameters(size_t engineIndex) {
    // Add parameter array for this engine
    std::array<float, static_cast<size_t>(ParameterID::COUNT)> params;
    
    // Initialize with default values
    for (size_t i = 0; i < params.size(); i++) {
        ParameterID param = static_cast<ParameterID>(i);
        params[i] = 0.5f; // Default to middle value for now
    }
    
    engineParameters_.push_back(params);
}

std::unique_ptr<SynthEngine> InstrumentSlot::createEngine(EngineType type) {
    return ::createSynthEngine(type);
}