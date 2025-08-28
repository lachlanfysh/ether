import SwiftUI
import Combine

/**
 * Synth Controller View Model - MVVM architecture for main application state
 * Coordinates between UI, audio engine, and data persistence
 */

@MainActor
class SynthControllerViewModel: ObservableObject {
    @Published var currentBPM: Double = 120.0
    @Published var isPlaying: Bool = false
    @Published var isPaused: Bool = false
    @Published var currentPosition: PlaybackPosition = PlaybackPosition(bar: 0, beat: 0, tick: 0)
    @Published var masterVolume: Float = 0.8
    @Published var masterPan: Float = 0.0
    @Published var cpuUsage: Float = 0.0
    @Published var memoryUsage: Float = 0.0
    @Published var audioLatency: Float = 0.0
    @Published var sampleRate: Float = 44100.0
    @Published var bufferSize: Int = 256
    @Published var dropouts: Int = 0
    
    // Global states
    @Published var isRecording: Bool = false
    @Published var isLooping: Bool = false
    @Published var clickEnabled: Bool = false
    @Published var clickVolume: Float = 0.5
    @Published var preCount: Int = 0
    @Published var swing: Float = 0.0
    @Published var shuffle: Float = 0.0
    
    // MIDI state
    @Published var midiInputs: [MIDIDevice] = []
    @Published var midiOutputs: [MIDIDevice] = []
    @Published var selectedMidiInput: String? = nil
    @Published var selectedMidiOutput: String? = nil
    @Published var midiThru: Bool = false
    @Published var midiClock: Bool = false
    
    // Audio device state
    @Published var audioInputs: [AudioDevice] = []
    @Published var audioOutputs: [AudioDevice] = []
    @Published var selectedAudioInput: String? = nil
    @Published var selectedAudioOutput: String? = nil
    
    // Project state
    @Published var currentProject: ProjectInfo? = nil
    @Published var hasUnsavedChanges: Bool = false
    @Published var isAutoSaveEnabled: Bool = true
    @Published var lastSaved: Date? = nil
    
    // UI state
    @Published var selectedView: MainView = .timeline
    @Published var isFullscreen: Bool = false
    @Published var showingPreferences: Bool = false
    @Published var showingAbout: Bool = false
    
    private var synthController: SynthController
    private var performanceTimer: Timer?
    private var autoSaveTimer: Timer?
    private var cancellables = Set<AnyCancellable>()
    
    enum MainView: String, CaseIterable {
        case timeline = "Timeline"
        case mix = "Mix"
        case mod = "Mod"
        case browser = "Browser"
    }
    
    init() {
        self.synthController = SynthController()
        
        setupBindings()
        startPerformanceMonitoring()
        setupAutoSave()
        refreshDevices()
        loadUserSettings()
    }
    
    deinit {
        // Note: Can't call @MainActor methods from deinit
        // These will be cleaned up automatically
        performanceTimer?.invalidate()
        autoSaveTimer?.invalidate()
    }
    
    // MARK: - Setup and Configuration
    
    private func setupBindings() {
        // Observe changes to mark as dirty
        Publishers.CombineLatest4(
            $currentBPM,
            $masterVolume,
            $masterPan,
            $swing
        )
        .dropFirst() // Skip initial values
        .sink { [weak self] _ in
            self?.markAsChanged()
        }
        .store(in: &cancellables)
        
        // Forward BPM changes to synth controller
        $currentBPM
            .debounce(for: 0.1, scheduler: RunLoop.main)
            .sink { [weak self] bpm in
                self?.synthController.setBPM(Float(bpm))
            }
            .store(in: &cancellables)
        
        // Forward master volume changes
        $masterVolume
            .debounce(for: 0.05, scheduler: RunLoop.main)
            .sink { [weak self] volume in
                self?.synthController.setMasterVolume(volume)
            }
            .store(in: &cancellables)
        
        // Forward master pan changes
        $masterPan
            .debounce(for: 0.05, scheduler: RunLoop.main)
            .sink { [weak self] pan in
                self?.synthController.setMasterPan(pan)
            }
            .store(in: &cancellables)
    }
    
    private func startPerformanceMonitoring() {
        performanceTimer = Timer.scheduledTimer(withTimeInterval: 0.5, repeats: true) { [weak self] _ in
            self?.updatePerformanceMetrics()
        }
    }
    
    private func stopPerformanceMonitoring() {
        performanceTimer?.invalidate()
        performanceTimer = nil
    }
    
    private func setupAutoSave() {
        guard isAutoSaveEnabled else { return }
        
        autoSaveTimer = Timer.scheduledTimer(withTimeInterval: 30.0, repeats: true) { [weak self] _ in
            self?.autoSave()
        }
    }
    
    private func stopAutoSave() {
        autoSaveTimer?.invalidate()
        autoSaveTimer = nil
    }
    
    // MARK: - Playback Control
    
    func startPlayback() {
        guard !isPlaying else { return }
        
        let success = synthController.startPlayback()
        if success {
            isPlaying = true
            isPaused = false
        }
    }
    
    func stopPlayback() {
        guard isPlaying || isPaused else { return }
        
        synthController.stopPlayback()
        isPlaying = false
        isPaused = false
        currentPosition = PlaybackPosition(bar: 0, beat: 0, tick: 0)
    }
    
    func pausePlayback() {
        guard isPlaying else { return }
        
        synthController.pausePlayback()
        isPlaying = false
        isPaused = true
    }
    
    func resumePlayback() {
        guard isPaused else { return }
        
        let success = synthController.resumePlayback()
        if success {
            isPlaying = true
            isPaused = false
        }
    }
    
    func seekToPosition(bar: Int, beat: Int = 0, tick: Int = 0) {
        let position = PlaybackPosition(bar: bar, beat: beat, tick: tick)
        synthController.seekToPosition(position: position)
        currentPosition = position
    }
    
    func setBPM(_ bpm: Double) {
        currentBPM = max(60.0, min(200.0, bpm))
    }
    
    func tapTempo() {
        let newBPM = synthController.tapTempo()
        if newBPM > 0 {
            currentBPM = Double(newBPM)
        }
    }
    
    // MARK: - Audio Control
    
    func setMasterVolume(_ volume: Float) {
        masterVolume = max(0.0, min(1.0, volume))
    }
    
    func setMasterPan(_ pan: Float) {
        masterPan = max(-1.0, min(1.0, pan))
    }
    
    func toggleClick() {
        clickEnabled.toggle()
        synthController.setClickEnabled(clickEnabled)
    }
    
    func setClickVolume(_ volume: Float) {
        clickVolume = max(0.0, min(1.0, volume))
        synthController.setClickVolume(clickVolume)
    }
    
    func setSwing(_ swing: Float) {
        self.swing = max(-0.5, min(0.5, swing))
        synthController.setSwing(self.swing)
    }
    
    func setShuffle(_ shuffle: Float) {
        self.shuffle = max(0.0, min(1.0, shuffle))
        synthController.setShuffle(self.shuffle)
    }
    
    // MARK: - Recording
    
    func startRecording() {
        guard !isRecording else { return }
        
        let success = synthController.startRecording()
        if success {
            isRecording = true
            if !isPlaying {
                startPlayback()
            }
        }
    }
    
    func stopRecording() {
        guard isRecording else { return }
        
        synthController.stopRecording()
        isRecording = false
    }
    
    func armTrackForRecording(trackId: Int) {
        synthController.armTrackForRecording(track: trackId)
        markAsChanged()
    }
    
    func disarmTrackForRecording(trackId: Int) {
        synthController.disarmTrackForRecording(track: trackId)
        markAsChanged()
    }
    
    // MARK: - Device Management
    
    func refreshDevices() {
        refreshAudioDevices()
        refreshMidiDevices()
    }
    
    private func refreshAudioDevices() {
        audioInputs = synthController.getAudioInputDevices()
        audioOutputs = synthController.getAudioOutputDevices()
        
        // Set defaults if not already set
        if selectedAudioInput == nil && !audioInputs.isEmpty {
            selectedAudioInput = audioInputs.first?.id
        }
        
        if selectedAudioOutput == nil && !audioOutputs.isEmpty {
            selectedAudioOutput = audioOutputs.first?.id
        }
    }
    
    private func refreshMidiDevices() {
        midiInputs = synthController.getMidiInputDevices()
        midiOutputs = synthController.getMidiOutputDevices()
    }
    
    func setAudioInput(_ deviceId: String) {
        selectedAudioInput = deviceId
        synthController.setAudioInputDevice(deviceId: deviceId)
        saveUserSettings()
    }
    
    func setAudioOutput(_ deviceId: String) {
        selectedAudioOutput = deviceId
        synthController.setAudioOutputDevice(deviceId: deviceId)
        saveUserSettings()
    }
    
    func setMidiInput(_ deviceId: String) {
        selectedMidiInput = deviceId
        synthController.setMidiInputDevice(deviceId: deviceId)
        saveUserSettings()
    }
    
    func setMidiOutput(_ deviceId: String) {
        selectedMidiOutput = deviceId
        synthController.setMidiOutputDevice(deviceId: deviceId)
        saveUserSettings()
    }
    
    func setSampleRate(_ sampleRate: Float) {
        self.sampleRate = sampleRate
        synthController.setSampleRate(sampleRate)
        markAsChanged()
    }
    
    func setBufferSize(_ bufferSize: Int) {
        self.bufferSize = bufferSize
        synthController.setBufferSize(bufferSize)
        markAsChanged()
    }
    
    // MARK: - Project Management
    
    func newProject() {
        if hasUnsavedChanges {
            // In a real app, show save dialog here
        }
        
        synthController.newProject()
        currentProject = nil
        hasUnsavedChanges = false
        lastSaved = nil
    }
    
    func loadProject(url: URL) -> Bool {
        guard synthController.loadProject(url: url) else {
            return false
        }
        
        currentProject = ProjectInfo(
            name: url.deletingPathExtension().lastPathComponent,
            url: url,
            lastModified: Date()
        )
        hasUnsavedChanges = false
        lastSaved = Date()
        
        return true
    }
    
    func saveProject() -> Bool {
        guard let projectURL = currentProject?.url else {
            return saveProjectAs()
        }
        
        return saveProject(to: projectURL)
    }
    
    func saveProjectAs() -> Bool {
        // In a real app, show save panel here
        let documentsURL = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!
        let projectURL = documentsURL.appendingPathComponent("Untitled.ether")
        
        return saveProject(to: projectURL)
    }
    
    private func saveProject(to url: URL) -> Bool {
        guard synthController.saveProject(url: url) else {
            return false
        }
        
        currentProject = ProjectInfo(
            name: url.deletingPathExtension().lastPathComponent,
            url: url,
            lastModified: Date()
        )
        hasUnsavedChanges = false
        lastSaved = Date()
        
        return true
    }
    
    private func autoSave() {
        guard hasUnsavedChanges, isAutoSaveEnabled else { return }
        
        // Create autosave file
        if let projectURL = currentProject?.url {
            let autosaveURL = projectURL.appendingPathExtension("autosave")
            _ = synthController.saveProject(url: autosaveURL)
        }
    }
    
    private func markAsChanged() {
        hasUnsavedChanges = true
    }
    
    // MARK: - Performance Monitoring
    
    private func updatePerformanceMetrics() {
        let metrics = synthController.getPerformanceMetrics()
        
        cpuUsage = metrics.cpuUsage
        memoryUsage = metrics.memoryUsage
        audioLatency = metrics.audioLatency
        dropouts = metrics.dropouts
        
        // Update current position if playing
        if isPlaying {
            currentPosition = synthController.getCurrentPosition()
        }
    }
    
    // MARK: - MIDI
    
    func sendMIDINote(channel: Int, note: Int, velocity: Int, on: Bool) {
        synthController.sendMIDINote(channel: channel, note: note, velocity: velocity, on: on)
    }
    
    func sendMIDICC(channel: Int, controller: Int, value: Int) {
        synthController.sendMIDICC(channel: channel, controller: controller, value: value)
    }
    
    func setMidiThru(_ enabled: Bool) {
        midiThru = enabled
        synthController.setMidiThru(enabled)
        saveUserSettings()
    }
    
    func setMidiClock(_ enabled: Bool) {
        midiClock = enabled
        synthController.setMidiClock(enabled)
        saveUserSettings()
    }
    
    // MARK: - User Settings
    
    private func loadUserSettings() {
        let defaults = UserDefaults.standard
        
        currentBPM = defaults.double(forKey: "DefaultBPM") != 0 ? defaults.double(forKey: "DefaultBPM") : 120.0
        masterVolume = defaults.float(forKey: "MasterVolume") != 0 ? defaults.float(forKey: "MasterVolume") : 0.8
        masterPan = defaults.float(forKey: "MasterPan")
        clickEnabled = defaults.bool(forKey: "ClickEnabled")
        clickVolume = defaults.float(forKey: "ClickVolume") != 0 ? defaults.float(forKey: "ClickVolume") : 0.5
        isAutoSaveEnabled = defaults.bool(forKey: "AutoSaveEnabled")
        selectedAudioInput = defaults.string(forKey: "SelectedAudioInput")
        selectedAudioOutput = defaults.string(forKey: "SelectedAudioOutput")
        selectedMidiInput = defaults.string(forKey: "SelectedMidiInput")
        selectedMidiOutput = defaults.string(forKey: "SelectedMidiOutput")
        midiThru = defaults.bool(forKey: "MidiThru")
        midiClock = defaults.bool(forKey: "MidiClock")
        
        if let viewString = defaults.string(forKey: "SelectedView"),
           let view = MainView(rawValue: viewString) {
            selectedView = view
        }
    }
    
    private func saveUserSettings() {
        let defaults = UserDefaults.standard
        
        defaults.set(currentBPM, forKey: "DefaultBPM")
        defaults.set(masterVolume, forKey: "MasterVolume")
        defaults.set(masterPan, forKey: "MasterPan")
        defaults.set(clickEnabled, forKey: "ClickEnabled")
        defaults.set(clickVolume, forKey: "ClickVolume")
        defaults.set(isAutoSaveEnabled, forKey: "AutoSaveEnabled")
        defaults.set(selectedAudioInput, forKey: "SelectedAudioInput")
        defaults.set(selectedAudioOutput, forKey: "SelectedAudioOutput")
        defaults.set(selectedMidiInput, forKey: "SelectedMidiInput")
        defaults.set(selectedMidiOutput, forKey: "SelectedMidiOutput")
        defaults.set(midiThru, forKey: "MidiThru")
        defaults.set(midiClock, forKey: "MidiClock")
        defaults.set(selectedView.rawValue, forKey: "SelectedView")
    }
    
    // MARK: - UI State
    
    func setSelectedView(_ view: MainView) {
        selectedView = view
        saveUserSettings()
    }
    
    func toggleFullscreen() {
        isFullscreen.toggle()
    }
    
    func showPreferences() {
        showingPreferences = true
    }
    
    func hidePreferences() {
        showingPreferences = false
    }
    
    func showAbout() {
        showingAbout = true
    }
    
    func hideAbout() {
        showingAbout = false
    }
    
    // MARK: - Utility Functions
    
    func formatTime(_ position: PlaybackPosition) -> String {
        return String(format: "%d:%d:%d", position.bar + 1, position.beat + 1, position.tick)
    }
    
    func formatBPM(_ bpm: Double) -> String {
        return String(format: "%.1f", bpm)
    }
    
    func formatLatency(_ latency: Float) -> String {
        return String(format: "%.1f ms", latency)
    }
    
    func formatCPU(_ cpu: Float) -> String {
        return String(format: "%.1f%%", cpu * 100)
    }
    
    func formatMemory(_ memory: Float) -> String {
        return String(format: "%.0f MB", memory)
    }
    
    // MARK: - Bridge to SynthController
    
    var synthControllerInstance: SynthController {
        return synthController
    }
}

// MARK: - Supporting Data Models

struct PlaybackPosition: Equatable {
    let bar: Int
    let beat: Int
    let tick: Int
}

struct ProjectInfo {
    let name: String
    let url: URL
    let lastModified: Date
}

struct MIDIDevice {
    let id: String
    let name: String
    let isConnected: Bool
}

struct AudioDevice {
    let id: String
    let name: String
    let sampleRates: [Float]
    let bufferSizes: [Int]
    let inputChannels: Int
    let outputChannels: Int
}

// PerformanceMetrics already defined in TimelineViewModel