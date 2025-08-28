import SwiftUI
import Combine
import AVFoundation

// C++ Bridge Interface
@_silgen_name("ether_create")
func ether_create() -> UnsafeMutableRawPointer?

@_silgen_name("ether_destroy")
func ether_destroy(_ synth: UnsafeMutableRawPointer)

@_silgen_name("ether_initialize")
func ether_initialize(_ synth: UnsafeMutableRawPointer) -> Int32

@_silgen_name("ether_shutdown")
func ether_shutdown(_ synth: UnsafeMutableRawPointer)

@_silgen_name("ether_note_on")
func ether_note_on(_ synth: UnsafeMutableRawPointer, _ key: Int32, _ velocity: Float, _ aftertouch: Float)

@_silgen_name("ether_note_off")
func ether_note_off(_ synth: UnsafeMutableRawPointer, _ key: Int32)

@_silgen_name("ether_play")
func ether_play(_ synth: UnsafeMutableRawPointer)

@_silgen_name("ether_stop")
func ether_stop(_ synth: UnsafeMutableRawPointer)

@_silgen_name("ether_record")
func ether_record(_ synth: UnsafeMutableRawPointer, _ enable: Int32)

@_silgen_name("ether_is_playing")
func ether_is_playing(_ synth: UnsafeMutableRawPointer) -> Int32

@_silgen_name("ether_is_recording")
func ether_is_recording(_ synth: UnsafeMutableRawPointer) -> Int32

@_silgen_name("ether_set_active_instrument")
func ether_set_active_instrument(_ synth: UnsafeMutableRawPointer, _ instrument: Int32)

@_silgen_name("ether_get_active_instrument")
func ether_get_active_instrument(_ synth: UnsafeMutableRawPointer) -> Int32

@_silgen_name("ether_set_parameter")
func ether_set_parameter(_ synth: UnsafeMutableRawPointer, _ param: Int32, _ value: Float)

@_silgen_name("ether_get_parameter")
func ether_get_parameter(_ synth: UnsafeMutableRawPointer, _ param: Int32) -> Float

@_silgen_name("ether_set_bpm")
func ether_set_bpm(_ synth: UnsafeMutableRawPointer, _ bpm: Float)

@_silgen_name("ether_get_bpm")
func ether_get_bpm(_ synth: UnsafeMutableRawPointer) -> Float

@_silgen_name("ether_get_cpu_usage")
func ether_get_cpu_usage(_ synth: UnsafeMutableRawPointer) -> Float

@_silgen_name("ether_get_active_voice_count")
func ether_get_active_voice_count(_ synth: UnsafeMutableRawPointer) -> Int32

@_silgen_name("ether_set_smart_knob")
func ether_set_smart_knob(_ synth: UnsafeMutableRawPointer, _ value: Float)

@_silgen_name("ether_set_touch_position")
func ether_set_touch_position(_ synth: UnsafeMutableRawPointer, _ x: Float, _ y: Float)

// NEW: Engine type management functions
@_silgen_name("ether_get_instrument_engine_type")
func ether_get_instrument_engine_type(_ synth: UnsafeMutableRawPointer, _ instrument: Int32) -> Int32

@_silgen_name("ether_set_instrument_engine_type")
func ether_set_instrument_engine_type(_ synth: UnsafeMutableRawPointer, _ instrument: Int32, _ engine_type: Int32)

@_silgen_name("ether_get_engine_type_name")
func ether_get_engine_type_name(_ engine_type: Int32) -> UnsafePointer<CChar>?

@_silgen_name("ether_get_instrument_color_name")
func ether_get_instrument_color_name(_ color_index: Int32) -> UnsafePointer<CChar>?

@_silgen_name("ether_get_engine_type_count")
func ether_get_engine_type_count() -> Int32

@_silgen_name("ether_get_instrument_color_count")
func ether_get_instrument_color_count() -> Int32

// Parameter IDs matching C++
enum EtherParameterID: Int32 {
    case volume = 0
    case attack = 1
    case decay = 2
    case sustain = 3
    case release = 4
    case filterCutoff = 5
    case filterResonance = 6
    case osc1Freq = 7
    case osc2Freq = 8
    case oscMix = 9
    case lfoRate = 10
    case lfoDepth = 11
}

// MARK: - Synth Controller
class SynthController: ObservableObject {
    // MARK: - Published Properties
    @Published var activeInstrument: InstrumentColor = .coral
    @Published var isPlaying: Bool = false
    @Published var isRecording: Bool = false
    @Published var bpm: Double = 120.0
    @Published var smartKnobValue: Double = 0.5
    @Published var activeVoices: Int = 0
    @Published var cpuUsage: Double = 0.0
    
    // New UI Properties
    @Published var currentView: String = "PATTERN"  // PATTERN or SONG
    @Published var selectedLFO: Int = 1
    @Published var showingInstrumentBrowser: Bool = false
    @Published var selectedInstrumentSlot: InstrumentColor? = nil
    @Published var currentPatternID: String = "A"
    @Published var scrollPosition: CGFloat = 0.0
    @Published var instrumentScrollPositions: [InstrumentColor: CGFloat] = [:]
    
    // LFO Configuration Properties
    @Published var showingLFOConfig: Bool = false
    @Published var lfoScrollPositions: [Int: CGFloat] = [:]
    @Published var lfoTypes: [Int: String] = [:] // LFO index to type
    @Published var lfoRates: [Int: Double] = [:] // LFO index to rate
    @Published var lfoDepths: [Int: Double] = [:] // LFO index to depth
    @Published var lfoPhases: [Int: Double] = [:] // LFO index to phase
    @Published var lfoAssignments: [Int: [(InstrumentColor, String)]] = [:] // LFO to instrument/param assignments
    
    // Instrument slot states (empty by default)
    @Published var instrumentNames: [InstrumentColor: String] = [
        .coral: "Empty slot",
        .peach: "Empty slot", 
        .cream: "Empty slot",
        .sage: "Empty slot",
        .teal: "Empty slot",
        .slate: "Empty slot",
        .pearl: "Empty slot",
        .stone: "Empty slot"
    ]
    
    // Instrument configuration state
    @Published var selectedFamilies: [InstrumentColor: String] = [:]
    @Published var selectedEngines: [InstrumentColor: String] = [:]
    
    // UI Layout Toggle for A/B Testing
    @Published var useFocusContextLayout: Bool = true  // true = Focus+Context, false = Original
    
    // MARK: - Parameter Properties
    @Published var volume: Double = 0.8
    @Published var attack: Double = 0.1
    @Published var decay: Double = 0.3
    @Published var sustain: Double = 0.7
    @Published var release: Double = 0.5
    @Published var filterCutoff: Double = 0.8
    
    // MARK: - Waveform Data
    @Published var waveformData: [Float] = Array(repeating: 0.0, count: 64)
    
    // MARK: - Private Properties
    private var audioEngine: AVAudioEngine
    private var mixer: AVAudioMixerNode
    private var playerNode: AVAudioPlayerNode
    private var timer: Timer?
    
    // C++ Bridge - Real engine integration
    private var cppSynthEngine: UnsafeMutableRawPointer?
    
    // Touch position for NSynth-style morphing
    private var touchPosition: CGPoint = CGPoint(x: 0.5, y: 0.5)
    
    // MARK: - Initialization
    init() {
        self.audioEngine = AVAudioEngine()
        self.mixer = AVAudioMixerNode()
        self.playerNode = AVAudioPlayerNode()
        
        setupAudioEngine()
        generateMockWaveform()
        
        // Initialize C++ engine immediately
        print("🚀🚀🚀 TEST MESSAGE FROM SWIFT CONSTRUCTOR 🚀🚀🚀")
        initializeCppEngine()
    }
    
    deinit {
        shutdown()
    }
    
    // MARK: - Audio Engine Setup
    private func setupAudioEngine() {
        audioEngine.attach(mixer)
        audioEngine.attach(playerNode)
        
        audioEngine.connect(playerNode, to: mixer, format: nil)
        audioEngine.connect(mixer, to: audioEngine.outputNode, format: nil)
        
        do {
            try audioEngine.start()
            print("SwiftUI Audio Engine started successfully")
        } catch {
            print("Failed to start audio engine: \(error)")
        }
    }
    
    private func initializeCppEngine() {
        // Initialize real C++ audio engine
        cppSynthEngine = ether_create()
        if let engine = cppSynthEngine {
            let result = ether_initialize(engine)
            print("🔍 ether_initialize returned: \(result)")
            if result != 0 {
                print("✅ C++ Engine initialized successfully")
                
                // Update instrument names with real engine names
                updateInstrumentNamesFromEngine()
            } else {
                print("❌ Failed to initialize C++ Engine (result = \(result))")
            }
        } else {
            print("❌ Failed to create C++ Engine")
        }
    }
    
    // MARK: - Public Interface
    func initialize() {
        print("🚀 SynthController.initialize() called!")
        startPerformanceMonitoring()
    }
    
    private func updateInstrumentNamesFromEngine() {
        print("🔄 Updating instrument names from C++ engine...")
        for color in InstrumentColor.allCases {
            if let engine = cppSynthEngine {
                let engineType = ether_get_instrument_engine_type(engine, Int32(color.index))
                if let cString = ether_get_engine_type_name(engineType) {
                    let engineName = String(cString: cString)
                    instrumentNames[color] = engineName
                    print("🔄 Updated \(color.rawValue) -> \(engineName)")
                }
            }
        }
    }
    
    func shutdown() {
        timer?.invalidate()
        audioEngine.stop()
        
        // Cleanup C++ engine
        if let engine = cppSynthEngine {
            ether_shutdown(engine)
            ether_destroy(engine)
            cppSynthEngine = nil
            print("✅ C++ Engine shut down")
        }
    }
    
    // MARK: - Instrument Management
    func selectInstrument(_ color: InstrumentColor) {
        // Save current scroll position for the previous instrument
        if activeInstrument != color {
            instrumentScrollPositions[activeInstrument] = scrollPosition
        }
        
        activeInstrument = color
        
        // Restore scroll position for the new instrument
        scrollPosition = instrumentScrollPositions[color] ?? 0.0
        
        // Close LFO config when selecting instrument (instrument takes priority)
        showingLFOConfig = false
        
        // Make sure we're in Pattern view to see instrument config
        currentView = "PATTERN"
        
        // Send to C++ engine
        if let engine = cppSynthEngine {
            ether_set_active_instrument(engine, Int32(color.index))
        }
        
        print("Selected instrument: \(color.rawValue)")
    }
    
    func switchPattern(_ patternID: String) {
        currentPatternID = patternID
        print("Switched to pattern \(patternID)")
    }
    
    func updateScrollPosition(_ position: CGFloat) {
        scrollPosition = position
        instrumentScrollPositions[activeInstrument] = position
    }
    
    // MARK: - UI Functions
    func switchView(_ view: String) {
        currentView = view
        print("Switched to \(view) view")
    }
    
    func selectLFO(_ lfoIndex: Int) {
        // Save current scroll position for the previous LFO
        if selectedLFO != lfoIndex {
            lfoScrollPositions[selectedLFO] = scrollPosition
        }
        
        selectedLFO = lfoIndex
        showingLFOConfig = true
        
        // Restore scroll position for the new LFO
        scrollPosition = lfoScrollPositions[lfoIndex] ?? 0.0
        
        print("Selected LFO \(lfoIndex) for configuration")
        
        // TODO: Send to C++ engine for detailed LFO editing
        // cppSelectLFO(cppSynthEngine, lfoIndex)
    }
    
    func getInstrumentName(for color: InstrumentColor) -> String {
        // Return the cached instrument name (updated from C++ engine during initialization)
        return instrumentNames[color] ?? "Empty slot"
    }
    
    func showInstrumentBrowser(for color: InstrumentColor) {
        selectedInstrumentSlot = color
        showingInstrumentBrowser = true
        print("Opening instrument browser for \(color.rawValue) slot")
    }
    
    func assignInstrument(_ instrumentName: String, to color: InstrumentColor) {
        instrumentNames[color] = instrumentName
        showingInstrumentBrowser = false
        selectedInstrumentSlot = nil
        print("Assigned \(instrumentName) to \(color.rawValue) slot")
    }
    
    func closeInstrumentBrowser() {
        showingInstrumentBrowser = false
        selectedInstrumentSlot = nil
    }
    
    func closeLFOConfig() {
        showingLFOConfig = false
        // Save scroll position when closing
        lfoScrollPositions[selectedLFO] = scrollPosition
    }
    
    func setLFOType(_ lfoIndex: Int, type: String) {
        lfoTypes[lfoIndex] = type
        print("Set LFO \(lfoIndex) type to \(type)")
    }
    
    func setLFORate(_ lfoIndex: Int, rate: Double) {
        lfoRates[lfoIndex] = rate
        print("Set LFO \(lfoIndex) rate to \(rate)")
    }
    
    func setLFODepth(_ lfoIndex: Int, depth: Double) {
        lfoDepths[lfoIndex] = depth
        print("Set LFO \(lfoIndex) depth to \(depth)")
    }
    
    func setLFOPhase(_ lfoIndex: Int, phase: Double) {
        lfoPhases[lfoIndex] = phase
        print("Set LFO \(lfoIndex) phase to \(phase)")
    }
    
    func getLFOType(for lfoIndex: Int) -> String {
        return lfoTypes[lfoIndex] ?? "Sine"
    }
    
    func getLFORate(for lfoIndex: Int) -> Double {
        return lfoRates[lfoIndex] ?? 1.0
    }
    
    func getLFODepth(for lfoIndex: Int) -> Double {
        return lfoDepths[lfoIndex] ?? 0.5
    }
    
    func getLFOPhase(for lfoIndex: Int) -> Double {
        return lfoPhases[lfoIndex] ?? 0.0
    }
    
    // MARK: - Instrument Configuration Functions
    func selectFamily(for instrument: InstrumentColor, family: String) {
        selectedFamilies[instrument] = family
        // Clear engine selection when family changes
        selectedEngines[instrument] = nil
        // Update instrument name when family is selected
        instrumentNames[instrument] = family
        print("Selected family \(family) for \(instrument.rawValue)")
    }
    
    func selectEngine(for instrument: InstrumentColor, engine: String) {
        selectedEngines[instrument] = engine
        // Update instrument name to show the engine
        instrumentNames[instrument] = engine
        print("Selected engine \(engine) for \(instrument.rawValue)")
    }
    
    func getSelectedFamily(for instrument: InstrumentColor) -> String? {
        return selectedFamilies[instrument]
    }
    
    func getSelectedEngine(for instrument: InstrumentColor) -> String? {
        return selectedEngines[instrument]
    }
    
    // MARK: - Real Engine Type Management
    func getAvailableEngineTypes() -> [String] {
        var engineTypes: [String] = []
        let count = Int(ether_get_engine_type_count())
        
        for i in 0..<count {
            if let cString = ether_get_engine_type_name(Int32(i)) {
                let engineName = String(cString: cString)
                engineTypes.append(engineName)
            }
        }
        
        return engineTypes
    }
    
    func setEngineType(for instrument: InstrumentColor, engineType: Int) {
        if let engine = cppSynthEngine {
            ether_set_instrument_engine_type(engine, Int32(instrument.index), Int32(engineType))
            print("Set \(instrument.rawValue) engine to type \(engineType)")
        }
    }
    
    func getCurrentEngineType(for instrument: InstrumentColor) -> Int {
        if let engine = cppSynthEngine {
            return Int(ether_get_instrument_engine_type(engine, Int32(instrument.index)))
        }
        return 0
    }
    
    
    // MARK: - Note Events
    func noteOn(_ keyIndex: Int) {
        let midiNote = 36 + keyIndex // Start from C2
        print("Note ON: \(midiNote)")
        
        // Send to C++ engine
        if let engine = cppSynthEngine {
            ether_note_on(engine, Int32(keyIndex), 0.8, 0.0)
        }
        
        // Mock voice count update
        DispatchQueue.main.async {
            self.activeVoices = min(8, self.activeVoices + 1)
        }
        
        generateMockWaveform()
    }
    
    func noteOff(_ keyIndex: Int) {
        let midiNote = 36 + keyIndex
        print("Note OFF: \(midiNote)")
        
        // Send to C++ engine
        if let engine = cppSynthEngine {
            ether_note_off(engine, Int32(keyIndex))
        }
        
        // Mock voice count update
        DispatchQueue.main.async {
            self.activeVoices = max(0, self.activeVoices - 1)
        }
    }
    
    func triggerNote(_ keyIndex: Int) {
        noteOn(keyIndex)
        
        // Auto-release after short duration
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
            self.noteOff(keyIndex)
        }
    }
    
    // MARK: - Transport Controls
    func play() {
        isPlaying = true
        print("Transport: PLAY")
        
        // Send to C++ engine
        if let engine = cppSynthEngine {
            ether_play(engine)
        }
    }
    
    func stop() {
        isPlaying = false
        print("Transport: STOP")
        
        // Send to C++ engine
        if let engine = cppSynthEngine {
            ether_stop(engine)
        }
        
        activeVoices = 0
    }
    
    func record() {
        isRecording.toggle()
        print("Transport: RECORD \(isRecording ? "ON" : "OFF")")
        
        // Send to C++ engine
        if let engine = cppSynthEngine {
            ether_record(engine, isRecording ? 1 : 0)
        }
    }
    
    // MARK: - Parameter Control
    func setSmartKnobValue(_ value: Double) {
        smartKnobValue = value
        print("Smart Knob: \(value)")
        
        // Send to C++ engine
        if let engine = cppSynthEngine {
            ether_set_smart_knob(engine, Float(value))
        }
    }
    
    func setTouchPosition(_ position: CGPoint) {
        touchPosition = position
        print("Touch: (\(position.x), \(position.y))")
        
        // Map touch position to synthesis parameters
        let morphX = Float(position.x)
        let morphY = Float(position.y)
        
        // Send to C++ engine
        if let engine = cppSynthEngine {
            ether_set_touch_position(engine, Float(position.x), Float(position.y))
        }
    }
    
    // MARK: - ADSR Parameters
    func setVolume(_ value: Double) {
        volume = value
        if let engine = cppSynthEngine {
            ether_set_parameter(engine, EtherParameterID.volume.rawValue, Float(value))
        }
    }
    
    func setAttack(_ value: Double) {
        attack = value
        if let engine = cppSynthEngine {
            ether_set_parameter(engine, EtherParameterID.attack.rawValue, Float(value))
        }
    }
    
    func setDecay(_ value: Double) {
        decay = value
        if let engine = cppSynthEngine {
            ether_set_parameter(engine, EtherParameterID.decay.rawValue, Float(value))
        }
    }
    
    func setSustain(_ value: Double) {
        sustain = value
        if let engine = cppSynthEngine {
            ether_set_parameter(engine, EtherParameterID.sustain.rawValue, Float(value))
        }
    }
    
    func setRelease(_ value: Double) {
        release = value
        if let engine = cppSynthEngine {
            ether_set_parameter(engine, EtherParameterID.release.rawValue, Float(value))
        }
    }
    
    func setFilterCutoff(_ value: Double) {
        filterCutoff = value
        if let engine = cppSynthEngine {
            ether_set_parameter(engine, EtherParameterID.filterCutoff.rawValue, Float(value))
        }
    }
    
    func setBPM(_ newBPM: Double) {
        bpm = newBPM
        if let engine = cppSynthEngine {
            ether_set_bpm(engine, Float(newBPM))
        }
        print("BPM set to: \(newBPM)")
    }
    
    // MARK: - Performance Monitoring
    private func startPerformanceMonitoring() {
        timer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { _ in
            self.updatePerformanceMetrics()
        }
    }
    
    private func updatePerformanceMetrics() {
        // Mock CPU usage based on voice count
        let baseCPU = Double(activeVoices) * 5.0
        let variance = Double.random(in: -2.0...2.0)
        cpuUsage = max(0, min(100, baseCPU + variance))
        
        // Get real metrics from C++ engine
        if let engine = cppSynthEngine {
            cpuUsage = Double(ether_get_cpu_usage(engine))
            activeVoices = Int(ether_get_active_voice_count(engine))
            
            // Get real BPM from engine
            let realBPM = Double(ether_get_bpm(engine))
            if abs(realBPM - bpm) > 0.1 {
                bpm = realBPM
            }
        }
        
        // Update waveform periodically
        if activeVoices > 0 {
            generateMockWaveform()
        }
    }
    
    // MARK: - Waveform Generation (Mock)
    private func generateMockWaveform() {
        let frequency = 440.0 * pow(2.0, (Double(activeVoices) - 1) / 12.0)
        let sampleRate = 44100.0
        let samplesPerWave = Int(sampleRate / frequency)
        
        var newWaveform: [Float] = []
        
        for i in 0..<64 {
            let phase = Double(i) / Double(samplesPerWave) * 2.0 * Double.pi
            let amplitude = Float(sin(phase) * 0.8 * volume)
            newWaveform.append(amplitude)
        }
        
        DispatchQueue.main.async {
            self.waveformData = newWaveform
        }
    }
}

// MARK: - C++ Bridge Functions (Placeholder)
// These would be implemented in a separate .mm file for actual C++ integration

/*
private func createCppSynthEngine() -> UnsafeMutableRawPointer? {
    // TODO: Create and return C++ EtherSynth instance
    return nil
}

private func destroyCppSynthEngine(_ engine: UnsafeMutableRawPointer) {
    // TODO: Cleanup C++ instance
}

private func setCppActiveInstrument(_ engine: UnsafeMutableRawPointer?, _ colorIndex: Int) {
    // TODO: Call C++ setActiveInstrument
}

private func cppNoteOn(_ engine: UnsafeMutableRawPointer?, _ keyIndex: UInt8, _ velocity: Float, _ aftertouch: Float) {
    // TODO: Call C++ noteOn
}

private func cppNoteOff(_ engine: UnsafeMutableRawPointer?, _ keyIndex: UInt8) {
    // TODO: Call C++ noteOff
}

private func cppPlay(_ engine: UnsafeMutableRawPointer?) {
    // TODO: Call C++ play
}

private func cppStop(_ engine: UnsafeMutableRawPointer?) {
    // TODO: Call C++ stop
}

private func cppRecord(_ engine: UnsafeMutableRawPointer?, _ recording: Bool) {
    // TODO: Call C++ record
}

private func cppSetSmartKnob(_ engine: UnsafeMutableRawPointer?, _ value: Float) {
    // TODO: Call C++ setSmartKnobValue
}

private func cppSetMorphPosition(_ engine: UnsafeMutableRawPointer?, _ x: Float, _ y: Float) {
    // TODO: Call C++ setTouchPosition or morphing function
}

private func cppSetParameter(_ engine: UnsafeMutableRawPointer?, _ param: Int, _ value: Float) {
    // TODO: Call C++ setParameter
}

private func cppGetCPUUsage(_ engine: UnsafeMutableRawPointer?) -> Float {
    // TODO: Call C++ getCPUUsage
    return 0.0
}

private func cppGetActiveVoiceCount(_ engine: UnsafeMutableRawPointer?) -> Int {
    // TODO: Call C++ getActiveVoiceCount
    return 0
}
*/