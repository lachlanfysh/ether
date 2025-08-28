import SwiftUI

/**
 * Mod Page - Comprehensive modulation system with LFOs and modulation matrix
 * Provides 8 LFOs, 8 envelopes, and flexible routing matrix for deep sound design
 */

// MARK: - Modulation Models
struct LFO {
    let id: Int
    var waveform: LFOWaveform = .sine
    var rate: Float = 1.0 // Hz
    var depth: Float = 0.5 // 0-1
    var phase: Float = 0.0 // 0-1
    var sync: LFOSync = .free
    var oneShot: Bool = false
    var bipolar: Bool = true
    var smooth: Float = 0.0 // Smoothing amount
    var keytrack: Float = 0.0 // Keyboard tracking
    var currentValue: Float = 0.0 // Real-time value
    
    var displayName: String {
        return "LFO \(id + 1)"
    }
}

enum LFOWaveform: String, CaseIterable {
    case sine = "Sine"
    case triangle = "Triangle"
    case square = "Square"
    case saw = "Saw"
    case ramp = "Ramp"
    case noise = "Noise"
    case sampleHold = "S&H"
    case custom = "Custom"
    
    var icon: String {
        switch self {
        case .sine: return "sine.wave"
        case .triangle: return "triangle"
        case .square: return "square"
        case .saw: return "angle"
        case .ramp: return "angle.fill"
        case .noise: return "waveform.path.ecg"
        case .sampleHold: return "stairstep"
        case .custom: return "waveform"
        }
    }
}

enum LFOSync: String, CaseIterable {
    case free = "Free"
    case tempo = "Tempo"
    case note = "Note"
    case key = "Key"
}

struct Envelope {
    let id: Int
    var attack: Float = 0.1
    var decay: Float = 0.3
    var sustain: Float = 0.7
    var release: Float = 0.5
    var curve: EnvelopeCurve = .exponential
    var loop: Bool = false
    var velocity: Float = 1.0 // Velocity sensitivity
    var currentPhase: EnvelopePhase = .idle
    var currentValue: Float = 0.0
    
    var displayName: String {
        return "ENV \(id + 1)"
    }
}

enum EnvelopeCurve: String, CaseIterable {
    case linear = "Linear"
    case exponential = "Exponential"
    case logarithmic = "Logarithmic"
}

enum EnvelopePhase: String {
    case idle = "Idle"
    case attack = "Attack"
    case decay = "Decay"
    case sustain = "Sustain"
    case release = "Release"
}

struct ModulationConnection {
    let id: String
    let source: ModulationSource
    let destination: ModulationDestination
    var amount: Float = 0.0 // -1 to 1
    var curve: ModulationCurve = .linear
    var enabled: Bool = true
    
    var displayAmount: String {
        return "\(amount >= 0 ? "+" : "")\(Int(amount * 100))%"
    }
}

struct ModulationSource {
    let id: String
    let name: String
    let type: ModSourceType
    let color: Color
    
    enum ModSourceType {
        case lfo(Int)
        case envelope(Int)
        case velocity
        case aftertouch
        case modwheel
        case pitchbend
        case keytrack
        case random
    }
}

struct ModulationDestination {
    let id: String
    let name: String
    let category: String
    let range: ClosedRange<Float>
    let color: Color
}

enum ModulationCurve: String, CaseIterable {
    case linear = "Linear"
    case exponential = "Exponential"
    case logarithmic = "Logarithmic"
    case sCurve = "S-Curve"
}

// MARK: - Main Mod Page
struct ModPage: View {
    @EnvironmentObject var synthController: SynthController
    @State private var lfos: [LFO] = []
    @State private var envelopes: [Envelope] = []
    @State private var modConnections: [ModulationConnection] = []
    @State private var selectedTab: ModTab = .lfos
    @State private var selectedSource: ModulationSource? = nil
    @State private var selectedDestination: ModulationDestination? = nil
    @State private var showingMatrix: Bool = false
    @State private var matrixFilter: String = ""
    
    enum ModTab: String, CaseIterable {
        case lfos = "LFOs"
        case envelopes = "Envelopes"
        case matrix = "Matrix"
        case macros = "Macros"
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            ModPageHeader(
                selectedTab: $selectedTab,
                connectionsCount: modConnections.filter { $0.enabled }.count,
                onShowMatrix: { showingMatrix = true }
            )
            .padding(.horizontal, 20)
            .padding(.vertical, 16)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(Color.purple.opacity(0.1))
            )
            
            // Tab Content
            switch selectedTab {
            case .lfos:
                LFOSectionView(
                    lfos: $lfos,
                    onParameterChange: { lfoId, param, value in
                        updateLFOParameter(lfoId: lfoId, param: param, value: value)
                    }
                )
                .padding(.horizontal, 20)
                .padding(.vertical, 16)
                
            case .envelopes:
                EnvelopeSectionView(
                    envelopes: $envelopes,
                    onParameterChange: { envId, param, value in
                        updateEnvelopeParameter(envId: envId, param: param, value: value)
                    }
                )
                .padding(.horizontal, 20)
                .padding(.vertical, 16)
                
            case .matrix:
                ModulationMatrixView(
                    connections: $modConnections,
                    sources: modulationSources,
                    destinations: modulationDestinations,
                    filterText: $matrixFilter,
                    onConnectionChange: { connection in
                        updateModulationConnection(connection)
                    }
                )
                .padding(.horizontal, 20)
                .padding(.vertical, 16)
                
            case .macros:
                MacroControlsView(
                    onMacroAssign: { macroId, sourceId in
                        assignMacro(macroId: macroId, sourceId: sourceId)
                    }
                )
                .padding(.horizontal, 20)
                .padding(.vertical, 16)
            }
            
            Spacer()
            
            // Real-time Modulation Display
            ModulationVisualizerView(
                lfos: lfos,
                envelopes: envelopes,
                connections: modConnections.filter { $0.enabled }
            )
            .frame(height: 80)
            .padding(.horizontal, 20)
            .padding(.bottom, 20)
        }
        .frame(width: 1000, height: 700)
        .background(
            RoundedRectangle(cornerRadius: 16)
                .fill(Color.white)
                .shadow(color: Color.black.opacity(0.3), radius: 12, x: 0, y: 8)
        )
        .sheet(isPresented: $showingMatrix) {
            ModulationMatrixSheet(
                connections: $modConnections,
                sources: modulationSources,
                destinations: modulationDestinations,
                onConnectionUpdate: { connection in
                    updateModulationConnection(connection)
                }
            )
        }
        .onAppear {
            initializeModulation()
            startModulationTimer()
        }
    }
    
    // MARK: - Helper Functions
    private func initializeModulation() {
        // Initialize 8 LFOs
        lfos = (0..<8).map { index in
            LFO(id: index)
        }
        
        // Initialize 8 envelopes
        envelopes = (0..<8).map { index in
            Envelope(id: index)
        }
        
        // Create some demo modulation connections
        modConnections = [
            ModulationConnection(
                id: "lfo1_cutoff",
                source: modulationSources[0], // LFO 1
                destination: modulationDestinations[0], // Filter Cutoff
                amount: 0.3,
                enabled: true
            ),
            ModulationConnection(
                id: "env1_amp",
                source: modulationSources[8], // Envelope 1
                destination: modulationDestinations[8], // Amplitude
                amount: 1.0,
                enabled: true
            )
        ]
    }
    
    private func startModulationTimer() {
        Timer.scheduledTimer(withTimeInterval: 1.0/60.0, repeats: true) { _ in
            updateRealtimeValues()
        }
    }
    
    private func updateRealtimeValues() {
        // Update LFO values (simplified simulation)
        for i in 0..<lfos.count {
            let time = Date().timeIntervalSince1970
            let phase = Float(time * Double(lfos[i].rate)).truncatingRemainder(dividingBy: 1.0)
            
            switch lfos[i].waveform {
            case .sine:
                lfos[i].currentValue = sin(phase * 2 * Float.pi) * lfos[i].depth
            case .triangle:
                lfos[i].currentValue = (abs(phase - 0.5) * 4 - 1) * lfos[i].depth
            case .square:
                lfos[i].currentValue = (phase < 0.5 ? -1 : 1) * lfos[i].depth
            case .saw:
                lfos[i].currentValue = (phase * 2 - 1) * lfos[i].depth
            default:
                lfos[i].currentValue = sin(phase * 2 * Float.pi) * lfos[i].depth
            }
            
            if !lfos[i].bipolar {
                lfos[i].currentValue = (lfos[i].currentValue + 1) * 0.5
            }
        }
        
        // Update envelope values (simplified)
        for i in 0..<envelopes.count {
            // For demo, just cycle through phases
            let time = Date().timeIntervalSince1970
            let cycleTime = Float(time.truncatingRemainder(dividingBy: 4.0))
            
            if cycleTime < envelopes[i].attack {
                envelopes[i].currentPhase = .attack
                envelopes[i].currentValue = cycleTime / envelopes[i].attack
            } else if cycleTime < envelopes[i].attack + envelopes[i].decay {
                envelopes[i].currentPhase = .decay
                let decayProgress = (cycleTime - envelopes[i].attack) / envelopes[i].decay
                envelopes[i].currentValue = 1.0 - (1.0 - envelopes[i].sustain) * decayProgress
            } else {
                envelopes[i].currentPhase = .sustain
                envelopes[i].currentValue = envelopes[i].sustain
            }
        }
    }
    
    private func updateLFOParameter(lfoId: Int, param: String, value: Float) {
        guard lfoId < lfos.count else { return }
        
        switch param {
        case "rate":
            lfos[lfoId].rate = value
        case "depth":
            lfos[lfoId].depth = value
        case "phase":
            lfos[lfoId].phase = value
        default:
            break
        }
        
        // TODO: Send to synth controller
        // synthController.setLFOParameter(lfo: lfoId, param: param, value: value)
    }
    
    private func updateEnvelopeParameter(envId: Int, param: String, value: Float) {
        guard envId < envelopes.count else { return }
        
        switch param {
        case "attack":
            envelopes[envId].attack = value
        case "decay":
            envelopes[envId].decay = value
        case "sustain":
            envelopes[envId].sustain = value
        case "release":
            envelopes[envId].release = value
        default:
            break
        }
        
        // TODO: Send to synth controller
        // synthController.setEnvelopeParameter(env: envId, param: param, value: value)
    }
    
    private func updateModulationConnection(_ connection: ModulationConnection) {
        if let index = modConnections.firstIndex(where: { $0.id == connection.id }) {
            modConnections[index] = connection
        } else {
            modConnections.append(connection)
        }
        
        // TODO: Send to synth controller
        // synthController.setModulationConnection(connection)
    }
    
    private func assignMacro(macroId: Int, sourceId: String) {
        // TODO: Implement macro assignment
    }
    
    private var modulationSources: [ModulationSource] {
        var sources: [ModulationSource] = []
        
        // LFOs
        for i in 0..<8 {
            sources.append(ModulationSource(
                id: "lfo\(i)",
                name: "LFO \(i + 1)",
                type: .lfo(i),
                color: .blue
            ))
        }
        
        // Envelopes
        for i in 0..<8 {
            sources.append(ModulationSource(
                id: "env\(i)",
                name: "ENV \(i + 1)",
                type: .envelope(i),
                color: .green
            ))
        }
        
        // MIDI sources
        sources.append(contentsOf: [
            ModulationSource(id: "velocity", name: "Velocity", type: .velocity, color: .orange),
            ModulationSource(id: "aftertouch", name: "Aftertouch", type: .aftertouch, color: .orange),
            ModulationSource(id: "modwheel", name: "Mod Wheel", type: .modwheel, color: .orange),
            ModulationSource(id: "pitchbend", name: "Pitch Bend", type: .pitchbend, color: .orange),
            ModulationSource(id: "keytrack", name: "Key Track", type: .keytrack, color: .purple),
            ModulationSource(id: "random", name: "Random", type: .random, color: .red)
        ])
        
        return sources
    }
    
    private var modulationDestinations: [ModulationDestination] {
        return [
            // Filter
            ModulationDestination(id: "filter_cutoff", name: "Cutoff", category: "Filter", range: 0...1, color: .blue),
            ModulationDestination(id: "filter_resonance", name: "Resonance", category: "Filter", range: 0...1, color: .blue),
            ModulationDestination(id: "filter_drive", name: "Drive", category: "Filter", range: 0...1, color: .blue),
            
            // Oscillator
            ModulationDestination(id: "osc_pitch", name: "Pitch", category: "Oscillator", range: -1...1, color: .green),
            ModulationDestination(id: "osc_detune", name: "Detune", category: "Oscillator", range: -1...1, color: .green),
            ModulationDestination(id: "osc_wave", name: "Waveform", category: "Oscillator", range: 0...1, color: .green),
            ModulationDestination(id: "osc_fm", name: "FM Amount", category: "Oscillator", range: 0...1, color: .green),
            
            // Amplitude
            ModulationDestination(id: "amp_level", name: "Level", category: "Amplitude", range: 0...1, color: .red),
            ModulationDestination(id: "amp_pan", name: "Pan", category: "Amplitude", range: -1...1, color: .red),
            
            // Effects
            ModulationDestination(id: "fx_delay_time", name: "Delay Time", category: "Effects", range: 0...1, color: .purple),
            ModulationDestination(id: "fx_reverb_size", name: "Reverb Size", category: "Effects", range: 0...1, color: .purple),
            ModulationDestination(id: "fx_chorus_rate", name: "Chorus Rate", category: "Effects", range: 0...1, color: .purple),
            
            // Hero Macros
            ModulationDestination(id: "macro_harmonics", name: "Harmonics", category: "Macros", range: 0...1, color: .orange),
            ModulationDestination(id: "macro_timbre", name: "Timbre", category: "Macros", range: 0...1, color: .orange),
            ModulationDestination(id: "macro_morph", name: "Morph", category: "Macros", range: 0...1, color: .orange)
        ]
    }
}

// MARK: - Mod Page Header
struct ModPageHeader: View {
    @Binding var selectedTab: ModPage.ModTab
    let connectionsCount: Int
    let onShowMatrix: () -> Void
    
    var body: some View {
        HStack {
            // Title
            HStack(spacing: 12) {
                Circle()
                    .fill(Color.purple)
                    .frame(width: 32, height: 32)
                    .overlay(
                        Image(systemName: "waveform.path.ecg.rectangle")
                            .font(.system(size: 16))
                            .foregroundColor(.white)
                    )
                
                VStack(alignment: .leading, spacing: 2) {
                    Text("Modulation")
                        .font(.system(size: 16, weight: .bold))
                        .foregroundColor(.primary)
                    
                    Text("\(connectionsCount) active connections")
                        .font(.system(size: 12))
                        .foregroundColor(.secondary)
                }
            }
            
            Spacer()
            
            // Tab Picker
            Picker("Mod Tab", selection: $selectedTab) {
                ForEach(ModPage.ModTab.allCases, id: \.self) { tab in
                    Text(tab.rawValue).tag(tab)
                }
            }
            .pickerStyle(SegmentedPickerStyle())
            .frame(width: 300)
            
            Spacer()
            
            // Matrix Button
            Button(action: onShowMatrix) {
                HStack(spacing: 4) {
                    Image(systemName: "grid")
                        .font(.system(size: 12))
                    Text("Matrix")
                        .font(.system(size: 12, weight: .medium))
                }
                .foregroundColor(.purple)
                .padding(.horizontal, 12)
                .padding(.vertical, 6)
                .background(
                    RoundedRectangle(cornerRadius: 16)
                        .fill(Color.purple.opacity(0.1))
                )
            }
            .buttonStyle(PlainButtonStyle())
        }
    }
}

// MARK: - LFO Section View
struct LFOSectionView: View {
    @Binding var lfos: [LFO]
    let onParameterChange: (Int, String, Float) -> Void
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Low Frequency Oscillators")
                .font(.system(size: 16, weight: .bold))
                .foregroundColor(.primary)
            
            LazyVGrid(columns: [
                GridItem(.flexible()),
                GridItem(.flexible()),
                GridItem(.flexible()),
                GridItem(.flexible())
            ], spacing: 16) {
                ForEach(Array(lfos.enumerated()), id: \.element.id) { index, lfo in
                    LFOControlView(
                        lfo: $lfos[index],
                        onParameterChange: { param, value in
                            onParameterChange(index, param, value)
                        }
                    )
                }
            }
        }
    }
}

// MARK: - LFO Control View
struct LFOControlView: View {
    @Binding var lfo: LFO
    let onParameterChange: (String, Float) -> Void
    
    var body: some View {
        VStack(spacing: 12) {
            // Header
            HStack {
                Text(lfo.displayName)
                    .font(.system(size: 12, weight: .bold))
                    .foregroundColor(.primary)
                
                Spacer()
                
                // Real-time value indicator
                Circle()
                    .fill(Color.blue)
                    .frame(width: 8, height: 8)
                    .opacity(Double(abs(lfo.currentValue)))
            }
            
            // Waveform Picker
            VStack(spacing: 4) {
                Text("Wave")
                    .font(.system(size: 10))
                    .foregroundColor(.secondary)
                
                Picker("Waveform", selection: $lfo.waveform) {
                    ForEach(LFOWaveform.allCases, id: \.self) { waveform in
                        HStack(spacing: 4) {
                            Image(systemName: waveform.icon)
                                .font(.system(size: 8))
                            Text(waveform.rawValue)
                                .font(.system(size: 8))
                        }
                        .tag(waveform)
                    }
                }
                .pickerStyle(MenuPickerStyle())
                .controlSize(.mini)
            }
            
            // Parameters
            VStack(spacing: 8) {
                ModParameterSlider(
                    name: "Rate",
                    value: Binding(
                        get: { lfo.rate },
                        set: { 
                            lfo.rate = $0
                            onParameterChange("rate", $0)
                        }
                    ),
                    range: 0.01...20.0,
                    unit: "Hz",
                    color: .blue
                )
                
                ModParameterSlider(
                    name: "Depth",
                    value: Binding(
                        get: { lfo.depth },
                        set: { 
                            lfo.depth = $0
                            onParameterChange("depth", $0)
                        }
                    ),
                    range: 0.0...1.0,
                    unit: "",
                    color: .blue
                )
                
                ModParameterSlider(
                    name: "Phase",
                    value: Binding(
                        get: { lfo.phase },
                        set: { 
                            lfo.phase = $0
                            onParameterChange("phase", $0)
                        }
                    ),
                    range: 0.0...1.0,
                    unit: "°",
                    color: .blue
                )
            }
            
            // Options
            VStack(spacing: 4) {
                Toggle("Bipolar", isOn: $lfo.bipolar)
                    .font(.system(size: 10))
                    .toggleStyle(SwitchToggleStyle(tint: .blue))
                
                Toggle("One Shot", isOn: $lfo.oneShot)
                    .font(.system(size: 10))
                    .toggleStyle(SwitchToggleStyle(tint: .blue))
            }
        }
        .padding(.vertical, 12)
        .padding(.horizontal, 10)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(Color.blue.opacity(0.05))
                .stroke(Color.blue.opacity(0.2), lineWidth: 1)
        )
    }
}

// MARK: - Envelope Section View
struct EnvelopeSectionView: View {
    @Binding var envelopes: [Envelope]
    let onParameterChange: (Int, String, Float) -> Void
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Envelopes")
                .font(.system(size: 16, weight: .bold))
                .foregroundColor(.primary)
            
            LazyVGrid(columns: [
                GridItem(.flexible()),
                GridItem(.flexible()),
                GridItem(.flexible()),
                GridItem(.flexible())
            ], spacing: 16) {
                ForEach(Array(envelopes.enumerated()), id: \.element.id) { index, envelope in
                    EnvelopeControlView(
                        envelope: $envelopes[index],
                        onParameterChange: { param, value in
                            onParameterChange(index, param, value)
                        }
                    )
                }
            }
        }
    }
}

// MARK: - Envelope Control View
struct EnvelopeControlView: View {
    @Binding var envelope: Envelope
    let onParameterChange: (String, Float) -> Void
    
    var body: some View {
        VStack(spacing: 12) {
            // Header
            HStack {
                Text(envelope.displayName)
                    .font(.system(size: 12, weight: .bold))
                    .foregroundColor(.primary)
                
                Spacer()
                
                // Phase indicator
                Text(envelope.currentPhase.rawValue)
                    .font(.system(size: 8))
                    .foregroundColor(.green)
                    .padding(.horizontal, 4)
                    .padding(.vertical, 2)
                    .background(
                        RoundedRectangle(cornerRadius: 4)
                            .fill(Color.green.opacity(0.1))
                    )
            }
            
            // ADSR Parameters
            VStack(spacing: 6) {
                ModParameterSlider(
                    name: "A",
                    value: Binding(
                        get: { envelope.attack },
                        set: { 
                            envelope.attack = $0
                            onParameterChange("attack", $0)
                        }
                    ),
                    range: 0.001...5.0,
                    unit: "s",
                    color: .green
                )
                
                ModParameterSlider(
                    name: "D",
                    value: Binding(
                        get: { envelope.decay },
                        set: { 
                            envelope.decay = $0
                            onParameterChange("decay", $0)
                        }
                    ),
                    range: 0.001...5.0,
                    unit: "s",
                    color: .green
                )
                
                ModParameterSlider(
                    name: "S",
                    value: Binding(
                        get: { envelope.sustain },
                        set: { 
                            envelope.sustain = $0
                            onParameterChange("sustain", $0)
                        }
                    ),
                    range: 0.0...1.0,
                    unit: "",
                    color: .green
                )
                
                ModParameterSlider(
                    name: "R",
                    value: Binding(
                        get: { envelope.release },
                        set: { 
                            envelope.release = $0
                            onParameterChange("release", $0)
                        }
                    ),
                    range: 0.001...10.0,
                    unit: "s",
                    color: .green
                )
            }
            
            // Curve Picker
            VStack(spacing: 4) {
                Text("Curve")
                    .font(.system(size: 10))
                    .foregroundColor(.secondary)
                
                Picker("Curve", selection: $envelope.curve) {
                    ForEach(EnvelopeCurve.allCases, id: \.self) { curve in
                        Text(curve.rawValue).tag(curve)
                    }
                }
                .pickerStyle(MenuPickerStyle())
                .controlSize(.mini)
            }
            
            // Loop option
            Toggle("Loop", isOn: $envelope.loop)
                .font(.system(size: 10))
                .toggleStyle(SwitchToggleStyle(tint: .green))
        }
        .padding(.vertical, 12)
        .padding(.horizontal, 10)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(Color.green.opacity(0.05))
                .stroke(Color.green.opacity(0.2), lineWidth: 1)
        )
    }
}

// MARK: - Modulation Matrix View
struct ModulationMatrixView: View {
    @Binding var connections: [ModulationConnection]
    let sources: [ModulationSource]
    let destinations: [ModulationDestination]
    @Binding var filterText: String
    let onConnectionChange: (ModulationConnection) -> Void
    
    var filteredDestinations: [ModulationDestination] {
        if filterText.isEmpty {
            return destinations
        }
        return destinations.filter { dest in
            dest.name.localizedCaseInsensitiveContains(filterText) ||
            dest.category.localizedCaseInsensitiveContains(filterText)
        }
    }
    
    var body: some View {
        VStack(spacing: 16) {
            // Header
            HStack {
                Text("Modulation Matrix")
                    .font(.system(size: 16, weight: .bold))
                    .foregroundColor(.primary)
                
                Spacer()
                
                // Filter
                HStack {
                    Image(systemName: "magnifyingglass")
                        .font(.system(size: 12))
                        .foregroundColor(.secondary)
                    
                    TextField("Filter destinations...", text: $filterText)
                        .textFieldStyle(PlainTextFieldStyle())
                        .font(.system(size: 12))
                }
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(
                    RoundedRectangle(cornerRadius: 8)
                        .fill(Color.white)
                        .stroke(Color.gray.opacity(0.3), lineWidth: 1)
                )
                .frame(width: 200)
            }
            
            // Matrix Grid
            ScrollView {
                LazyVGrid(columns: Array(repeating: GridItem(.flexible(), spacing: 4), count: 6), spacing: 8) {
                    ForEach(filteredDestinations, id: \.id) { destination in
                        ModulationSlotView(
                            destination: destination,
                            connections: connections.filter { $0.destination.id == destination.id },
                            sources: sources,
                            onConnectionChange: onConnectionChange
                        )
                    }
                }
            }
        }
    }
}

// MARK: - Modulation Slot View
struct ModulationSlotView: View {
    let destination: ModulationDestination
    let connections: [ModulationConnection]
    let sources: [ModulationSource]
    let onConnectionChange: (ModulationConnection) -> Void
    
    @State private var selectedSource: ModulationSource? = nil
    @State private var amount: Float = 0.0
    
    var body: some View {
        VStack(spacing: 8) {
            // Destination info
            VStack(spacing: 2) {
                Text(destination.name)
                    .font(.system(size: 11, weight: .bold))
                    .foregroundColor(.primary)
                    .lineLimit(1)
                
                Text(destination.category)
                    .font(.system(size: 9))
                    .foregroundColor(destination.color)
            }
            
            // Source picker
            Picker("Source", selection: $selectedSource) {
                Text("None").tag(ModulationSource?.none)
                ForEach(sources, id: \.id) { source in
                    Text(source.name).tag(ModulationSource?.some(source))
                }
            }
            .pickerStyle(MenuPickerStyle())
            .controlSize(.mini)
            
            // Amount slider
            if selectedSource != nil {
                VStack(spacing: 4) {
                    Slider(value: $amount, in: -1.0...1.0)
                        .accentColor(destination.color)
                        .controlSize(.mini)
                    
                    Text("\(amount >= 0 ? "+" : "")\(Int(amount * 100))%")
                        .font(.system(size: 8))
                        .foregroundColor(.primary)
                }
                .onChange(of: amount) { newAmount in
                    updateConnection(newAmount)
                }
            }
            
            // Active connections display
            if !connections.isEmpty {
                VStack(spacing: 2) {
                    ForEach(connections.prefix(2), id: \.id) { connection in
                        HStack(spacing: 4) {
                            Circle()
                                .fill(connection.source.color)
                                .frame(width: 6, height: 6)
                            
                            Text(connection.source.name)
                                .font(.system(size: 8))
                                .foregroundColor(.secondary)
                                .lineLimit(1)
                            
                            Spacer()
                            
                            Text(connection.displayAmount)
                                .font(.system(size: 8, weight: .bold))
                                .foregroundColor(.primary)
                        }
                    }
                    
                    if connections.count > 2 {
                        Text("+\(connections.count - 2) more")
                            .font(.system(size: 8))
                            .foregroundColor(.secondary)
                    }
                }
            }
        }
        .padding(.vertical, 8)
        .padding(.horizontal, 6)
        .background(
            RoundedRectangle(cornerRadius: 6)
                .fill(destination.color.opacity(0.05))
                .stroke(destination.color.opacity(0.2), lineWidth: 1)
        )
        .onAppear {
            if let connection = connections.first {
                selectedSource = connection.source
                amount = connection.amount
            }
        }
    }
    
    private func updateConnection(_ newAmount: Float) {
        guard let source = selectedSource else { return }
        
        let connection = ModulationConnection(
            id: "\(source.id)_\(destination.id)",
            source: source,
            destination: destination,
            amount: newAmount,
            enabled: abs(newAmount) > 0.01
        )
        
        onConnectionChange(connection)
    }
}

// MARK: - Macro Controls View
struct MacroControlsView: View {
    let onMacroAssign: (Int, String) -> Void
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Macro Controls")
                .font(.system(size: 16, weight: .bold))
                .foregroundColor(.primary)
            
            Text("Macro controls coming soon...")
                .font(.system(size: 14))
                .foregroundColor(.secondary)
        }
    }
}

// MARK: - Modulation Visualizer View
struct ModulationVisualizerView: View {
    let lfos: [LFO]
    let envelopes: [Envelope]
    let connections: [ModulationConnection]
    
    var body: some View {
        HStack(spacing: 16) {
            // LFO waveforms
            ForEach(lfos.prefix(4), id: \.id) { lfo in
                VStack(spacing: 4) {
                    Text("LFO \(lfo.id + 1)")
                        .font(.system(size: 8))
                        .foregroundColor(.secondary)
                    
                    LFOWaveformView(lfo: lfo)
                        .frame(width: 60, height: 30)
                }
            }
            
            Spacer()
            
            // Connection activity
            VStack(spacing: 4) {
                Text("Connections")
                    .font(.system(size: 8))
                    .foregroundColor(.secondary)
                
                Text("\(connections.count)")
                    .font(.system(size: 16, weight: .bold))
                    .foregroundColor(.purple)
            }
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 8)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(Color.gray.opacity(0.05))
        )
    }
}

// MARK: - LFO Waveform View
struct LFOWaveformView: View {
    let lfo: LFO
    
    var body: some View {
        GeometryReader { geometry in
            Path { path in
                let width = geometry.size.width
                let height = geometry.size.height
                let centerY = height / 2
                let samples = 100
                
                path.move(to: CGPoint(x: 0, y: centerY))
                
                for i in 0..<samples {
                    let x = CGFloat(i) / CGFloat(samples - 1) * width
                    let phase = Float(i) / Float(samples - 1)
                    
                    var value: Float
                    switch lfo.waveform {
                    case .sine:
                        value = sin(phase * 2 * Float.pi)
                    case .triangle:
                        value = abs(phase - 0.5) * 4 - 1
                    case .square:
                        value = phase < 0.5 ? -1 : 1
                    case .saw:
                        value = phase * 2 - 1
                    default:
                        value = sin(phase * 2 * Float.pi)
                    }
                    
                    if !lfo.bipolar {
                        value = (value + 1) * 0.5
                    }
                    
                    let y = centerY - CGFloat(value * lfo.depth) * centerY * 0.8
                    path.addLine(to: CGPoint(x: x, y: y))
                }
            }
            .stroke(Color.blue, lineWidth: 2)
            
            // Current position indicator
            let currentX = CGFloat(lfo.phase) * geometry.size.width
            Rectangle()
                .fill(Color.red)
                .frame(width: 2)
                .position(x: currentX, y: geometry.size.height / 2)
        }
        .background(
            RoundedRectangle(cornerRadius: 4)
                .stroke(Color.gray.opacity(0.3), lineWidth: 1)
        )
    }
}

// MARK: - Mod Parameter Slider
struct ModParameterSlider: View {
    let name: String
    @Binding var value: Float
    let range: ClosedRange<Float>
    let unit: String
    let color: Color
    
    var body: some View {
        VStack(spacing: 2) {
            HStack {
                Text(name)
                    .font(.system(size: 9))
                    .foregroundColor(.secondary)
                Spacer()
                Text(valueString)
                    .font(.system(size: 9, weight: .bold))
                    .foregroundColor(.primary)
            }
            
            Slider(value: $value, in: range)
                .accentColor(color)
                .controlSize(.mini)
        }
    }
    
    private var valueString: String {
        switch unit {
        case "Hz":
            return String(format: "%.2f Hz", value)
        case "s":
            return String(format: "%.3f s", value)
        case "°":
            return String(format: "%.0f°", value * 360)
        case "":
            return String(format: "%.0f%%", value * 100)
        default:
            return String(format: "%.2f %@", value, unit)
        }
    }
}

// MARK: - Modulation Matrix Sheet
struct ModulationMatrixSheet: View {
    @Binding var connections: [ModulationConnection]
    let sources: [ModulationSource]
    let destinations: [ModulationDestination]
    let onConnectionUpdate: (ModulationConnection) -> Void
    
    @Environment(\.presentationMode) var presentationMode
    
    var body: some View {
        VStack {
            Text("Full Modulation Matrix")
                .font(.headline)
            
            Text("Complete modulation matrix view coming soon...")
                .foregroundColor(.secondary)
            
            Button("Close") {
                presentationMode.wrappedValue.dismiss()
            }
        }
        .frame(width: 800, height: 600)
        .padding()
    }
}