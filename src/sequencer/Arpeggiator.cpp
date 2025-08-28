#include "Arpeggiator.h"
#include <algorithm>
#include <cmath>

namespace EtherSynth {

Arpeggiator::Arpeggiator() 
    : sampleRate_(48000.0f), tempo_(120.0f), externalSync_(false),
      samplesPerBeat_(0.0f), samplesPerStep_(0.0f), sampleCounter_(0),
      nextStepTime_(0), currentStep_(0), patternLength_(0),
      isRunning_(false), latchActive_(false), currentNoteId_(1),
      lastNoteOnTime_(0), lastNoteOffTime_(0), swingOffset_(0.0f),
      isSwingStep_(false), randomSeed_(12345) {
    
    // Initialize CC mappings to invalid values
    ccMappings_.fill(255);
    
    // Calculate initial timing
    updateClockDivision();
    
    // Reserve space for performance
    heldNotes_.reserve(16);
    arpNotes_.reserve(128); // Up to 4 octaves * 16 notes * 2 (up/down)
}

void Arpeggiator::setSettings(const ArpSettings& settings) {
    settings_ = settings;
    
    // Clamp values to valid ranges
    settings_.octaves = std::clamp(settings_.octaves, uint8_t(1), uint8_t(4));
    settings_.noteLength = std::clamp(settings_.noteLength, uint8_t(1), uint8_t(100));
    settings_.swing = std::clamp(settings_.swing, uint8_t(0), uint8_t(100));
    settings_.velocity = std::clamp(settings_.velocity, 0.0f, 200.0f);
    settings_.transpose = std::clamp(settings_.transpose, int8_t(-24), int8_t(24));
    settings_.gateLength = std::clamp(settings_.gateLength, uint8_t(1), uint8_t(100));
    
    updateClockDivision();
    updateSwingTiming();
    generatePattern();
}

void Arpeggiator::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updateClockDivision();
}

void Arpeggiator::setTempo(float bpm) {
    tempo_ = std::clamp(bpm, 60.0f, 200.0f);
    updateClockDivision();
}

void Arpeggiator::noteOn(uint8_t midiNote, float velocity) {
    // Add to held notes
    addNoteToHeld(midiNote, velocity);
    
    // Regenerate pattern with new notes
    generatePattern();
    
    // Handle retrigger
    if (settings_.retrigger && !arpNotes_.empty()) {
        currentStep_ = 0;
        nextStepTime_ = sampleCounter_;
        isRunning_ = true;
    }
    
    lastNoteOnTime_ = sampleCounter_;
}

void Arpeggiator::noteOff(uint8_t midiNote) {
    if (!settings_.hold && !settings_.latch) {
        removeNoteFromHeld(midiNote);
        generatePattern();
        
        // Stop if no more notes
        if (heldNotes_.empty()) {
            isRunning_ = false;
        }
    } else if (settings_.latch) {
        // Toggle latch mode
        if (latchActive_) {
            removeNoteFromHeld(midiNote);
            generatePattern();
            if (heldNotes_.empty()) {
                latchActive_ = false;
                isRunning_ = false;
            }
        }
    }
    
    lastNoteOffTime_ = sampleCounter_;
}

void Arpeggiator::allNotesOff() {
    heldNotes_.clear();
    if (!settings_.hold) {
        arpNotes_.clear();
        isRunning_ = false;
    }
}

void Arpeggiator::panicStop() {
    heldNotes_.clear();
    arpNotes_.clear();
    isRunning_ = false;
    latchActive_ = false;
    currentStep_ = 0;
}

Arpeggiator::ArpOutput Arpeggiator::process() {
    ArpOutput output;
    output.noteOn = false;
    output.noteOff = false;
    output.midiNote = 60;
    output.velocity = 0.8f;
    output.noteId = 0;
    
    sampleCounter_++;
    
    if (!isActive() || arpNotes_.empty()) {
        return output;
    }
    
    // Check if it's time for the next step
    if (isStepTime()) {
        // Calculate swing offset for this step
        updateSwingTiming();
        
        // Get current arpeggio note
        if (currentStep_ < patternLength_) {
            const ArpNote& arpNote = arpNotes_[currentStep_];
            
            // Apply transpose
            uint8_t transposedNote = transposeNote(arpNote.midiNote, settings_.transpose);
            
            // Apply velocity scaling
            float scaledVelocity = arpNote.velocity * (settings_.velocity / 100.0f);
            scaledVelocity = std::clamp(scaledVelocity, 0.0f, 1.0f);
            
            // Apply accent pattern
            if (isAccentStep(currentStep_)) {
                scaledVelocity = getVelocityForStep(currentStep_, scaledVelocity);
            }
            
            // Trigger note
            output.noteOn = true;
            output.midiNote = transposedNote;
            output.velocity = scaledVelocity;
            output.noteId = currentNoteId_++;
            
            // Schedule note off based on gate length
            // (This would be handled by the calling engine)
        }
        
        // Advance to next step
        currentStep_++;
        if (currentStep_ >= patternLength_) {
            currentStep_ = 0;
            
            // Regenerate random pattern if needed
            if (settings_.pattern == Pattern::RANDOM) {
                generateRandomPattern();
            }
        }
        
        // Calculate next step time with swing
        float stepInterval = samplesPerStep_;
        if (isSwingStep_) {
            stepInterval += swingOffset_;
        }
        nextStepTime_ = sampleCounter_ + static_cast<uint32_t>(stepInterval);
    }
    
    return output;
}

void Arpeggiator::generatePattern() {
    arpNotes_.clear();
    
    if (heldNotes_.empty()) {
        patternLength_ = 0;
        return;
    }
    
    switch (settings_.pattern) {
        case Pattern::UP:
            generateUpPattern();
            break;
        case Pattern::DOWN:
            generateDownPattern();
            break;
        case Pattern::UP_DOWN:
            generateUpDownPattern();
            break;
        case Pattern::DOWN_UP:
            generateDownUpPattern();
            break;
        case Pattern::UP_DOWN_INCLUSIVE:
            generateUpDownInclusivePattern();
            break;
        case Pattern::RANDOM:
            generateRandomPattern();
            break;
        case Pattern::PLAYED_ORDER:
            generatePlayedOrderPattern();
            break;
        case Pattern::CHORD:
            generateChordPattern();
            break;
    }
    
    patternLength_ = static_cast<int>(arpNotes_.size());
}

void Arpeggiator::generateUpPattern() {
    // Sort held notes by pitch
    std::vector<ArpNote> sortedNotes = heldNotes_;
    std::sort(sortedNotes.begin(), sortedNotes.end(),
              [](const ArpNote& a, const ArpNote& b) {
                  return a.midiNote < b.midiNote;
              });
    
    // Generate pattern for specified octave range
    for (int octave = 0; octave < settings_.octaves; ++octave) {
        for (const auto& note : sortedNotes) {
            ArpNote arpNote = note;
            arpNote.midiNote += octave * 12;
            
            // Keep within MIDI range
            if (arpNote.midiNote <= 127) {
                arpNotes_.push_back(arpNote);
            }
        }
    }
}

void Arpeggiator::generateDownPattern() {
    generateUpPattern(); // Generate up first
    std::reverse(arpNotes_.begin(), arpNotes_.end()); // Then reverse
}

void Arpeggiator::generateUpDownPattern() {
    generateUpPattern();
    
    // Add down pattern (excluding first and last to avoid duplication)
    if (arpNotes_.size() > 2) {
        size_t upSize = arpNotes_.size();
        for (int i = static_cast<int>(upSize) - 2; i > 0; --i) {
            arpNotes_.push_back(arpNotes_[i]);
        }
    }
}

void Arpeggiator::generateRandomPattern() {
    generateUpPattern(); // Start with up pattern
    
    // Shuffle using fast random generator
    for (size_t i = arpNotes_.size(); i > 1; --i) {
        size_t j = fastRandom() % i;
        std::swap(arpNotes_[i - 1], arpNotes_[j]);
    }
}

void Arpeggiator::generatePlayedOrderPattern() {
    // Sort held notes by the time they were played
    std::vector<ArpNote> sortedNotes = heldNotes_;
    std::sort(sortedNotes.begin(), sortedNotes.end(),
              [](const ArpNote& a, const ArpNote& b) {
                  return a.startTime < b.startTime;
              });
    
    // Generate pattern for specified octave range
    for (int octave = 0; octave < settings_.octaves; ++octave) {
        for (const auto& note : sortedNotes) {
            ArpNote arpNote = note;
            arpNote.midiNote += octave * 12;
            
            if (arpNote.midiNote <= 127) {
                arpNotes_.push_back(arpNote);
            }
        }
    }
}

void Arpeggiator::generateChordPattern() {
    // All notes played simultaneously (pattern length = 1)
    arpNotes_ = heldNotes_;
}

// Private helper methods

void Arpeggiator::updateClockDivision() {
    if (tempo_ <= 0.0f || sampleRate_ <= 0.0f) return;
    
    samplesPerBeat_ = (60.0f / tempo_) * sampleRate_;
    float divisionMultiplier = getDivisionMultiplier(settings_.division);
    samplesPerStep_ = samplesPerBeat_ / divisionMultiplier;
}

void Arpeggiator::updateSwingTiming() {
    float swingAmount = (settings_.swing - 50.0f) / 50.0f; // -1.0 to 1.0
    swingOffset_ = swingAmount * samplesPerStep_ * 0.1f; // Max 10% timing offset
    
    isSwingStep_ = (currentStep_ % 2) == 1; // Swing on off-beats
}

void Arpeggiator::addNoteToHeld(uint8_t midiNote, float velocity) {
    // Remove if already exists
    removeNoteFromHeld(midiNote);
    
    // Add new note
    heldNotes_.emplace_back(midiNote, velocity, sampleCounter_);
}

void Arpeggiator::removeNoteFromHeld(uint8_t midiNote) {
    heldNotes_.erase(
        std::remove_if(heldNotes_.begin(), heldNotes_.end(),
                       [midiNote](const ArpNote& note) {
                           return note.midiNote == midiNote;
                       }),
        heldNotes_.end()
    );
}

uint32_t Arpeggiator::fastRandom() {
    // Linear congruential generator for fast random numbers
    randomSeed_ = randomSeed_ * 1664525u + 1013904223u;
    return randomSeed_;
}

float Arpeggiator::getDivisionMultiplier(Division division) const {
    switch (division) {
        case Division::WHOLE: return 1.0f;
        case Division::HALF: return 2.0f;
        case Division::QUARTER: return 4.0f;
        case Division::EIGHTH: return 8.0f;
        case Division::SIXTEENTH: return 16.0f;
        case Division::THIRTY_SECOND: return 32.0f;
        case Division::EIGHTH_TRIPLET: return 12.0f; // 3 triplets per quarter
        case Division::SIXTEENTH_TRIPLET: return 24.0f; // 6 triplets per quarter
        default: return 16.0f;
    }
}

bool Arpeggiator::isAccentStep(int step) const {
    if (settings_.accentPattern == 0) return false;
    
    // Simple accent patterns
    switch (settings_.accentPattern) {
        case 1: return (step % 4) == 0; // Every 4 steps
        case 2: return (step % 2) == 0; // Every 2 steps  
        case 3: return (step % 8) == 0; // Every 8 steps
        default: return false;
    }
}

float Arpeggiator::getVelocityForStep(int step, float baseVelocity) const {
    if (isAccentStep(step)) {
        return std::min(1.0f, baseVelocity * 1.2f); // 20% louder accent
    }
    return baseVelocity;
}

uint8_t Arpeggiator::transposeNote(uint8_t note, int8_t semitones) const {
    int transposed = static_cast<int>(note) + semitones;
    return static_cast<uint8_t>(std::clamp(transposed, 0, 127));
}

void Arpeggiator::reset() {
    sampleCounter_ = 0;
    nextStepTime_ = 0;
    currentStep_ = 0;
    isRunning_ = false;
}

} // namespace EtherSynth