import Foundation
import SwiftUI

// MARK: - Real Data Models (Based on C++ SequencerPattern architecture)

/// Real step data with note events and timing parameters
struct SequencerStep {
    var noteEvents: [NoteEvent] = []
    var microTiming: Int8 = 0        // -50 to +50 ms
    var accent: Bool = false
    var tie: Bool = false
    var ratchetCount: UInt8 = 1      // 1-8 subdivision hits
    var probability: Float = 1.0     // 0.0-1.0 probability
    var velocity: UInt8 = 100        // 0-127 MIDI velocity
    var slide: Bool = false          // Slide to next note
    var condition: StepCondition = .always
    
    var isEmpty: Bool {
        noteEvents.isEmpty && !accent && !tie && ratchetCount <= 1
    }
}

/// Step execution conditions
enum StepCondition: UInt8, CaseIterable {
    case always = 0
    case firstTime
    case notFirstTime  
    case fill
    case notFill
}

/// Real pattern data matching C++ SequencerPattern
class SequencerPattern: ObservableObject {
    // Pattern configuration
    static let maxTracks = 8
    static let maxSteps = 64
    static let defaultSteps = 16
    
    @Published var id: String
    @Published var name: String
    @Published var length: Int = 16              // 1-64 steps
    @Published var steps: [[SequencerStep]]      // [trackIndex][stepIndex]
    @Published var isActive: Bool = false
    @Published var isMuted: Bool = false
    
    // Timing parameters
    @Published var swing: Float = 0.5            // 0.0-1.0 (0.5 = no swing)
    @Published var shuffle: Float = 0.0          // 0.0-1.0 shuffle amount
    @Published var humanize: Float = 0.0         // 0.0-1.0 timing randomization
    @Published var gate: Float = 0.8             // 0.0-1.0 gate length
    
    // Track configuration
    @Published var trackConfigs: [TrackConfig]
    
    init(id: String, name: String = "") {
        self.id = id
        self.name = name.isEmpty ? "Pattern \(id)" : name
        
        // Initialize empty steps for all tracks
        self.steps = Array(repeating: Array(repeating: SequencerStep(), count: Self.defaultSteps), count: Self.maxTracks)
        
        // Initialize track configurations
        self.trackConfigs = (0..<Self.maxTracks).map { trackIndex in
            TrackConfig(
                trackIndex: trackIndex,
                type: trackIndex == 0 ? .monoSynth : .drum,
                instrumentSlot: trackIndex,
                enabled: true
            )
        }
    }
    
    /// Add note event to specific step and track
    func addNoteEvent(track: Int, step: Int, note: NoteEvent) {
        guard isValidPosition(track: track, step: step) else { return }
        steps[track][step].noteEvents.append(note)
    }
    
    /// Remove note event from step
    func removeNoteEvent(track: Int, step: Int, noteIndex: Int) {
        guard isValidPosition(track: track, step: step),
              noteIndex < steps[track][step].noteEvents.count else { return }
        steps[track][step].noteEvents.remove(at: noteIndex)
    }
    
    /// Toggle step (add/remove default note)
    func toggleStep(track: Int, step: Int, pitch: UInt8 = 60, velocity: UInt8 = 100) {
        guard isValidPosition(track: track, step: step) else { return }
        
        if steps[track][step].noteEvents.isEmpty {
            // Add default note
            let note = NoteEvent(
                instrument: UInt32(trackConfigs[track].instrumentSlot),
                pitch: pitch,
                velocity: velocity,
                length: 1
            )
            steps[track][step].noteEvents.append(note)
        } else {
            // Remove all notes
            steps[track][step].noteEvents.removeAll()
        }
    }
    
    /// Set step accent
    func setAccent(track: Int, step: Int, enabled: Bool) {
        guard isValidPosition(track: track, step: step) else { return }
        steps[track][step].accent = enabled
    }
    
    /// Set step tie
    func setTie(track: Int, step: Int, enabled: Bool) {
        guard isValidPosition(track: track, step: step) else { return }
        steps[track][step].tie = enabled
    }
    
    /// Set ratchet count for step
    func setRatchet(track: Int, step: Int, count: UInt8) {
        guard isValidPosition(track: track, step: step) else { return }
        steps[track][step].ratchetCount = max(1, min(8, count))
    }
    
    /// Check if step has any content
    func hasContent(track: Int, step: Int) -> Bool {
        guard isValidPosition(track: track, step: step) else { return false }
        return !steps[track][step].isEmpty
    }
    
    /// Get pattern content level (0.0-1.0)
    func getContentLevel() -> Float {
        let totalSteps = trackConfigs.filter(\.enabled).count * length
        let contentSteps = trackConfigs.enumerated().compactMap { trackIndex, config in
            config.enabled ? steps[trackIndex].prefix(length).filter { !$0.isEmpty }.count : nil
        }.reduce(0, +)
        
        return totalSteps > 0 ? Float(contentSteps) / Float(totalSteps) : 0.0
    }
    
    /// Change pattern length
    func setLength(_ newLength: Int) {
        let clampedLength = max(1, min(Self.maxSteps, newLength))
        
        if clampedLength > length {
            // Extend pattern - add empty steps
            for trackIndex in 0..<Self.maxTracks {
                while steps[trackIndex].count < clampedLength {
                    steps[trackIndex].append(SequencerStep())
                }
            }
        }
        
        length = clampedLength
    }
    
    /// Clear entire pattern
    func clear() {
        for trackIndex in 0..<Self.maxTracks {
            for stepIndex in 0..<steps[trackIndex].count {
                steps[trackIndex][stepIndex] = SequencerStep()
            }
        }
    }
    
    /// Copy pattern data from another pattern
    func copyFrom(_ other: SequencerPattern) {
        self.length = other.length
        self.steps = other.steps.map { track in
            track.map { step in
                // Deep copy step data
                var newStep = SequencerStep()
                newStep.noteEvents = step.noteEvents
                newStep.microTiming = step.microTiming
                newStep.accent = step.accent
                newStep.tie = step.tie
                newStep.ratchetCount = step.ratchetCount
                newStep.probability = step.probability
                newStep.velocity = step.velocity
                newStep.slide = step.slide
                newStep.condition = step.condition
                return newStep
            }
        }
        self.swing = other.swing
        self.shuffle = other.shuffle
        self.humanize = other.humanize
        self.gate = other.gate
    }
    
    private func isValidPosition(track: Int, step: Int) -> Bool {
        return track >= 0 && track < Self.maxTracks && 
               step >= 0 && step < length && step < steps[track].count
    }
}

/// Track configuration matching C++ TrackConfig
struct TrackConfig {
    var trackIndex: Int
    var type: TrackType
    var instrumentSlot: Int          // Which instrument this track uses
    var enabled: Bool = true
    var muted: Bool = false
    var solo: Bool = false
    var level: Float = 0.8           // 0.0-1.0
    var pan: Float = 0.5             // 0.0 (left) to 1.0 (right)
    var transpose: Int8 = 0          // Semitones -24 to +24
    var velocityOffset: Int8 = 0     // Velocity adjustment -64 to +64
}

/// Track types matching C++ enum
enum TrackType: UInt8, CaseIterable {
    case monoSynth = 0
    case polySynth
    case drum  
    case sampler
    case aux
    
    var displayName: String {
        switch self {
        case .monoSynth: return "Mono Synth"
        case .polySynth: return "Poly Synth"
        case .drum: return "Drum"
        case .sampler: return "Sampler"
        case .aux: return "Aux"
        }
    }
}

/// Song section for arrangement
struct SongSection: Identifiable {
    let id = UUID()
    var name: String
    var type: SectionType
    var patternBlocks: [PatternBlock] = []
    var isActive: Bool = false
    var color: SectionColor = .blue
    
    var totalLength: Int {
        patternBlocks.reduce(0) { $0 + $1.length }
    }
}

/// Pattern block in song timeline
struct PatternBlock: Identifiable {
    let id = UUID()
    var patternId: String           // A, B, C, etc.
    var length: Int = 16            // Length in steps
    var repetitions: Int = 1        // How many times to repeat
    var variation: Int = 0          // Pattern variation (0 = original)
    var transpose: Int8 = 0         // Transpose in semitones
    var isActive: Bool = false
    
    var totalSteps: Int {
        length * repetitions
    }
}

/// Song section types
enum SectionType: String, CaseIterable {
    case intro = "Intro"
    case verse = "Verse"
    case chorus = "Chorus"
    case bridge = "Bridge"
    case breakdown = "Breakdown"
    case build = "Build"
    case drop = "Drop"
    case outro = "Outro"
    case custom = "Custom"
}

/// Section colors for visual organization
enum SectionColor: String, CaseIterable {
    case blue = "blue"
    case green = "green"
    case purple = "purple"
    case orange = "orange"
    case red = "red"
    case yellow = "yellow"
    case pink = "pink"
    case gray = "gray"
    
    var color: Color {
        switch self {
        case .blue: return .blue
        case .green: return .green
        case .purple: return .purple
        case .orange: return .orange
        case .red: return .red
        case .yellow: return .yellow
        case .pink: return .pink
        case .gray: return .gray
        }
    }
}