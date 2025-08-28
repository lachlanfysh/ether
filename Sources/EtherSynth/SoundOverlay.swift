import SwiftUI

/**
 * Sound Overlay - Deep synthesis control for each engine
 * Shows hero macro parameters and engine-specific controls
 */

// MARK: - Sound Overlay Models
struct HeroParameter {
    let id: String
    let name: String
    let shortName: String
    let value: Float
    let range: ClosedRange<Float>
    let curve: ParameterCurve
    let unit: String
    let color: Color
    
    enum ParameterCurve {
        case linear
        case exponential
        case logarithmic
    }
}

struct EnginePreset {
    let name: String
    let description: String
    let parameters: [String: Float] // Parameter ID -> Value
    let category: String
}

// MARK: - Main Sound Overlay
struct SoundOverlay: View {
    let engineType: Int
    let engineName: String
    let trackColor: InstrumentColor
    
    @EnvironmentObject var synthController: SynthController
    @State private var heroParameters: [HeroParameter] = []
    @State private var selectedPreset: String? = nil
    @State private var showingPresets: Bool = false
    @State private var showingAdvanced: Bool = false
    @Environment(\.presentationMode) var presentationMode
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            SoundOverlayHeader(
                engineName: engineName,
                trackColor: trackColor,
                onClose: { presentationMode.wrappedValue.dismiss() },
                onPresets: { showingPresets = true },
                onAdvanced: { showingAdvanced.toggle() }
            )
            .padding(.horizontal, 20)
            .padding(.vertical, 16)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(trackColor.pastelColor.opacity(0.1))
            )
            
            // Hero Macro Controls
            HeroMacroSection(
                heroParameters: $heroParameters,
                trackColor: trackColor,
                onParameterChange: { paramId, value in
                    updateParameter(paramId: paramId, value: value)
                }
            )
            .padding(.horizontal, 20)
            .padding(.vertical, 16)
            
            if showingAdvanced {
                // Advanced Parameters
                AdvancedParametersSection(
                    engineType: engineType,
                    trackColor: trackColor,
                    onParameterChange: { paramId, value in
                        updateParameter(paramId: paramId, value: value)
                    }
                )
                .padding(.horizontal, 20)
                .padding(.vertical, 16)
                .transition(.opacity.combined(with: .move(edge: .top)))
            }
            
            Spacer()
            
            // Performance Info
            PerformanceInfo(engineType: engineType)
                .padding(.horizontal, 20)
                .padding(.bottom, 20)
        }
        .frame(width: 720, height: showingAdvanced ? 640 : 480)
        .background(
            RoundedRectangle(cornerRadius: 16)
                .fill(Color.white)
                .shadow(color: Color.black.opacity(0.3), radius: 12, x: 0, y: 8)
        )
        .sheet(isPresented: $showingPresets) {
            PresetBrowser(
                engineType: engineType,
                trackColor: trackColor,
                onPresetSelect: { preset in
                    loadPreset(preset)
                }
            )
        }
        .onAppear {
            initializeHeroParameters()
        }
        .animation(.easeInOut(duration: 0.3), value: showingAdvanced)
    }
    
    // MARK: - Helper Functions
    private func initializeHeroParameters() {
        heroParameters = getHeroParametersForEngine(engineType: engineType)
    }
    
    private func updateParameter(paramId: String, value: Float) {
        // Update synth controller with new parameter value
        synthController.setParameter(paramId: paramId, value: value)
    }
    
    private func loadPreset(_ preset: EnginePreset) {
        // Load preset parameters
        for (paramId, value) in preset.parameters {
            updateParameter(paramId: paramId, value: value)
        }
        showingPresets = false
    }
    
    private func getHeroParametersForEngine(engineType: Int) -> [HeroParameter] {
        switch engineType {
        case 0...5, 8...10: // Synthesis engines
            return [
                HeroParameter(
                    id: "harmonics",
                    name: "Harmonics",
                    shortName: "HARM",
                    value: 0.5,
                    range: 0.0...1.0,
                    curve: .linear,
                    unit: "",
                    color: trackColor.pastelColor
                ),
                HeroParameter(
                    id: "timbre",
                    name: "Timbre",
                    shortName: "TMBR",
                    value: 0.5,
                    range: 0.0...1.0,
                    curve: .linear,
                    unit: "",
                    color: trackColor.pastelColor.opacity(0.8)
                ),
                HeroParameter(
                    id: "morph",
                    name: "Morph",
                    shortName: "MRPH",
                    value: 0.0,
                    range: 0.0...1.0,
                    curve: .linear,
                    unit: "",
                    color: trackColor.pastelColor.opacity(0.6)
                )
            ]
        case 6, 7: // Texture engines
            return [
                HeroParameter(
                    id: "density",
                    name: "Density",
                    shortName: "DENS",
                    value: 0.5,
                    range: 0.0...1.0,
                    curve: .exponential,
                    unit: "",
                    color: trackColor.pastelColor
                ),
                HeroParameter(
                    id: "texture",
                    name: "Texture",
                    shortName: "TEXT",
                    value: 0.5,
                    range: 0.0...1.0,
                    curve: .linear,
                    unit: "",
                    color: trackColor.pastelColor.opacity(0.8)
                ),
                HeroParameter(
                    id: "character",
                    name: "Character",
                    shortName: "CHAR",
                    value: 0.0,
                    range: 0.0...1.0,
                    curve: .linear,
                    unit: "",
                    color: trackColor.pastelColor.opacity(0.6)
                )
            ]
        case 11: // DrumKit
            return [
                HeroParameter(
                    id: "level",
                    name: "Level",
                    shortName: "LVL",
                    value: 0.8,
                    range: 0.0...1.0,
                    curve: .linear,
                    unit: "",
                    color: trackColor.pastelColor
                ),
                HeroParameter(
                    id: "tune",
                    name: "Tune",
                    shortName: "TUNE",
                    value: 0.5,
                    range: -1.0...1.0,
                    curve: .linear,
                    unit: "st",
                    color: trackColor.pastelColor.opacity(0.8)
                ),
                HeroParameter(
                    id: "snap",
                    name: "Snap",
                    shortName: "SNAP",
                    value: 0.3,
                    range: 0.0...1.0,
                    curve: .exponential,
                    unit: "",
                    color: trackColor.pastelColor.opacity(0.6)
                )
            ]
        case 12, 13: // Samplers
            return [
                HeroParameter(
                    id: "level",
                    name: "Level",
                    shortName: "LVL",
                    value: 0.8,
                    range: 0.0...1.0,
                    curve: .linear,
                    unit: "",
                    color: trackColor.pastelColor
                ),
                HeroParameter(
                    id: "pitch",
                    name: "Pitch",
                    shortName: "PTCH",
                    value: 0.5,
                    range: 0.0...1.0,
                    curve: .linear,
                    unit: "st",
                    color: trackColor.pastelColor.opacity(0.8)
                ),
                HeroParameter(
                    id: "filter",
                    name: "Filter",
                    shortName: "FILT",
                    value: 0.7,
                    range: 0.0...1.0,
                    curve: .exponential,
                    unit: "Hz",
                    color: trackColor.pastelColor.opacity(0.6)
                )
            ]
        default:
            return []
        }
    }
}

// MARK: - Sound Overlay Header
struct SoundOverlayHeader: View {
    let engineName: String
    let trackColor: InstrumentColor
    let onClose: () -> Void
    let onPresets: () -> Void
    let onAdvanced: () -> Void
    
    var body: some View {
        HStack {
            // Engine Info
            HStack(spacing: 12) {
                Circle()
                    .fill(trackColor.pastelColor)
                    .frame(width: 40, height: 40)
                    .overlay(
                        Image(systemName: "waveform")
                            .font(.system(size: 18))
                            .foregroundColor(.white)
                    )
                
                VStack(alignment: .leading, spacing: 2) {
                    Text(engineName)
                        .font(.system(size: 18, weight: .bold))
                        .foregroundColor(.primary)
                    
                    Text(trackColor.rawValue)
                        .font(.system(size: 14))
                        .foregroundColor(.secondary)
                }
            }
            
            Spacer()
            
            // Action Buttons
            HStack(spacing: 8) {
                Button(action: onPresets) {
                    HStack(spacing: 4) {
                        Image(systemName: "folder")
                            .font(.system(size: 12))
                        Text("Presets")
                            .font(.system(size: 12, weight: .medium))
                    }
                    .foregroundColor(.primary)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 6)
                    .background(
                        RoundedRectangle(cornerRadius: 16)
                            .fill(Color.white)
                            .shadow(color: Color.black.opacity(0.1), radius: 2, x: 1, y: 1)
                    )
                }
                .buttonStyle(PlainButtonStyle())
                
                Button(action: onAdvanced) {
                    HStack(spacing: 4) {
                        Image(systemName: "gearshape")
                            .font(.system(size: 12))
                        Text("Advanced")
                            .font(.system(size: 12, weight: .medium))
                    }
                    .foregroundColor(.primary)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 6)
                    .background(
                        RoundedRectangle(cornerRadius: 16)
                            .fill(Color.white)
                            .shadow(color: Color.black.opacity(0.1), radius: 2, x: 1, y: 1)
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

// MARK: - Hero Macro Section
struct HeroMacroSection: View {
    @Binding var heroParameters: [HeroParameter]
    let trackColor: InstrumentColor
    let onParameterChange: (String, Float) -> Void
    
    var body: some View {
        VStack(spacing: 20) {
            Text("Hero Macros")
                .font(.system(size: 16, weight: .bold))
                .foregroundColor(.primary)
            
            HStack(spacing: 32) {
                ForEach(Array(heroParameters.enumerated()), id: \.element.id) { index, parameter in
                    HeroMacroKnob(
                        parameter: parameter,
                        onValueChange: { newValue in
                            heroParameters[index] = HeroParameter(
                                id: parameter.id,
                                name: parameter.name,
                                shortName: parameter.shortName,
                                value: newValue,
                                range: parameter.range,
                                curve: parameter.curve,
                                unit: parameter.unit,
                                color: parameter.color
                            )
                            onParameterChange(parameter.id, newValue)
                        }
                    )
                }
            }
        }
        .padding(.vertical, 20)
        .background(
            RoundedRectangle(cornerRadius: 12)
                .fill(trackColor.pastelColor.opacity(0.05))
        )
    }
}

// MARK: - Hero Macro Knob
struct HeroMacroKnob: View {
    let parameter: HeroParameter
    let onValueChange: (Float) -> Void
    
    @State private var isDragging: Bool = false
    @State private var dragOffset: CGFloat = 0
    
    private let knobSize: CGFloat = 80
    private let sensitivity: Float = 0.01
    
    var body: some View {
        VStack(spacing: 8) {
            // Knob
            ZStack {
                // Background circle
                Circle()
                    .fill(Color.white)
                    .frame(width: knobSize, height: knobSize)
                    .shadow(color: Color.black.opacity(0.2), radius: 4, x: 2, y: 2)
                
                // Value arc
                Circle()
                    .trim(from: 0, to: CGFloat(normalizedValue))
                    .stroke(
                        parameter.color,
                        style: StrokeStyle(lineWidth: 4, lineCap: .round)
                    )
                    .frame(width: knobSize - 8, height: knobSize - 8)
                    .rotationEffect(.degrees(-90))
                
                // Center dot
                Circle()
                    .fill(parameter.color)
                    .frame(width: 8, height: 8)
                
                // Value indicator
                Rectangle()
                    .fill(parameter.color)
                    .frame(width: 2, height: 16)
                    .offset(y: -knobSize/2 + 12)
                    .rotationEffect(.degrees(Double(normalizedValue * 270 - 135)))
            }
            .gesture(
                DragGesture()
                    .onChanged { value in
                        if !isDragging {
                            isDragging = true
                            dragOffset = 0
                        }
                        
                        let delta = Float(value.translation.y - dragOffset) * -sensitivity
                        let newValue = clamp(parameter.value + delta, to: parameter.range)
                        onValueChange(newValue)
                        dragOffset = value.translation.y
                    }
                    .onEnded { _ in
                        isDragging = false
                        dragOffset = 0
                    }
            )
            
            // Parameter name
            Text(parameter.shortName)
                .font(.system(size: 12, weight: .bold))
                .foregroundColor(.primary)
            
            // Value display
            Text(valueString)
                .font(.system(size: 10, weight: .medium))
                .foregroundColor(.secondary)
                .frame(minWidth: 40)
        }
    }
    
    private var normalizedValue: Float {
        (parameter.value - parameter.range.lowerBound) / (parameter.range.upperBound - parameter.range.lowerBound)
    }
    
    private var valueString: String {
        let value = parameter.value
        if parameter.unit.isEmpty {
            return String(format: "%.0f", value * 100)
        } else {
            switch parameter.unit {
            case "st":
                return String(format: "%.1f%@", (value - 0.5) * 24, parameter.unit)
            case "Hz":
                let freq = 20 + value * 20000
                return freq < 1000 ? String(format: "%.0fHz", freq) : String(format: "%.1fkHz", freq/1000)
            default:
                return String(format: "%.2f%@", value, parameter.unit)
            }
        }
    }
    
    private func clamp(_ value: Float, to range: ClosedRange<Float>) -> Float {
        return Swift.min(Swift.max(value, range.lowerBound), range.upperBound)
    }
}

// MARK: - Advanced Parameters Section
struct AdvancedParametersSection: View {
    let engineType: Int
    let trackColor: InstrumentColor
    let onParameterChange: (String, Float) -> Void
    
    private let advancedParams = [
        "Filter Cutoff", "Filter Resonance", "Attack", "Decay", 
        "Sustain", "Release", "LFO Rate", "LFO Depth"
    ]
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Advanced Parameters")
                .font(.system(size: 14, weight: .bold))
                .foregroundColor(.primary)
            
            LazyVGrid(columns: [
                GridItem(.flexible()),
                GridItem(.flexible()),
                GridItem(.flexible()),
                GridItem(.flexible())
            ], spacing: 12) {
                ForEach(advancedParams, id: \.self) { paramName in
                    AdvancedParameterSlider(
                        name: paramName,
                        value: 0.5,
                        color: trackColor.pastelColor,
                        onValueChange: { value in
                            onParameterChange(paramName.lowercased().replacingOccurrences(of: " ", with: "_"), value)
                        }
                    )
                }
            }
        }
        .padding(.vertical, 16)
        .background(
            RoundedRectangle(cornerRadius: 12)
                .fill(Color.gray.opacity(0.05))
        )
    }
}

// MARK: - Advanced Parameter Slider
struct AdvancedParameterSlider: View {
    let name: String
    @State var value: Float
    let color: Color
    let onValueChange: (Float) -> Void
    
    var body: some View {
        VStack(spacing: 4) {
            Text(name)
                .font(.system(size: 10, weight: .medium))
                .foregroundColor(.primary)
                .lineLimit(1)
            
            VStack(spacing: 2) {
                Slider(value: Binding(
                    get: { Double(value) },
                    set: { newValue in
                        value = Float(newValue)
                        onValueChange(value)
                    }
                ), in: 0...1) {
                    EmptyView()
                } minimumValueLabel: {
                    EmptyView()
                } maximumValueLabel: {
                    EmptyView()
                }
                .accentColor(color)
                .controlSize(.mini)
                
                Text(String(format: "%.0f", value * 100))
                    .font(.system(size: 8))
                    .foregroundColor(.secondary)
            }
        }
        .frame(width: 80)
    }
}

// MARK: - Performance Info
struct PerformanceInfo: View {
    let engineType: Int
    
    var body: some View {
        HStack {
            HStack(spacing: 12) {
                VStack(alignment: .leading, spacing: 2) {
                    Text("CPU")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.secondary)
                    Text("12%")
                        .font(.system(size: 12, weight: .bold))
                        .foregroundColor(.primary)
                }
                
                VStack(alignment: .leading, spacing: 2) {
                    Text("Voices")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.secondary)
                    Text("3")
                        .font(.system(size: 12, weight: .bold))
                        .foregroundColor(.primary)
                }
                
                VStack(alignment: .leading, spacing: 2) {
                    Text("Polyphony")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.secondary)
                    Text("16")
                        .font(.system(size: 12, weight: .bold))
                        .foregroundColor(.primary)
                }
            }
            
            Spacer()
            
            Text("Real-time safe")
                .font(.system(size: 10, weight: .medium))
                .foregroundColor(.green)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(
                    RoundedRectangle(cornerRadius: 8)
                        .fill(Color.green.opacity(0.1))
                )
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 12)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(Color.gray.opacity(0.05))
        )
    }
}

// MARK: - Preset Browser (Placeholder)
struct PresetBrowser: View {
    let engineType: Int
    let trackColor: InstrumentColor
    let onPresetSelect: (EnginePreset) -> Void
    
    var body: some View {
        VStack {
            Text("Preset Browser")
                .font(.headline)
            
            Text("Preset browser for \(engineType) would go here")
                .foregroundColor(.secondary)
            
            Button("Close") {
                // Close preset browser
            }
        }
        .frame(width: 480, height: 360)
        .padding()
    }
}