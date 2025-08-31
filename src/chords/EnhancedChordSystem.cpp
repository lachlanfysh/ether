#include "EnhancedChordSystem.h"
#include "../synthesis/EngineFactory.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace EtherSynth {

// Initialize chord database
const std::map<EnhancedChordSystem::ChordType, EnhancedChordSystem::ChordIntervals> 
EnhancedChordSystem::chordDatabase_ = {
    // Major family
    {ChordType::MAJOR,        {{0, 4, 7}, "maj", "Major Triad"}},
    {ChordType::MAJOR_6,      {{0, 4, 7, 9}, "6", "Major 6th"}},
    {ChordType::MAJOR_7,      {{0, 4, 7, 11}, "maj7", "Major 7th"}},
    {ChordType::MAJOR_9,      {{0, 4, 7, 11, 14}, "maj9", "Major 9th"}},
    {ChordType::MAJOR_ADD9,   {{0, 4, 7, 14}, "add9", "Major Add 9"}},
    {ChordType::MAJOR_11,     {{0, 4, 7, 11, 14, 17}, "maj11", "Major 11th"}},
    {ChordType::MAJOR_13,     {{0, 4, 7, 11, 14, 21}, "maj13", "Major 13th"}},
    {ChordType::MAJOR_6_9,    {{0, 4, 7, 9, 14}, "6/9", "Major 6/9"}},
    
    // Minor family
    {ChordType::MINOR,        {{0, 3, 7}, "m", "Minor Triad"}},
    {ChordType::MINOR_6,      {{0, 3, 7, 9}, "m6", "Minor 6th"}},
    {ChordType::MINOR_7,      {{0, 3, 7, 10}, "m7", "Minor 7th"}},
    {ChordType::MINOR_9,      {{0, 3, 7, 10, 14}, "m9", "Minor 9th"}},
    {ChordType::MINOR_ADD9,   {{0, 3, 7, 14}, "m(add9)", "Minor Add 9"}},
    {ChordType::MINOR_11,     {{0, 3, 7, 10, 14, 17}, "m11", "Minor 11th"}},
    {ChordType::MINOR_13,     {{0, 3, 7, 10, 14, 21}, "m13", "Minor 13th"}},
    {ChordType::MINOR_MAJ7,   {{0, 3, 7, 11}, "mMaj7", "Minor Major 7th"}},
    
    // Dominant family
    {ChordType::DOMINANT_7,       {{0, 4, 7, 10}, "7", "Dominant 7th"}},
    {ChordType::DOMINANT_9,       {{0, 4, 7, 10, 14}, "9", "Dominant 9th"}},
    {ChordType::DOMINANT_11,      {{0, 4, 7, 10, 14, 17}, "11", "Dominant 11th"}},
    {ChordType::DOMINANT_13,      {{0, 4, 7, 10, 14, 21}, "13", "Dominant 13th"}},
    {ChordType::DOMINANT_7_SHARP_5, {{0, 4, 8, 10}, "7#5", "Dominant 7#5"}},
    {ChordType::DOMINANT_7_FLAT_5,  {{0, 4, 6, 10}, "7b5", "Dominant 7b5"}},
    
    // Diminished family
    {ChordType::DIMINISHED,       {{0, 3, 6}, "dim", "Diminished"}},
    {ChordType::DIMINISHED_7,     {{0, 3, 6, 9}, "dim7", "Diminished 7th"}},
    {ChordType::HALF_DIMINISHED_7, {{0, 3, 6, 10}, "m7b5", "Half Diminished 7th"}},
    
    // Augmented family
    {ChordType::AUGMENTED,        {{0, 4, 8}, "aug", "Augmented"}},
    {ChordType::AUGMENTED_7,      {{0, 4, 8, 10}, "aug7", "Augmented 7th"}},
    {ChordType::AUGMENTED_MAJ7,   {{0, 4, 8, 11}, "augMaj7", "Augmented Major 7th"}},
    
    // Sus family
    {ChordType::SUS_2,        {{0, 2, 7}, "sus2", "Suspended 2nd"}},
    {ChordType::SUS_4,        {{0, 5, 7}, "sus4", "Suspended 4th"}},
    {ChordType::SEVEN_SUS_4,  {{0, 5, 7, 10}, "7sus4", "7th Suspended 4th"}}
};

EnhancedChordSystem::EnhancedChordSystem() {
    engineFactory_ = std::make_unique<EngineFactory>();
    initializeDefaultPresets();
    
    std::cout << "EnhancedChordSystem: Initialized Bicep-style multi-engine chord system\n";
}

EnhancedChordSystem::~EnhancedChordSystem() = default;

void EnhancedChordSystem::ChordConfiguration::initializeDefaultVoicing() {
    // Initialize 5 voices with default Bicep-style arrangement
    voices[0] = ChordVoice(EngineType::MACRO_VA, VoiceRole::ROOT, 0.0f);        // Root - VA Bass
    voices[1] = ChordVoice(EngineType::MACRO_FM, VoiceRole::HARMONY_1, 4.0f);   // 3rd - FM Harmony
    voices[2] = ChordVoice(EngineType::MACRO_WAVETABLE, VoiceRole::HARMONY_2, 7.0f); // 5th - Wavetable Pad
    voices[3] = ChordVoice(EngineType::MACRO_HARMONICS, VoiceRole::COLOR, 11.0f);    // 7th - Harmonics Color
    voices[4] = ChordVoice(EngineType::MACRO_WAVESHAPER, VoiceRole::DOUBLING, 12.0f); // Octave - Waveshaper Lead
    
    // Set default levels and pans
    voices[0].level = 0.9f; voices[0].pan = 0.0f;    // Root center, loud
    voices[1].level = 0.7f; voices[1].pan = -0.3f;   // 3rd left, medium
    voices[2].level = 0.8f; voices[2].pan = 0.3f;    // 5th right, medium-loud
    voices[3].level = 0.6f; voices[3].pan = -0.1f;   // 7th left-center, quiet
    voices[4].level = 0.5f; voices[4].pan = 0.5f;    // Octave right, quiet
}

void EnhancedChordSystem::setCurrentChord(ChordType chordType, uint8_t rootNote) {
    currentChordType_ = chordType;
    currentRootNote_ = rootNote;
    
    // Update chord configuration with new intervals
    auto chordNotes = generateChordNotes(chordType, rootNote, currentConfig_.spread);
    
    // Map chord notes to active voices
    for (int i = 0; i < MAX_CHORD_VOICES && i < chordNotes.size(); i++) {
        if (currentConfig_.voices[i].active) {
            currentConfig_.voices[i].noteOffset = chordNotes[i] - rootNote;
        }
    }
    
    std::cout << "EnhancedChordSystem: Set chord to " << getChordSymbol(chordType, rootNote) << "\n";
}

void EnhancedChordSystem::setVoiceEngine(int voiceIndex, EngineType engineType) {
    if (voiceIndex < 0 || voiceIndex >= MAX_CHORD_VOICES) return;
    
    currentConfig_.voices[voiceIndex].engineType = engineType;
    currentConfig_.voices[voiceIndex].engineNeedsUpdate = true;
    
    std::cout << "EnhancedChordSystem: Voice " << voiceIndex 
              << " assigned to engine " << static_cast<int>(engineType) << "\n";
}

void EnhancedChordSystem::assignInstrument(int instrumentIndex, const std::vector<int>& voiceIndices) {
    if (instrumentIndex < 0 || instrumentIndex >= MAX_INSTRUMENTS) return;
    
    // Find or create instrument assignment
    auto it = std::find_if(currentConfig_.instrumentAssignments.begin(),
                          currentConfig_.instrumentAssignments.end(),
                          [instrumentIndex](const InstrumentChordAssignment& assignment) {
                              return assignment.instrumentIndex == instrumentIndex;
                          });
    
    if (it == currentConfig_.instrumentAssignments.end()) {
        currentConfig_.instrumentAssignments.emplace_back(instrumentIndex);
        it = currentConfig_.instrumentAssignments.end() - 1;
    }
    
    it->voiceIndices = voiceIndices;
    
    std::cout << "EnhancedChordSystem: Instrument " << instrumentIndex 
              << " assigned " << voiceIndices.size() << " voices\n";
}

void EnhancedChordSystem::playChord(uint8_t rootNote, float velocity) {
    // Update chord configuration for new root
    setCurrentChord(currentChordType_, rootNote);
    
    // Distribute to assigned instruments
    distributeToInstruments(rootNote, velocity);
    
    std::cout << "EnhancedChordSystem: Playing " << getChordSymbol(currentChordType_, rootNote) 
              << " with " << currentConfig_.instrumentAssignments.size() << " instruments\n";
}

void EnhancedChordSystem::distributeToInstruments(uint8_t rootNote, float velocity) {
    for (const auto& assignment : currentConfig_.instrumentAssignments) {
        // Calculate strum timing
        float strumDelay = assignment.strumOffset;
        if (currentConfig_.strumTime > 0.0f) {
            // Add global strum timing
            float globalStrum = currentConfig_.strumTime * 
                (currentConfig_.strumUp ? assignment.instrumentIndex : 
                 (MAX_INSTRUMENTS - assignment.instrumentIndex - 1)) / MAX_INSTRUMENTS;
            strumDelay += globalStrum;
        }
        
        // Play assigned voices on this instrument
        for (int voiceIndex : assignment.voiceIndices) {
            if (voiceIndex >= 0 && voiceIndex < MAX_CHORD_VOICES) {
                const auto& voice = currentConfig_.voices[voiceIndex];
                if (voice.active) {
                    uint8_t noteToPlay = rootNote + static_cast<uint8_t>(voice.noteOffset);
                    float adjustedVelocity = velocity * voice.level * assignment.velocityScale;
                    
                    // TODO: Send note to instrument with strum delay
                    // This would integrate with the main instrument/engine system
                    
                    activeNotes_.push_back(noteToPlay);
                }
            }
        }
    }
    
    activeVoiceCount_ = activeNotes_.size();
}

std::vector<float> EnhancedChordSystem::generateChordNotes(ChordType chordType, uint8_t rootNote, float spread) const {
    auto it = chordDatabase_.find(chordType);
    if (it == chordDatabase_.end()) {
        return {static_cast<float>(rootNote)}; // Just root if chord not found
    }
    
    std::vector<float> notes;
    float rootFloat = static_cast<float>(rootNote);
    
    for (int interval : it->second.intervals) {
        float note = rootFloat + interval;
        
        // Apply spread - distribute voices across the spread range
        if (notes.size() > 0 && spread > 0.0f) {
            float spreadFactor = static_cast<float>(notes.size()) / it->second.intervals.size();
            note += spreadFactor * spread;
        }
        
        notes.push_back(note);
    }
    
    return notes;
}

std::string EnhancedChordSystem::getChordSymbol(ChordType chordType, uint8_t rootNote) const {
    std::string rootName = noteNumberToName(rootNote);
    
    auto it = chordDatabase_.find(chordType);
    if (it != chordDatabase_.end()) {
        return rootName + it->second.symbol;
    }
    
    return rootName;
}

std::vector<std::string> EnhancedChordSystem::getChordToneNames(ChordType chordType, uint8_t rootNote) const {
    auto chordNotes = generateChordNotes(chordType, rootNote, 0.0f);
    std::vector<std::string> names;
    
    for (float note : chordNotes) {
        names.push_back(noteNumberToName(static_cast<uint8_t>(note)));
    }
    
    return names;
}

std::string EnhancedChordSystem::noteNumberToName(uint8_t noteNumber) const {
    static const std::array<std::string, 12> noteNames = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    
    int octave = noteNumber / 12 - 1;
    int noteIndex = noteNumber % 12;
    
    return noteNames[noteIndex] + std::to_string(octave);
}

float EnhancedChordSystem::noteNumberToFrequency(uint8_t noteNumber) const {
    return 440.0f * std::pow(2.0f, (noteNumber - 69) / 12.0f);
}

void EnhancedChordSystem::initializeDefaultPresets() {
    // Bicep House preset
    ChordConfiguration bicepHouse;
    bicepHouse.name = "Bicep House";
    bicepHouse.voices[0] = ChordVoice(EngineType::MACRO_VA, VoiceRole::ROOT, 0.0f);
    bicepHouse.voices[1] = ChordVoice(EngineType::MACRO_FM, VoiceRole::HARMONY_1, 4.0f);
    bicepHouse.voices[2] = ChordVoice(EngineType::MACRO_WAVETABLE, VoiceRole::HARMONY_2, 7.0f);
    bicepHouse.voices[3] = ChordVoice(EngineType::MACRO_HARMONICS, VoiceRole::COLOR, 10.0f);
    bicepHouse.voices[4].active = false; // 4-voice house chords
    
    // Set up multi-instrument distribution
    bicepHouse.instrumentAssignments.push_back(InstrumentChordAssignment(0)); // Bass
    bicepHouse.instrumentAssignments.back().voiceIndices = {0};
    
    bicepHouse.instrumentAssignments.push_back(InstrumentChordAssignment(1)); // Pad
    bicepHouse.instrumentAssignments.back().voiceIndices = {1, 2, 3};
    
    bicepHouse.instrumentAssignments.push_back(InstrumentChordAssignment(2)); // Lead
    bicepHouse.instrumentAssignments.back().voiceIndices = {3};
    bicepHouse.instrumentAssignments.back().arpeggiate = true;
    
    arrangementPresets_["Bicep House"] = bicepHouse;
    
    // Ambient Pad preset
    ChordConfiguration ambientPad;
    ambientPad.name = "Ambient Pad";
    ambientPad.spread = 24.0f; // Wide spread
    ambientPad.humanization = 0.3f; // Some timing variation
    ambientPad.enableVoiceLeading = true;
    
    for (int i = 0; i < MAX_CHORD_VOICES; i++) {
        ambientPad.voices[i] = ChordVoice(EngineType::MACRO_WAVETABLE, 
                                         static_cast<VoiceRole>(i % 5), i * 3.0f);
        ambientPad.voices[i].level = 0.6f; // Quieter, blended
        ambientPad.voices[i].detune = (i - 2) * 5.0f; // Slight detuning
    }
    
    arrangementPresets_["Ambient Pad"] = ambientPad;
    
    std::cout << "EnhancedChordSystem: Loaded " << arrangementPresets_.size() << " arrangement presets\n";
}

void EnhancedChordSystem::loadArrangementPreset(const std::string& presetName) {
    auto it = arrangementPresets_.find(presetName);
    if (it != arrangementPresets_.end()) {
        currentConfig_ = it->second;
        std::cout << "EnhancedChordSystem: Loaded arrangement preset '" << presetName << "'\n";
    } else {
        std::cout << "EnhancedChordSystem: Preset '" << presetName << "' not found\n";
    }
}

std::vector<std::string> EnhancedChordSystem::getArrangementPresets() const {
    std::vector<std::string> presetNames;
    for (const auto& preset : arrangementPresets_) {
        presetNames.push_back(preset.first);
    }
    return presetNames;
}

void EnhancedChordSystem::releaseChord() {
    activeNotes_.clear();
    activeVoiceCount_ = 0;
    
    // TODO: Send note off to all assigned instruments
    
    std::cout << "EnhancedChordSystem: Released all chord voices\n";
}

float EnhancedChordSystem::getCPUUsage() const {
    // Estimate CPU usage based on active voices and engine complexity
    float usage = activeVoiceCount_ * 0.02f; // Base cost per voice
    
    for (const auto& voice : currentConfig_.voices) {
        if (voice.active && voice.engineInstance) {
            // Add engine-specific overhead
            switch (voice.engineType) {
                case EngineType::MACRO_FM:
                case EngineType::MACRO_HARMONICS:
                    usage += 0.05f; // Higher CPU engines
                    break;
                default:
                    usage += 0.02f; // Standard engines
                    break;
            }
        }
    }
    
    return std::min(usage, 1.0f);
}

// Voice leading engine implementation
std::vector<EnhancedChordSystem::VoiceLeadingEngine::VoiceMovement> 
EnhancedChordSystem::VoiceLeadingEngine::calculateOptimalVoicing(
    const std::vector<float>& fromNotes,
    const std::vector<float>& toNotes,
    float maxMovement) {
    
    std::vector<VoiceMovement> movements;
    
    // Simple voice leading: minimize total movement
    for (size_t i = 0; i < fromNotes.size() && i < toNotes.size(); i++) {
        VoiceMovement movement;
        movement.voiceIndex = i;
        movement.fromNote = fromNotes[i];
        movement.toNote = toNotes[i];
        movement.distance = std::abs(toNotes[i] - fromNotes[i]);
        
        // Find closest voice in target chord if movement is too large
        if (movement.distance > maxMovement) {
            float bestDistance = movement.distance;
            float bestTarget = movement.toNote;
            
            for (float targetNote : toNotes) {
                float testDistance = std::abs(targetNote - movement.fromNote);
                if (testDistance < bestDistance) {
                    bestDistance = testDistance;
                    bestTarget = targetNote;
                }
            }
            
            movement.toNote = bestTarget;
            movement.distance = bestDistance;
        }
        
        movements.push_back(movement);
    }
    
    return movements;
}

} // namespace EtherSynth