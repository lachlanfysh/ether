import SwiftUI
import AVFoundation
import Combine

// MARK: - Bird Structure and Flocking Animation
struct Bird {
    var position: CGPoint
    var velocity: CGPoint
    var id: UUID = UUID()
    
    mutating func update(in bounds: CGRect, near neighbors: [Bird]) {
        // Simple flocking rules
        let separation = calculateSeparation(from: neighbors)
        let alignment = calculateAlignment(from: neighbors)
        let cohesion = calculateCohesion(from: neighbors)
        
        // Apply forces
        velocity.x += separation.x * 0.5 + alignment.x * 0.1 + cohesion.x * 0.1
        velocity.y += separation.y * 0.5 + alignment.y * 0.1 + cohesion.y * 0.1
        
        // Limit speed
        let speed = sqrt(velocity.x * velocity.x + velocity.y * velocity.y)
        if speed > 2.0 {
            velocity.x = (velocity.x / speed) * 2.0
            velocity.y = (velocity.y / speed) * 2.0
        }
        
        // Update position
        position.x += velocity.x
        position.y += velocity.y
        
        // Wrap around edges
        if position.x < -10 { position.x = bounds.width + 10 }
        if position.x > bounds.width + 10 { position.x = -10 }
        if position.y < -10 { position.y = bounds.height + 10 }
        if position.y > bounds.height + 10 { position.y = -10 }
    }
    
    private func calculateSeparation(from neighbors: [Bird]) -> CGPoint {
        var separation = CGPoint.zero
        var count = 0
        
        for neighbor in neighbors {
            let distance = sqrt(pow(position.x - neighbor.position.x, 2) + pow(position.y - neighbor.position.y, 2))
            if distance < 20 && distance > 0 {
                var diff = CGPoint(x: position.x - neighbor.position.x, y: position.y - neighbor.position.y)
                diff.x /= distance
                diff.y /= distance
                separation.x += diff.x
                separation.y += diff.y
                count += 1
            }
        }
        
        if count > 0 {
            separation.x /= CGFloat(count)
            separation.y /= CGFloat(count)
        }
        
        return separation
    }
    
    private func calculateAlignment(from neighbors: [Bird]) -> CGPoint {
        var alignment = CGPoint.zero
        var count = 0
        
        for neighbor in neighbors {
            let distance = sqrt(pow(position.x - neighbor.position.x, 2) + pow(position.y - neighbor.position.y, 2))
            if distance < 40 {
                alignment.x += neighbor.velocity.x
                alignment.y += neighbor.velocity.y
                count += 1
            }
        }
        
        if count > 0 {
            alignment.x /= CGFloat(count)
            alignment.y /= CGFloat(count)
        }
        
        return alignment
    }
    
    private func calculateCohesion(from neighbors: [Bird]) -> CGPoint {
        var center = CGPoint.zero
        var count = 0
        
        for neighbor in neighbors {
            let distance = sqrt(pow(position.x - neighbor.position.x, 2) + pow(position.y - neighbor.position.y, 2))
            if distance < 40 {
                center.x += neighbor.position.x
                center.y += neighbor.position.y
                count += 1
            }
        }
        
        if count > 0 {
            center.x /= CGFloat(count)
            center.y /= CGFloat(count)
            return CGPoint(x: (center.x - position.x) * 0.01, y: (center.y - position.y) * 0.01)
        }
        
        return CGPoint.zero
    }
}

struct FlockView: View {
    @State private var birds: [Bird] = []
    @State private var timer: Timer?
    let bounds: CGRect
    let birdCount: Int = 25
    let baseOpacity: Double = 0.15
    
    var body: some View {
        Canvas { context, size in
            for bird in birds {
                let angle = atan2(bird.velocity.y, bird.velocity.x)
                
                // Draw triangular bird
                var path = Path()
                let center = bird.position
                let size: CGFloat = 4
                
                // Triangle pointing in direction of movement
                let point1 = CGPoint(
                    x: center.x + cos(angle) * size,
                    y: center.y + sin(angle) * size
                )
                let point2 = CGPoint(
                    x: center.x + cos(angle + 2.4) * (size * 0.7),
                    y: center.y + sin(angle + 2.4) * (size * 0.7)
                )
                let point3 = CGPoint(
                    x: center.x + cos(angle - 2.4) * (size * 0.7),
                    y: center.y + sin(angle - 2.4) * (size * 0.7)
                )
                
                path.move(to: point1)
                path.addLine(to: point2)
                path.addLine(to: point3)
                path.closeSubpath()
                
                context.fill(path, with: .color(.primary.opacity(baseOpacity)))
            }
        }
        .onAppear {
            initializeBirds()
            startAnimation()
        }
        .onDisappear {
            timer?.invalidate()
        }
    }
    
    private func initializeBirds() {
        birds = (0..<birdCount).map { _ in
            Bird(
                position: CGPoint(
                    x: Double.random(in: 0...bounds.width),
                    y: Double.random(in: 0...bounds.height)
                ),
                velocity: CGPoint(
                    x: Double.random(in: -1...1),
                    y: Double.random(in: -1...1)
                )
            )
        }
    }
    
    private func startAnimation() {
        timer = Timer.scheduledTimer(withTimeInterval: 1/30, repeats: true) { _ in
            for i in 0..<birds.count {
                birds[i].update(in: bounds, near: birds)
            }
        }
    }
}

// MARK: - Main App
@main
struct EtherSynthApp: App {
    @StateObject private var synthController = SynthController()
    
    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(synthController)
                .preferredColorScheme(.light)
                .frame(width: 960, height: 320)
                .fixedSize()
        }
        .windowResizability(.contentSize)
    }
}

// MARK: - Main Content View
struct ContentView: View {
    @EnvironmentObject var synthController: SynthController
    
    private var currentThemeColor: Color {
        return synthController.activeInstrument.pastelColor
    }
    
    var body: some View {
        ZStack {
            // 90s Desktop Background with animated birds
            RoundedRectangle(cornerRadius: 2)
                .fill(currentThemeColor.opacity(0.2))
                .ignoresSafeArea()
                .overlay(
                    FlockView(bounds: CGRect(x: 0, y: 0, width: 960, height: 320))
                        .allowsHitTesting(false)
                )
            
            if synthController.useFocusContextLayout {
                // Focus + Context Layout
                HStack(spacing: 0) {
                    // Left: Context Sidebar (Persistent Song State)
                    ContextSidebar()
                        .frame(width: 180, height: 280)
                    
                    // Center: Large Focus Area
                    FocusArea()
                        .frame(width: 600, height: 280)
                    
                    // Right: Control Panel (LFOs/Transport/Quick Access)
                    ControlPanel()
                        .frame(width: 180, height: 280)
                }
                .padding(20)
            } else {
                // Original Layout
                HStack(spacing: 12) {
                    // Left: LFO/Modulation Window
                    LFOModulationPanel()
                        .frame(width: 200, height: 280)
                    
                    // Center: Main Pattern/Song Window
                    CenterMainView()
                        .frame(width: 472, height: 280)
                    
                    // Right: Instrument Slots Window  
                    InstrumentSlotsPanel()
                        .frame(width: 200, height: 280)
                }
                .padding(20)
            }
            
            // Layout Toggle Button (Top Right)
            VStack {
                HStack {
                    Spacer()
                    Button(action: {
                        synthController.useFocusContextLayout.toggle()
                    }) {
                        Text(synthController.useFocusContextLayout ? "Original" : "Focus+Context")
                            .font(.custom("FormaDJRText-Regular-Testing", size: 10))
                            .foregroundColor(.primary)
                            .padding(.horizontal, 8)
                            .padding(.vertical, 4)
                            .background(
                                RoundedRectangle(cornerRadius: 6)
                                    .fill(Color.white.opacity(0.8))
                                    .shadow(color: Color.black.opacity(0.3), radius: 2, x: 1, y: 1)
                            )
                    }
                    .buttonStyle(PlainButtonStyle())
                    .padding(.trailing, 20)
                    .padding(.top, 10)
                }
                Spacer()
            }
        }
        .onAppear {
            synthController.initialize()
        }
    }
}

// MARK: - Context Sidebar (Left Side - Persistent Song State)
struct ContextSidebar: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 0) {
            // Song Context Header
            VStack(spacing: 4) {
                Text("Song: Untitled")
                    .font(.custom("FormaDJRDisplay-Bold-Testing", size: 10))
                    .foregroundColor(.primary)
                
                HStack {
                    Text("Bar: 4/32")
                        .font(.custom("FormaDJRText-Regular-Testing", size: 8))
                        .foregroundColor(.secondary)
                    Spacer()
                    Text("Sec: A")
                        .font(.custom("FormaDJRText-Regular-Testing", size: 8))
                        .foregroundColor(.secondary)
                }
                
                Text("2:34")
                    .font(.custom("FormaDJRText-Bold-Testing", size: 14))
                    .foregroundColor(.primary)
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .fill(Color.white.opacity(0.8))
                    .shadow(color: Color.black.opacity(0.2), radius: 2, x: 1, y: 1)
            )
            .padding(.bottom, 8)
            
            // Instrument Status Overview
            VStack(spacing: 4) {
                ForEach(Array(InstrumentColor.allCases.prefix(8).enumerated()), id: \.element) { index, color in
                    InstrumentStatusRow(color: color, isActive: synthController.activeInstrument == color)
                }
            }
            .padding(.horizontal, 0)
            
            Spacer()
            
            // Pattern Timeline
            VStack(spacing: 4) {
                Text("Timeline")
                    .font(.custom("FormaDJRText-Bold-Testing", size: 8))
                    .foregroundColor(.primary)
                
                HStack(spacing: 2) {
                    ForEach(["A", "B", "A", "C"], id: \.self) { pattern in
                        Button(action: {
                            synthController.switchPattern(pattern)
                        }) {
                            Text(pattern)
                                .font(.custom("FormaDJRText-Bold-Testing", size: 8))
                                .foregroundColor(synthController.currentPatternID == pattern ? .white : .primary)
                                .frame(width: 20, height: 20)
                                .background(
                                    RoundedRectangle(cornerRadius: 4)
                                        .fill(synthController.currentPatternID == pattern ? synthController.activeInstrument.pastelColor : Color.white)
                                        .shadow(color: Color.black.opacity(0.2), radius: 1, x: 1, y: 1)
                                )
                        }
                        .buttonStyle(PlainButtonStyle())
                    }
                }
            }
            .padding(.horizontal, 12)
            .padding(.bottom, 16)
        }
        .background(
            RoundedRectangle(cornerRadius: 16)
                .fill(Color.clear)
                .shadow(color: Color.black.opacity(0.3), radius: 6, x: 3, y: 3)
        )
    }
}

// MARK: - Instrument Status Row
struct InstrumentStatusRow: View {
    let color: InstrumentColor
    let isActive: Bool
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        Button(action: {
            synthController.selectInstrument(color)
        }) {
            HStack(spacing: 6) {
                // Status indicator
                Circle()
                    .fill(synthController.getInstrumentName(for: color) != "Empty slot" ? color.pastelColor : Color.gray.opacity(0.3))
                    .frame(width: 8, height: 8)
                
                // Instrument name
                Text(synthController.getInstrumentName(for: color) != "Empty slot" ? synthController.getInstrumentName(for: color) : "Empty")
                    .font(.custom("FormaDJRText-Regular-Testing", size: 8))
                    .foregroundColor(isActive ? .white : .primary)
                    .lineLimit(1)
                
                Spacer()
                
                // Activity indicator (mock)
                if synthController.getInstrumentName(for: color) != "Empty slot" {
                    Text("●")
                        .font(.custom("FormaDJRText-Regular-Testing", size: 6))
                        .foregroundColor(isActive ? .white : color.pastelColor)
                }
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .frame(maxWidth: .infinity, alignment: .leading)
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .fill(isActive ? color.pastelColor.opacity(0.8) : Color.white.opacity(0.6))
                    .shadow(color: Color.black.opacity(0.15), radius: 2, x: 1, y: 1)
            )
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - Focus Area (Center - Large Parameter Control)
struct FocusArea: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 0) {
            // Breadcrumb Navigation
            BreadcrumbNavigation()
                .padding(.bottom, 8)
            
            // Main Focus Content
            if synthController.showingLFOConfig {
                LFOFocusView()
            } else if synthController.currentView == "PATTERN" {
                InstrumentFocusView()
            } else {
                SongArrangementFocus()
            }
        }
        .background(
            RoundedRectangle(cornerRadius: 16)
                .fill(synthController.activeInstrument.pastelColor.opacity(0.1))
                .shadow(color: Color.black.opacity(0.3), radius: 8, x: 4, y: 4)
        )
    }
}

// MARK: - Breadcrumb Navigation
struct BreadcrumbNavigation: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        HStack {
            // Home button
            Button(action: {
                synthController.switchView("SONG")
                synthController.closeLFOConfig()
            }) {
                Text("🏠")
                    .font(.custom("FormaDJRText-Regular-Testing", size: 12))
            }
            .buttonStyle(PlainButtonStyle())
            
            if synthController.showingLFOConfig {
                Text(" → LFO \(synthController.selectedLFO)")
                    .font(.custom("FormaDJRText-Regular-Testing", size: 10))
                    .foregroundColor(.secondary)
            } else if synthController.currentView == "PATTERN" {
                Text(" → \(synthController.activeInstrument.rawValue) \(synthController.getInstrumentName(for: synthController.activeInstrument))")
                    .font(.custom("FormaDJRText-Regular-Testing", size: 10))
                    .foregroundColor(.secondary)
            } else {
                Text(" → Song Arrangement")
                    .font(.custom("FormaDJRText-Regular-Testing", size: 10))
                    .foregroundColor(.secondary)
            }
            
            Spacer()
            
            // Smart Knob Status
            Text("🎛️ Filter Cutoff")
                .font(.custom("FormaDJRText-Regular-Testing", size: 8))
                .foregroundColor(.secondary)
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 6)
    }
}

// MARK: - Original Center Main View (for A/B testing)
struct CenterMainView: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 0) {
            // Mode switching tabs with 90s style
            HStack(spacing: 4) {
                ModeTabButton(mode: "Pattern", isActive: synthController.currentView == "PATTERN")
                ModeTabButton(mode: "Song", isActive: synthController.currentView == "SONG")
            }
            .padding(.horizontal, 16)
            .padding(.vertical, 8)
            .background(
                RoundedRectangle(cornerRadius: 16)
                    .fill(Color.gray.opacity(0.15))
                    .shadow(color: Color.black.opacity(0.2), radius: 4, x: 2, y: 2)
            )
            
            // Main content area
            if synthController.showingLFOConfig {
                LFOConfigView()
            } else if synthController.currentView == "PATTERN" {
                InstrumentConfigView()
            } else {
                SongChainingView()
            }
        }
        .background(
            RoundedRectangle(cornerRadius: 20)
                .fill(synthController.activeInstrument.pastelColor.opacity(0.1))
                .shadow(color: Color.black.opacity(0.3), radius: 8, x: 4, y: 4)
        )
    }
}

// MARK: - LFO/Modulation Panel (Now simplified for Control Panel)
struct LFOModulationPanel: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 0) {
            // Top space for icons
            HStack(spacing: 8) {
                // Pattern icon
                Button(action: {
                    synthController.switchView("PATTERN")
                }) {
                    VStack(spacing: 2) {
                        Text("⊞")
                            .font(.custom("FormaDJRText-Regular-Testing", size: 20))
                            .foregroundColor(synthController.currentView == "PATTERN" ? .primary : .secondary)
                    }
                    .frame(width: 40, height: 40)
                    .background(
                        RoundedRectangle(cornerRadius: 12)
                            .fill(synthController.currentView == "PATTERN" ? synthController.activeInstrument.pastelColor.opacity(0.6) : Color.white)
                            .shadow(color: Color.black.opacity(0.2), radius: 4, x: 2, y: 2)
                    )
                }
                .buttonStyle(PlainButtonStyle())
                
                // Song icon
                Button(action: {
                    synthController.switchView("SONG")
                }) {
                    VStack(spacing: 2) {
                        Text("♫")
                            .font(.custom("FormaDJRText-Regular-Testing", size: 18))
                            .foregroundColor(synthController.currentView == "SONG" ? .primary : .secondary)
                    }
                    .frame(width: 40, height: 40)
                    .background(
                        RoundedRectangle(cornerRadius: 12)
                            .fill(synthController.currentView == "SONG" ? synthController.activeInstrument.pastelColor.opacity(0.6) : Color.white)
                            .shadow(color: Color.black.opacity(0.2), radius: 4, x: 2, y: 2)
                    )
                }
                .buttonStyle(PlainButtonStyle())
                
                // Tape icon
                Button(action: {
                    // Future tape functionality
                }) {
                    VStack(spacing: 2) {
                        Text("⏺")
                            .font(.custom("FormaDJRText-Regular-Testing", size: 18))
                            .foregroundColor(.secondary)
                    }
                    .frame(width: 40, height: 40)
                    .background(
                        RoundedRectangle(cornerRadius: 12)
                            .fill(Color.white)
                            .shadow(color: Color.black.opacity(0.2), radius: 4, x: 2, y: 2)
                    )
                }
                .buttonStyle(PlainButtonStyle())
            }
            .padding(.top, 16)
            .padding(.horizontal, 16)
            
            // LFO buttons (reduced to 6)
            VStack(spacing: 6) {
                ForEach(1...6, id: \.self) { lfoIndex in
                    LFOSlotView(lfoIndex: lfoIndex)
                }
            }
            .padding(.horizontal, 0)
            .padding(.vertical, 16)
            
            Spacer()
        }
        .background(
            RoundedRectangle(cornerRadius: 20)
                .fill(Color.clear)
                .shadow(color: Color.black.opacity(0.3), radius: 8, x: 4, y: 4)
        )
    }
}

// MARK: - LFO Slot View
struct LFOSlotView: View {
    let lfoIndex: Int
    @EnvironmentObject var synthController: SynthController
    
    private var assignmentColor: Color {
        // Mock: show different assignments
        switch lfoIndex {
        case 1: return InstrumentColor.coral.pastelColor
        case 2: return InstrumentColor.teal.pastelColor
        case 4: return InstrumentColor.sage.pastelColor
        default: return Color.gray.opacity(0.3)
        }
    }
    
    private var assignmentText: String {
        switch lfoIndex {
        case 1: return "Coral filter"
        case 2: return "Teal attack"
        case 4: return "Sage pitch"
        default: return "Unassigned"
        }
    }
    
    var body: some View {
        Button(action: {
            synthController.selectLFO(lfoIndex)
        }) {
            HStack(spacing: 8) {
                // LFO number
                Text("LFO \(lfoIndex)")
                    .font(.custom("FormaDJRText-Bold-Testing", size: 12))
                    .foregroundColor(.primary)
                    .frame(width: 50, alignment: .leading)
                
                // Assignment indicator dot
                Circle()
                    .fill(assignmentColor)
                    .frame(width: 10, height: 10)
                
                // Assignment text
                Text(assignmentText)
                    .font(.custom("FormaDJRText-Regular-Testing", size: 9))
                    .foregroundColor(.secondary)
                
                Spacer()
            }
            .padding(.horizontal, 0)
            .padding(.vertical, 6)
            .frame(maxWidth: .infinity)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(assignmentColor.opacity(0.2))
                    .shadow(color: Color.black.opacity(0.2), radius: 3, x: 2, y: 2)
            )
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - Instrument Focus View (Large Parameter Control Area)
struct InstrumentFocusView: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        ScrollView(.vertical, showsIndicators: false) {
            VStack(spacing: 16) {
                // Instrument Header with visual emphasis
                VStack(spacing: 8) {
                    HStack {
                        Circle()
                            .fill(synthController.activeInstrument.pastelColor)
                            .frame(width: 40, height: 40)
                            .overlay(
                                Text("🎵")
                                    .font(.custom("FormaDJRText-Regular-Testing", size: 20))
                                    .foregroundColor(.white)
                            )
                        
                        VStack(alignment: .leading, spacing: 2) {
                            Text(synthController.activeInstrument.rawValue)
                                .font(.custom("FormaDJRDisplay-Bold-Testing", size: 16))
                                .foregroundColor(.primary)
                            
                            Text(synthController.getInstrumentName(for: synthController.activeInstrument))
                                .font(.custom("FormaDJRText-Regular-Testing", size: 12))
                                .foregroundColor(.secondary)
                        }
                        
                        Spacer()
                    }
                    .padding(16)
                    .background(
                        RoundedRectangle(cornerRadius: 12)
                            .fill(synthController.activeInstrument.pastelColor.opacity(0.2))
                    )
                }
                
                // Family & Engine Selection (Large Format)
                VStack(spacing: 12) {
                    Text("Synthesis Engine")
                        .font(.custom("FormaDJRText-Bold-Testing", size: 12))
                        .foregroundColor(.primary)
                    
                    // Family selection with large buttons
                    LazyVGrid(columns: [
                        GridItem(.flexible()),
                        GridItem(.flexible()),
                        GridItem(.flexible())
                    ], spacing: 8) {
                        ForEach(["Bass", "Leads", "Pads", "Keys", "Drums", "FX"], id: \.self) { family in
                            Button(action: {
                                synthController.selectFamily(for: synthController.activeInstrument, family: family)
                            }) {
                                VStack(spacing: 4) {
                                    Text(family == "Bass" ? "🔊" : family == "Leads" ? "⚡" : family == "Pads" ? "🌊" : family == "Keys" ? "🎹" : family == "Drums" ? "🥁" : "🌟")
                                        .font(.custom("FormaDJRText-Regular-Testing", size: 24))
                                    
                                    Text(family)
                                        .font(.custom("FormaDJRText-Regular-Testing", size: 10))
                                        .foregroundColor(.primary)
                                }
                                .frame(width: 60, height: 60)
                                .background(
                                    RoundedRectangle(cornerRadius: 12)
                                        .fill(synthController.getSelectedFamily(for: synthController.activeInstrument) == family ? synthController.activeInstrument.pastelColor.opacity(0.6) : Color.white)
                                        .shadow(color: Color.black.opacity(0.2), radius: 3, x: 2, y: 2)
                                )
                            }
                            .buttonStyle(PlainButtonStyle())
                        }
                    }
                }
                
                // Parameter Control Grid (Large Format)
                VStack(spacing: 8) {
                    Text("Parameters")
                        .font(.custom("FormaDJRText-Bold-Testing", size: 12))
                        .foregroundColor(.primary)
                    
                    LazyVGrid(columns: [
                        GridItem(.flexible()),
                        GridItem(.flexible()),
                        GridItem(.flexible()),
                        GridItem(.flexible())
                    ], spacing: 12) {
                        ForEach(0..<8, id: \.self) { paramIndex in
                            VStack(spacing: 6) {
                                // Parameter icon
                                Text(paramIndex == 0 ? "🎵" : paramIndex == 1 ? "🎛️" : paramIndex == 2 ? "🌊" : paramIndex == 3 ? "⚡" : paramIndex == 4 ? "🎚️" : paramIndex == 5 ? "🔄" : paramIndex == 6 ? "⏰" : "🎭")
                                    .font(.custom("FormaDJRText-Regular-Testing", size: 20))
                                
                                // Parameter name
                                Text(paramIndex == 0 ? "Volume" : paramIndex == 1 ? "Cutoff" : paramIndex == 2 ? "Resonance" : paramIndex == 3 ? "Attack" : paramIndex == 4 ? "Decay" : paramIndex == 5 ? "LFO Rate" : paramIndex == 6 ? "Delay" : "Reverb")
                                    .font(.custom("FormaDJRText-Regular-Testing", size: 8))
                                    .foregroundColor(.secondary)
                                
                                // Parameter value
                                Text("\(Int.random(in: 0...100))")
                                    .font(.custom("FormaDJRText-Bold-Testing", size: 14))
                                    .foregroundColor(.primary)
                                
                                // Parameter bar
                                RoundedRectangle(cornerRadius: 3)
                                    .fill(Color.gray.opacity(0.3))
                                    .frame(height: 6)
                                    .overlay(
                                        HStack {
                                            RoundedRectangle(cornerRadius: 3)
                                                .fill(synthController.activeInstrument.pastelColor)
                                                .frame(width: CGFloat.random(in: 10...50))
                                            Spacer()
                                        }
                                    )
                            }
                            .frame(width: 80, height: 80)
                            .background(
                                RoundedRectangle(cornerRadius: 12)
                                    .fill(Color.white.opacity(0.8))
                                    .shadow(color: Color.black.opacity(0.15), radius: 2, x: 1, y: 1)
                            )
                        }
                    }
                }
                
                // Euclidean Pattern Preview
                VStack(spacing: 8) {
                    HStack {
                        Text("Pattern \(synthController.currentPatternID)")
                            .font(.custom("FormaDJRText-Bold-Testing", size: 12))
                            .foregroundColor(.primary)
                        
                        Spacer()
                        
                        Button("◀") { }
                        .font(.custom("FormaDJRText-Regular-Testing", size: 10))
                        .foregroundColor(.secondary)
                        
                        Button("▶") { }
                        .font(.custom("FormaDJRText-Regular-Testing", size: 10))
                        .foregroundColor(.secondary)
                    }
                    
                    // Large euclidean pattern visualization
                    VStack(spacing: 4) {
                        Text("6 hits / 16 steps, rotation 2")
                            .font(.custom("FormaDJRText-Regular-Testing", size: 10))
                            .foregroundColor(.secondary)
                        
                        HStack(spacing: 4) {
                            ForEach(0..<16, id: \.self) { step in
                                RoundedRectangle(cornerRadius: 4)
                                    .fill([0,2,5,8,10,13].contains(step) ? synthController.activeInstrument.pastelColor : Color.gray.opacity(0.3))
                                    .frame(width: 28, height: 28)
                                    .overlay(
                                        Text("\(step + 1)")
                                            .font(.custom("FormaDJRText-Regular-Testing", size: 8))
                                            .foregroundColor([0,2,5,8,10,13].contains(step) ? .white : .secondary)
                                    )
                            }
                        }
                    }
                    .padding(12)
                    .background(
                        RoundedRectangle(cornerRadius: 12)
                            .fill(Color.white.opacity(0.8))
                            .shadow(color: Color.black.opacity(0.15), radius: 2, x: 1, y: 1)
                    )
                }
            }
            .padding(16)
        }
    }
}

// MARK: - Song Arrangement Focus
struct SongArrangementFocus: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Song Arrangement")
                .font(.custom("FormaDJRDisplay-Bold-Testing", size: 16))
                .foregroundColor(.primary)
            
            // Timeline with large pattern blocks
            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: 8) {
                    ForEach(Array(["A", "A", "B", "A", "C", "B", "A"].enumerated()), id: \.offset) { index, pattern in
                        VStack(spacing: 4) {
                            Text("\(index + 1)")
                                .font(.custom("FormaDJRText-Regular-Testing", size: 8))
                                .foregroundColor(.secondary)
                            
                            Button(action: {
                                synthController.switchPattern(pattern)
                            }) {
                                VStack(spacing: 4) {
                                    Text(pattern)
                                        .font(.custom("FormaDJRDisplay-Bold-Testing", size: 24))
                                        .foregroundColor(.primary)
                                    
                                    Text("4 bars")
                                        .font(.custom("FormaDJRText-Regular-Testing", size: 8))
                                        .foregroundColor(.secondary)
                                }
                                .frame(width: 60, height: 80)
                                .background(
                                    RoundedRectangle(cornerRadius: 8)
                                        .fill(synthController.currentPatternID == pattern ? synthController.activeInstrument.pastelColor.opacity(0.8) : Color.white)
                                        .shadow(color: Color.black.opacity(0.2), radius: 3, x: 2, y: 2)
                                )
                            }
                            .buttonStyle(PlainButtonStyle())
                        }
                    }
                }
                .padding(.horizontal, 16)
            }
            
            Spacer()
        }
        .padding(16)
    }
}

// MARK: - Mode Tab Button
struct ModeTabButton: View {
    let mode: String
    let isActive: Bool
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        Button(action: {
            synthController.switchView(mode)
        }) {
            Text(mode)
                .font(.custom("FormaDJRDisplay-Bold-Testing", size: 14))
                .foregroundColor(isActive ? .primary : .secondary)
                .frame(maxWidth: .infinity)
                .padding(.vertical, 10)
                .padding(.horizontal, 16)
                .background(
                    RoundedRectangle(cornerRadius: 12)
                        .fill(isActive ? synthController.activeInstrument.pastelColor.opacity(0.8) : Color.white)
                        .shadow(color: Color.black.opacity(0.2), radius: 3, x: 1, y: 1)
                )
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - Pattern View (All instruments, selected highlighted)
struct PatternView: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Pattern A - All instruments")
                .font(.custom("FormaDJRDisplay-Bold-Testing", size: 12))
                .foregroundColor(.primary)
                .padding(.top)
            
            // All 8 instruments in pattern view
            LazyVGrid(columns: [
                GridItem(.flexible()),
                GridItem(.flexible())
            ], spacing: 16) {
                ForEach(InstrumentColor.allCases, id: \.self) { color in
                    PatternInstrumentView(color: color)
                }
            }
            .padding()
            
            Spacer()
        }
    }
}

// MARK: - Pattern Instrument View (with dimming)
struct PatternInstrumentView: View {
    let color: InstrumentColor
    @EnvironmentObject var synthController: SynthController
    
    private var isSelected: Bool {
        synthController.activeInstrument == color
    }
    
    private var isDimmed: Bool {
        !isSelected && synthController.getInstrumentName(for: color) != "Empty slot"
    }
    
    var body: some View {
        Button(action: {
            synthController.selectInstrument(color)
        }) {
            VStack(spacing: 8) {
                // Color indicator
                Circle()
                    .fill(color.pastelColor)
                    .frame(width: isSelected ? 40 : 32, height: isSelected ? 40 : 32)
                    .overlay(
                        Circle()
                            .stroke(Color.primary, lineWidth: isSelected ? 3 : 1)
                    )
                
                // Instrument name
                Text(synthController.getInstrumentName(for: color))
                    .font(.custom(isSelected ? "FormaDJRText-Bold-Testing" : "FormaDJRText-Regular-Testing", size: isSelected ? 12 : 10))
                    .foregroundColor(isSelected ? .primary : .secondary)
                    .multilineTextAlignment(.center)
                
                // Pattern steps visualization
                if synthController.getInstrumentName(for: color) != "Empty slot" {
                    HStack(spacing: 2) {
                        ForEach(0..<16, id: \.self) { step in
                            RoundedRectangle(cornerRadius: 2)
                                .fill(step % 4 == 0 ? color.pastelColor : Color.gray.opacity(0.3))
                                .frame(width: 4, height: 8)
                        }
                    }
                }
            }
            .padding()
            .frame(width: 180, height: 120)
            .background(
                RoundedRectangle(cornerRadius: 2)
                    .fill(isSelected ? color.pastelColor.opacity(0.4) : Color.white)
                    .overlay(
                        RoundedRectangle(cornerRadius: 2)
                            .stroke(isSelected ? Color.black : Color.gray.opacity(0.6), lineWidth: isSelected ? 3 : 2)
                    )
                    .overlay(
                        RoundedRectangle(cornerRadius: 2)
                            .fill(Color.black.opacity(0.25))
                            .offset(x: isSelected ? 3 : 2, y: isSelected ? 3 : 2)
                            .mask(
                                RoundedRectangle(cornerRadius: 2).stroke(Color.black, lineWidth: isSelected ? 3 : 2)
                            )
                    )
            )
            .opacity(isDimmed ? 0.4 : 1.0)
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - Song Chaining View
struct SongChainingView: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Song structure")
                .font(.custom("FormaDJRDisplay-Bold-Testing", size: 12))
                .foregroundColor(.primary)
                .padding(.top)
            
            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: 8) {
                    ForEach(["A", "A", "B", "A", "C", "B", "A"], id: \.self) { pattern in
                        PatternBlockView(patternName: pattern)
                    }
                }
                .padding(.horizontal)
            }
            
            Text("Drag patterns to rearrange")
                .font(.system(size: 10, design: .default))
                .foregroundColor(.secondary)
            
            Spacer()
        }
    }
}

// MARK: - Pattern Block View (for song chaining)
struct PatternBlockView: View {
    let patternName: String
    
    var body: some View {
        VStack(spacing: 4) {
            Text("Pat \(patternName)")
                .font(.custom("FormaDJRText-Bold-Testing", size: 10))
                .foregroundColor(.primary)
            
            RoundedRectangle(cornerRadius: 2)
                .fill(Color.gray.opacity(0.3))
                .frame(width: 60, height: 40)
                .overlay(
                    Text(patternName)
                        .font(.custom("FormaDJRDisplay-Bold-Testing", size: 20))
                        .foregroundColor(.primary)
                )
        }
    }
}

// MARK: - Control Panel (Right Side - LFOs/Transport/Quick Access)
struct ControlPanel: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 0) {
            // Mode buttons at top
            HStack(spacing: 6) {
                // Pattern icon
                Button(action: {
                    synthController.switchView("PATTERN")
                }) {
                    VStack(spacing: 2) {
                        Text("⊞")
                            .font(.custom("FormaDJRText-Regular-Testing", size: 16))
                            .foregroundColor(synthController.currentView == "PATTERN" ? .primary : .secondary)
                    }
                    .frame(width: 32, height: 32)
                    .background(
                        RoundedRectangle(cornerRadius: 8)
                            .fill(synthController.currentView == "PATTERN" ? synthController.activeInstrument.pastelColor.opacity(0.6) : Color.white)
                            .shadow(color: Color.black.opacity(0.2), radius: 2, x: 1, y: 1)
                    )
                }
                .buttonStyle(PlainButtonStyle())
                
                // Song icon
                Button(action: {
                    synthController.switchView("SONG")
                }) {
                    VStack(spacing: 2) {
                        Text("♫")
                            .font(.custom("FormaDJRText-Regular-Testing", size: 14))
                            .foregroundColor(synthController.currentView == "SONG" ? .primary : .secondary)
                    }
                    .frame(width: 32, height: 32)
                    .background(
                        RoundedRectangle(cornerRadius: 8)
                            .fill(synthController.currentView == "SONG" ? synthController.activeInstrument.pastelColor.opacity(0.6) : Color.white)
                            .shadow(color: Color.black.opacity(0.2), radius: 2, x: 1, y: 1)
                    )
                }
                .buttonStyle(PlainButtonStyle())
                
                // Tape icon
                Button(action: {
                    // Future tape functionality
                }) {
                    VStack(spacing: 2) {
                        Text("⏺")
                            .font(.custom("FormaDJRText-Regular-Testing", size: 14))
                            .foregroundColor(.secondary)
                    }
                    .frame(width: 32, height: 32)
                    .background(
                        RoundedRectangle(cornerRadius: 8)
                            .fill(Color.white)
                            .shadow(color: Color.black.opacity(0.2), radius: 2, x: 1, y: 1)
                    )
                }
                .buttonStyle(PlainButtonStyle())
            }
            .padding(.top, 12)
            .padding(.horizontal, 12)
            
            // LFO section
            VStack(spacing: 6) {
                Text("LFOs")
                    .font(.custom("FormaDJRText-Bold-Testing", size: 10))
                    .foregroundColor(.primary)
                    .padding(.top, 8)
                
                ForEach(1...6, id: \.self) { lfoIndex in
                    Button(action: {
                        synthController.selectLFO(lfoIndex)
                    }) {
                        HStack(spacing: 6) {
                            Text("LFO\(lfoIndex)")
                                .font(.custom("FormaDJRText-Bold-Testing", size: 9))
                                .foregroundColor(.primary)
                                .frame(width: 35, alignment: .leading)
                            
                            Circle()
                                .fill(lfoIndex <= 3 ? InstrumentColor.allCases[lfoIndex-1].pastelColor : Color.gray.opacity(0.3))
                                .frame(width: 8, height: 8)
                            
                            Spacer()
                        }
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .frame(maxWidth: .infinity)
                        .background(
                            RoundedRectangle(cornerRadius: 6)
                                .fill(Color.white.opacity(0.8))
                                .shadow(color: Color.black.opacity(0.1), radius: 1, x: 1, y: 1)
                        )
                    }
                    .buttonStyle(PlainButtonStyle())
                }
            }
            .padding(.horizontal, 12)
            
            Spacer()
            
            // Transport controls
            VStack(spacing: 6) {
                Text("Transport")
                    .font(.custom("FormaDJRText-Bold-Testing", size: 10))
                    .foregroundColor(.primary)
                
                HStack(spacing: 4) {
                    Button(action: { synthController.play() }) {
                        Text(synthController.isPlaying ? "⏸" : "▶️")
                            .font(.custom("FormaDJRText-Regular-Testing", size: 14))
                    }
                    .frame(width: 28, height: 28)
                    .background(
                        RoundedRectangle(cornerRadius: 6)
                            .fill(synthController.isPlaying ? Color.green.opacity(0.3) : Color.white)
                            .shadow(color: Color.black.opacity(0.2), radius: 2, x: 1, y: 1)
                    )
                    .buttonStyle(PlainButtonStyle())
                    
                    Button(action: { synthController.stop() }) {
                        Text("⏹")
                            .font(.custom("FormaDJRText-Regular-Testing", size: 14))
                    }
                    .frame(width: 28, height: 28)
                    .background(
                        RoundedRectangle(cornerRadius: 6)
                            .fill(Color.white)
                            .shadow(color: Color.black.opacity(0.2), radius: 2, x: 1, y: 1)
                    )
                    .buttonStyle(PlainButtonStyle())
                    
                    Button(action: { synthController.record() }) {
                        Text("⏺")
                            .font(.custom("FormaDJRText-Regular-Testing", size: 14))
                            .foregroundColor(synthController.isRecording ? .red : .primary)
                    }
                    .frame(width: 28, height: 28)
                    .background(
                        RoundedRectangle(cornerRadius: 6)
                            .fill(synthController.isRecording ? Color.red.opacity(0.3) : Color.white)
                            .shadow(color: Color.black.opacity(0.2), radius: 2, x: 1, y: 1)
                    )
                    .buttonStyle(PlainButtonStyle())
                }
            }
            .padding(.horizontal, 12)
            .padding(.bottom, 16)
        }
        .background(
            RoundedRectangle(cornerRadius: 16)
                .fill(Color.clear)
                .shadow(color: Color.black.opacity(0.3), radius: 6, x: 3, y: 3)
        )
    }
}

// MARK: - LFO Focus View
struct LFOFocusView: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 16) {
            // LFO Header
            VStack(spacing: 8) {
                HStack {
                    Circle()
                        .fill(Color.orange)
                        .frame(width: 40, height: 40)
                        .overlay(
                            Text("∿")
                                .font(.custom("FormaDJRText-Regular-Testing", size: 20))
                                .foregroundColor(.white)
                        )
                    
                    VStack(alignment: .leading, spacing: 2) {
                        Text("LFO \(synthController.selectedLFO)")
                            .font(.custom("FormaDJRDisplay-Bold-Testing", size: 16))
                            .foregroundColor(.primary)
                        
                        Text(synthController.getLFOType(for: synthController.selectedLFO))
                            .font(.custom("FormaDJRText-Regular-Testing", size: 12))
                            .foregroundColor(.secondary)
                    }
                    
                    Spacer()
                }
                .padding(16)
                .background(
                    RoundedRectangle(cornerRadius: 12)
                        .fill(Color.orange.opacity(0.2))
                )
            }
            
            // LFO Parameters in large format
            LazyVGrid(columns: [
                GridItem(.flexible()),
                GridItem(.flexible()),
                GridItem(.flexible())
            ], spacing: 16) {
                ForEach(["Rate", "Depth", "Phase"], id: \.self) { param in
                    VStack(spacing: 8) {
                        Text(param == "Rate" ? "⏱" : param == "Depth" ? "📊" : "🔄")
                            .font(.custom("FormaDJRText-Regular-Testing", size: 24))
                        
                        Text(param)
                            .font(.custom("FormaDJRText-Bold-Testing", size: 12))
                            .foregroundColor(.primary)
                        
                        Text(param == "Rate" ? "1.2 Hz" : param == "Depth" ? "75%" : "45°")
                            .font(.custom("FormaDJRText-Bold-Testing", size: 14))
                            .foregroundColor(.primary)
                        
                        RoundedRectangle(cornerRadius: 4)
                            .fill(Color.gray.opacity(0.3))
                            .frame(height: 8)
                            .overlay(
                                HStack {
                                    RoundedRectangle(cornerRadius: 4)
                                        .fill(Color.orange)
                                        .frame(width: param == "Rate" ? 30 : param == "Depth" ? 60 : 35)
                                    Spacer()
                                }
                            )
                    }
                    .frame(width: 120, height: 120)
                    .background(
                        RoundedRectangle(cornerRadius: 12)
                            .fill(Color.white.opacity(0.8))
                            .shadow(color: Color.black.opacity(0.15), radius: 2, x: 1, y: 1)
                    )
                }
            }
            
            Spacer()
        }
        .padding(16)
    }
}

// MARK: - Instrument Slots Panel (Legacy - keeping for compatibility)
struct InstrumentSlotsPanel: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        ControlPanel() // Use the new control panel instead
    }
}

// MARK: - Instrument Slot View (Empty by Default)
struct InstrumentSlotView: View {
    let color: InstrumentColor
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        Button(action: {
            // Always just select the instrument slot for configuration in middle panel
            synthController.selectInstrument(color)
        }) {
            HStack(spacing: 8) {
                // Family icon placeholder
                Circle()
                    .fill(Color.gray.opacity(0.3))
                    .frame(width: 14, height: 14)
                    .overlay(
                        Text("♪")
                            .font(.custom("FormaDJRText-Regular-Testing", size: 8))
                            .foregroundColor(.secondary)
                    )
                
                // Instrument name or prompt
                VStack(alignment: .leading, spacing: 1) {
                    if synthController.getInstrumentName(for: color) != "Empty slot" {
                        Text(synthController.getInstrumentName(for: color))
                            .font(.custom("FormaDJRText-Regular-Testing", size: 11))
                            .foregroundColor(.primary)
                    } else {
                        Text("Touch to add →")
                            .font(.custom("FormaDJRText-Regular-Testing", size: 11))
                            .foregroundColor(.secondary)
                    }
                }
                
                Spacer()
            }
            .padding(.horizontal, 0)
            .padding(.vertical, 6)
            .frame(maxWidth: .infinity)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(synthController.activeInstrument == color ? 
                          color.pastelColor.opacity(0.8) : color.pastelColor.opacity(0.2))
                    .shadow(color: Color.black.opacity(0.2), radius: 3, x: 2, y: 2)
            )
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - Instrument Config View (Full Workflow)
struct InstrumentConfigView: View {
    @EnvironmentObject var synthController: SynthController
    
    private let instrumentFamilies = [
        "Bass", "Leads", "Pads", "Keys", "Drums"
    ]
    
    private let engines = [
        "Bass": ["TB-303", "Moog", "Sub Bass"],
        "Leads": ["DX7", "Saw", "Square"],
        "Pads": ["Vintage", "Warm", "String"],
        "Keys": ["Rhodes", "Wurly", "Piano"],
        "Drums": ["TR-808", "TR-909", "Acoustic"]
    ]
    
    
    var body: some View {
        InstrumentConfigContent()
    }
}

// MARK: - Instrument Config Content (Separated to avoid compiler issues)
struct InstrumentConfigContent: View {
    @EnvironmentObject var synthController: SynthController
    
    private let instrumentFamilies = [
        "Bass", "Leads", "Pads", "Keys", "Drums"
    ]
    
    private let engines = [
        "Bass": ["TB-303", "Moog", "Sub Bass"],
        "Leads": ["DX7", "Saw", "Square"],
        "Pads": ["Vintage", "Warm", "String"],
        "Keys": ["Rhodes", "Wurly", "Piano"],
        "Drums": ["TR-808", "TR-909", "Acoustic"]
    ]
    
    var body: some View {
        ScrollViewReader { proxy in
            ScrollView(.vertical, showsIndicators: false) {
                VStack(spacing: 12) {
                    // Family Selection
                    VStack(spacing: 6) {
                        Text("Instrument family")
                            .font(.custom("FormaDJRText-Bold-Testing", size: 8))
                            .foregroundColor(.primary)
                        
                        HStack(spacing: 4) {
                            ForEach(instrumentFamilies, id: \.self) { family in
                                Button(action: {
                                    synthController.selectFamily(for: synthController.activeInstrument, family: family)
                                }) {
                                    VStack(spacing: 2) {
                                        Text("♫")
                                            .font(.custom("FormaDJRText-Regular-Testing", size: 16))
                                            .foregroundColor(.secondary)
                                        Text(family)
                                            .font(.custom("FormaDJRText-Regular-Testing", size: 8))
                                            .foregroundColor(.secondary)
                                    }
                                }
                                .padding(.horizontal, 6)
                                .padding(.vertical, 4)
                                .background(
                                    RoundedRectangle(cornerRadius: 4)
                                        .fill(Color.white)
                                        .overlay(
                                            RoundedRectangle(cornerRadius: 4)
                                                .stroke(Color.gray.opacity(0.4), lineWidth: 1)
                                        )
                                )
                            }
                        }
                    }
                    .id("family")
                    
                    // Engine Selection
                    VStack(spacing: 6) {
                        Text("Engine")
                            .font(.custom("FormaDJRText-Bold-Testing", size: 8))
                            .foregroundColor(.primary)
                        
                        HStack(spacing: 4) {
                            ForEach(engines["Bass"] ?? [], id: \.self) { engine in
                                Button(action: {
                                    synthController.selectEngine(for: synthController.activeInstrument, engine: engine)
                                }) {
                                    VStack(spacing: 2) {
                                        Text("⚙")
                                            .font(.custom("FormaDJRText-Regular-Testing", size: 14))
                                            .foregroundColor(.secondary)
                                        Text(engine)
                                            .font(.custom("FormaDJRText-Regular-Testing", size: 8))
                                            .foregroundColor(.secondary)
                                    }
                                }
                                .padding(.horizontal, 6)
                                .padding(.vertical, 4)
                                .background(
                                    RoundedRectangle(cornerRadius: 4)
                                        .fill(Color.white)
                                        .overlay(
                                            RoundedRectangle(cornerRadius: 4)
                                                .stroke(Color.gray.opacity(0.4), lineWidth: 1)
                                        )
                                )
                            }
                        }
                    }
                    .id("engine")
                    
                    // Parameters Area (fixed height)
                    VStack(spacing: 4) {
                        Text("Parameters")
                            .font(.custom("FormaDJRText-Bold-Testing", size: 8))
                            .foregroundColor(.primary)
                        
                        // Parameter icons with values and bars
                        HStack(spacing: 8) {
                            ForEach(0..<4, id: \.self) { paramIndex in
                                VStack(spacing: 4) {
                                    // Parameter icon
                                    Text(paramIndex == 0 ? "♫" : paramIndex == 1 ? "●" : paramIndex == 2 ? "▲" : "▼")
                                        .font(.custom("FormaDJRText-Regular-Testing", size: 16))
                                        .foregroundColor(.secondary)
                                    
                                    // Parameter value
                                    Text("50")
                                        .font(.custom("FormaDJRText-Bold-Testing", size: 10))
                                        .foregroundColor(.primary)
                                    
                                    // Parameter bar
                                    RoundedRectangle(cornerRadius: 2)
                                        .fill(Color.gray.opacity(0.3))
                                        .frame(height: 4)
                                        .overlay(
                                            HStack {
                                                RoundedRectangle(cornerRadius: 2)
                                                    .fill(synthController.activeInstrument.pastelColor)
                                                    .frame(width: 20)
                                                Spacer()
                                            }
                                        )
                                }
                                .frame(width: 40)
                            }
                        }
                    }
                    .frame(height: 80)
                    .id("parameters")
                    
                    // Pattern Section with Navigation
                    VStack(spacing: 6) {
                        HStack {
                            Button("◀") {
                                // Previous pattern
                            }
                            .font(.custom("FormaDJRText-Regular-Testing", size: 8))
                            
                            Spacer()
                            
                            Text("Pattern \(synthController.currentPatternID)")
                                .font(.custom("FormaDJRText-Bold-Testing", size: 8))
                                .foregroundColor(.primary)
                            
                            Spacer()
                            
                            Button("▶") {
                                // Next pattern
                            }
                            .font(.custom("FormaDJRText-Regular-Testing", size: 8))
                        }
                        
                        // 16-step pattern grid (1 row x 16)
                        HStack(spacing: 2) {
                            ForEach(0..<16, id: \.self) { step in
                                RoundedRectangle(cornerRadius: 2)
                                    .fill(step % 4 == 0 ? synthController.activeInstrument.pastelColor : Color.gray.opacity(0.3))
                                    .frame(width: 20, height: 20)
                                    .overlay(
                                        RoundedRectangle(cornerRadius: 2)
                                            .stroke(Color.gray.opacity(0.5), lineWidth: 1)
                                    )
                            }
                        }
                    }
                    .id("pattern")
                }
                .padding(12)
            }
            .onAppear {
                // Restore scroll position for this instrument
                if let position = synthController.instrumentScrollPositions[synthController.activeInstrument] {
                    proxy.scrollTo("pattern", anchor: .center)
                }
            }
        }
    }
}

// MARK: - Instrument Color Enum
enum InstrumentColor: String, CaseIterable {
    case coral = "Coral"
    case peach = "Peach"
    case cream = "Cream"
    case sage = "Sage"
    case teal = "Teal"
    case slate = "Slate"
    case pearl = "Pearl"
    case stone = "Stone"
    
    var color: Color {
        switch self {
        case .coral: return Color(red: 0.82, green: 0.68, blue: 0.62)
        case .peach: return Color(red: 0.89, green: 0.78, blue: 0.74)
        case .cream: return Color(red: 0.93, green: 0.91, blue: 0.88)
        case .sage: return Color(red: 0.74, green: 0.81, blue: 0.76)
        case .teal: return Color(red: 0.65, green: 0.75, blue: 0.73)
        case .slate: return Color(red: 0.54, green: 0.54, blue: 0.54)
        case .pearl: return Color(red: 0.91, green: 0.90, blue: 0.87)
        case .stone: return Color(red: 0.88, green: 0.85, blue: 0.82)
        }
    }
    
    var pastelColor: Color {
        switch self {
        case .coral: return Color(red: 0.82, green: 0.68, blue: 0.62)
        case .peach: return Color(red: 0.89, green: 0.78, blue: 0.74)
        case .cream: return Color(red: 0.93, green: 0.91, blue: 0.88)
        case .sage: return Color(red: 0.74, green: 0.81, blue: 0.76)
        case .teal: return Color(red: 0.65, green: 0.75, blue: 0.73)
        case .slate: return Color(red: 0.54, green: 0.54, blue: 0.54)
        case .pearl: return Color(red: 0.91, green: 0.90, blue: 0.87)
        case .stone: return Color(red: 0.88, green: 0.85, blue: 0.82)
        }
    }
    
    var index: Int {
        switch self {
        case .coral: return 0
        case .peach: return 1
        case .cream: return 2
        case .sage: return 3
        case .teal: return 4
        case .slate: return 5
        case .pearl: return 6
        case .stone: return 7
        }
    }
}

// MARK: - LFO Configuration View
struct LFOConfigView: View {
    @EnvironmentObject var synthController: SynthController
    
    private let lfoTypes = ["Sine", "Saw", "Square", "Triangle", "Noise"]
    
    var body: some View {
        ScrollViewReader { proxy in
            ScrollView(.vertical, showsIndicators: false) {
                VStack(spacing: 12) {
                    // Header with back button
                    HStack {
                        Button("← Back") {
                            synthController.closeLFOConfig()
                        }
                        .font(.custom("FormaDJRText-Regular-Testing", size: 14))
                        .foregroundColor(.primary)
                        
                        Spacer()
                        
                        Text("LFO \(synthController.selectedLFO) Configuration")
                            .font(.custom("FormaDJRText-Bold-Testing", size: 16))
                            .foregroundColor(.primary)
                        
                        Spacer()
                    }
                    .padding(.bottom, 12)
                    
                    // LFO Type Selection
                    VStack(spacing: 6) {
                        Text("Waveform type")
                            .font(.custom("FormaDJRText-Bold-Testing", size: 8))
                            .foregroundColor(.primary)
                        
                        HStack(spacing: 8) {
                            ForEach(lfoTypes, id: \.self) { type in
                                Button(action: {
                                    synthController.setLFOType(synthController.selectedLFO, type: type)
                                }) {
                                    VStack(spacing: 4) {
                                        Text(type == "Sine" ? "∿" : type == "Saw" ? "⊿" : type == "Square" ? "⎕" : type == "Triangle" ? "▲" : "•")
                                            .font(.custom("FormaDJRText-Regular-Testing", size: 20))
                                            .foregroundColor(synthController.getLFOType(for: synthController.selectedLFO) == type ? .primary : .secondary)
                                        Text(type)
                                            .font(.custom("FormaDJRText-Regular-Testing", size: 10))
                                            .foregroundColor(synthController.getLFOType(for: synthController.selectedLFO) == type ? .primary : .secondary)
                                    }
                                }
                                .padding(.horizontal, 8)
                                .padding(.vertical, 6)
                                .background(
                                    RoundedRectangle(cornerRadius: 4)
                                        .fill(synthController.getLFOType(for: synthController.selectedLFO) == type ? 
                                              Color.gray.opacity(0.4) : Color.white)
                                        .overlay(
                                            RoundedRectangle(cornerRadius: 4)
                                                .stroke(Color.gray.opacity(0.4), lineWidth: 1)
                                        )
                                )
                            }
                        }
                    }
                    .id("type")
                    
                    // LFO Parameters (fixed height)
                    VStack(spacing: 4) {
                        Text("Parameters")
                            .font(.custom("FormaDJRText-Bold-Testing", size: 8))
                            .foregroundColor(.primary)
                        
                        VStack(spacing: 4) {
                            // Rate parameter
                            HStack {
                                Text("Rate")
                                    .font(.custom("FormaDJRText-Regular-Testing", size: 6))
                                Spacer()
                                Text(String(format: "%.2f", synthController.getLFORate(for: synthController.selectedLFO)))
                                    .font(.custom("FormaDJRText-Regular-Testing", size: 6))
                            }
                            .padding(.horizontal, 8)
                            .padding(.vertical, 2)
                            .background(
                                RoundedRectangle(cornerRadius: 2)
                                    .fill(Color.white.opacity(0.5))
                            )
                            
                            // Depth parameter
                            HStack {
                                Text("Depth")
                                    .font(.custom("FormaDJRText-Regular-Testing", size: 6))
                                Spacer()
                                Text(String(format: "%.2f", synthController.getLFODepth(for: synthController.selectedLFO)))
                                    .font(.custom("FormaDJRText-Regular-Testing", size: 6))
                            }
                            .padding(.horizontal, 8)
                            .padding(.vertical, 2)
                            .background(
                                RoundedRectangle(cornerRadius: 2)
                                    .fill(Color.white.opacity(0.5))
                            )
                            
                            // Phase parameter
                            HStack {
                                Text("Phase")
                                    .font(.custom("FormaDJRText-Regular-Testing", size: 6))
                                Spacer()
                                Text(String(format: "%.0f°", synthController.getLFOPhase(for: synthController.selectedLFO) * 360))
                                    .font(.custom("FormaDJRText-Regular-Testing", size: 6))
                            }
                            .padding(.horizontal, 8)
                            .padding(.vertical, 2)
                            .background(
                                RoundedRectangle(cornerRadius: 2)
                                    .fill(Color.white.opacity(0.5))
                            )
                        }
                    }
                    .frame(height: 80)
                    .id("parameters")
                    
                    // Assignment Section
                    VStack(spacing: 6) {
                        Text("Assignments")
                            .font(.custom("FormaDJRText-Bold-Testing", size: 8))
                            .foregroundColor(.primary)
                        
                        // Mock assignments display
                        VStack(spacing: 2) {
                            ForEach(0..<3, id: \.self) { assignmentIndex in
                                HStack {
                                    Circle()
                                        .fill(assignmentIndex == 0 ? InstrumentColor.coral.pastelColor : Color.gray.opacity(0.3))
                                        .frame(width: 8, height: 8)
                                    
                                    Text(assignmentIndex == 0 ? "Coral filter cutoff" : "Unassigned")
                                        .font(.custom("FormaDJRText-Regular-Testing", size: 6))
                                        .foregroundColor(assignmentIndex == 0 ? .primary : .secondary)
                                    
                                    Spacer()
                                    
                                    if assignmentIndex == 0 {
                                        Text("×")
                                            .font(.custom("FormaDJRText-Regular-Testing", size: 6))
                                            .foregroundColor(.secondary)
                                    }
                                }
                                .padding(.horizontal, 8)
                                .padding(.vertical, 2)
                                .background(
                                    RoundedRectangle(cornerRadius: 2)
                                        .fill(assignmentIndex == 0 ? Color.gray.opacity(0.1) : Color.clear)
                                        .overlay(
                                            RoundedRectangle(cornerRadius: 2)
                                                .stroke(Color.gray.opacity(0.3), lineWidth: 1)
                                        )
                                )
                            }
                        }
                    }
                    .id("assignments")
                }
                .padding(12)
            }
            .onAppear {
                // Restore scroll position for this LFO
                if let position = synthController.lfoScrollPositions[synthController.selectedLFO] {
                    proxy.scrollTo("parameters", anchor: .center)
                }
            }
        }
    }
}

// MARK: - Instrument Browser View
struct InstrumentBrowserView: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 8) {
            // Header with back button
            HStack {
                Button("← Back") {
                    synthController.closeInstrumentBrowser()
                }
                .font(.custom("FormaDJRText-Regular-Testing", size: 10))
                .foregroundColor(.primary)
                
                Spacer()
                
                Text("Select engine")
                    .font(.custom("FormaDJRText-Bold-Testing", size: 12))
                    .foregroundColor(.primary)
            }
            .padding(.bottom, 8)
            
            ScrollView {
                VStack(spacing: 8) {
                    // Get real C++ engine types
                    ForEach(Array(synthController.getAvailableEngineTypes().enumerated()), id: \.offset) { index, engineInfo in
                        let (engineType, engineName) = engineInfo
                        
                        Button(action: {
                            if let slot = synthController.selectedInstrumentSlot {
                                synthController.assignEngine(engineType: engineType, to: slot)
                            }
                        }) {
                            HStack {
                                Text(engineName)
                                    .font(.custom("FormaDJRText-Regular-Testing", size: 10))
                                    .foregroundColor(.primary)
                                Spacer()
                            }
                            .padding(.horizontal, 12)
                            .padding(.vertical, 8)
                            .background(
                                RoundedRectangle(cornerRadius: 3)
                                    .fill(Color.white)
                                    .overlay(
                                        RoundedRectangle(cornerRadius: 3)
                                            .stroke(Color.gray.opacity(0.5), lineWidth: 1)
                                    )
                            )
                        }
                        .buttonStyle(PlainButtonStyle())
                    }
                }
                .padding(.horizontal, 4)
            }
        }
    }
}