import SwiftUI
import AVFoundation
import Combine

#if os(macOS)
import AppKit
#elseif os(iOS)
import UIKit
#endif

// C++ Bridge Function Declarations
@_silgen_name("ether_create")
func ether_create() -> UnsafeMutableRawPointer?

@_silgen_name("ether_destroy") 
func ether_destroy(_ engine: UnsafeMutableRawPointer)

@_silgen_name("ether_initialize")
func ether_initialize(_ engine: UnsafeMutableRawPointer) -> Int32

@_silgen_name("ether_pattern_create")
func ether_pattern_create(_ engine: UnsafeMutableRawPointer, _ trackIndex: Int32, _ patternID: UnsafePointer<CChar>, _ startPosition: Int32, _ length: Int32)

@_silgen_name("ether_pattern_delete")
func ether_pattern_delete(_ engine: UnsafeMutableRawPointer, _ trackIndex: Int32, _ patternID: UnsafePointer<CChar>)

@_silgen_name("ether_pattern_move")
func ether_pattern_move(_ engine: UnsafeMutableRawPointer, _ trackIndex: Int32, _ patternID: UnsafePointer<CChar>, _ newPosition: Int32)

// Engine Parameter Mapping Functions (for 16-key system)
@_silgen_name("ether_set_parameter_by_key")
func ether_set_parameter_by_key(_ engine: UnsafeMutableRawPointer, _ instrument: Int32, _ keyIndex: Int32, _ value: Float)

@_silgen_name("ether_get_parameter_by_key")
func ether_get_parameter_by_key(_ engine: UnsafeMutableRawPointer, _ instrument: Int32, _ keyIndex: Int32) -> Float

@_silgen_name("ether_get_parameter_name")
func ether_get_parameter_name(_ engine: UnsafeMutableRawPointer, _ engineType: Int32, _ keyIndex: Int32) -> UnsafePointer<CChar>

@_silgen_name("ether_get_parameter_unit")
func ether_get_parameter_unit(_ engine: UnsafeMutableRawPointer, _ engineType: Int32, _ keyIndex: Int32) -> UnsafePointer<CChar>

@_silgen_name("ether_get_parameter_min")
func ether_get_parameter_min(_ engine: UnsafeMutableRawPointer, _ engineType: Int32, _ keyIndex: Int32) -> Float

@_silgen_name("ether_get_parameter_max")
func ether_get_parameter_max(_ engine: UnsafeMutableRawPointer, _ engineType: Int32, _ keyIndex: Int32) -> Float

@_silgen_name("ether_get_parameter_group")
func ether_get_parameter_group(_ engine: UnsafeMutableRawPointer, _ engineType: Int32, _ keyIndex: Int32) -> Int32

@_silgen_name("ether_set_instrument_engine")
func ether_set_instrument_engine(_ engine: UnsafeMutableRawPointer, _ instrument: Int32, _ engineType: Int32)

@_silgen_name("ether_get_instrument_engine")
func ether_get_instrument_engine(_ engine: UnsafeMutableRawPointer, _ instrument: Int32) -> Int32

@_silgen_name("ether_get_engine_count")
func ether_get_engine_count(_ engine: UnsafeMutableRawPointer) -> Int32

@_silgen_name("ether_get_engine_name")
func ether_get_engine_name(_ engine: UnsafeMutableRawPointer, _ engineType: Int32) -> UnsafePointer<CChar>

// LFO Control Functions
@_silgen_name("ether_assign_lfo_to_parameter")
func ether_assign_lfo_to_parameter(_ engine: UnsafeMutableRawPointer, _ instrument: Int32, _ lfoIndex: Int32, _ keyIndex: Int32, _ depth: Float)

@_silgen_name("ether_remove_lfo_assignment")
func ether_remove_lfo_assignment(_ engine: UnsafeMutableRawPointer, _ instrument: Int32, _ lfoIndex: Int32, _ keyIndex: Int32)

@_silgen_name("ether_set_lfo_waveform")
func ether_set_lfo_waveform(_ engine: UnsafeMutableRawPointer, _ instrument: Int32, _ lfoIndex: Int32, _ waveform: Int32)

@_silgen_name("ether_set_lfo_rate")
func ether_set_lfo_rate(_ engine: UnsafeMutableRawPointer, _ instrument: Int32, _ lfoIndex: Int32, _ rate: Float)

@_silgen_name("ether_get_parameter_lfo_info")
func ether_get_parameter_lfo_info(_ engine: UnsafeMutableRawPointer, _ instrument: Int32, _ keyIndex: Int32, _ activeLFOs: UnsafeMutablePointer<Int32>, _ currentValue: UnsafeMutablePointer<Float>) -> Int32

@_silgen_name("ether_apply_lfo_template")
func ether_apply_lfo_template(_ engine: UnsafeMutableRawPointer, _ instrument: Int32, _ templateType: Int32)

// MARK: - UI Constants for 960×320 + 2×16 Layout
struct UIConstants {
    static let screenWidth: CGFloat = 960
    static let screenHeight: CGFloat = 320
    
    // Layout heights
    static let headerHeight: CGFloat = 32
    static let optionLineHeight: CGFloat = 48
    static let stepRowHeight: CGFloat = 56
    static let footerHeight: CGFloat = 28
    
    // Computed view area height
    static let viewAreaHeight: CGFloat = screenHeight - headerHeight - optionLineHeight - stepRowHeight - footerHeight
    
    // Button sizes
    static let optionButtonWidth: CGFloat = 54  // 960 / 16 - gaps
    static let stepButtonWidth: CGFloat = 56
    static let stepButtonHeight: CGFloat = 48
    
    // Colors for option line quads
    static let quadColors = [
        Color(red: 0.94, green: 0.89, blue: 0.95), // Lilac - Pattern/Instrument 
        Color(red: 0.89, green: 0.93, blue: 0.98), // Blue - Tools
        Color(red: 0.99, green: 0.93, blue: 0.89), // Peach - Stamps
        Color(red: 0.89, green: 0.96, blue: 0.93)  // Mint - Performance
    ]
}

// MARK: - Engine Data Models
struct EngineType {
    let id: String
    let name: String
    let cpuClass: String
    let stereo: Bool
    let extras: [String]
    
    static let allEngines = [
        // Real engines from C++ backend
        EngineType(id: "MacroVA", name: "MacroVA", cpuClass: "Medium", stereo: true, extras: ["OSC_MIX", "TONE", "WARMTH", "SUB"]),
        EngineType(id: "MacroFM", name: "MacroFM", cpuClass: "Medium", stereo: true, extras: ["ALGO", "RATIO", "INDEX", "FEEDBACK"]),
        EngineType(id: "MacroWT", name: "MacroWT", cpuClass: "Medium", stereo: true, extras: ["WAVE", "UNISON", "SPREAD", "SYNC"]),
        EngineType(id: "MacroWS", name: "MacroWS", cpuClass: "Light", stereo: true, extras: ["SYMMETRY", "DRIVE", "FOLD", "BIAS"]),
        EngineType(id: "MacroChord", name: "MacroChord", cpuClass: "Medium", stereo: true, extras: ["CHORD", "SPREAD", "VOICE", "DETUNE"]),
        EngineType(id: "MacroHarm", name: "MacroHarm", cpuClass: "Medium", stereo: true, extras: ["PARTIALS", "EVEN/ODD", "SKEW", "WARP"]),
        EngineType(id: "FormantVocal", name: "Vocal", cpuClass: "Medium", stereo: true, extras: ["VOWEL", "FORMANT", "BREATH", "NOISE"]),
        EngineType(id: "NoiseParticles", name: "Noise", cpuClass: "Light", stereo: true, extras: ["COLOR", "DENSITY", "HP", "LP"]),
        EngineType(id: "TidesOsc", name: "Tides", cpuClass: "Medium", stereo: true, extras: ["SLOPE", "SMOOTH", "SHAPE", "RATE"]),
        EngineType(id: "RingsVoice", name: "Rings", cpuClass: "Medium", stereo: true, extras: ["EXCITER", "DECAY", "DAMP", "BRIGHT"]),
        EngineType(id: "ElementsVoice", name: "Elements", cpuClass: "Heavy", stereo: true, extras: ["EXCITER", "MATERIAL", "SPACE", "BRIGHT"]),
        EngineType(id: "DrumKit", name: "DrumKit", cpuClass: "Heavy", stereo: false, extras: ["ACCENT", "HUMANIZE", "SEED", "VARIATION"]),
        EngineType(id: "SamplerKit", name: "SamplerKit", cpuClass: "Medium", stereo: false, extras: ["START", "LOOP", "PITCH", "FILTER"]),
        EngineType(id: "SamplerSlicer", name: "Slicer", cpuClass: "Medium", stereo: true, extras: ["SLICE", "PITCH", "START", "FILTER"])
    ]
    
    var color: Color {
        let palette: [String: Color] = [
            "MacroVA": Color(red: 0.82, green: 0.68, blue: 0.62),      // Coral
            "MacroFM": Color(red: 0.99, green: 0.55, blue: 0.38),      // Orange  
            "MacroWT": Color(red: 0.55, green: 0.63, blue: 0.80),      // Blue
            "MacroWS": Color(red: 0.98, green: 0.60, blue: 0.60),      // Pink
            "MacroChord": Color(red: 0.65, green: 0.81, blue: 0.89),   // Light Blue
            "MacroHarm": Color(red: 0.89, green: 0.78, blue: 0.74),    // Peach
            "FormantVocal": Color(red: 0.91, green: 0.54, blue: 0.76), // Purple
            "NoiseParticles": Color(red: 0.54, green: 0.54, blue: 0.54), // Slate
            "TidesOsc": Color(red: 0.75, green: 0.73, blue: 0.85),     // Lavender
            "RingsVoice": Color(red: 0.74, green: 0.81, blue: 0.76),   // Sage
            "ElementsVoice": Color(red: 0.65, green: 0.75, blue: 0.73), // Teal
            "DrumKit": Color(red: 1.0, green: 0.85, blue: 0.18),       // Yellow
            "SamplerKit": Color(red: 0.93, green: 0.91, blue: 0.88),   // Cream
            "SamplerSlicer": Color(red: 0.91, green: 0.90, blue: 0.87) // Pearl
        ]
        return palette[id] ?? Color.gray
    }
}

struct PatternBlock: Identifiable, Equatable {
    let id = UUID()
    var startPosition: Int // Position in steps (0-based)
    var length: Int = 16 // Length in steps
    var patternID: String = "A" // A, B, C, etc.
    var notes: [SequenceNote] = [] // For pitched patterns
    var drumHits: [DrumHit] = [] // For drum patterns
    
    var endPosition: Int {
        return startPosition + length
    }
}

struct SequenceNote: Equatable {
    var step: Int
    var note: Int // MIDI note number
    var velocity: Float = 0.8
    var probability: Float = 1.0
    var ratchet: Int = 1
    var microTune: Float = 0.0
    var tie: Bool = false
    var slide: Bool = false
}

struct DrumHit: Equatable {
    var step: Int
    var lane: Int // Which drum lane (0-15)
    var velocity: Float = 0.8
    var probability: Float = 1.0
    var ratchet: Int = 1
    var variation: Int = 0 // Which sample variation (0-3)
}

struct Track {
    var engine: EngineType?
    var preset: String = "—"
    var polyphony: Int = 6
    var patterns: [PatternBlock] = []
    var isMuted: Bool = false
    var isSoloed: Bool = false
    var volume: Float = 0.8
    var pan: Float = 0.0
}

// MARK: - App State Management
// MARK: - Tool System
enum Tool: String {
    case copy, cut, delete, nudge
}

// MARK: - Stamp System
struct StampState {
    var accent: Bool = false
    var ratchet: Int = 0  // 0-3 for no ratch, 2x, 3x, 4x
    var tie: Bool = false
    var micro: Int = 0    // -1, 0, 1 for micro-, off, micro+
}

// MARK: - UI Mode System
enum UIMode: String {
    case step, pad, fx, mix, seq, chop
}

// MARK: - Parameter System
struct ParameterState {
    var isParameterMode: Bool = false
    var selectedParameterIndex: Int = 0
    var parameterValues: [Float] = Array(repeating: 0.5, count: 16)
    var parameterNames: [String] = Array(repeating: "PARAM", count: 16)
}

// MARK: - LFO System
struct LFOState {
    var activeLFOs: [Bool] = Array(repeating: false, count: 4)  // LFO1-4
    var lfoRates: [Float] = [0.5, 4.0, 2.0, 1.0]              // Default rates
    var lfoDepths: [Float] = [0.3, 0.15, 0.2, 0.4]            // Default depths
    var lfoWaveforms: [Int] = [0, 1, 0, 7]                     // SINE, TRI, SINE, S&H
    var lfoSynced: [Bool] = [false, false, false, false]       // Free running by default
    
    // Parameter LFO assignments (parameter index -> active LFO bitmask)
    var parameterLFOAssignments: [Int] = Array(repeating: 0, count: 16)
    var parameterLFOValues: [Float] = Array(repeating: 0.0, count: 16) // Current LFO values
    
    var showLFOInterface: Bool = false
    var selectedLFO: Int = 0  // Currently selected LFO (0-3)
}

// MARK: - Scope System
enum Scope: String {
    case step, instr, pattern
}

// MARK: - Performance Macros System
struct PerformanceMacro: Identifiable {
    let id: UInt32
    var name: String
    var type: MacroType
    var triggerMode: TriggerMode
    var keyIndex: Int = -1
    var requiresShift: Bool = false
    var requiresAlt: Bool = false
    var isActive: Bool = false
    var progress: Float = 0.0
    var color: UInt32 = 0xFF6B73
    var category: String = "General"
    
    enum MacroType: Int, CaseIterable {
        case parameterSet = 0, patternTrigger, effectChain, sceneMoreph, 
             filterSweep, volumeFade, tempoRamp, harmonyStack, 
             rhythmFill, loopCapture, custom
        
        var name: String {
            switch self {
            case .parameterSet: return "Parameter Set"
            case .patternTrigger: return "Pattern Trigger"
            case .effectChain: return "Effect Chain"
            case .sceneMoreph: return "Scene Morph"
            case .filterSweep: return "Filter Sweep"
            case .volumeFade: return "Volume Fade"
            case .tempoRamp: return "Tempo Ramp"
            case .harmonyStack: return "Harmony Stack"
            case .rhythmFill: return "Rhythm Fill"
            case .loopCapture: return "Loop Capture"
            case .custom: return "Custom"
            }
        }
    }
    
    enum TriggerMode: Int, CaseIterable {
        case immediate = 0, quantized, hold, toggle, timed
        
        var name: String {
            switch self {
            case .immediate: return "Immediate"
            case .quantized: return "Quantized"
            case .hold: return "Hold"
            case .toggle: return "Toggle"
            case .timed: return "Timed"
            }
        }
    }
}

struct LiveLoop: Identifiable {
    let id: UInt32
    var name: String
    var isRecording: Bool = false
    var isPlaying: Bool = false
    var recordingTrack: Int
    var targetTrack: Int = -1
    var loopLength: Float = 4.0
    var volume: Float = 1.0
}

struct PerformanceStats {
    var macrosExecuted: Int = 0
    var scenesRecalled: Int = 0
    var averageRecallTime: Float = 0.0
    var keyPressesPerMinute: Int = 0
}

// MARK: - Euclidean Sequencer System
struct EuclideanPattern: Identifiable {
    let id = UUID()
    var trackIndex: Int
    var pulses: Int = 4
    var rotation: Int = 0
    var probability: Float = 1.0
    var swing: Float = 0.0
    var humanization: Float = 0.0
    var isEnabled: Bool = true
    var patternSteps: [Bool] = Array(repeating: false, count: 16)
    var velocities: [Float] = Array(repeating: 0.7, count: 16)
    
    var density: Float {
        return Float(pulses) / 16.0
    }
    
    var complexity: Int {
        return pulses + abs(rotation) + Int(humanization * 10)
    }
}

class EtherSynthState: ObservableObject {
    @Published var tracks: [Track] = Array(repeating: Track(), count: 8)
    @Published var currentView: String = "timeline"
    @Published var currentTrack: Int?
    @Published var showingOverlay: Bool = false
    
    // Effects state
    @Published var activeEffects: [UInt32] = []        // Active effect IDs
    @Published var currentEffectID: UInt32 = 0         // Currently selected effect
    @Published var effectParameters: [String: Float] = [:]  // Effect parameter values
    @Published var effectPresets: [String] = ["Classic", "Warm", "Bright", "Spacey", "Vintage", "Modern"]
    @Published var selectedEffectPreset: Int = 0
    
    // Performance effects state
    @Published var reverbThrowActive: Bool = false
    @Published var delayThrowActive: Bool = false
    @Published var performanceFilterType: Int = 0      // 0=LP, 1=HP, 2=BP, 3=Notch
    @Published var performanceFilterCutoff: Float = 1000.0
    @Published var performanceFilterResonance: Float = 0.0
    @Published var noteRepeatActive: Bool = false
    @Published var noteRepeatDivision: Int = 4
    @Published var reverbSend: Float = 0.0
    @Published var delaySend: Float = 0.0
    
    // Pattern chain state
    @Published var currentPatterns: [UInt32] = Array(repeating: 0, count: 8)  // Current pattern per track
    @Published var queuedPatterns: [UInt32] = Array(repeating: 0, count: 8)   // Queued pattern per track
    @Published var armedPatterns: [UInt32] = Array(repeating: 0, count: 8)    // Armed pattern per track
    @Published var chainModes: [Int] = Array(repeating: 0, count: 8)          // Chain mode per track (0=Manual, 1=Auto, etc.)
    @Published var performanceMode: Bool = false
    @Published var globalQuantization: Int = 1                                // Quantization in bars
    
    // AI Generative Sequencer state
    @Published var generationMode: Int = 0                                    // 0=ASSIST, 1=GENERATE, 2=EVOLVE, etc.
    @Published var musicalStyle: Int = 0                                      // 0=ELECTRONIC, 1=TECHNO, 2=HOUSE, etc.
    @Published var generationComplexity: Int = 1                             // 0=SIMPLE, 1=MODERATE, 2=COMPLEX, 3=ADAPTIVE
    @Published var generationDensity: Float = 0.5                            // Note density (0-1)
    @Published var generationTension: Float = 0.5                            // Harmonic tension (0-1)
    @Published var generationCreativity: Float = 0.5                         // Creativity vs. predictability (0-1)
    @Published var generationResponsiveness: Float = 0.7                     // Responsiveness to user (0-1)
    @Published var adaptiveMode: Bool = false                                 // AI adaptive learning mode
    @Published var generativeSuggestions: [UInt32] = []                      // AI pattern suggestions
    @Published var detectedScale: (root: Int, type: Int, confidence: Float) = (0, 0, 0.0) // Detected musical scale
    @Published var patternComplexities: [UInt32: Float] = [:]                 // Pattern complexity cache
    @Published var patternInterests: [UInt32: Float] = [:]                    // Pattern interest cache
    
    // Performance Macros state
    @Published var availableMacros: [PerformanceMacro] = []                   // Available performance macros
    @Published var activeMacros: Set<UInt32> = []                            // Currently executing macros
    @Published var macroKeyBindings: [Int: UInt32] = [:]                      // Key index -> macro ID
    @Published var isPerformanceModeEnabled: Bool = false                     // Performance mode active
    @Published var liveLoops: [LiveLoop] = []                                // Active live loops
    @Published var recordingLoop: UInt32? = nil                              // Currently recording loop ID
    @Published var performanceStats: PerformanceStats = PerformanceStats()   // Performance statistics
    
    // Euclidean Sequencer state
    @Published var euclideanPatterns: [EuclideanPattern] = []                 // Per-track euclidean patterns
    @Published var euclideanEnabled: Bool = false                            // Global euclidean enable
    @Published var euclideanPresets: [String] = []                           // Available presets
    @Published var selectedEuclideanPreset: String = ""                      // Currently selected preset
    
    // Enhanced Chord System state (Bicep Mode)
    @Published var chordSystemEnabled: Bool = false                          // Global chord system enable
    @Published var currentChordType: Int = 0                                 // 0=MAJOR, 1=MINOR, etc. (32 types total)
    @Published var currentChordRoot: Int = 60                                // MIDI note number (Middle C)
    @Published var chordVoiceEngines: [Int] = [0, 0, 0, 0, 0]               // Engine type per voice (5 voices max)
    @Published var chordVoiceRoles: [Int] = [0, 1, 2, 3, 4]                 // Voice roles: ROOT, HARMONY_1, HARMONY_2, COLOR, DOUBLING
    @Published var chordVoiceLevels: [Float] = [1.0, 0.8, 0.8, 0.6, 0.4]   // Mix level per voice
    @Published var chordVoicePans: [Float] = [0.0, -0.3, 0.3, -0.6, 0.6]   // Stereo position per voice
    @Published var chordVoiceActive: [Bool] = [true, true, true, false, false] // Voice enable state
    @Published var chordInstrumentAssignments: [[Int]] = Array(repeating: [], count: 8) // Which voices each instrument plays
    @Published var chordStrumOffsets: [Float] = Array(repeating: 0.0, count: 8) // Strum timing per instrument (ms)
    @Published var chordArpeggioModes: [Bool] = Array(repeating: false, count: 8) // Arpeggio enable per instrument
    @Published var chordArpeggioRates: [Float] = Array(repeating: 8.0, count: 8) // Arpeggio rate per instrument
    @Published var chordSpread: Float = 12.0                                 // Voice spread in semitones
    @Published var chordStrumTime: Float = 0.0                               // Global strum time (0-100ms)
    @Published var chordStrumUp: Bool = true                                 // Strum direction
    @Published var chordHumanization: Float = 0.0                           // Timing humanization
    @Published var chordVoiceLeadingEnabled: Bool = true                     // Voice leading enable
    @Published var chordVoiceLeadingStrength: Float = 0.8                    // Voice leading strength
    @Published var chordArrangementPresets: [String] = ["Bicep House", "Bicep Ambient", "Bicep Jazz", "Modern Pop", "Vintage Keys", "Orchestral Pad", "Trap Chords"]
    @Published var selectedChordPreset: Int = 0                              // Currently selected arrangement preset
    @Published var chordCPUUsage: Float = 0.0                                // Chord system CPU usage
    @Published var chordActiveVoices: Int = 0                                // Active chord voices count
    
    // Scene management
    @Published var availableScenes: [String] = ["Scene A", "Scene B", "Scene C", "Scene D"]
    @Published var selectedSceneId: UInt32 = 0
    
    // Arrangement mode
    @Published var arrangementMode: Bool = false
    @Published var currentSection: Int = 0
    @Published var sectionNames: [String] = ["Intro", "Verse", "Chorus", "Bridge", "Outro"]
    
    @Published var overlayType: OverlayType?
    @Published var isPlaying: Bool = false
    @Published var isRecording: Bool = false
    @Published var bpm: Float = 120.0
    @Published var swing: Float = 54.0
    @Published var currentBar: String = "05:03"
    @Published var cpuUsage: String = "▓▓▓░"
    @Published var projectName: String = "Engine UI v2"
    @Published var currentTool: Tool?
    @Published var stamps = StampState()
    @Published var currentMode: UIMode = .step
    @Published var instButtonHeld: Bool = false
    @Published var parameterState = ParameterState()
    @Published var lfoState = LFOState()
    @Published var armedInstrumentIndex: Int = 0
    @Published var currentScope: Scope = .step
    @Published var shiftButtonPressed: Bool = false
    @Published var currentPatternIndex: Int = 0
    
    // Pattern management
    @Published var selectedPatternID: UUID?
    @Published var copiedPattern: PatternBlock?
    private var patternIDCounter = 0
    
    // C++ Engine
    private var synthEngine: UnsafeMutableRawPointer?
    
    init() {
        setupEngine()
        initializeChordSystem()
    }
    
    deinit {
        if let engine = synthEngine {
            ether_destroy(engine)
        }
    }
    
    // MARK: - New 2×16 UI Functions
    
    func triggerOptionButton(_ index: Int) {
        switch index {
        case 0: // INST
            holdInstButton()
            break
        case 1: // SOUND
            // Open instrument selection overlay
            break
        case 2: // PATTERN
            // Open pattern selection overlay  
            break
        case 3: // CLONE
            openCloneOverlay()
            break
        case 4: // COPY
            currentTool = .copy
            break
        case 5: // CUT
            currentTool = .cut
            break
        case 6: // DELETE
            currentTool = .delete
            break
        case 7: // NUDGE
            currentTool = .nudge
            break
        case 8: // ACCENT
            stamps.accent.toggle()
            break
        case 9: // RATCHET
            stamps.ratchet = (stamps.ratchet + 1) % 4
            break
        case 10: // TIE
            stamps.tie.toggle()
            break
        case 11: // MICRO
            stamps.micro = stamps.micro == 0 ? 1 : (stamps.micro == 1 ? -1 : 0)
            break
        case 12: // FX
            openFXOverlay()
            break
        case 13: // CHORD
            openChordOverlay()
            break
        case 14: // SWING
            // Adjust swing
            break
        case 15: // SCENE
            openSceneOverlay()
            break
        default:
            break
        }
    }
    
    // MARK: - Overlay Management
    func openSoundOverlay() {
        overlayType = .sound(trackIndex: armedInstrumentIndex)
        showingOverlay = true
    }
    
    func openPatternOverlay() {
        overlayType = .pattern(trackIndex: armedInstrumentIndex)
        showingOverlay = true
    }
    
    func openCloneOverlay() {
        overlayType = .clone
        showingOverlay = true
    }
    
    func openFXOverlay() {
        overlayType = .fx
        showingOverlay = true
    }
    
    func openSceneOverlay() {
        overlayType = .scene
        showingOverlay = true
    }
    
    func openEngineOverlay() {
        overlayType = .engine
        showingOverlay = true
    }
    
    func openChordOverlay() {
        overlayType = .chord
        showingOverlay = true
    }
    
    func closeOverlay() {
        showingOverlay = false
        overlayType = nil
    }
    
    func holdInstButton() {
        instButtonHeld = true
        parameterState.isParameterMode = true
        updateParameterNamesForCurrentEngine()
    }
    
    func releaseInstButton() {
        instButtonHeld = false
        parameterState.isParameterMode = false
    }
    
    func setParameterValue(_ parameterIndex: Int, _ value: Float) {
        guard parameterIndex < parameterState.parameterValues.count else { return }
        let clampedValue = max(0.0, min(1.0, value))
        parameterState.parameterValues[parameterIndex] = clampedValue
        
        // Send to C++ engine using real bridge
        if let engine = synthEngine {
            ether_set_parameter_by_key(engine, Int32(armedInstrumentIndex), Int32(parameterIndex), clampedValue)
            print("✅ C++ Bridge: Set I\(armedInstrumentIndex) key \(parameterIndex) = \(clampedValue)")
        }
    }
    
    func getParameterValue(_ parameterIndex: Int) -> Float {
        guard parameterIndex < parameterState.parameterValues.count else { return 0.5 }
        
        // Get real value from C++ engine
        if let engine = synthEngine {
            let realValue = ether_get_parameter_by_key(engine, Int32(armedInstrumentIndex), Int32(parameterIndex))
            parameterState.parameterValues[parameterIndex] = realValue
            return realValue
        }
        
        return parameterState.parameterValues[parameterIndex]
    }
    
    func getCurrentEngineID() -> String {
        return tracks[armedInstrumentIndex].engine?.id ?? "None"
    }
    
    func selectParameter(_ index: Int) {
        parameterState.selectedParameterIndex = index
    }
    
    func updateParameterNamesForCurrentEngine() {
        let currentEngine = tracks[armedInstrumentIndex].engine
        guard let engineID = currentEngine?.id else { return }
        
        // Get real parameter names from C++ engine
        if let engine = synthEngine {
            let engineTypeId = getEngineTypeId(from: engineID)
            var paramNames: [String] = []
            
            for keyIndex in 0..<16 {
                let namePointer = ether_get_parameter_name(engine, Int32(engineTypeId), Int32(keyIndex))
                let paramName = String(cString: namePointer)
                paramNames.append(paramName)
            }
            
            parameterState.parameterNames = paramNames
            print("✅ Updated parameter names for \(engineID): \(paramNames)")
        } else {
            // Fallback to generic names
            parameterState.parameterNames = generateParameterNames(for: engineID)
        }
    }
    
    private func getEngineTypeId(from engineID: String) -> Int {
        // Map engine IDs to engine type indices (matches C++ EngineType enum)
        switch engineID {
        case "MacroVA": return 0
        case "MacroFM": return 1
        case "MacroWS": return 2
        case "MacroWT": return 3
        case "MacroChord": return 4
        case "MacroHarm": return 5
        case "FormantVocal": return 6
        case "NoiseParticles": return 7
        case "TidesOsc": return 8
        case "RingsVoice": return 9
        case "ElementsVoice": return 10
        case "DrumKit": return 11
        case "SamplerKit": return 12
        case "SamplerSlicer": return 13
        default: return 0 // Default to MacroVA
        }
    }
    
    // MARK: - LFO Control Functions
    func toggleLFOInterface() {
        lfoState.showLFOInterface.toggle()
        print("LFO Interface: \(lfoState.showLFOInterface ? "SHOWN" : "HIDDEN")")
    }
    
    func selectLFO(_ index: Int) {
        guard index >= 0 && index < 4 else { return }
        lfoState.selectedLFO = index
        print("Selected LFO\(index + 1)")
    }
    
    func assignLFOToCurrentParameter(_ lfoIndex: Int, depth: Float = 0.3) {
        guard let engine = synthEngine else { return }
        guard lfoIndex >= 0 && lfoIndex < 4 else { return }
        
        let paramIndex = parameterState.selectedParameterIndex
        
        // Update local state
        lfoState.parameterLFOAssignments[paramIndex] |= (1 << lfoIndex)
        lfoState.activeLFOs[lfoIndex] = true
        
        // Send to C++ bridge
        ether_assign_lfo_to_parameter(engine, Int32(armedInstrumentIndex), Int32(lfoIndex), Int32(paramIndex), depth)
        
        print("✅ LFO\(lfoIndex + 1) assigned to I\(armedInstrumentIndex) parameter \(paramIndex) (depth: \(depth))")
    }
    
    func removeLFOFromCurrentParameter(_ lfoIndex: Int) {
        guard let engine = synthEngine else { return }
        guard lfoIndex >= 0 && lfoIndex < 4 else { return }
        
        let paramIndex = parameterState.selectedParameterIndex
        
        // Update local state
        lfoState.parameterLFOAssignments[paramIndex] &= ~(1 << lfoIndex)
        
        // Check if this LFO is used anywhere else
        var lfoUsed = false
        for assignments in lfoState.parameterLFOAssignments {
            if assignments & (1 << lfoIndex) != 0 {
                lfoUsed = true
                break
            }
        }
        lfoState.activeLFOs[lfoIndex] = lfoUsed
        
        // Send to C++ bridge
        ether_remove_lfo_assignment(engine, Int32(armedInstrumentIndex), Int32(lfoIndex), Int32(paramIndex))
        
        print("❌ LFO\(lfoIndex + 1) removed from I\(armedInstrumentIndex) parameter \(paramIndex)")
    }
    
    func setLFOWaveform(_ lfoIndex: Int, waveform: Int) {
        guard let engine = synthEngine else { return }
        guard lfoIndex >= 0 && lfoIndex < 4 else { return }
        
        lfoState.lfoWaveforms[lfoIndex] = waveform
        ether_set_lfo_waveform(engine, Int32(armedInstrumentIndex), Int32(lfoIndex), Int32(waveform))
        
        let waveformNames = ["SINE", "TRI", "SAW↗", "SAW↘", "SQR", "PLS", "NOISE", "S&H", "EXP↗", "EXP↘", "LOG", "CUSTOM"]
        let wfName = waveform < waveformNames.count ? waveformNames[waveform] : "UNK"
        print("🌊 LFO\(lfoIndex + 1) waveform: \(wfName)")
    }
    
    func setLFORate(_ lfoIndex: Int, rate: Float) {
        guard let engine = synthEngine else { return }
        guard lfoIndex >= 0 && lfoIndex < 4 else { return }
        
        lfoState.lfoRates[lfoIndex] = rate
        ether_set_lfo_rate(engine, Int32(armedInstrumentIndex), Int32(lfoIndex), rate)
        
        print("📈 LFO\(lfoIndex + 1) rate: \(rate)Hz")
    }
    
    func applyLFOTemplate(_ templateType: Int) {
        guard let engine = synthEngine else { return }
        
        ether_apply_lfo_template(engine, Int32(armedInstrumentIndex), Int32(templateType))
        
        // Update local state based on template type
        switch templateType {
        case 0: // BASIC
            lfoState.parameterLFOAssignments[4] = 1  // LFO1 → Cutoff
            lfoState.parameterLFOAssignments[1] = 2  // LFO2 → Timbre
            lfoState.parameterLFOAssignments[14] = 4 // LFO3 → Volume
            lfoState.activeLFOs = [true, true, true, false]
        case 3: // MACRO_VA
            lfoState.parameterLFOAssignments[4] = 1   // LFO1 → Cutoff
            lfoState.parameterLFOAssignments[0] = 2   // LFO2 → OSC Mix
            lfoState.parameterLFOAssignments[5] = 4   // LFO3 → Resonance
            lfoState.parameterLFOAssignments[2] = 8   // LFO4 → Detune
            lfoState.activeLFOs = [true, true, true, true]
        default:
            break
        }
        
        let templateNames = ["BASIC", "PERFORMANCE", "EXPERIMENTAL", "MACRO_VA", "MACRO_FM", "DRUM_KIT"]
        let templateName = templateType < templateNames.count ? templateNames[templateType] : "UNKNOWN"
        print("🎛️ Applied \(templateName) LFO template to I\(armedInstrumentIndex)")
    }
    
    func updateLFOValues() {
        // Get real-time LFO values for visual feedback
        guard let engine = synthEngine else { return }
        
        for paramIndex in 0..<16 {
            if lfoState.parameterLFOAssignments[paramIndex] != 0 {
                var activeLFOs: Int32 = 0
                var currentValue: Float = 0.0
                
                let hasLFO = ether_get_parameter_lfo_info(engine, Int32(armedInstrumentIndex), Int32(paramIndex), &activeLFOs, &currentValue)
                
                if hasLFO != 0 {
                    lfoState.parameterLFOValues[paramIndex] = currentValue
                }
            }
        }
    }
    
    private func generateParameterNames(for engineID: String) -> [String] {
        switch engineID {
        case "MacroVA":
            return ["OSC_MIX", "TONE", "WARMTH", "SUB_LVL", "DETUNE", "ATTACK", "DECAY", "SUSTAIN", "RELEASE", "LFO_RATE", "LFO_DEPTH", "MOVEMENT", "SPACE", "CHARACTER", "MACRO_1", "MACRO_2"]
        case "MacroFM":
            return ["ALGORITHM", "RATIO", "INDEX", "FEEDBACK", "BRIGHTNESS", "RESONANCE", "MOD_ENV", "CAR_ENV", "ATTACK", "DECAY", "SUSTAIN", "RELEASE", "LFO_RATE", "LFO_AMT", "DRIVE", "MACRO_FM"]
        default:
            return Array(1...16).map { "PARAM_\($0)" }
        }
    }
    
    func exitTool() {
        currentTool = nil
        currentScope = .step
    }
    
    func cycleScopeUp() {
        guard currentTool != nil else { return }
        switch currentScope {
        case .step:
            currentScope = .instr
        case .instr:
            currentScope = .pattern
        case .pattern:
            currentScope = .step
        }
    }
    
    private func setupEngine() {
        synthEngine = ether_create()
        if let engine = synthEngine {
            let result = ether_initialize(engine)
            if result != 1 {
                print("Failed to initialize EtherSynth engine")
            } else {
                print("EtherSynth engine initialized successfully")
                // Load factory macros
                loadFactoryMacros()
            }
        }
    }
    
    enum OverlayType {
        case assign(trackIndex: Int)
        case sound(trackIndex: Int)
        case engine
        case pattern(trackIndex: Int)
        case clone
        case fx
        case scene
        case chord
    }
    
    func openAssignOverlay(for trackIndex: Int) {
        currentTrack = trackIndex
        overlayType = .assign(trackIndex: trackIndex)
        showingOverlay = true
    }
    
    func openSoundOverlay(for trackIndex: Int) {
        guard tracks[trackIndex].engine != nil else {
            openAssignOverlay(for: trackIndex)
            return
        }
        currentTrack = trackIndex
        overlayType = .sound(trackIndex: trackIndex)
        showingOverlay = true
    }
    
    func openPatternOverlay(for trackIndex: Int) {
        guard tracks[trackIndex].engine != nil else {
            openAssignOverlay(for: trackIndex)
            return
        }
        currentTrack = trackIndex
        overlayType = .pattern(trackIndex: trackIndex)
        showingOverlay = true
    }
    
    func assignEngine(_ engine: EngineType, to trackIndex: Int) {
        tracks[trackIndex].engine = engine
        tracks[trackIndex].preset = "\(engine.name) Default"
        tracks[trackIndex].polyphony = engine.id == "DrumKit" ? 1 : 6
        showingOverlay = false
        overlayType = nil
        
        // Auto-open sound overlay after assignment
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
            self.openSoundOverlay(for: trackIndex)
        }
    }
    
    func switchView(to view: String) {
        currentView = view
    }
    
    func togglePlay() {
        isPlaying.toggle()
    }
    
    func toggleRecord() {
        isRecording.toggle()
    }
    
    // MARK: - Pattern Management Functions
    
    func createNewPattern(on trackIndex: Int, at position: Int) {
        guard trackIndex < tracks.count else { return }
        guard tracks[trackIndex].engine != nil else { return } // Can't create patterns on unassigned tracks
        
        // Find next available pattern letter
        let existingIDs = tracks[trackIndex].patterns.map { $0.patternID }
        let nextID = getNextPatternID(existing: existingIDs)
        
        // Check for overlaps and find a safe position
        let safePosition = findSafePosition(trackIndex: trackIndex, preferredPosition: position, length: 16)
        
        let newPattern = PatternBlock(
            startPosition: safePosition,
            length: 16,
            patternID: nextID
        )
        
        tracks[trackIndex].patterns.append(newPattern)
        selectedPatternID = newPattern.id
        
        // Notify C++ engine about new pattern
        notifyEnginePatternCreated(trackIndex: trackIndex, pattern: newPattern)
    }
    
    func deletePattern(trackIndex: Int, patternID: UUID) {
        guard trackIndex < tracks.count else { return }
        
        if let index = tracks[trackIndex].patterns.firstIndex(where: { $0.id == patternID }) {
            let pattern = tracks[trackIndex].patterns[index]
            tracks[trackIndex].patterns.remove(at: index)
            
            // Clear selection if this was the selected pattern
            if selectedPatternID == patternID {
                selectedPatternID = nil
            }
            
            // Notify C++ engine about pattern deletion
            notifyEnginePatternDeleted(trackIndex: trackIndex, pattern: pattern)
        }
    }
    
    func copyPattern(_ pattern: PatternBlock) {
        copiedPattern = pattern
    }
    
    func pastePattern(on trackIndex: Int, at position: Int) {
        guard let pattern = copiedPattern else { return }
        guard trackIndex < tracks.count else { return }
        guard tracks[trackIndex].engine != nil else { return }
        
        // Find safe position for pasted pattern
        let safePosition = findSafePosition(trackIndex: trackIndex, preferredPosition: position, length: pattern.length)
        
        // Create new pattern with copied data but new ID
        let existingIDs = tracks[trackIndex].patterns.map { $0.patternID }
        let nextID = getNextPatternID(existing: existingIDs)
        
        var newPattern = pattern
        newPattern.startPosition = safePosition
        newPattern.patternID = nextID
        
        tracks[trackIndex].patterns.append(newPattern)
        selectedPatternID = newPattern.id
        
        // Notify C++ engine about new pattern
        notifyEnginePatternCreated(trackIndex: trackIndex, pattern: newPattern)
    }
    
    func clonePattern(trackIndex: Int, patternID: UUID, variation: String = "") {
        guard trackIndex < tracks.count else { return }
        guard let originalIndex = tracks[trackIndex].patterns.firstIndex(where: { $0.id == patternID }) else { return }
        
        let original = tracks[trackIndex].patterns[originalIndex]
        let existingIDs = tracks[trackIndex].patterns.map { $0.patternID }
        
        // Create variation ID (e.g., A2, B2, etc.)
        let baseID = original.patternID
        let nextVariation = variation.isEmpty ? getNextVariationID(baseID: baseID, existing: existingIDs) : variation
        
        // Place clone right after original
        let clonePosition = findSafePosition(trackIndex: trackIndex, preferredPosition: original.endPosition + 4, length: original.length)
        
        var clone = original
        clone.startPosition = clonePosition
        clone.patternID = nextVariation
        
        tracks[trackIndex].patterns.append(clone)
        selectedPatternID = clone.id
        
        // Notify C++ engine about cloned pattern
        notifyEnginePatternCreated(trackIndex: trackIndex, pattern: clone)
    }
    
    func movePattern(trackIndex: Int, patternID: UUID, to newPosition: Int) {
        guard trackIndex < tracks.count else { return }
        guard let index = tracks[trackIndex].patterns.firstIndex(where: { $0.id == patternID }) else { return }
        
        var pattern = tracks[trackIndex].patterns[index]
        let safePosition = findSafePosition(trackIndex: trackIndex, preferredPosition: newPosition, length: pattern.length, excludePatternID: patternID)
        
        pattern.startPosition = safePosition
        tracks[trackIndex].patterns[index] = pattern
        
        // Notify C++ engine about pattern move
        notifyEnginePatternMoved(trackIndex: trackIndex, pattern: pattern)
    }
    
    // MARK: - Helper Functions
    
    private func getNextPatternID(existing: [String]) -> String {
        let letters = ["A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P"]
        for letter in letters {
            if !existing.contains(letter) {
                return letter
            }
        }
        return "A\(patternIDCounter)" // Fallback for >16 patterns
    }
    
    private func getNextVariationID(baseID: String, existing: [String]) -> String {
        var counter = 2
        while existing.contains("\(baseID)\(counter)") {
            counter += 1
        }
        return "\(baseID)\(counter)"
    }
    
    private func findSafePosition(trackIndex: Int, preferredPosition: Int, length: Int, excludePatternID: UUID? = nil) -> Int {
        let patterns = tracks[trackIndex].patterns.filter { $0.id != excludePatternID }
        var position = max(0, preferredPosition)
        
        // Snap to grid (4-step increments)
        position = (position / 4) * 4
        
        // Check for overlaps and find next available position
        while hasOverlap(position: position, length: length, with: patterns) {
            position += 4
        }
        
        return position
    }
    
    private func hasOverlap(position: Int, length: Int, with patterns: [PatternBlock]) -> Bool {
        let endPosition = position + length
        return patterns.contains { pattern in
            !(endPosition <= pattern.startPosition || position >= pattern.endPosition)
        }
    }
    
    // MARK: - C++ Engine Integration
    
    private func notifyEnginePatternCreated(trackIndex: Int, pattern: PatternBlock) {
        guard let synthEngine = synthEngine else { return }
        pattern.patternID.withCString { patternIDCString in
            ether_pattern_create(synthEngine, Int32(trackIndex), patternIDCString, Int32(pattern.startPosition), Int32(pattern.length))
        }
    }
    
    private func notifyEnginePatternDeleted(trackIndex: Int, pattern: PatternBlock) {
        guard let synthEngine = synthEngine else { return }
        pattern.patternID.withCString { patternIDCString in
            ether_pattern_delete(synthEngine, Int32(trackIndex), patternIDCString)
        }
    }
    
    private func notifyEnginePatternMoved(trackIndex: Int, pattern: PatternBlock) {
        guard let synthEngine = synthEngine else { return }
        pattern.patternID.withCString { patternIDCString in
            ether_pattern_move(synthEngine, Int32(trackIndex), patternIDCString, Int32(pattern.startPosition))
        }
    }
    
    // MARK: - Effects Control
    
    func addEffect(type: Int, slot: Int) -> UInt32 {
        guard let synthEngine = synthEngine else { return 0 }
        let effectId = ether_add_effect(synthEngine, Int32(type), Int32(slot))
        if effectId != 0 {
            activeEffects.append(effectId)
            currentEffectID = effectId
            // Initialize default parameters for this effect
            for i in 0..<16 {
                effectParameters["\(effectId)_\(i)"] = 0.0
            }
        }
        return effectId
    }
    
    func removeEffect(effectId: UInt32) {
        guard let synthEngine = synthEngine else { return }
        ether_remove_effect(synthEngine, effectId)
        activeEffects.removeAll { $0 == effectId }
        // Clean up parameter state
        for i in 0..<16 {
            effectParameters.removeValue(forKey: "\(effectId)_\(i)")
        }
        if currentEffectID == effectId {
            currentEffectID = activeEffects.first ?? 0
        }
    }
    
    func setEffectParameter(effectId: UInt32, keyIndex: Int, value: Float) {
        guard let synthEngine = synthEngine else { return }
        ether_set_effect_parameter(synthEngine, effectId, Int32(keyIndex), value)
        effectParameters["\(effectId)_\(keyIndex)"] = value
    }
    
    func getEffectParameter(effectId: UInt32, keyIndex: Int) -> Float {
        return effectParameters["\(effectId)_\(keyIndex)"] ?? 0.0
    }
    
    func getEffectParameterName(effectId: UInt32, keyIndex: Int) -> String {
        guard let synthEngine = synthEngine else { return "PARAM" }
        let name = UnsafeMutablePointer<CChar>.allocate(capacity: 16)
        defer { name.deallocate() }
        ether_get_effect_parameter_name(synthEngine, effectId, Int32(keyIndex), name, 16)
        return String(cString: name)
    }
    
    // Performance effects functions
    func triggerReverbThrow() {
        guard let synthEngine = synthEngine else { return }
        ether_trigger_reverb_throw(synthEngine)
        reverbThrowActive.toggle()
    }
    
    func triggerDelayThrow() {
        guard let synthEngine = synthEngine else { return }
        ether_trigger_delay_throw(synthEngine)
        delayThrowActive.toggle()
    }
    
    func setPerformanceFilter(cutoff: Float, resonance: Float, type: Int) {
        guard let synthEngine = synthEngine else { return }
        ether_set_performance_filter(synthEngine, cutoff, resonance, Int32(type))
        performanceFilterCutoff = cutoff
        performanceFilterResonance = resonance
        performanceFilterType = type
    }
    
    func toggleNoteRepeat(division: Int) {
        guard let synthEngine = synthEngine else { return }
        ether_toggle_note_repeat(synthEngine, Int32(division))
        noteRepeatActive.toggle()
        noteRepeatDivision = division
    }
    
    func setReverbSend(level: Float) {
        guard let synthEngine = synthEngine else { return }
        ether_set_reverb_send(synthEngine, level)
        reverbSend = level
    }
    
    func setDelaySend(level: Float) {
        guard let synthEngine = synthEngine else { return }
        ether_set_delay_send(synthEngine, level)
        delaySend = level
    }
    
    // Effects preset management
    func saveEffectPreset(slot: Int, name: String) {
        guard let synthEngine = synthEngine else { return }
        name.withCString { namePtr in
            ether_save_effects_preset(synthEngine, Int32(slot), namePtr)
        }
        if slot < effectPresets.count {
            effectPresets[slot] = name
        }
    }
    
    func loadEffectPreset(slot: Int) -> Bool {
        guard let synthEngine = synthEngine else { return false }
        let success = ether_load_effects_preset(synthEngine, Int32(slot))
        if success {
            selectedEffectPreset = slot
            // Refresh effect parameters after loading preset
            refreshEffectParameters()
        }
        return success
    }
    
    private func refreshEffectParameters() {
        guard let synthEngine = synthEngine else { return }
        for effectId in activeEffects {
            for i in 0..<16 {
                let value = ether_get_effect_parameter(synthEngine, effectId, Int32(i))
                effectParameters["\(effectId)_\(i)"] = value
            }
        }
    }
    
    // Effects metering (for visual feedback)
    func getEffectsMetering() -> (reverb: Float, delay: Float, compression: Float, lufs: Float, peak: Float, limiterActive: Bool) {
        guard let synthEngine = synthEngine else { 
            return (0.0, 0.0, 0.0, -14.0, 0.0, false) 
        }
        return (
            ether_get_reverb_level(synthEngine),
            ether_get_delay_level(synthEngine),
            ether_get_compression_reduction(synthEngine),
            ether_get_lufs_level(synthEngine),
            ether_get_peak_level(synthEngine),
            ether_is_limiter_active(synthEngine)
        )
    }
    
    // MARK: - Pattern Chain Management
    
    func createPatternChain(startPatternId: UInt32, patternIds: [UInt32]) {
        guard let synthEngine = synthEngine else { return }
        let patternArray = patternIds.map { Int32($0) }
        patternArray.withUnsafeBufferPointer { buffer in
            ether_create_pattern_chain(synthEngine, startPatternId, buffer.baseAddress?.withMemoryRebound(to: UInt32.self, capacity: buffer.count) { $0 }, Int32(buffer.count))
        }
    }
    
    func queuePattern(patternId: UInt32, trackIndex: Int, triggerMode: Int = 1) {
        guard let synthEngine = synthEngine else { return }
        ether_queue_pattern(synthEngine, patternId, Int32(trackIndex), Int32(triggerMode))
        queuedPatterns[trackIndex] = patternId
    }
    
    func triggerPattern(patternId: UInt32, trackIndex: Int, immediate: Bool = false) {
        guard let synthEngine = synthEngine else { return }
        ether_trigger_pattern(synthEngine, patternId, Int32(trackIndex), immediate)
        currentPatterns[trackIndex] = patternId
        if !immediate {
            queuedPatterns[trackIndex] = 0 // Clear queue
        }
    }
    
    func getCurrentPattern(trackIndex: Int) -> UInt32 {
        guard let synthEngine = synthEngine else { return 0 }
        return ether_get_current_pattern(synthEngine, Int32(trackIndex))
    }
    
    func getQueuedPattern(trackIndex: Int) -> UInt32 {
        guard let synthEngine = synthEngine else { return 0 }
        return ether_get_queued_pattern(synthEngine, Int32(trackIndex))
    }
    
    func setChainMode(trackIndex: Int, mode: Int) {
        guard let synthEngine = synthEngine else { return }
        ether_set_chain_mode(synthEngine, Int32(trackIndex), Int32(mode))
        chainModes[trackIndex] = mode
    }
    
    // Live Performance Functions
    func armPattern(patternId: UInt32, trackIndex: Int) {
        guard let synthEngine = synthEngine else { return }
        ether_arm_pattern(synthEngine, patternId, Int32(trackIndex))
        armedPatterns[trackIndex] = patternId
    }
    
    func launchArmedPatterns() {
        guard let synthEngine = synthEngine else { return }
        ether_launch_armed_patterns(synthEngine)
        // Clear all armed patterns after launch
        for i in 0..<8 {
            armedPatterns[i] = 0
        }
    }
    
    func setPerformanceMode(_ enabled: Bool) {
        guard let synthEngine = synthEngine else { return }
        ether_set_performance_mode(synthEngine, enabled)
        performanceMode = enabled
    }
    
    func setGlobalQuantization(bars: Int) {
        guard let synthEngine = synthEngine else { return }
        ether_set_global_quantization(synthEngine, Int32(bars))
        globalQuantization = bars
    }
    
    // Pattern Variations and Mutations
    func generatePatternVariation(sourcePatternId: UInt32, mutationAmount: Float) {
        guard let synthEngine = synthEngine else { return }
        ether_generate_pattern_variation(synthEngine, sourcePatternId, mutationAmount)
    }
    
    func applyEuclideanRhythm(patternId: UInt32, steps: Int, pulses: Int, rotation: Int = 0) {
        guard let synthEngine = synthEngine else { return }
        ether_apply_euclidean_rhythm(synthEngine, patternId, Int32(steps), Int32(pulses), Int32(rotation))
    }
    
    func morphPatternTiming(patternId: UInt32, swing: Float, humanize: Float) {
        guard let synthEngine = synthEngine else { return }
        ether_morph_pattern_timing(synthEngine, patternId, swing, humanize)
    }
    
    // Scene Management
    func saveScene(name: String) -> UInt32 {
        guard let synthEngine = synthEngine else { return 0 }
        return name.withCString { namePtr in
            ether_save_scene(synthEngine, namePtr)
        }
    }
    
    func loadScene(sceneId: UInt32) -> Bool {
        guard let synthEngine = synthEngine else { return false }
        let success = ether_load_scene(synthEngine, sceneId)
        if success {
            selectedSceneId = sceneId
            // Refresh UI state after scene load
            refreshSceneState()
        }
        return success
    }
    
    func createSection(type: Int, name: String, patternIds: [UInt32]) -> UInt32 {
        guard let synthEngine = synthEngine else { return 0 }
        return name.withCString { namePtr in
            patternIds.withUnsafeBufferPointer { buffer in
                ether_create_section(synthEngine, Int32(type), namePtr, 
                                   buffer.baseAddress?.withMemoryRebound(to: UInt32.self, capacity: buffer.count) { $0 }, 
                                   Int32(buffer.count))
            }
        }
    }
    
    // Pattern Intelligence
    func getSuggestedPatterns(currentPattern: UInt32, maxCount: Int = 4) -> [UInt32] {
        guard let synthEngine = synthEngine else { return [] }
        var suggestions = Array<UInt32>(repeating: 0, count: maxCount)
        suggestions.withUnsafeMutableBufferPointer { buffer in
            ether_get_suggested_patterns(synthEngine, currentPattern, buffer.baseAddress, Int32(maxCount))
        }
        return suggestions.filter { $0 != 0 }
    }
    
    func generateIntelligentChain(startPattern: UInt32, chainLength: Int) {
        guard let synthEngine = synthEngine else { return }
        ether_generate_intelligent_chain(synthEngine, startPattern, Int32(chainLength))
    }
    
    // Hardware Integration
    func processPatternKey(keyIndex: Int, pressed: Bool, trackIndex: Int) {
        guard let synthEngine = synthEngine else { return }
        ether_process_pattern_key(synthEngine, Int32(keyIndex), pressed, Int32(trackIndex))
    }
    
    func processChainKnob(value: Float, trackIndex: Int) {
        guard let synthEngine = synthEngine else { return }
        ether_process_chain_knob(synthEngine, value, Int32(trackIndex))
    }
    
    // Chain Templates
    func createBasicLoop(patternIds: [UInt32]) {
        guard let synthEngine = synthEngine else { return }
        patternIds.withUnsafeBufferPointer { buffer in
            ether_create_basic_loop(synthEngine, buffer.baseAddress?.withMemoryRebound(to: UInt32.self, capacity: buffer.count) { $0 }, Int32(buffer.count))
        }
    }
    
    func createVerseChorus(versePattern: UInt32, chorusPattern: UInt32) {
        guard let synthEngine = synthEngine else { return }
        ether_create_verse_chorus(synthEngine, versePattern, chorusPattern)
    }
    
    // MARK: - AI Generative Sequencer Functions
    
    func generatePattern(mode: Int, style: Int, complexity: Int, trackIndex: Int) -> UInt32 {
        guard let synthEngine = synthEngine else { return 0 }
        let patternId = ether_generate_pattern(synthEngine, Int32(mode), Int32(style), Int32(complexity), Int32(trackIndex))
        
        // Update pattern complexity and interest cache
        let complexity = ether_get_pattern_complexity(synthEngine, patternId)
        let interest = ether_get_pattern_interest(synthEngine, patternId)
        patternComplexities[patternId] = complexity
        patternInterests[patternId] = interest
        
        return patternId
    }
    
    func setGenerationParams(density: Float, tension: Float, creativity: Float, responsiveness: Float) {
        guard let synthEngine = synthEngine else { return }
        ether_set_generation_params(synthEngine, density, tension, creativity, responsiveness)
        
        // Update local state
        self.generationDensity = density
        self.generationTension = tension
        self.generationCreativity = creativity
        self.generationResponsiveness = responsiveness
    }
    
    func evolvePattern(patternId: UInt32, evolutionAmount: Float) {
        guard let synthEngine = synthEngine else { return }
        ether_evolve_pattern(synthEngine, patternId, evolutionAmount)
        
        // Update complexity cache
        let complexity = ether_get_pattern_complexity(synthEngine, patternId)
        let interest = ether_get_pattern_interest(synthEngine, patternId)
        patternComplexities[patternId] = complexity
        patternInterests[patternId] = interest
    }
    
    func generateHarmony(sourcePatternId: UInt32) -> UInt32 {
        guard let synthEngine = synthEngine else { return 0 }
        return ether_generate_harmony(synthEngine, sourcePatternId)
    }
    
    func generateRhythmVariation(sourcePatternId: UInt32, variationAmount: Float) -> UInt32 {
        guard let synthEngine = synthEngine else { return 0 }
        return ether_generate_rhythm_variation(synthEngine, sourcePatternId, variationAmount)
    }
    
    func setAdaptiveMode(enabled: Bool) {
        guard let synthEngine = synthEngine else { return }
        ether_set_adaptive_mode(synthEngine, enabled)
        self.adaptiveMode = enabled
    }
    
    func resetLearningModel() {
        guard let synthEngine = synthEngine else { return }
        ether_reset_learning_model(synthEngine)
    }
    
    func getGenerativeSuggestions() {
        guard let synthEngine = synthEngine else { return }
        var suggestions = Array<UInt32>(repeating: 0, count: 8)
        ether_get_generative_suggestions(synthEngine, &suggestions, 8)
        self.generativeSuggestions = suggestions.filter { $0 != 0 }
    }
    
    func setMusicalStyle(style: Int) {
        guard let synthEngine = synthEngine else { return }
        ether_set_musical_style(synthEngine, Int32(style))
        self.musicalStyle = style
    }
    
    func loadStyleTemplate(styleType: Int) {
        guard let synthEngine = synthEngine else { return }
        ether_load_style_template(synthEngine, Int32(styleType))
    }
    
    func getScaleAnalysis() {
        guard let synthEngine = synthEngine else { return }
        var rootNote: Int32 = 0
        var scaleType: Int32 = 0
        var confidence: Float = 0.0
        
        ether_get_scale_analysis(synthEngine, &rootNote, &scaleType, &confidence)
        self.detectedScale = (Int(rootNote), Int(scaleType), confidence)
    }
    
    func processGenerativeKey(keyIndex: Int, pressed: Bool, velocity: Float) {
        guard let synthEngine = synthEngine else { return }
        ether_process_generative_key(synthEngine, Int32(keyIndex), pressed, velocity)
    }
    
    func processGenerativeKnob(value: Float, paramIndex: Int) {
        guard let synthEngine = synthEngine else { return }
        ether_process_generative_knob(synthEngine, value, Int32(paramIndex))
    }
    
    func triggerGenerativeEvent(eventType: Int) {
        guard let synthEngine = synthEngine else { return }
        ether_trigger_generative_event(synthEngine, Int32(eventType))
    }
    
    func optimizePatternForHardware(patternId: UInt32) {
        guard let synthEngine = synthEngine else { return }
        ether_optimize_pattern_for_hardware(synthEngine, patternId)
    }
    
    func quantizePattern(patternId: UInt32, strength: Float) {
        guard let synthEngine = synthEngine else { return }
        ether_quantize_pattern(synthEngine, patternId, strength)
    }
    
    func addPatternSwing(patternId: UInt32, swingAmount: Float) {
        guard let synthEngine = synthEngine else { return }
        ether_add_pattern_swing(synthEngine, patternId, swingAmount)
    }
    
    func humanizePattern(patternId: UInt32, humanizeAmount: Float) {
        guard let synthEngine = synthEngine else { return }
        ether_humanize_pattern(synthEngine, patternId, humanizeAmount)
    }
    
    // MARK: - Performance Macros Functions
    
    func createMacro(name: String, type: PerformanceMacro.MacroType, triggerMode: PerformanceMacro.TriggerMode) -> UInt32 {
        guard let synthEngine = synthEngine else { return 0 }
        let macroId = ether_create_macro(synthEngine, name, Int32(type.rawValue), Int32(triggerMode.rawValue))
        
        if macroId != 0 {
            let macro = PerformanceMacro(
                id: macroId,
                name: name,
                type: type,
                triggerMode: triggerMode
            )
            availableMacros.append(macro)
        }
        
        return macroId
    }
    
    func deleteMacro(macroId: UInt32) -> Bool {
        guard let synthEngine = synthEngine else { return false }
        let success = ether_delete_macro(synthEngine, macroId)
        
        if success {
            availableMacros.removeAll { $0.id == macroId }
            macroKeyBindings = macroKeyBindings.filter { $0.value != macroId }
            activeMacros.remove(macroId)
        }
        
        return success
    }
    
    func executeMacro(macroId: UInt32, intensity: Float = 1.0) {
        guard let synthEngine = synthEngine else { return }
        ether_execute_macro(synthEngine, macroId, intensity)
        
        activeMacros.insert(macroId)
        
        // Update macro state
        if let index = availableMacros.firstIndex(where: { $0.id == macroId }) {
            availableMacros[index].isActive = true
        }
    }
    
    func stopMacro(macroId: UInt32) {
        guard let synthEngine = synthEngine else { return }
        ether_stop_macro(synthEngine, macroId)
        
        activeMacros.remove(macroId)
        
        // Update macro state
        if let index = availableMacros.firstIndex(where: { $0.id == macroId }) {
            availableMacros[index].isActive = false
        }
    }
    
    func bindMacroToKey(macroId: UInt32, keyIndex: Int, requiresShift: Bool = false, requiresAlt: Bool = false) {
        guard let synthEngine = synthEngine else { return }
        ether_bind_macro_to_key(synthEngine, macroId, Int32(keyIndex), requiresShift, requiresAlt)
        
        macroKeyBindings[keyIndex] = macroId
        
        // Update macro state
        if let index = availableMacros.firstIndex(where: { $0.id == macroId }) {
            availableMacros[index].keyIndex = keyIndex
            availableMacros[index].requiresShift = requiresShift
            availableMacros[index].requiresAlt = requiresAlt
        }
    }
    
    func processPerformanceKey(keyIndex: Int, pressed: Bool, shiftHeld: Bool = false, altHeld: Bool = false) {
        guard let synthEngine = synthEngine else { return }
        ether_process_performance_key(synthEngine, Int32(keyIndex), pressed, shiftHeld, altHeld)
        
        // Handle macro execution locally as well
        if pressed, let macroId = macroKeyBindings[keyIndex] {
            executeMacro(macroId: macroId)
        }
    }
    
    func setPerformanceModeEnabled(_ enabled: Bool) {
        guard let synthEngine = synthEngine else { return }
        ether_set_performance_mode(synthEngine, enabled)
        isPerformanceModeEnabled = enabled
    }
    
    // Scene Management
    func captureScene(name: String) -> UInt32 {
        guard let synthEngine = synthEngine else { return 0 }
        return ether_capture_scene(synthEngine, name)
    }
    
    func recallScene(sceneId: UInt32, morphTime: Float = 0.5) -> Bool {
        guard let synthEngine = synthEngine else { return false }
        return ether_recall_scene(synthEngine, sceneId, morphTime)
    }
    
    // Live Looping
    func createLiveLoop(name: String, recordingTrack: Int) -> UInt32 {
        guard let synthEngine = synthEngine else { return 0 }
        let loopId = ether_create_live_loop(synthEngine, name, Int32(recordingTrack))
        
        if loopId != 0 {
            let loop = LiveLoop(id: loopId, name: name, recordingTrack: recordingTrack)
            liveLoops.append(loop)
        }
        
        return loopId
    }
    
    func startLoopRecording(loopId: UInt32) {
        guard let synthEngine = synthEngine else { return }
        ether_start_loop_recording(synthEngine, loopId)
        
        recordingLoop = loopId
        
        if let index = liveLoops.firstIndex(where: { $0.id == loopId }) {
            liveLoops[index].isRecording = true
        }
    }
    
    func stopLoopRecording(loopId: UInt32) {
        guard let synthEngine = synthEngine else { return }
        ether_stop_loop_recording(synthEngine, loopId)
        
        if recordingLoop == loopId {
            recordingLoop = nil
        }
        
        if let index = liveLoops.firstIndex(where: { $0.id == loopId }) {
            liveLoops[index].isRecording = false
        }
    }
    
    func startLoopPlayback(loopId: UInt32, targetTrack: Int = -1) {
        guard let synthEngine = synthEngine else { return }
        ether_start_loop_playback(synthEngine, loopId, Int32(targetTrack))
        
        if let index = liveLoops.firstIndex(where: { $0.id == loopId }) {
            liveLoops[index].isPlaying = true
            if targetTrack >= 0 {
                liveLoops[index].targetTrack = targetTrack
            }
        }
    }
    
    func stopLoopPlayback(loopId: UInt32) {
        guard let synthEngine = synthEngine else { return }
        ether_stop_loop_playback(synthEngine, loopId)
        
        if let index = liveLoops.firstIndex(where: { $0.id == loopId }) {
            liveLoops[index].isPlaying = false
        }
    }
    
    // Factory Macros
    func loadFactoryMacros() {
        guard let synthEngine = synthEngine else { return }
        ether_load_factory_macros(synthEngine)
        
        // Create some default macros for UI
        let filterSweepId = createMacro(name: "Filter Sweep", type: .filterSweep, triggerMode: .immediate)
        let volumeFadeId = createMacro(name: "Volume Swell", type: .volumeFade, triggerMode: .hold)
        let reverbThrowId = createMacro(name: "Reverb Throw", type: .effectChain, triggerMode: .immediate)
        let halftimeId = createMacro(name: "Halftime", type: .tempoRamp, triggerMode: .toggle)
        
        // Assign some default key bindings
        bindMacroToKey(macroId: filterSweepId, keyIndex: 0)
        bindMacroToKey(macroId: volumeFadeId, keyIndex: 1)
        bindMacroToKey(macroId: reverbThrowId, keyIndex: 2)
        bindMacroToKey(macroId: halftimeId, keyIndex: 3)
    }
    
    func getPerformanceStats() {
        guard let synthEngine = synthEngine else { return }
        var macrosExecuted: Int32 = 0
        var scenesRecalled: Int32 = 0
        var averageRecallTime: Float = 0.0
        
        ether_get_performance_stats(synthEngine, &macrosExecuted, &scenesRecalled, &averageRecallTime)
        
        performanceStats.macrosExecuted = Int(macrosExecuted)
        performanceStats.scenesRecalled = Int(scenesRecalled)
        performanceStats.averageRecallTime = averageRecallTime
    }
    
    // MARK: - Euclidean Sequencer Functions
    
    func setEuclideanPattern(trackIndex: Int, pulses: Int, rotation: Int = 0) {
        guard let synthEngine = synthEngine, trackIndex >= 0, trackIndex < 8 else { return }
        ether_set_euclidean_pattern(synthEngine, Int32(trackIndex), Int32(pulses), Int32(rotation))
        
        // Update local pattern
        if trackIndex < euclideanPatterns.count {
            euclideanPatterns[trackIndex].pulses = pulses
            euclideanPatterns[trackIndex].rotation = rotation
        }
    }
    
    func setEuclideanProbability(trackIndex: Int, probability: Float) {
        guard let synthEngine = synthEngine, trackIndex >= 0, trackIndex < 8 else { return }
        ether_set_euclidean_probability(synthEngine, Int32(trackIndex), probability)
        
        if trackIndex < euclideanPatterns.count {
            euclideanPatterns[trackIndex].probability = probability
        }
    }
    
    func setEuclideanSwing(trackIndex: Int, swing: Float) {
        guard let synthEngine = synthEngine, trackIndex >= 0, trackIndex < 8 else { return }
        ether_set_euclidean_swing(synthEngine, Int32(trackIndex), swing)
        
        if trackIndex < euclideanPatterns.count {
            euclideanPatterns[trackIndex].swing = swing
        }
    }
    
    func setEuclideanHumanization(trackIndex: Int, humanization: Float) {
        guard let synthEngine = synthEngine, trackIndex >= 0, trackIndex < 8 else { return }
        ether_set_euclidean_humanization(synthEngine, Int32(trackIndex), humanization)
        
        if trackIndex < euclideanPatterns.count {
            euclideanPatterns[trackIndex].humanization = humanization
        }
    }
    
    func shouldTriggerEuclideanStep(trackIndex: Int, stepIndex: Int) -> Bool {
        guard let synthEngine = synthEngine, trackIndex >= 0, trackIndex < 8, stepIndex >= 0, stepIndex < 16 else { return false }
        return ether_should_trigger_euclidean_step(synthEngine, Int32(trackIndex), Int32(stepIndex))
    }
    
    func getEuclideanStepVelocity(trackIndex: Int, stepIndex: Int) -> Float {
        guard let synthEngine = synthEngine, trackIndex >= 0, trackIndex < 8, stepIndex >= 0, stepIndex < 16 else { return 0.0 }
        return ether_get_euclidean_step_velocity(synthEngine, Int32(trackIndex), Int32(stepIndex))
    }
    
    func getEuclideanDensity(trackIndex: Int) -> Float {
        guard let synthEngine = synthEngine, trackIndex >= 0, trackIndex < 8 else { return 0.0 }
        return ether_get_euclidean_density(synthEngine, Int32(trackIndex))
    }
    
    func loadEuclideanPreset(trackIndex: Int, presetName: String) {
        guard let synthEngine = synthEngine, trackIndex >= 0, trackIndex < 8 else { return }
        ether_load_euclidean_preset(synthEngine, Int32(trackIndex), presetName)
        
        selectedEuclideanPreset = presetName
    }
    
    func initializeEuclideanPatterns() {
        // Initialize patterns for all 8 tracks
        euclideanPatterns = (0..<8).map { trackIndex in
            EuclideanPattern(
                trackIndex: trackIndex,
                pulses: (trackIndex < 4) ? 4 : 2,  // Drums get 4 hits, others get 2
                rotation: 0,
                probability: 1.0,
                swing: 0.0,
                humanization: 0.0
            )
        }
        
        // Load preset names
        guard let synthEngine = synthEngine else { return }
        var presetNames = [Int8](repeating: 0, count: 1000)
        ether_get_euclidean_preset_names(synthEngine, &presetNames, 1000)
        
        let presetString = String(cString: presetNames)
        euclideanPresets = presetString.components(separatedBy: ",").filter { !$0.isEmpty }
        
        if !euclideanPresets.isEmpty {
            selectedEuclideanPreset = euclideanPresets[0]
        }
    }
    
    // MARK: - Enhanced Chord System (Bicep Mode)
    
    // Update pattern visualization
    func regenerateAllEuclideanPatterns() {
        for i in 0..<euclideanPatterns.count {
            updateEuclideanPatternVisualization(trackIndex: i)
        }
    }
    
    private func updateEuclideanPatternVisualization(trackIndex: Int) {
        guard trackIndex >= 0, trackIndex < euclideanPatterns.count else { return }
        
        // Update the pattern steps based on current parameters
        for step in 0..<16 {
            euclideanPatterns[trackIndex].patternSteps[step] = shouldTriggerEuclideanStep(trackIndex: trackIndex, stepIndex: step)
            euclideanPatterns[trackIndex].velocities[step] = getEuclideanStepVelocity(trackIndex: trackIndex, stepIndex: step)
        }
    }
    
    // MARK: - Enhanced Chord System Management (Bicep Mode)
    
    func enableChordSystem(enabled: Bool) {
        chordSystemEnabled = enabled
        if let engine = synthEngine {
            ether_chord_set_enabled(engine, enabled ? 1 : 0)
        }
    }
    
    func setChordType(chordType: Int) {
        currentChordType = chordType
        if let engine = synthEngine {
            ether_chord_set_type(engine, Int32(chordType))
        }
    }
    
    func setChordRoot(rootNote: Int) {
        currentChordRoot = rootNote
        if let engine = synthEngine {
            ether_chord_set_root(engine, Int32(rootNote))
        }
    }
    
    func playChord(rootNote: Int? = nil, velocity: Float = 0.8) {
        let root = rootNote ?? currentChordRoot
        if let engine = synthEngine {
            ether_chord_play(engine, Int32(root), velocity)
        }
    }
    
    func releaseChord() {
        if let engine = synthEngine {
            ether_chord_release(engine)
        }
    }
    
    func setVoiceEngine(voiceIndex: Int, engineType: Int) {
        guard voiceIndex >= 0 && voiceIndex < 5 else { return }
        chordVoiceEngines[voiceIndex] = engineType
        if let engine = synthEngine {
            ether_chord_set_voice_engine(engine, Int32(voiceIndex), Int32(engineType))
        }
    }
    
    func setVoiceRole(voiceIndex: Int, role: Int) {
        guard voiceIndex >= 0 && voiceIndex < 5 else { return }
        chordVoiceRoles[voiceIndex] = role
        if let engine = synthEngine {
            ether_chord_set_voice_role(engine, Int32(voiceIndex), Int32(role))
        }
    }
    
    func setVoiceLevel(voiceIndex: Int, level: Float) {
        guard voiceIndex >= 0 && voiceIndex < 5 else { return }
        chordVoiceLevels[voiceIndex] = level
        if let engine = synthEngine {
            ether_chord_set_voice_level(engine, Int32(voiceIndex), level)
        }
    }
    
    func setVoicePan(voiceIndex: Int, pan: Float) {
        guard voiceIndex >= 0 && voiceIndex < 5 else { return }
        chordVoicePans[voiceIndex] = pan
        if let engine = synthEngine {
            ether_chord_set_voice_pan(engine, Int32(voiceIndex), pan)
        }
    }
    
    func enableVoice(voiceIndex: Int, enabled: Bool) {
        guard voiceIndex >= 0 && voiceIndex < 5 else { return }
        chordVoiceActive[voiceIndex] = enabled
        if let engine = synthEngine {
            ether_chord_enable_voice(engine, Int32(voiceIndex), enabled ? 1 : 0)
        }
    }
    
    func assignInstrument(instrumentIndex: Int, voiceIndices: [Int]) {
        guard instrumentIndex >= 0 && instrumentIndex < 8 else { return }
        chordInstrumentAssignments[instrumentIndex] = voiceIndices
        if let engine = synthEngine {
            // Convert Swift array to C array for bridge
            let voiceArray = voiceIndices.map { Int32($0) }
            voiceArray.withUnsafeBufferPointer { bufferPtr in
                ether_chord_assign_instrument(engine, Int32(instrumentIndex), bufferPtr.baseAddress, Int32(voiceIndices.count))
            }
        }
    }
    
    func setInstrumentStrumOffset(instrumentIndex: Int, offsetMs: Float) {
        guard instrumentIndex >= 0 && instrumentIndex < 8 else { return }
        chordStrumOffsets[instrumentIndex] = offsetMs
        if let engine = synthEngine {
            ether_chord_set_instrument_strum_offset(engine, Int32(instrumentIndex), offsetMs)
        }
    }
    
    func setInstrumentArpeggio(instrumentIndex: Int, enabled: Bool, rate: Float = 8.0) {
        guard instrumentIndex >= 0 && instrumentIndex < 8 else { return }
        chordArpeggioModes[instrumentIndex] = enabled
        chordArpeggioRates[instrumentIndex] = rate
        if let engine = synthEngine {
            ether_chord_set_instrument_arpeggio(engine, Int32(instrumentIndex), enabled ? 1 : 0, rate)
        }
    }
    
    func setChordSpread(spread: Float) {
        chordSpread = spread
        if let engine = synthEngine {
            ether_chord_set_spread(engine, spread)
        }
    }
    
    func setChordStrumTime(strumTime: Float) {
        chordStrumTime = strumTime
        if let engine = synthEngine {
            ether_chord_set_strum_time(engine, strumTime)
        }
    }
    
    func setChordHumanization(humanization: Float) {
        chordHumanization = humanization
        if let engine = synthEngine {
            ether_chord_set_humanization(engine, humanization)
        }
    }
    
    func enableVoiceLeading(enabled: Bool) {
        chordVoiceLeadingEnabled = enabled
        if let engine = synthEngine {
            ether_chord_enable_voice_leading(engine, enabled ? 1 : 0)
        }
    }
    
    func setVoiceLeadingStrength(strength: Float) {
        chordVoiceLeadingStrength = strength
        if let engine = synthEngine {
            ether_chord_set_voice_leading_strength(engine, strength)
        }
    }
    
    func loadChordArrangementPreset(presetName: String) {
        if let engine = synthEngine {
            presetName.withCString { cString in
                ether_chord_load_arrangement_preset(engine, cString)
            }
        }
        // Update local state based on loaded preset
        updateChordStateFromEngine()
    }
    
    func saveChordArrangementPreset(presetName: String) {
        if let engine = synthEngine {
            presetName.withCString { cString in
                ether_chord_save_arrangement_preset(engine, cString)
            }
        }
        // Add to local preset list if not already present
        if !chordArrangementPresets.contains(presetName) {
            chordArrangementPresets.append(presetName)
        }
    }
    
    func clearInstrumentAssignments() {
        for i in 0..<8 {
            chordInstrumentAssignments[i] = []
        }
        if let engine = synthEngine {
            ether_chord_clear_instrument_assignments(engine)
        }
    }
    
    func arpeggiate(rootNote: Int, velocity: Float = 0.8, rate: Float = 8.0) {
        if let engine = synthEngine {
            ether_chord_arpeggiate(engine, Int32(rootNote), velocity, rate)
        }
    }
    
    func updateChordStats() {
        if let engine = synthEngine {
            chordCPUUsage = ether_chord_get_cpu_usage(engine)
            chordActiveVoices = Int(ether_chord_get_active_voice_count(engine))
        }
    }
    
    private func updateChordStateFromEngine() {
        // Called after loading presets to sync UI state with engine
        updateChordStats()
        // Could add more detailed state synchronization here if needed
    }
    
    func initializeChordSystem() {
        // Initialize chord system with default configuration
        enableChordSystem(enabled: false) // Start disabled
        setChordType(chordType: 0) // Major chord
        setChordRoot(rootNote: 60) // Middle C
        
        // Setup default voice configuration (Bicep-style)
        setVoiceEngine(voiceIndex: 0, engineType: 0)  // MacroVA for root
        setVoiceEngine(voiceIndex: 1, engineType: 2)  // MacroWT for harmony 1
        setVoiceEngine(voiceIndex: 2, engineType: 1)  // MacroFM for harmony 2
        setVoiceEngine(voiceIndex: 3, engineType: 5)  // MacroHarm for color
        setVoiceEngine(voiceIndex: 4, engineType: 3)  // MacroWS for doubling
        
        // Default instrument assignments for multi-instrument chord distribution
        assignInstrument(instrumentIndex: 0, voiceIndices: [0])     // Track 1: Root
        assignInstrument(instrumentIndex: 1, voiceIndices: [1, 2]) // Track 2: Harmonies
        assignInstrument(instrumentIndex: 2, voiceIndices: [3])     // Track 3: Color
        assignInstrument(instrumentIndex: 3, voiceIndices: [4])     // Track 4: Doubling
    }

    private func refreshSceneState() {
        // Update UI state based on loaded scene
        for i in 0..<8 {
            let currentPattern = getCurrentPattern(trackIndex: i)
            currentPatterns[i] = currentPattern
        }
    }
}

// MARK: - Main App
@main
struct EtherSynthApp: App {
    @StateObject private var appState = EtherSynthState()
    
    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(appState)
                .preferredColorScheme(.light)
                .frame(width: 960, height: 320)
                .fixedSize()
        }
        .windowResizability(.contentSize)
    }
}

// MARK: - Main Content View
struct ContentView: View {
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        ZStack {
            // Main app container with professional styling
            RoundedRectangle(cornerRadius: 16)
                .fill(Color(red: 0.97, green: 0.98, blue: 0.99))
                .shadow(color: Color.black.opacity(0.08), radius: 12, x: 0, y: 10)
                .overlay(
                    VStack(spacing: 0) {
                        // Header: Transport + Globals
                        HeaderView()
                            .frame(height: UIConstants.headerHeight)
                        
                        // Option Line: 16 buttons in quads  
                        OptionLineView()
                            .frame(height: UIConstants.optionLineHeight)
                        
                        // View Area: Context-sensitive content
                        ViewAreaView()
                            .frame(height: UIConstants.viewAreaHeight)
                        
                        // Step Row: Always-live 16 steps
                        StepRowView()
                            .frame(height: UIConstants.stepRowHeight)
                        
                        // Footer: Status and feedback
                        FooterView()
                            .frame(height: UIConstants.footerHeight)
                    }
                    .clipShape(RoundedRectangle(cornerRadius: 16))
                )
            
            // Modal overlays
            if appState.showingOverlay {
                ModalOverlayView()
            }
        }
        .frame(width: UIConstants.screenWidth, height: UIConstants.screenHeight)
        .background(Color(red: 0.95, green: 0.96, blue: 0.97))
    }
}

// MARK: - NEW 2×16 UI COMPONENTS

// MARK: - Header View (Transport + Globals)
struct HeaderView: View {
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        HStack(spacing: 8) {
            // Transport controls
            HStack(spacing: 4) {
                Button(action: { appState.togglePlay() }) {
                    Text(appState.isPlaying ? "■" : "▶")
                        .font(.system(size: 14, weight: .medium))
                        .foregroundColor(.white)
                }
                .frame(width: 28, height: 20)
                .background(appState.isPlaying ? Color.green : Color.gray)
                .cornerRadius(4)
                
                Button(action: { /* REC */ }) {
                    Text("REC")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.white)
                }
                .frame(width: 32, height: 20)
                .background(appState.isRecording ? Color.red : Color.gray)
                .cornerRadius(4)
                
                Button(action: { /* WRITE */ }) {
                    Text("WRITE")
                        .font(.system(size: 9, weight: .medium))
                        .foregroundColor(.white)
                }
                .frame(width: 40, height: 20)
                .background(Color.blue)
                .cornerRadius(4)
                
                Button(action: { appState.shiftButtonPressed.toggle() }) {
                    Text("SHIFT")
                        .font(.system(size: 9, weight: .medium))
                        .foregroundColor(.white)
                }
                .frame(width: 40, height: 20)
                .background(appState.shiftButtonPressed ? Color.orange : Color.gray)
                .cornerRadius(4)
            }
            
            Spacer()
            
            // Mode and status
            HStack(spacing: 6) {
                Text(appState.currentMode.rawValue)
                    .font(.custom("Barlow Condensed", size: 10))
                    .fontWeight(.medium)
                    .foregroundColor(Color.white)
                    .padding(.horizontal, 6)
                    .padding(.vertical, 2)
                    .background(Color.blue)
                    .cornerRadius(4)
                
                if appState.parameterState.isParameterMode {
                    Text("PARAM")
                        .font(.custom("Barlow Condensed", size: 10))
                        .fontWeight(.medium)
                        .foregroundColor(Color.white)
                        .padding(.horizontal, 6)
                        .padding(.vertical, 2)
                        .background(Color.orange)
                        .cornerRadius(4)
                }
            }
            
            Spacer()
            
            // Global values
            HStack(spacing: 8) {
                Button(action: { appState.openPatternOverlay() }) {
                    Text("Pat \(String(Character(UnicodeScalar(65 + appState.currentPatternIndex)!)))")
                        .font(.custom("Barlow Condensed", size: 11))
                        .foregroundColor(Color.black)
                }
                
                Text("•")
                    .foregroundColor(Color.black.opacity(0.3))
                
                Text("\(Int(appState.bpm)) BPM")
                    .font(.custom("Barlow Condensed", size: 11))
                    .foregroundColor(Color.black)
                
                Text("•")
                    .foregroundColor(Color.black.opacity(0.3))
                
                Text("\(Int(appState.swing))%")
                    .font(.custom("Barlow Condensed", size: 11))
                    .foregroundColor(Color.black)
                
                Text("•")
                    .foregroundColor(Color.black.opacity(0.3))
                
                Text("I\(appState.armedInstrumentIndex + 1)")
                    .font(.custom("Barlow Condensed", size: 11))
                    .fontWeight(.medium)
                    .foregroundColor(Color.blue)
                
                Text(appState.cpuUsage)
                    .font(.custom("Barlow Condensed", size: 11))
                    .foregroundColor(Color.black)
            }
        }
        .padding(.horizontal, 12)
        .background(Color(red: 0.94, green: 0.96, blue: 0.98))
    }
}

// MARK: - Option Line View (16 buttons in quads)
struct OptionLineView: View {
    @EnvironmentObject var appState: EtherSynthState
    
    private let optionLabels = [
        "INST", "SOUND", "PATT", "CLONE",  // Quad A - Pattern/Instrument
        "COPY", "CUT", "DEL", "NUDGE",     // Quad B - Tools
        "ACC", "RATCH", "TIE", "MICRO",   // Quad C - Stamps
        "FX", "CHORD", "SWG", "SCENE"      // Quad D - Performance
    ]
    
    var body: some View {
        HStack(spacing: 2) {
            ForEach(0..<16, id: \.self) { index in
                let quadIndex = index / 4
                let isActive = getButtonActiveState(index)
                
                Button(action: {
                    appState.triggerOptionButton(index)
                }) {
                    Text(optionLabels[index])
                        .font(.custom("Barlow Condensed", size: 9))
                        .foregroundColor(Color.black)
                        .frame(maxWidth: .infinity, maxHeight: .infinity)
                }
                .frame(width: UIConstants.optionButtonWidth, height: 36)
                .background(
                    isActive ? 
                    UIConstants.quadColors[quadIndex].opacity(0.8) :
                    UIConstants.quadColors[quadIndex].opacity(0.3)
                )
                .cornerRadius(6)
            }
        }
        .padding(.horizontal, 6)
        .background(Color(red: 0.96, green: 0.97, blue: 0.98))
    }
    
    private func getButtonActiveState(_ index: Int) -> Bool {
        switch index {
        case 0: return appState.instButtonHeld
        case 4: return appState.currentTool == .copy
        case 5: return appState.currentTool == .cut
        case 6: return appState.currentTool == .delete
        case 7: return appState.currentTool == .nudge
        case 8: return appState.stamps.accent
        case 9: return appState.stamps.ratchet > 0
        case 10: return appState.stamps.tie
        case 11: return appState.stamps.micro != 0
        case 12: return appState.currentMode == .fx
        default: return false
        }
    }
}

// MARK: - View Area (Context-sensitive content)
struct ViewAreaView: View {
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        ZStack {
            Color(red: 0.98, green: 0.99, blue: 1.0)
            
            VStack {
                switch appState.currentMode {
                case .step:
                    StepModeView()
                case .pad:
                    PadModeView()
                case .fx:
                    FxModeView()
                case .mix:
                    MixModeView()
                case .seq:
                    SeqModeView()
                case .chop:
                    ChopModeView()
                }
            }
        }
    }
}

// MARK: - Step Row (Always-live 16 steps)
struct StepRowView: View {
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        VStack(spacing: 4) {
            // Parameter mode overlay (shown when INST held)
            if appState.parameterState.isParameterMode {
                ParameterModeOverlay()
                    .transition(.opacity)
            }
            
            // Step buttons
            HStack(spacing: 2) {
                ForEach(0..<16, id: \.self) { stepIndex in
                    Button(action: {
                        if appState.parameterState.isParameterMode {
                            appState.selectParameter(stepIndex)
                        } else {
                            handleStepInput(stepIndex)
                        }
                    }) {
                        VStack(spacing: 1) {
                            if appState.parameterState.isParameterMode {
                                // Parameter mode: Show parameter names and values
                                Text(appState.parameterState.parameterNames[stepIndex])
                                    .font(.custom("Barlow Condensed", size: 7))
                                    .foregroundColor(Color.black)
                                    .lineLimit(1)
                                    .minimumScaleFactor(0.5)
                                
                                // Parameter value bar
                                let paramValue = appState.getParameterValue(stepIndex)
                                Rectangle()
                                    .fill(Color.blue)
                                    .frame(width: CGFloat(paramValue * 40), height: 3)
                                    .animation(.easeInOut(duration: 0.1), value: paramValue)
                                
                                Text(String(format: "%.0f", paramValue * 100))
                                    .font(.custom("Barlow Condensed", size: 6))
                                    .foregroundColor(Color.black.opacity(0.7))
                            } else {
                                // Step mode: Show step number and indicators
                                Text("\(stepIndex + 1)")
                                    .font(.custom("Barlow Condensed", size: 12))
                                    .foregroundColor(Color.black)
                                
                                // Step indicators (will be enhanced with real step data)
                                HStack(spacing: 1) {
                                    if stepIndex % 4 == 0 { // Mock: every 4th step has accent
                                        Circle()
                                            .fill(Color.orange)
                                            .frame(width: 3, height: 3)
                                    }
                                }
                                .frame(height: 4)
                            }
                        }
                    }
                    .frame(width: UIConstants.stepButtonWidth, height: UIConstants.stepButtonHeight)
                    .background(getStepBackgroundColor(stepIndex))
                    .cornerRadius(8)
                    .scaleEffect(getStepScale(stepIndex))
                    .animation(.spring(response: 0.2, dampingFraction: 0.8), value: appState.parameterState.selectedParameterIndex)
                }
            }
        }
        .padding(.horizontal, 6)
        .background(Color(red: 0.94, green: 0.95, blue: 0.96))
    }
    
    private func getStepBackgroundColor(_ stepIndex: Int) -> Color {
        if appState.parameterState.isParameterMode {
            if stepIndex == appState.parameterState.selectedParameterIndex {
                return Color.blue.opacity(0.7)
            } else {
                // Group parameters by function with subtle color coding
                let group = getParameterGroup(stepIndex)
                switch group {
                case 0: return Color(red: 0.95, green: 0.93, blue: 0.98) // Oscillator - Purple tint
                case 1: return Color(red: 0.93, green: 0.98, blue: 0.95) // Filter - Green tint  
                case 2: return Color(red: 0.98, green: 0.95, blue: 0.93) // Envelope - Orange tint
                case 3: return Color(red: 0.93, green: 0.95, blue: 0.98) // Modulation - Blue tint
                default: return Color.gray.opacity(0.3)
                }
            }
        } else {
            // Step mode coloring
            if stepIndex % 4 == 0 {
                return Color(red: 0.98, green: 0.97, blue: 0.95) // Beat emphasis
            } else {
                return Color.white.opacity(0.85)
            }
        }
    }
    
    private func getStepScale(_ stepIndex: Int) -> CGFloat {
        if appState.parameterState.isParameterMode && stepIndex == appState.parameterState.selectedParameterIndex {
            return 1.05
        } else {
            return 1.0
        }
    }
    
    private func getParameterGroup(_ paramIndex: Int) -> Int {
        // Group parameters by function (0-3 = osc, 4-7 = filter, etc.)
        return paramIndex / 4
    }
    
    private func handleStepInput(_ stepIndex: Int) {
        // Step input logic will be implemented
        print("Step \(stepIndex + 1) pressed")
    }
}

// MARK: - Footer View (Status and feedback)
struct FooterView: View {
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        HStack {
            // Stamp status
            HStack(spacing: 6) {
                Text("Stamps:")
                    .font(.custom("Barlow Condensed", size: 9))
                    .foregroundColor(Color.black.opacity(0.6))
                
                Text(appState.stamps.accent ? "ACCENT" : "—")
                    .font(.custom("Barlow Condensed", size: 9))
                    .foregroundColor(appState.stamps.accent ? Color.black : Color.black.opacity(0.4))
                
                Text(appState.stamps.ratchet > 0 ? "RATCH×\(appState.stamps.ratchet + 1)" : "—")
                    .font(.custom("Barlow Condensed", size: 9))
                    .foregroundColor(appState.stamps.ratchet > 0 ? Color.black : Color.black.opacity(0.4))
                
                Text(appState.stamps.tie ? "TIE" : "—")
                    .font(.custom("Barlow Condensed", size: 9))
                    .foregroundColor(appState.stamps.tie ? Color.black : Color.black.opacity(0.4))
                
                Text(appState.stamps.micro != 0 ? "MICRO\(appState.stamps.micro > 0 ? "+" : "-")" : "—")
                    .font(.custom("Barlow Condensed", size: 9))
                    .foregroundColor(appState.stamps.micro != 0 ? Color.black : Color.black.opacity(0.4))
            }
            
            Spacer()
            
            // Tool status
            if let tool = appState.currentTool {
                Text("Tool: \(tool.rawValue) (\(appState.currentScope.rawValue)) — PLAY to exit")
                    .font(.custom("Barlow Condensed", size: 9))
                    .foregroundColor(Color.black.opacity(0.7))
            }
        }
        .padding(.horizontal, 12)
        .background(Color(red: 0.92, green: 0.94, blue: 0.96))
    }
}

// MARK: - Mode Views (Placeholders)
struct StepModeView: View {
    var body: some View {
        Text("STEP Mode")
            .font(.custom("Barlow Condensed", size: 14))
    }
}

struct PadModeView: View {
    var body: some View {
        Text("PAD Mode")
            .font(.custom("Barlow Condensed", size: 14))
    }
}

struct FxModeView: View {
    var body: some View {
        Text("FX Mode")
            .font(.custom("Barlow Condensed", size: 14))
    }
}

struct MixModeView: View {
    var body: some View {
        Text("MIX Mode")
            .font(.custom("Barlow Condensed", size: 14))
    }
}

struct SeqModeView: View {
    var body: some View {
        Text("SEQ Mode")
            .font(.custom("Barlow Condensed", size: 14))
    }
}

struct ChopModeView: View {
    var body: some View {
        Text("CHOP Mode")
            .font(.custom("Barlow Condensed", size: 14))
    }
}

// MARK: - Parameter Mode Overlay
struct ParameterModeOverlay: View {
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        VStack(spacing: 4) {
            HStack {
                Text("PARAMETER MODE • \(appState.getCurrentEngineID())")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(Color.black.opacity(0.7))
                    .fontWeight(.medium)
                
                Spacer()
                
                Text("SmartKnob: \(appState.parameterState.parameterNames[appState.parameterState.selectedParameterIndex])")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(Color.blue)
                    .fontWeight(.medium)
            }
            
            // SmartKnob simulation
            SmartKnobSimulator()
        }
        .padding(.horizontal, 8)
        .padding(.vertical, 4)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(Color.white.opacity(0.9))
                .shadow(color: Color.black.opacity(0.1), radius: 4, x: 0, y: 2)
        )
    }
}

// MARK: - SmartKnob Simulator
struct SmartKnobSimulator: View {
    @EnvironmentObject var appState: EtherSynthState
    @GestureState private var dragAmount = CGSize.zero
    @State private var lastValue: Float = 0.5
    
    var body: some View {
        HStack(spacing: 12) {
            // Visual knob
            ZStack {
                // Knob body
                Circle()
                    .fill(
                        LinearGradient(
                            gradient: Gradient(colors: [Color.white, Color.gray.opacity(0.3)]),
                            startPoint: .topLeading,
                            endPoint: .bottomTrailing
                        )
                    )
                    .frame(width: 40, height: 40)
                    .shadow(color: Color.black.opacity(0.2), radius: 2, x: 1, y: 1)
                
                // Knob indicator line
                let currentValue = appState.getParameterValue(appState.parameterState.selectedParameterIndex)
                let angle = Double(currentValue) * 270.0 - 135.0 // -135 to +135 degrees
                Rectangle()
                    .fill(Color.black)
                    .frame(width: 2, height: 15)
                    .offset(y: -7)
                    .rotationEffect(.degrees(angle))
                
                // Center dot
                Circle()
                    .fill(Color.black)
                    .frame(width: 4, height: 4)
            }
            .gesture(
                DragGesture()
                    .updating($dragAmount) { value, state, _ in
                        state = value.translation
                    }
                    .onChanged { value in
                        let sensitivity: Float = 0.005
                        let deltaY = Float(-value.translation.height) * sensitivity
                        let newValue = lastValue + deltaY
                        let clampedValue = max(0.0, min(1.0, newValue))
                        
                        appState.setParameterValue(appState.parameterState.selectedParameterIndex, clampedValue)
                    }
                    .onEnded { _ in
                        lastValue = appState.getParameterValue(appState.parameterState.selectedParameterIndex)
                    }
            )
            
            // Parameter info
            VStack(alignment: .leading, spacing: 2) {
                Text(appState.parameterState.parameterNames[appState.parameterState.selectedParameterIndex])
                    .font(.custom("Barlow Condensed", size: 11))
                    .fontWeight(.medium)
                    .foregroundColor(Color.black)
                
                let currentValue = appState.getParameterValue(appState.parameterState.selectedParameterIndex)
                Text("\(Int(currentValue * 100))%")
                    .font(.custom("Barlow Condensed", size: 14))
                    .fontWeight(.bold)
                    .foregroundColor(Color.blue)
                
                // Parameter range hint
                Text(getParameterHint(appState.parameterState.selectedParameterIndex))
                    .font(.custom("Barlow Condensed", size: 9))
                    .foregroundColor(Color.black.opacity(0.6))
            }
            
            Spacer()
            
            // Parameter group indicator
            VStack(spacing: 1) {
                ForEach(0..<4, id: \.self) { group in
                    Rectangle()
                        .fill(getParameterGroupColor(group, appState.parameterState.selectedParameterIndex))
                        .frame(width: 20, height: 4)
                        .cornerRadius(2)
                }
            }
        }
    }
    
    private func getParameterHint(_ paramIndex: Int) -> String {
        let engineID = appState.getCurrentEngineID()
        switch engineID {
        case "MacroVA":
            let hints = ["Osc A/B Mix", "Cutoff+Harm", "Res+Drive", "Sub Level", "Detune Amt", "Env Attack", "Env Decay", "Env Sustain", "Env Release", "LFO Speed", "LFO Amount", "Mod Routing", "Reverb+Width", "Sat+Texture", "User Macro", "User Macro"]
            return paramIndex < hints.count ? hints[paramIndex] : "Parameter"
        case "MacroFM":
            let hints = ["FM Algorithm", "Carrier Ratio", "Mod Index", "Feedback", "Brightness", "Resonance", "Mod Envelope", "Car Envelope", "Env Attack", "Env Decay", "Env Sustain", "Env Release", "LFO Speed", "LFO Amount", "Output Drive", "FM Macro"]
            return paramIndex < hints.count ? hints[paramIndex] : "Parameter"
        default:
            return "Generic Param"
        }
    }
    
    private func getParameterGroupColor(_ group: Int, _ selectedParam: Int) -> Color {
        let currentGroup = selectedParam / 4
        if group == currentGroup {
            return Color.blue.opacity(0.8)
        } else {
            return Color.gray.opacity(0.3)
        }
    }
}

// MARK: - Non-Modal Overlay System
struct NonModalOverlay: View {
    @EnvironmentObject var appState: EtherSynthState
    let type: EtherSynthState.OverlayType
    
    var body: some View {
        VStack {
            switch type {
            case .sound(let trackIndex):
                SoundSelectOverlay(trackIndex: trackIndex)
            case .engine:
                EngineSelectOverlay()
            case .pattern(let trackIndex):
                PatternSelectOverlay(trackIndex: trackIndex)
            case .clone:
                CloneOverlay()
            case .fx:
                FXOverlay()
            case .scene:
                SceneOverlay()
            default:
                Text("Overlay: \(type)")
                    .font(.custom("Barlow Condensed", size: 12))
            }
        }
        .background(
            RoundedRectangle(cornerRadius: 12)
                .fill(Color.white.opacity(0.95))
                .shadow(color: Color.black.opacity(0.15), radius: 8, x: 0, y: 4)
        )
        .padding(8)
    }
}

// MARK: - Clone Overlay
struct CloneOverlay: View {
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        VStack(spacing: 8) {
            HStack {
                Text("CLONE PATTERN")
                    .font(.custom("Barlow Condensed", size: 11))
                    .foregroundColor(.black)
                Spacer()
                Button("✕") { appState.showingOverlay = false }
                    .foregroundColor(.black)
            }
            
            HStack(spacing: 4) {
                ForEach(0..<8, id: \.self) { trackIndex in
                    Button("I\(trackIndex + 1)") {
                        // Clone current pattern to this track
                        print("Clone to track I\(trackIndex + 1)")
                        appState.showingOverlay = false
                    }
                    .frame(width: 32, height: 24)
                    .background(Color.gray.opacity(0.3))
                    .cornerRadius(4)
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(.black)
                }
            }
            
            Text("Select destination track for clone operation")
                .font(.custom("Barlow Condensed", size: 10))
                .foregroundColor(.black.opacity(0.7))
        }
        .padding(12)
        .background(Color.white)
        .cornerRadius(8)
    }
}

// MARK: - FX Overlay

// Canonical FX types used by the UI menu
private enum FXType: Int, CaseIterable {
    case tape = 0
    case delay = 1
    case reverb = 2
    case filter = 3

    var displayName: String {
        switch self {
        case .tape: return "TAPE"
        case .delay: return "DELAY"
        case .reverb: return "REVERB"
        case .filter: return "FILTER"
        }
    }

    var color: Color {
        switch self {
        case .tape: return .orange
        case .delay: return .green
        case .reverb: return .blue
        case .filter: return .purple
        }
    }

    // Sensible default insertion slots for each FX type
    var defaultSlot: Int {
        switch self {
        case .tape: return 1      // POST_ENGINE
        case .delay: return 3     // SEND_2
        case .reverb: return 2    // SEND_1
        case .filter: return 0    // PRE_FILTER
        }
    }
}

struct FXOverlay: View {
    @EnvironmentObject var appState: EtherSynthState
    @State private var selectedFX: FXType = .tape
    @State private var showingParameters: Bool = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("PROFESSIONAL EFFECTS — 16 Key Control")
                    .font(.custom("Barlow Condensed", size: 12))
                    .foregroundColor(Color(red: 0.04, green: 0.07, blue: 0.13))
                
                Spacer()
                
                Button(action: {
                    appState.showingOverlay = false
                }) {
                    Text("✕")
                        .font(.custom("Barlow Condensed", size: 14))
                        .foregroundColor(Color(red: 0.04, green: 0.07, blue: 0.13))
                        .frame(width: 24, height: 24)
                        .background(Color(red: 0.96, green: 0.97, blue: 0.98))
                        .cornerRadius(12)
                }
                .buttonStyle(PlainButtonStyle())
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(Color(red: 0.96, green: 0.97, blue: 0.98))
            
            // Effect Type Selection (single source of truth)
            HStack(spacing: 8) {
                Menu {
                    ForEach(FXType.allCases, id: \.rawValue) { fx in
                        Button(fx.displayName) {
                            selectedFX = fx
                            let _ = appState.addEffect(type: fx.rawValue, slot: fx.defaultSlot)
                            showingParameters = true
                        }
                    }
                } label: {
                    HStack(spacing: 6) {
                        Circle().fill(selectedFX.color).frame(width: 10, height: 10)
                        Text(selectedFX.displayName)
                            .font(.custom("Barlow Condensed", size: 11))
                            .foregroundColor(.white)
                            .padding(.horizontal, 10)
                            .padding(.vertical, 6)
                            .background(selectedFX.color)
                            .cornerRadius(6)
                    }
                }

                // Quick-pick buttons (kept for speed, driven by same enum)
                ForEach(FXType.allCases, id: \.rawValue) { fx in
                    Button(action: {
                        selectedFX = fx
                        let _ = appState.addEffect(type: fx.rawValue, slot: fx.defaultSlot)
                        showingParameters = true
                    }) {
                        Text(fx.displayName)
                            .font(.custom("Barlow Condensed", size: 11))
                            .foregroundColor(.white)
                            .frame(width: 70, height: 28)
                            .background((selectedFX == fx) ? fx.color : fx.color.opacity(0.6))
                            .cornerRadius(6)
                    }
                    .buttonStyle(PlainButtonStyle())
                }
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            
            // Performance Effects Section
            VStack(spacing: 6) {
                Text("PERFORMANCE CONTROLS")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(.black.opacity(0.7))
                
                HStack(spacing: 8) {
                    // Reverb Throw
                    Button(action: { appState.triggerReverbThrow() }) {
                        VStack(spacing: 2) {
                            Text("REV")
                            Text("THROW")
                        }
                        .font(.custom("Barlow Condensed", size: 9))
                        .foregroundColor(.white)
                        .frame(width: 50, height: 32)
                        .background(appState.reverbThrowActive ? Color.blue : Color.gray.opacity(0.6))
                        .cornerRadius(6)
                    }
                    .buttonStyle(PlainButtonStyle())
                    
                    // Delay Throw
                    Button(action: { appState.triggerDelayThrow() }) {
                        VStack(spacing: 2) {
                            Text("DLY")
                            Text("THROW")
                        }
                        .font(.custom("Barlow Condensed", size: 9))
                        .foregroundColor(.white)
                        .frame(width: 50, height: 32)
                        .background(appState.delayThrowActive ? Color.green : Color.gray.opacity(0.6))
                        .cornerRadius(6)
                    }
                    .buttonStyle(PlainButtonStyle())
                    
                    // Note Repeat
                    Button(action: { appState.toggleNoteRepeat(division: 16) }) {
                        VStack(spacing: 2) {
                            Text("NOTE")
                            Text("RPT")
                        }
                        .font(.custom("Barlow Condensed", size: 9))
                        .foregroundColor(.white)
                        .frame(width: 50, height: 32)
                        .background(appState.noteRepeatActive ? Color.orange : Color.gray.opacity(0.6))
                        .cornerRadius(6)
                    }
                    .buttonStyle(PlainButtonStyle())
                }
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            
            // Send Levels
            VStack(spacing: 4) {
                Text("SEND LEVELS")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(.black.opacity(0.7))
                
                HStack(spacing: 12) {
                    VStack {
                        Text("REV")
                            .font(.custom("Barlow Condensed", size: 9))
                        
                        VStack {
                            Text(String(format: "%.1f", appState.reverbSend * 100))
                                .font(.custom("Barlow Condensed", size: 8))
                            
                            Slider(value: Binding(
                                get: { appState.reverbSend },
                                set: { appState.setReverbSend(level: $0) }
                            ), in: 0...1)
                            .frame(width: 60)
                        }
                    }
                    
                    VStack {
                        Text("DLY")
                            .font(.custom("Barlow Condensed", size: 9))
                        
                        VStack {
                            Text(String(format: "%.1f", appState.delaySend * 100))
                                .font(.custom("Barlow Condensed", size: 8))
                            
                            Slider(value: Binding(
                                get: { appState.delaySend },
                                set: { appState.setDelaySend(level: $0) }
                            ), in: 0...1)
                            .frame(width: 60)
                        }
                    }
                }
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 6)
            
            // Parameter Control Interface (when effect selected)
            if showingParameters && appState.currentEffectID != 0 {
                Divider()
                EffectParameterInterface(effectId: appState.currentEffectID, effectType: selectedFX.rawValue)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 8)
            }
            
            // Presets
            VStack(spacing: 4) {
                Text("PRESETS")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(.black.opacity(0.7))
                
                HStack(spacing: 4) {
                    ForEach(0..<6) { slot in
                        Button(action: { 
                            let _ = appState.loadEffectPreset(slot: slot)
                        }) {
                            Text(appState.effectPresets[safe: slot] ?? "Empty")
                                .font(.custom("Barlow Condensed", size: 9))
                                .foregroundColor(.black)
                                .frame(width: 50, height: 20)
                                .background(appState.selectedEffectPreset == slot ? Color.yellow.opacity(0.3) : Color.gray.opacity(0.2))
                                .cornerRadius(4)
                        }
                        .buttonStyle(PlainButtonStyle())
                    }
                }
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
        }
        .frame(width: 460, height: 320)
        .background(Color.white)
        .cornerRadius(14)
        .shadow(color: Color.black.opacity(0.15), radius: 16, x: 0, y: 8)
    }
}

// MARK: - Effect Type Button
struct EffectTypeButton: View {
    let title: String
    let type: Int
    let isSelected: Bool
    let color: Color
    let action: () -> Void
    
    var body: some View {
        Button(action: action) {
            Text(title)
                .font(.custom("Barlow Condensed", size: 11))
                .foregroundColor(.white)
                .frame(width: 80, height: 28)
                .background(isSelected ? color : color.opacity(0.6))
                .cornerRadius(6)
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - Effect Parameter Interface
struct EffectParameterInterface: View {
    let effectId: UInt32
    let effectType: Int
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        VStack(spacing: 6) {
            Text("16-KEY PARAMETER CONTROL")
                .font(.custom("Barlow Condensed", size: 10))
                .foregroundColor(.black.opacity(0.7))
            
            // 16 parameter controls in 4x4 grid
            VStack(spacing: 4) {
                ForEach(0..<4) { row in
                    HStack(spacing: 6) {
                        ForEach(0..<4) { col in
                            let keyIndex = row * 4 + col
                            let paramName = appState.getEffectParameterName(effectId: effectId, keyIndex: keyIndex)
                            let currentValue = appState.getEffectParameter(effectId: effectId, keyIndex: keyIndex)
                            
                            VStack(spacing: 2) {
                                Text(paramName)
                                    .font(.custom("Barlow Condensed", size: 8))
                                    .foregroundColor(.black)
                                
                                Text(String(format: "%.2f", currentValue))
                                    .font(.custom("Barlow Condensed", size: 7))
                                    .foregroundColor(.black.opacity(0.7))
                                
                                // Visual knob representation
                                Circle()
                                    .stroke(Color.gray.opacity(0.3), lineWidth: 1)
                                    .frame(width: 24, height: 24)
                                    .overlay(
                                        Circle()
                                            .fill(getEffectTypeColor(effectType))
                                            .frame(width: 4, height: 4)
                                            .offset(y: -8)
                                            .rotationEffect(.degrees(Double(currentValue) * 270 - 135))
                                    )
                                    .onTapGesture {
                                        // Cycle through preset values for demo
                                        let newValue = currentValue >= 0.8 ? 0.0 : currentValue + 0.25
                                        appState.setEffectParameter(effectId: effectId, keyIndex: keyIndex, value: newValue)
                                    }
                            }
                            .frame(width: 40)
                        }
                    }
                }
            }
            
            Text("Use 16-key interface to control these parameters")
                .font(.custom("Barlow Condensed", size: 8))
                .foregroundColor(.black.opacity(0.5))
        }
    }
    
    private func getEffectTypeColor(_ type: Int) -> Color {
        switch type {
        case 0: return Color.orange  // Tape
        case 1: return Color.green   // Delay
        case 2: return Color.blue    // Reverb  
        case 3: return Color.purple  // Filter
        default: return Color.gray
        }
    }
}

// Helper extension for safe array access
extension Array {
    subscript(safe index: Index) -> Element? {
        return indices.contains(index) ? self[index] : nil
    }
}

// MARK: - Scene Overlay
struct SceneOverlay: View {
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        VStack(spacing: 8) {
            HStack {
                Text("SCENE SNAPSHOTS A–D")
                    .font(.custom("Barlow Condensed", size: 11))
                    .foregroundColor(.black)
                Spacer()
                Button("✕") { appState.showingOverlay = false }
                    .foregroundColor(.black)
            }
            
            HStack(spacing: 4) {
                ForEach(["A", "B", "C", "D"], id: \.self) { scene in
                    Button(scene) {
                        print("Load scene \(scene)")
                        appState.showingOverlay = false
                    }
                    .frame(width: 32, height: 24)
                    .background(Color.gray.opacity(0.3))
                    .cornerRadius(4)
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(.black)
                }
            }
            
            Text("Scene management: mixer + macro states")
                .font(.custom("Barlow Condensed", size: 10))
                .foregroundColor(.black.opacity(0.7))
        }
        .padding(12)
        .background(Color.white)
        .cornerRadius(8)
    }
}

// MARK: - Enhanced Chord System Overlay (Bicep Mode)
struct ChordOverlay: View {
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        VStack(spacing: 8) {
            // Header
            HStack {
                Text("CHORD SYSTEM (BICEP MODE)")
                    .font(.system(size: 12, weight: .bold))
                    .foregroundColor(.primary)
                Spacer()
                Button("×") {
                    appState.closeOverlay()
                }
                .font(.system(size: 16, weight: .bold))
                .foregroundColor(.primary)
            }
            .padding(.horizontal, 16)
            .padding(.top, 12)
            
            ScrollView {
                VStack(spacing: 16) {
                    // Chord System Enable
                    VStack(alignment: .leading, spacing: 8) {
                        HStack {
                            Text("CHORD SYSTEM")
                                .font(.system(size: 10, weight: .bold))
                                .foregroundColor(.secondary)
                            Spacer()
                            Button(appState.chordSystemEnabled ? "ENABLED" : "DISABLED") {
                                appState.enableChordSystem(enabled: !appState.chordSystemEnabled)
                            }
                            .font(.system(size: 10, weight: .bold))
                            .foregroundColor(appState.chordSystemEnabled ? .green : .secondary)
                            .padding(.horizontal, 8)
                            .padding(.vertical, 4)
                            .background(appState.chordSystemEnabled ? Color.green.opacity(0.2) : Color.gray.opacity(0.2))
                            .cornerRadius(4)
                        }
                    }
                    
                    if appState.chordSystemEnabled {
                        // Chord Type & Root Selection
                        VStack(alignment: .leading, spacing: 8) {
                            Text("CHORD SELECTION")
                                .font(.system(size: 10, weight: .bold))
                                .foregroundColor(.secondary)
                            
                            HStack {
                                // Chord Type
                                VStack(alignment: .leading) {
                                    Text("TYPE")
                                        .font(.system(size: 9))
                                        .foregroundColor(.secondary)
                                    Menu {
                                        ForEach(0..<8, id: \.self) { chordType in
                                            Button(chordTypeName(chordType)) {
                                                appState.setChordType(chordType: chordType)
                                            }
                                        }
                                    } label: {
                                        Text(chordTypeName(appState.currentChordType))
                                            .font(.system(size: 10, weight: .medium))
                                            .padding(.horizontal, 8)
                                            .padding(.vertical, 4)
                                            .background(Color.blue.opacity(0.2))
                                            .cornerRadius(4)
                                    }
                                }
                                
                                // Root Note
                                VStack(alignment: .leading) {
                                    Text("ROOT")
                                        .font(.system(size: 9))
                                        .foregroundColor(.secondary)
                                    Menu {
                                        ForEach(Array(stride(from: 48, to: 73, by: 1)), id: \.self) { note in
                                            Button(noteNumberToName(note)) {
                                                appState.setChordRoot(rootNote: note)
                                            }
                                        }
                                    } label: {
                                        Text(noteNumberToName(appState.currentChordRoot))
                                            .font(.system(size: 10, weight: .medium))
                                            .padding(.horizontal, 8)
                                            .padding(.vertical, 4)
                                            .background(Color.orange.opacity(0.2))
                                            .cornerRadius(4)
                                    }
                                }
                                
                                Spacer()
                                
                                // Play/Release buttons
                                HStack {
                                    Button("PLAY") {
                                        appState.playChord()
                                    }
                                    .font(.system(size: 10, weight: .bold))
                                    .foregroundColor(.white)
                                    .padding(.horizontal, 12)
                                    .padding(.vertical, 6)
                                    .background(Color.green)
                                    .cornerRadius(4)
                                    
                                    Button("STOP") {
                                        appState.releaseChord()
                                    }
                                    .font(.system(size: 10, weight: .bold))
                                    .foregroundColor(.white)
                                    .padding(.horizontal, 12)
                                    .padding(.vertical, 6)
                                    .background(Color.red)
                                    .cornerRadius(4)
                                }
                            }
                        }
                        
                        // Voice Configuration
                        VStack(alignment: .leading, spacing: 8) {
                            Text("VOICE CONFIGURATION (5 VOICES)")
                                .font(.system(size: 10, weight: .bold))
                                .foregroundColor(.secondary)
                            
                            ForEach(0..<5, id: \.self) { voiceIndex in
                                VStack(spacing: 4) {
                                    HStack {
                                        // Voice number and enable
                                        HStack {
                                            Button(appState.chordVoiceActive[voiceIndex] ? "●" : "○") {
                                                appState.enableVoice(voiceIndex: voiceIndex, enabled: !appState.chordVoiceActive[voiceIndex])
                                            }
                                            .foregroundColor(appState.chordVoiceActive[voiceIndex] ? .green : .gray)
                                            .font(.system(size: 12))
                                            
                                            Text("V\(voiceIndex + 1)")
                                                .font(.system(size: 9, weight: .bold))
                                                .frame(width: 20)
                                        }
                                        
                                        // Engine selection
                                        Menu {
                                            ForEach(0..<6, id: \.self) { engineType in
                                                Button(engineTypeName(engineType)) {
                                                    appState.setVoiceEngine(voiceIndex: voiceIndex, engineType: engineType)
                                                }
                                            }
                                        } label: {
                                            Text(engineTypeName(appState.chordVoiceEngines[voiceIndex]))
                                                .font(.system(size: 9))
                                                .padding(.horizontal, 6)
                                                .padding(.vertical, 2)
                                                .background(engineColor(appState.chordVoiceEngines[voiceIndex]).opacity(0.3))
                                                .cornerRadius(3)
                                        }
                                        
                                        // Role selection
                                        Menu {
                                            ForEach(0..<5, id: \.self) { role in
                                                Button(voiceRoleName(role)) {
                                                    appState.setVoiceRole(voiceIndex: voiceIndex, role: role)
                                                }
                                            }
                                        } label: {
                                            Text(voiceRoleName(appState.chordVoiceRoles[voiceIndex]))
                                                .font(.system(size: 9))
                                                .padding(.horizontal, 6)
                                                .padding(.vertical, 2)
                                                .background(Color.purple.opacity(0.3))
                                                .cornerRadius(3)
                                        }
                                        
                                        Spacer()
                                        
                                        // Level indicator
                                        Text("\(Int(appState.chordVoiceLevels[voiceIndex] * 100))%")
                                            .font(.system(size: 8))
                                            .foregroundColor(.secondary)
                                    }
                                    
                                    // Level and pan controls
                                    HStack {
                                        Text("LVL")
                                            .font(.system(size: 8))
                                            .frame(width: 24)
                                        Slider(value: Binding(
                                            get: { appState.chordVoiceLevels[voiceIndex] },
                                            set: { appState.setVoiceLevel(voiceIndex: voiceIndex, level: $0) }
                                        ), in: 0...1)
                                        .accentColor(.green)
                                        
                                        Text("PAN")
                                            .font(.system(size: 8))
                                            .frame(width: 24)
                                        Slider(value: Binding(
                                            get: { appState.chordVoicePans[voiceIndex] },
                                            set: { appState.setVoicePan(voiceIndex: voiceIndex, pan: $0) }
                                        ), in: -1...1)
                                        .accentColor(.blue)
                                    }
                                }
                                .padding(.vertical, 4)
                                .padding(.horizontal, 8)
                                .background(voiceIndex % 2 == 0 ? Color.clear : Color.gray.opacity(0.1))
                                .cornerRadius(4)
                            }
                        }
                        
                        // Multi-Instrument Assignment Matrix
                        VStack(alignment: .leading, spacing: 8) {
                            Text("MULTI-INSTRUMENT ASSIGNMENT")
                                .font(.system(size: 10, weight: .bold))
                                .foregroundColor(.secondary)
                            
                            HStack {
                                Text("TRACK")
                                    .font(.system(size: 9, weight: .bold))
                                    .frame(width: 40)
                                Text("VOICES")
                                    .font(.system(size: 9, weight: .bold))
                                Text("STRUM")
                                    .font(.system(size: 9, weight: .bold))
                                    .frame(width: 50)
                                Text("ARP")
                                    .font(.system(size: 9, weight: .bold))
                                    .frame(width: 40)
                            }
                            .foregroundColor(.secondary)
                            
                            ForEach(0..<8, id: \.self) { trackIndex in
                                HStack {
                                    Text("T\(trackIndex + 1)")
                                        .font(.system(size: 9, weight: .bold))
                                        .frame(width: 40)
                                        .foregroundColor(.primary)
                                    
                                    // Voice assignment buttons
                                    HStack(spacing: 2) {
                                        ForEach(0..<5, id: \.self) { voiceIndex in
                                            Button("\(voiceIndex + 1)") {
                                                toggleVoiceAssignment(trackIndex: trackIndex, voiceIndex: voiceIndex)
                                            }
                                            .font(.system(size: 8, weight: .bold))
                                            .foregroundColor(appState.chordInstrumentAssignments[trackIndex].contains(voiceIndex) ? .white : .gray)
                                            .frame(width: 16, height: 16)
                                            .background(appState.chordInstrumentAssignments[trackIndex].contains(voiceIndex) ? Color.blue : Color.clear)
                                            .overlay(
                                                RoundedRectangle(cornerRadius: 2)
                                                    .stroke(Color.gray, lineWidth: 1)
                                            )
                                            .cornerRadius(2)
                                        }
                                    }
                                    
                                    // Strum offset
                                    Text("\(Int(appState.chordStrumOffsets[trackIndex]))ms")
                                        .font(.system(size: 8))
                                        .frame(width: 50)
                                        .foregroundColor(.secondary)
                                    
                                    // Arpeggio indicator
                                    Button(appState.chordArpeggioModes[trackIndex] ? "ON" : "OFF") {
                                        appState.setInstrumentArpeggio(
                                            instrumentIndex: trackIndex, 
                                            enabled: !appState.chordArpeggioModes[trackIndex]
                                        )
                                    }
                                    .font(.system(size: 8, weight: .bold))
                                    .foregroundColor(appState.chordArpeggioModes[trackIndex] ? .white : .gray)
                                    .frame(width: 40, height: 16)
                                    .background(appState.chordArpeggioModes[trackIndex] ? Color.orange : Color.clear)
                                    .overlay(
                                        RoundedRectangle(cornerRadius: 2)
                                            .stroke(Color.gray, lineWidth: 1)
                                    )
                                    .cornerRadius(2)
                                }
                            }
                        }
                        
                        // Arrangement Presets
                        VStack(alignment: .leading, spacing: 8) {
                            Text("ARRANGEMENT PRESETS")
                                .font(.system(size: 10, weight: .bold))
                                .foregroundColor(.secondary)
                            
                            ScrollView(.horizontal, showsIndicators: false) {
                                HStack(spacing: 8) {
                                    ForEach(0..<appState.chordArrangementPresets.count, id: \.self) { presetIndex in
                                        Button(appState.chordArrangementPresets[presetIndex]) {
                                            appState.selectedChordPreset = presetIndex
                                            appState.loadChordArrangementPreset(presetName: appState.chordArrangementPresets[presetIndex])
                                        }
                                        .font(.system(size: 9, weight: .medium))
                                        .foregroundColor(appState.selectedChordPreset == presetIndex ? .white : .primary)
                                        .padding(.horizontal, 8)
                                        .padding(.vertical, 4)
                                        .background(appState.selectedChordPreset == presetIndex ? Color.blue : Color.gray.opacity(0.2))
                                        .cornerRadius(4)
                                    }
                                }
                                .padding(.horizontal)
                            }
                        }
                        
                        // Performance Stats
                        HStack {
                            Text("CPU: \(Int(appState.chordCPUUsage * 100))%")
                                .font(.system(size: 8))
                                .foregroundColor(.secondary)
                            Spacer()
                            Text("Voices: \(appState.chordActiveVoices)")
                                .font(.system(size: 8))
                                .foregroundColor(.secondary)
                        }
                        .padding(.horizontal)
                    }
                }
                .padding(.horizontal)
            }
        }
        .frame(width: UIConstants.screenWidth * 0.9, height: UIConstants.screenHeight * 0.8)
        .background(Color(.systemBackground))
        .cornerRadius(8)
        .onAppear {
            appState.updateChordStats()
        }
    }
    
    // Helper functions for UI display
    private func chordTypeName(_ type: Int) -> String {
        let names = ["Major", "Major 6", "Major 7", "Major 9", "Minor", "Minor 7", "Minor 9", "Dominant 7"]
        return names[min(type, names.count - 1)]
    }
    
    private func noteNumberToName(_ noteNumber: Int) -> String {
        let notes = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
        let octave = noteNumber / 12 - 1
        let note = notes[noteNumber % 12]
        return "\(note)\(octave)"
    }
    
    private func engineTypeName(_ type: Int) -> String {
        let names = ["MacroVA", "MacroFM", "MacroWT", "MacroWS", "MacroChord", "MacroHarm"]
        return names[min(type, names.count - 1)]
    }
    
    private func engineColor(_ type: Int) -> Color {
        let colors = [Color.red, Color.orange, Color.blue, Color.pink, Color.cyan, Color.yellow]
        return colors[min(type, colors.count - 1)]
    }
    
    private func voiceRoleName(_ role: Int) -> String {
        let names = ["ROOT", "HARM1", "HARM2", "COLOR", "DOUBLE"]
        return names[min(role, names.count - 1)]
    }
    
    private func toggleVoiceAssignment(trackIndex: Int, voiceIndex: Int) {
        var currentAssignments = appState.chordInstrumentAssignments[trackIndex]
        if currentAssignments.contains(voiceIndex) {
            currentAssignments.removeAll { $0 == voiceIndex }
        } else {
            currentAssignments.append(voiceIndex)
        }
        appState.assignInstrument(instrumentIndex: trackIndex, voiceIndices: currentAssignments)
    }
}

// MARK: - Sound Select Overlay (I1-I16)
struct SoundSelectOverlay: View {
    let trackIndex: Int
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        VStack(spacing: 8) {
            HStack {
                Text("SELECT INSTRUMENT I1–I16")
                    .font(.custom("Barlow Condensed", size: 11))
                    .fontWeight(.medium)
                    .foregroundColor(Color.black)
                
                Spacer()
                
                Button("Close") {
                    appState.closeOverlay()
                }
                .font(.custom("Barlow Condensed", size: 10))
            }
            
            // 4x4 grid of instruments
            LazyVGrid(columns: Array(repeating: GridItem(.flexible(), spacing: 2), count: 8), spacing: 2) {
                ForEach(0..<16, id: \.self) { instrumentIndex in
                    Button(action: {
                        appState.armedInstrumentIndex = instrumentIndex
                        appState.updateParameterNamesForCurrentEngine()
                        appState.closeOverlay()
                    }) {
                        VStack(spacing: 2) {
                            // Engine color chip
                            let track = appState.tracks[min(instrumentIndex, 7)]
                            RoundedRectangle(cornerRadius: 4)
                                .fill(track.engine?.color ?? Color.gray.opacity(0.3))
                                .frame(width: 16, height: 6)
                            
                            Text("I\(instrumentIndex + 1)")
                                .font(.custom("Barlow Condensed", size: 9))
                                .foregroundColor(Color.black)
                            
                            Text(track.engine?.name ?? "—")
                                .font(.custom("Barlow Condensed", size: 7))
                                .foregroundColor(Color.black.opacity(0.6))
                                .lineLimit(1)
                        }
                    }
                    .frame(width: 50, height: 36)
                    .background(
                        instrumentIndex == appState.armedInstrumentIndex ?
                        Color.blue.opacity(0.3) : Color.gray.opacity(0.1)
                    )
                    .cornerRadius(6)
                }
            }
        }
        .padding(12)
        .frame(width: 420, height: 140)
    }
}

// MARK: - Engine Select Overlay
struct EngineSelectOverlay: View {
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        VStack(spacing: 8) {
            HStack {
                Text("ASSIGN ENGINE • I\(appState.armedInstrumentIndex + 1)")
                    .font(.custom("Barlow Condensed", size: 11))
                    .fontWeight(.medium)
                    .foregroundColor(Color.black)
                
                Spacer()
                
                Button("Close") {
                    appState.closeOverlay()
                }
                .font(.custom("Barlow Condensed", size: 10))
            }
            
            // Engine grid
            LazyVGrid(columns: Array(repeating: GridItem(.flexible(), spacing: 2), count: 7), spacing: 2) {
                ForEach(EngineType.allEngines, id: \.id) { engine in
                    Button(action: {
                        if appState.armedInstrumentIndex < appState.tracks.count {
                            appState.tracks[appState.armedInstrumentIndex].engine = engine
                            appState.updateParameterNamesForCurrentEngine()
                        }
                        appState.closeOverlay()
                    }) {
                        VStack(spacing: 2) {
                            RoundedRectangle(cornerRadius: 4)
                                .fill(engine.color)
                                .frame(width: 20, height: 8)
                            
                            Text(engine.name)
                                .font(.custom("Barlow Condensed", size: 8))
                                .foregroundColor(Color.black)
                                .lineLimit(1)
                                .minimumScaleFactor(0.7)
                            
                            Text(engine.cpuClass)
                                .font(.custom("Barlow Condensed", size: 6))
                                .foregroundColor(Color.black.opacity(0.5))
                        }
                    }
                    .frame(width: 52, height: 40)
                    .background(Color.white.opacity(0.8))
                    .cornerRadius(6)
                }
            }
        }
        .padding(12)
        .frame(width: 420, height: 160)
    }
}

// MARK: - Pattern Select Overlay
struct PatternSelectOverlay: View {
    let trackIndex: Int
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        VStack(spacing: 8) {
            HStack {
                Text("SELECT PATTERN A–P")
                    .font(.custom("Barlow Condensed", size: 11))
                    .fontWeight(.medium)
                    .foregroundColor(Color.black)
                
                Spacer()
                
                Button("Close") {
                    appState.closeOverlay()
                }
                .font(.custom("Barlow Condensed", size: 10))
            }
            
            // Pattern grid A-P
            LazyVGrid(columns: Array(repeating: GridItem(.flexible(), spacing: 2), count: 8), spacing: 2) {
                ForEach(0..<16, id: \.self) { patternIndex in
                    let patternLetter = String(Character(UnicodeScalar(65 + patternIndex)!))
                    
                    Button(action: {
                        appState.currentPatternIndex = patternIndex
                        appState.closeOverlay()
                    }) {
                        VStack(spacing: 2) {
                            Text(patternLetter)
                                .font(.custom("Barlow Condensed", size: 14))
                                .fontWeight(.bold)
                                .foregroundColor(Color.black)
                            
                            // Pattern length indicator
                            Text("16") // Mock length
                                .font(.custom("Barlow Condensed", size: 8))
                                .foregroundColor(Color.black.opacity(0.6))
                        }
                    }
                    .frame(width: 40, height: 32)
                    .background(
                        patternIndex == appState.currentPatternIndex ?
                        Color.green.opacity(0.4) : Color.gray.opacity(0.2)
                    )
                    .cornerRadius(6)
                }
            }
            
            // Pattern length controls
            HStack {
                Text("Length:")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(Color.black)
                
                ForEach([16, 32, 48, 64], id: \.self) { length in
                    Button("\(length)") {
                        // Set pattern length
                    }
                    .font(.custom("Barlow Condensed", size: 9))
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(Color.blue.opacity(0.2))
                    .cornerRadius(4)
                }
            }
        }
        .padding(12)
        .frame(width: 380, height: 120)
    }
}

// MARK: - Modal Overlay View
struct ModalOverlayView: View {
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        ZStack {
            // Overlay background
            Color.black.opacity(0.3)
                .onTapGesture {
                    appState.closeOverlay()
                }
            
            // Modal content
            if let overlayType = appState.overlayType {
                switch overlayType {
                case .sound(let trackIndex):
                    SoundSelectOverlay(trackIndex: trackIndex)
                case .engine:
                    EngineSelectOverlay()
                case .pattern(let trackIndex):
                    PatternSelectOverlay(trackIndex: trackIndex)
                case .clone:
                    CloneOverlay()
                case .fx:
                    FXOverlay()
                case .scene:
                    SceneOverlay()
                case .chord:
                    ChordOverlay()
                case .assign(let trackIndex):
                    // Legacy - will be updated
                    Text("Assign overlay for track \(trackIndex)")
                        .padding()
                        .background(Color.white)
                        .cornerRadius(8)
                default:
                    Text("Overlay: \(overlayType)")
                        .padding()
                        .background(Color.white)
                        .cornerRadius(8)
                }
            }
        }
    }
}

// MARK: - LEGACY COMPONENTS (will be removed)

// MARK: - Top Bar (Transport Controls)
struct TopBar: View {
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        HStack(spacing: 8) {
            // Transport buttons
            Button(action: { appState.togglePlay() }) {
                Text(appState.isPlaying ? "⏸" : "▶")
                    .font(.system(size: 14, weight: .medium))
                    .foregroundColor(.white)
            }
            .frame(width: 28, height: 24)
            .background(appState.isPlaying ? Color(red: 0.52, green: 0.80, blue: 0.09) : Color(red: 0.88, green: 0.90, blue: 0.94))
            .cornerRadius(6)
            .shadow(color: Color.black.opacity(0.06), radius: 1, x: 0, y: 1)
            
            Button(action: { appState.toggleRecord() }) {
                Text("●")
                    .font(.system(size: 12, weight: .medium))
                    .foregroundColor(.white)
            }
            .frame(width: 28, height: 24)
            .background(appState.isRecording ? Color(red: 0.94, green: 0.27, blue: 0.27) : Color(red: 0.88, green: 0.90, blue: 0.94))
            .cornerRadius(6)
            .shadow(color: Color.black.opacity(0.06), radius: 1, x: 0, y: 1)
            
            // Separator
            Rectangle()
                .fill(Color.black.opacity(0.06))
                .frame(width: 1, height: 20)
                .padding(.horizontal, 8)
            
            // BPM/Swing
            Text("BPM \(Int(appState.bpm)) • Swing \(Int(appState.swing))%")
                .font(.custom("Barlow Condensed", size: 12))
                .foregroundColor(Color(red: 0.36, green: 0.42, blue: 0.49))
            
            Spacer()
            
            // Center info (Bar/Beat, CPU)
            HStack {
                Text("Bar \(appState.currentBar) • CPU \(appState.cpuUsage)")
                    .font(.custom("Barlow Condensed", size: 12))
                    .foregroundColor(Color(red: 0.36, green: 0.42, blue: 0.49))
            }
            .frame(maxWidth: .infinity)
            .position(x: 480, y: 12) // Center in 960px width
            
            Spacer()
            
            // Project name
            Text("Project: \(appState.projectName)")
                .font(.custom("Barlow Condensed", size: 12))
                .foregroundColor(Color(red: 0.36, green: 0.42, blue: 0.49))
        }
        .padding(.horizontal, 8)
        .background(
            LinearGradient(
                gradient: Gradient(colors: [
                    Color(red: 0.97, green: 0.98, blue: 0.99),
                    Color(red: 0.96, green: 0.97, blue: 0.98)
                ]),
                startPoint: .top,
                endPoint: .bottom
            )
        )
    }
}

// MARK: - Navigation Bar
struct NavigationBar: View {
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        HStack(spacing: 10) {
            Spacer()
            
            // Timeline tab
            NavigationTab(
                icon: "▥",
                viewName: "timeline",
                isActive: appState.currentView == "timeline"
            )
            
            // Mod tab
            NavigationTab(
                icon: "∿",
                viewName: "mod", 
                isActive: appState.currentView == "mod"
            )
            
            // Mix tab
            NavigationTab(
                icon: "▤",
                viewName: "mix",
                isActive: appState.currentView == "mix"
            )
            
            Spacer()
        }
        .frame(height: 36)
        .background(Color(red: 0.93, green: 0.95, blue: 0.97)) // --nav-bg color
    }
}

struct NavigationTab: View {
    let icon: String
    let viewName: String
    let isActive: Bool
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        Button(action: {
            appState.switchView(to: viewName)
        }) {
            Text(icon)
                .font(.system(size: 24))
                .foregroundColor(Color(red: 0.04, green: 0.07, blue: 0.13))
        }
        .frame(width: 48, height: 28)
        .background(Color.white)
        .cornerRadius(8)
        .shadow(
            color: Color.black.opacity(isActive ? 0.06 : 0.05),
            radius: isActive ? 2 : 1,
            x: 0,
            y: isActive ? 1 : -1
        )
    }
}

// MARK: - Timeline View
struct TimelineView: View {
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        VStack(spacing: 0) {
            // Header instruction
            HStack {
                Text("Tap a pattern to open Pattern • Tap engine chip to open Sound • Long-press header to Assign/Reassign")
                    .font(.custom("Barlow Condensed", size: 12))
                    .foregroundColor(Color(red: 0.36, green: 0.42, blue: 0.49))
                    .padding(.horizontal, 10)
                Spacer()
            }
            .frame(height: 24)
            .background(Color(red: 0.96, green: 0.97, blue: 0.98))
            
            // 8 track rows
            VStack(spacing: 0) {
                ForEach(0..<8, id: \.self) { trackIndex in
                    TrackRow(trackIndex: trackIndex)
                        .frame(height: 28) // 224 / 8 = 28px per track
                }
            }
        }
    }
}

struct TrackRow: View {
    let trackIndex: Int
    @EnvironmentObject var appState: EtherSynthState
    
    private var track: Track {
        appState.tracks[trackIndex]
    }
    
    var body: some View {
        HStack(spacing: 0) {
            // Track header (180px width)
            TrackHeader(trackIndex: trackIndex)
                .frame(width: 180)
            
            // Timeline grid (remaining width)
            TrackGrid(trackIndex: trackIndex)
                .frame(maxWidth: .infinity)
        }
        .background(
            Rectangle()
                .fill(Color.black.opacity(0.06))
                .frame(height: 1)
                .offset(y: -0.5),
            alignment: .top
        )
    }
}

struct TrackHeader: View {
    let trackIndex: Int
    @EnvironmentObject var appState: EtherSynthState
    
    private var track: Track {
        appState.tracks[trackIndex]
    }
    
    var body: some View {
        HStack {
            if let engine = track.engine {
                // Assigned track
                Button(action: {
                    appState.openSoundOverlay(for: trackIndex)
                }) {
                    HStack(spacing: 6) {
                        // Color chip
                        RoundedRectangle(cornerRadius: 4)
                            .fill(engine.color)
                            .frame(width: 14, height: 14)
                            .shadow(color: Color.black.opacity(0.06), radius: 1, x: 0, y: 1)
                        
                        VStack(alignment: .leading, spacing: 2) {
                            Text(engine.name)
                                .font(.custom("Barlow Condensed", size: 12))
                                .foregroundColor(Color(red: 0.04, green: 0.07, blue: 0.13))
                            
                            Text("Preset • Poly \(track.polyphony) • A/B/C")
                                .font(.custom("Barlow Condensed", size: 10))
                                .foregroundColor(Color(red: 0.36, green: 0.42, blue: 0.49))
                        }
                    }
                }
                .buttonStyle(PlainButtonStyle())
                
                Spacer()
                
                // Reassign button
                Button(action: {
                    appState.openAssignOverlay(for: trackIndex)
                }) {
                    Text("Reassign")
                        .font(.custom("Barlow Condensed", size: 11))
                        .foregroundColor(Color(red: 0.04, green: 0.07, blue: 0.13))
                        .padding(.horizontal, 8)
                        .padding(.vertical, 2)
                        .background(Color.white)
                        .cornerRadius(10)
                        .shadow(color: Color.black.opacity(0.06), radius: 1, x: 0, y: 1)
                }
                .buttonStyle(PlainButtonStyle())
                
            } else {
                // Unassigned track
                HStack(spacing: 6) {
                    Text("Unassigned")
                        .font(.custom("Barlow Condensed", size: 10))
                        .foregroundColor(Color(red: 0.36, green: 0.42, blue: 0.49))
                        .padding(.horizontal, 8)
                        .padding(.vertical, 2)
                        .background(Color(red: 0.89, green: 0.91, blue: 0.94))
                        .cornerRadius(10)
                        .shadow(color: Color.black.opacity(0.06), radius: 1, x: 0, y: 1)
                }
                
                Spacer()
                
                Button(action: {
                    appState.openAssignOverlay(for: trackIndex)
                }) {
                    Text("Assign")
                        .font(.custom("Barlow Condensed", size: 11))
                        .foregroundColor(Color(red: 0.04, green: 0.07, blue: 0.13))
                        .padding(.horizontal, 10)
                        .padding(.vertical, 2)
                        .background(Color.white)
                        .cornerRadius(10)
                        .shadow(color: Color.black.opacity(0.06), radius: 1, x: 0, y: 1)
                }
                .buttonStyle(PlainButtonStyle())
            }
        }
        .padding(.horizontal, 8)
        .padding(.vertical, 4)
        .background(Color(red: 0.96, green: 0.97, blue: 0.98))
        .onLongPressGesture {
            appState.openAssignOverlay(for: trackIndex)
        }
    }
}

struct TrackGrid: View {
    let trackIndex: Int
    @EnvironmentObject var appState: EtherSynthState
    
    private var track: Track {
        appState.tracks[trackIndex]
    }
    
    var body: some View {
        ZStack {
            // Grid background pattern (like HTML prototype)
            GridBackground()
            
            // Pattern blocks if engine is assigned
            if let engine = track.engine {
                PatternBlocks(engine: engine, trackIndex: trackIndex)
            }
        }
    }
}

struct GridBackground: View {
    var body: some View {
        // Recreate the CSS repeating-linear-gradient pattern
        Canvas { context, size in
            let stepWidth = size.width / 128
            let barWidth = size.width / 16
            let beatWidth = size.width / 64
            
            // Draw vertical grid lines
            for i in 0...128 {
                let x = CGFloat(i) * stepWidth
                let color: Color
                let alpha: Double
                
                if i % 16 == 0 {
                    color = Color(red: 0.89, green: 0.91, blue: 0.95)
                    alpha = 1.0
                } else if i % 4 == 0 {
                    color = Color(red: 0.93, green: 0.94, blue: 0.96)
                    alpha = 1.0
                } else {
                    color = Color(red: 0.96, green: 0.97, blue: 0.98)
                    alpha = 0.7
                }
                
                context.fill(
                    Path { path in
                        path.move(to: CGPoint(x: x, y: 0))
                        path.addLine(to: CGPoint(x: x + 1, y: 0))
                        path.addLine(to: CGPoint(x: x + 1, y: size.height))
                        path.addLine(to: CGPoint(x: x, y: size.height))
                    },
                    with: .color(color.opacity(alpha))
                )
            }
        }
        .background(Color.white)
    }
}

struct PatternBlocks: View {
    let engine: EngineType
    let trackIndex: Int
    @EnvironmentObject var appState: EtherSynthState
    @State private var showingNewPatternButton = false
    @State private var newPatternPosition: Int = 0
    
    private var track: Track {
        appState.tracks[trackIndex]
    }
    
    var body: some View {
        GeometryReader { geometry in
            ZStack(alignment: .leading) {
                // Existing patterns
                ForEach(track.patterns) { pattern in
                    PatternBlockView(
                        pattern: pattern,
                        engine: engine,
                        trackIndex: trackIndex,
                        timelineWidth: geometry.size.width
                    )
                }
                
                // New pattern button (shows when hovering over empty space)
                if showingNewPatternButton {
                    NewPatternButton(
                        trackIndex: trackIndex,
                        position: newPatternPosition,
                        onCancel: { showingNewPatternButton = false }
                    )
                    .position(x: CGFloat(newPatternPosition) / 128 * geometry.size.width + 20, y: geometry.size.height / 2)
                }
            }
            .contentShape(Rectangle()) // Make entire area tappable
            .onTapGesture { location in
                // Calculate step position from tap
                let stepPosition = Int((location.x / geometry.size.width) * 128)
                createPatternAtPosition(stepPosition)
            }
            .gesture(
                DragGesture(minimumDistance: 0)
                    .onChanged { value in
                        let stepPosition = Int((value.location.x / geometry.size.width) * 128)
                        if !isPositionOccupied(stepPosition) {
                            newPatternPosition = stepPosition
                            showingNewPatternButton = true
                        } else {
                            showingNewPatternButton = false
                        }
                    }
                    .onEnded { _ in
                        showingNewPatternButton = false
                    }
            )
        }
        .frame(height: 28)
    }
    
    private func createPatternAtPosition(_ position: Int) {
        // Only create if position is not occupied
        if !isPositionOccupied(position) {
            appState.createNewPattern(on: trackIndex, at: position)
        }
    }
    
    private func isPositionOccupied(_ position: Int) -> Bool {
        return track.patterns.contains { pattern in
            position >= pattern.startPosition && position < pattern.endPosition
        }
    }
}

struct PatternBlockView: View {
    let pattern: PatternBlock
    let engine: EngineType
    let trackIndex: Int
    let timelineWidth: CGFloat
    @EnvironmentObject var appState: EtherSynthState
    @State private var showingContextMenu = false
    @State private var dragOffset = CGSize.zero
    @State private var isDragging = false
    
    private var blockWidth: CGFloat {
        return (CGFloat(pattern.length) / 128) * timelineWidth
    }
    
    private var blockPosition: CGFloat {
        return (CGFloat(pattern.startPosition) / 128) * timelineWidth
    }
    
    var body: some View {
        Button(action: {
            if !isDragging {
                appState.openPatternOverlay(for: trackIndex)
                appState.selectedPatternID = pattern.id
            }
        }) {
            VStack(spacing: 2) {
                Text(pattern.patternID)
                    .font(.custom("Barlow Condensed", size: 11))
                    .foregroundColor(.black)
                    .fontWeight(.medium)
                
                // Small indicator for pattern content
                if !pattern.notes.isEmpty || !pattern.drumHits.isEmpty {
                    Rectangle()
                        .fill(Color.black.opacity(0.3))
                        .frame(height: 2)
                        .padding(.horizontal, 4)
                } else {
                    Rectangle()
                        .fill(Color.clear)
                        .frame(height: 2)
                }
            }
        }
        .frame(width: max(30, blockWidth - 6), height: 22)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(appState.selectedPatternID == pattern.id ? engine.color.opacity(0.9) : engine.color)
                .shadow(color: Color.black.opacity(0.2), radius: isDragging ? 4 : 1, x: 0, y: isDragging ? 2 : 1)
        )
        .scaleEffect(isDragging ? 1.05 : 1.0)
        .offset(x: blockPosition + dragOffset.width, y: dragOffset.height)
        .zIndex(isDragging ? 1 : 0)
        .buttonStyle(PlainButtonStyle())
        .contextMenu {
            PatternContextMenu(
                pattern: pattern,
                trackIndex: trackIndex,
                onCopy: { appState.copyPattern(pattern) },
                onClone: { appState.clonePattern(trackIndex: trackIndex, patternID: pattern.id) },
                onDelete: { appState.deletePattern(trackIndex: trackIndex, patternID: pattern.id) }
            )
        }
        .gesture(
            DragGesture(coordinateSpace: .local)
                .onChanged { value in
                    if !isDragging {
                        isDragging = true
                        appState.selectedPatternID = pattern.id
                    }
                    dragOffset = value.translation
                }
                .onEnded { value in
                    isDragging = false
                    
                    // Calculate new position based on drag
                    let newStepPosition = pattern.startPosition + Int((value.translation.width / timelineWidth) * 128)
                    appState.movePattern(trackIndex: trackIndex, patternID: pattern.id, to: newStepPosition)
                    
                    dragOffset = .zero
                }
        )
        .animation(.easeOut(duration: 0.2), value: isDragging)
        .animation(.easeOut(duration: 0.2), value: appState.selectedPatternID)
    }
}

struct NewPatternButton: View {
    let trackIndex: Int
    let position: Int
    let onCancel: () -> Void
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        Button(action: {
            appState.createNewPattern(on: trackIndex, at: position)
            onCancel()
        }) {
            HStack(spacing: 4) {
                Image(systemName: "plus.circle.fill")
                    .foregroundColor(.white)
                    .font(.system(size: 12))
                Text("New")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(.white)
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .fill(Color.blue)
                    .shadow(color: Color.black.opacity(0.3), radius: 2, x: 0, y: 1)
            )
        }
        .buttonStyle(PlainButtonStyle())
    }
}

struct PatternContextMenu: View {
    let pattern: PatternBlock
    let trackIndex: Int
    let onCopy: () -> Void
    let onClone: () -> Void
    let onDelete: () -> Void
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        Group {
            Button("Copy", action: onCopy)
            Button("Paste") {
                appState.pastePattern(on: trackIndex, at: pattern.endPosition + 4)
            }
            .disabled(appState.copiedPattern == nil)
            
            Button("Clone", action: onClone)
            
            Divider()
            
            Button("Delete", action: onDelete)
        }
    }
}


// MARK: - Mod View (Placeholder)
struct ModView: View {
    var body: some View {
        VStack {
            Text("Mod View")
                .font(.custom("Barlow Condensed", size: 16))
                .foregroundColor(Color(red: 0.04, green: 0.07, blue: 0.13))
            
            Text("LFO controls and modulation matrix will go here")
                .font(.custom("Barlow Condensed", size: 12))
                .foregroundColor(Color(red: 0.36, green: 0.42, blue: 0.49))
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(red: 0.97, green: 0.98, blue: 0.99))
    }
}

// MARK: - Mix View (Placeholder)
struct MixView: View {
    var body: some View {
        VStack {
            Text("Mix View")
                .font(.custom("Barlow Condensed", size: 16))
                .foregroundColor(Color(red: 0.04, green: 0.07, blue: 0.13))
            
            Text("8-track mixer with sends and master section will go here")
                .font(.custom("Barlow Condensed", size: 12))
                .foregroundColor(Color(red: 0.36, green: 0.42, blue: 0.49))
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(red: 0.97, green: 0.98, blue: 0.99))
    }
}

// MARK: - Overlay System
struct OverlayView: View {
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        ZStack {
            // Overlay background
            Color.black.opacity(0.24)
                .onTapGesture {
                    appState.closeOverlay()
                }
            
            // Modal content
            if let overlayType = appState.overlayType {
                switch overlayType {
                case .assign(let trackIndex):
                    AssignEngineOverlay(trackIndex: trackIndex)
                case .sound(let trackIndex):
                    SoundOverlay(trackIndex: trackIndex)
                case .pattern(let trackIndex):
                    PatternOverlay(trackIndex: trackIndex)
                }
            }
        }
    }
}

// MARK: - Assign Engine Overlay
struct AssignEngineOverlay: View {
    let trackIndex: Int
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Assign Engine — Track \(trackIndex + 1)")
                    .font(.custom("Barlow Condensed", size: 12))
                    .foregroundColor(Color(red: 0.04, green: 0.07, blue: 0.13))
                
                Spacer()
                
                Button(action: {
                    appState.closeOverlay()
                }) {
                    Text("Close")
                        .font(.custom("Barlow Condensed", size: 11))
                        .foregroundColor(Color(red: 0.04, green: 0.07, blue: 0.13))
                        .padding(.horizontal, 10)
                        .padding(.vertical, 6)
                        .background(Color(red: 0.96, green: 0.97, blue: 0.98))
                        .cornerRadius(10)
                        .shadow(color: Color.black.opacity(0.06), radius: 1, x: 0, y: 1)
                }
                .buttonStyle(PlainButtonStyle())
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 6)
            .background(Color(red: 0.96, green: 0.97, blue: 0.98))
            
            // Engine grid
            ScrollView {
                LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 5), spacing: 8) {
                    ForEach(EngineType.allEngines, id: \.id) { engine in
                        EngineCard(engine: engine, trackIndex: trackIndex)
                    }
                }
                .padding(8)
            }
            .frame(maxHeight: 172)
        }
        .frame(width: 820, height: 208)
        .background(Color.white)
        .cornerRadius(14)
        .shadow(color: Color.black.opacity(0.08), radius: 12, x: 0, y: 10)
    }
}

struct EngineCard: View {
    let engine: EngineType
    let trackIndex: Int
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        Button(action: {
            appState.assignEngine(engine, to: trackIndex)
        }) {
            VStack(alignment: .leading, spacing: 2) {
                HStack {
                    RoundedRectangle(cornerRadius: 4)
                        .fill(engine.color)
                        .frame(width: 14, height: 14)
                    
                    Text(engine.name)
                        .font(.custom("Barlow Condensed", size: 13))
                        .foregroundColor(Color(red: 0.04, green: 0.07, blue: 0.13))
                    
                    Spacer()
                }
                
                Text("CPU \(engine.cpuClass) • \(engine.stereo ? "Stereo" : "Mono")")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(Color(red: 0.36, green: 0.42, blue: 0.49))
            }
            .padding(8)
            .background(Color(red: 0.96, green: 0.97, blue: 0.98))
            .cornerRadius(12)
            .shadow(color: Color.black.opacity(0.06), radius: 1, x: 0, y: 1)
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - Sound Overlay (Placeholder)
struct SoundOverlay: View {
    let trackIndex: Int
    @EnvironmentObject var appState: EtherSynthState
    
    private var track: Track {
        appState.tracks[trackIndex]
    }
    
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("\(track.engine?.name ?? "Unknown") — Sound")
                    .font(.custom("Barlow Condensed", size: 12))
                    .foregroundColor(Color(red: 0.04, green: 0.07, blue: 0.13))
                
                Spacer()
                
                Button(action: {
                    appState.closeOverlay()
                }) {
                    Text("Close")
                        .font(.custom("Barlow Condensed", size: 11))
                        .padding(.horizontal, 10)
                        .padding(.vertical, 6)
                        .background(Color(red: 0.96, green: 0.97, blue: 0.98))
                        .cornerRadius(10)
                }
                .buttonStyle(PlainButtonStyle())
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 6)
            .background(Color(red: 0.96, green: 0.97, blue: 0.98))
            
            VStack {
                Text("Sound controls will go here")
                    .font(.custom("Barlow Condensed", size: 12))
                    .foregroundColor(Color(red: 0.36, green: 0.42, blue: 0.49))
                
                Text("Hero macros, extras, channel strip, inserts")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(Color(red: 0.36, green: 0.42, blue: 0.49))
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
        .frame(width: 820, height: 208)
        .background(Color.white)
        .cornerRadius(14)
        .shadow(color: Color.black.opacity(0.08), radius: 12, x: 0, y: 10)
    }
}

// MARK: - Advanced Pattern Management Overlay
struct PatternOverlay: View {
    let trackIndex: Int
    @EnvironmentObject var appState: EtherSynthState
    @State private var selectedTab: PatternTab = .chains
    @State private var selectedChainMode: Int = 0
    @State private var selectedPatternId: UInt32 = 1
    @State private var euclideanSteps: Int = 16
    @State private var euclideanPulses: Int = 4
    @State private var mutationAmount: Float = 0.3
    @State private var swingAmount: Float = 0.0
    
    enum PatternTab {
        case chains, variations, scenes, arrangement, ai
    }
    
    private var track: Track {
        appState.tracks[trackIndex]
    }
    
    private var currentPattern: UInt32 {
        appState.currentPatterns[trackIndex]
    }
    
    private var queuedPattern: UInt32 {
        appState.queuedPatterns[trackIndex]
    }
    
    private var chainModeNames: [String] {
        ["Manual", "Automatic", "Conditional", "Performance", "Arrangement"]
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("PATTERN MANAGEMENT — Track \(trackIndex + 1)")
                    .font(.custom("Barlow Condensed", size: 12))
                    .foregroundColor(Color(red: 0.04, green: 0.07, blue: 0.13))
                
                Spacer()
                
                // Performance mode indicator
                if appState.performanceMode {
                    Text("PERFORMANCE")
                        .font(.custom("Barlow Condensed", size: 9))
                        .foregroundColor(.white)
                        .padding(.horizontal, 6)
                        .padding(.vertical, 2)
                        .background(Color.orange)
                        .cornerRadius(4)
                }
                
                Button(action: {
                    appState.closeOverlay()
                }) {
                    Text("✕")
                        .font(.custom("Barlow Condensed", size: 14))
                        .foregroundColor(Color(red: 0.04, green: 0.07, blue: 0.13))
                        .frame(width: 24, height: 24)
                        .background(Color(red: 0.96, green: 0.97, blue: 0.98))
                        .cornerRadius(12)
                }
                .buttonStyle(PlainButtonStyle())
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(Color(red: 0.96, green: 0.97, blue: 0.98))
            
            // Tab Navigation
            HStack(spacing: 0) {
                PatternTabButton(title: "CHAINS", tab: .chains, isSelected: selectedTab == .chains) {
                    selectedTab = .chains
                }
                
                PatternTabButton(title: "VARIATIONS", tab: .variations, isSelected: selectedTab == .variations) {
                    selectedTab = .variations
                }
                
                PatternTabButton(title: "SCENES", tab: .scenes, isSelected: selectedTab == .scenes) {
                    selectedTab = .scenes
                }
                
                PatternTabButton(title: "ARRANGEMENT", tab: .arrangement, isSelected: selectedTab == .arrangement) {
                    selectedTab = .arrangement
                }
                
                PatternTabButton(title: "AI", tab: .ai, isSelected: selectedTab == .ai) {
                    selectedTab = .ai
                }
            }
            .padding(.horizontal, 12)
            .padding(.top, 8)
            
            // Tab Content
            ScrollView {
                switch selectedTab {
                case .chains:
                    PatternChainsView(trackIndex: trackIndex, selectedChainMode: $selectedChainMode)
                
                case .variations:
                    PatternVariationsView(trackIndex: trackIndex, patternId: selectedPatternId,
                                        euclideanSteps: $euclideanSteps, euclideanPulses: $euclideanPulses,
                                        mutationAmount: $mutationAmount, swingAmount: $swingAmount)
                
                case .scenes:
                    PatternScenesView(trackIndex: trackIndex)
                
                case .arrangement:
                    PatternArrangementView(trackIndex: trackIndex)
                
                case .ai:
                    AIGenerativeView(trackIndex: trackIndex)
                }
            }
            .frame(maxHeight: 240)
            .padding(.horizontal, 12)
            .padding(.bottom, 12)
        }
        .frame(width: 520, height: 360)
        .background(Color.white)
        .cornerRadius(14)
        .shadow(color: Color.black.opacity(0.15), radius: 16, x: 0, y: 8)
    }
}

// MARK: - Pattern Tab Button
struct PatternTabButton: View {
    let title: String
    let tab: PatternOverlay.PatternTab
    let isSelected: Bool
    let action: () -> Void
    
    var body: some View {
        Button(action: action) {
            Text(title)
                .font(.custom("Barlow Condensed", size: 10))
                .foregroundColor(isSelected ? .white : .black.opacity(0.7))
                .frame(maxWidth: .infinity, minHeight: 24)
                .background(isSelected ? Color.blue : Color.gray.opacity(0.1))
                .cornerRadius(4)
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - Pattern Chains View
struct PatternChainsView: View {
    let trackIndex: Int
    @Binding var selectedChainMode: Int
    @EnvironmentObject var appState: EtherSynthState
    @State private var showingChainBuilder = false
    
    private var currentPattern: UInt32 {
        appState.currentPatterns[trackIndex]
    }
    
    private var queuedPattern: UInt32 {
        appState.queuedPatterns[trackIndex]
    }
    
    private var armedPattern: UInt32 {
        appState.armedPatterns[trackIndex]
    }
    
    var body: some View {
        VStack(spacing: 12) {
            // Current Status
            HStack {
                VStack(alignment: .leading, spacing: 4) {
                    Text("CURRENT PATTERN")
                        .font(.custom("Barlow Condensed", size: 9))
                        .foregroundColor(.black.opacity(0.7))
                    
                    Text("Pattern \(currentPattern)")
                        .font(.custom("Barlow Condensed", size: 14))
                        .foregroundColor(.black)
                        .fontWeight(.bold)
                }
                
                Spacer()
                
                if queuedPattern != 0 {
                    VStack(alignment: .trailing, spacing: 4) {
                        Text("QUEUED")
                            .font(.custom("Barlow Condensed", size: 9))
                            .foregroundColor(.orange.opacity(0.8))
                        
                        Text("Pattern \(queuedPattern)")
                            .font(.custom("Barlow Condensed", size: 12))
                            .foregroundColor(.orange)
                    }
                }
                
                if armedPattern != 0 {
                    VStack(alignment: .trailing, spacing: 4) {
                        Text("ARMED")
                            .font(.custom("Barlow Condensed", size: 9))
                            .foregroundColor(.red.opacity(0.8))
                        
                        Text("Pattern \(armedPattern)")
                            .font(.custom("Barlow Condensed", size: 12))
                            .foregroundColor(.red)
                    }
                }
            }
            .padding(12)
            .background(Color.gray.opacity(0.1))
            .cornerRadius(8)
            
            // Chain Mode Selection
            VStack(alignment: .leading, spacing: 6) {
                Text("CHAIN MODE")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(.black.opacity(0.7))
                
                Picker("Chain Mode", selection: $selectedChainMode) {
                    ForEach(0..<5) { index in
                        Text(["Manual", "Auto", "Conditional", "Performance", "Arrangement"][index])
                            .tag(index)
                    }
                }
                .pickerStyle(SegmentedPickerStyle())
                .onChange(of: selectedChainMode) { mode in
                    appState.setChainMode(trackIndex: trackIndex, mode: mode)
                }
            }
            
            // Live Performance Controls
            VStack(alignment: .leading, spacing: 8) {
                Text("LIVE CONTROLS")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(.black.opacity(0.7))
                
                HStack(spacing: 8) {
                    // Performance Mode Toggle
                    Button(action: {
                        appState.setPerformanceMode(!appState.performanceMode)
                    }) {
                        HStack(spacing: 4) {
                            Image(systemName: appState.performanceMode ? "play.fill" : "play")
                            Text("PERFORMANCE")
                        }
                        .font(.custom("Barlow Condensed", size: 10))
                        .foregroundColor(.white)
                        .padding(.horizontal, 8)
                        .padding(.vertical, 6)
                        .background(appState.performanceMode ? Color.orange : Color.gray.opacity(0.6))
                        .cornerRadius(6)
                    }
                    .buttonStyle(PlainButtonStyle())
                    
                    // Launch Armed Patterns
                    Button(action: {
                        appState.launchArmedPatterns()
                    }) {
                        HStack(spacing: 4) {
                            Image(systemName: "arrow.up.circle.fill")
                            Text("LAUNCH ALL")
                        }
                        .font(.custom("Barlow Condensed", size: 10))
                        .foregroundColor(.white)
                        .padding(.horizontal, 8)
                        .padding(.vertical, 6)
                        .background(Color.green)
                        .cornerRadius(6)
                    }
                    .buttonStyle(PlainButtonStyle())
                    
                    Spacer()
                }
                
                // Global Quantization
                HStack {
                    Text("QUANTIZE")
                        .font(.custom("Barlow Condensed", size: 9))
                        .foregroundColor(.black.opacity(0.7))
                    
                    Picker("Quantization", selection: Binding(
                        get: { appState.globalQuantization },
                        set: { appState.setGlobalQuantization(bars: $0) }
                    )) {
                        Text("1 Bar").tag(1)
                        Text("2 Bars").tag(2)
                        Text("4 Bars").tag(4)
                        Text("8 Bars").tag(8)
                    }
                    .pickerStyle(SegmentedPickerStyle())
                    .frame(maxWidth: 200)
                }
            }
            
            // Quick Pattern Triggers (16 keys)
            VStack(alignment: .leading, spacing: 6) {
                Text("16-KEY PATTERN TRIGGERS")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(.black.opacity(0.7))
                
                LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 8), spacing: 4) {
                    ForEach(0..<16) { keyIndex in
                        let patternId = UInt32(keyIndex + 1)
                        
                        Button(action: {
                            if appState.performanceMode {
                                appState.armPattern(patternId: patternId, trackIndex: trackIndex)
                            } else {
                                appState.queuePattern(patternId: patternId, trackIndex: trackIndex)
                            }
                        }) {
                            VStack(spacing: 2) {
                                Text("\(keyIndex + 1)")
                                    .font(.custom("Barlow Condensed", size: 10))
                                    .fontWeight(.bold)
                                
                                // Status indicator
                                Circle()
                                    .fill(getPatternStatusColor(patternId: patternId))
                                    .frame(width: 6, height: 6)
                            }
                            .foregroundColor(getPatternStatusColor(patternId: patternId) == Color.clear ? .black.opacity(0.6) : .white)
                            .frame(width: 32, height: 24)
                            .background(getPatternStatusColor(patternId: patternId) == Color.clear ? Color.gray.opacity(0.2) : getPatternStatusColor(patternId: patternId))
                            .cornerRadius(4)
                        }
                        .buttonStyle(PlainButtonStyle())
                    }
                }
            }
            
            // Chain Templates
            VStack(alignment: .leading, spacing: 6) {
                Text("CHAIN TEMPLATES")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(.black.opacity(0.7))
                
                HStack(spacing: 6) {
                    ChainTemplateButton(title: "BASIC LOOP", color: Color.blue) {
                        appState.createBasicLoop(patternIds: [1, 2, 3, 4])
                    }
                    
                    ChainTemplateButton(title: "V-C", color: Color.green) {
                        appState.createVerseChorus(versePattern: 1, chorusPattern: 2)
                    }
                    
                    ChainTemplateButton(title: "BUILD+DROP", color: Color.orange) {
                        // Generate intelligent build+drop
                        appState.generateIntelligentChain(startPattern: 1, chainLength: 8)
                    }
                }
            }
        }
    }
    
    private func getPatternStatusColor(patternId: UInt32) -> Color {
        if currentPattern == patternId {
            return Color.blue
        } else if queuedPattern == patternId {
            return Color.orange
        } else if armedPattern == patternId {
            return Color.red
        }
        return Color.clear
    }
}

// MARK: - Chain Template Button
struct ChainTemplateButton: View {
    let title: String
    let color: Color
    let action: () -> Void
    
    var body: some View {
        Button(action: action) {
            Text(title)
                .font(.custom("Barlow Condensed", size: 9))
                .foregroundColor(.white)
                .padding(.horizontal, 8)
                .padding(.vertical, 6)
                .background(color)
                .cornerRadius(6)
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - Pattern Variations View
struct PatternVariationsView: View {
    let trackIndex: Int
    let patternId: UInt32
    @Binding var euclideanSteps: Int
    @Binding var euclideanPulses: Int
    @Binding var mutationAmount: Float
    @Binding var swingAmount: Float
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        VStack(spacing: 16) {
            // Pattern Mutations
            VStack(alignment: .leading, spacing: 8) {
                Text("PATTERN MUTATIONS")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(.black.opacity(0.7))
                
                VStack(spacing: 6) {
                    HStack {
                        Text("MUTATION")
                            .font(.custom("Barlow Condensed", size: 9))
                            .frame(width: 80, alignment: .leading)
                        
                        Slider(value: $mutationAmount, in: 0...1)
                            .frame(maxWidth: 120)
                        
                        Text(String(format: "%.1f%%", mutationAmount * 100))
                            .font(.custom("Barlow Condensed", size: 9))
                            .frame(width: 40, alignment: .trailing)
                    }
                    
                    HStack {
                        Text("SWING")
                            .font(.custom("Barlow Condensed", size: 9))
                            .frame(width: 80, alignment: .leading)
                        
                        Slider(value: $swingAmount, in: -0.5...0.5)
                            .frame(maxWidth: 120)
                        
                        Text(String(format: "%.1f%%", swingAmount * 100))
                            .font(.custom("Barlow Condensed", size: 9))
                            .frame(width: 40, alignment: .trailing)
                    }
                }
                
                HStack(spacing: 8) {
                    Button("GENERATE VARIATION") {
                        appState.generatePatternVariation(sourcePatternId: patternId, mutationAmount: mutationAmount)
                    }
                    .buttonStyle(ActionButtonStyle(color: .purple))
                    
                    Button("APPLY SWING") {
                        appState.morphPatternTiming(patternId: patternId, swing: swingAmount, humanize: 0.1)
                    }
                    .buttonStyle(ActionButtonStyle(color: .blue))
                }
            }
            
            // Euclidean Rhythms
            VStack(alignment: .leading, spacing: 8) {
                Text("EUCLIDEAN RHYTHMS")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(.black.opacity(0.7))
                
                HStack(spacing: 12) {
                    VStack {
                        Text("STEPS")
                            .font(.custom("Barlow Condensed", size: 9))
                        
                        Picker("Steps", selection: $euclideanSteps) {
                            ForEach([8, 16, 24, 32], id: \.self) { steps in
                                Text("\(steps)").tag(steps)
                            }
                        }
                        .pickerStyle(SegmentedPickerStyle())
                        .frame(maxWidth: 120)
                    }
                    
                    VStack {
                        Text("PULSES")
                            .font(.custom("Barlow Condensed", size: 9))
                        
                        Picker("Pulses", selection: $euclideanPulses) {
                            ForEach(1...8, id: \.self) { pulses in
                                Text("\(pulses)").tag(pulses)
                            }
                        }
                        .pickerStyle(SegmentedPickerStyle())
                        .frame(maxWidth: 160)
                    }
                }
                
                Button("APPLY EUCLIDEAN RHYTHM") {
                    appState.applyEuclideanRhythm(patternId: patternId, steps: euclideanSteps, pulses: euclideanPulses)
                }
                .buttonStyle(ActionButtonStyle(color: .green))
            }
            
            // AI Suggestions
            VStack(alignment: .leading, spacing: 8) {
                Text("AI PATTERN SUGGESTIONS")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(.black.opacity(0.7))
                
                let suggestions = appState.getSuggestedPatterns(currentPattern: patternId)
                
                HStack(spacing: 6) {
                    ForEach(suggestions, id: \.self) { suggestion in
                        Button("P\(suggestion)") {
                            appState.queuePattern(patternId: suggestion, trackIndex: trackIndex)
                        }
                        .font(.custom("Barlow Condensed", size: 10))
                        .foregroundColor(.white)
                        .frame(width: 32, height: 24)
                        .background(Color.cyan)
                        .cornerRadius(4)
                        .buttonStyle(PlainButtonStyle())
                    }
                }
            }
        }
    }
}

// MARK: - Pattern Scenes View
struct PatternScenesView: View {
    let trackIndex: Int
    @EnvironmentObject var appState: EtherSynthState
    @State private var newSceneName = "Scene"
    
    var body: some View {
        VStack(spacing: 12) {
            // Save New Scene
            VStack(alignment: .leading, spacing: 6) {
                Text("SAVE NEW SCENE")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(.black.opacity(0.7))
                
                HStack(spacing: 8) {
                    TextField("Scene name", text: $newSceneName)
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .frame(maxWidth: 120)
                    
                    Button("SAVE") {
                        let sceneId = appState.saveScene(name: newSceneName)
                        if sceneId != 0 {
                            newSceneName = "Scene" // Reset
                        }
                    }
                    .buttonStyle(ActionButtonStyle(color: .blue))
                }
            }
            
            // Available Scenes
            VStack(alignment: .leading, spacing: 6) {
                Text("AVAILABLE SCENES")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(.black.opacity(0.7))
                
                LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 4), spacing: 8) {
                    ForEach(Array(appState.availableScenes.enumerated()), id: \.offset) { index, sceneName in
                        let sceneId = UInt32(index + 1)
                        
                        Button(action: {
                            let _ = appState.loadScene(sceneId: sceneId)
                        }) {
                            VStack(spacing: 4) {
                                Text(sceneName)
                                    .font(.custom("Barlow Condensed", size: 10))
                                    .fontWeight(.medium)
                                
                                if appState.selectedSceneId == sceneId {
                                    Text("ACTIVE")
                                        .font(.custom("Barlow Condensed", size: 8))
                                        .foregroundColor(.white)
                                        .padding(.horizontal, 6)
                                        .padding(.vertical, 2)
                                        .background(Color.green)
                                        .cornerRadius(3)
                                }
                            }
                            .foregroundColor(appState.selectedSceneId == sceneId ? .white : .black)
                            .frame(width: 80, height: 50)
                            .background(appState.selectedSceneId == sceneId ? Color.blue : Color.gray.opacity(0.2))
                            .cornerRadius(8)
                        }
                        .buttonStyle(PlainButtonStyle())
                    }
                }
            }
        }
    }
}

// MARK: - Pattern Arrangement View
struct PatternArrangementView: View {
    let trackIndex: Int
    @EnvironmentObject var appState: EtherSynthState
    
    var body: some View {
        VStack(spacing: 12) {
            // Arrangement Mode Toggle
            HStack {
                Toggle("Arrangement Mode", isOn: Binding(
                    get: { appState.arrangementMode },
                    set: { appState.arrangementMode = $0 }
                ))
                .toggleStyle(SwitchToggleStyle(tint: .blue))
                
                Spacer()
            }
            
            if appState.arrangementMode {
                // Song Sections
                VStack(alignment: .leading, spacing: 8) {
                    Text("SONG SECTIONS")
                        .font(.custom("Barlow Condensed", size: 10))
                        .foregroundColor(.black.opacity(0.7))
                    
                    HStack(spacing: 6) {
                        ForEach(Array(appState.sectionNames.enumerated()), id: \.offset) { index, sectionName in
                            Button(action: {
                                appState.currentSection = index
                            }) {
                                VStack(spacing: 2) {
                                    Text(sectionName.uppercased())
                                        .font(.custom("Barlow Condensed", size: 9))
                                        .fontWeight(.medium)
                                    
                                    if appState.currentSection == index {
                                        Rectangle()
                                            .fill(Color.white)
                                            .frame(height: 2)
                                            .padding(.horizontal, 4)
                                    }
                                }
                                .foregroundColor(appState.currentSection == index ? .white : .black.opacity(0.7))
                                .frame(width: 60, height: 32)
                                .background(appState.currentSection == index ? Color.orange : Color.gray.opacity(0.2))
                                .cornerRadius(6)
                            }
                            .buttonStyle(PlainButtonStyle())
                        }
                    }
                }
                
                Text("Arrangement features: Section creation, pattern assignment, and full song workflow")
                    .font(.custom("Barlow Condensed", size: 10))
                    .foregroundColor(.black.opacity(0.6))
                    .multilineTextAlignment(.center)
                    .padding(.vertical, 8)
            } else {
                Text("Enable arrangement mode for full song workflow")
                    .font(.custom("Barlow Condensed", size: 11))
                    .foregroundColor(.black.opacity(0.6))
                    .frame(maxWidth: .infinity, minHeight: 80)
            }
        }
    }
}

// MARK: - AI Generative View
struct AIGenerativeView: View {
    let trackIndex: Int
    @EnvironmentObject var appState: EtherSynthState
    
    private let generationModeNames = ["Assist", "Generate", "Evolve", "Respond", "Harmonize", "Rhythmize"]
    private let styleNames = ["Electronic", "Techno", "House", "Ambient", "D&B", "Acid", "Industrial", "Melodic", "Experimental", "Custom"]
    private let complexityNames = ["Simple", "Moderate", "Complex", "Adaptive"]
    
    var body: some View {
        VStack(spacing: 12) {
            // AI Status
            HStack {
                VStack(alignment: .leading, spacing: 2) {
                    Text("AI GENERATIVE SEQUENCER")
                        .font(.custom("Barlow Condensed", size: 10))
                        .foregroundColor(.black.opacity(0.7))
                    
                    if appState.adaptiveMode {
                        Text("● LEARNING MODE ACTIVE")
                            .font(.custom("Barlow Condensed", size: 9))
                            .foregroundColor(.green)
                    }
                    
                    if appState.detectedScale.confidence > 0.5 {
                        Text("Scale: \(scaleString) (\(Int(appState.detectedScale.confidence * 100))%)")
                            .font(.custom("Barlow Condensed", size: 9))
                            .foregroundColor(.blue.opacity(0.7))
                    }
                }
                
                Spacer()
                
                Button(action: {
                    appState.setAdaptiveMode(enabled: !appState.adaptiveMode)
                }) {
                    Text(appState.adaptiveMode ? "ADAPTIVE ON" : "ADAPTIVE OFF")
                        .font(.custom("Barlow Condensed", size: 9))
                        .foregroundColor(.white)
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(appState.adaptiveMode ? Color.green : Color.gray)
                        .cornerRadius(4)
                }
                .buttonStyle(PlainButtonStyle())
            }
            
            // Generation Controls
            HStack(spacing: 12) {
                VStack(alignment: .leading, spacing: 6) {
                    Text("MODE")
                        .font(.custom("Barlow Condensed", size: 9))
                        .foregroundColor(.black.opacity(0.6))
                    
                    Picker("Mode", selection: Binding(
                        get: { appState.generationMode },
                        set: { appState.generationMode = $0 }
                    )) {
                        ForEach(Array(generationModeNames.enumerated()), id: \.offset) { index, name in
                            Text(name).tag(index)
                        }
                    }
                    .pickerStyle(MenuPickerStyle())
                    .frame(width: 80)
                }
                
                VStack(alignment: .leading, spacing: 6) {
                    Text("STYLE")
                        .font(.custom("Barlow Condensed", size: 9))
                        .foregroundColor(.black.opacity(0.6))
                    
                    Picker("Style", selection: Binding(
                        get: { appState.musicalStyle },
                        set: { 
                            appState.musicalStyle = $0
                            appState.setMusicalStyle(style: $0)
                        }
                    )) {
                        ForEach(Array(styleNames.enumerated()), id: \.offset) { index, name in
                            Text(name).tag(index)
                        }
                    }
                    .pickerStyle(MenuPickerStyle())
                    .frame(width: 80)
                }
                
                VStack(alignment: .leading, spacing: 6) {
                    Text("COMPLEXITY")
                        .font(.custom("Barlow Condensed", size: 9))
                        .foregroundColor(.black.opacity(0.6))
                    
                    Picker("Complexity", selection: Binding(
                        get: { appState.generationComplexity },
                        set: { appState.generationComplexity = $0 }
                    )) {
                        ForEach(Array(complexityNames.enumerated()), id: \.offset) { index, name in
                            Text(name).tag(index)
                        }
                    }
                    .pickerStyle(MenuPickerStyle())
                    .frame(width: 80)
                }
            }
            
            // Parameter Sliders
            HStack(spacing: 12) {
                VStack(alignment: .leading, spacing: 4) {
                    Text("DENSITY")
                        .font(.custom("Barlow Condensed", size: 9))
                        .foregroundColor(.black.opacity(0.6))
                    
                    Slider(value: Binding(
                        get: { appState.generationDensity },
                        set: { 
                            appState.generationDensity = $0
                            appState.setGenerationParams(
                                density: appState.generationDensity,
                                tension: appState.generationTension,
                                creativity: appState.generationCreativity,
                                responsiveness: appState.generationResponsiveness
                            )
                        }
                    ), in: 0...1)
                    .frame(width: 60)
                }
                
                VStack(alignment: .leading, spacing: 4) {
                    Text("TENSION")
                        .font(.custom("Barlow Condensed", size: 9))
                        .foregroundColor(.black.opacity(0.6))
                    
                    Slider(value: Binding(
                        get: { appState.generationTension },
                        set: { 
                            appState.generationTension = $0
                            appState.setGenerationParams(
                                density: appState.generationDensity,
                                tension: appState.generationTension,
                                creativity: appState.generationCreativity,
                                responsiveness: appState.generationResponsiveness
                            )
                        }
                    ), in: 0...1)
                    .frame(width: 60)
                }
                
                VStack(alignment: .leading, spacing: 4) {
                    Text("CREATIVITY")
                        .font(.custom("Barlow Condensed", size: 9))
                        .foregroundColor(.black.opacity(0.6))
                    
                    Slider(value: Binding(
                        get: { appState.generationCreativity },
                        set: { 
                            appState.generationCreativity = $0
                            appState.setGenerationParams(
                                density: appState.generationDensity,
                                tension: appState.generationTension,
                                creativity: appState.generationCreativity,
                                responsiveness: appState.generationResponsiveness
                            )
                        }
                    ), in: 0...1)
                    .frame(width: 60)
                }
                
                VStack(alignment: .leading, spacing: 4) {
                    Text("RESPONSIVE")
                        .font(.custom("Barlow Condensed", size: 9))
                        .foregroundColor(.black.opacity(0.6))
                    
                    Slider(value: Binding(
                        get: { appState.generationResponsiveness },
                        set: { 
                            appState.generationResponsiveness = $0
                            appState.setGenerationParams(
                                density: appState.generationDensity,
                                tension: appState.generationTension,
                                creativity: appState.generationCreativity,
                                responsiveness: appState.generationResponsiveness
                            )
                        }
                    ), in: 0...1)
                    .frame(width: 60)
                }
            }
            
            // Action Buttons
            HStack(spacing: 8) {
                Button(action: {
                    let patternId = appState.generatePattern(
                        mode: appState.generationMode,
                        style: appState.musicalStyle,
                        complexity: appState.generationComplexity,
                        trackIndex: trackIndex
                    )
                    if patternId != 0 {
                        appState.currentPatterns[trackIndex] = patternId
                    }
                }) {
                    Text("GENERATE")
                }
                .buttonStyle(ActionButtonStyle(color: .blue))
                
                if appState.currentPatterns[trackIndex] != 0 {
                    Button(action: {
                        appState.evolvePattern(patternId: appState.currentPatterns[trackIndex], evolutionAmount: 0.3)
                    }) {
                        Text("EVOLVE")
                    }
                    .buttonStyle(ActionButtonStyle(color: .purple))
                    
                    Button(action: {
                        let harmonyId = appState.generateHarmony(sourcePatternId: appState.currentPatterns[trackIndex])
                        if harmonyId != 0 {
                            // Could assign to another track or queue
                            appState.queuedPatterns[trackIndex] = harmonyId
                        }
                    }) {
                        Text("HARMONY")
                    }
                    .buttonStyle(ActionButtonStyle(color: .green))
                    
                    Button(action: {
                        let rhythmId = appState.generateRhythmVariation(sourcePatternId: appState.currentPatterns[trackIndex], variationAmount: 0.5)
                        if rhythmId != 0 {
                            appState.queuedPatterns[trackIndex] = rhythmId
                        }
                    }) {
                        Text("RHYTHM VAR")
                    }
                    .buttonStyle(ActionButtonStyle(color: .orange))
                }
                
                Button(action: {
                    appState.getScaleAnalysis()
                    appState.getGenerativeSuggestions()
                }) {
                    Text("ANALYZE")
                }
                .buttonStyle(ActionButtonStyle(color: .gray))
            }
            
            // AI Suggestions
            if !appState.generativeSuggestions.isEmpty {
                VStack(alignment: .leading, spacing: 6) {
                    Text("AI SUGGESTIONS")
                        .font(.custom("Barlow Condensed", size: 9))
                        .foregroundColor(.black.opacity(0.6))
                    
                    HStack(spacing: 4) {
                        ForEach(appState.generativeSuggestions.prefix(6), id: \.self) { suggestionId in
                            Button(action: {
                                appState.currentPatterns[trackIndex] = suggestionId
                            }) {
                                VStack(spacing: 2) {
                                    Text("\(suggestionId % 1000)")
                                        .font(.custom("Barlow Condensed", size: 10))
                                        .fontWeight(.medium)
                                    
                                    if let complexity = appState.patternComplexities[suggestionId] {
                                        Rectangle()
                                            .fill(Color.blue.opacity(complexity))
                                            .frame(width: 24, height: 2)
                                    }
                                }
                                .foregroundColor(.black.opacity(0.7))
                                .frame(width: 32, height: 24)
                                .background(Color.gray.opacity(0.1))
                                .cornerRadius(4)
                            }
                            .buttonStyle(PlainButtonStyle())
                        }
                        
                        Spacer()
                        
                        Button(action: {
                            appState.resetLearningModel()
                        }) {
                            Text("RESET AI")
                                .font(.custom("Barlow Condensed", size: 8))
                                .foregroundColor(.white)
                                .padding(.horizontal, 6)
                                .padding(.vertical, 3)
                                .background(Color.red.opacity(0.8))
                                .cornerRadius(3)
                        }
                        .buttonStyle(PlainButtonStyle())
                    }
                }
            }
        }
    }
    
    private var scaleString: String {
        let noteNames = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
        let scaleNames = ["Major", "Minor", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Aeolian", "Locrian", "Pentatonic", "Blues", "Harmonic Minor", "Chromatic"]
        
        let rootName = noteNames[appState.detectedScale.root % 12]
        let scaleName = scaleNames[min(appState.detectedScale.type, scaleNames.count - 1)]
        
        return "\(rootName) \(scaleName)"
    }
}

// MARK: - Action Button Style
struct ActionButtonStyle: ButtonStyle {
    let color: Color
    
    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .font(.custom("Barlow Condensed", size: 9))
            .foregroundColor(.white)
            .padding(.horizontal, 12)
            .padding(.vertical, 6)
            .background(configuration.isPressed ? color.opacity(0.8) : color)
            .cornerRadius(6)
    }
}
