import SwiftUI

/**
 * Scatter Browser - 2D sample cloud visualization for intuitive sample discovery
 * Maps samples across timbre/energy axes with real-time audio preview
 */

// MARK: - Scatter Browser Models
struct SamplePoint {
    let id: String
    let name: String
    let path: String
    let position: CGPoint // X: timbre (0-1), Y: energy (0-1)
    let color: Color
    let category: SampleCategory
    let duration: Float
    let bpm: Float?
    let key: String?
    let tags: [String]
    
    var displayName: String {
        return String(name.prefix(20)) + (name.count > 20 ? "..." : "")
    }
}

struct SampleCategory {
    let name: String
    let color: Color
    let icon: String
    
    static let categories = [
        SampleCategory(name: "Drums", color: .red, icon: "drum"),
        SampleCategory(name: "Bass", color: .purple, icon: "waveform.path"),
        SampleCategory(name: "Synths", color: .blue, icon: "sine.wave"),
        SampleCategory(name: "Vocals", color: .green, icon: "person.wave.2"),
        SampleCategory(name: "FX", color: .orange, icon: "sparkles"),
        SampleCategory(name: "Loops", color: .pink, icon: "repeat.circle"),
        SampleCategory(name: "Ambient", color: .cyan, icon: "cloud"),
        SampleCategory(name: "Percussion", color: .yellow, icon: "figure.drumming")
    ]
}

struct ScatterAnalysis {
    var isAnalyzing: Bool = false
    var analyzedSamples: Int = 0
    var totalSamples: Int = 0
    var currentSample: String = ""
    
    var progress: Float {
        guard totalSamples > 0 else { return 0.0 }
        return Float(analyzedSamples) / Float(totalSamples)
    }
}

// MARK: - Main Scatter Browser
struct ScatterBrowser: View {
    let onSampleSelect: (String, String) -> Void
    
    @State private var samplePoints: [SamplePoint] = []
    @State private var selectedPoint: SamplePoint? = nil
    @State private var hoveredPoint: SamplePoint? = nil
    @State private var scatterAnalysis = ScatterAnalysis()
    @State private var selectedCategories: Set<String> = Set(SampleCategory.categories.map { $0.name })
    @State private var searchText: String = ""
    @State private var zoomLevel: Float = 1.0
    @State private var panOffset: CGSize = .zero
    @State private var showingAnalysisSettings: Bool = false
    @State private var previewingSample: String? = nil
    
    private let scatterSize: CGSize = CGSize(width: 600, height: 400)
    
    var filteredSamplePoints: [SamplePoint] {
        samplePoints.filter { point in
            let categoryMatch = selectedCategories.contains(point.category.name)
            let searchMatch = searchText.isEmpty || 
                             point.name.localizedCaseInsensitiveContains(searchText) ||
                             point.tags.contains { $0.localizedCaseInsensitiveContains(searchText) }
            return categoryMatch && searchMatch
        }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            ScatterBrowserHeader(
                selectedPoint: selectedPoint,
                onAnalyze: { showingAnalysisSettings = true },
                onClose: { /* Close browser */ }
            )
            .padding(.horizontal, 20)
            .padding(.vertical, 16)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(Color.blue.opacity(0.1))
            )
            
            HStack(spacing: 0) {
                // Left Panel - Controls
                VStack(spacing: 16) {
                    // Search
                    ScatterSearchBar(searchText: $searchText)
                    
                    // Category Filters
                    CategoryFilterPanel(
                        selectedCategories: $selectedCategories,
                        allCategories: SampleCategory.categories
                    )
                    
                    // Navigation Controls
                    ScatterNavigationControls(
                        zoomLevel: $zoomLevel,
                        panOffset: $panOffset
                    )
                    
                    Spacer()
                    
                    // Selected Sample Info
                    if let selected = selectedPoint {
                        SelectedSampleInfo(
                            sample: selected,
                            onPreview: { previewSample(selected.path) },
                            onSelect: { 
                                onSampleSelect(selected.path, selected.name)
                            }
                        )
                    }
                }
                .frame(width: 240)
                .padding(.horizontal, 16)
                .padding(.vertical, 16)
                .background(Color.gray.opacity(0.05))
                
                // Main Scatter Plot
                VStack(spacing: 12) {
                    // Axis Labels
                    HStack {
                        Spacer()
                        Text("Timbre →")
                            .font(.system(size: 12, weight: .medium))
                            .foregroundColor(.secondary)
                        Spacer()
                    }
                    
                    HStack(spacing: 12) {
                        // Y-axis label
                        VStack {
                            Spacer()
                            Text("Energy")
                                .font(.system(size: 12, weight: .medium))
                                .foregroundColor(.secondary)
                                .rotationEffect(.degrees(-90))
                            Text("↑")
                                .font(.system(size: 12, weight: .medium))
                                .foregroundColor(.secondary)
                            Spacer()
                        }
                        .frame(width: 20)
                        
                        // Scatter Plot
                        ScatterPlotView(
                            samplePoints: filteredSamplePoints,
                            selectedPoint: $selectedPoint,
                            hoveredPoint: $hoveredPoint,
                            zoomLevel: zoomLevel,
                            panOffset: panOffset,
                            onPointSelect: { point in
                                selectedPoint = point
                            },
                            onPointHover: { point in
                                hoveredPoint = point
                            },
                            onPointDoubleClick: { point in
                                onSampleSelect(point.path, point.name)
                            }
                        )
                        .frame(width: scatterSize.width, height: scatterSize.height)
                    }
                    
                    // Stats
                    ScatterStats(
                        totalSamples: samplePoints.count,
                        visibleSamples: filteredSamplePoints.count,
                        selectedCategories: selectedCategories.count,
                        analysis: scatterAnalysis
                    )
                    .padding(.horizontal, 20)
                }
                .padding(.vertical, 16)
            }
        }
        .frame(width: 900, height: 600)
        .background(
            RoundedRectangle(cornerRadius: 16)
                .fill(Color.white)
                .shadow(color: Color.black.opacity(0.3), radius: 12, x: 0, y: 8)
        )
        .sheet(isPresented: $showingAnalysisSettings) {
            AnalysisSettingsSheet(
                onStartAnalysis: { settings in
                    startSampleAnalysis(settings: settings)
                }
            )
        }
        .onAppear {
            initializeDemoSamples()
        }
    }
    
    // MARK: - Helper Functions
    private func initializeDemoSamples() {
        // Generate demo sample points
        samplePoints = [
            // Drums - High energy, varied timbre
            SamplePoint(id: "kick1", name: "Kick_808.wav", path: "/samples/kick_808.wav", 
                       position: CGPoint(x: 0.2, y: 0.8), color: .red, 
                       category: SampleCategory.categories[0], duration: 1.2, bpm: nil, key: "C", tags: ["kick", "808", "sub"]),
            SamplePoint(id: "snare1", name: "Snare_Clap.wav", path: "/samples/snare_clap.wav", 
                       position: CGPoint(x: 0.6, y: 0.9), color: .red, 
                       category: SampleCategory.categories[0], duration: 0.8, bpm: nil, key: nil, tags: ["snare", "clap", "bright"]),
            SamplePoint(id: "hihat1", name: "HiHat_Closed.wav", path: "/samples/hihat_closed.wav", 
                       position: CGPoint(x: 0.8, y: 0.6), color: .red, 
                       category: SampleCategory.categories[0], duration: 0.3, bpm: nil, key: nil, tags: ["hihat", "closed", "crisp"]),
            
            // Bass - Low timbre, medium energy
            SamplePoint(id: "bass1", name: "Bass_Sub.wav", path: "/samples/bass_sub.wav", 
                       position: CGPoint(x: 0.1, y: 0.4), color: .purple, 
                       category: SampleCategory.categories[1], duration: 2.0, bpm: 120, key: "F", tags: ["bass", "sub", "deep"]),
            SamplePoint(id: "bass2", name: "Bass_Reese.wav", path: "/samples/bass_reese.wav", 
                       position: CGPoint(x: 0.3, y: 0.5), color: .purple, 
                       category: SampleCategory.categories[1], duration: 1.5, bpm: 128, key: "A", tags: ["bass", "reese", "growl"]),
            
            // Synths - Medium timbre, varied energy
            SamplePoint(id: "synth1", name: "Synth_Lead.wav", path: "/samples/synth_lead.wav", 
                       position: CGPoint(x: 0.5, y: 0.7), color: .blue, 
                       category: SampleCategory.categories[2], duration: 4.0, bpm: 140, key: "C", tags: ["synth", "lead", "bright"]),
            SamplePoint(id: "synth2", name: "Synth_Pad.wav", path: "/samples/synth_pad.wav", 
                       position: CGPoint(x: 0.4, y: 0.3), color: .blue, 
                       category: SampleCategory.categories[2], duration: 8.0, bpm: nil, key: "Em", tags: ["synth", "pad", "ambient"]),
            
            // Vocals - High timbre, medium energy
            SamplePoint(id: "vocal1", name: "Vocal_Chop.wav", path: "/samples/vocal_chop.wav", 
                       position: CGPoint(x: 0.7, y: 0.5), color: .green, 
                       category: SampleCategory.categories[3], duration: 1.0, bpm: nil, key: nil, tags: ["vocal", "chop", "human"]),
            
            // FX - High timbre, varied energy
            SamplePoint(id: "fx1", name: "FX_Sweep.wav", path: "/samples/fx_sweep.wav", 
                       position: CGPoint(x: 0.9, y: 0.4), color: .orange, 
                       category: SampleCategory.categories[4], duration: 3.0, bpm: nil, key: nil, tags: ["fx", "sweep", "transition"]),
            
            // Loops - Varied timbre and energy
            SamplePoint(id: "loop1", name: "Loop_Drum.wav", path: "/samples/loop_drum.wav", 
                       position: CGPoint(x: 0.45, y: 0.75), color: .pink, 
                       category: SampleCategory.categories[5], duration: 8.0, bpm: 130, key: nil, tags: ["loop", "drum", "groove"]),
            
            // Ambient - Low timbre, low energy
            SamplePoint(id: "ambient1", name: "Ambient_Drone.wav", path: "/samples/ambient_drone.wav", 
                       position: CGPoint(x: 0.2, y: 0.2), color: .cyan, 
                       category: SampleCategory.categories[6], duration: 16.0, bpm: nil, key: "Dm", tags: ["ambient", "drone", "texture"]),
            
            // Percussion - Medium timbre, high energy
            SamplePoint(id: "perc1", name: "Perc_Shaker.wav", path: "/samples/perc_shaker.wav", 
                       position: CGPoint(x: 0.6, y: 0.8), color: .yellow, 
                       category: SampleCategory.categories[7], duration: 0.5, bpm: nil, key: nil, tags: ["percussion", "shaker", "rhythm"])
        ]
    }
    
    private func startSampleAnalysis(settings: AnalysisSettings) {
        scatterAnalysis.isAnalyzing = true
        scatterAnalysis.totalSamples = samplePoints.count
        scatterAnalysis.analyzedSamples = 0
        
        showingAnalysisSettings = false
        
        // Simulate analysis process
        Timer.scheduledTimer(withTimeInterval: 0.5, repeats: true) { timer in
            if scatterAnalysis.analyzedSamples < scatterAnalysis.totalSamples {
                scatterAnalysis.analyzedSamples += 1
                scatterAnalysis.currentSample = samplePoints[scatterAnalysis.analyzedSamples - 1].name
            } else {
                scatterAnalysis.isAnalyzing = false
                timer.invalidate()
            }
        }
    }
    
    private func previewSample(_ path: String) {
        previewingSample = path
        // TODO: Play sample preview
        
        DispatchQueue.main.asyncAfter(deadline: .now() + 2.0) {
            previewingSample = nil
        }
    }
}

// MARK: - Scatter Browser Header
struct ScatterBrowserHeader: View {
    let selectedPoint: SamplePoint?
    let onAnalyze: () -> Void
    let onClose: () -> Void
    
    var body: some View {
        HStack {
            // Title
            HStack(spacing: 12) {
                Circle()
                    .fill(Color.blue)
                    .frame(width: 32, height: 32)
                    .overlay(
                        Image(systemName: "cloud.circle.fill")
                            .font(.system(size: 16))
                            .foregroundColor(.white)
                    )
                
                VStack(alignment: .leading, spacing: 2) {
                    Text("Scatter Browser")
                        .font(.system(size: 16, weight: .bold))
                        .foregroundColor(.primary)
                    
                    Text("2D sample exploration")
                        .font(.system(size: 12))
                        .foregroundColor(.secondary)
                }
            }
            
            Spacer()
            
            // Actions
            HStack(spacing: 8) {
                Button(action: onAnalyze) {
                    HStack(spacing: 4) {
                        Image(systemName: "waveform.path.ecg")
                            .font(.system(size: 12))
                        Text("Analyze")
                            .font(.system(size: 12, weight: .medium))
                    }
                    .foregroundColor(.blue)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 6)
                    .background(
                        RoundedRectangle(cornerRadius: 16)
                            .fill(Color.blue.opacity(0.1))
                    )
                }
                .buttonStyle(PlainButtonStyle())
                
                Button(action: onClose) {
                    Image(systemName: "xmark.circle.fill")
                        .font(.system(size: 20))
                        .foregroundColor(.secondary)
                }
                .buttonStyle(PlainButtonStyle())
            }
        }
    }
}

// MARK: - Scatter Search Bar
struct ScatterSearchBar: View {
    @Binding var searchText: String
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Search Samples")
                .font(.system(size: 12, weight: .bold))
                .foregroundColor(.primary)
            
            HStack {
                Image(systemName: "magnifyingglass")
                    .font(.system(size: 12))
                    .foregroundColor(.secondary)
                
                TextField("Sample name or tag...", text: $searchText)
                    .textFieldStyle(PlainTextFieldStyle())
                    .font(.system(size: 12))
                
                if !searchText.isEmpty {
                    Button(action: { searchText = "" }) {
                        Image(systemName: "xmark.circle.fill")
                            .font(.system(size: 12))
                            .foregroundColor(.secondary)
                    }
                    .buttonStyle(PlainButtonStyle())
                }
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 6)
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .fill(Color.white)
                    .stroke(Color.gray.opacity(0.3), lineWidth: 1)
            )
        }
    }
}

// MARK: - Category Filter Panel
struct CategoryFilterPanel: View {
    @Binding var selectedCategories: Set<String>
    let allCategories: [SampleCategory]
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Categories")
                    .font(.system(size: 12, weight: .bold))
                    .foregroundColor(.primary)
                
                Spacer()
                
                Button(action: toggleAllCategories) {
                    Text(selectedCategories.count == allCategories.count ? "None" : "All")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.blue)
                }
                .buttonStyle(PlainButtonStyle())
            }
            
            LazyVGrid(columns: [
                GridItem(.flexible()),
                GridItem(.flexible())
            ], spacing: 6) {
                ForEach(allCategories, id: \.name) { category in
                    CategoryToggle(
                        category: category,
                        isSelected: selectedCategories.contains(category.name),
                        onToggle: { toggleCategory(category.name) }
                    )
                }
            }
        }
    }
    
    private func toggleCategory(_ categoryName: String) {
        if selectedCategories.contains(categoryName) {
            selectedCategories.remove(categoryName)
        } else {
            selectedCategories.insert(categoryName)
        }
    }
    
    private func toggleAllCategories() {
        if selectedCategories.count == allCategories.count {
            selectedCategories.removeAll()
        } else {
            selectedCategories = Set(allCategories.map { $0.name })
        }
    }
}

// MARK: - Category Toggle
struct CategoryToggle: View {
    let category: SampleCategory
    let isSelected: Bool
    let onToggle: () -> Void
    
    var body: some View {
        Button(action: onToggle) {
            HStack(spacing: 4) {
                Image(systemName: category.icon)
                    .font(.system(size: 8))
                    .foregroundColor(isSelected ? .white : category.color)
                
                Text(category.name)
                    .font(.system(size: 8, weight: .medium))
                    .foregroundColor(isSelected ? .white : category.color)
                    .lineLimit(1)
            }
            .padding(.horizontal, 6)
            .padding(.vertical, 4)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(isSelected ? category.color : category.color.opacity(0.1))
            )
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - Scatter Navigation Controls
struct ScatterNavigationControls: View {
    @Binding var zoomLevel: Float
    @Binding var panOffset: CGSize
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Navigation")
                .font(.system(size: 12, weight: .bold))
                .foregroundColor(.primary)
            
            // Zoom
            VStack(spacing: 4) {
                HStack {
                    Text("Zoom")
                        .font(.system(size: 10))
                        .foregroundColor(.secondary)
                    Spacer()
                    Text("\(String(format: "%.1f", zoomLevel))x")
                        .font(.system(size: 10, weight: .bold))
                        .foregroundColor(.primary)
                }
                
                Slider(value: $zoomLevel, in: 1.0...5.0)
                    .accentColor(.blue)
                    .controlSize(.mini)
            }
            
            // Reset Pan
            Button(action: { panOffset = .zero }) {
                HStack(spacing: 4) {
                    Image(systemName: "arrow.uturn.backward")
                        .font(.system(size: 10))
                    Text("Reset View")
                        .font(.system(size: 10, weight: .medium))
                }
                .foregroundColor(.blue)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(
                    RoundedRectangle(cornerRadius: 12)
                        .fill(Color.blue.opacity(0.1))
                )
            }
            .buttonStyle(PlainButtonStyle())
        }
    }
}

// MARK: - Scatter Plot View
struct ScatterPlotView: View {
    let samplePoints: [SamplePoint]
    @Binding var selectedPoint: SamplePoint?
    @Binding var hoveredPoint: SamplePoint?
    let zoomLevel: Float
    let panOffset: CGSize
    let onPointSelect: (SamplePoint) -> Void
    let onPointHover: (SamplePoint?) -> Void
    let onPointDoubleClick: (SamplePoint) -> Void
    
    @State private var dragOffset: CGSize = .zero
    @State private var isDragging: Bool = false
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Background grid
                ScatterGrid(size: geometry.size)
                
                // Sample points
                ForEach(samplePoints, id: \.id) { point in
                    let isSelected = selectedPoint?.id == point.id
                    let isHovered = hoveredPoint?.id == point.id
                    
                    ScatterPoint(
                        point: point,
                        isSelected: isSelected,
                        isHovered: isHovered,
                        containerSize: geometry.size,
                        zoomLevel: zoomLevel,
                        panOffset: CGSize(
                            width: panOffset.width + dragOffset.width,
                            height: panOffset.height + dragOffset.height
                        ),
                        onSelect: { onPointSelect(point) },
                        onHover: { onPointHover(isHovered ? nil : point) },
                        onDoubleClick: { onPointDoubleClick(point) }
                    )
                }
                
                // Hover info
                if let hovered = hoveredPoint {
                    ScatterHoverInfo(point: hovered)
                        .position(
                            x: CGFloat(hovered.position.x) * geometry.size.width,
                            y: (1.0 - CGFloat(hovered.position.y)) * geometry.size.height - 40
                        )
                }
            }
            .clipped()
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .fill(Color.white)
                    .stroke(Color.gray.opacity(0.3), lineWidth: 1)
            )
            .gesture(
                DragGesture()
                    .onChanged { value in
                        if !isDragging {
                            isDragging = true
                        }
                        dragOffset = value.translation
                    }
                    .onEnded { value in
                        // panOffset = CGSize(
                        //     width: panOffset.width + dragOffset.width,
                        //     height: panOffset.height + dragOffset.height
                        // )
                        dragOffset = .zero
                        isDragging = false
                    }
            )
        }
    }
}

// MARK: - Scatter Grid
struct ScatterGrid: View {
    let size: CGSize
    
    var body: some View {
        Path { path in
            // Vertical lines
            for i in 0...10 {
                let x = (CGFloat(i) / 10.0) * size.width
                path.move(to: CGPoint(x: x, y: 0))
                path.addLine(to: CGPoint(x: x, y: size.height))
            }
            
            // Horizontal lines
            for i in 0...10 {
                let y = (CGFloat(i) / 10.0) * size.height
                path.move(to: CGPoint(x: 0, y: y))
                path.addLine(to: CGPoint(x: size.width, y: y))
            }
        }
        .stroke(Color.gray.opacity(0.2), lineWidth: 0.5)
    }
}

// MARK: - Scatter Point
struct ScatterPoint: View {
    let point: SamplePoint
    let isSelected: Bool
    let isHovered: Bool
    let containerSize: CGSize
    let zoomLevel: Float
    let panOffset: CGSize
    let onSelect: () -> Void
    let onHover: () -> Void
    let onDoubleClick: () -> Void
    
    private var position: CGPoint {
        let baseX = CGFloat(point.position.x) * containerSize.width
        let baseY = (1.0 - CGFloat(point.position.y)) * containerSize.height // Invert Y
        
        return CGPoint(
            x: baseX + panOffset.width,
            y: baseY + panOffset.height
        )
    }
    
    private var pointSize: CGFloat {
        let baseSize: CGFloat = 8
        let selectedSize: CGFloat = 12
        let hoveredSize: CGFloat = 10
        
        if isSelected {
            return selectedSize * CGFloat(zoomLevel)
        } else if isHovered {
            return hoveredSize * CGFloat(zoomLevel)
        } else {
            return baseSize * CGFloat(zoomLevel)
        }
    }
    
    var body: some View {
        Circle()
            .fill(point.category.color)
            .frame(width: pointSize, height: pointSize)
            .overlay(
                Circle()
                    .stroke(isSelected ? Color.white : Color.clear, lineWidth: 2)
            )
            .shadow(color: Color.black.opacity(0.3), radius: isSelected ? 3 : 1, x: 1, y: 1)
            .position(position)
            .onTapGesture {
                onSelect()
            }
            .onHover { hovering in
                if hovering {
                    onHover()
                }
            }
            .onDoubleClick {
                onDoubleClick()
            }
            .animation(.easeInOut(duration: 0.2), value: isSelected)
            .animation(.easeInOut(duration: 0.1), value: isHovered)
    }
}

// MARK: - Scatter Hover Info
struct ScatterHoverInfo: View {
    let point: SamplePoint
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(point.displayName)
                .font(.system(size: 11, weight: .bold))
                .foregroundColor(.primary)
            
            HStack(spacing: 8) {
                Text(point.category.name)
                    .font(.system(size: 9))
                    .foregroundColor(point.category.color)
                
                if let bpm = point.bpm {
                    Text("\(Int(bpm)) BPM")
                        .font(.system(size: 9))
                        .foregroundColor(.secondary)
                }
                
                if let key = point.key {
                    Text(key)
                        .font(.system(size: 9))
                        .foregroundColor(.secondary)
                }
            }
            
            Text("\(String(format: "%.1f", point.duration))s")
                .font(.system(size: 9))
                .foregroundColor(.secondary)
        }
        .padding(.horizontal, 8)
        .padding(.vertical, 6)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(Color.black.opacity(0.8))
        )
        .foregroundColor(.white)
    }
}

// MARK: - Selected Sample Info
struct SelectedSampleInfo: View {
    let sample: SamplePoint
    let onPreview: () -> Void
    let onSelect: () -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Selected Sample")
                .font(.system(size: 12, weight: .bold))
                .foregroundColor(.primary)
            
            VStack(alignment: .leading, spacing: 8) {
                Text(sample.name)
                    .font(.system(size: 12, weight: .medium))
                    .foregroundColor(.primary)
                    .lineLimit(2)
                
                HStack {
                    Image(systemName: sample.category.icon)
                        .font(.system(size: 10))
                        .foregroundColor(sample.category.color)
                    
                    Text(sample.category.name)
                        .font(.system(size: 10))
                        .foregroundColor(sample.category.color)
                    
                    Spacer()
                }
                
                HStack {
                    Text("\(String(format: "%.1f", sample.duration))s")
                        .font(.system(size: 10))
                        .foregroundColor(.secondary)
                    
                    if let bpm = sample.bpm {
                        Text("•")
                            .font(.system(size: 10))
                            .foregroundColor(.secondary)
                        Text("\(Int(bpm)) BPM")
                            .font(.system(size: 10))
                            .foregroundColor(.secondary)
                    }
                    
                    Spacer()
                }
                
                if let key = sample.key {
                    Text("Key: \(key)")
                        .font(.system(size: 10))
                        .foregroundColor(.secondary)
                }
                
                // Tags
                if !sample.tags.isEmpty {
                    ScrollView(.horizontal, showsIndicators: false) {
                        HStack(spacing: 4) {
                            ForEach(sample.tags.prefix(3), id: \.self) { tag in
                                Text(tag)
                                    .font(.system(size: 8))
                                    .foregroundColor(.secondary)
                                    .padding(.horizontal, 6)
                                    .padding(.vertical, 2)
                                    .background(
                                        RoundedRectangle(cornerRadius: 8)
                                            .fill(Color.gray.opacity(0.1))
                                    )
                            }
                        }
                    }
                }
            }
            .padding(.vertical, 8)
            .padding(.horizontal, 10)
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .fill(sample.category.color.opacity(0.05))
                    .stroke(sample.category.color.opacity(0.2), lineWidth: 1)
            )
            
            // Actions
            VStack(spacing: 8) {
                Button(action: onPreview) {
                    HStack(spacing: 4) {
                        Image(systemName: "speaker.wave.2")
                            .font(.system(size: 12))
                        Text("Preview")
                            .font(.system(size: 12, weight: .medium))
                    }
                    .foregroundColor(.blue)
                    .frame(maxWidth: .infinity)
                    .padding(.vertical, 8)
                    .background(
                        RoundedRectangle(cornerRadius: 8)
                            .fill(Color.blue.opacity(0.1))
                    )
                }
                .buttonStyle(PlainButtonStyle())
                
                Button(action: onSelect) {
                    HStack(spacing: 4) {
                        Image(systemName: "checkmark.circle")
                            .font(.system(size: 12))
                        Text("Select")
                            .font(.system(size: 12, weight: .medium))
                    }
                    .foregroundColor(.white)
                    .frame(maxWidth: .infinity)
                    .padding(.vertical, 8)
                    .background(
                        RoundedRectangle(cornerRadius: 8)
                            .fill(sample.category.color)
                    )
                }
                .buttonStyle(PlainButtonStyle())
            }
        }
    }
}

// MARK: - Scatter Stats
struct ScatterStats: View {
    let totalSamples: Int
    let visibleSamples: Int
    let selectedCategories: Int
    let analysis: ScatterAnalysis
    
    var body: some View {
        HStack {
            if analysis.isAnalyzing {
                HStack(spacing: 8) {
                    ProgressView()
                        .controlSize(.mini)
                    
                    Text("Analyzing: \(analysis.currentSample)")
                        .font(.system(size: 10))
                        .foregroundColor(.secondary)
                    
                    Text("\(analysis.analyzedSamples)/\(analysis.totalSamples)")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.blue)
                }
            } else {
                Text("Showing \(visibleSamples) of \(totalSamples) samples")
                    .font(.system(size: 10))
                    .foregroundColor(.secondary)
                
                Spacer()
                
                Text("\(selectedCategories) categories")
                    .font(.system(size: 10))
                    .foregroundColor(.secondary)
            }
        }
    }
}

// MARK: - Analysis Settings Sheet
struct AnalysisSettings {
    var analyzeTimbre: Bool = true
    var analyzeEnergy: Bool = true
    var analyzeTempo: Bool = true
    var analyzeKey: Bool = true
    var audioPreview: Bool = true
}

struct AnalysisSettingsSheet: View {
    let onStartAnalysis: (AnalysisSettings) -> Void
    
    @State private var settings = AnalysisSettings()
    @Environment(\.presentationMode) var presentationMode
    
    var body: some View {
        VStack(spacing: 20) {
            Text("Analysis Settings")
                .font(.headline)
            
            VStack(alignment: .leading, spacing: 12) {
                Toggle("Timbre Analysis", isOn: $settings.analyzeTimbre)
                Toggle("Energy Analysis", isOn: $settings.analyzeEnergy)
                Toggle("Tempo Detection", isOn: $settings.analyzeTempo)
                Toggle("Key Detection", isOn: $settings.analyzeKey)
                Toggle("Audio Preview", isOn: $settings.audioPreview)
            }
            
            HStack(spacing: 12) {
                Button("Cancel") {
                    presentationMode.wrappedValue.dismiss()
                }
                
                Button("Start Analysis") {
                    onStartAnalysis(settings)
                    presentationMode.wrappedValue.dismiss()
                }
                .buttonStyle(.borderedProminent)
            }
        }
        .padding()
        .frame(width: 300, height: 250)
    }
}

// MARK: - Double Click Gesture Extension
extension View {
    func onDoubleClick(perform action: @escaping () -> Void) -> some View {
        self.onTapGesture(count: 2, perform: action)
    }
}