import SwiftUI

/**
 * Icon Library - Comprehensive SVG icon system with ColorBrewer palette
 * Provides scalable vector icons for all UI elements with semantic naming
 */

// MARK: - Icon Categories
enum IconCategory: String, CaseIterable {
    case transport = "Transport"
    case navigation = "Navigation"
    case synthesis = "Synthesis"
    case effects = "Effects"
    case editing = "Editing"
    case file = "File"
    case audio = "Audio"
    case midi = "MIDI"
    case interface = "Interface"
    case status = "Status"
}

// MARK: - Icon Library
struct IconLibrary {
    
    // MARK: - Transport Icons
    static let play = "play.fill"
    static let pause = "pause.fill"
    static let stop = "stop.fill"
    static let record = "record.circle"
    static let loop = "repeat"
    static let shuffle = "shuffle"
    static let skipForward = "forward.end.fill"
    static let skipBackward = "backward.end.fill"
    static let fastForward = "forward.fill"
    static let rewind = "backward.fill"
    static let metronome = "metronome"
    
    // MARK: - Navigation Icons
    static let timeline = "timeline.selection"
    static let mixer = "slider.horizontal.3"
    static let browser = "folder"
    static let settings = "gear"
    static let help = "questionmark.circle"
    static let home = "house"
    static let back = "chevron.left"
    static let forward = "chevron.right"
    static let up = "chevron.up"
    static let down = "chevron.down"
    static let expand = "chevron.down.circle"
    static let collapse = "chevron.up.circle"
    
    // MARK: - Synthesis Icons
    static let oscillator = "waveform"
    static let filter = "slider.horizontal.below.rectangle"
    static let envelope = "chart.line.uptrend.xyaxis"
    static let lfo = "sine.wave"
    static let noise = "waveform.path.ecg"
    static let fm = "waveform.path"
    static let wavetable = "waveform.circle"
    static let granular = "circle.hexagongrid"
    static let sampler = "waveform.and.mic"
    static let drum = "drum"
    static let synth = "pianokeys"
    static let modulation = "arrow.triangle.branch"
    
    // MARK: - Effects Icons
    static let reverb = "speaker.wave.3"
    static let delay = "arrow.clockwise"
    static let chorus = "waveform.path.badge.plus"
    static let distortion = "bolt"
    static let compressor = "waveform.path.ecg"
    static let equalizer = "slider.horizontal.3"
    static let phaser = "waveform.circle.fill"
    static let flanger = "waveform.circle"
    static let bitcrusher = "square.grid.3x3"
    static let limiter = "waveform.path.ecg.rectangle"
    
    // MARK: - Editing Icons
    static let cut = "scissors"
    static let copy = "doc.on.doc"
    static let paste = "doc.on.clipboard"
    static let delete = "trash"
    static let undo = "arrow.uturn.left"
    static let redo = "arrow.uturn.right"
    static let select = "selection.pin.in.out"
    static let move = "arrow.up.and.down.and.arrow.left.and.right"
    static let resize = "arrow.up.left.and.down.right.and.arrow.up.right.and.down.left"
    static let duplicate = "plus.square.on.square"
    static let quantize = "grid"
    
    // MARK: - File Icons
    static let newFile = "doc.badge.plus"
    static let openFile = "folder.badge.plus"
    static let saveFile = "square.and.arrow.down"
    static let exportFile = "square.and.arrow.up"
    static let importFile = "square.and.arrow.down.on.square"
    static let project = "folder.fill.badge.gear"
    static let preset = "star.circle"
    static let template = "doc.text.fill"
    
    // MARK: - Audio Icons
    static let waveform = "waveform"
    static let spectrum = "waveform.path.ecg"
    static let peak = "chart.bar"
    static let rms = "chart.line.downtrend.xyaxis"
    static let phase = "dial.max"
    static let stereo = "speaker.2"
    static let mono = "speaker"
    static let mute = "speaker.slash"
    static let solo = "speaker.wave.2.circle"
    static let input = "mic"
    static let output = "speaker.3"
    
    // MARK: - MIDI Icons
    static let midiIn = "cable.connector.horizontal"
    static let midiOut = "cable.connector"
    static let keyboard = "pianokeys"
    static let midiController = "gamecontroller"
    static let midiClock = "clock"
    static let velocity = "speedometer"
    static let noteOn = "music.note"
    static let noteOff = "music.note.slash"
    static let pitchBend = "tuningfork"
    static let modWheel = "dial.min"
    
    // MARK: - Interface Icons
    static let knob = "dial.max"
    static let slider = "slider.horizontal.3"
    static let button = "button.programmable"
    static let toggle = "switch.2"
    static let dropdown = "chevron.down.square"
    static let tab = "rectangle.3.offgrid"
    static let panel = "sidebar.left"
    static let overlay = "rectangle.inset.filled"
    static let popup = "rectangle.portrait.and.arrow.right"
    static let fullscreen = "arrow.up.left.and.arrow.down.right"
    
    // MARK: - Status Icons
    static let online = "circle.fill"
    static let offline = "circle"
    static let warning = "exclamationmark.triangle"
    static let error = "xmark.circle"
    static let success = "checkmark.circle"
    static let info = "info.circle"
    static let loading = "arrow.clockwise.circle"
    static let cpu = "cpu"
    static let memory = "memorychip"
    static let latency = "timer"
    
    // MARK: - Custom SVG Icons
    static func customIcon(name: String, size: CGFloat = 16, color: Color = .primary) -> some View {
        CustomSVGIcon(name: name, size: size, color: color)
    }
    
    // MARK: - Icon with Badge
    static func iconWithBadge(icon: String, badge: String? = nil, badgeColor: Color = .red) -> some View {
        IconWithBadge(icon: icon, badge: badge, badgeColor: badgeColor)
    }
    
    // MARK: - Animated Icons
    static func animatedIcon(icon: String, isAnimating: Bool = false) -> some View {
        AnimatedIcon(icon: icon, isAnimating: isAnimating)
    }
}

// MARK: - ColorBrewer Palette Integration
struct ColorBrewer {
    
    // Qualitative palettes for categorical data
    static let set1: [Color] = [
        Color(red: 0.894, green: 0.102, blue: 0.110),  // Red
        Color(red: 0.216, green: 0.494, blue: 0.722),  // Blue
        Color(red: 0.302, green: 0.686, blue: 0.290),  // Green
        Color(red: 0.596, green: 0.306, blue: 0.639),  // Purple
        Color(red: 1.000, green: 0.498, blue: 0.000),  // Orange
        Color(red: 1.000, green: 1.000, blue: 0.200),  // Yellow
        Color(red: 0.651, green: 0.337, blue: 0.157),  // Brown
        Color(red: 0.969, green: 0.506, blue: 0.749)   // Pink
    ]
    
    static let set2: [Color] = [
        Color(red: 0.400, green: 0.761, blue: 0.647),  // Teal
        Color(red: 0.988, green: 0.553, blue: 0.384),  // Orange
        Color(red: 0.553, green: 0.627, blue: 0.796),  // Blue
        Color(red: 0.906, green: 0.541, blue: 0.765),  // Pink
        Color(red: 0.651, green: 0.847, blue: 0.329),  // Green
        Color(red: 1.000, green: 0.851, blue: 0.184),  // Yellow
        Color(red: 0.898, green: 0.769, blue: 0.580),  // Beige
        Color(red: 0.702, green: 0.702, blue: 0.702)   // Gray
    ]
    
    static let pastel: [Color] = [
        Color(red: 0.984, green: 0.706, blue: 0.682),  // Light Red
        Color(red: 0.702, green: 0.871, blue: 0.412),  // Light Green
        Color(red: 0.988, green: 0.804, blue: 0.898),  // Light Pink
        Color(red: 0.851, green: 0.851, blue: 0.851),  // Light Gray
        Color(red: 0.678, green: 0.847, blue: 0.902),  // Light Blue
        Color(red: 1.000, green: 0.929, blue: 0.435),  // Light Yellow
        Color(red: 0.859, green: 0.859, blue: 0.753),  // Light Olive
        Color(red: 0.776, green: 0.690, blue: 0.835)   // Light Purple
    ]
    
    // Sequential palettes for ordered data
    static let blues: [Color] = [
        Color(red: 0.969, green: 0.984, blue: 1.000),
        Color(red: 0.871, green: 0.922, blue: 0.969),
        Color(red: 0.776, green: 0.859, blue: 0.937),
        Color(red: 0.620, green: 0.792, blue: 0.882),
        Color(red: 0.416, green: 0.682, blue: 0.839),
        Color(red: 0.255, green: 0.576, blue: 0.776),
        Color(red: 0.129, green: 0.443, blue: 0.709),
        Color(red: 0.031, green: 0.318, blue: 0.612),
        Color(red: 0.031, green: 0.188, blue: 0.420)
    ]
    
    static let greens: [Color] = [
        Color(red: 0.969, green: 0.988, blue: 0.961),
        Color(red: 0.898, green: 0.969, blue: 0.875),
        Color(red: 0.780, green: 0.914, blue: 0.753),
        Color(red: 0.631, green: 0.851, blue: 0.608),
        Color(red: 0.455, green: 0.769, blue: 0.463),
        Color(red: 0.255, green: 0.671, blue: 0.365),
        Color(red: 0.137, green: 0.545, blue: 0.271),
        Color(red: 0.000, green: 0.427, blue: 0.173),
        Color(red: 0.000, green: 0.267, blue: 0.106)
    ]
    
    // Diverging palettes for data with meaningful center
    static let rdbu: [Color] = [
        Color(red: 0.404, green: 0.000, blue: 0.122),
        Color(red: 0.698, green: 0.094, blue: 0.169),
        Color(red: 0.839, green: 0.376, blue: 0.302),
        Color(red: 0.957, green: 0.647, blue: 0.510),
        Color(red: 0.992, green: 0.859, blue: 0.780),
        Color(red: 0.878, green: 0.953, blue: 0.973),
        Color(red: 0.670, green: 0.851, blue: 0.914),
        Color(red: 0.404, green: 0.714, blue: 0.824),
        Color(red: 0.196, green: 0.533, blue: 0.741),
        Color(red: 0.019, green: 0.188, blue: 0.380)
    ]
    
    // Engine category colors
    static func colorForEngineCategory(_ category: String) -> Color {
        switch category {
        case "Synthesizers": return set1[1] // Blue
        case "Multi-Voice": return set1[2] // Green
        case "Textures": return set1[3] // Purple
        case "Physical Models": return set1[4] // Orange
        case "Percussion & Samples": return set1[0] // Red
        default: return Color.gray
        }
    }
    
    // Track colors (8 distinct colors for tracks)
    static func colorForTrack(_ trackIndex: Int) -> Color {
        return set1[trackIndex % set1.count]
    }
    
    // Status colors with semantic meaning
    static let statusGreen = Color(red: 0.302, green: 0.686, blue: 0.290)
    static let statusRed = Color(red: 0.894, green: 0.102, blue: 0.110)
    static let statusYellow = Color(red: 1.000, green: 0.647, blue: 0.000)
    static let statusBlue = Color(red: 0.216, green: 0.494, blue: 0.722)
    static let statusPurple = Color(red: 0.596, green: 0.306, blue: 0.639)
}

// MARK: - Custom SVG Icon Component
struct CustomSVGIcon: View {
    let name: String
    let size: CGFloat
    let color: Color
    
    var body: some View {
        // In a real implementation, this would load SVG from bundle
        Image(systemName: "gear") // Fallback to SF Symbol
            .font(.system(size: size))
            .foregroundColor(color)
    }
}

// MARK: - Icon with Badge Component
struct IconWithBadge: View {
    let icon: String
    let badge: String?
    let badgeColor: Color
    
    var body: some View {
        ZStack(alignment: .topTrailing) {
            Image(systemName: icon)
                .font(.system(size: 16))
            
            if let badge = badge {
                Text(badge)
                    .font(.system(size: 8, weight: .bold))
                    .foregroundColor(.white)
                    .padding(.horizontal, 4)
                    .padding(.vertical, 2)
                    .background(
                        Capsule()
                            .fill(badgeColor)
                    )
                    .offset(x: 8, y: -8)
            } else {
                Circle()
                    .fill(badgeColor)
                    .frame(width: 8, height: 8)
                    .offset(x: 6, y: -6)
            }
        }
    }
}

// MARK: - Animated Icon Component
struct AnimatedIcon: View {
    let icon: String
    let isAnimating: Bool
    
    @State private var rotationAngle: Double = 0
    
    var body: some View {
        Image(systemName: icon)
            .font(.system(size: 16))
            .rotationEffect(.degrees(rotationAngle))
            .onAppear {
                if isAnimating {
                    withAnimation(.linear(duration: 1.0).repeatForever(autoreverses: false)) {
                        rotationAngle = 360
                    }
                }
            }
            .onChange(of: isAnimating) { animating in
                if animating {
                    withAnimation(.linear(duration: 1.0).repeatForever(autoreverses: false)) {
                        rotationAngle = 360
                    }
                } else {
                    withAnimation(.easeOut(duration: 0.3)) {
                        rotationAngle = 0
                    }
                }
            }
    }
}

// MARK: - Icon Button Component
struct IconButton: View {
    let icon: String
    let size: CGFloat
    let color: Color
    let backgroundColor: Color
    let action: () -> Void
    
    init(
        icon: String,
        size: CGFloat = 16,
        color: Color = .primary,
        backgroundColor: Color = .clear,
        action: @escaping () -> Void
    ) {
        self.icon = icon
        self.size = size
        self.color = color
        self.backgroundColor = backgroundColor
        self.action = action
    }
    
    var body: some View {
        Button(action: action) {
            Image(systemName: icon)
                .font(.system(size: size))
                .foregroundColor(color)
                .padding(8)
                .background(
                    Circle()
                        .fill(backgroundColor)
                )
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - Status Indicator Component
struct StatusIndicator: View {
    let status: Status
    let size: CGFloat
    
    enum Status {
        case online
        case offline
        case warning
        case error
        case loading
        
        var color: Color {
            switch self {
            case .online: return ColorBrewer.statusGreen
            case .offline: return .gray
            case .warning: return ColorBrewer.statusYellow
            case .error: return ColorBrewer.statusRed
            case .loading: return ColorBrewer.statusBlue
            }
        }
        
        var icon: String {
            switch self {
            case .online: return "circle.fill"
            case .offline: return "circle"
            case .warning: return "exclamationmark.triangle.fill"
            case .error: return "xmark.circle.fill"
            case .loading: return "arrow.clockwise.circle.fill"
            }
        }
    }
    
    var body: some View {
        if status == .loading {
            AnimatedIcon(icon: status.icon, isAnimating: true)
                .font(.system(size: size))
                .foregroundColor(status.color)
        } else {
            Image(systemName: status.icon)
                .font(.system(size: size))
                .foregroundColor(status.color)
        }
    }
}

// MARK: - Category Icon Component
struct CategoryIcon: View {
    let category: IconCategory
    let size: CGFloat
    let isSelected: Bool
    
    private var iconName: String {
        switch category {
        case .transport: return IconLibrary.play
        case .navigation: return IconLibrary.timeline
        case .synthesis: return IconLibrary.synth
        case .effects: return IconLibrary.reverb
        case .editing: return IconLibrary.cut
        case .file: return IconLibrary.project
        case .audio: return IconLibrary.waveform
        case .midi: return IconLibrary.keyboard
        case .interface: return IconLibrary.panel
        case .status: return IconLibrary.info
        }
    }
    
    private var color: Color {
        if isSelected {
            return ColorBrewer.statusBlue
        } else {
            return .secondary
        }
    }
    
    var body: some View {
        VStack(spacing: 4) {
            Image(systemName: iconName)
                .font(.system(size: size))
                .foregroundColor(color)
            
            Text(category.rawValue)
                .font(.system(size: size * 0.5, weight: .medium))
                .foregroundColor(color)
        }
    }
}

// MARK: - Engine Type Icon Component
struct EngineTypeIcon: View {
    let engineType: Int
    let size: CGFloat
    
    private var iconData: (icon: String, color: Color) {
        switch engineType {
        case 0...5: // Synthesizers
            return (IconLibrary.synth, ColorBrewer.colorForEngineCategory("Synthesizers"))
        case 6, 7: // Textures
            return (IconLibrary.noise, ColorBrewer.colorForEngineCategory("Textures"))
        case 8...10: // Physical Models
            return (IconLibrary.modulation, ColorBrewer.colorForEngineCategory("Physical Models"))
        case 11: // DrumKit
            return (IconLibrary.drum, ColorBrewer.colorForEngineCategory("Percussion & Samples"))
        case 12, 13: // Samplers
            return (IconLibrary.sampler, ColorBrewer.colorForEngineCategory("Percussion & Samples"))
        default:
            return (IconLibrary.synth, .gray)
        }
    }
    
    var body: some View {
        Image(systemName: iconData.icon)
            .font(.system(size: size))
            .foregroundColor(iconData.color)
    }
}

// MARK: - Preview Helper
struct IconLibrary_Previews: PreviewProvider {
    static var previews: some View {
        VStack(spacing: 20) {
            // Transport icons
            HStack(spacing: 16) {
                Image(systemName: IconLibrary.play)
                Image(systemName: IconLibrary.pause)
                Image(systemName: IconLibrary.stop)
                Image(systemName: IconLibrary.record)
                Image(systemName: IconLibrary.loop)
            }
            .font(.system(size: 24))
            
            // Status indicators
            HStack(spacing: 16) {
                StatusIndicator(status: .online, size: 16)
                StatusIndicator(status: .warning, size: 16)
                StatusIndicator(status: .error, size: 16)
                StatusIndicator(status: .loading, size: 16)
            }
            
            // Category icons
            HStack(spacing: 16) {
                CategoryIcon(category: .synthesis, size: 24, isSelected: true)
                CategoryIcon(category: .effects, size: 24, isSelected: false)
                CategoryIcon(category: .transport, size: 24, isSelected: false)
            }
            
            // Color palette
            HStack(spacing: 8) {
                ForEach(0..<ColorBrewer.set1.count, id: \.self) { index in
                    Rectangle()
                        .fill(ColorBrewer.set1[index])
                        .frame(width: 24, height: 24)
                        .cornerRadius(4)
                }
            }
        }
        .padding()
    }
}