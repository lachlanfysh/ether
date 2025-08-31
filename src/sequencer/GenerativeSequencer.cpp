#include "GenerativeSequencer.h"
#include "../core/Logger.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <fstream>

namespace EtherSynth {

// Music theory constants
const std::array<std::array<int, 7>, 12> GenerativeSequencer::MusicTheory::scales = {{
    {0, 2, 4, 5, 7, 9, 11}, // Major
    {0, 2, 3, 5, 7, 8, 10}, // Minor
    {0, 2, 3, 5, 7, 9, 10}, // Dorian
    {0, 1, 3, 5, 7, 8, 10}, // Phrygian
    {0, 2, 4, 6, 7, 9, 11}, // Lydian
    {0, 2, 4, 5, 7, 9, 10}, // Mixolydian
    {0, 2, 3, 5, 6, 8, 10}, // Aeolian (Natural Minor)
    {0, 1, 3, 5, 6, 8, 10}, // Locrian
    {0, 2, 4, 7, 9},        // Pentatonic Major
    {0, 3, 5, 7, 10},       // Pentatonic Minor
    {0, 1, 4, 5, 7, 8, 11}, // Harmonic Minor
    {0, 1, 3, 4, 6, 8, 10}  // Chromatic (simplified)
}};

const std::array<int, 24> GenerativeSequencer::MusicTheory::chordProgressions = {
    1, 5, 6, 4,  // I-V-vi-IV (Pop progression)
    6, 4, 1, 5,  // vi-IV-I-V (Alternative)
    1, 6, 4, 5,  // I-vi-IV-V (50s progression)
    1, 4, 5, 1,  // I-IV-V-I (Classic)
    2, 5, 1, 1,  // ii-V-I (Jazz)
    1, 7, 4, 1   // I-VII-IV-I (Modal)
};

const std::map<GenerativeSequencer::MusicalStyle, float> GenerativeSequencer::MusicTheory::styleTension = {
    {MusicalStyle::ELECTRONIC, 0.4f},
    {MusicalStyle::TECHNO, 0.6f},
    {MusicalStyle::HOUSE, 0.3f},
    {MusicalStyle::AMBIENT, 0.2f},
    {MusicalStyle::DRUM_AND_BASS, 0.8f},
    {MusicalStyle::ACID, 0.7f},
    {MusicalStyle::INDUSTRIAL, 0.9f},
    {MusicalStyle::MELODIC, 0.3f},
    {MusicalStyle::EXPERIMENTAL, 1.0f}
};

GenerativeSequencer::GenerativeSequencer() 
    : rng_(std::chrono::high_resolution_clock::now().time_since_epoch().count())
    , dist_(0.0f, 1.0f)
{
    Logger::getInstance().log("GenerativeSequencer: Initializing AI composition engine");
    
    // Initialize style templates
    initializeStyleTemplates();
    
    // Set default parameters
    params_.mode = GenerationMode::ASSIST;
    params_.style = MusicalStyle::ELECTRONIC;
    params_.complexity = GenerationComplexity::MODERATE;
    
    // Initialize current scale to C major
    currentScale_.rootNote = 0;
    currentScale_.scaleType = 0;
    currentScale_.confidence = 1.0f;
    for (int i : scales[0]) {
        currentScale_.notes[i] = true;
    }
    
    Logger::getInstance().log("GenerativeSequencer: AI engine initialized successfully");
}

uint32_t GenerativeSequencer::generatePattern(const GenerationParams& params, int trackIndex) {
    Logger::getInstance().log("GenerativeSequencer: Generating pattern for track " + std::to_string(trackIndex));
    
    uint32_t patternId = generateUniquePatternId();
    std::vector<NoteEvent> events;
    
    switch (params.mode) {
        case GenerationMode::GENERATE: {
            // Full AI generation based on style and parameters
            events = generateFullPattern(params, trackIndex);
            break;
        }
        case GenerationMode::EVOLVE: {
            // Evolve from existing patterns
            if (!patternComplexityCache_.empty()) {
                auto basePattern = patternComplexityCache_.begin()->first;
                events = evolveFromPattern(basePattern, params);
            } else {
                events = generateFullPattern(params, trackIndex);
            }
            break;
        }
        case GenerationMode::HARMONIZE: {
            // Generate harmonic accompaniment
            events = generateHarmonicPattern(params, trackIndex);
            break;
        }
        case GenerationMode::RHYTHMIZE: {
            // Generate rhythmic variations
            events = generateRhythmicPattern(params, trackIndex);
            break;
        }
        default:
            events = generateFullPattern(params, trackIndex);
    }
    
    // Apply quantization and humanization
    if (params.quantization > 0.0f) {
        events = quantizeEvents(events, params.quantization);
    }
    if (params.swing != 0.0f) {
        events = addSwing(events, params.swing);
    }
    if (params.humanization > 0.0f) {
        events = humanizeEvents(events, params.humanization);
    }
    
    // Optimize for hardware if requested
    if (params.respectKeyLayout) {
        optimizeForHardware(patternId);
    }
    
    // Cache analysis results
    patternComplexityCache_[patternId] = calculatePatternComplexity(patternId);
    patternInterestCache_[patternId] = calculatePatternInterest(patternId);
    
    Logger::getInstance().log("GenerativeSequencer: Generated pattern " + std::to_string(patternId) + 
                             " with " + std::to_string(events.size()) + " events");
    
    return patternId;
}

std::vector<NoteEvent> GenerativeSequencer::generateFullPattern(const GenerationParams& params, int trackIndex) {
    std::vector<NoteEvent> events;
    
    // Determine pattern length based on style
    int patternLength = 64; // Default to 4 bars at 16th note resolution
    
    switch (params.style) {
        case MusicalStyle::TECHNO:
        case MusicalStyle::HOUSE:
            patternLength = 64; // 4 bars
            break;
        case MusicalStyle::DRUM_AND_BASS:
            patternLength = 32; // 2 bars
            break;
        case MusicalStyle::AMBIENT:
            patternLength = 128; // 8 bars
            break;
        default:
            patternLength = 64;
    }
    
    // Generate based on track type (inferred from trackIndex)
    if (trackIndex < 4) {
        // Melodic tracks
        events = generateMelodicLine(currentScale_, patternLength / 16);
    } else {
        // Rhythmic tracks
        RhythmicPattern rhythmTemplate = generateStyleRhythm(params.style, patternLength / 16);
        events = generatePercussion(rhythmTemplate, params.rhythmicVariation);
    }
    
    // Apply density control
    if (params.density < 1.0f) {
        events = applyDensityFilter(events, params.density);
    }
    
    // Apply complexity adjustments
    switch (params.complexity) {
        case GenerationComplexity::SIMPLE:
            events = simplifyPattern(events);
            break;
        case GenerationComplexity::COMPLEX:
            events = complexifyPattern(events);
            break;
        case GenerationComplexity::ADAPTIVE:
            float complexity = performanceAnalysis_.melodicComplexity;
            if (complexity > 0.7f) {
                events = complexifyPattern(events);
            } else if (complexity < 0.3f) {
                events = simplifyPattern(events);
            }
            break;
    }
    
    return events;
}

std::vector<NoteEvent> GenerativeSequencer::generateMelodicLine(const ScaleAnalysis& scale, int bars) {
    std::vector<NoteEvent> events;
    
    // Get scale notes
    std::vector<int> scaleNotes;
    for (int i = 0; i < 12; i++) {
        if (scale.notes[i]) {
            scaleNotes.push_back(i);
        }
    }
    
    if (scaleNotes.empty()) {
        Logger::getInstance().log("GenerativeSequencer: Warning - empty scale, using chromatic");
        for (int i = 0; i < 12; i++) scaleNotes.push_back(i);
    }
    
    int currentNote = scaleNotes[rng_() % scaleNotes.size()];
    float currentTime = 0.0f;
    float timeStep = 0.25f; // 16th notes
    
    for (int bar = 0; bar < bars; bar++) {
        for (int beat = 0; beat < 16; beat++) {
            // Probability of note generation
            float noteProbability = 0.3f + (params_.density * 0.4f);
            
            if (dist_(rng_) < noteProbability) {
                // Generate note
                NoteEvent event;
                event.type = NoteEventType::NOTE_ON;
                event.timestamp = currentTime;
                event.note = 60 + currentNote; // Middle C + scale note
                event.velocity = 0.5f + (dist_(rng_) * 0.4f); // 0.5-0.9 velocity
                event.channel = 0;
                
                events.push_back(event);
                
                // Add corresponding note off
                NoteEvent offEvent = event;
                offEvent.type = NoteEventType::NOTE_OFF;
                offEvent.timestamp = currentTime + timeStep * (0.8f + dist_(rng_) * 0.4f);
                events.push_back(offEvent);
                
                // Move to next note in scale (with some randomness)
                if (dist_(rng_) < 0.7f) {
                    // Step-wise motion
                    auto it = std::find(scaleNotes.begin(), scaleNotes.end(), currentNote);
                    if (it != scaleNotes.end()) {
                        int index = std::distance(scaleNotes.begin(), it);
                        if (dist_(rng_) < 0.5f && index > 0) {
                            currentNote = scaleNotes[index - 1]; // Step down
                        } else if (index < scaleNotes.size() - 1) {
                            currentNote = scaleNotes[index + 1]; // Step up
                        }
                    }
                } else {
                    // Jump to random note
                    currentNote = scaleNotes[rng_() % scaleNotes.size()];
                }
            }
            
            currentTime += timeStep;
        }
    }
    
    // Sort events by timestamp
    std::sort(events.begin(), events.end(), 
              [](const NoteEvent& a, const NoteEvent& b) {
                  return a.timestamp < b.timestamp;
              });
    
    return events;
}

GenerativeSequencer::RhythmicPattern GenerativeSequencer::generateStyleRhythm(MusicalStyle style, int bars) {
    RhythmicPattern pattern;
    pattern.subdivision = 16;
    
    int totalSteps = bars * 16;
    pattern.kicks.resize(totalSteps, false);
    pattern.snares.resize(totalSteps, false);
    pattern.hihats.resize(totalSteps, false);
    pattern.velocities.resize(totalSteps, 0.0f);
    
    switch (style) {
        case MusicalStyle::TECHNO: {
            // Four-on-the-floor kick pattern
            for (int i = 0; i < totalSteps; i += 4) {
                pattern.kicks[i] = true;
                pattern.velocities[i] = 0.8f + (dist_(rng_) * 0.2f);
            }
            
            // Off-beat hi-hats
            for (int i = 2; i < totalSteps; i += 4) {
                if (dist_(rng_) < 0.8f) {
                    pattern.hihats[i] = true;
                    pattern.velocities[i] = 0.4f + (dist_(rng_) * 0.3f);
                }
            }
            
            // Occasional snares
            for (int i = 8; i < totalSteps; i += 16) {
                if (dist_(rng_) < 0.6f) {
                    pattern.snares[i] = true;
                    pattern.velocities[i] = 0.7f + (dist_(rng_) * 0.2f);
                }
            }
            break;
        }
        
        case MusicalStyle::HOUSE: {
            // Classic house pattern
            for (int i = 0; i < totalSteps; i += 4) {
                pattern.kicks[i] = true;
                pattern.velocities[i] = 0.8f;
            }
            
            // Steady hi-hats
            for (int i = 0; i < totalSteps; i += 2) {
                pattern.hihats[i] = true;
                pattern.velocities[i] = (i % 4 == 0) ? 0.6f : 0.4f;
            }
            
            // Snare on 2 and 4
            for (int i = 8; i < totalSteps; i += 16) {
                pattern.snares[i] = true;
                pattern.snares[i + 8] = true;
                pattern.velocities[i] = 0.7f;
                pattern.velocities[i + 8] = 0.7f;
            }
            break;
        }
        
        case MusicalStyle::DRUM_AND_BASS: {
            // Breakbeat-style pattern
            std::vector<int> breakbeat = {0, 6, 8, 10, 14};
            for (int bar = 0; bar < bars; bar++) {
                for (int hit : breakbeat) {
                    int pos = bar * 16 + hit;
                    if (pos < totalSteps) {
                        if (hit == 0 || hit == 8) {
                            pattern.kicks[pos] = true;
                            pattern.velocities[pos] = 0.9f;
                        } else {
                            pattern.snares[pos] = true;
                            pattern.velocities[pos] = 0.7f + (dist_(rng_) * 0.2f);
                        }
                    }
                }
            }
            
            // Fast hi-hats
            for (int i = 0; i < totalSteps; i++) {
                if (!pattern.kicks[i] && !pattern.snares[i] && dist_(rng_) < 0.4f) {
                    pattern.hihats[i] = true;
                    pattern.velocities[i] = 0.3f + (dist_(rng_) * 0.2f);
                }
            }
            break;
        }
        
        case MusicalStyle::AMBIENT: {
            // Sparse, atmospheric rhythms
            for (int i = 0; i < totalSteps; i += 8) {
                if (dist_(rng_) < 0.3f) {
                    pattern.kicks[i] = true;
                    pattern.velocities[i] = 0.5f + (dist_(rng_) * 0.3f);
                }
            }
            
            // Occasional texture elements
            for (int i = 0; i < totalSteps; i++) {
                if (dist_(rng_) < 0.1f) {
                    pattern.hihats[i] = true;
                    pattern.velocities[i] = 0.2f + (dist_(rng_) * 0.3f);
                }
            }
            break;
        }
        
        default: {
            // Default electronic pattern
            for (int i = 0; i < totalSteps; i += 4) {
                if (dist_(rng_) < 0.7f) {
                    pattern.kicks[i] = true;
                    pattern.velocities[i] = 0.7f + (dist_(rng_) * 0.3f);
                }
            }
        }
    }
    
    return pattern;
}

void GenerativeSequencer::evolvePattern(uint32_t patternId, float evolutionAmount) {
    Logger::getInstance().log("GenerativeSequencer: Evolving pattern " + std::to_string(patternId) +
                             " with amount " + std::to_string(evolutionAmount));
    
    mutatePattern(patternId, evolutionAmount * params_.evolution);
    
    // Update complexity cache
    patternComplexityCache_[patternId] = calculatePatternComplexity(patternId);
    patternInterestCache_[patternId] = calculatePatternInterest(patternId);
}

void GenerativeSequencer::mutatePattern(uint32_t patternId, float mutationRate) {
    // This would interface with the actual pattern data structure
    // For now, we'll log the mutation intent
    Logger::getInstance().log("GenerativeSequencer: Mutating pattern " + std::to_string(patternId) +
                             " with rate " + std::to_string(mutationRate));
    
    // Pattern mutation would involve:
    // - Randomly changing note velocities
    // - Shifting note timings slightly
    // - Adding/removing notes based on mutation rate
    // - Changing note durations
    // - Modifying rhythmic patterns
}

float GenerativeSequencer::calculatePatternComplexity(uint32_t patternId) {
    // Simplified complexity calculation
    // In a full implementation, this would analyze:
    // - Note density
    // - Rhythmic complexity
    // - Harmonic complexity
    // - Timing variations
    
    return 0.5f + (dist_(rng_) * 0.5f); // Placeholder
}

float GenerativeSequencer::calculatePatternInterest(uint32_t patternId) {
    // Simplified interest calculation
    // In a full implementation, this would analyze:
    // - Surprise elements
    // - Variation patterns
    // - Melodic contour
    // - Rhythmic syncopation
    
    return 0.4f + (dist_(rng_) * 0.6f); // Placeholder
}

void GenerativeSequencer::analyzeUserPerformance(const std::vector<NoteEvent>& events) {
    if (events.empty()) return;
    
    Logger::getInstance().log("GenerativeSequencer: Analyzing " + std::to_string(events.size()) + " user events");
    
    // Calculate average velocity
    float totalVelocity = 0.0f;
    int noteOnCount = 0;
    
    for (const auto& event : events) {
        if (event.type == NoteEventType::NOTE_ON) {
            totalVelocity += event.velocity;
            noteOnCount++;
            
            // Update note preferences
            learningModel_.notePreferences[event.note % 12] += params_.responsiveness * 0.1f;
        }
    }
    
    if (noteOnCount > 0) {
        performanceAnalysis_.averageVelocity = totalVelocity / noteOnCount;
        performanceAnalysis_.notesPlayed += noteOnCount;
    }
    
    // Analyze scale
    currentScale_ = analyzeScale(events);
    
    // Update learning model
    learningModel_.sessionCount++;
    updateAdaptiveModel(performanceAnalysis_);
}

GenerativeSequencer::ScaleAnalysis GenerativeSequencer::analyzeScale(const std::vector<NoteEvent>& events) {
    ScaleAnalysis analysis;
    std::array<int, 12> noteHistogram{};
    
    // Count note occurrences
    for (const auto& event : events) {
        if (event.type == NoteEventType::NOTE_ON) {
            noteHistogram[event.note % 12]++;
        }
    }
    
    // Find root note (most common note)
    int maxCount = 0;
    for (int i = 0; i < 12; i++) {
        if (noteHistogram[i] > maxCount) {
            maxCount = noteHistogram[i];
            analysis.rootNote = i;
        }
    }
    
    // Determine scale type by comparing note patterns
    float bestMatch = 0.0f;
    for (int scaleType = 0; scaleType < scales.size(); scaleType++) {
        float match = 0.0f;
        for (int i = 0; i < 7; i++) {
            int scaleNote = (analysis.rootNote + scales[scaleType][i]) % 12;
            if (noteHistogram[scaleNote] > 0) {
                match += 1.0f;
            }
        }
        if (match > bestMatch) {
            bestMatch = match;
            analysis.scaleType = scaleType;
        }
    }
    
    // Set active notes
    analysis.notes.fill(false);
    for (int i = 0; i < 7; i++) {
        int note = (analysis.rootNote + scales[analysis.scaleType][i]) % 12;
        analysis.notes[note] = true;
    }
    
    analysis.confidence = bestMatch / 7.0f;
    
    Logger::getInstance().log("GenerativeSequencer: Detected scale - Root: " + std::to_string(analysis.rootNote) +
                             ", Type: " + std::to_string(analysis.scaleType) +
                             ", Confidence: " + std::to_string(analysis.confidence));
    
    return analysis;
}

std::vector<NoteEvent> GenerativeSequencer::quantizeEvents(const std::vector<NoteEvent>& events, float strength) {
    std::vector<NoteEvent> quantized = events;
    
    float quantizeGrid = 0.25f; // 16th note grid
    
    for (auto& event : quantized) {
        float gridTime = std::round(event.timestamp / quantizeGrid) * quantizeGrid;
        event.timestamp = event.timestamp * (1.0f - strength) + gridTime * strength;
    }
    
    return quantized;
}

std::vector<NoteEvent> GenerativeSequencer::addSwing(const std::vector<NoteEvent>& events, float swingAmount) {
    std::vector<NoteEvent> swung = events;
    
    for (auto& event : swung) {
        // Apply swing to off-beats (2nd, 4th, 6th, 8th 16th notes in each beat)
        float beatPosition = std::fmod(event.timestamp, 1.0f);
        float sixteenthPosition = std::fmod(event.timestamp, 0.25f);
        
        if (std::abs(sixteenthPosition - 0.125f) < 0.01f || std::abs(sixteenthPosition - 0.375f) < 0.01f) {
            // This is an off-beat 16th note
            event.timestamp += swingAmount * 0.1f;
        }
    }
    
    return swung;
}

std::vector<NoteEvent> GenerativeSequencer::humanizeEvents(const std::vector<NoteEvent>& events, float amount) {
    std::vector<NoteEvent> humanized = events;
    
    for (auto& event : humanized) {
        // Add random timing variation
        float timeVariation = (dist_(rng_) - 0.5f) * amount * 0.05f; // Up to 50ms at full humanization
        event.timestamp += timeVariation;
        
        // Add random velocity variation
        float velocityVariation = (dist_(rng_) - 0.5f) * amount * 0.2f;
        event.velocity = std::clamp(event.velocity + velocityVariation, 0.0f, 1.0f);
    }
    
    return humanized;
}

uint32_t GenerativeSequencer::generateUniquePatternId() {
    static uint32_t counter = 10000; // Start high to avoid conflicts
    return counter++;
}

void GenerativeSequencer::initializeStyleTemplates() {
    // This would load style templates from file or create them programmatically
    Logger::getInstance().log("GenerativeSequencer: Initializing style templates");
    
    // For now, we'll create some basic templates
    for (int style = 0; style < static_cast<int>(MusicalStyle::COUNT); style++) {
        MusicalStyle styleEnum = static_cast<MusicalStyle>(style);
        styleTemplates_[styleEnum] = generateStyleRhythm(styleEnum, 4);
    }
}

void GenerativeSequencer::updateAdaptiveModel(const PerformanceAnalysis& analysis) {
    // Update learning model based on performance analysis
    learningModel_.adaptationRate = std::max(0.01f, learningModel_.adaptationRate * learningModel_.decayRate);
    
    // Adapt generation parameters based on user behavior
    if (analysis.averageVelocity > 0.8f) {
        params_.density = std::min(1.0f, params_.density + learningModel_.adaptationRate);
    } else if (analysis.averageVelocity < 0.3f) {
        params_.density = std::max(0.0f, params_.density - learningModel_.adaptationRate);
    }
}

} // namespace EtherSynth