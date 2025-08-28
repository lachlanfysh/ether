import SwiftUI
import AVFoundation
import Combine

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

// MARK: - Engine Data Models
struct EngineType {
    let id: String
    let name: String
    let cpuClass: String
    let stereo: Bool
    let extras: [String]
    
    static let allEngines = [
        EngineType(id: "MacroWT", name: "MacroWT", cpuClass: "Medium", stereo: true, extras: ["UNISON", "SPREAD", "SUB", "NOISE", "SYNC"]),
        EngineType(id: "MacroFM", name: "MacroFM", cpuClass: "Medium", stereo: true, extras: ["ALGO", "FEEDBACK", "INDEX", "RATIO", "FOLD"]),
        EngineType(id: "Additive", name: "Additive", cpuClass: "Medium", stereo: true, extras: ["PARTIALS", "EVEN/ODD", "SKEW", "WARP"]),
        EngineType(id: "Waveshape", name: "Waveshape", cpuClass: "Light", stereo: true, extras: ["SYMMETRY", "DRIVE ALG", "FOLD THR", "BIAS"]),
        EngineType(id: "Vocal", name: "Vocal", cpuClass: "Medium", stereo: true, extras: ["VOWEL", "FORMANT", "BREATH", "NOISE MIX"]),
        EngineType(id: "Resonator", name: "Resonator", cpuClass: "Medium", stereo: true, extras: ["EXCITER", "DECAY", "DAMP", "BRIGHT"]),
        EngineType(id: "Modal", name: "Modal", cpuClass: "Heavy", stereo: true, extras: ["EXCITER", "MATERIAL", "BRIGHT", "SPACE"]),
        EngineType(id: "Noise", name: "Noise", cpuClass: "Light", stereo: true, extras: ["COLOR", "DENSITY", "HP", "LP"]),
        EngineType(id: "Tides", name: "Tides", cpuClass: "Medium", stereo: true, extras: ["SLOPE", "SMOOTH", "SHAPE", "RATE"]),
        EngineType(id: "DrumKit", name: "DrumKit", cpuClass: "Heavy", stereo: false, extras: ["ACCENT", "HUMANIZE", "SEED"])
    ]
    
    var color: Color {
        let palette: [String: Color] = [
            "MacroWT": Color(red: 0.55, green: 0.63, blue: 0.80),
            "MacroFM": Color(red: 0.99, green: 0.55, blue: 0.38),
            "Additive": Color(red: 0.65, green: 0.81, blue: 0.89),
            "Waveshape": Color(red: 0.98, green: 0.60, blue: 0.60),
            "Vocal": Color(red: 0.91, green: 0.54, blue: 0.76),
            "Resonator": Color(red: 0.70, green: 0.87, blue: 0.54),
            "Modal": Color(red: 1.0, green: 1.0, blue: 0.60),
            "Noise": Color(red: 0.70, green: 0.70, blue: 0.70),
            "Tides": Color(red: 0.75, green: 0.73, blue: 0.85),
            "DrumKit": Color(red: 1.0, green: 0.85, blue: 0.18)
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
class EtherSynthState: ObservableObject {
    @Published var tracks: [Track] = Array(repeating: Track(), count: 8)
    @Published var currentView: String = "timeline"
    @Published var currentTrack: Int?
    @Published var showingOverlay: Bool = false
    @Published var overlayType: OverlayType?
    @Published var isPlaying: Bool = false
    @Published var isRecording: Bool = false
    @Published var bpm: Float = 120.0
    @Published var swing: Float = 54.0
    @Published var currentBar: String = "05:03"
    @Published var cpuUsage: String = "▓▓▓░"
    @Published var projectName: String = "Engine UI v2"
    
    // Pattern management
    @Published var selectedPatternID: UUID?
    @Published var copiedPattern: PatternBlock?
    private var patternIDCounter = 0
    
    // C++ Engine
    private var synthEngine: UnsafeMutableRawPointer?
    
    init() {
        setupEngine()
    }
    
    deinit {
        if let engine = synthEngine {
            ether_destroy(engine)
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
            }
        }
    }
    
    enum OverlayType {
        case assign(trackIndex: Int)
        case sound(trackIndex: Int)
        case pattern(trackIndex: Int)
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
    
    func closeOverlay() {
        showingOverlay = false
        overlayType = nil
        currentTrack = nil
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
            // Main app container with rounded corners and shadow (like HTML prototype)
            RoundedRectangle(cornerRadius: 24)
                .fill(Color(red: 0.97, green: 0.98, blue: 0.99)) // --dev-bg color
                .shadow(color: Color.black.opacity(0.08), radius: 12, x: 0, y: 10)
                .overlay(
                    VStack(spacing: 0) {
                        // Top bar (36px height)
                        TopBar()
                            .frame(height: 36)
                        
                        // Navigation bar (36px height) 
                        NavigationBar()
                            .frame(height: 36)
                        
                        // Main view area (remaining height: 320 - 72 = 248px)
                        ZStack {
                            // View content
                            Group {
                                if appState.currentView == "timeline" {
                                    TimelineView()
                                } else if appState.currentView == "mod" {
                                    ModView()
                                } else if appState.currentView == "mix" {
                                    MixView()
                                }
                            }
                            .frame(maxWidth: .infinity, maxHeight: .infinity)
                            
                            // Overlay system
                            if appState.showingOverlay {
                                OverlayView()
                            }
                        }
                        .frame(height: 248)
                    }
                    .clipShape(RoundedRectangle(cornerRadius: 24))
                )
        }
        .frame(width: 960, height: 320)
        .background(Color(red: 0.95, green: 0.96, blue: 0.97)) // --app-bg color
    }
}

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

// MARK: - Pattern Overlay (Placeholder)
struct PatternOverlay: View {
    let trackIndex: Int
    @EnvironmentObject var appState: EtherSynthState
    
    private var track: Track {
        appState.tracks[trackIndex]
    }
    
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("\(track.engine?.name ?? "Unknown") — Pattern A")
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
                Text("Pattern editor will go here")
                    .font(.custom("Barlow Condensed", size: 12))
                    .foregroundColor(Color(red: 0.36, green: 0.42, blue: 0.49))
                
                Text(track.engine?.id == "DrumKit" ? "Drum grid interface" : "Pitched step sequencer")
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