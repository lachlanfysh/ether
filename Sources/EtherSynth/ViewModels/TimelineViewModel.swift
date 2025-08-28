import SwiftUI
import Combine

/**
 * Timeline View Model - MVVM architecture for timeline management
 * Handles track state, pattern management, and playback coordination
 */

// MARK: - Missing Data Models
struct AutomationPoint: Codable {
    let time: Float
    let value: Float
    let parameterId: Int
    
    init(time: Float, value: Float, parameterId: Int) {
        self.time = time
        self.value = value
        self.parameterId = parameterId
    }
}

@MainActor
class TimelineViewModel: ObservableObject {
    @Published var tracks: [TimelineTrack] = []
    @Published var currentBeat: Int = 0
    @Published var currentBar: Int = 0
    @Published var totalBars: Int = 16
    @Published var isPlaying: Bool = false
    @Published var selectedTrack: Int? = nil
    @Published var playheadPosition: Double = 0.0
    @Published var zoomLevel: Float = 1.0
    @Published var scrollOffset: Float = 0.0
    @Published var snapToGrid: Bool = true
    @Published var gridResolution: GridResolution = .sixteenth
    @Published var loopEnabled: Bool = true
    @Published var loopStart: Int = 0
    @Published var loopEnd: Int = 16
    
    // Performance tracking
    @Published var cpuUsage: Float = 0.0
    @Published var memoryUsage: Float = 0.0
    @Published var voiceCount: Int = 0
    @Published var renderLatency: Float = 0.0
    
    private var synthController: SynthController
    private var playbackTimer: Timer?
    private var performanceTimer: Timer?
    private var cancellables = Set<AnyCancellable>()
    
    enum GridResolution: String, CaseIterable {
        case quarter = "1/4"
        case eighth = "1/8"
        case sixteenth = "1/16"
        case thirtysecond = "1/32"
        
        var subdivisions: Int {
            switch self {
            case .quarter: return 4
            case .eighth: return 8
            case .sixteenth: return 16
            case .thirtysecond: return 32
            }
        }
    }
    
    init(synthController: SynthController) {
        self.synthController = synthController
        setupTracks()
        startPerformanceMonitoring()
    }
    
    deinit {
        stopPlayback()
        stopPerformanceMonitoring()
    }
    
    // MARK: - Track Management
    
    private func setupTracks() {
        tracks = (0..<8).map { index in
            TimelineTrack(
                id: index,
                name: "Track \(index + 1)",
                color: InstrumentColor.allCases[index % InstrumentColor.allCases.count],
                isMuted: false,
                isSoloed: false,
                volume: 0.8,
                patterns: [],
                assignedEngine: nil
            )
        }
    }
    
    func selectTrack(_ trackId: Int) {
        if selectedTrack == trackId {
            selectedTrack = nil
        } else {
            selectedTrack = trackId
        }
    }
    
    func toggleMute(trackId: Int) {
        guard trackId < tracks.count else { return }
        tracks[trackId].isMuted.toggle()
        
        // Update synth controller
        synthController.setTrackMute(track: trackId, muted: tracks[trackId].isMuted)
    }
    
    func toggleSolo(trackId: Int) {
        guard trackId < tracks.count else { return }
        tracks[trackId].isSoloed.toggle()
        
        // Handle solo logic
        let soloedTracks = tracks.enumerated().compactMap { index, track in
            track.isSoloed ? index : nil
        }
        
        for (index, track) in tracks.enumerated() {
            let shouldBeMuted = !soloedTracks.isEmpty && !track.isSoloed
            synthController.setTrackMute(track: index, muted: shouldBeMuted)
        }
    }
    
    func setTrackVolume(trackId: Int, volume: Float) {
        guard trackId < tracks.count else { return }
        tracks[trackId].volume = volume
        synthController.setTrackVolume(track: trackId, volume: volume)
    }
    
    func assignEngine(trackId: Int, engineType: Int) {
        guard trackId < tracks.count else { return }
        tracks[trackId].assignedEngine = engineType
        synthController.assignEngineToTrack(track: trackId, engineType: engineType)
    }
    
    func clearEngine(trackId: Int) {
        guard trackId < tracks.count else { return }
        tracks[trackId].assignedEngine = nil
        synthController.clearTrackEngine(track: trackId)
    }
    
    // MARK: - Pattern Management
    
    func addPattern(trackId: Int, startBar: Int, length: Int, type: PatternType) {
        guard trackId < tracks.count else { return }
        
        let pattern = TimelinePattern(
            id: UUID().uuidString,
            startBar: startBar,
            length: length,
            type: type,
            data: generateDefaultPatternData(type: type, length: length)
        )
        
        tracks[trackId].patterns.append(pattern)
        sortTrackPatterns(trackId: trackId)
    }
    
    func removePattern(trackId: Int, patternId: String) {
        guard trackId < tracks.count else { return }
        tracks[trackId].patterns.removeAll { $0.id == patternId }
    }
    
    func duplicatePattern(trackId: Int, patternId: String) {
        guard trackId < tracks.count,
              let pattern = tracks[trackId].patterns.first(where: { $0.id == patternId }) else { return }
        
        let duplicatedPattern = TimelinePattern(
            id: UUID().uuidString,
            startBar: pattern.startBar + pattern.length,
            length: pattern.length,
            type: pattern.type,
            data: pattern.data
        )
        
        tracks[trackId].patterns.append(duplicatedPattern)
        sortTrackPatterns(trackId: trackId)
    }
    
    func movePattern(trackId: Int, patternId: String, newStartBar: Int) {
        guard trackId < tracks.count,
              let patternIndex = tracks[trackId].patterns.firstIndex(where: { $0.id == patternId }) else { return }
        
        tracks[trackId].patterns[patternIndex].startBar = max(0, newStartBar)
        sortTrackPatterns(trackId: trackId)
    }
    
    func resizePattern(trackId: Int, patternId: String, newLength: Int) {
        guard trackId < tracks.count,
              let patternIndex = tracks[trackId].patterns.firstIndex(where: { $0.id == patternId }) else { return }
        
        tracks[trackId].patterns[patternIndex].length = max(1, newLength)
    }
    
    private func sortTrackPatterns(trackId: Int) {
        tracks[trackId].patterns.sort { $0.startBar < $1.startBar }
    }
    
    private func generateDefaultPatternData(type: PatternType, length: Int) -> PatternData {
        switch type {
        case .notes:
            return .notes([])
        case .drums:
            return .drums(generateDefaultDrumPattern(length: length))
        case .automation:
            return .automation([])
        }
    }
    
    private func generateDefaultDrumPattern(length: Int) -> [DrumHit] {
        // Generate a basic kick and snare pattern
        var hits: [DrumHit] = []
        
        for bar in 0..<length {
            for beat in 0..<4 {
                let position = bar * 4 + beat
                
                // Kick on 1 and 3
                if beat == 0 || beat == 2 {
                    hits.append(DrumHit(
                        position: Float(position),
                        velocity: 0.8,
                        drumSlot: 0, // Kick
                        probability: 1.0
                    ))
                }
                
                // Snare on 2 and 4
                if beat == 1 || beat == 3 {
                    hits.append(DrumHit(
                        position: Float(position),
                        velocity: 0.7,
                        drumSlot: 1, // Snare
                        probability: 1.0
                    ))
                }
            }
        }
        
        return hits
    }
    
    // MARK: - Playback Control
    
    func startPlayback() {
        guard !isPlaying else { return }
        
        isPlaying = true
        synthController.startPlayback()
        
        playbackTimer = Timer.scheduledTimer(withTimeInterval: 1.0/60.0, repeats: true) { [weak self] _ in
            self?.updatePlayheadPosition()
        }
    }
    
    func stopPlayback() {
        guard isPlaying else { return }
        
        isPlaying = false
        synthController.stopPlayback()
        
        playbackTimer?.invalidate()
        playbackTimer = nil
        
        // Reset playhead to start
        currentBeat = 0
        currentBar = 0
        playheadPosition = 0.0
    }
    
    func pausePlayback() {
        guard isPlaying else { return }
        
        isPlaying = false
        synthController.pausePlayback()
        
        playbackTimer?.invalidate()
        playbackTimer = nil
    }
    
    func seekToBar(_ bar: Int) {
        let clampedBar = max(0, min(totalBars - 1, bar))
        currentBar = clampedBar
        currentBeat = 0
        playheadPosition = Double(clampedBar)
        
        synthController.seekToPosition(bar: clampedBar, beat: 0)
    }
    
    func seekToBeat(_ beat: Int) {
        let beatsPerBar = 4
        let totalBeats = beat
        let newBar = totalBeats / beatsPerBar
        let newBeat = totalBeats % beatsPerBar
        
        if newBar < totalBars {
            currentBar = newBar
            currentBeat = newBeat
            playheadPosition = Double(newBar) + Double(newBeat) / Double(beatsPerBar)
            
            synthController.seekToPosition(bar: newBar, beat: newBeat)
        }
    }
    
    private func updatePlayheadPosition() {
        // Get current position from synth controller
        let position = synthController.getCurrentPosition()
        currentBar = position.bar
        currentBeat = position.beat
        playheadPosition = Double(position.bar) + Double(position.beat) / 4.0
        
        // Handle loop
        if loopEnabled && currentBar >= loopEnd {
            seekToBar(loopStart)
        }
        
        // Trigger pattern events
        triggerPatternsAtCurrentPosition()
    }
    
    private func triggerPatternsAtCurrentPosition() {
        for (trackIndex, track) in tracks.enumerated() {
            for pattern in track.patterns {
                if currentBar >= pattern.startBar && currentBar < pattern.startBar + pattern.length {
                    let patternBar = currentBar - pattern.startBar
                    triggerPatternEvents(trackId: trackIndex, pattern: pattern, bar: patternBar, beat: currentBeat)
                }
            }
        }
    }
    
    private func triggerPatternEvents(trackId: Int, pattern: TimelinePattern, bar: Int, beat: Int) {
        switch pattern.data {
        case .notes(let notes):
            triggerNoteEvents(trackId: trackId, notes: notes, bar: bar, beat: beat)
        case .drums(let hits):
            triggerDrumEvents(trackId: trackId, hits: hits, bar: bar, beat: beat)
        case .automation(let points):
            triggerAutomationEvents(trackId: trackId, points: points, bar: bar, beat: beat)
        }
    }
    
    private func triggerNoteEvents(trackId: Int, notes: [Note], bar: Int, beat: Int) {
        let currentPosition = Float(bar * 4 + beat)
        
        for note in notes {
            if abs(note.position - currentPosition) < 0.1 {
                synthController.triggerNote(
                    track: trackId,
                    pitch: note.pitch,
                    velocity: note.velocity,
                    length: note.length
                )
            }
        }
    }
    
    private func triggerDrumEvents(trackId: Int, hits: [DrumHit], bar: Int, beat: Int) {
        let currentPosition = Float(bar * 4 + beat)
        
        for hit in hits {
            if abs(hit.position - currentPosition) < 0.1 {
                // Apply probability
                if Float.random(in: 0...1) <= hit.probability {
                    synthController.triggerDrumHit(
                        track: trackId,
                        drumSlot: hit.drumSlot,
                        velocity: hit.velocity
                    )
                }
            }
        }
    }
    
    private func triggerAutomationEvents(trackId: Int, points: [AutomationPoint], bar: Int, beat: Int) {
        let currentPosition = Float(bar * 4 + beat)
        
        for point in points {
            if abs(point.position - currentPosition) < 0.1 {
                synthController.setAutomationValue(
                    track: trackId,
                    parameter: point.parameter,
                    value: point.value
                )
            }
        }
    }
    
    // MARK: - Loop Management
    
    func setLoopRegion(start: Int, end: Int) {
        loopStart = max(0, start)
        loopEnd = min(totalBars, max(loopStart + 1, end))
    }
    
    func clearLoop() {
        loopStart = 0
        loopEnd = totalBars
    }
    
    // MARK: - View Management
    
    func setZoom(_ zoom: Float) {
        zoomLevel = max(0.1, min(5.0, zoom))
    }
    
    func setScroll(_ offset: Float) {
        scrollOffset = max(0.0, offset)
    }
    
    func fitToView() {
        zoomLevel = 1.0
        scrollOffset = 0.0
    }
    
    func zoomToSelection() {
        if let trackId = selectedTrack, !tracks[trackId].patterns.isEmpty {
            let patterns = tracks[trackId].patterns
            let minBar = patterns.map { $0.startBar }.min() ?? 0
            let maxBar = patterns.map { $0.startBar + $0.length }.max() ?? totalBars
            
            // Calculate zoom to fit selection
            let selectionLength = maxBar - minBar
            zoomLevel = Float(8) / Float(selectionLength)
            scrollOffset = Float(minBar) / Float(totalBars)
        }
    }
    
    // MARK: - Performance Monitoring
    
    private func startPerformanceMonitoring() {
        performanceTimer = Timer.scheduledTimer(withTimeInterval: 0.5, repeats: true) { [weak self] _ in
            self?.updatePerformanceMetrics()
        }
    }
    
    private func stopPerformanceMonitoring() {
        performanceTimer?.invalidate()
        performanceTimer = nil
    }
    
    private func updatePerformanceMetrics() {
        let metrics = synthController.getPerformanceMetrics()
        
        cpuUsage = metrics.cpuUsage
        memoryUsage = metrics.memoryUsage
        voiceCount = metrics.activeVoices
        renderLatency = metrics.renderLatency
    }
    
    // MARK: - Export/Import
    
    func exportProject() -> ProjectData {
        return ProjectData(
            tracks: tracks,
            totalBars: totalBars,
            loopStart: loopStart,
            loopEnd: loopEnd,
            gridResolution: gridResolution
        )
    }
    
    func importProject(_ projectData: ProjectData) {
        tracks = projectData.tracks
        totalBars = projectData.totalBars
        loopStart = projectData.loopStart
        loopEnd = projectData.loopEnd
        gridResolution = projectData.gridResolution
        
        // Update synth controller with new track configuration
        for (index, track) in tracks.enumerated() {
            if let engineType = track.assignedEngine {
                synthController.assignEngineToTrack(track: index, engineType: engineType)
            }
            synthController.setTrackVolume(track: index, volume: track.volume)
            synthController.setTrackMute(track: index, muted: track.isMuted)
        }
    }
    
    // MARK: - Utility Functions
    
    func getTrackAtPosition(_ position: CGPoint, viewHeight: CGFloat) -> Int? {
        let trackHeight = viewHeight / CGFloat(tracks.count)
        let trackIndex = Int(position.y / trackHeight)
        return trackIndex < tracks.count ? trackIndex : nil
    }
    
    func getBarAtPosition(_ position: CGPoint, viewWidth: CGFloat) -> Int {
        let barWidth = viewWidth / CGFloat(totalBars) * CGFloat(zoomLevel)
        let scrolledPosition = position.x + CGFloat(scrollOffset) * viewWidth
        return Int(scrolledPosition / barWidth)
    }
    
    func getBeatAtPosition(_ position: CGPoint, viewWidth: CGFloat) -> Int {
        let barWidth = viewWidth / CGFloat(totalBars) * CGFloat(zoomLevel)
        let beatWidth = barWidth / 4
        let scrolledPosition = position.x + CGFloat(scrollOffset) * viewWidth
        return Int(scrolledPosition / beatWidth)
    }
    
    func snapToGrid(_ position: Float) -> Float {
        guard snapToGrid else { return position }
        
        let gridSize = 4.0 / Float(gridResolution.subdivisions)
        return round(position / gridSize) * gridSize
    }
}

// MARK: - Data Models for Export/Import

struct ProjectData {
    let tracks: [TimelineTrack]
    let totalBars: Int
    let loopStart: Int
    let loopEnd: Int
    let gridResolution: TimelineViewModel.GridResolution
}

// MARK: - Performance Metrics

struct PerformanceMetrics {
    let cpuUsage: Float
    let memoryUsage: Float
    let activeVoices: Int
    let renderLatency: Float
    let bufferUnderruns: Int
}