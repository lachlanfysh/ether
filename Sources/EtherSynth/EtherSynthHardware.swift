import SwiftUI
import AVFoundation
import Combine

// MARK: - Hardware-Accurate EtherSynth Interface (2×16 Grid Layout)

struct EtherSynthHardware: View {
    @StateObject private var synthController = SynthController()
    
    // MARK: - Transport State (Transport-4: PLAY, REC, WRITE, SHIFT)
    @State private var isPlaying = false
    @State private var isRecording = false
    @State private var isWriting = false
    @State private var shiftHeld = false
    @State private var shiftLatched = false
    var shiftActive: Bool { shiftHeld || shiftLatched }
    
    // MARK: - Pattern & Timing (Using real SynthController data where possible)
    @State private var patternLength = 16
    @State private var currentPage = 0
    @State private var bpm: Float = 120.0
    @State private var swing: Float = 55.0
    @State private var quantize: Float = 50.0
    
    // MARK: - Step Sequencer (Bottom Row - Always Active)
    // Note: Using real SynthController data instead of mock @State arrays
    
    // MARK: - Instrument System (Using real SynthController data)
    @State private var instrumentEngines = Array(repeating: 0, count: 16)
    @State private var instrumentParameters: [[Float]] = Array(repeating: Array(repeating: 0.0, count: 16), count: 16)
    
    // MARK: - SHIFT Layer System (4 Quads)
    @State private var showShiftLegend = false
    @State private var activeTool: ToolType? = nil
    @State private var toolScope: ToolScope = .step
    
    // MARK: - Stamps (Apply to new steps)
    @State private var stamps = StepStamps()
    
    // MARK: - Overlays (Non-modal, steps remain active)
    @State private var showInstOverlay = false
    @State private var showSoundOverlay = false  
    @State private var showPatternOverlay = false
    @State private var showCloneOverlay = false
    @State private var showFXOverlay = false
    @State private var showSceneOverlay = false
    
    // MARK: - Advanced Features (Top-up spec)
    @State private var dropSettings = DropSettings()
    @State private var retriggerSettings = RetriggerSettings()
    @State private var scenes: [SceneSnapshot] = Array(repeating: SceneSnapshot(), count: 4)
    
    // MARK: - Performance State
    @State private var cpuUsage: Float = 12.0
    @State private var activeVoices = 6
    @State private var toastMessage: String? = nil
    
    var body: some View {
        VStack(spacing: 0) {
            HeaderView()
            
            // Main interface area with dynamic content
            MainContentArea()
            
            // Top Row (16) - Option Bank with SHIFT layers
            OptionBankView()
            
            // Bottom Row (16) - Step Sequencer (always active)
            StepSequencerView()
            
            FooterView()
        }
        .frame(width: 960, height: 320)
        .background(Color(UIColor.systemGray5))
        .overlay(
            RoundedRectangle(cornerRadius: 4)
                .stroke(Color.black, lineWidth: 1)
        )
        .modifier(BevelOutModifier())
        .overlay(
            Group {
                // All overlays (non-modal, steps remain active)
                if showInstOverlay { InstSetupOverlay() }
                if showSoundOverlay { SoundSelectOverlay() }
                if showPatternOverlay { PatternSelectOverlay() }
                if showCloneOverlay { ClonePatternOverlay() }
                if showFXOverlay { FXRepeatOverlay() }
                if showSceneOverlay { SceneSnapshotOverlay() }
                
                // SHIFT Legend
                if showShiftLegend { ShiftLegendOverlay() }
                
                // Toast notifications
                if let message = toastMessage {
                    ToastView(message: message)
                }
            }
        )
        .onAppear {
            initializeSynthesizer()
        }
    }
    
    // MARK: - Header (Transport + System Info)
    @ViewBuilder
    func HeaderView() -> some View {
        HStack(spacing: 4) {
            // Transport-4 Controls
            TransportButton(
                icon: isPlaying ? "ui-stop-1" : "ui-play-1",
                isActive: isPlaying
            ) {
                if shiftActive {
                    // SHIFT+PLAY = Tap Tempo
                    tapTempo()
                } else {
                    togglePlayback()
                }
            }
            
            TransportButton(
                icon: "ui-rec-1",
                isActive: isRecording
            ) {
                if shiftActive {
                    // SHIFT+REC = Instrument Setup
                    showInstOverlay = true
                } else {
                    toggleRecording()
                }
            }
            
            TransportButton(
                icon: "ui-write-1",
                isActive: isWriting
            ) {
                if shiftActive {
                    // SHIFT+WRITE = Toggle Step ↔ Pad mode
                    toggleStepPadMode()
                } else {
                    isWriting.toggle()
                }
            }
            
            TransportButton(
                icon: "ui-shift-1",
                isActive: shiftActive
            ) {
                // Handled by gesture recognizers
            }
            .onLongPressGesture(minimumDuration: 0, maximumDistance: .infinity) { pressing in
                shiftHeld = pressing
                showShiftLegend = pressing || shiftLatched
            } perform: {}
            .onTapGesture(count: 2) {
                shiftLatched.toggle()
                showShiftLegend = shiftLatched
            }
            
            Spacer()
            
            // Pattern & Timing Info
            SmallChip(
                label: "Pat \\(synthController.currentPatternID)",
                action: { showPatternOverlay = true }
            )
            
            SmallChip(
                label: "BPM \\(Int(bpm))",
                action: { adjustBPM(1) }
            )
            
            SmallChip(
                label: "Sw \\(Int(swing))%",
                action: { adjustSwing(1) }
            )
            
            SmallChip(
                label: "Q \\(Int(quantize))%",
                action: { adjustQuantize(1) }
            )
            
            Spacer()
            
            // System Monitoring
            SystemInfoView()
        }
        .padding(.horizontal, 6)
        .frame(height: 32)
        .background(Color(UIColor.systemGray6))
        .modifier(BevelOutModifier())
    }
    
    // MARK: - Main Content Area (720×160 approx)
    @ViewBuilder
    func MainContentArea() -> some View {
        let availableHeight: CGFloat = 160
        
        VStack(alignment: .leading, spacing: 2) {
            // Mode indicator and navigation
            HStack {
                Text("STEP — I\\(synthController.uiArmedInstrumentIndex + 1) (\\(engineNames[instrumentEngines[synthController.uiArmedInstrumentIndex]]))")
                    .font(.system(size: 10, weight: .medium))
                    .foregroundColor(.primary)
                
                Spacer()
                
                // Page dots for patterns >16 steps
                if patternLength > 16 {
                    HStack(spacing: 2) {
                        ForEach(0..<pageCount, id: \\.self) { pageIndex in
                            Circle()
                                .frame(width: 6, height: 6)
                                .foregroundColor(pageIndex == currentPage ? .primary : .secondary)
                                .onTapGesture { currentPage = pageIndex }
                        }
                    }
                }
                
                // Tool scope indicator (when tool active)
                if activeTool != nil {
                    Text("Scope: \\(toolScope.displayName)")
                        .font(.system(size: 9))
                        .padding(.horizontal, 6)
                        .padding(.vertical, 2)
                        .background(Color.blue.opacity(0.2))
                        .cornerRadius(3)
                }
            }
            .padding(.horizontal, 8)
            .padding(.top, 4)
            
            // Main view content (macros, pad visualization, etc.)
            HStack(spacing: 8) {
                // 4 Macro Knobs (M1-M4)
                ForEach(0..<4, id: \\.self) { macroIndex in
                    MacroKnobView(
                        label: "M\\(macroIndex + 1)",
                        value: .constant(0.5),
                        icon: macroIndex == 0 ? "ui-fx-lp-1" : 
                              macroIndex == 1 ? "ui-env-adsr-1" :
                              macroIndex == 2 ? "ui-lfo-sine-1" : "ui-velocity-1"
                    ) { value in
                        setMacroParameter(macroIndex, value: value)
                    }
                }
                
                Spacer()
                
                // Armed instrument visualization
                VStack {
                    IconView(name: engineIcons[instrumentEngines[synthController.uiArmedInstrumentIndex]], size: 40)
                    Text(engineNames[instrumentEngines[synthController.uiArmedInstrumentIndex]])
                        .font(.system(size: 8))
                        .foregroundColor(.secondary)
                }
                .padding(8)
                .background(pastelColors[synthController.uiArmedInstrumentIndex % pastelColors.count].opacity(0.3))
                .cornerRadius(6)
                .modifier(BevelInModifier())
                
                Spacer()
                
                // Active stamps indicator
                VStack(alignment: .leading, spacing: 2) {
                    Text("Stamps:")
                        .font(.system(size: 8))
                        .foregroundColor(.secondary)
                    
                    HStack(spacing: 4) {
                        if stamps.accent {
                            StampChip(icon: "ui-accent-1", label: "ACC")
                        }
                        if stamps.ratchet > 0 {
                            StampChip(icon: "ui-ratchet-1", label: "×\\(stamps.ratchet + 1)")
                        }
                        if stamps.tie {
                            StampChip(icon: "ui-tie-1", label: "TIE")
                        }
                        if stamps.micro != 0 {
                            StampChip(icon: "ui-micro-1", 
                                    label: stamps.micro > 0 ? "+25%" : "-25%")
                        }
                    }
                }
            }
            .padding(.horizontal, 8)
            .frame(maxHeight: .infinity)
        }
        .frame(height: availableHeight)
        .background(Color(UIColor.systemGray6))
        .modifier(BevelInModifier())
    }
    
    // MARK: - Option Bank (Top Row - 16 keys with SHIFT layers)
    @ViewBuilder
    func OptionBankView() -> some View {
        HStack(spacing: 2) {
            ForEach(0..<16, id: \\.self) { index in
                let option = getOptionForIndex(index)
                let isActive = isOptionActive(index)
                
                OptionButton(
                    icon: option.icon,
                    label: option.label,
                    isActive: isActive,
                    quadColor: getQuadColor(index)
                ) {
                    handleOptionPress(index)
                }
            }
        }
        .padding(.horizontal, 6)
        .frame(height: 64)
        .background(Color(UIColor.systemGray6))
        .modifier(BevelOutModifier())
    }
    
    // MARK: - Step Sequencer (Bottom Row - Always Active)
    @ViewBuilder  
    func StepSequencerView() -> some View {
        HStack(spacing: 2) {
            ForEach(0..<16, id: \\.self) { stepIndex in
                let globalStep = currentPage * 16 + stepIndex
                let isActive = globalStep < patternLength && synthController.stepHasContent(stepIndex)
                let accentColor = pastelColors[synthController.uiArmedInstrumentIndex % pastelColors.count]
                
                StepButton(
                    stepNumber: globalStep + 1,
                    isActive: isActive,
                    accentColor: accentColor,
                    metadata: StepMetadata(), // TODO: Get real metadata from SynthController
                    notes: Set<Int>() // TODO: Get real notes from SynthController
                ) {
                    handleStepPress(stepIndex)
                } onLongPress: {
                    openStepEditor(stepIndex)
                }
            }
        }
        .padding(.horizontal, 6)
        .frame(height: 64)
        .background(Color(UIColor.systemGray6))
        .modifier(BevelOutModifier())
    }
    
    // MARK: - Footer (Tool status and help)
    @ViewBuilder
    func FooterView() -> some View {
        HStack {
            if let tool = activeTool {
                HStack(spacing: 4) {
                    IconView(name: tool.icon, size: 12)
                    Text("\\(tool.rawValue.uppercased()): \\(tool.helpText) — PLAY to exit")
                        .font(.system(size: 9))
                        .foregroundColor(.secondary)
                }
            } else {
                Text("Ready — Hold SHIFT for functions")
                    .font(.system(size: 9))
                    .foregroundColor(.secondary)
            }
            
            Spacer()
            
            // Performance indicator
            Text("CPU \\(Int(cpuUsage))% • Voices \\(activeVoices)")
                .font(.system(size: 9))
                .foregroundColor(.secondary)
        }
        .padding(.horizontal, 8)
        .frame(height: 20)
        .background(Color(UIColor.systemGray6))
        .modifier(BevelOutModifier())
    }
    
    // MARK: - Core Functions
    
    private func initializeSynthesizer() {
        synthController.initialize()
        loadDefaultEngineAssignments()
    }
    
    private func loadDefaultEngineAssignments() {
        // Assign engines to instruments (cycling through available engines)
        for i in 0..<16 {
            let engineType = i % engineNames.count
            instrumentEngines[i] = engineType
            synthController.setInstrumentEngine(instrument: i, engineType: engineType)
        }
    }
    
    private func togglePlayback() {
        if isPlaying {
            synthController.stop()
            exitActiveTool()
        } else {
            synthController.play()
        }
        isPlaying.toggle()
        showToast(isPlaying ? "Playing" : "Stopped")
    }
    
    private func toggleRecording() {
        if isRecording {
            synthController.stopRecording()
        } else {
            synthController.startRecording()
        }
        isRecording.toggle()
        showToast(isRecording ? "Recording" : "Recording stopped")
    }
    
    private func tapTempo() {
        // Implement tap tempo logic
        bpm = min(200, bpm + 1)
        showToast("Tap Tempo: \\(Int(bpm)) BPM")
    }
    
    private func toggleStepPadMode() {
        showToast("Step ↔ Pad mode toggle")
    }
    
    // MARK: - Option Handling (2×16 Grid + SHIFT Layers)
    
    private func getOptionForIndex(_ index: Int) -> OptionDefinition {
        if shiftActive {
            return shiftOptions[index]
        } else {
            return primaryOptions[index]
        }
    }
    
    private func isOptionActive(_ index: Int) -> Bool {
        if shiftActive {
            return isShiftOptionActive(index)
        } else {
            return isPrimaryOptionActive(index)
        }
    }
    
    private func isPrimaryOptionActive(_ index: Int) -> Bool {
        switch index {
        case 4: return activeTool == .copy
        case 5: return activeTool == .cut
        case 6: return activeTool == .delete
        case 7: return activeTool == .nudge
        case 8: return stamps.accent
        case 9: return stamps.ratchet > 0
        case 10: return stamps.tie
        case 11: return stamps.micro != 0
        default: return false
        }
    }
    
    private func isShiftOptionActive(_ index: Int) -> Bool {
        // SHIFT layer activations
        return false
    }
    
    private func getQuadColor(_ index: Int) -> Color {
        let quad = index / 4
        switch quad {
        case 0: return .purple.opacity(0.6)  // Palette - Lilac
        case 1: return .blue.opacity(0.6)    // Tools - Blue
        case 2: return .orange.opacity(0.6)  // Stamps - Peach
        case 3: return .green.opacity(0.6)   // Performance - Mint
        default: return .gray.opacity(0.6)
        }
    }
    
    private func handleOptionPress(_ index: Int) {
        let option = getOptionForIndex(index)
        
        switch option.action {
        case .showOverlay(let overlayName):
            showOverlay(overlayName)
        case .setTool(let tool):
            setActiveTool(tool)
        case .toggleStamp(let stampName):
            toggleStamp(stampName)
        case .customAction(let actionName):
            handleCustomAction(actionName)
        }
    }
    
    private func showOverlay(_ overlayName: String) {
        hideAllOverlays()
        
        switch overlayName {
        case "inst": showInstOverlay = true
        case "sound": showSoundOverlay = true
        case "pattern": showPatternOverlay = true
        case "clone": showCloneOverlay = true
        case "fx": showFXOverlay = true
        case "scene": showSceneOverlay = true
        default: break
        }
    }
    
    private func setActiveTool(_ tool: Tool) {
        activeTool = tool
        showToast("\\(tool.rawValue.uppercased()) tool active — \\(tool.helpText)")
    }
    
    private func toggleStamp(_ stampName: String) {
        switch stampName {
        case "accent": stamps.accent.toggle()
        case "ratchet": stamps.ratchet = (stamps.ratchet + 1) % 4
        case "tie": stamps.tie.toggle()  
        case "micro": stamps.micro = stamps.micro == 0 ? 1 : (stamps.micro == 1 ? -1 : 0)
        default: break
        }
        updateStampDisplay()
    }
    
    private func handleCustomAction(_ actionName: String) {
        switch actionName {
        case "clone": showCloneOverlay = true
        case "quantize": applyQuantize()
        case "swing": adjustSwing(1)
        default: showToast("Action: \\(actionName)")
        }
    }
    
    // MARK: - Step Handling
    
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
        // Use SynthController's real toggleStep function instead of mock arrays
        synthController.toggleStep(stepIndex)
        
        // TODO: Apply stamps (accent, ratchet, tie, micro) to the toggled step
        // This requires extending SynthController to support step metadata/stamps
    }
    
    private func handleToolAction(_ tool: Tool, stepIndex: Int) {
        switch tool {
        case .copy:
            handleCopyTool(stepIndex: stepIndex)
        case .cut:
            handleCutTool(stepIndex: stepIndex)
        case .delete:
            handleDeleteTool(stepIndex: stepIndex)
        case .nudge:
            handleNudgeTool(stepIndex: stepIndex)
        }
    }
    
    private func exitActiveTool() {
        activeTool = nil
        toolScope = .step
    }
    
    // MARK: - Utility Functions
    
    private var pageCount: Int {
        return (patternLength + 15) / 16
    }
    
    private func hideAllOverlays() {
        showInstOverlay = false
        showSoundOverlay = false
        showPatternOverlay = false
        showCloneOverlay = false
        showFXOverlay = false
        showSceneOverlay = false
    }
    
    private func showToast(_ message: String) {
        toastMessage = message
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.5) {
            toastMessage = nil
        }
    }
    
    private func updateStampDisplay() {
        // Update UI to reflect stamp changes
    }
    
    private func setMacroParameter(_ macro: Int, value: Float) {
        synthController.setMacroParameter(
            instrument: synthController.uiArmedInstrumentIndex,
            macro: macro,
            value: value
        )
    }
    
    // MARK: - Tool Implementation Stubs
    private func handleCopyTool(stepIndex: Int) { showToast("Copy from step \\(stepIndex + 1)") }
    private func handleCutTool(stepIndex: Int) { showToast("Cut from step \\(stepIndex + 1)") }
    private func handleDeleteTool(stepIndex: Int) { showToast("Delete step \\(stepIndex + 1)") }
    private func handleNudgeTool(stepIndex: Int) { showToast("Nudge step \\(stepIndex + 1)") }
    private func openStepEditor(_ stepIndex: Int) { showToast("Edit step \\(stepIndex + 1)") }
    private func applyQuantize() { showToast("Quantize \\(Int(quantize))%") }
    private func adjustBPM(_ delta: Int) { bpm = max(60, min(200, bpm + Float(delta))) }
    private func adjustSwing(_ delta: Int) { swing = max(45, min(75, swing + Float(delta))) }
    private func adjustQuantize(_ delta: Int) { quantize = max(0, min(100, quantize + Float(delta))) }
}

// MARK: - Data Structures



struct StepStamps {
    var accent = false
    var tie = false
    var ratchet = 0  // 0=off, 1=×2, 2=×3, 3=×4
    var micro = 0    // -1=−25%, 0=0, 1=+25%
}

struct RetriggerSettings {
    var shape: RetriggerShape = .flat
    var spreadSemitones = 0
    var curve: RetriggerCurve = .linear
    var gainDecay: Float = 15.0
    var type: RetriggerType = .restart
    var startPercent: Float = 0.0
    var anchor: TimeAnchor = .keepStep
    var jitterMs: Float = 1.0
}

enum RetriggerShape: String, CaseIterable {
    case flat, up, down, upDown, random
}

enum RetriggerCurve: String, CaseIterable {
    case linear, exponential
}

enum RetriggerType: String, CaseIterable {
    case restart, `continue`, granular
}

enum TimeAnchor: String, CaseIterable {
    case keepStep, fixedRate
}

struct DropSettings {
    var mode: DropMode = .soloArmed
    var length: DropLength = .oneBa
    var filterSweep = false
    var latched = false
}

enum DropMode: String, CaseIterable {
    case soloArmed, muteArmed, killAll, kickOnly, drumsOnly
}

enum DropLength: String, CaseIterable {
    case hold, quarterBar, halfBar, oneBar, twoBars
}

struct SceneSnapshot {
    var name: String = ""
    var mixerState: [String: Any] = [:]
    var macroValues: [Float] = Array(repeating: 0.0, count: 16)
    var used = false
}

struct OptionDefinition {
    let icon: String
    let label: String
    let action: OptionAction
}

enum OptionAction {
    case showOverlay(String)
    case setTool(Tool)
    case toggleStamp(String)
    case customAction(String)
}

// MARK: - Constants

let patternLabels = ["A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P"]

let engineNames = [
    "MacroVA", "MacroFM", "MacroWT", "MacroWS", "MacroHarm", "MacroChord",
    "Formant", "Noise", "TidesOsc", "RingsVoice", "Elements", "DrumKit", 
    "SamplerKit", "SamplerSlicer"
]

let engineIcons = [
    "eng-analog-1", "eng-fm-1", "eng-wavetable-1", "eng-wavefold-1", "eng-string-1", "eng-modal-1",
    "eng-membrane-1", "eng-noise-1", "eng-granular-1", "eng-modal-1", "eng-string-1", "eng-kit-1",
    "eng-sampler-1", "eng-sliced-1"
]

let pastelColors: [Color] = [
    Color(red: 0.99, green: 0.89, blue: 0.89), Color(red: 0.89, green: 0.94, blue: 0.80),
    Color(red: 0.80, green: 0.91, blue: 0.94), Color(red: 0.92, green: 0.84, blue: 0.90),
    Color(red: 1.0, green: 0.95, blue: 0.73), Color(red: 0.89, green: 0.93, blue: 0.97),
    Color(red: 0.95, green: 0.82, blue: 0.86), Color(red: 0.84, green: 0.96, blue: 0.84),
    Color(red: 0.92, green: 0.85, blue: 0.78), Color(red: 0.85, green: 0.91, blue: 0.89),
    Color(red: 0.90, green: 0.84, blue: 0.97), Color(red: 0.99, green: 0.91, blue: 0.78),
    Color(red: 0.82, green: 0.89, blue: 0.99), Color(red: 0.95, green: 0.91, blue: 1.0),
    Color(red: 0.90, green: 0.98, blue: 0.84), Color(red: 1.0, green: 0.89, blue: 0.89)
]

// Primary Options (no SHIFT) - Quad layout matching your spec
let primaryOptions: [OptionDefinition] = [
    // Quad A - Pattern/Instrument (P1-P4)
    OptionDefinition(icon: "ui-inst-1", label: "INST", action: .showOverlay("inst")),
    OptionDefinition(icon: "ui-sound-1", label: "SOUND", action: .showOverlay("sound")),
    OptionDefinition(icon: "ui-pattern-1", label: "PATT", action: .showOverlay("pattern")),
    OptionDefinition(icon: "ui-clone-1", label: "CLONE", action: .customAction("clone")),
    
    // Quad B - Edit Tools (P5-P8)
    OptionDefinition(icon: "ui-copy-1", label: "COPY", action: .setTool(.copy)),
    OptionDefinition(icon: "ui-cut-1", label: "CUT", action: .setTool(.cut)),
    OptionDefinition(icon: "ui-delete-1", label: "DEL", action: .setTool(.delete)),
    OptionDefinition(icon: "ui-nudge-1", label: "NUDGE", action: .setTool(.nudge)),
    
    // Quad C - Stamps (P9-P12)
    OptionDefinition(icon: "ui-accent-1", label: "ACC", action: .toggleStamp("accent")),
    OptionDefinition(icon: "ui-ratchet-1", label: "RATCH", action: .toggleStamp("ratchet")),
    OptionDefinition(icon: "ui-tie-1", label: "TIE", action: .toggleStamp("tie")),
    OptionDefinition(icon: "ui-micro-1", label: "MICRO", action: .toggleStamp("micro")),
    
    // Quad D - Performance/Mix (P13-P16)
    OptionDefinition(icon: "ui-fx-1", label: "FX", action: .showOverlay("fx")),
    OptionDefinition(icon: "ui-quant-1", label: "QUANT", action: .customAction("quantize")),
    OptionDefinition(icon: "ui-swing-1", label: "SWING", action: .customAction("swing")),
    OptionDefinition(icon: "ui-scene-1", label: "SCENE", action: .showOverlay("scene"))
]

// SHIFT Options - Extended functions
let shiftOptions: [OptionDefinition] = [
    // Advanced options exposed via SHIFT
    OptionDefinition(icon: "eng-analog-1", label: "ENGINE", action: .customAction("engine")),
    OptionDefinition(icon: "ui-lfo-sine-1", label: "LFO", action: .customAction("lfo")),
    OptionDefinition(icon: "ui-mod-matrix-1", label: "MOD", action: .customAction("modulation")),
    OptionDefinition(icon: "ui-pattern-chain-1", label: "CHAIN", action: .customAction("chain")),
    OptionDefinition(icon: "ui-chord-1", label: "CHORD", action: .customAction("chord")),
    OptionDefinition(icon: "ui-drop-1", label: "DROP", action: .customAction("drop")),
    OptionDefinition(icon: "ui-zipper-1", label: "ZIPPER", action: .customAction("zipper")),
    OptionDefinition(icon: "ui-velocity-1", label: "VEL", action: .customAction("velocity")),
    OptionDefinition(icon: "ui-fx-lp-1", label: "EQ", action: .customAction("eq")),
    OptionDefinition(icon: "ui-prob-1", label: "PROB", action: .customAction("probability")),
    OptionDefinition(icon: "ui-cond-1", label: "COND", action: .customAction("conditional")),
    OptionDefinition(icon: "ui-glide-1", label: "GLIDE", action: .customAction("glide")),
    OptionDefinition(icon: "ui-scale-lock-1", label: "SCALE", action: .customAction("scale")),
    OptionDefinition(icon: "ui-sync-1", label: "SYNC", action: .customAction("sync")),
    OptionDefinition(icon: "ui-tap-1", label: "TAP", action: .customAction("tap")),
    OptionDefinition(icon: "ui-scene-morph-1", label: "MORPH", action: .customAction("morph"))
]