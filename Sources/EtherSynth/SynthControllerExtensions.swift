import SwiftUI
import Combine

// MARK: - Instrument Color Enum

enum InstrumentColor: String, CaseIterable {
    case coral = "Coral"
    case peach = "Peach"  
    case cream = "Cream"
    case sage = "Sage"
    case teal = "Teal"
    case slate = "Slate"
    case pearl = "Pearl"
    case stone = "Stone"
    
    var color: Color {
        switch self {
        case .coral: return Color(red: 1.0, green: 0.5, blue: 0.3)
        case .peach: return Color(red: 1.0, green: 0.7, blue: 0.5)
        case .cream: return Color(red: 1.0, green: 0.9, blue: 0.7)
        case .sage: return Color(red: 0.6, green: 0.8, blue: 0.6)
        case .teal: return Color(red: 0.3, green: 0.8, blue: 0.8)
        case .slate: return Color(red: 0.5, green: 0.5, blue: 0.6)
        case .pearl: return Color(red: 0.9, green: 0.9, blue: 0.9)
        case .stone: return Color(red: 0.4, green: 0.4, blue: 0.4)
        }
    }
    
    var index: Int {
        switch self {
        case .coral: return 0
        case .peach: return 1
        case .cream: return 2
        case .sage: return 3
        case .teal: return 4
        case .slate: return 5
        case .pearl: return 6
        case .stone: return 7
        }
    }
}

// MARK: - SynthController Extensions for New UI Architecture

extension SynthController {
    
    // MARK: - Hardware Integration
    
    /// Publisher for hardware button events
    var hardwareButtonPublisher: AnyPublisher<HardwareButtonPress, Never> {
        // Simulate hardware button presses for now
        // In real implementation, this would connect to actual hardware
        Just(HardwareButtonPress(buttonType: .topButton(0), isPressed: true))
            .eraseToAnyPublisher()
    }
    
    // MARK: - UI State Properties
    
    var uiCpuUsage: Int {
        // Mock CPU usage - in real implementation, get from audio engine
        return Int.random(in: 10...25)
    }
    
    var uiActiveVoices: Int {
        // Mock active voices - in real implementation, get from audio engine
        return Int.random(in: 0...12)
    }
    
    var songName: String {
        return "Untitled"
    }
    
    var songKey: String {
        return UserDefaults.standard.string(forKey: "songKey") ?? "C"
    }
    
    var songScale: String {
        return UserDefaults.standard.string(forKey: "songScale") ?? "Major"
    }
    
    var chainMode: String {
        return UserDefaults.standard.string(forKey: "chainMode") ?? "Auto"
    }
    
    var globalSwing: Float {
        let value = UserDefaults.standard.float(forKey: "globalSwing")
        return value == 0 ? 0.56 : value
    }
    
    var globalQuantize: Float {
        let value = UserDefaults.standard.float(forKey: "globalQuantize")
        return value == 0 ? 0.75 : value
    }
    
    var uiBPM: Float {
        let value = UserDefaults.standard.float(forKey: "masterBPM")
        return value == 0 ? 128.0 : value
    }
    
    var uiMasterVolume: Float {
        let value = UserDefaults.standard.float(forKey: "masterVolume")
        return value == 0 ? 0.85 : value
    }
    
    // MARK: - Pattern Properties
    
    var currentPatternPage: Int {
        return 0 // TODO: Implement pattern paging
    }
    
    var currentSwing: Float {
        return 0.56
    }
    
    var currentQuantize: Float {
        return 0.75
    }
    
    // MARK: - Stamp System
    
    var activeStamps: StampState {
        return StampState(accent: false, ratchet: 0, tie: false, micro: 0)
    }
    
    var activeTool: ToolType? {
        return nil // TODO: Implement tool system
    }
    
    var toolScope: ToolScope {
        return .step
    }
    
    // MARK: - FX System
    
    var noteRepeatRate: String {
        return "1/16"
    }
    
    var noteRepeatRamp: String {
        return "Flat"
    }
    
    var noteRepeatSpread: Int {
        return 0
    }
    
    var isFXLatched: Bool {
        return false
    }
    
    
    // MARK: - Pattern Management
    
    func getPatternLength(_ patternIndex: Int? = nil) -> Int {
        // TODO: Implement pattern length management
        return 16
    }
    
    func getTotalPatternPages() -> Int {
        return max(1, getPatternLength() / 16)
    }
    
    func switchToPatternPage(_ pageIndex: Int) {
        // TODO: Implement pattern page switching
    }
    
    func setPatternLength(_ length: Int) {
        // TODO: Implement pattern length setting
    }
    
    // MARK: - Step Data Management
    
    func getStepData(_ stepIndex: Int) -> StepData {
        // Generate mock step data for demonstration
        let hasArmedNote = Bool.random() && stepIndex % 3 == 0 // Some steps have armed instrument notes
        let hasOtherNotes = Bool.random() && stepIndex % 4 == 0 // Some steps have other instrument notes
        
        var noteEvents: [NoteEvent] = []
        
        // Add armed instrument note if present
        if hasArmedNote {
            let note = NoteEvent(
                instrument: UInt32(uiArmedInstrumentIndex),
                pitch: UInt8(60 + Int.random(in: -12...12)), // C4 ± octave
                velocity: UInt8(Int.random(in: 80...127))
            )
            noteEvents.append(note)
        }
        
        // Add other instrument notes if present
        if hasOtherNotes {
            let otherCount = Int.random(in: 1...6)
            for i in 0..<otherCount {
                let instrumentIndex = (uiArmedInstrumentIndex + i + 1) % 16
                let note = NoteEvent(
                    instrument: UInt32(instrumentIndex),
                    pitch: UInt8(60 + Int.random(in: -12...12)),
                    velocity: UInt8(Int.random(in: 60...100))
                )
                noteEvents.append(note)
            }
        }
        
        return StepData(
            noteEvents: noteEvents,
            hasMetadata: hasArmedNote || hasOtherNotes,
            tie: Bool.random() && hasArmedNote
        )
    }
    
    func toggleStep(_ stepIndex: Int) {
        guard let pattern = currentPattern else {
            print("❌ No current pattern to edit")
            return
        }
        
        // Use selected track and armed instrument
        let trackIndex = selectedTrack
        let instrumentSlot = activeInstrument.index
        let pitch: UInt8 = 60 // Middle C default
        let velocity: UInt8 = 100
        
        pattern.toggleStep(track: trackIndex, step: stepIndex, pitch: pitch, velocity: velocity)
        print("✅ Toggled step \(stepIndex) on track \(trackIndex) for instrument \(activeInstrument.rawValue)")
        
        // Update UI
        objectWillChange.send()
    }
    
    /// Check if a step has content (for UI display)
    func stepHasContent(_ stepIndex: Int) -> Bool {
        guard let pattern = currentPattern else { return false }
        return pattern.hasContent(track: selectedTrack, step: stepIndex)
    }
    
    func openStepEditor(_ stepIndex: Int) {
        print("Open step editor for step \(stepIndex)")
        // TODO: Implement step editor
    }
    
    // MARK: - Instrument Management
    
    func getInstrumentTypeName() -> String {
        let selectedEngine = getSelectedEngine() ?? "MacroVA"
        return getEngineTypeName(for: selectedEngine)
    }
    
    func getInstrumentName(_ instrumentIndex: Int) -> String {
        let engineName = UserDefaults.standard.string(forKey: "selectedEngineName_\(instrumentIndex)") ?? "MacroVA"
        return engineName
    }
    
    private func getEngineTypeName(for engineName: String) -> String {
        switch engineName {
        case "MacroVA": return "Virtual Analog"
        case "MacroFM": return "FM Synthesis"
        case "MacroWT": return "Wavetable"
        case "MacroWS": return "Waveshaper"
        case "MacroChord": return "Chord Generator"
        case "MacroHarmonics": return "Harmonic Series"
        case "MacroWaveshaper": return "Advanced Waveshaping"
        case "MacroWavetable": return "Advanced Wavetable"
        case "NoiseEngine": return "Noise Generator"
        case "FormantEngine": return "Vocal Formants"
        case "RingsVoice": return "Physical Modeling"
        case "TidesOsc": return "Complex Oscillator"
        case "ElementsVoice": return "Modal Synthesis"
        case "SerialHPLP": return "Serial Filter"
        case "SlideAccentBass": return "Slide Bass"
        case "SamplerSlicer": return "Sample Slicer"
        default: return "Synthesizer"
        }
    }
    
    func getInstrumentColor(_ instrumentIndex: Int) -> Color {
        let colors: [Color] = [
            .red, .orange, .yellow, .green, .blue, .purple, .pink, .gray,
            .red, .orange, .yellow, .green, .blue, .purple, .pink, .gray
        ]
        return colors[instrumentIndex % colors.count]
    }
    
    func getArmedInstrumentColor() -> Color {
        return getInstrumentColor(Int(uiArmedInstrumentIndex))
    }
    
    func getInstrumentActivityLevel(_ instrumentIndex: Int) -> Float {
        return Float.random(in: 0...1) // TODO: Get real activity level
    }
    
    func getNoteDisplayName(_ note: NoteEvent) -> String {
        let noteNames = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
        let octave = Int(note.pitch) / 12 - 1
        let noteName = noteNames[Int(note.pitch) % 12]
        return "\(noteName)\(octave)"
    }
    
    var uiArmedInstrumentIndex: Int {
        get { UserDefaults.standard.integer(forKey: "uiArmedInstrument") }
        set { UserDefaults.standard.set(newValue, forKey: "uiArmedInstrument") }
    }
    
    func uiArmInstrument(_ instrumentIndex: Int) {
        uiArmedInstrumentIndex = instrumentIndex
    }
    
    func isInstrumentUnassigned() -> Bool {
        return getInstrumentName(Int(uiArmedInstrumentIndex)) == "Empty slot"
    }
    
    func showEngineSelector() {
        print("🔧 DEBUG: showEngineSelector() called!")
        print("Available engines:")
        print("  0: MacroVA          - Virtual Analog")
        print("  1: MacroFM          - FM Synthesis") 
        print("  2: MacroWT          - Wavetable")
        print("  3: MacroWS          - Waveshaper")
        print("  4: MacroChord       - Chord Generator")
        print("  5: MacroHarmonics   - Harmonic Series")
        print("  6: MacroWaveshaper  - Advanced Waveshaping")
        print("  7: MacroWavetable   - Advanced Wavetable")
        print("  8: NoiseEngine      - Noise Generator")
        print("  9: FormantEngine    - Vocal Formants")
        print(" 10: RingsVoice       - Mutable Rings")
        print(" 11: TidesOsc         - Mutable Tides")
        print(" 12: ElementsVoice    - Mutable Elements")
        print(" 13: SerialHPLP       - Serial Filter")
        print(" 14: SlideAccentBass  - Slide Bass")
        print(" 15: SamplerSlicer    - Sample Slicer")
        
        // Auto-cycle through engines for demo
        let currentEngine = getSelectedEngine() ?? "MacroVA"
        let engines = [
            "MacroVA", "MacroFM", "MacroWT", "MacroWS", 
            "MacroChord", "MacroHarmonics", "MacroWaveshaper", "MacroWavetable",
            "NoiseEngine", "FormantEngine", "RingsVoice", "TidesOsc",
            "ElementsVoice", "SerialHPLP", "SlideAccentBass", "SamplerSlicer"
        ]
        
        if let currentIndex = engines.firstIndex(of: currentEngine) {
            let nextIndex = (currentIndex + 1) % engines.count
            let nextEngine = engines[nextIndex]
            
            print("🔄 Switching from \(currentEngine) to \(nextEngine)")
            
            // Store the selection
            UserDefaults.standard.set(nextIndex, forKey: "selectedEngineType_\(uiArmedInstrumentIndex)")
            UserDefaults.standard.set(nextEngine, forKey: "selectedEngineName_\(uiArmedInstrumentIndex)")
            
            // Set engine type via C++ bridge
            setEngineType(for: Int(uiArmedInstrumentIndex), engineType: nextIndex)
            print("📡 Called ether_set_engine_type(\(nextIndex)) for instrument \(uiArmedInstrumentIndex)")
            
            print("✅ Engine switched to \(nextEngine) for instrument \(uiArmedInstrumentIndex)")
        } else {
            // Default to first engine
            print("🔄 Setting default engine: MacroVA")
            UserDefaults.standard.set(0, forKey: "selectedEngineType_\(uiArmedInstrumentIndex)")
            UserDefaults.standard.set("MacroVA", forKey: "selectedEngineName_\(uiArmedInstrumentIndex)")
        }
        
        // Force UI update to refresh parameters
        DispatchQueue.main.async {
            print("🔄 Refreshing UI with new engine parameters")
            self.objectWillChange.send()
        }
    }
    
    func setEngineType(for instrumentIndex: Int, engineType: Int) {
        print("🔧 Setting instrument \(instrumentIndex) to engine type \(engineType)")
        
        // Use real C++ bridge function
        if let engine = cppSynthEngine {
            ether_set_instrument_engine_type(engine, Int32(instrumentIndex), Int32(engineType))
            print("📡 Called ether_set_instrument_engine_type(\(instrumentIndex), \(engineType))")
        } else {
            print("❌ No C++ engine available for engine switching")
        }
        
        print("✅ Engine type \(engineType) set for instrument \(instrumentIndex)")
    }
    
    
    func showPresetSelector() {
        print("Show preset selector")
        // TODO: Implement preset selector
    }
    
    func getSelectedEngine() -> String? {
        guard !isInstrumentUnassigned() else { return nil }
        
        // Get stored engine name for current instrument
        let engineName = UserDefaults.standard.string(forKey: "selectedEngineName_\(uiArmedInstrumentIndex)")
        return engineName ?? "MacroVA" // Default to MacroVA if none stored
    }
    
    func getSelectedPreset() -> String? {
        return isInstrumentUnassigned() ? nil : "Warm Lead"
    }
    
    // MARK: - Parameter System
    
    var currentParameterPage: Int {
        return _currentParameterPage
    }
    
    var selectedParameterIndex: Int? {
        return _selectedParameterIndex
    }
    
    private var _currentParameterPage: Int {
        get { UserDefaults.standard.integer(forKey: "currentParameterPage") }
        set { UserDefaults.standard.set(newValue, forKey: "currentParameterPage") }
    }
    
    private var _selectedParameterIndex: Int? {
        get { 
            let value = UserDefaults.standard.integer(forKey: "selectedParameterIndex") 
            return value == -1 ? nil : value
        }
        set { UserDefaults.standard.set(newValue ?? -1, forKey: "selectedParameterIndex") }
    }
    
    func getTotalParameterPages() -> Int {
        return 2 // Most instruments have 16+ parameters
    }
    
    func getParameterData(_ paramIndex: Int) -> ParameterData {
        // Get engine-specific parameters based on selected engine
        let currentEngine = getSelectedEngine() ?? "MacroVA"
        let parameterSet = getEngineParameters(for: currentEngine)
        
        let name = parameterSet[paramIndex % parameterSet.count]
        let value = getStoredParameterValue(paramIndex) ?? Float(paramIndex) / 16.0
        
        return ParameterData(
            name: name,
            value: value,
            displayValue: formatParameterValue(name: name, value: value)
        )
    }
    
    private func getEngineParameters(for engineName: String) -> [String] {
        // Consistent parameter layout:
        // 1-4: Engine-specific primary controls
        // 5-8: ADSR Envelope (Attack, Decay, Sustain, Release)  
        // 9-12: Filter/Tone shaping
        // 13-14: Modulation (LFO, Mod)
        // 15: Tuning/Pitch
        // 16: Volume (always last)
        
        switch engineName {
        case "MacroVA":
            return ["Wav", "PWM", "Sub", "Drv", "Atk", "Dec", "Sus", "Rel", "Cut", "Res", "Env", "Flt", "LFO", "Mod", "Tun", "Vol"]
            
        case "MacroFM":
            return ["Car", "Mod", "Rat", "Alg", "Atk", "Dec", "Sus", "Rel", "FB", "Op1", "Op2", "Op3", "LFO", "Mod", "Tun", "Vol"]
            
        case "MacroWT":
            return ["Wav", "Pos", "Siz", "Mor", "Atk", "Dec", "Sus", "Rel", "Cut", "Res", "Env", "Flt", "LFO", "Mod", "Tun", "Vol"]
            
        case "SamplerSlicer":
            return ["Smp", "St", "End", "Lp", "Atk", "Dec", "Sus", "Rel", "Cut", "Res", "Pan", "Rev", "LFO", "Mod", "Pit", "Vol"]
            
        case "NoiseEngine":
            return ["Col", "Drv", "Sat", "Shp", "Atk", "Dec", "Sus", "Rel", "Cut", "Res", "BP", "HP", "LFO", "Mod", "Tun", "Vol"]
            
        case "FormantEngine":
            return ["Vow", "Con", "For", "Mor", "Atk", "Dec", "Sus", "Rel", "Q", "Brth", "Nse", "Flt", "LFO", "Mod", "Pit", "Vol"]
            
        case "RingsVoice":
            return ["Str", "Pos", "Dmp", "Exc", "Atk", "Dec", "Sus", "Rel", "Res", "Brg", "Pol", "Int", "LFO", "Mod", "Tun", "Vol"]
            
        case "ElementsVoice":
            return ["Bow", "Blw", "Str", "Geo", "Atk", "Dec", "Sus", "Rel", "Brg", "Dmp", "Pos", "Spc", "LFO", "Mod", "Tun", "Vol"]
            
        case "TidesOsc":
            return ["Frq", "Sha", "Slo", "Shi", "Atk", "Dec", "Sus", "Rel", "Smt", "FM", "AM", "Out", "LFO", "Mod", "Tun", "Vol"]
            
        case "MacroChord":
            return ["Chd", "Inv", "Spr", "Vel", "Atk", "Dec", "Sus", "Rel", "Cut", "Res", "Pan", "Arp", "LFO", "Mod", "Tun", "Vol"]
            
        case "MacroHarmonics":
            return ["Fun", "Har", "Amp", "Pha", "Atk", "Dec", "Sus", "Rel", "Cut", "Res", "Mor", "Env", "LFO", "Mod", "Tun", "Vol"]
            
        case "MacroWaveshaper":
            return ["Shp", "Drv", "Sym", "Fld", "Atk", "Dec", "Sus", "Rel", "Cut", "Res", "Amt", "Out", "LFO", "Mod", "Tun", "Vol"]
            
        case "SerialHPLP":
            return ["HP", "LP", "BP", "Ord", "Atk", "Dec", "Sus", "Rel", "Cut", "Res", "Drv", "Sat", "LFO", "Mod", "Tun", "Vol"]
            
        case "SlideAccentBass":
            return ["Sld", "Acc", "Sub", "Gld", "Atk", "Dec", "Sus", "Rel", "Cut", "Res", "Drv", "Pit", "LFO", "Mod", "Tun", "Vol"]
            
        default:
            return ["Src1", "Src2", "Mix", "Shp", "Atk", "Dec", "Sus", "Rel", "Cut", "Res", "Env", "Flt", "LFO", "Mod", "Tun", "Vol"]
        }
    }
    
    private func formatParameterValue(name: String, value: Float) -> String {
        switch name {
        case "Cut", "Frq", "Pit":
            // Frequency parameters - show in Hz/semitones
            return String(format: "%.0f", value * 20000) + "Hz"
        case "Res", "Q":
            // Resonance/Q - show as percentage
            return String(format: "%.0f", value * 100) + "%"
        case "Atk", "Dec", "Rel", "Att":
            // Time parameters - show in ms/s
            if value < 0.1 {
                return String(format: "%.0f", value * 1000) + "ms"
            } else {
                return String(format: "%.1f", value * 10) + "s"
            }
        case "Vol", "Sus", "Amt":
            // Volume/level parameters - show as percentage
            return String(format: "%.0f", value * 100) + "%"
        case "Pan":
            // Pan - show L/R
            let panValue = (value - 0.5) * 2
            if abs(panValue) < 0.1 {
                return "C"
            } else if panValue < 0 {
                return String(format: "L%.0f", abs(panValue) * 100)
            } else {
                return String(format: "R%.0f", panValue * 100)
            }
        case "Tun":
            // Tuning in cents
            return "\(Int((value - 0.5) * 24))c"
        default:
            // Generic parameters - show as 0-100
            return String(format: "%.0f", value * 100)
        }
    }
    
    private func formatParameterValue(_ value: Float, unit: String) -> String {
        switch unit.lowercased() {
        case "hz":
            return "\(Int(value))Hz"
        case "ms":
            return "\(Int(value * 1000))ms"
        case "db":
            return "\(Int(value))dB"
        case "cents", "c":
            return "\(Int(value))c"
        case "%", "percent":
            return "\(Int(value * 100))%"
        default:
            return String(format: "%.2f", value)
        }
    }
    
    func selectParameter(_ paramIndex: Int) {
        let actualIndex = (currentParameterPage * 16) + paramIndex
        _selectedParameterIndex = actualIndex
        print("Selected parameter \(actualIndex) (page \(currentParameterPage), index \(paramIndex))")
    }
    
    func switchParameterPage(_ pageIndex: Int) {
        guard pageIndex >= 0 && pageIndex < getTotalParameterPages() else { return }
        _currentParameterPage = pageIndex
        _selectedParameterIndex = nil // Clear selection when changing pages
        print("Switched to parameter page \(pageIndex)")
    }
    
    func getParameterPageData() -> [ParameterData] {
        let startIndex = currentParameterPage * 16
        return (0..<16).map { index in
            getParameterData(startIndex + index)
        }
    }
    
    func handleBottomButtonPress(_ buttonIndex: Int, isShiftHeld: Bool) {
        if isShiftHeld && getTotalParameterPages() > 1 {
            // SHIFT + button switches to page 2 (or cycles through pages)
            let targetPage = (currentParameterPage + 1) % getTotalParameterPages()
            switchParameterPage(targetPage)
        } else {
            // Regular button press selects parameter
            selectParameter(buttonIndex)
        }
    }
    
    func adjustSelectedParameter(_ delta: Float) {
        guard let selectedIndex = selectedParameterIndex else { return }
        
        // Get current parameter data
        var currentData = getParameterData(selectedIndex)
        
        // Adjust value with delta (normalized -1.0 to 1.0)
        var newValue = currentData.value + (delta * 0.01) // Scale knob sensitivity
        newValue = max(0.0, min(1.0, newValue)) // Clamp to valid range
        
        // Update parameter value (this would connect to actual synth engine in real implementation)
        updateParameterValue(selectedIndex, value: newValue)
        
        print("Adjusted parameter \(selectedIndex) from \(currentData.value) to \(newValue)")
    }
    
    func updateParameterValue(_ paramIndex: Int, value: Float) {
        // Store parameter values in UserDefaults for now
        // In real implementation, this would update the actual synth engine
        let key = "param_\(uiArmedInstrumentIndex)_\(paramIndex)"
        UserDefaults.standard.set(value, forKey: key)
    }
    
    func getStoredParameterValue(_ paramIndex: Int) -> Float? {
        let key = "param_\(uiArmedInstrumentIndex)_\(paramIndex)"
        let value = UserDefaults.standard.float(forKey: key)
        return value == 0 && UserDefaults.standard.object(forKey: key) == nil ? nil : value
    }
    
    func handleSmartKnobRotation(_ delta: Float) {
        adjustSelectedParameter(delta)
    }
    
    // MARK: - Song Management
    
    func getTotalSongBars() -> Int {
        return 142 // Mock song length
    }
    
    func getCurrentSongBar() -> Int {
        return 5 // Mock current position
    }
    
    func getSongPatternSequence() -> [SongPatternBlock] {
        // Mock realistic song arrangement
        return [
            SongPatternBlock(patternID: "A", sectionName: "Intro", barIndex: 0, isCurrentlyPlaying: false),
            SongPatternBlock(patternID: "A", sectionName: "Intro", barIndex: 4, isCurrentlyPlaying: false),
            SongPatternBlock(patternID: "B", sectionName: "Verse", barIndex: 8, isCurrentlyPlaying: false),
            SongPatternBlock(patternID: "B", sectionName: "Verse", barIndex: 12, isCurrentlyPlaying: false),
            SongPatternBlock(patternID: "C", sectionName: "Chorus", barIndex: 16, isCurrentlyPlaying: true),
            SongPatternBlock(patternID: "C", sectionName: "Chorus", barIndex: 20, isCurrentlyPlaying: false),
            SongPatternBlock(patternID: "B", sectionName: "Verse", barIndex: 24, isCurrentlyPlaying: false),
            SongPatternBlock(patternID: "C", sectionName: "Chorus", barIndex: 28, isCurrentlyPlaying: false),
            SongPatternBlock(patternID: "D", sectionName: "Bridge", barIndex: 32, isCurrentlyPlaying: false),
            SongPatternBlock(patternID: "C", sectionName: "Chorus", barIndex: 36, isCurrentlyPlaying: false),
            SongPatternBlock(patternID: "A", sectionName: "Outro", barIndex: 40, isCurrentlyPlaying: false)
        ]
    }
    
    func getSongSections() -> [SongSection] {
        return [
            SongSection(name: "Intro", type: .intro, patternBlocks: [
                PatternBlock(patternId: "A", length: 8)
            ], color: .gray),
            SongSection(name: "Verse", type: .verse, patternBlocks: [
                PatternBlock(patternId: "B", length: 8)
            ], color: .blue),
            SongSection(name: "Chorus", type: .chorus, patternBlocks: [
                PatternBlock(patternId: "C", length: 4)
            ], color: .green), 
            SongSection(name: "Bridge", type: .bridge, patternBlocks: [
                PatternBlock(patternId: "D", length: 4)
            ], color: .orange),
            SongSection(name: "Outro", type: .outro, patternBlocks: [
                PatternBlock(patternId: "A", length: 4)
            ], color: .purple)
        ]
    }
    
    func tapTempo() {
        print("Tap tempo")
        // TODO: Implement tap tempo
    }
    
    func setChainMode(_ mode: String) {
        print("Set chain mode: \(mode)")
        UserDefaults.standard.set(mode, forKey: "chainMode")
        // TODO: Implement chain mode
    }
    
    func jumpToPreviousSection() {
        // TODO: Jump to previous song section
        print("Jump to previous section")
    }
    
    func jumpToNextSection() {
        // TODO: Jump to next song section
        print("Jump to next section")
    }
    
    func addSongSection() {
        // TODO: Add new song section
        print("Add song section")
    }
    
    func editSongSection(_ sectionIndex: Int) {
        // TODO: Edit song section
        print("Edit section \(sectionIndex)")
    }
    
    func setSongKey(_ key: String) {
        print("Set song key: \(key)")
        UserDefaults.standard.set(key, forKey: "songKey")
    }
    
    func setSongScale(_ scale: String) {
        print("Set song scale: \(scale)")
        UserDefaults.standard.set(scale, forKey: "songScale")
    }
    
    func adjustGlobalSwing(_ delta: Float) {
        let newValue = max(0.0, min(1.0, globalSwing + delta))
        UserDefaults.standard.set(newValue, forKey: "globalSwing")
        print("Global swing: \(Int(newValue * 100))%")
    }
    
    func adjustGlobalQuantize(_ delta: Float) {
        let newValue = max(0.0, min(1.0, globalQuantize + delta))
        UserDefaults.standard.set(newValue, forKey: "globalQuantize") 
        print("Global quantize: \(Int(newValue * 100))%")
    }
    
    func adjustMasterBPM(_ delta: Float) {
        let newValue = max(60.0, min(200.0, uiBPM + delta))
        UserDefaults.standard.set(newValue, forKey: "masterBPM")
        print("Master BPM: \(Int(newValue))")
    }
    
    
    // MARK: - Mixer Management
    
    func getInstrumentVolume(_ instrumentIndex: Int) -> Float {
        let key = "volume_\(instrumentIndex)"
        let volume = UserDefaults.standard.float(forKey: key)
        return volume == 0 ? 0.75 : volume // Default to 75% if not set
    }
    
    func isInstrumentMuted(_ instrumentIndex: Int) -> Bool {
        return UserDefaults.standard.bool(forKey: "mute_\(instrumentIndex)")
    }
    
    func isInstrumentSoloed(_ instrumentIndex: Int) -> Bool {
        return UserDefaults.standard.bool(forKey: "solo_\(instrumentIndex)")
    }
    
    func toggleInstrumentMute(_ instrumentIndex: Int) {
        let key = "mute_\(instrumentIndex)"
        let currentState = UserDefaults.standard.bool(forKey: key)
        UserDefaults.standard.set(!currentState, forKey: key)
        print("Toggle mute for instrument \(instrumentIndex): \(!currentState)")
    }
    
    func toggleInstrumentSolo(_ instrumentIndex: Int) {
        print("Toggle solo for instrument \(instrumentIndex)")
        let key = "solo_\(instrumentIndex)"
        let currentState = UserDefaults.standard.bool(forKey: key)
        UserDefaults.standard.set(!currentState, forKey: key)
    }
    
    func setInstrumentVolume(_ instrumentIndex: Int, volume: Float) {
        let key = "volume_\(instrumentIndex)"
        UserDefaults.standard.set(volume, forKey: key)
        print("Set instrument \(instrumentIndex) volume to \(Int(volume * 100))%")
    }
    
    func clearAllSolo() {
        print("Clear all solo")
        for i in 0..<16 {
            UserDefaults.standard.set(false, forKey: "solo_\(i)")
        }
    }
    
    func clearAllMute() {
        print("Clear all mute")
        for i in 0..<16 {
            UserDefaults.standard.set(false, forKey: "mute_\(i)")
        }
    }
    
    func adjustMasterVolume(_ delta: Float) {
        let currentVolume = uiMasterVolume
        let newVolume = max(0.0, min(1.0, currentVolume + delta))
        UserDefaults.standard.set(newVolume, forKey: "masterVolume")
        print("Master volume: \(Int(newVolume * 100))%")
    }
    
    func getMasterLevel() -> Float {
        // Mock master level with some animation
        return Float.random(in: 0.4...0.8)
    }
    
    func selectMixerChannel(_ channelIndex: Int) {
        print("Select mixer channel \(channelIndex + 1)")
        UserDefaults.standard.set(channelIndex, forKey: "selectedMixerChannel")
    }
    
    var selectedMixerChannel: Int? {
        let value = UserDefaults.standard.integer(forKey: "selectedMixerChannel")
        return UserDefaults.standard.object(forKey: "selectedMixerChannel") != nil ? value : nil
    }
    
    // MARK: - Hardware Button Actions
    
    func toggleWrite() {
        uiIsWriteEnabled.toggle()
        print("Write mode: \(uiIsWriteEnabled)")
    }
    
    func toggleShift() {
        uiIsShiftHeld.toggle()
        print("Shift: \(uiIsShiftHeld)")
    }
    
    func triggerClone() {
        print("Trigger clone")
        // TODO: Implement clone action
    }
    
    func triggerCopy() {
        print("Trigger copy")
        // TODO: Implement copy action
    }
    
    func triggerDelete() {
        print("Trigger delete")
        // TODO: Implement delete action
    }
    
    func triggerCut() {
        print("Trigger cut")
        // TODO: Implement cut action
    }
    
    func toggleStamp(_ stamp: StampType) {
        print("Toggle stamp: \(stamp)")
        // TODO: Implement stamp toggle
    }
    
    func triggerNudge() {
        print("Trigger nudge")
        // TODO: Implement nudge action
    }
    
    // MARK: - FX Actions
    
    func setNoteRepeatRate(_ rate: String) {
        print("Set note repeat rate: \(rate)")
        // TODO: Implement note repeat rate
    }
    
    func setNoteRepeatRamp(_ ramp: String) {
        print("Set note repeat ramp: \(ramp)")
        // TODO: Implement note repeat ramp
    }
    
    func toggleFXLatch() {
        print("Toggle FX latch")
        // TODO: Implement FX latch
    }
    
    func bakeFX() {
        print("Bake FX")
        // TODO: Implement FX baking
    }
    
    
    // MARK: - Additional State Properties (moved to main SynthController class)
    
    func getPatternContentLevel(_ patternIndex: Int) -> Float {
        // Return 0.0-1.0 representing how much content is in the pattern  
        return Float.random(in: 0.3...0.8)
    }
    
    func setActiveFX(_ effectName: String) {
        self.activeFX = effectName
        print("Set active FX: \(effectName)")
    }
    
    // MARK: - Hardware Integration System
    
    func processHardwareButtonPress(_ buttonType: HardwareButtonType, isPressed: Bool) {
        guard isPressed else { return } // Only handle button press, not release
        
        switch buttonType {
        case .topButton(let index):
            handleTopHardwareButton(index)
        case .bottomButton(let index):
            handleBottomHardwareButton(index)
        }
    }
    
    private func handleTopHardwareButton(_ index: Int) {
        switch index {
        case 0: play() // PLAY
        case 1: record() // REC  
        case 2: uiIsWriteEnabled.toggle() // WRITE
        case 3: uiIsShiftHeld.toggle() // SHIFT
        case 4: /* INST - handled by main view */ break
        case 5: /* SOUND - handled by main view */ break
        case 6: /* PATT - handled by main view */ break
        case 7: uiIsShiftHeld ? triggerClone() : triggerCopy() // COPY/CLONE
        case 8: uiIsShiftHeld ? triggerDelete() : triggerCut() // CUT/DELETE
        case 9: toggleStamp(.accent) // ACC
        case 10: toggleStamp(.ratchet) // RTCH
        case 11: toggleStamp(.tie) // TIE
        case 12: toggleStamp(.micro) // MICRO
        case 13: /* FX - handled by main view */ break
        case 14: triggerNudge() // NUDGE
        default: break
        }
    }
    
    private func handleBottomHardwareButton(_ index: Int) {
        // Context-dependent - handled by main view based on current screen
        print("Hardware bottom button \(index + 1) pressed")
    }
    
    func getHardwareButtonState(_ buttonType: HardwareButtonType) -> Bool {
        switch buttonType {
        case .topButton(let index):
            return getTopButtonState(index)
        case .bottomButton(let index):
            return getBottomButtonState(index)
        }
    }
    
    private func getTopButtonState(_ index: Int) -> Bool {
        switch index {
        case 0: return isPlaying
        case 1: return isRecording
        case 2: return uiIsWriteEnabled
        case 3: return uiIsShiftHeld
        case 9: return activeStamps.accent
        case 10: return activeStamps.ratchet > 0
        case 11: return activeStamps.tie
        case 12: return activeStamps.micro != 0
        default: return false
        }
    }
    
    private func getBottomButtonState(_ index: Int) -> Bool {
        // Context-dependent based on current screen
        return false
    }
    
    // MARK: - SmartKnob Integration
    
    func handleSmartKnobRotation(_ delta: Float, context: SmartKnobContext) {
        switch context {
        case .parameter(let instrumentIndex, let parameterIndex):
            setInstrumentParameterEnhanced(instrumentIndex, parameterIndex, value: getInstrumentParameterEnhanced(instrumentIndex, parameterIndex) + delta * 0.01)
        case .parameterEdit:
            adjustSelectedParameter(delta)
        case .tempo:
            adjustMasterBPM(delta * 2.0)
        case .swing:
            adjustGlobalSwing(delta * 0.01)
        case .patternLength:
            // TODO: Implement pattern length adjustment
            break
        case .mixerVolume(let channelIndex):
            if let currentVolume = selectedMixerChannel.map({ getInstrumentVolume($0) }) {
                let newVolume = max(0.0, min(1.0, currentVolume + delta * 0.02))
                setInstrumentVolume(selectedMixerChannel!, volume: newVolume)
            }
        case .channelVolume(let channelIndex):
            if let currentVolume = selectedMixerChannel.map({ getInstrumentVolume($0) }) {
                let newVolume = max(0.0, min(1.0, currentVolume + delta * 0.02))
                setInstrumentVolume(selectedMixerChannel!, volume: newVolume)
            }
        case .masterVolume:
            adjustMasterVolume(delta)
        case .globalBPM:
            adjustMasterBPM(delta * 2.0)
        case .globalSwing:
            adjustGlobalSwing(delta * 0.01)
        case .globalQuantize:
            adjustGlobalQuantize(delta * 0.01)
        case .songKey:
            // TODO: Implement song key adjustment
            break
        case .songScale:
            // TODO: Implement song scale adjustment
            break
        case .chainMode:
            // TODO: Implement chain mode adjustment
            break
        case .idle:
            // No action in idle state
            break
        }
    }
    
    // MARK: - Hardware Feedback System
    
    func getHardwareButtonLEDState(_ buttonType: HardwareButtonType) -> HardwareLEDState {
        let isActive = getHardwareButtonState(buttonType)
        
        switch buttonType {
        case .topButton(let index):
            switch index {
            case 0: return isActive ? .solid(.green) : .off // PLAY
            case 1: return isActive ? .solid(.red) : .off // REC
            case 2: return isActive ? .solid(.blue) : .off // WRITE
            case 3: return isActive ? .solid(.orange) : .off // SHIFT
            case 9...12: return isActive ? .solid(.yellow) : .off // STAMPS
            default: return .off
            }
        case .bottomButton(_):
            return isActive ? .solid(.blue) : .dimmed(.blue)
        }
    }
    
    // MARK: - Missing Function Stubs for Compatibility
    
    func startRecording() {
        record()
    }
    
    func stopRecording() {
        record()
    }
}

// MARK: - Supporting Data Types

enum HardwareButtonType {
    case topButton(Int)
    case bottomButton(Int)
}

struct HardwareButtonPress {
    let buttonType: HardwareButtonType
    let isPressed: Bool
}

struct StepData {
    let noteEvents: [NoteEvent]
    let hasMetadata: Bool
    let tie: Bool
}

struct ParameterData {
    let name: String
    let value: Float
    let displayValue: String
}

struct StampState {
    let accent: Bool
    let ratchet: Int
    let tie: Bool
    let micro: Int
}

enum ToolType: String {
    case copy = "copy"
    case cut = "cut"  
    case nudge = "nudge"
    case delete = "delete"
}

enum ToolScope: String {
    case step = "step"
    case instrument = "instrument"
    case pattern = "pattern"
}

enum StampType {
    case accent
    case ratchet
    case tie
    case micro
}

// MARK: - Song Arrangement Data Types

struct SongPatternBlock {
    let patternID: String
    let sectionName: String
    let barIndex: Int
    let isCurrentlyPlaying: Bool
}


// MARK: - Hardware Integration Data Types

enum SmartKnobContext {
    case parameter(instrumentIndex: Int, parameterIndex: Int)
    case parameterEdit
    case tempo
    case swing
    case patternLength
    case mixerVolume(channelIndex: Int)
    case channelVolume(channelIndex: Int)
    case masterVolume
    case globalBPM
    case globalSwing
    case globalQuantize
    case songKey
    case songScale
    case chainMode
    case idle
}

enum HardwareLEDState {
    case off
    case dim
    case bright
    case solid(Color)
    case dimmed(Color)
    case blinking
    case breathing
    
    var brightness: Float {
        switch self {
        case .off: return 0.0
        case .dim: return 0.3
        case .bright: return 1.0
        case .solid(_): return 1.0
        case .dimmed(_): return 0.3
        case .blinking: return 1.0  // Hardware handles blinking pattern
        case .breathing: return 1.0  // Hardware handles breathing pattern
        }
    }
}

struct HardwareButtonState {
    let buttonIndex: Int
    let ledState: HardwareLEDState
    let isPressed: Bool
    let lastPressTime: Date?
}

struct SmartKnobState {
    let context: SmartKnobContext
    let currentValue: Float
    let minValue: Float
    let maxValue: Float
    let displayName: String
    let displayValue: String
    let hasChanged: Bool
}