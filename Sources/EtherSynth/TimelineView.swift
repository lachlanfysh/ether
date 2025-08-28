import SwiftUI

/**
 * Timeline View - 8-track engine assignment and pattern sequencing
 * Shows horizontal timeline with tracks, patterns, and engine assignments
 */

// MARK: - Timeline View Models
struct TimelineTrack {
    let id: Int
    let color: InstrumentColor
    var engineType: Int? // Engine type ID, nil = empty
    var patterns: [TimelinePattern]
    var muted: Bool = false
    var soloed: Bool = false
    var volume: Float = 0.8
    
    init(id: Int, color: InstrumentColor) {
        self.id = id
        self.color = color
        self.patterns = []
    }
}

struct TimelinePattern {
    let id: String
    let name: String
    let startBeat: Int
    let lengthBeat: Int
    let color: Color
    
    init(name: String, startBeat: Int, lengthBeat: Int, color: Color) {
        self.id = UUID().uuidString
        self.name = name
        self.startBeat = startBeat
        self.lengthBeat = lengthBeat
        self.color = color
    }
}

// MARK: - Main Timeline View
struct TimelineView: View {
    @EnvironmentObject var synthController: SynthController
    @State private var tracks: [TimelineTrack] = []
    @State private var currentPlayheadPosition: Double = 0
    @State private var totalBars: Int = 16
    @State private var selectedTrack: Int? = nil
    @State private var showingEngineAssignment: Bool = false
    @State private var isPlaying: Bool = false
    @State private var zoom: Double = 1.0
    
    // Timeline dimensions
    private let trackHeight: CGFloat = 48
    private let beatWidth: CGFloat = 16
    private let headerWidth: CGFloat = 120
    
    var body: some View {
        VStack(spacing: 0) {
            // Timeline Header with Transport
            TimelineHeader(
                isPlaying: $isPlaying,
                totalBars: $totalBars,
                zoom: $zoom,
                onPlay: { synthController.play() },
                onStop: { synthController.stop() },
                onRecord: { synthController.record() }
            )
            .padding(.horizontal, 16)
            .padding(.vertical, 8)
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .fill(Color.white.opacity(0.9))
                    .shadow(color: Color.black.opacity(0.1), radius: 2, x: 1, y: 1)
            )
            
            // Main Timeline Content
            GeometryReader { geometry in
                ScrollView([.horizontal, .vertical], showsIndicators: true) {
                    VStack(spacing: 0) {
                        // Beat ruler
                        BeatRuler(
                            totalBars: totalBars,
                            beatWidth: beatWidth * zoom,
                            headerWidth: headerWidth
                        )
                        
                        // Tracks
                        ForEach(Array(tracks.enumerated()), id: \.element.id) { index, track in
                            TimelineTrackRow(
                                track: track,
                                beatWidth: beatWidth * zoom,
                                headerWidth: headerWidth,
                                totalBars: totalBars,
                                isSelected: selectedTrack == track.id,
                                onTrackSelect: { selectedTrack = track.id },
                                onEngineAssign: { 
                                    selectedTrack = track.id
                                    showingEngineAssignment = true
                                },
                                onMute: { toggleMute(trackId: track.id) },
                                onSolo: { toggleSolo(trackId: track.id) }
                            )
                        }
                    }
                    .overlay(
                        // Playhead
                        TimelinePlayhead(
                            position: currentPlayheadPosition,
                            beatWidth: beatWidth * zoom,
                            headerWidth: headerWidth,
                            trackCount: tracks.count,
                            trackHeight: trackHeight
                        )
                    )
                }
            }
            .background(Color.gray.opacity(0.05))
        }
        .sheet(isPresented: $showingEngineAssignment) {
            if let trackId = selectedTrack {
                EngineAssignmentOverlay(
                    trackId: trackId,
                    trackColor: tracks[trackId].color,
                    currentEngineType: tracks[trackId].engineType,
                    onEngineSelect: { engineType in
                        assignEngine(trackId: trackId, engineType: engineType)
                    },
                    onClose: {
                        showingEngineAssignment = false
                    }
                )
            }
        }
        .onAppear {
            initializeTracks()
            startPlayheadTimer()
        }
    }
    
    // MARK: - Helper Functions
    private func initializeTracks() {
        tracks = InstrumentColor.allCases.enumerated().map { index, color in
            var track = TimelineTrack(id: index, color: color)
            
            // Add some demo patterns
            if index < 4 {
                track.patterns = [
                    TimelinePattern(name: "A", startBeat: 0, lengthBeat: 16, color: color.pastelColor),
                    TimelinePattern(name: "B", startBeat: 32, lengthBeat: 16, color: color.pastelColor.opacity(0.7))
                ]
            }
            
            return track
        }
    }
    
    private func toggleMute(trackId: Int) {
        if let index = tracks.firstIndex(where: { $0.id == trackId }) {
            tracks[index].muted.toggle()
        }
    }
    
    private func toggleSolo(trackId: Int) {
        if let index = tracks.firstIndex(where: { $0.id == trackId }) {
            tracks[index].soloed.toggle()
        }
    }
    
    private func assignEngine(trackId: Int, engineType: Int) {
        if let index = tracks.firstIndex(where: { $0.id == trackId }) {
            tracks[index].engineType = engineType
            
            // Update synth controller
            let color = InstrumentColor.allCases[trackId]
            synthController.assignEngine(engineType: engineType, to: color)
        }
        showingEngineAssignment = false
    }
    
    private func startPlayheadTimer() {
        Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { _ in
            if synthController.isPlaying {
                // Update playhead position based on BPM and time
                let beatsPerSecond = synthController.bpm / 60.0
                currentPlayheadPosition += beatsPerSecond * 0.1
                
                // Wrap around at end
                if currentPlayheadPosition >= Double(totalBars * 4) {
                    currentPlayheadPosition = 0
                }
            }
        }
    }
}

// MARK: - Timeline Header
struct TimelineHeader: View {
    @Binding var isPlaying: Bool
    @Binding var totalBars: Int
    @Binding var zoom: Double
    
    let onPlay: () -> Void
    let onStop: () -> Void
    let onRecord: () -> Void
    
    var body: some View {
        HStack {
            // Transport Controls
            HStack(spacing: 8) {
                Button(action: onPlay) {
                    Image(systemName: isPlaying ? "pause.fill" : "play.fill")
                        .font(.system(size: 14))
                        .foregroundColor(.primary)
                        .frame(width: 28, height: 28)
                        .background(
                            RoundedRectangle(cornerRadius: 6)
                                .fill(isPlaying ? Color.green.opacity(0.2) : Color.white)
                                .shadow(color: Color.black.opacity(0.1), radius: 1, x: 1, y: 1)
                        )
                }
                .buttonStyle(PlainButtonStyle())
                
                Button(action: onStop) {
                    Image(systemName: "stop.fill")
                        .font(.system(size: 14))
                        .foregroundColor(.primary)
                        .frame(width: 28, height: 28)
                        .background(
                            RoundedRectangle(cornerRadius: 6)
                                .fill(Color.white)
                                .shadow(color: Color.black.opacity(0.1), radius: 1, x: 1, y: 1)
                        )
                }
                .buttonStyle(PlainButtonStyle())
                
                Button(action: onRecord) {
                    Image(systemName: "record.circle.fill")
                        .font(.system(size: 14))
                        .foregroundColor(.red)
                        .frame(width: 28, height: 28)
                        .background(
                            RoundedRectangle(cornerRadius: 6)
                                .fill(Color.white)
                                .shadow(color: Color.black.opacity(0.1), radius: 1, x: 1, y: 1)
                        )
                }
                .buttonStyle(PlainButtonStyle())
            }
            
            Spacer()
            
            // Timeline Controls
            HStack(spacing: 12) {
                // Bar count
                HStack {
                    Text("Bars:")
                        .font(.system(size: 10))
                        .foregroundColor(.secondary)
                    
                    Text("\(totalBars)")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundColor(.primary)
                        .frame(minWidth: 20)
                }
                
                // Zoom
                HStack {
                    Text("Zoom:")
                        .font(.system(size: 10))
                        .foregroundColor(.secondary)
                    
                    Slider(value: $zoom, in: 0.5...3.0, step: 0.1) { }
                        .frame(width: 60)
                        .controlSize(.mini)
                }
                
                Text("\(Int(zoom * 100))%")
                    .font(.system(size: 10))
                    .foregroundColor(.secondary)
                    .frame(minWidth: 30)
            }
        }
    }
}

// MARK: - Beat Ruler
struct BeatRuler: View {
    let totalBars: Int
    let beatWidth: Double
    let headerWidth: CGFloat
    
    var body: some View {
        HStack(spacing: 0) {
            // Header spacer
            Rectangle()
                .fill(Color.clear)
                .frame(width: headerWidth, height: 24)
            
            // Beat markers
            HStack(spacing: 0) {
                ForEach(0..<(totalBars * 4), id: \.self) { beat in
                    VStack(spacing: 2) {
                        if beat % 4 == 0 {
                            // Bar marker
                            Text("\(beat / 4 + 1)")
                                .font(.system(size: 10, weight: .medium))
                                .foregroundColor(.primary)
                        } else {
                            // Beat marker
                            Text("\(beat % 4 + 1)")
                                .font(.system(size: 8))
                                .foregroundColor(.secondary)
                        }
                        
                        Rectangle()
                            .fill(beat % 4 == 0 ? Color.primary : Color.secondary.opacity(0.5))
                            .frame(width: 1, height: beat % 4 == 0 ? 8 : 4)
                    }
                    .frame(width: beatWidth, height: 24)
                }
            }
            
            Spacer()
        }
        .background(Color.white.opacity(0.8))
    }
}

// MARK: - Timeline Track Row
struct TimelineTrackRow: View {
    let track: TimelineTrack
    let beatWidth: Double
    let headerWidth: CGFloat
    let totalBars: Int
    let isSelected: Bool
    
    let onTrackSelect: () -> Void
    let onEngineAssign: () -> Void
    let onMute: () -> Void
    let onSolo: () -> Void
    
    var body: some View {
        HStack(spacing: 0) {
            // Track Header
            TrackHeader(
                track: track,
                isSelected: isSelected,
                onSelect: onTrackSelect,
                onEngineAssign: onEngineAssign,
                onMute: onMute,
                onSolo: onSolo
            )
            .frame(width: headerWidth, height: 48)
            .background(
                RoundedRectangle(cornerRadius: 4)
                    .fill(isSelected ? track.color.pastelColor.opacity(0.3) : Color.white.opacity(0.8))
                    .shadow(color: Color.black.opacity(0.05), radius: 1, x: 1, y: 1)
            )
            
            // Track Content Area
            ZStack(alignment: .leading) {
                // Background grid
                HStack(spacing: 0) {
                    ForEach(0..<(totalBars * 4), id: \.self) { beat in
                        Rectangle()
                            .stroke(Color.gray.opacity(beat % 4 == 0 ? 0.3 : 0.1), lineWidth: 0.5)
                            .frame(width: beatWidth, height: 48)
                    }
                }
                
                // Pattern blocks
                HStack(spacing: 0) {
                    ForEach(track.patterns, id: \.id) { pattern in
                        PatternBlock(
                            pattern: pattern,
                            beatWidth: beatWidth
                        )
                        .offset(x: Double(pattern.startBeat) * beatWidth)
                    }
                }
            }
        }
        .opacity(track.muted ? 0.5 : 1.0)
    }
}

// MARK: - Track Header
struct TrackHeader: View {
    let track: TimelineTrack
    let isSelected: Bool
    
    let onSelect: () -> Void
    let onEngineAssign: () -> Void
    let onMute: () -> Void
    let onSolo: () -> Void
    
    var body: some View {
        HStack(spacing: 4) {
            // Color indicator & Engine info
            VStack(spacing: 2) {
                Button(action: onSelect) {
                    Circle()
                        .fill(track.color.pastelColor)
                        .frame(width: 16, height: 16)
                        .overlay(
                            Circle()
                                .stroke(isSelected ? Color.primary : Color.clear, lineWidth: 2)
                        )
                }
                .buttonStyle(PlainButtonStyle())
                
                if let engineType = track.engineType {
                    Text("ENG")
                        .font(.system(size: 6, weight: .bold))
                        .foregroundColor(.primary)
                } else {
                    Button(action: onEngineAssign) {
                        Text("+")
                            .font(.system(size: 10, weight: .bold))
                            .foregroundColor(.secondary)
                    }
                    .buttonStyle(PlainButtonStyle())
                }
            }
            
            // Track info
            VStack(alignment: .leading, spacing: 1) {
                Text(track.color.rawValue)
                    .font(.system(size: 10, weight: .medium))
                    .foregroundColor(.primary)
                
                if let engineType = track.engineType {
                    // Would get engine name from synth controller
                    Text("Engine")
                        .font(.system(size: 8))
                        .foregroundColor(.secondary)
                } else {
                    Text("Empty")
                        .font(.system(size: 8))
                        .foregroundColor(.secondary)
                }
            }
            
            Spacer()
            
            // Mute/Solo buttons
            VStack(spacing: 2) {
                Button(action: onSolo) {
                    Text("S")
                        .font(.system(size: 8, weight: .bold))
                        .foregroundColor(track.soloed ? .orange : .secondary)
                        .frame(width: 16, height: 12)
                        .background(
                            RoundedRectangle(cornerRadius: 2)
                                .fill(track.soloed ? Color.orange.opacity(0.2) : Color.clear)
                        )
                }
                .buttonStyle(PlainButtonStyle())
                
                Button(action: onMute) {
                    Text("M")
                        .font(.system(size: 8, weight: .bold))
                        .foregroundColor(track.muted ? .red : .secondary)
                        .frame(width: 16, height: 12)
                        .background(
                            RoundedRectangle(cornerRadius: 2)
                                .fill(track.muted ? Color.red.opacity(0.2) : Color.clear)
                        )
                }
                .buttonStyle(PlainButtonStyle())
            }
        }
        .padding(.horizontal, 8)
    }
}

// MARK: - Pattern Block
struct PatternBlock: View {
    let pattern: TimelinePattern
    let beatWidth: Double
    
    var body: some View {
        RoundedRectangle(cornerRadius: 4)
            .fill(pattern.color.opacity(0.8))
            .frame(
                width: Double(pattern.lengthBeat) * beatWidth,
                height: 32
            )
            .overlay(
                Text(pattern.name)
                    .font(.system(size: 10, weight: .medium))
                    .foregroundColor(.white)
            )
            .shadow(color: Color.black.opacity(0.2), radius: 2, x: 1, y: 1)
    }
}

// MARK: - Timeline Playhead
struct TimelinePlayhead: View {
    let position: Double
    let beatWidth: Double
    let headerWidth: CGFloat
    let trackCount: Int
    let trackHeight: CGFloat
    
    var body: some View {
        Rectangle()
            .fill(Color.red)
            .frame(width: 2, height: CGFloat(trackCount) * trackHeight + 24)
            .offset(
                x: headerWidth + (position * beatWidth),
                y: 0
            )
    }
}

