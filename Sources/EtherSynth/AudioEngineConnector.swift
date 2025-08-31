import Foundation
import Combine

// MARK: - Audio Engine Connector
// Bridge between SwiftUI interface and C++ synthesis engines

class AudioEngineConnector: ObservableObject {
    
    // MARK: - C Bridge Declarations
    private typealias EtherEngine = OpaquePointer
    
    // Core lifecycle
    @_silgen_name("ether_create")
    private func ether_create() -> EtherEngine?
    
    @_silgen_name("ether_destroy")
    private func ether_destroy(_ engine: EtherEngine)
    
    @_silgen_name("ether_initialize")
    private func ether_initialize(_ engine: EtherEngine) -> Int32
    
    @_silgen_name("ether_shutdown")
    private func ether_shutdown(_ engine: EtherEngine)
    
    // Transport
    @_silgen_name("ether_play")
    private func ether_play(_ engine: EtherEngine)
    
    @_silgen_name("ether_stop")
    private func ether_stop(_ engine: EtherEngine)
    
    @_silgen_name("ether_record")
    private func ether_record(_ engine: EtherEngine, _ enable: Int32)
    
    @_silgen_name("ether_is_playing")
    private func ether_is_playing(_ engine: EtherEngine) -> Int32
    
    @_silgen_name("ether_is_recording")
    private func ether_is_recording(_ engine: EtherEngine) -> Int32
    
    // Parameters
    @_silgen_name("ether_set_parameter")
    private func ether_set_parameter(_ engine: EtherEngine, _ paramId: Int32, _ value: Float)
    
    @_silgen_name("ether_get_parameter")
    private func ether_get_parameter(_ engine: EtherEngine, _ paramId: Int32) -> Float
    
    @_silgen_name("ether_set_instrument_parameter")
    private func ether_set_instrument_parameter(_ engine: EtherEngine, _ instrument: Int32, _ paramId: Int32, _ value: Float)
    
    @_silgen_name("ether_get_instrument_parameter")
    private func ether_get_instrument_parameter(_ engine: EtherEngine, _ instrument: Int32, _ paramId: Int32) -> Float
    
    // Note events
    @_silgen_name("ether_note_on")
    private func ether_note_on(_ engine: EtherEngine, _ note: Int32, _ velocity: Float, _ aftertouch: Float)
    
    @_silgen_name("ether_note_off")
    private func ether_note_off(_ engine: EtherEngine, _ note: Int32)
    
    @_silgen_name("ether_all_notes_off")
    private func ether_all_notes_off(_ engine: EtherEngine)
    
    // Timing
    @_silgen_name("ether_set_bpm")
    private func ether_set_bpm(_ engine: EtherEngine, _ bpm: Float)
    
    @_silgen_name("ether_get_bpm")
    private func ether_get_bpm(_ engine: EtherEngine) -> Float
    
    // Performance
    @_silgen_name("ether_get_cpu_usage")
    private func ether_get_cpu_usage(_ engine: EtherEngine) -> Float
    
    @_silgen_name("ether_get_active_voice_count")
    private func ether_get_active_voice_count(_ engine: EtherEngine) -> Int32
    
    @_silgen_name("ether_get_master_volume")
    private func ether_get_master_volume(_ engine: EtherEngine) -> Float
    
    @_silgen_name("ether_set_master_volume")
    private func ether_set_master_volume(_ engine: EtherEngine, _ volume: Float)
    
    // Instrument management
    @_silgen_name("ether_set_active_instrument")
    private func ether_set_active_instrument(_ engine: EtherEngine, _ colorIndex: Int32)
    
    @_silgen_name("ether_get_active_instrument")
    private func ether_get_active_instrument(_ engine: EtherEngine) -> Int32
    
    // MARK: - Properties
    private var engine: EtherEngine?
    private var updateTimer: Timer?
    
    // Published properties for UI binding
    @Published var isInitialized: Bool = false
    @Published var isConnected: Bool = false
    @Published var engineCpuUsage: Float = 0.0
    @Published var engineActiveVoices: Int = 0
    @Published var engineMasterVolume: Float = 1.0
    @Published var engineBPM: Float = 120.0
    @Published var engineIsPlaying: Bool = false
    @Published var engineIsRecording: Bool = false
    @Published var engineActiveInstrument: Int = 0
    
    // Parameter cache for real-time updates
    private var parameterCache: [Int: [Int: Float]] = [:]  // [instrumentIndex: [paramIndex: value]]
    
    // MARK: - Initialization
    
    init() {
        setupEngine()
    }
    
    deinit {
        shutdown()
    }
    
    // MARK: - Engine Management
    
    private func setupEngine() {
        guard let newEngine = ether_create() else {
            print("❌ Failed to create audio engine")
            return
        }
        
        engine = newEngine
        
        let result = ether_initialize(newEngine)
        if result == 1 {
            isInitialized = true
            isConnected = true
            startStatusUpdates()
            print("✅ Audio engine initialized successfully")
        } else {
            print("❌ Failed to initialize audio engine")
            ether_destroy(newEngine)
            engine = nil
        }
    }
    
    func shutdown() {
        stopStatusUpdates()
        
        if let engine = engine {
            ether_shutdown(engine)
            ether_destroy(engine)
            self.engine = nil
        }
        
        isInitialized = false
        isConnected = false
    }
    
    // MARK: - Status Updates
    
    private func startStatusUpdates() {
        updateTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { _ in
            self.updateEngineStatus()
        }
    }
    
    private func stopStatusUpdates() {
        updateTimer?.invalidate()
        updateTimer = nil
    }
    
    private func updateEngineStatus() {
        guard let engine = engine else { return }
        
        DispatchQueue.main.async {
            self.engineCpuUsage = self.ether_get_cpu_usage(engine)
            self.engineActiveVoices = Int(self.ether_get_active_voice_count(engine))
            self.engineMasterVolume = self.ether_get_master_volume(engine)
            self.engineBPM = self.ether_get_bpm(engine)
            self.engineIsPlaying = self.ether_is_playing(engine) == 1
            self.engineIsRecording = self.ether_is_recording(engine) == 1
            self.engineActiveInstrument = Int(self.ether_get_active_instrument(engine))
        }
    }
    
    // MARK: - Transport Control
    
    func play() {
        guard let engine = engine else { return }
        ether_play(engine)
    }
    
    func stop() {
        guard let engine = engine else { return }
        ether_stop(engine)
    }
    
    func toggleRecord() {
        guard let engine = engine else { return }
        let newRecordState = engineIsRecording ? 0 : 1
        ether_record(engine, Int32(newRecordState))
    }
    
    // MARK: - Timing Control
    
    func setBPM(_ bpm: Float) {
        guard let engine = engine else { return }
        ether_set_bpm(engine, bpm)
        engineBPM = bpm
    }
    
    // MARK: - Instrument Management
    
    func setActiveInstrument(_ instrumentIndex: Int) {
        guard let engine = engine else { return }
        ether_set_active_instrument(engine, Int32(instrumentIndex))
        engineActiveInstrument = instrumentIndex
    }
    
    // MARK: - Parameter Management
    
    func setParameter(_ paramId: Int, value: Float) {
        guard let engine = engine else { return }
        ether_set_parameter(engine, Int32(paramId), value)
    }
    
    func getParameter(_ paramId: Int) -> Float {
        guard let engine = engine else { return 0.0 }
        return ether_get_parameter(engine, Int32(paramId))
    }
    
    func setInstrumentParameter(_ instrumentIndex: Int, _ paramIndex: Int, value: Float) {
        guard let engine = engine else { return }
        ether_set_instrument_parameter(engine, Int32(instrumentIndex), Int32(paramIndex), value)
        
        // Update cache
        if parameterCache[instrumentIndex] == nil {
            parameterCache[instrumentIndex] = [:]
        }
        parameterCache[instrumentIndex]![paramIndex] = value
    }
    
    func getInstrumentParameter(_ instrumentIndex: Int, _ paramIndex: Int) -> Float {
        guard let engine = engine else { return 0.0 }
        
        // Check cache first for performance
        if let instrumentParams = parameterCache[instrumentIndex],
           let cachedValue = instrumentParams[paramIndex] {
            return cachedValue
        }
        
        // Fallback to engine query
        let value = ether_get_instrument_parameter(engine, Int32(instrumentIndex), Int32(paramIndex))
        
        // Update cache
        if parameterCache[instrumentIndex] == nil {
            parameterCache[instrumentIndex] = [:]
        }
        parameterCache[instrumentIndex]![paramIndex] = value
        
        return value
    }
    
    // MARK: - Note Events
    
    func noteOn(_ note: Int, velocity: Float = 1.0, aftertouch: Float = 0.0) {
        guard let engine = engine else { return }
        ether_note_on(engine, Int32(note), velocity, aftertouch)
    }
    
    func noteOff(_ note: Int) {
        guard let engine = engine else { return }
        ether_note_off(engine, Int32(note))
    }
    
    func allNotesOff() {
        guard let engine = engine else { return }
        ether_all_notes_off(engine)
    }
    
    // MARK: - Volume Control
    
    func setMasterVolume(_ volume: Float) {
        guard let engine = engine else { return }
        ether_set_master_volume(engine, volume)
        engineMasterVolume = volume
    }
    
    // MARK: - Parameter ID Constants (matching C bridge)
    
    enum ParameterID: Int {
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
    
    // MARK: - Convenience Methods
    
    func setInstrumentVolume(_ instrumentIndex: Int, _ volume: Float) {
        setInstrumentParameter(instrumentIndex, ParameterID.volume.rawValue, value: volume)
    }
    
    func getInstrumentVolume(_ instrumentIndex: Int) -> Float {
        return getInstrumentParameter(instrumentIndex, ParameterID.volume.rawValue)
    }
    
    func setInstrumentFilterCutoff(_ instrumentIndex: Int, _ cutoff: Float) {
        setInstrumentParameter(instrumentIndex, ParameterID.filterCutoff.rawValue, value: cutoff)
    }
    
    func getInstrumentFilterCutoff(_ instrumentIndex: Int) -> Float {
        return getInstrumentParameter(instrumentIndex, ParameterID.filterCutoff.rawValue)
    }
}

// MARK: - Audio Engine Status
struct AudioEngineStatus {
    let isConnected: Bool
    let cpuUsage: Float
    let activeVoices: Int
    let masterVolume: Float
    let bpm: Float
    let isPlaying: Bool
    let isRecording: Bool
    let activeInstrument: Int
}