import SwiftUI

/**
 * Pattern Overlay - Euclidean and drum pattern sequencing
 * Different interfaces for pitched instruments vs. drums
 */

// MARK: - Pattern Models
struct EuclideanPattern {
    var hits: Int
    var steps: Int
    var rotation: Int
    var accent: Float
    
    var pattern: [Bool] {
        generateEuclideanPattern(hits: hits, steps: steps, rotation: rotation)
    }
}

struct DrumPattern {
    var steps: [DrumStep]
    var length: Int
    var swing: Float
    
    init(length: Int = 16) {
        self.length = length
        self.swing = 0.0
        self.steps = (0..<length).map { _ in DrumStep() }
    }
}

struct DrumStep {
    var active: Bool = false
    var velocity: Float = 0.8
    var accent: Bool = false
    var flam: Bool = false
    var probability: Float = 1.0
}

enum PatternType {
    case euclidean // For pitched instruments
    case drum      // For drum machines
    case sample    // For samplers
}

// MARK: - Main Pattern Overlay
struct PatternOverlay: View {
    let trackId: Int
    let trackColor: InstrumentColor
    let engineType: Int
    let patternType: PatternType
    
    @Environment(\.presentationMode) var presentationMode
    @State private var euclideanPattern = EuclideanPattern(hits: 4, steps: 16, rotation: 0, accent: 0.8)
    @State private var drumPattern = DrumPattern()
    @State private var currentStep: Int = 0
    @State private var isPlaying: Bool = false
    @State private var selectedDrumSlot: Int = 0
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            PatternOverlayHeader(
                trackId: trackId,
                trackColor: trackColor,
                patternType: patternType,
                onClose: { presentationMode.wrappedValue.dismiss() }
            )
            .padding(.horizontal, 20)
            .padding(.vertical, 16)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(trackColor.pastelColor.opacity(0.1))
            )
            
            // Pattern Content
            switch patternType {
            case .euclidean:
                EuclideanPatternView(
                    pattern: $euclideanPattern,
                    currentStep: currentStep,
                    trackColor: trackColor,
                    isPlaying: isPlaying
                )
                .padding(.horizontal, 20)
                .padding(.vertical, 16)
                
            case .drum:
                DrumPatternView(
                    pattern: $drumPattern,
                    selectedSlot: $selectedDrumSlot,
                    currentStep: currentStep,
                    trackColor: trackColor,
                    isPlaying: isPlaying
                )
                .padding(.horizontal, 20)
                .padding(.vertical, 16)
                
            case .sample:
                SamplePatternView(
                    currentStep: currentStep,
                    trackColor: trackColor,
                    isPlaying: isPlaying
                )
                .padding(.horizontal, 20)
                .padding(.vertical, 16)
            }
            
            Spacer()
            
            // Transport & Actions
            PatternTransportControls(
                isPlaying: $isPlaying,
                onPlay: { startPattern() },
                onStop: { stopPattern() },
                onClear: { clearPattern() },
                onRandomize: { randomizePattern() }
            )
            .padding(.horizontal, 20)
            .padding(.bottom, 20)
        }
        .frame(width: 800, height: 600)
        .background(
            RoundedRectangle(cornerRadius: 16)
                .fill(Color.white)
                .shadow(color: Color.black.opacity(0.3), radius: 12, x: 0, y: 8)
        )
        .onAppear {
            startStepTimer()
        }
    }
    
    // MARK: - Helper Functions
    private func startPattern() {
        isPlaying = true
    }
    
    private func stopPattern() {
        isPlaying = false
        currentStep = 0
    }
    
    private func clearPattern() {
        switch patternType {
        case .euclidean:
            euclideanPattern.hits = 0
        case .drum:
            drumPattern.steps = drumPattern.steps.map { _ in DrumStep() }
        case .sample:
            break
        }
    }
    
    private func randomizePattern() {
        switch patternType {
        case .euclidean:
            euclideanPattern.hits = Int.random(in: 1...euclideanPattern.steps)
            euclideanPattern.rotation = Int.random(in: 0..<euclideanPattern.steps)
        case .drum:
            drumPattern.steps = drumPattern.steps.map { _ in
                DrumStep(
                    active: Bool.random(),
                    velocity: Float.random(in: 0.5...1.0),
                    accent: Bool.random() && Bool.random(), // 25% chance
                    probability: Float.random(in: 0.7...1.0)
                )
            }
        case .sample:
            break
        }
    }
    
    private func startStepTimer() {
        Timer.scheduledTimer(withTimeInterval: 0.125, repeats: true) { _ in
            if isPlaying {
                currentStep = (currentStep + 1) % max(euclideanPattern.steps, drumPattern.length)
            }
        }
    }
}

// MARK: - Pattern Overlay Header
struct PatternOverlayHeader: View {
    let trackId: Int
    let trackColor: InstrumentColor
    let patternType: PatternType
    let onClose: () -> Void
    
    var patternTypeName: String {
        switch patternType {
        case .euclidean: return "Euclidean Pattern"
        case .drum: return "Drum Pattern"
        case .sample: return "Sample Pattern"
        }
    }
    
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
                    
                    Text(patternTypeName)
                        .font(.system(size: 12))
                        .foregroundColor(.secondary)
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

// MARK: - Euclidean Pattern View
struct EuclideanPatternView: View {
    @Binding var pattern: EuclideanPattern
    let currentStep: Int
    let trackColor: InstrumentColor
    let isPlaying: Bool
    
    var body: some View {
        VStack(spacing: 24) {
            // Euclidean Parameters
            VStack(spacing: 16) {
                Text("Euclidean Generator")
                    .font(.system(size: 16, weight: .bold))
                    .foregroundColor(.primary)
                
                HStack(spacing: 32) {
                    // Hits
                    VStack(spacing: 8) {
                        Text("Hits")
                            .font(.system(size: 12, weight: .medium))
                            .foregroundColor(.secondary)
                        
                        Stepper("\(pattern.hits)", value: $pattern.hits, in: 0...pattern.steps)
                            .labelsHidden()
                        
                        Text("\(pattern.hits)")
                            .font(.system(size: 18, weight: .bold))
                            .foregroundColor(trackColor.pastelColor)
                    }
                    
                    // Steps
                    VStack(spacing: 8) {
                        Text("Steps")
                            .font(.system(size: 12, weight: .medium))
                            .foregroundColor(.secondary)
                        
                        Stepper("\(pattern.steps)", value: $pattern.steps, in: 1...32)
                            .labelsHidden()
                        
                        Text("\(pattern.steps)")
                            .font(.system(size: 18, weight: .bold))
                            .foregroundColor(trackColor.pastelColor)
                    }
                    
                    // Rotation
                    VStack(spacing: 8) {
                        Text("Rotation")
                            .font(.system(size: 12, weight: .medium))
                            .foregroundColor(.secondary)
                        
                        Stepper("\(pattern.rotation)", value: $pattern.rotation, in: 0...max(0, pattern.steps-1))
                            .labelsHidden()
                        
                        Text("\(pattern.rotation)")
                            .font(.system(size: 18, weight: .bold))
                            .foregroundColor(trackColor.pastelColor)
                    }
                    
                    // Accent
                    VStack(spacing: 8) {
                        Text("Accent")
                            .font(.system(size: 12, weight: .medium))
                            .foregroundColor(.secondary)
                        
                        Slider(value: $pattern.accent, in: 0...1)
                            .frame(width: 80)
                            .accentColor(trackColor.pastelColor)
                        
                        Text("\(Int(pattern.accent * 100))%")
                            .font(.system(size: 12, weight: .bold))
                            .foregroundColor(trackColor.pastelColor)
                    }
                }
            }
            .padding(.vertical, 20)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(trackColor.pastelColor.opacity(0.05))
            )
            
            // Pattern Visualization
            VStack(spacing: 16) {
                Text("Pattern: \(pattern.hits) hits / \(pattern.steps) steps, rotation \(pattern.rotation)")
                    .font(.system(size: 14, weight: .medium))
                    .foregroundColor(.secondary)
                
                // Step Grid
                let columns = Array(repeating: GridItem(.flexible(), spacing: 4), count: min(pattern.steps, 16))
                
                LazyVGrid(columns: columns, spacing: 4) {
                    ForEach(0..<pattern.steps, id: \.self) { step in
                        let isActive = pattern.pattern[step]
                        let isCurrent = step == currentStep && isPlaying
                        
                        EuclideanStepView(
                            step: step + 1,
                            isActive: isActive,
                            isCurrent: isCurrent,
                            color: trackColor.pastelColor
                        )
                    }
                }
                
                if pattern.steps > 16 {
                    // Second row for patterns > 16 steps
                    let secondRowColumns = Array(repeating: GridItem(.flexible(), spacing: 4), count: pattern.steps - 16)
                    
                    LazyVGrid(columns: secondRowColumns, spacing: 4) {
                        ForEach(16..<pattern.steps, id: \.self) { step in
                            let isActive = pattern.pattern[step]
                            let isCurrent = step == currentStep && isPlaying
                            
                            EuclideanStepView(
                                step: step + 1,
                                isActive: isActive,
                                isCurrent: isCurrent,
                                color: trackColor.pastelColor
                            )
                        }
                    }
                }
            }
        }
    }
}

// MARK: - Euclidean Step View
struct EuclideanStepView: View {
    let step: Int
    let isActive: Bool
    let isCurrent: Bool
    let color: Color
    
    var body: some View {
        VStack(spacing: 4) {
            Circle()
                .fill(isActive ? color : Color.gray.opacity(0.2))
                .frame(width: 32, height: 32)
                .overlay(
                    Circle()
                        .stroke(isCurrent ? Color.red : Color.clear, lineWidth: 3)
                )
                .overlay(
                    Text("\(step)")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(isActive ? .white : .secondary)
                )
            
            if step % 4 == 1 {
                Rectangle()
                    .fill(Color.primary)
                    .frame(width: 1, height: 8)
            } else {
                Rectangle()
                    .fill(Color.secondary.opacity(0.3))
                    .frame(width: 1, height: 4)
            }
        }
    }
}

// MARK: - Drum Pattern View
struct DrumPatternView: View {
    @Binding var pattern: DrumPattern
    @Binding var selectedSlot: Int
    let currentStep: Int
    let trackColor: InstrumentColor
    let isPlaying: Bool
    
    private let drumNames = [
        "Kick", "Snare", "Hi-Hat", "Open Hat", "Crash", "Ride",
        "Tom 1", "Tom 2", "Tom 3", "Clap", "Rim", "Cowbell"
    ]
    
    var body: some View {
        VStack(spacing: 20) {
            // Drum Slot Selector
            VStack(spacing: 12) {
                Text("Drum Slot")
                    .font(.system(size: 14, weight: .bold))
                    .foregroundColor(.primary)
                
                ScrollView(.horizontal, showsIndicators: false) {
                    HStack(spacing: 8) {
                        ForEach(0..<12, id: \.self) { slot in
                            Button(action: { selectedSlot = slot }) {
                                VStack(spacing: 4) {
                                    Circle()
                                        .fill(selectedSlot == slot ? trackColor.pastelColor : Color.gray.opacity(0.2))
                                        .frame(width: 32, height: 32)
                                        .overlay(
                                            Text("\(slot + 1)")
                                                .font(.system(size: 12, weight: .medium))
                                                .foregroundColor(selectedSlot == slot ? .white : .secondary)
                                        )
                                    
                                    Text(drumNames[slot])
                                        .font(.system(size: 8))
                                        .foregroundColor(.secondary)
                                        .lineLimit(1)
                                }
                            }
                            .buttonStyle(PlainButtonStyle())
                        }
                    }
                    .padding(.horizontal, 4)
                }
            }
            
            // Step Sequencer
            VStack(spacing: 16) {
                Text("Step Sequencer - \(drumNames[selectedSlot])")
                    .font(.system(size: 14, weight: .bold))
                    .foregroundColor(.primary)
                
                // Steps Grid
                LazyVGrid(columns: Array(repeating: GridItem(.flexible(), spacing: 4), count: 16), spacing: 8) {
                    ForEach(0..<pattern.length, id: \.self) { step in
                        DrumStepView(
                            step: step,
                            drumStep: $pattern.steps[step],
                            isCurrent: step == currentStep && isPlaying,
                            color: trackColor.pastelColor
                        )
                    }
                }
                
                // Pattern Controls
                HStack(spacing: 16) {
                    VStack(spacing: 4) {
                        Text("Length")
                            .font(.system(size: 10, weight: .medium))
                            .foregroundColor(.secondary)
                        
                        Stepper("\(pattern.length)", value: $pattern.length, in: 1...32)
                            .labelsHidden()
                        
                        Text("\(pattern.length)")
                            .font(.system(size: 12, weight: .bold))
                            .foregroundColor(trackColor.pastelColor)
                    }
                    
                    VStack(spacing: 4) {
                        Text("Swing")
                            .font(.system(size: 10, weight: .medium))
                            .foregroundColor(.secondary)
                        
                        Slider(value: $pattern.swing, in: -0.5...0.5)
                            .frame(width: 80)
                            .accentColor(trackColor.pastelColor)
                        
                        Text("\(Int(pattern.swing * 100))%")
                            .font(.system(size: 12, weight: .bold))
                            .foregroundColor(trackColor.pastelColor)
                    }
                }
            }
        }
    }
}

// MARK: - Drum Step View
struct DrumStepView: View {
    let step: Int
    @Binding var drumStep: DrumStep
    let isCurrent: Bool
    let color: Color
    
    var body: some View {
        Button(action: { drumStep.active.toggle() }) {
            VStack(spacing: 2) {
                RoundedRectangle(cornerRadius: 6)
                    .fill(drumStep.active ? color : Color.gray.opacity(0.2))
                    .frame(width: 32, height: 32)
                    .overlay(
                        RoundedRectangle(cornerRadius: 6)
                            .stroke(isCurrent ? Color.red : Color.clear, lineWidth: 2)
                    )
                    .overlay(
                        VStack(spacing: 1) {
                            Text("\(step + 1)")
                                .font(.system(size: 8, weight: .medium))
                                .foregroundColor(drumStep.active ? .white : .secondary)
                            
                            if drumStep.accent {
                                Circle()
                                    .fill(Color.yellow)
                                    .frame(width: 4, height: 4)
                            }
                        }
                    )
                
                // Velocity bar
                Rectangle()
                    .fill(drumStep.active ? color.opacity(0.6) : Color.clear)
                    .frame(width: CGFloat(drumStep.velocity * 24), height: 3)
                    .animation(.easeInOut(duration: 0.1), value: drumStep.velocity)
            }
        }
        .buttonStyle(PlainButtonStyle())
        .contextMenu {
            Button("Accent") { drumStep.accent.toggle() }
            Button("Flam") { drumStep.flam.toggle() }
            Slider(value: $drumStep.velocity, in: 0...1) {
                Text("Velocity")
            }
            Slider(value: $drumStep.probability, in: 0...1) {
                Text("Probability")
            }
        }
    }
}

// MARK: - Sample Pattern View (Placeholder)
struct SamplePatternView: View {
    let currentStep: Int
    let trackColor: InstrumentColor
    let isPlaying: Bool
    
    var body: some View {
        VStack {
            Text("Sample Pattern")
                .font(.headline)
            
            Text("Sample-specific pattern controls would go here")
                .foregroundColor(.secondary)
            
            Text("Including slice triggers, loop points, and probability")
                .font(.caption)
                .foregroundColor(.secondary)
        }
        .frame(maxWidth: .infinity, minHeight: 200)
        .background(
            RoundedRectangle(cornerRadius: 12)
                .fill(trackColor.pastelColor.opacity(0.05))
        )
    }
}

// MARK: - Pattern Transport Controls
struct PatternTransportControls: View {
    @Binding var isPlaying: Bool
    let onPlay: () -> Void
    let onStop: () -> Void
    let onClear: () -> Void
    let onRandomize: () -> Void
    
    var body: some View {
        HStack(spacing: 16) {
            // Transport
            HStack(spacing: 8) {
                Button(action: isPlaying ? onStop : onPlay) {
                    HStack(spacing: 4) {
                        Image(systemName: isPlaying ? "pause.fill" : "play.fill")
                            .font(.system(size: 12))
                        Text(isPlaying ? "Pause" : "Play")
                            .font(.system(size: 12, weight: .medium))
                    }
                    .foregroundColor(.white)
                    .padding(.horizontal, 16)
                    .padding(.vertical, 8)
                    .background(
                        RoundedRectangle(cornerRadius: 20)
                            .fill(isPlaying ? Color.orange : Color.green)
                            .shadow(color: Color.black.opacity(0.2), radius: 3, x: 1, y: 1)
                    )
                }
                .buttonStyle(PlainButtonStyle())
                
                Button(action: onStop) {
                    HStack(spacing: 4) {
                        Image(systemName: "stop.fill")
                            .font(.system(size: 12))
                        Text("Stop")
                            .font(.system(size: 12, weight: .medium))
                    }
                    .foregroundColor(.primary)
                    .padding(.horizontal, 16)
                    .padding(.vertical, 8)
                    .background(
                        RoundedRectangle(cornerRadius: 20)
                            .fill(Color.white)
                            .shadow(color: Color.black.opacity(0.1), radius: 2, x: 1, y: 1)
                    )
                }
                .buttonStyle(PlainButtonStyle())
            }
            
            Spacer()
            
            // Pattern Actions
            HStack(spacing: 8) {
                Button(action: onClear) {
                    Text("Clear")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundColor(.red)
                        .padding(.horizontal, 12)
                        .padding(.vertical, 6)
                        .background(
                            RoundedRectangle(cornerRadius: 16)
                                .fill(Color.red.opacity(0.1))
                        )
                }
                .buttonStyle(PlainButtonStyle())
                
                Button(action: onRandomize) {
                    Text("Randomize")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundColor(.blue)
                        .padding(.horizontal, 12)
                        .padding(.vertical, 6)
                        .background(
                            RoundedRectangle(cornerRadius: 16)
                                .fill(Color.blue.opacity(0.1))
                        )
                }
                .buttonStyle(PlainButtonStyle())
            }
        }
    }
}

// MARK: - Euclidean Algorithm
func generateEuclideanPattern(hits: Int, steps: Int, rotation: Int = 0) -> [Bool] {
    guard hits > 0 && steps > 0 && hits <= steps else {
        return Array(repeating: false, count: max(steps, 1))
    }
    
    var pattern = Array(repeating: false, count: steps)
    
    // Bresenham-like algorithm for Euclidean rhythms
    var slope = Double(hits) / Double(steps)
    var previous = -1.0
    
    for step in 0..<steps {
        let current = slope * Double(step)
        if floor(current) != floor(previous) {
            pattern[step] = true
        }
        previous = current
    }
    
    // Apply rotation
    if rotation > 0 {
        let rotationAmount = rotation % steps
        let rotatedPattern = Array(pattern.dropFirst(rotationAmount) + pattern.prefix(rotationAmount))
        return rotatedPattern
    }
    
    return pattern
}