#include "LFOParameterMapping.h"
#include <algorithm>
#include <cstring>

namespace EtherSynth {

LFOParameterMapping::LFOParameterMapping() {
    // Initialize all instruments with empty LFO states
    for (auto& instrument : instruments_) {
        for (auto& lfo : instrument.lfos) {
            lfo = std::make_unique<AdvancedLFO>();
        }
    }
}

void LFOParameterMapping::initialize(float sampleRate, float tempo) {
    sampleRate_ = sampleRate;
    tempo_ = tempo;
    
    // Initialize all LFOs with default settings
    for (int inst = 0; inst < MAX_INSTRUMENTS; inst++) {
        for (int lfo = 0; lfo < MAX_INSTRUMENT_LFOS; lfo++) {
            if (instruments_[inst].lfos[lfo]) {
                instruments_[inst].lfos[lfo]->setSampleRate(sampleRate);
                instruments_[inst].lfos[lfo]->setTempo(tempo);
                
                // Set up default LFO settings for each slot
                AdvancedLFO::LFOSettings settings;
                switch (lfo) {
                case 0: // LFO1 - Filter modulation
                    settings.waveform = AdvancedLFO::Waveform::SINE;
                    settings.rate = 0.5f;
                    settings.depth = 0.3f;
                    break;
                case 1: // LFO2 - Pitch/Vibrato
                    settings.waveform = AdvancedLFO::Waveform::TRIANGLE;
                    settings.rate = 4.0f;
                    settings.depth = 0.15f;
                    break;
                case 2: // LFO3 - Amplitude/Tremolo
                    settings.waveform = AdvancedLFO::Waveform::SINE;
                    settings.rate = 2.0f;
                    settings.depth = 0.2f;
                    break;
                case 3: // LFO4 - Creative/Experimental
                    settings.waveform = AdvancedLFO::Waveform::SAMPLE_HOLD;
                    settings.rate = 1.0f;
                    settings.depth = 0.4f;
                    break;
                }
                
                instruments_[inst].lfos[lfo]->setSettings(settings);
            }
        }
        
        // Apply basic template by default
        createBasicTemplate(inst);
    }
}

void LFOParameterMapping::setInstrumentEngine(int instrumentIndex, EngineType engineType) {
    if (!isValidInstrument(instrumentIndex)) return;
    
    instruments_[instrumentIndex].engineType = engineType;
    applyEngineTemplate(instrumentIndex, engineType);
}

void LFOParameterMapping::assignLFOToParameter(int instrumentIndex, int lfoIndex, int keyIndex, float depth) {
    if (!isValidInstrument(instrumentIndex) || !isValidLFO(lfoIndex) || !isValidKey(keyIndex)) return;
    
    auto& instrument = instruments_[instrumentIndex];
    
    // Remove existing assignment for this LFO+parameter combination
    instrument.assignments.erase(
        std::remove_if(instrument.assignments.begin(), instrument.assignments.end(),
            [lfoIndex, keyIndex](const LFOAssignment& assign) {
                return assign.lfoIndex == lfoIndex && assign.keyIndex == keyIndex;
            }),
        instrument.assignments.end());
    
    // Create new assignment
    LFOAssignment assignment;
    assignment.enabled = true;
    assignment.lfoIndex = lfoIndex;
    assignment.keyIndex = keyIndex;
    assignment.parameterID = EngineParameterMappings::getParameterAt(instrument.engineType, keyIndex);
    assignment.depth = depth;
    assignment.bipolar = true;
    assignment.processing = AdvancedModulationMatrix::ModProcessing::DIRECT;
    
    instrument.assignments.push_back(assignment);
}

void LFOParameterMapping::removeLFOAssignment(int instrumentIndex, int lfoIndex, int keyIndex) {
    if (!isValidInstrument(instrumentIndex)) return;
    
    auto& assignments = instruments_[instrumentIndex].assignments;
    assignments.erase(
        std::remove_if(assignments.begin(), assignments.end(),
            [lfoIndex, keyIndex](const LFOAssignment& assign) {
                return assign.lfoIndex == lfoIndex && assign.keyIndex == keyIndex;
            }),
        assignments.end());
}

LFOParameterMapping::LFOAssignment* LFOParameterMapping::getAssignment(int instrumentIndex, int lfoIndex, int keyIndex) {
    if (!isValidInstrument(instrumentIndex)) return nullptr;
    
    auto& assignments = instruments_[instrumentIndex].assignments;
    auto it = std::find_if(assignments.begin(), assignments.end(),
        [lfoIndex, keyIndex](const LFOAssignment& assign) {
            return assign.lfoIndex == lfoIndex && assign.keyIndex == keyIndex && assign.enabled;
        });
    
    return (it != assignments.end()) ? &(*it) : nullptr;
}

std::vector<LFOParameterMapping::LFOAssignment> LFOParameterMapping::getParameterAssignments(int instrumentIndex, int keyIndex) {
    std::vector<LFOAssignment> result;
    if (!isValidInstrument(instrumentIndex)) return result;
    
    const auto& assignments = instruments_[instrumentIndex].assignments;
    for (const auto& assign : assignments) {
        if (assign.keyIndex == keyIndex && assign.enabled) {
            result.push_back(assign);
        }
    }
    
    return result;
}

AdvancedLFO* LFOParameterMapping::getLFO(int instrumentIndex, int lfoIndex) {
    if (!isValidInstrument(instrumentIndex) || !isValidLFO(lfoIndex)) return nullptr;
    return instruments_[instrumentIndex].lfos[lfoIndex].get();
}

void LFOParameterMapping::setLFOWaveform(int instrumentIndex, int lfoIndex, AdvancedLFO::Waveform waveform) {
    auto* lfo = getLFO(instrumentIndex, lfoIndex);
    if (lfo) {
        lfo->setWaveform(waveform);
    }
}

void LFOParameterMapping::processFrame() {
    // Process all LFOs and update parameter modulations
    for (int inst = 0; inst < MAX_INSTRUMENTS; inst++) {
        auto& instrument = instruments_[inst];
        
        // Process all LFOs for this instrument
        for (int lfo = 0; lfo < MAX_INSTRUMENT_LFOS; lfo++) {
            if (instrument.lfos[lfo]) {
                instrument.lfos[lfo]->process();
            }
        }
    }
}

float LFOParameterMapping::getModulatedParameterValue(int instrumentIndex, int keyIndex, float baseValue) {
    if (!isValidInstrument(instrumentIndex) || !isValidKey(keyIndex)) return baseValue;
    
    const auto& instrument = instruments_[instrumentIndex];
    float modulatedValue = baseValue;
    
    // Apply all LFO assignments for this parameter
    for (const auto& assign : instrument.assignments) {
        if (assign.keyIndex == keyIndex && assign.enabled) {
            auto* lfo = instrument.lfos[assign.lfoIndex].get();
            if (lfo && lfo->isActive()) {
                float lfoValue = lfo->getCurrentValue();
                float modAmount = lfoValue * assign.depth * instrument.masterDepth;
                
                // Apply processing
                switch (assign.processing) {
                case AdvancedModulationMatrix::ModProcessing::DIRECT:
                    modulatedValue += modAmount;
                    break;
                case AdvancedModulationMatrix::ModProcessing::INVERTED:
                    modulatedValue += -modAmount;
                    break;
                case AdvancedModulationMatrix::ModProcessing::RECTIFIED:
                    modulatedValue += std::abs(modAmount);
                    break;
                default:
                    modulatedValue += modAmount;
                    break;
                }
            }
        }
    }
    
    return std::clamp(modulatedValue, 0.0f, 1.0f);
}

LFOParameterMapping::LFODisplayInfo LFOParameterMapping::getParameterLFOInfo(int instrumentIndex, int keyIndex) {
    LFODisplayInfo info;
    if (!isValidInstrument(instrumentIndex) || !isValidKey(keyIndex)) return info;
    
    const auto assignments = getParameterAssignments(instrumentIndex, keyIndex);
    if (assignments.empty()) return info;
    
    info.hasLFO = true;
    info.activeLFOs = 0;
    float totalValue = 0.0f;
    
    for (const auto& assign : assignments) {
        info.activeLFOs |= (1 << assign.lfoIndex);
        
        auto* lfo = instruments_[instrumentIndex].lfos[assign.lfoIndex].get();
        if (lfo) {
            totalValue += lfo->getCurrentValue() * assign.depth;
            info.waveform = lfo->getCurrentWaveform();
            info.rate = lfo->getSettings().rate;
            info.synced = (lfo->getSettings().syncMode != AdvancedLFO::SyncMode::FREE_RUNNING);
        }
    }
    
    info.currentValue = totalValue / assignments.size(); // Average value
    return info;
}

void LFOParameterMapping::applyEngineTemplate(int instrumentIndex, EngineType engineType) {
    if (!isValidInstrument(instrumentIndex)) return;
    
    // Clear existing assignments
    instruments_[instrumentIndex].assignments.clear();
    
    // Apply engine-specific template
    switch (engineType) {
    case EngineType::MACRO_VA:
        createMacroVATemplate(instrumentIndex);
        break;
    case EngineType::MACRO_FM:
        createMacroFMTemplate(instrumentIndex);
        break;
    case EngineType::MACRO_WAVETABLE:
        createMacroWTTemplate(instrumentIndex);
        break;
    case EngineType::DRUM_KIT:
        createDrumKitTemplate(instrumentIndex);
        break;
    case EngineType::SAMPLER_KIT:
    case EngineType::SAMPLER_SLICER:
        createSamplerTemplate(instrumentIndex);
        break;
    default:
        createBasicTemplate(instrumentIndex);
        break;
    }
}

void LFOParameterMapping::createBasicTemplate(int instrumentIndex) {
    // Basic template suitable for most engines
    // LFO1 → Filter Cutoff (Key 5)
    assignLFOToParameter(instrumentIndex, 0, 4, 0.3f);
    
    // LFO2 → Pitch/Timbre (Key 2) 
    assignLFOToParameter(instrumentIndex, 1, 1, 0.15f);
    
    // LFO3 → Volume (Key 15)
    assignLFOToParameter(instrumentIndex, 2, 14, 0.2f);
}

void LFOParameterMapping::createMacroVATemplate(int instrumentIndex) {
    // Analog-style modulation routing
    // LFO1 → Filter Cutoff (slow filter sweep)
    assignLFOToParameter(instrumentIndex, 0, 4, 0.4f);  // CUTOFF
    
    // LFO2 → OSC Mix (movement between oscillators)
    assignLFOToParameter(instrumentIndex, 1, 0, 0.25f); // OSC_MIX
    
    // LFO3 → Filter Resonance (filter emphasis)
    assignLFOToParameter(instrumentIndex, 2, 5, 0.15f); // RESONANCE
    
    // LFO4 → Detune (pitch drift)
    assignLFOToParameter(instrumentIndex, 3, 2, 0.1f);  // DETUNE
}

void LFOParameterMapping::createMacroFMTemplate(int instrumentIndex) {
    // FM synthesis modulation routing
    // LFO1 → Index (classic FM wobble)
    assignLFOToParameter(instrumentIndex, 0, 2, 0.5f);  // INDEX
    
    // LFO2 → Ratio (harmonic content change)
    assignLFOToParameter(instrumentIndex, 1, 1, 0.2f);  // RATIO
    
    // LFO3 → Filter Cutoff
    assignLFOToParameter(instrumentIndex, 2, 4, 0.3f);  // CUTOFF
    
    // LFO4 → Feedback (distortion character)
    assignLFOToParameter(instrumentIndex, 3, 3, 0.3f);  // FEEDBACK
}

void LFOParameterMapping::createMacroWTTemplate(int instrumentIndex) {
    // Wavetable synthesis modulation
    // LFO1 → Wave position
    assignLFOToParameter(instrumentIndex, 0, 0, 0.6f);  // WAVE
    
    // LFO2 → Unison spread
    assignLFOToParameter(instrumentIndex, 1, 2, 0.3f);  // SPREAD
    
    // LFO3 → Filter Cutoff
    assignLFOToParameter(instrumentIndex, 2, 4, 0.35f); // CUTOFF
    
    // LFO4 → Sync amount
    assignLFOToParameter(instrumentIndex, 3, 3, 0.25f); // SYNC
}

void LFOParameterMapping::createDrumKitTemplate(int instrumentIndex) {
    // Drum-oriented modulation
    // LFO1 → Accent (dynamics)
    assignLFOToParameter(instrumentIndex, 0, 0, 0.4f);  // ACCENT
    
    // LFO2 → Filter Cutoff
    assignLFOToParameter(instrumentIndex, 1, 4, 0.3f);  // CUTOFF
    
    // LFO3 → Variation (sample variation)
    assignLFOToParameter(instrumentIndex, 2, 3, 0.5f);  // VARIATION
    
    // LFO4 → Humanize
    assignLFOToParameter(instrumentIndex, 3, 1, 0.2f);  // HUMANIZE
}

void LFOParameterMapping::createSamplerTemplate(int instrumentIndex) {
    // Sample-based instrument modulation
    // LFO1 → Filter Cutoff
    assignLFOToParameter(instrumentIndex, 0, 4, 0.4f);  // CUTOFF
    
    // LFO2 → Pitch
    assignLFOToParameter(instrumentIndex, 1, 2, 0.1f);  // PITCH
    
    // LFO3 → Start position
    assignLFOToParameter(instrumentIndex, 2, 0, 0.3f);  // START
    
    // LFO4 → Loop point
    assignLFOToParameter(instrumentIndex, 3, 1, 0.2f);  // LOOP
}

void LFOParameterMapping::triggerLFOs(int instrumentIndex) {
    if (!isValidInstrument(instrumentIndex)) return;
    
    for (auto& lfo : instruments_[instrumentIndex].lfos) {
        if (lfo) {
            lfo->trigger();
        }
    }
}

void LFOParameterMapping::setGlobalTempo(float bpm) {
    tempo_ = bpm;
    
    // Update all LFOs
    for (auto& instrument : instruments_) {
        for (auto& lfo : instrument.lfos) {
            if (lfo) {
                lfo->setTempo(bpm);
            }
        }
    }
}

bool LFOParameterMapping::isValidInstrument(int index) const {
    return index >= 0 && index < MAX_INSTRUMENTS;
}

bool LFOParameterMapping::isValidLFO(int index) const {
    return index >= 0 && index < MAX_INSTRUMENT_LFOS;
}

bool LFOParameterMapping::isValidKey(int index) const {
    return index >= 0 && index < MAX_PARAMETERS;
}

// LFOUIManager implementation
LFOUIManager::LFOUIManager(LFOParameterMapping* mapping) : mapping_(mapping) {}

void LFOUIManager::updateVisualState() {
    if (!mapping_) return;
    
    for (int inst = 0; inst < LFOParameterMapping::MAX_INSTRUMENTS; inst++) {
        for (int lfo = 0; lfo < LFOParameterMapping::MAX_INSTRUMENT_LFOS; lfo++) {
            auto* lfoPtr = mapping_->getLFO(inst, lfo);
            if (lfoPtr) {
                auto& visual = visualState_[inst][lfo];
                visual.phase = lfoPtr->getCurrentPhase();
                visual.value = lfoPtr->getCurrentValue();
                visual.active = lfoPtr->isActive();
                
                // Color coding for different LFOs
                const uint32_t lfoColors[] = {
                    0xFF6B6B, // LFO1 - Red
                    0x4ECDC4, // LFO2 - Teal
                    0x45B7D1, // LFO3 - Blue
                    0xF39C12  // LFO4 - Orange
                };
                visual.color = lfoColors[lfo];
                visual.intensity = std::abs(visual.value);
            }
        }
    }
}

const char* LFOUIManager::getWaveformName(AdvancedLFO::Waveform waveform) {
    static const char* names[] = {
        "SINE", "TRI", "SAW↗", "SAW↘", "SQR", "PLS", 
        "NOISE", "S&H", "EXP↗", "EXP↘", "LOG", "CUSTOM"
    };
    
    int index = static_cast<int>(waveform);
    if (index >= 0 && index < static_cast<int>(AdvancedLFO::Waveform::COUNT)) {
        return names[index];
    }
    return "UNK";
}

uint32_t LFOUIManager::getParameterLFOColor(int instrumentIndex, int keyIndex) {
    if (!mapping_) return 0x808080;
    
    auto info = mapping_->getParameterLFOInfo(instrumentIndex, keyIndex);
    if (!info.hasLFO) return 0x808080;
    
    // Return color based on most significant active LFO
    const uint32_t lfoColors[] = { 0xFF6B6B, 0x4ECDC4, 0x45B7D1, 0xF39C12 };
    
    for (int i = 0; i < 4; i++) {
        if (info.activeLFOs & (1 << i)) {
            return lfoColors[i];
        }
    }
    
    return 0x808080;
}

} // namespace EtherSynth