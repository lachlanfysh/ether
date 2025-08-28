import SwiftUI
import Combine

/**
 * Instrument Browser View Model - MVVM architecture for engine management
 * Handles engine discovery, categorization, and assignment logic
 */

// MARK: - Missing Data Models
struct PatternData {
    var drumHits: [DrumHit] = []
    var notes: [Note] = []
    var tempo: Float = 120.0
    var timeSignature: (Int, Int) = (4, 4)
}

struct DrumHit {
    let velocity: Float
    let time: Float
    let drum: Int
}

struct Note {
    let pitch: Int
    let velocity: Float
    let startTime: Float
    let duration: Float
}

@MainActor
class InstrumentBrowserViewModel: ObservableObject {
    @Published var engines: [EngineInfo] = []
    @Published var categories: [EngineCategory] = []
    @Published var selectedCategory: String = "All"
    @Published var searchText: String = ""
    @Published var selectedEngine: EngineInfo? = nil
    @Published var isLoading: Bool = false
    @Published var sortOrder: SortOrder = .category
    @Published var viewMode: ViewMode = .grid
    @Published var favoriteEngines: Set<Int> = []
    @Published var recentEngines: [Int] = []
    
    // Performance metrics for engines
    @Published var engineMetrics: [Int: EngineMetrics] = [:]
    
    private var synthController: SynthController
    private var metricsTimer: Timer?
    private var cancellables = Set<AnyCancellable>()
    
    enum SortOrder: String, CaseIterable {
        case category = "Category"
        case name = "Name"
        case recent = "Recent"
        case favorites = "Favorites"
        case cpu = "CPU Usage"
    }
    
    enum ViewMode: String, CaseIterable {
        case grid = "Grid"
        case list = "List"
        case compact = "Compact"
    }
    
    var filteredEngines: [EngineInfo] {
        var filtered = engines
        
        // Apply category filter
        if selectedCategory != "All" {
            filtered = filtered.filter { $0.category == selectedCategory }
        }
        
        // Apply search filter
        if !searchText.isEmpty {
            filtered = filtered.filter { engine in
                engine.name.localizedCaseInsensitiveContains(searchText) ||
                engine.description.localizedCaseInsensitiveContains(searchText) ||
                engine.tags.contains { $0.localizedCaseInsensitiveContains(searchText) }
            }
        }
        
        // Apply sorting
        switch sortOrder {
        case .category:
            filtered.sort { $0.category < $1.category }
        case .name:
            filtered.sort { $0.name < $1.name }
        case .recent:
            filtered.sort { engine1, engine2 in
                let recent1 = recentEngines.firstIndex(of: engine1.id) ?? Int.max
                let recent2 = recentEngines.firstIndex(of: engine2.id) ?? Int.max
                return recent1 < recent2
            }
        case .favorites:
            filtered.sort { engine1, engine2 in
                let fav1 = favoriteEngines.contains(engine1.id)
                let fav2 = favoriteEngines.contains(engine2.id)
                if fav1 == fav2 {
                    return engine1.name < engine2.name
                }
                return fav1 && !fav2
            }
        case .cpu:
            filtered.sort { engine1, engine2 in
                let cpu1 = engineMetrics[engine1.id]?.cpuUsage ?? 0.0
                let cpu2 = engineMetrics[engine2.id]?.cpuUsage ?? 0.0
                return cpu1 < cpu2
            }
        }
        
        return filtered
    }
    
    var favoriteEnginesList: [EngineInfo] {
        engines.filter { favoriteEngines.contains($0.id) }
    }
    
    var recentEnginesList: [EngineInfo] {
        recentEngines.compactMap { engineId in
            engines.first { $0.id == engineId }
        }
    }
    
    init(synthController: SynthController) {
        self.synthController = synthController
        loadEngines()
        setupCategories()
        startMetricsMonitoring()
        loadUserPreferences()
    }
    
    deinit {
        stopMetricsMonitoring()
        saveUserPreferences()
    }
    
    // MARK: - Engine Management
    
    private func loadEngines() {
        isLoading = true
        
        // Load engines from synth controller
        let engineCount = synthController.getEngineCount()
        engines = (0..<engineCount).compactMap { index in
            synthController.getEngineInfo(index: index)
        }
        
        isLoading = false
    }
    
    private func setupCategories() {
        let uniqueCategories = Set(engines.map { $0.category })
        
        categories = [
            EngineCategory(
                name: "All",
                engines: engines,
                icon: "grid.circle",
                color: .gray
            )
        ]
        
        categories.append(contentsOf: uniqueCategories.sorted().map { categoryName in
            let categoryEngines = engines.filter { $0.category == categoryName }
            return EngineCategory(
                name: categoryName,
                engines: categoryEngines,
                icon: iconForCategory(categoryName),
                color: colorForCategory(categoryName)
            )
        })
    }
    
    private func iconForCategory(_ category: String) -> String {
        switch category {
        case "Synthesizers": return "waveform"
        case "Multi-Voice": return "music.quarternote.3"
        case "Textures": return "cloud"
        case "Physical Models": return "tuningfork"
        case "Percussion & Samples": return "drum"
        default: return "gear"
        }
    }
    
    private func colorForCategory(_ category: String) -> Color {
        switch category {
        case "Synthesizers": return .blue
        case "Multi-Voice": return .green
        case "Textures": return .purple
        case "Physical Models": return .orange
        case "Percussion & Samples": return .red
        default: return .gray
        }
    }
    
    func selectEngine(_ engine: EngineInfo) {
        selectedEngine = engine
        addToRecent(engineId: engine.id)
    }
    
    func clearSelection() {
        selectedEngine = nil
    }
    
    func toggleFavorite(_ engineId: Int) {
        if favoriteEngines.contains(engineId) {
            favoriteEngines.remove(engineId)
        } else {
            favoriteEngines.insert(engineId)
        }
        saveUserPreferences()
    }
    
    private func addToRecent(engineId: Int) {
        // Remove if already in recent
        recentEngines.removeAll { $0 == engineId }
        
        // Add to front
        recentEngines.insert(engineId, at: 0)
        
        // Limit to 10 recent engines
        if recentEngines.count > 10 {
            recentEngines = Array(recentEngines.prefix(10))
        }
        
        saveUserPreferences()
    }
    
    // MARK: - Search and Filter
    
    func setSearchText(_ text: String) {
        searchText = text
    }
    
    func clearSearch() {
        searchText = ""
    }
    
    func setCategory(_ category: String) {
        selectedCategory = category
    }
    
    func setSortOrder(_ order: SortOrder) {
        sortOrder = order
    }
    
    func setViewMode(_ mode: ViewMode) {
        viewMode = mode
        saveUserPreferences()
    }
    
    // MARK: - Engine Assignment
    
    func assignToTrack(engineId: Int, trackId: Int) -> Bool {
        guard let engine = engines.first(where: { $0.id == engineId }) else {
            return false
        }
        
        let success = synthController.assignEngineToTrack(track: trackId, engineType: engineId)
        if success {
            addToRecent(engineId: engineId)
            updateEngineMetrics(engineId: engineId)
        }
        
        return success
    }
    
    func canAssignToTrack(engineId: Int, trackId: Int) -> Bool {
        // Check if track is available and engine is compatible
        return synthController.canAssignEngineToTrack(track: trackId, engineType: engineId)
    }
    
    func getEngineUsage(engineId: Int) -> [Int] {
        // Return list of tracks using this engine
        return synthController.getTracksUsingEngine(engineType: engineId)
    }
    
    // MARK: - Performance Monitoring
    
    private func startMetricsMonitoring() {
        metricsTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            self?.updateAllEngineMetrics()
        }
    }
    
    private func stopMetricsMonitoring() {
        metricsTimer?.invalidate()
        metricsTimer = nil
    }
    
    private func updateAllEngineMetrics() {
        for engine in engines {
            updateEngineMetrics(engineId: engine.id)
        }
    }
    
    private func updateEngineMetrics(engineId: Int) {
        let metrics = synthController.getEngineMetrics(engineType: engineId)
        engineMetrics[engineId] = EngineMetrics(
            cpuUsage: metrics.cpuUsage,
            memoryUsage: metrics.memoryUsage,
            voiceCount: metrics.voiceCount,
            polyphony: metrics.maxPolyphony
        )
    }
    
    // MARK: - Presets
    
    func getPresetsForEngine(_ engineId: Int) -> [PresetInfo] {
        return synthController.getEnginePresets(engineType: engineId)
    }
    
    func loadPreset(engineId: Int, presetId: String) -> Bool {
        return synthController.loadEnginePreset(engineType: engineId, presetId: presetId)
    }
    
    func savePreset(engineId: Int, name: String, description: String) -> Bool {
        return synthController.saveEnginePreset(engineType: engineId, name: name, description: description)
    }
    
    // MARK: - Audio Preview
    
    func startPreview(engineId: Int) {
        synthController.startEnginePreview(engineType: engineId)
    }
    
    func stopPreview() {
        synthController.stopEnginePreview()
    }
    
    func isPreviewPlaying(engineId: Int) -> Bool {
        return synthController.isEnginePreviewPlaying(engineType: engineId)
    }
    
    // MARK: - User Preferences
    
    private func loadUserPreferences() {
        if let data = UserDefaults.standard.data(forKey: "InstrumentBrowserPreferences"),
           let preferences = try? JSONDecoder().decode(BrowserPreferences.self, from: data) {
            favoriteEngines = Set(preferences.favoriteEngines)
            recentEngines = preferences.recentEngines
            sortOrder = preferences.sortOrder
            viewMode = preferences.viewMode
            selectedCategory = preferences.selectedCategory
        }
    }
    
    private func saveUserPreferences() {
        let preferences = BrowserPreferences(
            favoriteEngines: Array(favoriteEngines),
            recentEngines: recentEngines,
            sortOrder: sortOrder,
            viewMode: viewMode,
            selectedCategory: selectedCategory
        )
        
        if let data = try? JSONEncoder().encode(preferences) {
            UserDefaults.standard.set(data, forKey: "InstrumentBrowserPreferences")
        }
    }
    
    // MARK: - Quick Actions
    
    func getDemoPattern(for engineId: Int) -> PatternData? {
        guard let engine = engines.first(where: { $0.id == engineId }) else { return nil }
        
        switch engine.category {
        case "Percussion & Samples":
            return generateDrumPattern()
        case "Synthesizers", "Multi-Voice", "Textures", "Physical Models":
            return generateMelodyPattern()
        default:
            return nil
        }
    }
    
    private func generateDrumPattern() -> PatternData {
        var hits: [DrumHit] = []
        
        // Simple kick/snare pattern
        for bar in 0..<2 {
            for beat in 0..<4 {
                let position = Float(bar * 4 + beat)
                
                if beat == 0 || beat == 2 {
                    hits.append(DrumHit(
                        position: position,
                        velocity: 0.8,
                        drumSlot: 0,
                        probability: 1.0
                    ))
                }
                
                if beat == 1 || beat == 3 {
                    hits.append(DrumHit(
                        position: position,
                        velocity: 0.7,
                        drumSlot: 1,
                        probability: 1.0
                    ))
                }
            }
        }
        
        return .drums(hits)
    }
    
    private func generateMelodyPattern() -> PatternData {
        let scale = [60, 62, 64, 65, 67, 69, 71, 72] // C major scale
        var notes: [Note] = []
        
        for i in 0..<8 {
            let pitch = scale[i % scale.count]
            let position = Float(i) * 0.5
            
            notes.append(Note(
                position: position,
                pitch: pitch,
                velocity: 0.7,
                length: 0.4
            ))
        }
        
        return .notes(notes)
    }
    
    func createPatternFromEngine(engineId: Int, trackId: Int) {
        guard let patternData = getDemoPattern(for: engineId) else { return }
        
        // This would typically be handled by the timeline view model
        // For now, we'll just trigger the synth controller to create a pattern
        synthController.createPattern(track: trackId, data: patternData)
    }
    
    // MARK: - Engine Information
    
    func getEngineDetails(_ engineId: Int) -> EngineDetails? {
        guard let engine = engines.first(where: { $0.id == engineId }) else { return nil }
        
        let details = synthController.getEngineDetails(engineType: engineId)
        let metrics = engineMetrics[engineId] ?? EngineMetrics(cpuUsage: 0, memoryUsage: 0, voiceCount: 0, polyphony: 16)
        let usage = getEngineUsage(engineId: engineId)
        
        return EngineDetails(
            engine: engine,
            metrics: metrics,
            tracksUsing: usage,
            parameters: details.parameters,
            modulations: details.modulations,
            presets: getPresetsForEngine(engineId)
        )
    }
    
    func refreshEngineList() {
        loadEngines()
        setupCategories()
    }
}

// MARK: - Supporting Data Models

struct EngineMetrics {
    let cpuUsage: Float
    let memoryUsage: Float
    let voiceCount: Int
    let polyphony: Int
}

struct EngineDetails {
    let engine: EngineInfo
    let metrics: EngineMetrics
    let tracksUsing: [Int]
    let parameters: [ParameterInfo]
    let modulations: [ModulationInfo]
    let presets: [PresetInfo]
}

struct ParameterInfo {
    let id: String
    let name: String
    let value: Float
    let range: ClosedRange<Float>
    let unit: String
}

struct ModulationInfo {
    let source: String
    let destination: String
    let amount: Float
}

struct PresetInfo: Codable {
    let id: String
    let name: String
    let description: String
    let category: String
    let tags: [String]
}

struct BrowserPreferences: Codable {
    let favoriteEngines: [Int]
    let recentEngines: [Int]
    let sortOrder: InstrumentBrowserViewModel.SortOrder
    let viewMode: InstrumentBrowserViewModel.ViewMode
    let selectedCategory: String
}

// MARK: - Extensions for Codable Support

extension InstrumentBrowserViewModel.SortOrder: Codable {}
extension InstrumentBrowserViewModel.ViewMode: Codable {}