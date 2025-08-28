import SwiftUI

/**
 * SamplerKit UI - 5x5 pad grid with MPC-style interface
 * Includes pad inspector, velocity sensitivity, and sample management
 */

// MARK: - SamplerKit Models
struct SamplerPad {
    let id: Int
    var sampleName: String?
    var samplePath: String?
    var volume: Float = 0.8
    var pitch: Float = 0.0 // -24 to +24 semitones
    var pan: Float = 0.0 // -1 to +1
    var attack: Float = 0.0
    var decay: Float = 0.3
    var filterCutoff: Float = 1.0
    var filterResonance: Float = 0.0
    var chokeGroup: Int = 0 // 0 = no choke
    var reverse: Bool = false
    var oneShot: Bool = true
    var loopMode: Bool = false
    var startOffset: Float = 0.0
    var endOffset: Float = 1.0
    
    var isEmpty: Bool {
        return sampleName == nil
    }
    
    var displayName: String {
        return sampleName ?? "Empty"
    }
}

// MARK: - Main SamplerKit UI
struct SamplerKitUI: View {
    let trackId: Int
    let trackColor: InstrumentColor
    
    @Environment(\.presentationMode) var presentationMode
    @State private var pads: [SamplerPad] = []
    @State private var selectedPad: Int? = nil
    @State private var showingPadInspector: Bool = false
    @State private var showingSampleBrowser: Bool = false
    @State private var velocitySensitivity: Float = 0.8
    @State private var masterVolume: Float = 0.9
    @State private var playingPads: Set<Int> = []
    
    // Grid layout - 5x5 = 25 pads (MPC style)
    private let gridSize = 5
    private let padSize: CGFloat = 80
    
    var body: some View {
        HStack(spacing: 0) {
            // Main Pad Grid
            VStack(spacing: 0) {
                // Header
                SamplerKitHeader(
                    trackId: trackId,
                    trackColor: trackColor,
                    masterVolume: $masterVolume,
                    velocitySensitivity: $velocitySensitivity,
                    onClose: { presentationMode.wrappedValue.dismiss() }
                )
                .padding(.horizontal, 20)
                .padding(.vertical, 16)
                .background(
                    RoundedRectangle(cornerRadius: 12)
                        .fill(trackColor.pastelColor.opacity(0.1))
                )
                
                Spacer()
                
                // 5x5 Pad Grid
                VStack(spacing: 8) {
                    ForEach(0..<gridSize, id: \.self) { row in
                        HStack(spacing: 8) {
                            ForEach(0..<gridSize, id: \.self) { col in
                                let padIndex = row * gridSize + col
                                
                                SamplerPadView(
                                    pad: pads[padIndex],
                                    isSelected: selectedPad == padIndex,
                                    isPlaying: playingPads.contains(padIndex),
                                    trackColor: trackColor,
                                    onTap: { selectPad(padIndex) },
                                    onPlay: { playPad(padIndex) },
                                    onLongPress: { 
                                        selectedPad = padIndex
                                        showingPadInspector = true
                                    }
                                )
                                .frame(width: padSize, height: padSize)
                            }
                        }
                    }
                }
                .padding(.horizontal, 20)
                
                Spacer()
                
                // Quick Actions
                SamplerQuickActions(
                    selectedPad: selectedPad,
                    onLoadSample: { 
                        showingSampleBrowser = true
                    },
                    onClearPad: { clearSelectedPad() },
                    onCopyPad: { copySelectedPad() },
                    onPastePad: { pasteSelectedPad() },
                    onInspector: { 
                        if selectedPad != nil {
                            showingPadInspector = true
                        }
                    }
                )
                .padding(.horizontal, 20)
                .padding(.bottom, 20)
            }
            .frame(width: 520)
            
            // Pad Inspector (Side Panel)
            if showingPadInspector, let padIndex = selectedPad {
                PadInspectorPanel(
                    pad: $pads[padIndex],
                    trackColor: trackColor,
                    onClose: { showingPadInspector = false },
                    onParameterChange: { paramName, value in
                        updatePadParameter(padIndex: padIndex, paramName: paramName, value: value)
                    }
                )
                .frame(width: 280)
                .background(
                    Rectangle()
                        .fill(Color.gray.opacity(0.05))
                )
                .transition(.move(edge: .trailing))
            }
        }
        .frame(width: showingPadInspector ? 800 : 520, height: 600)
        .background(
            RoundedRectangle(cornerRadius: 16)
                .fill(Color.white)
                .shadow(color: Color.black.opacity(0.3), radius: 12, x: 0, y: 8)
        )
        .sheet(isPresented: $showingSampleBrowser) {
            SampleBrowserSheet(
                onSampleSelect: { samplePath, sampleName in
                    loadSampleToPad(samplePath: samplePath, sampleName: sampleName)
                }
            )
        }
        .onAppear {
            initializePads()
        }
        .animation(.easeInOut(duration: 0.3), value: showingPadInspector)
    }
    
    // MARK: - Helper Functions
    private func initializePads() {
        pads = (0..<25).map { SamplerPad(id: $0) }
        
        // Add some demo samples to first few pads
        pads[0].sampleName = "Kick_01.wav"
        pads[1].sampleName = "Snare_02.wav"
        pads[2].sampleName = "HiHat_03.wav"
        pads[5].sampleName = "Perc_Loop.wav"
        pads[5].loopMode = true
        pads[5].oneShot = false
    }
    
    private func selectPad(_ index: Int) {
        selectedPad = index
    }
    
    private func playPad(_ index: Int) {
        guard !pads[index].isEmpty else { return }
        
        playingPads.insert(index)
        
        // Simulate pad play duration
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
            playingPads.remove(index)
        }
        
        // TODO: Send note on to synth controller
        // synthController.playPad(index: index, velocity: 0.8)
    }
    
    private func clearSelectedPad() {
        guard let index = selectedPad else { return }
        pads[index] = SamplerPad(id: index)
    }
    
    private func copySelectedPad() {
        // TODO: Implement pad clipboard
    }
    
    private func pasteSelectedPad() {
        // TODO: Implement pad clipboard
    }
    
    private func loadSampleToPad(samplePath: String, sampleName: String) {
        guard let index = selectedPad else { return }
        pads[index].samplePath = samplePath
        pads[index].sampleName = sampleName
        showingSampleBrowser = false
    }
    
    private func updatePadParameter(padIndex: Int, paramName: String, value: Float) {
        // TODO: Send parameter update to synth controller
        // synthController.setPadParameter(pad: padIndex, param: paramName, value: value)
    }
}

// MARK: - SamplerKit Header
struct SamplerKitHeader: View {
    let trackId: Int
    let trackColor: InstrumentColor
    @Binding var masterVolume: Float
    @Binding var velocitySensitivity: Float
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
                    Text("SamplerKit • Track \(trackId + 1)")
                        .font(.system(size: 16, weight: .bold))
                        .foregroundColor(.primary)
                    
                    Text("25 pads • MPC-style sampler")
                        .font(.system(size: 12))
                        .foregroundColor(.secondary)
                }
            }
            
            Spacer()
            
            // Global Controls
            HStack(spacing: 16) {
                VStack(spacing: 4) {
                    Text("Master")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.secondary)
                    
                    Slider(value: $masterVolume, in: 0...1)
                        .frame(width: 60)
                        .accentColor(trackColor.pastelColor)
                    
                    Text("\(Int(masterVolume * 100))%")
                        .font(.system(size: 10))
                        .foregroundColor(.primary)
                }
                
                VStack(spacing: 4) {
                    Text("Velocity")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.secondary)
                    
                    Slider(value: $velocitySensitivity, in: 0...1)
                        .frame(width: 60)
                        .accentColor(trackColor.pastelColor)
                    
                    Text("\(Int(velocitySensitivity * 100))%")
                        .font(.system(size: 10))
                        .foregroundColor(.primary)
                }
            }
            
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

// MARK: - Sampler Pad View
struct SamplerPadView: View {
    let pad: SamplerPad
    let isSelected: Bool
    let isPlaying: Bool
    let trackColor: InstrumentColor
    let onTap: () -> Void
    let onPlay: () -> Void
    let onLongPress: () -> Void
    
    var body: some View {
        Button(action: {
            onTap()
            if !pad.isEmpty {
                onPlay()
            }
        }) {
            VStack(spacing: 4) {
                // Pad visual
                RoundedRectangle(cornerRadius: 8)
                    .fill(padColor)
                    .overlay(
                        RoundedRectangle(cornerRadius: 8)
                            .stroke(isSelected ? trackColor.pastelColor : Color.clear, lineWidth: 2)
                    )
                    .overlay(
                        VStack(spacing: 2) {
                            // Pad number
                            Text("\(pad.id + 1)")
                                .font(.system(size: 12, weight: .bold))
                                .foregroundColor(pad.isEmpty ? .secondary : .white)
                            
                            // Sample name (truncated)
                            if !pad.isEmpty {
                                Text(String(pad.displayName.prefix(8)))
                                    .font(.system(size: 8, weight: .medium))
                                    .foregroundColor(.white.opacity(0.8))
                                    .lineLimit(1)
                            }
                            
                            // Status indicators
                            HStack(spacing: 2) {
                                if pad.loopMode {
                                    Image(systemName: "repeat")
                                        .font(.system(size: 6))
                                        .foregroundColor(.white.opacity(0.8))
                                }
                                if pad.reverse {
                                    Image(systemName: "backward")
                                        .font(.system(size: 6))
                                        .foregroundColor(.white.opacity(0.8))
                                }
                                if pad.chokeGroup > 0 {
                                    Text("C\(pad.chokeGroup)")
                                        .font(.system(size: 6, weight: .bold))
                                        .foregroundColor(.white.opacity(0.8))
                                }
                            }
                        }
                    )
                    .scaleEffect(isPlaying ? 0.95 : 1.0)
                    .animation(.easeInOut(duration: 0.1), value: isPlaying)
            }
        }
        .buttonStyle(PlainButtonStyle())
        .onLongPressGesture {
            onLongPress()
        }
    }
    
    private var padColor: Color {
        if pad.isEmpty {
            return Color.gray.opacity(0.3)
        } else if isPlaying {
            return trackColor.pastelColor.opacity(0.9)
        } else {
            return trackColor.pastelColor.opacity(0.7)
        }
    }
}

// MARK: - Sampler Quick Actions
struct SamplerQuickActions: View {
    let selectedPad: Int?
    let onLoadSample: () -> Void
    let onClearPad: () -> Void
    let onCopyPad: () -> Void
    let onPastePad: () -> Void
    let onInspector: () -> Void
    
    var body: some View {
        HStack(spacing: 12) {
            Text("Pad \(selectedPad != nil ? "\(selectedPad! + 1)" : "-") selected")
                .font(.system(size: 12, weight: .medium))
                .foregroundColor(.secondary)
            
            Spacer()
            
            HStack(spacing: 8) {
                Button(action: onLoadSample) {
                    HStack(spacing: 4) {
                        Image(systemName: "folder.badge.plus")
                            .font(.system(size: 12))
                        Text("Load")
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
                .disabled(selectedPad == nil)
                
                Button(action: onClearPad) {
                    HStack(spacing: 4) {
                        Image(systemName: "trash")
                            .font(.system(size: 12))
                        Text("Clear")
                            .font(.system(size: 12, weight: .medium))
                    }
                    .foregroundColor(.red)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 6)
                    .background(
                        RoundedRectangle(cornerRadius: 16)
                            .fill(Color.red.opacity(0.1))
                    )
                }
                .buttonStyle(PlainButtonStyle())
                .disabled(selectedPad == nil)
                
                Button(action: onCopyPad) {
                    Image(systemName: "doc.on.doc")
                        .font(.system(size: 12))
                        .foregroundColor(.secondary)
                        .padding(.horizontal, 8)
                        .padding(.vertical, 6)
                        .background(
                            RoundedRectangle(cornerRadius: 16)
                                .fill(Color.gray.opacity(0.1))
                        )
                }
                .buttonStyle(PlainButtonStyle())
                .disabled(selectedPad == nil)
                
                Button(action: onPastePad) {
                    Image(systemName: "doc.on.clipboard")
                        .font(.system(size: 12))
                        .foregroundColor(.secondary)
                        .padding(.horizontal, 8)
                        .padding(.vertical, 6)
                        .background(
                            RoundedRectangle(cornerRadius: 16)
                                .fill(Color.gray.opacity(0.1))
                        )
                }
                .buttonStyle(PlainButtonStyle())
                .disabled(selectedPad == nil)
                
                Button(action: onInspector) {
                    HStack(spacing: 4) {
                        Image(systemName: "slider.horizontal.3")
                            .font(.system(size: 12))
                        Text("Inspector")
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
                .disabled(selectedPad == nil)
            }
        }
    }
}

// MARK: - Pad Inspector Panel
struct PadInspectorPanel: View {
    @Binding var pad: SamplerPad
    let trackColor: InstrumentColor
    let onClose: () -> Void
    let onParameterChange: (String, Float) -> Void
    
    var body: some View {
        VStack(spacing: 0) {
            // Inspector Header
            HStack {
                VStack(alignment: .leading, spacing: 2) {
                    Text("Pad \(pad.id + 1) Inspector")
                        .font(.system(size: 14, weight: .bold))
                        .foregroundColor(.primary)
                    
                    Text(pad.displayName)
                        .font(.system(size: 12))
                        .foregroundColor(.secondary)
                        .lineLimit(1)
                }
                
                Spacer()
                
                Button(action: onClose) {
                    Image(systemName: "xmark.circle.fill")
                        .font(.system(size: 16))
                        .foregroundColor(.secondary)
                }
                .buttonStyle(PlainButtonStyle())
            }
            .padding(.horizontal, 16)
            .padding(.vertical, 12)
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .fill(trackColor.pastelColor.opacity(0.1))
            )
            
            ScrollView {
                VStack(spacing: 20) {
                    // Sample Info
                    if !pad.isEmpty {
                        PadParameterSection(title: "Sample", content: {
                            VStack(spacing: 8) {
                                HStack {
                                    Text("Start")
                                        .font(.system(size: 10))
                                        .foregroundColor(.secondary)
                                    Spacer()
                                    Text("\(Int(pad.startOffset * 100))%")
                                        .font(.system(size: 10))
                                        .foregroundColor(.primary)
                                }
                                Slider(value: $pad.startOffset, in: 0...0.9)
                                    .accentColor(trackColor.pastelColor)
                                
                                HStack {
                                    Text("End")
                                        .font(.system(size: 10))
                                        .foregroundColor(.secondary)
                                    Spacer()
                                    Text("\(Int(pad.endOffset * 100))%")
                                        .font(.system(size: 10))
                                        .foregroundColor(.primary)
                                }
                                Slider(value: $pad.endOffset, in: 0.1...1.0)
                                    .accentColor(trackColor.pastelColor)
                                
                                HStack(spacing: 16) {
                                    Toggle("Reverse", isOn: $pad.reverse)
                                        .font(.system(size: 10))
                                    
                                    Toggle("Loop", isOn: $pad.loopMode)
                                        .font(.system(size: 10))
                                }
                                .toggleStyle(SwitchToggleStyle(tint: trackColor.pastelColor))
                            }
                        })
                    }
                    
                    // Level & Pitch
                    PadParameterSection(title: "Level & Pitch", content: {
                        VStack(spacing: 8) {
                            PadSlider(name: "Volume", value: $pad.volume, range: 0...1, unit: "%", color: trackColor.pastelColor)
                            PadSlider(name: "Pitch", value: $pad.pitch, range: -24...24, unit: "st", color: trackColor.pastelColor)
                            PadSlider(name: "Pan", value: $pad.pan, range: -1...1, unit: "", color: trackColor.pastelColor)
                        }
                    })
                    
                    // Envelope
                    PadParameterSection(title: "Envelope", content: {
                        VStack(spacing: 8) {
                            PadSlider(name: "Attack", value: $pad.attack, range: 0...2, unit: "s", color: trackColor.pastelColor)
                            PadSlider(name: "Decay", value: $pad.decay, range: 0...5, unit: "s", color: trackColor.pastelColor)
                        }
                    })
                    
                    // Filter
                    PadParameterSection(title: "Filter", content: {
                        VStack(spacing: 8) {
                            PadSlider(name: "Cutoff", value: $pad.filterCutoff, range: 0...1, unit: "Hz", color: trackColor.pastelColor)
                            PadSlider(name: "Resonance", value: $pad.filterResonance, range: 0...1, unit: "", color: trackColor.pastelColor)
                        }
                    })
                    
                    // Choke Group
                    PadParameterSection(title: "Choke Group", content: {
                        VStack(spacing: 8) {
                            HStack {
                                Text("Group")
                                    .font(.system(size: 10))
                                    .foregroundColor(.secondary)
                                Spacer()
                                Stepper("\(pad.chokeGroup)", value: $pad.chokeGroup, in: 0...8)
                                    .labelsHidden()
                                Text("\(pad.chokeGroup == 0 ? "None" : "\(pad.chokeGroup)")")
                                    .font(.system(size: 10))
                                    .foregroundColor(.primary)
                            }
                            
                            Text("Pads in the same choke group will cut each other off")
                                .font(.system(size: 8))
                                .foregroundColor(.secondary)
                                .multilineTextAlignment(.leading)
                        }
                    })
                }
                .padding(.horizontal, 16)
                .padding(.vertical, 12)
            }
        }
    }
}

// MARK: - Pad Parameter Section
struct PadParameterSection<Content: View>: View {
    let title: String
    let content: Content
    
    init(title: String, @ViewBuilder content: () -> Content) {
        self.title = title
        self.content = content()
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text(title)
                .font(.system(size: 12, weight: .bold))
                .foregroundColor(.primary)
            
            content
        }
        .padding(.vertical, 8)
        .padding(.horizontal, 12)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(Color.white)
                .shadow(color: Color.black.opacity(0.05), radius: 2, x: 1, y: 1)
        )
    }
}

// MARK: - Pad Slider
struct PadSlider: View {
    let name: String
    @Binding var value: Float
    let range: ClosedRange<Float>
    let unit: String
    let color: Color
    
    var body: some View {
        VStack(spacing: 4) {
            HStack {
                Text(name)
                    .font(.system(size: 10))
                    .foregroundColor(.secondary)
                Spacer()
                Text(valueString)
                    .font(.system(size: 10))
                    .foregroundColor(.primary)
            }
            
            Slider(value: $value, in: range)
                .accentColor(color)
        }
    }
    
    private var valueString: String {
        switch unit {
        case "%":
            return "\(Int(value * 100))%"
        case "st":
            return "\(value > 0 ? "+" : "")\(String(format: "%.1f", value))st"
        case "s":
            return "\(String(format: "%.2f", value))s"
        case "Hz":
            let freq = 20 + value * 20000
            return freq < 1000 ? "\(Int(freq))Hz" : "\(String(format: "%.1f", freq/1000))kHz"
        default:
            if value == 0 {
                return "Center"
            } else {
                return value > 0 ? "R\(Int(abs(value) * 100))" : "L\(Int(abs(value) * 100))"
            }
        }
    }
}

// MARK: - Sample Browser Sheet (Placeholder)
struct SampleBrowserSheet: View {
    let onSampleSelect: (String, String) -> Void
    
    var body: some View {
        VStack {
            Text("Sample Browser")
                .font(.headline)
            
            Text("Sample browser with audio preview would go here")
                .foregroundColor(.secondary)
            
            Button("Select Demo Sample") {
                onSampleSelect("/path/to/sample.wav", "Demo_Sample.wav")
            }
        }
        .frame(width: 600, height: 400)
        .padding()
    }
}