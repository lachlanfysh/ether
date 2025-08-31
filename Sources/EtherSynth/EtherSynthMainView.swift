import SwiftUI

// MARK: - Main EtherSynth View (New Architecture)
struct EtherSynthMainView: View {
    @EnvironmentObject var synthController: SynthController
    @State private var currentScreen: ScreenType = .instrument
    @State private var showingOverlay: OverlayType? = nil
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Background
                Rectangle()
                    .fill(Color(hex: "F8F9FA"))
                    .ignoresSafeArea()
                
                // Main Layout Structure
                VStack(spacing: 0) {
                    // Header
                    HeaderView()
                        .frame(height: 28)
                    
                    // Main Content Area
                    HStack(spacing: 0) {
                        // Side Menu
                        SideMenuView(currentScreen: $currentScreen)
                            .frame(width: 120)
                        
                        // Content Area
                        ContentAreaView(currentScreen: currentScreen)
                            .frame(maxWidth: .infinity)
                    }
                    .frame(height: 268)
                    
                    // Footer
                    FooterView()
                        .frame(height: 24)
                }
                
                // Single Active Overlay System
                if let overlayType = showingOverlay {
                    OverlayContainerView(overlayType: overlayType, onClose: {
                        showingOverlay = nil
                    })
                }
            }
        }
        .frame(width: 960, height: 320)
        .gesture(
            // SmartKnob simulation via vertical drag
            DragGesture()
                .onChanged { value in
                    // Convert vertical drag to parameter adjustment
                    let delta = Float(-value.translation.height * 0.001) // Negative for intuitive direction
                    synthController.handleSmartKnobRotation(delta)
                }
        )
        .onReceive(synthController.hardwareButtonPublisher) { buttonPress in
            handleHardwareButton(buttonPress)
        }
    }
    
    private func handleHardwareButton(_ buttonPress: HardwareButtonPress) {
        switch buttonPress.buttonType {
        case .topButton(let index):
            handleTopButton(index)
        case .bottomButton(let index):
            handleBottomButton(index)
        }
    }
    
    private func handleTopButton(_ index: Int) {
        switch index {
        case 0: synthController.play() // PLAY
        case 1: synthController.record() // REC
        case 2: synthController.uiIsWriteEnabled.toggle() // WRITE
        case 3: synthController.uiIsShiftHeld.toggle() // SHIFT
        case 4: currentScreen = .instrument // INST
        case 5: showingOverlay = .sound // SOUND
        case 6: showingOverlay = .pattern // PATT
        case 7: 
            if synthController.uiIsShiftHeld {
                synthController.triggerClone() // Long press COPY = CLONE
            } else {
                synthController.triggerCopy() // COPY
            }
        case 8:
            if synthController.uiIsShiftHeld {
                synthController.triggerDelete() // Long press CUT = DELETE
            } else {
                synthController.triggerCut() // CUT
            }
        case 9: synthController.toggleStamp(.accent) // ACC
        case 10: synthController.toggleStamp(.ratchet) // RTCH
        case 11: synthController.toggleStamp(.tie) // TIE
        case 12: synthController.toggleStamp(.micro) // MICRO
        case 13: showingOverlay = .fx // FX
        case 14: synthController.triggerNudge() // NUDGE
        default: break
        }
    }
    
    private func handleBottomButton(_ index: Int) {
        switch currentScreen {
        case .pattern:
            // Step entry/editing (1-16)
            synthController.toggleStep(index)
            
        case .instrument:
            // Parameter selection/editing (1-16)
            synthController.handleBottomButtonPress(index, isShiftHeld: synthController.uiIsShiftHeld)
            
        case .song:
            // Pattern selection/navigation (A-P patterns)
            if index < 16 {
                let patternID = String(Character(UnicodeScalar(65 + index)!)) // A-P
                synthController.switchPattern(patternID)
            }
            
        case .mix:
            // Mixer channel selection (1-16)
            synthController.selectMixerChannel(index)
        }
    }
}

// MARK: - Screen Types
enum ScreenType: String, CaseIterable {
    case pattern = "Pattern"
    case instrument = "Instrmt" 
    case song = "Song"
    case mix = "Mix"
}

enum OverlayType: String {
    case sound = "Sound"
    case pattern = "Pattern"
    case fx = "FX"
}

// MARK: - Header View
struct HeaderView: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        HStack {
            // Left side - Pattern and status
            HStack(spacing: 12) {
                Text("PATTERN \(synthController.currentPatternID)")
                    .font(.system(size: 10, weight: .medium, design: .monospaced))
                    .foregroundColor(.primary)
                
                Text("BPM \(Int(synthController.uiBPM))")
                    .font(.system(size: 10, weight: .medium, design: .monospaced))
                    .foregroundColor(.primary)
                
                Text("I\(synthController.uiArmedInstrumentIndex + 1) Armed")
                    .font(.system(size: 10, weight: .medium, design: .monospaced))
                    .foregroundColor(.secondary)
            }
            
            Spacer()
            
            // Transport Controls
            HStack(spacing: 8) {
                Button(action: {
                    synthController.play()
                }) {
                    HStack(spacing: 2) {
                        Image(systemName: synthController.isPlaying ? "pause.fill" : "play.fill")
                        Text(synthController.isPlaying ? "STOP" : "PLAY")
                    }
                    .font(.system(size: 9, weight: .bold, design: .monospaced))
                    .foregroundColor(synthController.isPlaying ? .red : .green)
                    .padding(.horizontal, 8)
                    .padding(.vertical, 2)
                    .background(
                        RoundedRectangle(cornerRadius: 4)
                            .fill(synthController.isPlaying ? Color.red.opacity(0.1) : Color.green.opacity(0.1))
                    )
                }
                .buttonStyle(PlainButtonStyle())
                
                Button(action: {
                    synthController.record()
                }) {
                    HStack(spacing: 2) {
                        Circle()
                            .fill(synthController.isRecording ? Color.red : Color.gray)
                            .frame(width: 6, height: 6)
                        Text("REC")
                    }
                    .font(.system(size: 9, weight: .bold, design: .monospaced))
                    .foregroundColor(synthController.isRecording ? .red : .secondary)
                    .padding(.horizontal, 6)
                    .padding(.vertical, 2)
                    .background(
                        RoundedRectangle(cornerRadius: 4)
                            .fill(synthController.isRecording ? Color.red.opacity(0.1) : Color.clear)
                    )
                }
                .buttonStyle(PlainButtonStyle())
            }
            
            Spacer()
            
            // Right side - System status
            HStack(spacing: 12) {
                Text("CPU \(synthController.uiCpuUsage)%")
                    .font(.system(size: 10, weight: .medium, design: .monospaced))
                    .foregroundColor(.secondary)
                
                Text("V \(synthController.uiActiveVoices)")
                    .font(.system(size: 10, weight: .medium, design: .monospaced))
                    .foregroundColor(.secondary)
                
                if synthController.isRecording {
                    Text("REC")
                        .font(.system(size: 10, weight: .bold, design: .monospaced))
                        .foregroundColor(.red)
                        .background(
                            RoundedRectangle(cornerRadius: 3)
                                .fill(Color.red.opacity(0.1))
                                .padding(-2)
                        )
                }
            }
        }
        .padding(.horizontal, 16)
        .background(
            Rectangle()
                .fill(Color(hex: "E8EAED"))
        )
    }
}

// MARK: - Side Menu View
struct SideMenuView: View {
    @Binding var currentScreen: ScreenType
    
    var body: some View {
        VStack(spacing: 0) {
            ForEach(ScreenType.allCases, id: \.self) { screen in
                SideMenuTabView(
                    screen: screen,
                    isActive: currentScreen == screen,
                    onTap: { currentScreen = screen }
                )
            }
            Spacer()
        }
        .background(Color(hex: "F8F9FA"))
    }
}

// MARK: - Side Menu Tab View
struct SideMenuTabView: View {
    let screen: ScreenType
    let isActive: Bool
    let onTap: () -> Void
    
    var body: some View {
        Button(action: onTap) {
            HStack {
                Text(screen.rawValue)
                    .font(.system(size: 12, weight: isActive ? .semibold : .regular))
                    .foregroundColor(isActive ? .primary : .secondary)
                    .frame(maxWidth: .infinity, alignment: .leading)
                
                Spacer()
            }
            .padding(.horizontal, 16)
            .padding(.vertical, 12)
            .background(
                // Chrome-style tab joining effect
                Group {
                    if isActive {
                        HStack(spacing: 0) {
                            RoundedRectangle(cornerRadius: 8, style: .continuous)
                                .fill(Color.white)
                            Rectangle()
                                .fill(Color.white)
                                .frame(width: 8)
                        }
                    } else {
                        Color.clear
                    }
                }
            )
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - Content Area View
struct ContentAreaView: View {
    let currentScreen: ScreenType
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        Group {
            switch currentScreen {
            case .pattern:
                PatternScreenView()
            case .instrument:
                InstrumentScreenView()
            case .song:
                SongScreenView()
            case .mix:
                MixScreenView()
            }
        }
        .background(Color.white)
        .clipShape(RoundedRectangle(cornerRadius: 8))
        .shadow(color: .black.opacity(0.1), radius: 4, x: 2, y: 2)
        .padding(.trailing, 16)
        .padding(.vertical, 8)
    }
}

// MARK: - Footer View
struct FooterView: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        HStack {
            // Left side - Active stamps
            HStack(spacing: 8) {
                if synthController.activeStamps.accent {
                    StampIndicator(name: "ACC", isActive: true)
                }
                if synthController.activeStamps.ratchet > 0 {
                    StampIndicator(name: "RTCH×\(synthController.activeStamps.ratchet + 1)", isActive: true)
                }
                if synthController.activeStamps.tie {
                    StampIndicator(name: "TIE", isActive: true)
                }
                if synthController.activeStamps.micro != 0 {
                    let microText = synthController.activeStamps.micro > 0 ? "MICRO+" : "MICRO-"
                    StampIndicator(name: microText, isActive: true)
                }
            }
            
            Spacer()
            
            // Right side - Active tool
            if let tool = synthController.activeTool {
                HStack(spacing: 4) {
                    Text("Tool: \(tool.rawValue.uppercased())")
                        .font(.system(size: 10, weight: .medium, design: .monospaced))
                        .foregroundColor(.primary)
                    
                    if synthController.toolScope != .step {
                        Text("(\(synthController.toolScope.rawValue.uppercased()))")
                            .font(.system(size: 10, weight: .medium, design: .monospaced))
                            .foregroundColor(.secondary)
                    }
                    
                    Text("— PLAY to exit")
                        .font(.system(size: 10, weight: .regular, design: .monospaced))
                        .foregroundColor(.secondary)
                }
            }
        }
        .padding(.horizontal, 16)
        .background(
            Rectangle()
                .fill(Color(hex: "E8EAED"))
        )
    }
}

// MARK: - Stamp Indicator
struct StampIndicator: View {
    let name: String
    let isActive: Bool
    
    var body: some View {
        Text(name)
            .font(.system(size: 9, weight: .bold, design: .monospaced))
            .foregroundColor(isActive ? .white : .secondary)
            .padding(.horizontal, 6)
            .padding(.vertical, 2)
            .background(
                RoundedRectangle(cornerRadius: 4)
                    .fill(isActive ? Color.blue : Color.clear)
            )
    }
}

// MARK: - Overlay Container View
struct OverlayContainerView: View {
    let overlayType: OverlayType
    let onClose: () -> Void
    
    var body: some View {
        ZStack {
            // Semi-transparent background
            Color.black.opacity(0.3)
                .ignoresSafeArea()
                .onTapGesture {
                    onClose()
                }
            
            // Overlay content
            Group {
                switch overlayType {
                case .sound:
                    SoundOverlayView(onClose: onClose)
                case .pattern:
                    PatternOverlayView(onClose: onClose)
                case .fx:
                    FXOverlayView(onClose: onClose)
                }
            }
        }
    }
}

// MARK: - Extensions
extension Color {
    init(hex: String) {
        let hex = hex.trimmingCharacters(in: CharacterSet.alphanumerics.inverted)
        var int: UInt64 = 0
        Scanner(string: hex).scanHexInt64(&int)
        let a, r, g, b: UInt64
        switch hex.count {
        case 3: // RGB (12-bit)
            (a, r, g, b) = (255, (int >> 8) * 17, (int >> 4 & 0xF) * 17, (int & 0xF) * 17)
        case 6: // RGB (24-bit)
            (a, r, g, b) = (255, int >> 16, int >> 8 & 0xFF, int & 0xFF)
        case 8: // ARGB (32-bit)
            (a, r, g, b) = (int >> 24, int >> 16 & 0xFF, int >> 8 & 0xFF, int & 0xFF)
        default:
            (a, r, g, b) = (1, 1, 1, 0)
        }
        
        self.init(
            .sRGB,
            red: Double(r) / 255,
            green: Double(g) / 255,
            blue:  Double(b) / 255,
            opacity: Double(a) / 255
        )
    }
}