import SwiftUI
import AVFoundation
import Combine

// MARK: - Enhanced EtherSynth Interface with Complete Backend Integration

struct EtherSynth: View {
    // MARK: - Core State
    @StateObject private var synthController = SynthController()
    
    // Transport & timing (matching JSX)
    @State private var isPlaying = false
    @State private var isRecording = false
    @State private var isWriting = false
    @State private var bpm: Float = 128.0
    @State private var swing: Float = 56.0
    @State private var quantize: Float = 50.0
    
    // Pattern system (enhanced from JSX)
    @State private var currentPattern = 0
    @State private var patternLength = 16
    @State private var currentPage = 0
    @State private var steps = Array(repeating: false, count: 16)
    @State private var stepNotes: [Set<Int>] = Array(repeating: [], count: 16)
    @State private var stepMeta: [StepMetadata] = Array(repeating: StepMetadata(), count: 16)
    
    // Instrument & engine system (vastly enhanced)
    @State private var armedInstrument = 0
    @State private var instrumentEngines = Array(repeating: 0, count: 16) // 16 instruments
    @State private var instrumentParameters: [[Float]] = Array(repeating: Array(repeating: 0.0, count: 16), count: 16)
    
    // UI modes (expanded from 6 to 12)
    @State private var currentMode: ViewMode = .step
    @State private var shiftHeld = false
    @State private var shiftLatched = false
    var shiftActive: Bool { shiftHeld || shiftLatched }
    
    // Tool system (from JSX)
    @State private var activeTool: Tool? = nil
    @State private var stamps = StepStamps()
    
    // Overlays (massively expanded)
    @State private var showInstOverlay = false
    @State private var showSoundOverlay = false
    @State private var showPatternOverlay = false
    @State private var showEngineOverlay = false
    @State private var showLFOOverlay = false
    @State private var showModulationOverlay = false
    @State private var showEffectsOverlay = false
    @State private var showEQOverlay = false
    @State private var showChordOverlay = false
    @State private var showChainOverlay = false
    @State private var showAIGenOverlay = false
    @State private var showMacroOverlay = false
    @State private var showSceneOverlay = false
    @State private var showSampleOverlay = false
    @State private var showPerformanceOverlay = false
    @State private var showSmartKnobOverlay = false
    
    // LFO system (8 LFOs with full matrix)
    @State private var lfoSettings: [LFOSettings] = Array(repeating: LFOSettings(), count: 8)
    @State private var lfoAssignments: [[[Float]]] = Array(repeating: Array(repeating: Array(repeating: 0.0, count: 16), count: 16), count: 8)
    
    // Effects & EQ system
    @State private var masterEQEnabled = true
    @State private var eqBands: [EQBandSettings] = Array(repeating: EQBandSettings(), count: 7)
    @State private var effectsChain: [EffectSettings] = []
    
    // Performance system
    @State private var macros: [MacroSettings] = []
    @State private var scenes: [SceneSettings] = []
    @State private var smartKnobParameter = 0
    
    // Generative system
    @State private var aiGenerationMode: GenerationMode = .assist
    @State private var musicalStyle: MusicalStyle = .electronic
    @State private var generationComplexity: GenerationComplexity = .moderate
    @State private var generationParams = GenerationParams()
    
    // Pattern chains
    @State private var patternChains: [PatternChain] = []
    @State private var activeChain: PatternChain? = nil
    
    // Toast notifications (from JSX)
    @State private var toastMessage: String? = nil
    
    var body: some View {
        ZStack {
            // Main interface with JSX styling
            VStack(spacing: 0) {
                HeaderView()
                OptionLineView()
                ViewAreaView()
                StepRowView()
                FooterView()
            }
            .frame(width: 960, height: 320)
            .background(Color(red: 0.85, green: 0.85, blue: 0.85))
            .overlay(
                RoundedRectangle(cornerRadius: 0)
                    .stroke(Color.black, lineWidth: 1)
            )
            .modifier(BevelOutModifier())
            
            // All overlays
            if showInstOverlay { InstOverlay() }
            if showSoundOverlay { SoundOverlay() }
            if showPatternOverlay { PatternOverlay() }
            if showEngineOverlay { EngineOverlay() }
            if showLFOOverlay { LFOOverlay() }
            if showModulationOverlay { ModulationOverlay() }
            if showEffectsOverlay { EffectsOverlay() }
            if showEQOverlay { EQOverlay() }
            if showChordOverlay { ChordOverlay() }
            if showChainOverlay { ChainOverlay() }
            if showAIGenOverlay { AIGenOverlay() }
            if showMacroOverlay { MacroOverlay() }
            if showSceneOverlay { SceneOverlay() }
            if showSampleOverlay { SampleOverlay() }
            if showPerformanceOverlay { PerformanceOverlay() }
            if showSmartKnobOverlay { SmartKnobOverlay() }
            
            // Toast notifications
            if let message = toastMessage {
                VStack {
                    Spacer()
                    Text(message)
                        .font(.system(size: 10))
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(Color(red: 0.85, green: 0.85, blue: 0.85))
                        .foregroundColor(.black)
                        .modifier(BevelOutModifier())
                    Spacer()
                        .frame(height: 100)
                }
            }
        }
        .onAppear {
            initializeSynth()
        }
    }
    
    // MARK: - Header (Transport Controls)
    @ViewBuilder
    func HeaderView() -> some View {
        HStack(spacing: 2) {
            // Transport
            BevelButton(label: isPlaying ? "■" : "▶", isActive: isPlaying) {
                if shiftActive {
                    adjustBPM(1)
                } else {
                    togglePlayback()
                }
            }
            .frame(width: 32, height: 20)
            
            BevelButton(label: "REC", isActive: isRecording) {
                if shiftActive {
                    showInstOverlay = true
                } else {
                    toggleRecording()
                }
            }
            .frame(width: 32, height: 20)
            
            BevelButton(label: isWriting ? "WRITE*" : "WRITE", isActive: isWriting) {
                if shiftActive {
                    switchMainMode()
                } else {
                    isWriting.toggle()
                }
            }
            .frame(width: 48, height: 20)
            
            BevelButton(label: "SHIFT", isActive: shiftActive) {
                // Handle shift press/release
            }
            .frame(width: 40, height: 20)
            .onLongPressGesture(minimumDuration: 0, maximumDistance: .infinity, pressing: { pressing in
                shiftHeld = pressing
            }, perform: {})
            .onTapGesture(count: 2) {
                shiftLatched.toggle()
            }
            
            // Pattern & tempo
            BevelButton(label: "Pat \\(patternLabels[currentPattern])") {
                showPatternOverlay = true
            }
            .frame(width: 40, height: 20)
            
            BevelButton(label: "BPM \\(Int(bpm))") {
                adjustBPM(1)
            }
            .frame(width: 48, height: 20)
            
            BevelButton(label: "Sw \\(Int(swing))%") {
                adjustSwing(1)
            }
            .frame(width: 48, height: 20)
            
            BevelButton(label: "Q \\(Int(quantize))%") {
                adjustQuantize(1)
            }
            .frame(width: 48, height: 20)
            
            Spacer()
            
            // System info
            VStack(alignment: .trailing, spacing: 1) {
                HStack(spacing: 4) {
                    Text("CPU")
                        .font(.system(size: 9))
                        .foregroundColor(.black)
                    Text("\\(Int(synthController.cpuUsage))%")
                        .font(.system(size: 9))
                        .padding(.horizontal, 4)
                        .background(Color(red: 0.85, green: 0.85, blue: 0.85))
                        .modifier(BevelInModifier())
                }
                
                HStack(spacing: 4) {
                    Text("V")
                        .font(.system(size: 9))
                        .foregroundColor(.black)
                    Text("\\(synthController.activeVoices)")
                        .font(.system(size: 9))
                        .padding(.horizontal, 4)
                        .background(Color(red: 0.85, green: 0.85, blue: 0.85))
                        .modifier(BevelInModifier())
                }
            }
        }
        .padding(.horizontal, 4)
        .frame(height: 28)
        .background(Color(red: 0.85, green: 0.85, blue: 0.85))
        .modifier(BevelOutModifier())
    }
    
    // MARK: - Option Line (16 primary + 16 shift functions)
    @ViewBuilder
    func OptionLineView() -> some View {
        HStack(spacing: 2) {
            ForEach(0..<16, id: \\.self) { index in
                let option = shiftActive ? shiftOptions[index] : primaryOptions[index]
                let isActive = isOptionActive(index, shifted: shiftActive)
                
                BevelButton(label: option.label, isActive: isActive) {
                    handleOptionPress(index, shifted: shiftActive)
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
        .padding(.horizontal, 4)
        .frame(height: 48)
        .background(Color(red: 0.85, green: 0.85, blue: 0.85))
        .modifier(BevelOutModifier())
    }
    
    // MARK: - View Area (Dynamic content based on mode)
    @ViewBuilder
    func ViewAreaView() -> some View {
        let availableHeight: CGFloat = 320 - 28 - 48 - 24 - 62 // Total - header - options - footer - steps
        
        VStack(alignment: .leading, spacing: 2) {
            // Mode indicator
            HStack {
                Text(currentMode.displayName)
                    .font(.system(size: 10))
                    .foregroundColor(.black)
                
                if currentMode == .step {
                    Text("I\\(armedInstrument + 1) (\\(engineNames[instrumentEngines[armedInstrument]]))")
                        .font(.system(size: 10))
                        .foregroundColor(.black)
                }
                
                Spacer()
                
                if currentMode.supportsPages {
                    HStack(spacing: 2) {
                        ForEach(0..<pageCount, id: \\.self) { pageIndex in
                            Circle()
                                .frame(width: 6, height: 6)
                                .foregroundColor(pageIndex == currentPage ? .black : Color.gray)
                                .onTapGesture { currentPage = pageIndex }
                        }
                    }
                }
            }
            .padding(.horizontal, 4)
            
            // Dynamic content
            Group {
                switch currentMode {
                case .step:
                    StepModeView()
                case .pad:
                    PadModeView()
                case .engine:
                    EngineModeView()
                case .lfo:
                    LFOModeView()
                case .modulation:
                    ModulationModeView()
                case .effects:
                    EffectsModeView()
                case .eq:
                    EQModeView()
                case .chord:
                    ChordModeView()
                case .chain:
                    ChainModeView()
                case .aiGen:
                    AIGenModeView()
                case .macro:
                    MacroModeView()
                case .scene:
                    SceneModeView()
                case .sample:
                    SampleModeView()
                case .performance:
                    PerformanceModeView()
                case .smartKnob:
                    SmartKnobModeView()
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
        .padding(.top, 2)
        .frame(height: availableHeight)
    }
    
    // MARK: - Step Row (16 steps with enhanced metadata)
    @ViewBuilder
    func StepRowView() -> some View {
        HStack(spacing: 2) {
            ForEach(0..<16, id: \\.self) { stepIndex in
                let globalStep = currentPage * 16 + stepIndex
                let isActive = globalStep < patternLength && steps[stepIndex]
                let accentColor = pastelColors[armedInstrument % pastelColors.count]
                
                Button(action: {
                    handleStepPress(stepIndex)
                }) {
                    ZStack {
                        Rectangle()
                            .foregroundColor(Color(red: 0.85, green: 0.85, blue: 0.85))
                            .modifier(isActive ? BevelInModifier() : BevelOutModifier())
                        
                        // Accent color overlay
                        Rectangle()
                            .foregroundColor(accentColor)
                            .opacity(isActive ? 0.25 : 0.1)
                        
                        // Step number
                        VStack(alignment: .leading) {
                            HStack {
                                Text("\\(globalStep + 1)")
                                    .font(.system(size: 8))
                                    .foregroundColor(.black)
                                Spacer()
                                // Accent indicator
                                if stepMeta[stepIndex].accent {
                                    Text("›")
                                        .font(.system(size: 8))
                                        .foregroundColor(.black)
                                }
                            }
                            Spacer()
                            
                            // Step content
                            if isActive {
                                if stepNotes[stepIndex].count == 1 {
                                    Text(noteNameForStep(stepIndex))
                                        .font(.system(size: 9, weight: .semibold))
                                        .foregroundColor(.black)
                                } else if stepNotes[stepIndex].count > 1 {
                                    Text("Chord \\(stepNotes[stepIndex].count)")
                                        .font(.system(size: 8, weight: .semibold))
                                        .foregroundColor(.black)
                                }
                            }
                            
                            Spacer()
                            
                            // Step metadata indicators
                            HStack {
                                if stepMeta[stepIndex].tie {
                                    Rectangle()
                                        .frame(height: 2)
                                        .foregroundColor(.black)
                                }
                                Spacer()
                                if stepMeta[stepIndex].ratchet > 1 {
                                    Text("×\\(stepMeta[stepIndex].ratchet)")
                                        .font(.system(size: 8))
                                        .foregroundColor(.black)
                                }
                                if stepMeta[stepIndex].micro != 0 {
                                    Circle()
                                        .frame(width: 4, height: 4)
                                        .foregroundColor(stepMeta[stepIndex].micro > 0 ? .black : .gray)
                                }
                            }
                        }
                        .padding(2)
                    }
                }
                .frame(width: 54, height: 56)
            }
        }
        .padding(.horizontal, 4)
        .padding(.vertical, 3)
    }
    
    // MARK: - Footer (Tool status & stamps)
    @ViewBuilder
    func FooterView() -> some View {
        HStack(spacing: 4) {
            // Stamps
            Text("Stamps:")
                .font(.system(size: 9))
                .foregroundColor(.gray)
            
            BevelButton(label: "Accent", isActive: stamps.accent) {
                stamps.accent.toggle()
            }
            .frame(width: 40, height: 16)
            
            BevelButton(label: stamps.ratchet == 0 ? "Rat off" : "Rat ×\\(stamps.ratchet + 1)", 
                       isActive: stamps.ratchet > 0) {
                stamps.ratchet = (stamps.ratchet + 1) % 4
            }
            .frame(width: 48, height: 16)
            
            BevelButton(label: "Tie", isActive: stamps.tie) {
                stamps.tie.toggle()
            }
            .frame(width: 32, height: 16)
            
            BevelButton(label: microDisplayText, isActive: stamps.micro != 0) {
                stamps.micro = stamps.micro == 0 ? 1 : (stamps.micro == 1 ? -1 : 0)
            }
            .frame(width: 60, height: 16)
            
            Spacer()
            
            // Tool status
            Text(toolStatusText)
                .font(.system(size: 9))
                .foregroundColor(.gray)
        }
        .padding(.horizontal, 4)
        .frame(height: 24)
        .background(Color(red: 0.85, green: 0.85, blue: 0.85))
        .modifier(BevelOutModifier())
    }
    
    // MARK: - Initialization & Core Functions
    private func initializeSynth() {
        synthController.initialize()
        loadDefaultSettings()
    }
    
    private func loadDefaultSettings() {
        // Initialize with default engine assignments and parameter values
        for i in 0..<16 {
            instrumentEngines[i] = i % 14 // Cycle through 14 engines
            synthController.setInstrumentEngine(instrument: i, engineType: instrumentEngines[i])
        }
        
        // Initialize LFOs
        for i in 0..<8 {
            lfoSettings[i] = LFOSettings()
        }
        
        // Initialize EQ bands
        eqBands = [
            EQBandSettings(frequency: 40, gain: 0, q: 0.707, type: .highPass),
            EQBandSettings(frequency: 120, gain: 0, q: 0.707, type: .lowShelf),
            EQBandSettings(frequency: 400, gain: 0, q: 1.0, type: .bell),
            EQBandSettings(frequency: 1200, gain: 0, q: 1.0, type: .bell),
            EQBandSettings(frequency: 3500, gain: 0, q: 1.0, type: .bell),
            EQBandSettings(frequency: 10000, gain: 0, q: 0.707, type: .highShelf),
            EQBandSettings(frequency: 18000, gain: 0, q: 0.707, type: .highShelf)
        ]
    }
    
    // MARK: - Transport Functions (Real C++ Integration)
    private func togglePlayback() {
        if isPlaying {
            synthController.stop()
        } else {
            synthController.play()
        }
        isPlaying.toggle()
    }
    
    private func toggleRecording() {
        if isRecording {
            synthController.stopRecording()
        } else {
            synthController.startRecording()
        }
        isRecording.toggle()
    }
    
    // MARK: - Pattern Functions (Real C++ Integration)
    private func handleStepPress(_ stepIndex: Int) {
        let globalStep = currentPage * 16 + stepIndex
        guard globalStep < patternLength else { return }
        
        if let tool = activeTool {
            handleToolAction(tool, stepIndex: stepIndex)
        } else {
            toggleStep(stepIndex)
        }
    }
    
    private func toggleStep(_ stepIndex: Int) {
        steps[stepIndex].toggle()
        
        if steps[stepIndex] {
            // Add note to step
            stepNotes[stepIndex] = [armedInstrument] // Simple single note for now
            stepMeta[stepIndex] = StepMetadata(
                accent: stamps.accent,
                tie: stamps.tie,
                ratchet: stamps.ratchet + 1,
                micro: stamps.micro == 1 ? 30 : (stamps.micro == -1 ? -30 : 0),
                length: 1
            )
            
            // Trigger real C++ note event for pattern
            synthController.setPatternStep(
                pattern: currentPattern,
                step: currentPage * 16 + stepIndex,
                instrument: armedInstrument,
                velocity: 100,
                metadata: stepMeta[stepIndex]
            )
        } else {
            // Remove note from step
            stepNotes[stepIndex] = []
            stepMeta[stepIndex] = StepMetadata()
            
            // Clear step in C++ pattern
            synthController.clearPatternStep(
                pattern: currentPattern,
                step: currentPage * 16 + stepIndex
            )
        }
    }
    
    // ... [Continue with remaining view builders and functions]
}

// MARK: - Data Structures (Enhanced from JSX concept)

enum ViewMode: String, CaseIterable {
    case step = "STEP"
    case pad = "PAD"
    case engine = "ENGINE"
    case lfo = "LFO"
    case modulation = "MOD"
    case effects = "FX"
    case eq = "EQ"
    case chord = "CHORD"
    case chain = "CHAIN"
    case aiGen = "AI-GEN"
    case macro = "MACRO"
    case scene = "SCENE"
    case sample = "SAMPLE"
    case performance = "PERF"
    case smartKnob = "SMART"
    
    var displayName: String { rawValue }
    var supportsPages: Bool {
        switch self {
        case .step, .pad, .sample: return true
        default: return false
        }
    }
}

enum Tool: String, CaseIterable {
    case copy = "COPY"
    case cut = "CUT"
    case delete = "DELETE"
    case nudge = "NUDGE"
}

struct StepMetadata {
    var accent = false
    var tie = false
    var ratchet = 1
    var micro = 0
    var length = 1
}

struct StepStamps {
    var accent = false
    var tie = false
    var ratchet = 0
    var micro = 0
}

struct LFOSettings {
    var waveform = 0 // SINE
    var rate: Float = 1.0
    var depth: Float = 0.0
    var syncMode = 0 // FREE_RUNNING
    var enabled = false
}

struct EQBandSettings {
    var frequency: Float
    var gain: Float
    var q: Float
    var type: FilterType
    var enabled = true
    
    enum FilterType {
        case bell, highShelf, lowShelf, highPass, lowPass, notch
    }
}

struct EffectSettings {
    var type: EffectType
    var parameters: [Float]
    var enabled = true
    var wetDryMix: Float = 0.5
    
    enum EffectType {
        case reverb, delay, distortion, chorus, flanger, phaser
    }
}

struct MacroSettings {
    var name: String
    var assignments: [(instrument: Int, parameter: Int, amount: Float)]
    var type: MacroType
    
    enum MacroType {
        case parameterSet, patternTrigger, effectChain, sceneMorph
    }
}

struct SceneSettings {
    var name: String
    var snapshot: [String: Any] // Complete state snapshot
}

struct GenerationParams {
    var density: Float = 0.5
    var tension: Float = 0.5
    var creativity: Float = 0.5
    var responsiveness: Float = 0.5
}

struct PatternChain {
    var patterns: [Int]
    var name: String
    var looped = false
}

enum GenerationMode {
    case assist, generate, evolve, respond
}

enum MusicalStyle {
    case electronic, techno, house, ambient, dnb, acid
}

enum GenerationComplexity {
    case simple, moderate, complex, adaptive
}

// MARK: - Constants

let patternLabels = ["A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P"]

let engineNames = [
    "MacroVA", "MacroFM", "MacroWT", "MacroWS", "MacroHarm", "MacroChord",
    "Formant", "Noise", "TidesOsc", "RingsVoice", "Elements", "DrumKit",
    "SamplerKit", "SamplerSlicer"
]

let pastelColors: [Color] = [
    Color(red: 0.99, green: 0.89, blue: 0.89), // Light red
    Color(red: 0.89, green: 0.94, blue: 0.80), // Light green
    Color(red: 0.80, green: 0.91, blue: 0.94), // Light blue
    Color(red: 0.92, green: 0.84, blue: 0.90), // Light purple
    Color(red: 1.0, green: 0.95, blue: 0.73),  // Light yellow
    Color(red: 0.89, green: 0.93, blue: 0.97), // Light cyan
    Color(red: 0.95, green: 0.82, blue: 0.86), // Light pink
    Color(red: 0.84, green: 0.96, blue: 0.84), // Light mint
    Color(red: 0.92, green: 0.85, blue: 0.78), // Light peach
    Color(red: 0.85, green: 0.91, blue: 0.89), // Light teal
    Color(red: 0.90, green: 0.84, blue: 0.97), // Light lavender
    Color(red: 0.99, green: 0.91, blue: 0.78), // Light orange
    Color(red: 0.82, green: 0.89, blue: 0.99), // Light sky blue
    Color(red: 0.95, green: 0.91, blue: 1.0),  // Light violet
    Color(red: 0.90, green: 0.98, blue: 0.84), // Light lime
    Color(red: 1.0, green: 0.89, blue: 0.89)   // Light coral
]

struct OptionDefinition {
    let label: String
    let action: OptionAction
}

enum OptionAction {
    case showOverlay(String)
    case setTool(Tool)
    case toggleStamp(String)
    case setMode(ViewMode)
    case customAction(String)
}

let primaryOptions: [OptionDefinition] = [
    OptionDefinition(label: "INST", action: .showOverlay("inst")),
    OptionDefinition(label: "SOUND", action: .showOverlay("sound")),
    OptionDefinition(label: "PATT", action: .showOverlay("pattern")),
    OptionDefinition(label: "CLONE", action: .customAction("clone")),
    OptionDefinition(label: "COPY", action: .setTool(.copy)),
    OptionDefinition(label: "CUT", action: .setTool(.cut)),
    OptionDefinition(label: "DEL", action: .setTool(.delete)),
    OptionDefinition(label: "NUDGE", action: .setTool(.nudge)),
    OptionDefinition(label: "ACC", action: .toggleStamp("accent")),
    OptionDefinition(label: "RATCH", action: .toggleStamp("ratchet")),
    OptionDefinition(label: "TIE", action: .toggleStamp("tie")),
    OptionDefinition(label: "MICRO", action: .toggleStamp("micro")),
    OptionDefinition(label: "FX", action: .showOverlay("effects")),
    OptionDefinition(label: "QNT", action: .customAction("quantize")),
    OptionDefinition(label: "SWG", action: .customAction("swing")),
    OptionDefinition(label: "SCENE", action: .showOverlay("scene"))
]

let shiftOptions: [OptionDefinition] = [
    OptionDefinition(label: "ENGINE", action: .setMode(.engine)),
    OptionDefinition(label: "LFO", action: .setMode(.lfo)),
    OptionDefinition(label: "MOD", action: .setMode(.modulation)),
    OptionDefinition(label: "CHAIN", action: .setMode(.chain)),
    OptionDefinition(label: "MACRO", action: .setMode(.macro)),
    OptionDefinition(label: "CHORD", action: .setMode(.chord)),
    OptionDefinition(label: "EUC", action: .customAction("euclidean")),
    OptionDefinition(label: "AI-GEN", action: .setMode(.aiGen)),
    OptionDefinition(label: "EQ", action: .setMode(.eq)),
    OptionDefinition(label: "SPEC", action: .customAction("spectrum")),
    OptionDefinition(label: "PERF", action: .setMode(.performance)),
    OptionDefinition(label: "LOOP", action: .customAction("loop")),
    OptionDefinition(label: "SAMP", action: .setMode(.sample)),
    OptionDefinition(label: "SMART", action: .setMode(.smartKnob)),
    OptionDefinition(label: "MIDI", action: .customAction("midi")),
    OptionDefinition(label: "SYNC", action: .customAction("sync"))
]

// MARK: - UI Modifiers (JSX Bevel Styling)

struct BevelOutModifier: ViewModifier {
    func body(content: Content) -> some View {
        content
            .background(Color(red: 0.85, green: 0.85, blue: 0.85))
            .overlay(
                RoundedRectangle(cornerRadius: 0)
                    .stroke(Color.white, lineWidth: 2)
                    .shadow(color: Color(red: 0.5, green: 0.5, blue: 0.5), radius: 0, x: 1, y: 1)
                    .shadow(color: Color(red: 0.35, green: 0.35, blue: 0.35), radius: 0, x: 2, y: 2)
            )
    }
}

struct BevelInModifier: ViewModifier {
    func body(content: Content) -> some View {
        content
            .background(Color(red: 0.85, green: 0.85, blue: 0.85))
            .overlay(
                RoundedRectangle(cornerRadius: 0)
                    .stroke(Color(red: 0.5, green: 0.5, blue: 0.5), lineWidth: 1)
                    .shadow(color: Color.white, radius: 0, x: 1, y: 1)
                    .shadow(color: Color(red: 0.85, green: 0.85, blue: 0.85), radius: 0, x: 2, y: 2)
            )
    }
}

struct BevelButton: View {
    let label: String
    let isActive: Bool
    let action: () -> Void
    
    var body: some View {
        Button(action: action) {
            Text(label)
                .font(.system(size: 10))
                .foregroundColor(.black)
                .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
        .background(Color(red: 0.85, green: 0.85, blue: 0.85))
        .modifier(isActive ? BevelInModifier() : BevelOutModifier())
    }
}