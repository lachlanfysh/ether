import SwiftUI

/**
 * Mix Page - Professional mixing interface with channel strips and bus FX
 * Provides 8-track mixing console with master bus, sends, and comprehensive effects
 */

// MARK: - Mix Page Models
struct ChannelStrip {
    let id: Int
    var name: String
    var color: InstrumentColor
    var isMuted: Bool = false
    var isSoloed: Bool = false
    var volume: Float = 0.75 // 0-1 (linear)
    var pan: Float = 0.0 // -1 to 1
    var gain: Float = 0.0 // -24 to +24 dB
    var highpass: Float = 0.0 // 20Hz-1kHz
    var lowpass: Float = 1.0 // 1kHz-20kHz
    var sendA: Float = 0.0 // Send to Bus A
    var sendB: Float = 0.0 // Send to Bus B
    var sendC: Float = 0.0 // Send to Bus C
    var sendD: Float = 0.0 // Send to Bus D
    var insertFX: [InsertSlot] = []
    var level: Float = 0.0 // Current output level (for meters)
    var peak: Float = 0.0 // Peak level
    var rms: Float = 0.0 // RMS level
    
    var volumeDb: Float {
        return 20 * log10(max(volume, 0.001)) // Convert to dB
    }
    
    var displayName: String {
        return name.isEmpty ? "Track \(id + 1)" : name
    }
}

struct InsertSlot {
    let id: Int
    var effect: InsertEffect?
    var enabled: Bool = true
    var bypass: Bool = false
    
    var isEmpty: Bool {
        return effect == nil
    }
}

struct InsertEffect {
    let id: String
    let name: String
    let type: EffectType
    var parameters: [String: Float] = [:]
    
    enum EffectType: String, CaseIterable {
        case compressor = "Compressor"
        case equalizer = "EQ"
        case reverb = "Reverb"
        case delay = "Delay"
        case chorus = "Chorus"
        case distortion = "Distortion"
        case filter = "Filter"
        case phaser = "Phaser"
        case flanger = "Flanger"
        case bitcrusher = "Bitcrusher"
        
        var icon: String {
            switch self {
            case .compressor: return "waveform.path.ecg"
            case .equalizer: return "slider.horizontal.3"
            case .reverb: return "speaker.wave.3"
            case .delay: return "arrow.clockwise"
            case .chorus: return "waveform.path"
            case .distortion: return "bolt"
            case .filter: return "line.3.horizontal.decrease"
            case .phaser: return "waveform.circle"
            case .flanger: return "waveform.circle.fill"
            case .bitcrusher: return "square.grid.3x3"
            }
        }
        
        var color: Color {
            switch self {
            case .compressor: return .blue
            case .equalizer: return .green
            case .reverb: return .purple
            case .delay: return .orange
            case .chorus: return .cyan
            case .distortion: return .red
            case .filter: return .yellow
            case .phaser: return .pink
            case .flanger: return .indigo
            case .bitcrusher: return .gray
            }
        }
    }
}

struct MasterBus {
    var volume: Float = 0.8
    var pan: Float = 0.0
    var isMuted: Bool = false
    var level: Float = 0.0
    var peak: Float = 0.0
    var rms: Float = 0.0
    var insertFX: [InsertSlot] = []
    var limiter: LimiterSettings = LimiterSettings()
    
    var volumeDb: Float {
        return 20 * log10(max(volume, 0.001))
    }
}

struct LimiterSettings {
    var enabled: Bool = true
    var threshold: Float = -3.0 // dB
    var release: Float = 50.0 // ms
    var ceiling: Float = -0.1 // dB
    var gain: Float = 0.0 // dB
}

struct SendBus {
    let id: String
    let name: String
    var volume: Float = 0.5
    var isMuted: Bool = false
    var insertFX: [InsertSlot] = []
    var returnLevel: Float = 0.3
    var level: Float = 0.0
    
    var displayName: String {
        return "Bus \(name)"
    }
}

// MARK: - Main Mix Page
struct MixPage: View {
    @EnvironmentObject var synthController: SynthController
    @State private var channelStrips: [ChannelStrip] = []
    @State private var masterBus = MasterBus()
    @State private var sendBuses: [SendBus] = []
    @State private var selectedChannel: Int? = nil
    @State private var showingFXLibrary: Bool = false
    @State private var showingMasterEQ: Bool = false
    @State private var mixerMode: MixerMode = .mix
    @State private var meteringMode: MeteringMode = .peak
    @State private var soloedChannels: Set<Int> = []
    
    enum MixerMode: String, CaseIterable {
        case mix = "Mix"
        case sends = "Sends"
        case fx = "FX"
        case master = "Master"
    }
    
    enum MeteringMode: String, CaseIterable {
        case peak = "Peak"
        case rms = "RMS"
        case peak_rms = "Peak+RMS"
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            MixPageHeader(
                mixerMode: $mixerMode,
                meteringMode: $meteringMode,
                soloCount: soloedChannels.count,
                onShowFXLibrary: { showingFXLibrary = true },
                onShowMasterEQ: { showingMasterEQ = true }
            )
            .padding(.horizontal, 20)
            .padding(.vertical, 16)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(Color.orange.opacity(0.1))
            )
            
            HStack(spacing: 0) {
                // Channel Strips
                ScrollView(.horizontal, showsIndicators: false) {
                    HStack(spacing: 8) {
                        ForEach(Array(channelStrips.enumerated()), id: \.element.id) { index, channel in
                            ChannelStripView(
                                channel: $channelStrips[index],
                                isSelected: selectedChannel == index,
                                isSoloed: soloedChannels.contains(index),
                                mixerMode: mixerMode,
                                meteringMode: meteringMode,
                                onSelect: { selectChannel(index) },
                                onSolo: { toggleSolo(index) },
                                onMute: { toggleMute(index) },
                                onParameterChange: { param, value in
                                    updateChannelParameter(channel: index, param: param, value: value)
                                }
                            )
                        }
                    }
                    .padding(.horizontal, 20)
                }
                .frame(height: 480)
                
                // Master Section
                VStack(spacing: 0) {
                    // Send Buses (when in sends mode)
                    if mixerMode == .sends {
                        VStack(spacing: 8) {
                            Text("Send Buses")
                                .font(.system(size: 12, weight: .bold))
                                .foregroundColor(.primary)
                            
                            ForEach(Array(sendBuses.enumerated()), id: \.element.id) { index, bus in
                                SendBusView(
                                    bus: $sendBuses[index],
                                    onParameterChange: { param, value in
                                        updateSendBusParameter(bus: index, param: param, value: value)
                                    }
                                )
                            }
                        }
                        .padding(.horizontal, 12)
                        .padding(.vertical, 16)
                        .frame(height: 200)
                    }
                    
                    Spacer()
                    
                    // Master Strip
                    MasterStripView(
                        masterBus: $masterBus,
                        meteringMode: meteringMode,
                        onParameterChange: { param, value in
                            updateMasterParameter(param: param, value: value)
                        }
                    )
                    .padding(.horizontal, 12)
                    .padding(.vertical, 16)
                }
                .frame(width: 180)
                .background(Color.gray.opacity(0.05))
            }
            
            // Transport & Global Controls
            MixTransportView(
                onClearSolo: { clearAllSolo() },
                onClearMute: { clearAllMute() },
                onResetLevels: { resetAllLevels() }
            )
            .padding(.horizontal, 20)
            .padding(.bottom, 20)
        }
        .frame(width: 1000, height: 700)
        .background(
            RoundedRectangle(cornerRadius: 16)
                .fill(Color.white)
                .shadow(color: Color.black.opacity(0.3), radius: 12, x: 0, y: 8)
        )
        .sheet(isPresented: $showingFXLibrary) {
            FXLibrarySheet(
                onEffectSelect: { effect in
                    addEffectToChannel(effect: effect)
                }
            )
        }
        .sheet(isPresented: $showingMasterEQ) {
            MasterEQSheet(
                onEQUpdate: { eqSettings in
                    updateMasterEQ(settings: eqSettings)
                }
            )
        }
        .onAppear {
            initializeMixer()
            startMeteringTimer()
        }
    }
    
    // MARK: - Helper Functions
    private func initializeMixer() {
        // Initialize 8 channel strips
        channelStrips = (0..<8).map { index in
            var channel = ChannelStrip(id: index, name: "", color: InstrumentColor.allCases[index])
            channel.insertFX = (0..<4).map { InsertSlot(id: $0) }
            return channel
        }
        
        // Initialize master bus
        masterBus.insertFX = (0..<4).map { InsertSlot(id: $0) }
        
        // Initialize send buses
        sendBuses = [
            SendBus(id: "A", name: "A", insertFX: (0..<2).map { InsertSlot(id: $0) }),
            SendBus(id: "B", name: "B", insertFX: (0..<2).map { InsertSlot(id: $0) }),
            SendBus(id: "C", name: "C", insertFX: (0..<2).map { InsertSlot(id: $0) }),
            SendBus(id: "D", name: "D", insertFX: (0..<2).map { InsertSlot(id: $0) })
        ]
    }
    
    private func startMeteringTimer() {
        Timer.scheduledTimer(withTimeInterval: 1.0/30.0, repeats: true) { _ in
            updateMeterLevels()
        }
    }
    
    private func updateMeterLevels() {
        // Simulate metering data
        for i in 0..<channelStrips.count {
            channelStrips[i].level = Float.random(in: 0.0...channelStrips[i].volume)
            channelStrips[i].peak = max(channelStrips[i].peak * 0.95, channelStrips[i].level)
            channelStrips[i].rms = channelStrips[i].level * 0.7
        }
        
        masterBus.level = Float.random(in: 0.0...masterBus.volume)
        masterBus.peak = max(masterBus.peak * 0.95, masterBus.level)
        masterBus.rms = masterBus.level * 0.7
        
        for i in 0..<sendBuses.count {
            sendBuses[i].level = Float.random(in: 0.0...sendBuses[i].volume)
        }
    }
    
    private func selectChannel(_ index: Int) {
        selectedChannel = selectedChannel == index ? nil : index
    }
    
    private func toggleSolo(_ index: Int) {
        if soloedChannels.contains(index) {
            soloedChannels.remove(index)
        } else {
            soloedChannels.insert(index)
        }
        channelStrips[index].isSoloed = soloedChannels.contains(index)
    }
    
    private func toggleMute(_ index: Int) {
        channelStrips[index].isMuted.toggle()
    }
    
    private func clearAllSolo() {
        soloedChannels.removeAll()
        for i in 0..<channelStrips.count {
            channelStrips[i].isSoloed = false
        }
    }
    
    private func clearAllMute() {
        for i in 0..<channelStrips.count {
            channelStrips[i].isMuted = false
        }
    }
    
    private func resetAllLevels() {
        for i in 0..<channelStrips.count {
            channelStrips[i].volume = 0.75
            channelStrips[i].pan = 0.0
            channelStrips[i].gain = 0.0
        }
        masterBus.volume = 0.8
        masterBus.pan = 0.0
    }
    
    private func updateChannelParameter(channel: Int, param: String, value: Float) {
        // TODO: Send to synth controller
        // synthController.setChannelParameter(channel: channel, param: param, value: value)
    }
    
    private func updateMasterParameter(param: String, value: Float) {
        // TODO: Send to synth controller
        // synthController.setMasterParameter(param: param, value: value)
    }
    
    private func updateSendBusParameter(bus: Int, param: String, value: Float) {
        // TODO: Send to synth controller
        // synthController.setSendBusParameter(bus: bus, param: param, value: value)
    }
    
    private func addEffectToChannel(effect: InsertEffect) {
        guard let channelIndex = selectedChannel else { return }
        
        // Find first empty slot
        if let emptySlot = channelStrips[channelIndex].insertFX.firstIndex(where: { $0.isEmpty }) {
            channelStrips[channelIndex].insertFX[emptySlot].effect = effect
        }
        
        showingFXLibrary = false
    }
    
    private func updateMasterEQ(settings: EQSettings) {
        // TODO: Update master EQ
        showingMasterEQ = false
    }
}

// MARK: - Mix Page Header
struct MixPageHeader: View {
    @Binding var mixerMode: MixPage.MixerMode
    @Binding var meteringMode: MixPage.MeteringMode
    let soloCount: Int
    let onShowFXLibrary: () -> Void
    let onShowMasterEQ: () -> Void
    
    var body: some View {
        HStack {
            // Title
            HStack(spacing: 12) {
                Circle()
                    .fill(Color.orange)
                    .frame(width: 32, height: 32)
                    .overlay(
                        Image(systemName: "slider.horizontal.3")
                            .font(.system(size: 16))
                            .foregroundColor(.white)
                    )
                
                VStack(alignment: .leading, spacing: 2) {
                    Text("Mix Console")
                        .font(.system(size: 16, weight: .bold))
                        .foregroundColor(.primary)
                    
                    Text("8-track mixing desk")
                        .font(.system(size: 12))
                        .foregroundColor(.secondary)
                }
            }
            
            Spacer()
            
            // Mode Picker
            Picker("Mixer Mode", selection: $mixerMode) {
                ForEach(MixPage.MixerMode.allCases, id: \.self) { mode in
                    Text(mode.rawValue).tag(mode)
                }
            }
            .pickerStyle(SegmentedPickerStyle())
            .frame(width: 240)
            
            Spacer()
            
            // Solo indicator
            if soloCount > 0 {
                HStack(spacing: 4) {
                    Circle()
                        .fill(Color.yellow)
                        .frame(width: 8, height: 8)
                    Text("SOLO (\(soloCount))")
                        .font(.system(size: 10, weight: .bold))
                        .foregroundColor(.yellow)
                }
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(
                    RoundedRectangle(cornerRadius: 8)
                        .fill(Color.yellow.opacity(0.1))
                )
            }
            
            // Actions
            HStack(spacing: 8) {
                Button(action: onShowFXLibrary) {
                    HStack(spacing: 4) {
                        Image(systemName: "waveform.path.ecg")
                            .font(.system(size: 12))
                        Text("FX")
                            .font(.system(size: 12, weight: .medium))
                    }
                    .foregroundColor(.orange)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 6)
                    .background(
                        RoundedRectangle(cornerRadius: 16)
                            .fill(Color.orange.opacity(0.1))
                    )
                }
                .buttonStyle(PlainButtonStyle())
                
                Button(action: onShowMasterEQ) {
                    HStack(spacing: 4) {
                        Image(systemName: "slider.horizontal.3")
                            .font(.system(size: 12))
                        Text("EQ")
                            .font(.system(size: 12, weight: .medium))
                    }
                    .foregroundColor(.orange)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 6)
                    .background(
                        RoundedRectangle(cornerRadius: 16)
                            .fill(Color.orange.opacity(0.1))
                    )
                }
                .buttonStyle(PlainButtonStyle())
            }
        }
    }
}

// MARK: - Channel Strip View
struct ChannelStripView: View {
    @Binding var channel: ChannelStrip
    let isSelected: Bool
    let isSoloed: Bool
    let mixerMode: MixPage.MixerMode
    let meteringMode: MixPage.MeteringMode
    let onSelect: () -> Void
    let onSolo: () -> Void
    let onMute: () -> Void
    let onParameterChange: (String, Float) -> Void
    
    var body: some View {
        VStack(spacing: 8) {
            // Channel Header
            ChannelHeaderView(
                channel: channel,
                isSelected: isSelected,
                onSelect: onSelect
            )
            
            // Insert FX (when in FX mode)
            if mixerMode == .fx {
                InsertFXSection(
                    insertSlots: $channel.insertFX,
                    channelColor: channel.color.pastelColor
                )
                .frame(height: 120)
            }
            
            // EQ Section (when in mix mode)
            if mixerMode == .mix {
                ChannelEQSection(
                    highpass: Binding(
                        get: { channel.highpass },
                        set: { 
                            channel.highpass = $0
                            onParameterChange("highpass", $0)
                        }
                    ),
                    lowpass: Binding(
                        get: { channel.lowpass },
                        set: { 
                            channel.lowpass = $0
                            onParameterChange("lowpass", $0)
                        }
                    ),
                    channelColor: channel.color.pastelColor
                )
                .frame(height: 80)
            }
            
            // Send Section (when in sends mode)
            if mixerMode == .sends {
                ChannelSendsSection(
                    sendA: Binding(
                        get: { channel.sendA },
                        set: { 
                            channel.sendA = $0
                            onParameterChange("sendA", $0)
                        }
                    ),
                    sendB: Binding(
                        get: { channel.sendB },
                        set: { 
                            channel.sendB = $0
                            onParameterChange("sendB", $0)
                        }
                    ),
                    sendC: Binding(
                        get: { channel.sendC },
                        set: { 
                            channel.sendC = $0
                            onParameterChange("sendC", $0)
                        }
                    ),
                    sendD: Binding(
                        get: { channel.sendD },
                        set: { 
                            channel.sendD = $0
                            onParameterChange("sendD", $0)
                        }
                    ),
                    channelColor: channel.color.pastelColor
                )
                .frame(height: 120)
            }
            
            Spacer()
            
            // Pan Control
            VStack(spacing: 4) {
                Text("PAN")
                    .font(.system(size: 8))
                    .foregroundColor(.secondary)
                
                PanKnob(
                    value: Binding(
                        get: { channel.pan },
                        set: { 
                            channel.pan = $0
                            onParameterChange("pan", $0)
                        }
                    ),
                    color: channel.color.pastelColor
                )
                .frame(width: 32, height: 32)
                
                Text(panString(channel.pan))
                    .font(.system(size: 8))
                    .foregroundColor(.primary)
            }
            
            // Level Meter
            ChannelMeter(
                level: channel.level,
                peak: channel.peak,
                rms: channel.rms,
                meteringMode: meteringMode,
                isMuted: channel.isMuted,
                isSoloed: isSoloed
            )
            .frame(width: 20, height: 120)
            
            // Volume Fader
            VStack(spacing: 4) {
                Text("\(Int(channel.volumeDb))dB")
                    .font(.system(size: 8))
                    .foregroundColor(.primary)
                
                VerticalSlider(
                    value: Binding(
                        get: { channel.volume },
                        set: { 
                            channel.volume = $0
                            onParameterChange("volume", $0)
                        }
                    ),
                    range: 0.0...1.0,
                    color: channel.color.pastelColor
                )
                .frame(width: 30, height: 100)
                
                Text("VOL")
                    .font(.system(size: 8))
                    .foregroundColor(.secondary)
            }
            
            // Mute/Solo Buttons
            HStack(spacing: 4) {
                Button(action: onMute) {
                    Text("M")
                        .font(.system(size: 10, weight: .bold))
                        .foregroundColor(channel.isMuted ? .white : .red)
                        .frame(width: 24, height: 20)
                        .background(
                            RoundedRectangle(cornerRadius: 4)
                                .fill(channel.isMuted ? Color.red : Color.red.opacity(0.1))
                        )
                }
                .buttonStyle(PlainButtonStyle())
                
                Button(action: onSolo) {
                    Text("S")
                        .font(.system(size: 10, weight: .bold))
                        .foregroundColor(isSoloed ? .black : .yellow)
                        .frame(width: 24, height: 20)
                        .background(
                            RoundedRectangle(cornerRadius: 4)
                                .fill(isSoloed ? Color.yellow : Color.yellow.opacity(0.1))
                        )
                }
                .buttonStyle(PlainButtonStyle())
            }
        }
        .frame(width: 80)
        .padding(.vertical, 8)
        .padding(.horizontal, 4)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(isSelected ? channel.color.pastelColor.opacity(0.1) : Color.clear)
                .stroke(isSelected ? channel.color.pastelColor : Color.gray.opacity(0.2), lineWidth: 1)
        )
    }
    
    private func panString(_ pan: Float) -> String {
        if abs(pan) < 0.05 {
            return "C"
        } else if pan > 0 {
            return "R\(Int(pan * 100))"
        } else {
            return "L\(Int(abs(pan) * 100))"
        }
    }
}

// MARK: - Channel Header View
struct ChannelHeaderView: View {
    let channel: ChannelStrip
    let isSelected: Bool
    let onSelect: () -> Void
    
    var body: some View {
        Button(action: onSelect) {
            VStack(spacing: 4) {
                Circle()
                    .fill(channel.color.pastelColor)
                    .frame(width: 20, height: 20)
                    .overlay(
                        Text("\(channel.id + 1)")
                            .font(.system(size: 10, weight: .bold))
                            .foregroundColor(.white)
                    )
                
                Text(channel.displayName)
                    .font(.system(size: 9, weight: .medium))
                    .foregroundColor(.primary)
                    .lineLimit(2)
                    .multilineTextAlignment(.center)
                    .frame(height: 24)
            }
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - Insert FX Section
struct InsertFXSection: View {
    @Binding var insertSlots: [InsertSlot]
    let channelColor: Color
    
    var body: some View {
        VStack(spacing: 4) {
            Text("INSERT FX")
                .font(.system(size: 8, weight: .bold))
                .foregroundColor(.secondary)
            
            VStack(spacing: 2) {
                ForEach(Array(insertSlots.enumerated()), id: \.offset) { index, slot in
                    InsertSlotView(
                        slot: $insertSlots[index],
                        slotNumber: index + 1,
                        channelColor: channelColor
                    )
                }
            }
        }
    }
}

// MARK: - Insert Slot View
struct InsertSlotView: View {
    @Binding var slot: InsertSlot
    let slotNumber: Int
    let channelColor: Color
    
    var body: some View {
        HStack(spacing: 4) {
            Text("\(slotNumber)")
                .font(.system(size: 8))
                .foregroundColor(.secondary)
                .frame(width: 8)
            
            if let effect = slot.effect {
                HStack(spacing: 2) {
                    Image(systemName: effect.type.icon)
                        .font(.system(size: 8))
                        .foregroundColor(effect.type.color)
                    
                    Text(effect.name)
                        .font(.system(size: 8))
                        .foregroundColor(.primary)
                        .lineLimit(1)
                }
                .padding(.horizontal, 4)
                .padding(.vertical: 2)
                .background(
                    RoundedRectangle(cornerRadius: 3)
                        .fill(effect.type.color.opacity(0.1))
                )
                
                Button(action: { slot.bypass.toggle() }) {
                    Image(systemName: slot.bypass ? "power" : "power.circle.fill")
                        .font(.system(size: 8))
                        .foregroundColor(slot.bypass ? .secondary : .green)
                }
                .buttonStyle(PlainButtonStyle())
            } else {
                Rectangle()
                    .fill(Color.gray.opacity(0.1))
                    .frame(height: 16)
                    .overlay(
                        Text("Empty")
                            .font(.system(size: 7))
                            .foregroundColor(.secondary)
                    )
            }
        }
        .frame(height: 18)
    }
}

// MARK: - Channel EQ Section
struct ChannelEQSection: View {
    @Binding var highpass: Float
    @Binding var lowpass: Float
    let channelColor: Color
    
    var body: some View {
        VStack(spacing: 8) {
            Text("EQ")
                .font(.system(size: 8, weight: .bold))
                .foregroundColor(.secondary)
            
            VStack(spacing: 8) {
                // High Pass
                VStack(spacing: 4) {
                    Text("HP")
                        .font(.system(size: 8))
                        .foregroundColor(.secondary)
                    
                    RotaryKnob(
                        value: $highpass,
                        range: 0.0...1.0,
                        color: channelColor
                    )
                    .frame(width: 24, height: 24)
                    
                    Text("\(Int(20 + highpass * 980))Hz")
                        .font(.system(size: 7))
                        .foregroundColor(.primary)
                }
                
                // Low Pass
                VStack(spacing: 4) {
                    Text("LP")
                        .font(.system(size: 8))
                        .foregroundColor(.secondary)
                    
                    RotaryKnob(
                        value: $lowpass,
                        range: 0.0...1.0,
                        color: channelColor
                    )
                    .frame(width: 24, height: 24)
                    
                    Text("\(Int(1000 + lowpass * 19000))Hz")
                        .font(.system(size: 7))
                        .foregroundColor(.primary)
                }
            }
        }
    }
}

// MARK: - Channel Sends Section
struct ChannelSendsSection: View {
    @Binding var sendA: Float
    @Binding var sendB: Float
    @Binding var sendC: Float
    @Binding var sendD: Float
    let channelColor: Color
    
    var body: some View {
        VStack(spacing: 4) {
            Text("SENDS")
                .font(.system(size: 8, weight: .bold))
                .foregroundColor(.secondary)
            
            VStack(spacing: 6) {
                SendKnob(name: "A", value: $sendA, color: .red)
                SendKnob(name: "B", value: $sendB, color: .green)
                SendKnob(name: "C", value: $sendC, color: .blue)
                SendKnob(name: "D", value: $sendD, color: .purple)
            }
        }
    }
}

// MARK: - Send Knob
struct SendKnob: View {
    let name: String
    @Binding var value: Float
    let color: Color
    
    var body: some View {
        HStack(spacing: 4) {
            Text(name)
                .font(.system(size: 8, weight: .bold))
                .foregroundColor(color)
                .frame(width: 8)
            
            RotaryKnob(
                value: $value,
                range: 0.0...1.0,
                color: color
            )
            .frame(width: 20, height: 20)
            
            Text("\(Int(value * 100))")
                .font(.system(size: 7))
                .foregroundColor(.primary)
                .frame(width: 20)
        }
    }
}

// MARK: - Channel Meter
struct ChannelMeter: View {
    let level: Float
    let peak: Float
    let rms: Float
    let meteringMode: MixPage.MeteringMode
    let isMuted: Bool
    let isSoloed: Bool
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Background
                Rectangle()
                    .fill(Color.black)
                    .cornerRadius(2)
                
                // Level bars
                VStack(spacing: 1) {
                    ForEach(0..<20, id: \.self) { segment in
                        let segmentLevel = 1.0 - Float(segment) / 20.0
                        let isActive = level >= segmentLevel
                        let isPeakSegment = peak >= segmentLevel && meteringMode != .rms
                        let isRMSSegment = rms >= segmentLevel && meteringMode == .rms
                        
                        Rectangle()
                            .fill(meterColor(for: segmentLevel, isActive: isActive || isPeakSegment || isRMSSegment))
                            .opacity(isActive || isPeakSegment || isRMSSegment ? 1.0 : 0.2)
                    }
                }
                .padding(1)
                
                // Mute/Solo overlay
                if isMuted {
                    Rectangle()
                        .fill(Color.red.opacity(0.7))
                        .overlay(
                            Text("M")
                                .font(.system(size: 8, weight: .bold))
                                .foregroundColor(.white)
                        )
                } else if isSoloed {
                    Rectangle()
                        .fill(Color.yellow.opacity(0.7))
                        .overlay(
                            Text("S")
                                .font(.system(size: 8, weight: .bold))
                                .foregroundColor(.black)
                        )
                }
            }
        }
    }
    
    private func meterColor(for level: Float, isActive: Bool) -> Color {
        if level > 0.9 {
            return .red
        } else if level > 0.7 {
            return .yellow
        } else {
            return .green
        }
    }
}

// MARK: - Master Strip View
struct MasterStripView: View {
    @Binding var masterBus: MasterBus
    let meteringMode: MixPage.MeteringMode
    let onParameterChange: (String, Float) -> Void
    
    var body: some View {
        VStack(spacing: 12) {
            // Master Header
            Text("MASTER")
                .font(.system(size: 12, weight: .bold))
                .foregroundColor(.primary)
            
            // Limiter Section
            VStack(spacing: 8) {
                Text("LIMITER")
                    .font(.system(size: 8, weight: .bold))
                    .foregroundColor(.secondary)
                
                Toggle("Enable", isOn: $masterBus.limiter.enabled)
                    .font(.system(size: 8))
                    .toggleStyle(SwitchToggleStyle(tint: .orange))
                
                if masterBus.limiter.enabled {
                    VStack(spacing: 4) {
                        Text("Thresh: \(String(format: "%.1f", masterBus.limiter.threshold))dB")
                            .font(.system(size: 7))
                            .foregroundColor(.primary)
                        
                        Slider(value: $masterBus.limiter.threshold, in: -12.0...0.0)
                            .accentColor(.orange)
                            .controlSize(.mini)
                    }
                }
            }
            .padding(.vertical, 8)
            .padding(.horizontal, 8)
            .background(
                RoundedRectangle(cornerRadius: 6)
                    .fill(Color.orange.opacity(0.05))
                    .stroke(Color.orange.opacity(0.2), lineWidth: 1)
            )
            
            Spacer()
            
            // Master Pan
            VStack(spacing: 4) {
                Text("PAN")
                    .font(.system(size: 8))
                    .foregroundColor(.secondary)
                
                PanKnob(
                    value: Binding(
                        get: { masterBus.pan },
                        set: { 
                            masterBus.pan = $0
                            onParameterChange("pan", $0)
                        }
                    ),
                    color: .orange
                )
                .frame(width: 40, height: 40)
                
                Text(panString(masterBus.pan))
                    .font(.system(size: 8))
                    .foregroundColor(.primary)
            }
            
            // Master Meter
            ChannelMeter(
                level: masterBus.level,
                peak: masterBus.peak,
                rms: masterBus.rms,
                meteringMode: meteringMode,
                isMuted: masterBus.isMuted,
                isSoloed: false
            )
            .frame(width: 30, height: 100)
            
            // Master Volume
            VStack(spacing: 4) {
                Text("\(Int(masterBus.volumeDb))dB")
                    .font(.system(size: 10, weight: .bold))
                    .foregroundColor(.primary)
                
                VerticalSlider(
                    value: Binding(
                        get: { masterBus.volume },
                        set: { 
                            masterBus.volume = $0
                            onParameterChange("volume", $0)
                        }
                    ),
                    range: 0.0...1.2,
                    color: .orange
                )
                .frame(width: 40, height: 120)
                
                Text("MASTER")
                    .font(.system(size: 8))
                    .foregroundColor(.secondary)
            }
            
            // Master Mute
            Button(action: { masterBus.isMuted.toggle() }) {
                Text("MUTE")
                    .font(.system(size: 10, weight: .bold))
                    .foregroundColor(masterBus.isMuted ? .white : .red)
                    .frame(width: 50, height: 24)
                    .background(
                        RoundedRectangle(cornerRadius: 4)
                            .fill(masterBus.isMuted ? Color.red : Color.red.opacity(0.1))
                    )
            }
            .buttonStyle(PlainButtonStyle())
        }
    }
    
    private func panString(_ pan: Float) -> String {
        if abs(pan) < 0.05 {
            return "CENTER"
        } else if pan > 0 {
            return "R\(Int(pan * 100))"
        } else {
            return "L\(Int(abs(pan) * 100))"
        }
    }
}

// MARK: - Send Bus View
struct SendBusView: View {
    @Binding var bus: SendBus
    let onParameterChange: (String, Float) -> Void
    
    var body: some View {
        HStack(spacing: 8) {
            Text(bus.displayName)
                .font(.system(size: 10, weight: .bold))
                .foregroundColor(.primary)
                .frame(width: 40)
            
            VerticalSlider(
                value: Binding(
                    get: { bus.volume },
                    set: { 
                        bus.volume = $0
                        onParameterChange("volume", $0)
                    }
                ),
                range: 0.0...1.0,
                color: .purple
            )
            .frame(width: 20, height: 30)
            
            Button(action: { bus.isMuted.toggle() }) {
                Text("M")
                    .font(.system(size: 8, weight: .bold))
                    .foregroundColor(bus.isMuted ? .white : .red)
                    .frame(width: 16, height: 16)
                    .background(
                        RoundedRectangle(cornerRadius: 2)
                            .fill(bus.isMuted ? Color.red : Color.red.opacity(0.1))
                    )
            }
            .buttonStyle(PlainButtonStyle())
        }
    }
}

// MARK: - Mix Transport View
struct MixTransportView: View {
    let onClearSolo: () -> Void
    let onClearMute: () -> Void
    let onResetLevels: () -> Void
    
    var body: some View {
        HStack {
            Text("Mix Console • 8 Tracks")
                .font(.system(size: 12, weight: .medium))
                .foregroundColor(.secondary)
            
            Spacer()
            
            HStack(spacing: 8) {
                Button(action: onClearSolo) {
                    Text("Clear Solo")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.yellow)
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(
                            RoundedRectangle(cornerRadius: 12)
                                .fill(Color.yellow.opacity(0.1))
                        )
                }
                .buttonStyle(PlainButtonStyle())
                
                Button(action: onClearMute) {
                    Text("Clear Mute")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.red)
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(
                            RoundedRectangle(cornerRadius: 12)
                                .fill(Color.red.opacity(0.1))
                        )
                }
                .buttonStyle(PlainButtonStyle())
                
                Button(action: onResetLevels) {
                    Text("Reset Levels")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.blue)
                        .padding(.horizontal: 8)
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
}

// MARK: - Custom Controls
struct PanKnob: View {
    @Binding var value: Float // -1 to 1
    let color: Color
    
    var body: some View {
        RotaryKnob(
            value: Binding(
                get: { (value + 1) / 2 }, // Convert to 0-1 range
                set: { value = $0 * 2 - 1 } // Convert back to -1 to 1
            ),
            range: 0.0...1.0,
            color: color
        )
    }
}

struct VerticalSlider: View {
    @Binding var value: Float
    let range: ClosedRange<Float>
    let color: Color
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Track
                Rectangle()
                    .fill(Color.gray.opacity(0.3))
                    .frame(width: 4)
                
                // Thumb
                Rectangle()
                    .fill(color)
                    .frame(width: geometry.size.width, height: 8)
                    .position(
                        x: geometry.size.width / 2,
                        y: geometry.size.height * CGFloat(1.0 - (value - range.lowerBound) / (range.upperBound - range.lowerBound))
                    )
                    .gesture(
                        DragGesture(minimumDistance: 0)
                            .onChanged { gesture in
                                let newValue = 1.0 - Float(gesture.location.y / geometry.size.height)
                                value = max(range.lowerBound, min(range.upperBound, range.lowerBound + newValue * (range.upperBound - range.lowerBound)))
                            }
                    )
            }
        }
    }
}

struct RotaryKnob: View {
    @Binding var value: Float
    let range: ClosedRange<Float>
    let color: Color
    
    var body: some View {
        ZStack {
            Circle()
                .stroke(Color.gray.opacity(0.3), lineWidth: 2)
            
            Circle()
                .trim(from: 0, to: CGFloat((value - range.lowerBound) / (range.upperBound - range.lowerBound)))
                .stroke(color, style: StrokeStyle(lineWidth: 2, lineCap: .round))
                .rotationEffect(.degrees(-90))
            
            Circle()
                .fill(color)
                .frame(width: 4, height: 4)
        }
        .gesture(
            DragGesture()
                .onChanged { gesture in
                    let angle = atan2(gesture.location.y - 20, gesture.location.x - 20)
                    let normalizedAngle = (angle + .pi) / (2 * .pi)
                    let newValue = range.lowerBound + Float(normalizedAngle) * (range.upperBound - range.lowerBound)
                    value = max(range.lowerBound, min(range.upperBound, newValue))
                }
        )
    }
}

// MARK: - FX Library Sheet
struct FXLibrarySheet: View {
    let onEffectSelect: (InsertEffect) -> Void
    
    @Environment(\.presentationMode) var presentationMode
    
    var body: some View {
        VStack {
            Text("FX Library")
                .font(.headline)
            
            LazyVGrid(columns: [
                GridItem(.flexible()),
                GridItem(.flexible()),
                GridItem(.flexible())
            ], spacing: 16) {
                ForEach(InsertEffect.EffectType.allCases, id: \.self) { effectType in
                    Button(action: {
                        let effect = InsertEffect(
                            id: UUID().uuidString,
                            name: effectType.rawValue,
                            type: effectType
                        )
                        onEffectSelect(effect)
                    }) {
                        VStack(spacing: 8) {
                            Image(systemName: effectType.icon)
                                .font(.system(size: 24))
                                .foregroundColor(effectType.color)
                            
                            Text(effectType.rawValue)
                                .font(.system(size: 12, weight: .medium))
                                .foregroundColor(.primary)
                        }
                        .frame(width: 80, height: 60)
                        .background(
                            RoundedRectangle(cornerRadius: 8)
                                .fill(effectType.color.opacity(0.1))
                        )
                    }
                    .buttonStyle(PlainButtonStyle())
                }
            }
            
            Button("Cancel") {
                presentationMode.wrappedValue.dismiss()
            }
            .padding(.top)
        }
        .padding()
        .frame(width: 400, height: 500)
    }
}

// MARK: - Master EQ Sheet
struct EQSettings {
    var low: Float = 0.0
    var lowMid: Float = 0.0
    var highMid: Float = 0.0
    var high: Float = 0.0
}

struct MasterEQSheet: View {
    let onEQUpdate: (EQSettings) -> Void
    
    @State private var eqSettings = EQSettings()
    @Environment(\.presentationMode) var presentationMode
    
    var body: some View {
        VStack {
            Text("Master EQ")
                .font(.headline)
            
            Text("Master EQ controls coming soon...")
                .foregroundColor(.secondary)
            
            Button("Close") {
                onEQUpdate(eqSettings)
            }
        }
        .padding()
        .frame(width: 400, height: 300)
    }
}