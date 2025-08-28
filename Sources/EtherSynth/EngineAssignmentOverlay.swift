import SwiftUI

/**
 * Engine Assignment Overlay - Shows all available synthesis engines
 * Organized by category for easy selection and assignment to tracks
 */

// MARK: - Engine Category Model
struct EngineCategory {
    let name: String
    let engines: [EngineInfo]
    let icon: String
    let color: Color
}

// Removed duplicate EngineMetrics - using the one from InstrumentBrowserViewModel

struct EngineInfo {
    let id: Int
    let type: Int
    let name: String
    let shortDescription: String
    let icon: String
    let heroParameters: [String]
    let category: String
    let cpuUsage: Float
    let hasPresets: Bool
    
    init(type: Int, name: String, shortDescription: String, icon: String, heroParameters: [String] = [], category: String = "Synth", cpuUsage: Float = 0.0, hasPresets: Bool = true) {
        self.id = type
        self.type = type
        self.name = name
        self.shortDescription = shortDescription
        self.icon = icon
        self.heroParameters = heroParameters
        self.category = category
        self.cpuUsage = cpuUsage
        self.hasPresets = hasPresets
    }
}

// MARK: - Main Engine Assignment Overlay
struct EngineAssignmentOverlay: View {
    let trackId: Int
    let trackColor: InstrumentColor
    let currentEngineType: Int?
    let onEngineSelect: (Int) -> Void
    let onClose: () -> Void
    
    @State private var selectedCategory: String = "Synthesizers"
    @State private var selectedEngineType: Int? = nil
    
    private let categories: [EngineCategory] = [
        EngineCategory(
            name: "Synthesizers",
            engines: [
                EngineInfo(type: 0, name: "MacroVA", shortDescription: "Virtual analog with classic filters", icon: "waveform", heroParameters: ["Harmonics", "Timbre", "Morph"]),
                EngineInfo(type: 1, name: "MacroFM", shortDescription: "2-operator FM synthesis", icon: "waveform.path", heroParameters: ["Harmonics", "Timbre", "Morph"]),
                EngineInfo(type: 2, name: "MacroWaveshaper", shortDescription: "Harmonic waveshaping and folding", icon: "waveform.path.badge.plus", heroParameters: ["Harmonics", "Timbre", "Morph"]),
                EngineInfo(type: 3, name: "MacroWavetable", shortDescription: "Morphing wavetable synthesis", icon: "waveform.and.magnifyingglass", heroParameters: ["Harmonics", "Timbre", "Morph"]),
                EngineInfo(type: 5, name: "MacroHarmonics", shortDescription: "Additive harmonic synthesis", icon: "waveform.badge.plus", heroParameters: ["Harmonics", "Timbre", "Morph"])
            ],
            icon: "waveform",
            color: Color.blue
        ),
        EngineCategory(
            name: "Multi-Voice",
            engines: [
                EngineInfo(type: 4, name: "MacroChord", shortDescription: "5-voice chord machine with per-voice engines", icon: "music.note.house", heroParameters: ["Harmonics", "Timbre", "Morph"])
            ],
            icon: "music.note.house",
            color: Color.purple
        ),
        EngineCategory(
            name: "Textures",
            engines: [
                EngineInfo(type: 6, name: "FormantVocal", shortDescription: "Vocal formant synthesis", icon: "person.wave.2", heroParameters: ["Harmonics", "Timbre", "Morph"]),
                EngineInfo(type: 7, name: "NoiseParticles", shortDescription: "Granular noise and particle synthesis", icon: "sparkles", heroParameters: ["Harmonics", "Timbre", "Morph"])
            ],
            icon: "person.wave.2",
            color: Color.orange
        ),
        EngineCategory(
            name: "Physical Models",
            engines: [
                EngineInfo(type: 8, name: "TidesOsc", shortDescription: "Mutable Instruments Tides slope oscillator", icon: "triangle", heroParameters: ["Harmonics", "Timbre", "Morph"]),
                EngineInfo(type: 9, name: "RingsVoice", shortDescription: "Mutable Instruments Rings modal resonator", icon: "circle.dotted", heroParameters: ["Harmonics", "Timbre", "Morph"]),
                EngineInfo(type: 10, name: "ElementsVoice", shortDescription: "Mutable Instruments Elements physical model", icon: "cube.transparent", heroParameters: ["Harmonics", "Timbre", "Morph"])
            ],
            icon: "triangle",
            color: Color.green
        ),
        EngineCategory(
            name: "Percussion & Samples",
            engines: [
                EngineInfo(type: 11, name: "DrumKit", shortDescription: "12-slot drum machine with synthesis models", icon: "dot.radiowaves.left.and.right", heroParameters: ["Level", "Tune", "Snap"]),
                EngineInfo(type: 12, name: "SamplerKit", shortDescription: "25-pad MPC-style sampler", icon: "square.grid.3x3", heroParameters: ["Level", "Pitch", "Filter"]),
                EngineInfo(type: 13, name: "SamplerSlicer", shortDescription: "Loop slicer with up to 25 slices", icon: "waveform.path.ecg", heroParameters: ["Level", "Pitch", "Slice"])
            ],
            icon: "dot.radiowaves.left.and.right",
            color: Color.red
        )
    ]
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            EngineAssignmentHeader(
                trackId: trackId,
                trackColor: trackColor,
                currentEngineType: currentEngineType,
                onClose: onClose
            )
            .padding(.horizontal, 20)
            .padding(.vertical, 16)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(trackColor.pastelColor.opacity(0.1))
            )
            
            // Category Tabs
            CategoryTabs(
                categories: categories,
                selectedCategory: $selectedCategory
            )
            .padding(.horizontal, 20)
            .padding(.vertical, 12)
            
            // Engine Grid
            if let category = categories.first(where: { $0.name == selectedCategory }) {
                EngineGrid(
                    category: category,
                    selectedEngineType: $selectedEngineType,
                    currentEngineType: currentEngineType,
                    onEngineSelect: { engineType in
                        selectedEngineType = engineType
                    }
                )
                .padding(.horizontal, 20)
                .padding(.vertical, 12)
            }
            
            Spacer()
            
            // Action Buttons
            ActionButtons(
                selectedEngineType: selectedEngineType,
                onAssign: {
                    if let engineType = selectedEngineType {
                        onEngineSelect(engineType)
                        onClose()
                    }
                },
                onCancel: onClose
            )
            .padding(.horizontal, 20)
            .padding(.bottom, 20)
        }
        .frame(width: 640, height: 480)
        .background(
            RoundedRectangle(cornerRadius: 16)
                .fill(Color.white)
                .shadow(color: Color.black.opacity(0.3), radius: 12, x: 0, y: 8)
        )
        .onAppear {
            selectedEngineType = currentEngineType
        }
    }
}

// MARK: - Engine Assignment Header
struct EngineAssignmentHeader: View {
    let trackId: Int
    let trackColor: InstrumentColor
    let currentEngineType: Int?
    let onClose: () -> Void
    
    var body: some View {
        HStack {
            // Track Info
            HStack(spacing: 12) {
                Circle()
                    .fill(trackColor.pastelColor)
                    .frame(width: 32, height: 32)
                    .overlay(
                        Text("\(trackId + 1)")
                            .font(.system(size: 14, weight: .bold))
                            .foregroundColor(.white)
                    )
                
                VStack(alignment: .leading, spacing: 2) {
                    Text("Track \(trackId + 1) • \(trackColor.rawValue)")
                        .font(.system(size: 16, weight: .bold))
                        .foregroundColor(.primary)
                    
                    if let engineType = currentEngineType {
                        Text("Currently: Engine Type \(engineType)")
                            .font(.system(size: 12))
                            .foregroundColor(.secondary)
                    } else {
                        Text("No engine assigned")
                            .font(.system(size: 12))
                            .foregroundColor(.secondary)
                    }
                }
            }
            
            Spacer()
            
            // Close Button
            Button(action: onClose) {
                Image(systemName: "xmark.circle.fill")
                    .font(.system(size: 20))
                    .foregroundColor(.secondary)
            }
            .buttonStyle(PlainButtonStyle())
        }
    }
}

// MARK: - Category Tabs
struct CategoryTabs: View {
    let categories: [EngineCategory]
    @Binding var selectedCategory: String
    
    var body: some View {
        ScrollView(.horizontal, showsIndicators: false) {
            HStack(spacing: 8) {
                ForEach(categories, id: \.name) { category in
                    Button(action: {
                        selectedCategory = category.name
                    }) {
                        HStack(spacing: 6) {
                            Image(systemName: category.icon)
                                .font(.system(size: 12))
                                .foregroundColor(selectedCategory == category.name ? .white : category.color)
                            
                            Text(category.name)
                                .font(.system(size: 12, weight: .medium))
                                .foregroundColor(selectedCategory == category.name ? .white : .primary)
                            
                            Text("(\(category.engines.count))")
                                .font(.system(size: 10))
                                .foregroundColor(selectedCategory == category.name ? .white.opacity(0.8) : .secondary)
                        }
                        .padding(.horizontal, 12)
                        .padding(.vertical, 8)
                        .background(
                            RoundedRectangle(cornerRadius: 20)
                                .fill(selectedCategory == category.name ? category.color : Color.gray.opacity(0.1))
                                .shadow(color: Color.black.opacity(0.1), radius: 2, x: 1, y: 1)
                        )
                    }
                    .buttonStyle(PlainButtonStyle())
                }
            }
            .padding(.horizontal, 4)
        }
    }
}

// MARK: - Engine Grid
struct EngineGrid: View {
    let category: EngineCategory
    @Binding var selectedEngineType: Int?
    let currentEngineType: Int?
    let onEngineSelect: (Int) -> Void
    
    var body: some View {
        ScrollView {
            LazyVGrid(columns: [
                GridItem(.flexible()),
                GridItem(.flexible()),
                GridItem(.flexible())
            ], spacing: 16) {
                ForEach(category.engines, id: \.type) { engine in
                    EngineCard(
                        engine: engine,
                        isSelected: selectedEngineType == engine.type,
                        isCurrent: currentEngineType == engine.type,
                        categoryColor: category.color,
                        onSelect: { onEngineSelect(engine.type) }
                    )
                }
            }
            .padding(.vertical, 8)
        }
    }
}

// MARK: - Engine Card
struct EngineCard: View {
    let engine: EngineInfo
    let isSelected: Bool
    let isCurrent: Bool
    let categoryColor: Color
    let onSelect: () -> Void
    
    var body: some View {
        Button(action: onSelect) {
            VStack(spacing: 12) {
                // Engine Icon
                ZStack {
                    RoundedRectangle(cornerRadius: 12)
                        .fill(categoryColor.opacity(0.1))
                        .frame(height: 60)
                    
                    Image(systemName: engine.icon)
                        .font(.system(size: 24))
                        .foregroundColor(categoryColor)
                }
                
                // Engine Info
                VStack(spacing: 4) {
                    Text(engine.name)
                        .font(.system(size: 14, weight: .bold))
                        .foregroundColor(.primary)
                        .multilineTextAlignment(.center)
                    
                    Text(engine.shortDescription)
                        .font(.system(size: 10))
                        .foregroundColor(.secondary)
                        .multilineTextAlignment(.center)
                        .lineLimit(2)
                        .fixedSize(horizontal: false, vertical: true)
                }
                
                // Hero Parameters
                HStack(spacing: 4) {
                    ForEach(engine.heroParameters.prefix(3), id: \.self) { param in
                        Text(param)
                            .font(.system(size: 8, weight: .medium))
                            .foregroundColor(categoryColor)
                            .padding(.horizontal, 6)
                            .padding(.vertical, 2)
                            .background(
                                RoundedRectangle(cornerRadius: 8)
                                    .fill(categoryColor.opacity(0.1))
                            )
                    }
                }
                
                // Current Engine Indicator
                if isCurrent {
                    HStack(spacing: 4) {
                        Image(systemName: "checkmark.circle.fill")
                            .font(.system(size: 10))
                            .foregroundColor(.green)
                        
                        Text("Current")
                            .font(.system(size: 8, weight: .medium))
                            .foregroundColor(.green)
                    }
                }
            }
            .padding(12)
            .frame(maxWidth: .infinity, minHeight: 160)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(Color.white)
                    .overlay(
                        RoundedRectangle(cornerRadius: 12)
                            .stroke(
                                isSelected ? categoryColor : Color.gray.opacity(0.2),
                                lineWidth: isSelected ? 2 : 1
                            )
                    )
                    .shadow(color: Color.black.opacity(isSelected ? 0.2 : 0.1), radius: isSelected ? 4 : 2, x: 1, y: 1)
            )
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - Action Buttons
struct ActionButtons: View {
    let selectedEngineType: Int?
    let onAssign: () -> Void
    let onCancel: () -> Void
    
    var body: some View {
        HStack(spacing: 12) {
            // Cancel
            Button(action: onCancel) {
                Text("Cancel")
                    .font(.system(size: 14, weight: .medium))
                    .foregroundColor(.secondary)
                    .frame(maxWidth: .infinity)
                    .padding(.vertical, 12)
                    .background(
                        RoundedRectangle(cornerRadius: 8)
                            .fill(Color.gray.opacity(0.1))
                            .shadow(color: Color.black.opacity(0.1), radius: 2, x: 1, y: 1)
                    )
            }
            .buttonStyle(PlainButtonStyle())
            
            // Assign Engine
            Button(action: onAssign) {
                HStack(spacing: 6) {
                    Image(systemName: "plus.circle.fill")
                        .font(.system(size: 12))
                    
                    Text(selectedEngineType != nil ? "Assign Engine" : "Select Engine")
                        .font(.system(size: 14, weight: .medium))
                }
                .foregroundColor(.white)
                .frame(maxWidth: .infinity)
                .padding(.vertical, 12)
                .background(
                    RoundedRectangle(cornerRadius: 8)
                        .fill(selectedEngineType != nil ? Color.blue : Color.gray)
                        .shadow(color: Color.black.opacity(0.2), radius: 3, x: 1, y: 1)
                )
            }
            .buttonStyle(PlainButtonStyle())
            .disabled(selectedEngineType == nil)
        }
    }
}

// MARK: - Preview Provider
struct EngineAssignmentOverlay_Previews: PreviewProvider {
    static var previews: some View {
        EngineAssignmentOverlay(
            trackId: 0,
            trackColor: .coral,
            currentEngineType: nil,
            onEngineSelect: { _ in },
            onClose: { }
        )
        .preferredColorScheme(.light)
    }
}