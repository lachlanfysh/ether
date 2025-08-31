import SwiftUI
import Combine

// MARK: - Simple SynthController for Demo

class SynthController: ObservableObject {
    @Published var isPlaying: Bool = false
    @Published var isRecording: Bool = false
    @Published var uiBPM: Double = 120.0
    @Published var uiArmedInstrumentIndex: Int = 0
    @Published var uiCpuUsage: Int = 12
    @Published var uiActiveVoices: Int = 4
    @Published var currentPatternID: String = "A"
    @Published var currentPatternPage: Int = 0
    @Published var activeStamps = ActiveStamps()
    @Published var activeTool: Tool? = nil
    @Published var toolScope: ToolScope = .step
    @Published var uiIsShiftHeld: Bool = false
    @Published var uiIsWriteEnabled: Bool = false
    
    // Hardware button publisher
    let hardwareButtonPublisher = PassthroughSubject<HardwareButtonPress, Never>()
    
    func play() {
        isPlaying.toggle()
        print("Play toggled: \(isPlaying)")
    }
    
    func record() {
        isRecording.toggle()
        print("Record toggled: \(isRecording)")
    }
    
    func toggleStamp(_ type: StampType) {
        switch type {
        case .accent: activeStamps.accent.toggle()
        case .ratchet: activeStamps.ratchet = activeStamps.ratchet > 0 ? 0 : 1
        case .tie: activeStamps.tie.toggle()
        case .micro: activeStamps.micro = activeStamps.micro == 0 ? 1 : 0
        }
    }
    
    func triggerNudge() { print("Nudge triggered") }
    func triggerCopy() { print("Copy triggered") }
    func triggerCut() { print("Cut triggered") }
    func triggerClone() { print("Clone triggered") }
    func triggerDelete() { print("Delete triggered") }
    
    func toggleStep(_ index: Int) {
        print("Toggle step \(index)")
    }
    
    func handleBottomButtonPress(_ index: Int, isShiftHeld: Bool) {
        print("Bottom button \(index) pressed, shift: \(isShiftHeld)")
    }
    
    func switchPattern(_ patternID: String) {
        currentPatternID = patternID
        print("Switched to pattern \(patternID)")
    }
    
    func selectMixerChannel(_ index: Int) {
        print("Selected mixer channel \(index)")
    }
    
    func getInstrumentTypeName() -> String { "MacroVA" }
    func getPatternLength() -> Int { 16 }
    func getTotalPatternPages() -> Int { 1 }
    func switchToPatternPage(_ page: Int) { currentPatternPage = page }
    func setPatternLength(_ length: Int) { print("Set pattern length: \(length)") }
    
    func getStepData(_ index: Int) -> StepData {
        return StepData(
            noteEvents: [],
            hasMetadata: false,
            accent: false,
            tie: false
        )
    }
    
    func getNoteDisplayName(_ note: NoteEvent) -> String { "C3" }
    func getArmedInstrumentColor() -> Color { Color.blue }
    func getInstrumentColor(_ index: Int) -> Color { Color.green }
    
    func openStepEditor(_ index: Int) { print("Open step editor for \(index)") }
    func handleSmartKnobRotation(_ delta: Float) { print("SmartKnob: \(delta)") }
    
    // Demo properties
    var currentSwing: Float = 0.5
    var currentQuantize: Float = 1.0
}

// MARK: - Supporting Types

enum StampType {
    case accent, ratchet, tie, micro
}

enum Tool {
    case copy, cut, nudge
    
    var rawValue: String {
        switch self {
        case .copy: return "copy"
        case .cut: return "cut"
        case .nudge: return "nudge"
        }
    }
}

enum ToolScope {
    case step, pattern, song
    
    var rawValue: String {
        switch self {
        case .step: return "step"
        case .pattern: return "pattern" 
        case .song: return "song"
        }
    }
}

struct ActiveStamps {
    var accent: Bool = false
    var ratchet: Int = 0
    var tie: Bool = false
    var micro: Int = 0
}

struct HardwareButtonPress {
    let buttonType: HardwareButtonType
    let isPressed: Bool
}

enum HardwareButtonType {
    case topButton(Int)
    case bottomButton(Int)
}

struct StepData {
    let noteEvents: [NoteEvent]
    let hasMetadata: Bool
    let accent: Bool
    let tie: Bool
}

struct NoteEvent {
    let instrumentSlotIndex: UInt32
    let pitch: UInt8
    let velocity: UInt8
}