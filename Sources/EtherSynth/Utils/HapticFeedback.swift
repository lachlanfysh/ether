import SwiftUI
import CoreHaptics

/**
 * Haptic Feedback Manager - Touch-optimized feedback system for macOS
 * Provides contextual haptic feedback for UI interactions and audio events
 */

@MainActor
class HapticFeedbackManager: ObservableObject {
    private var hapticEngine: CHHapticEngine?
    private var isHapticsEnabled: Bool = true
    private var feedbackIntensity: Float = 0.7
    
    @Published var isSupported: Bool = false
    @Published var isRunning: Bool = false
    @Published var errorMessage: String? = nil
    
    // Feedback type definitions
    enum FeedbackType {
        // UI Interactions
        case buttonPress
        case toggle
        case sliderMove
        case knobTurn
        case menuOpen
        case menuClose
        case tabSwitch
        case modalOpen
        case modalClose
        
        // Transport
        case playStart
        case playStop
        case recordStart
        case recordStop
        case seekJump
        
        // Timeline
        case notePlace
        case noteDelete
        case patternSelect
        case patternMove
        case quantize
        case loopPoint
        
        // Synthesis
        case engineAssign
        case parameterChange
        case presetLoad
        case modConnection
        
        // Mix
        case muteToggle
        case soloToggle
        case faderMove
        case effectBypass
        
        // Audio Events
        case noteOn
        case noteOff
        case beatTick
        case downbeat
        case clipLimit
        case dropout
        
        // Alerts
        case success
        case warning
        case error
        case notification
        
        var hapticPattern: HapticPattern {
            switch self {
            // UI Interactions - Light, crisp feedback
            case .buttonPress:
                return HapticPattern(intensity: 0.6, sharpness: 0.8, duration: 0.05)
            case .toggle:
                return HapticPattern(intensity: 0.7, sharpness: 0.6, duration: 0.08)
            case .sliderMove:
                return HapticPattern(intensity: 0.3, sharpness: 0.9, duration: 0.03)
            case .knobTurn:
                return HapticPattern(intensity: 0.4, sharpness: 0.7, duration: 0.04)
            case .menuOpen:
                return HapticPattern(intensity: 0.5, sharpness: 0.5, duration: 0.1)
            case .menuClose:
                return HapticPattern(intensity: 0.4, sharpness: 0.6, duration: 0.08)
            case .tabSwitch:
                return HapticPattern(intensity: 0.6, sharpness: 0.7, duration: 0.06)
            case .modalOpen:
                return HapticPattern(intensity: 0.8, sharpness: 0.4, duration: 0.12)
            case .modalClose:
                return HapticPattern(intensity: 0.6, sharpness: 0.6, duration: 0.08)
                
            // Transport - Pronounced feedback for important actions
            case .playStart:
                return HapticPattern(intensity: 0.9, sharpness: 0.6, duration: 0.15)
            case .playStop:
                return HapticPattern(intensity: 0.8, sharpness: 0.7, duration: 0.12)
            case .recordStart:
                return HapticPattern(intensity: 1.0, sharpness: 0.5, duration: 0.18, pulseCount: 2)
            case .recordStop:
                return HapticPattern(intensity: 0.9, sharpness: 0.6, duration: 0.15)
            case .seekJump:
                return HapticPattern(intensity: 0.5, sharpness: 0.8, duration: 0.06)
                
            // Timeline - Precision feedback for musical timing
            case .notePlace:
                return HapticPattern(intensity: 0.6, sharpness: 0.9, duration: 0.04)
            case .noteDelete:
                return HapticPattern(intensity: 0.5, sharpness: 0.8, duration: 0.05)
            case .patternSelect:
                return HapticPattern(intensity: 0.7, sharpness: 0.6, duration: 0.08)
            case .patternMove:
                return HapticPattern(intensity: 0.4, sharpness: 0.7, duration: 0.05)
            case .quantize:
                return HapticPattern(intensity: 0.8, sharpness: 0.9, duration: 0.06, pulseCount: 3)
            case .loopPoint:
                return HapticPattern(intensity: 0.9, sharpness: 0.5, duration: 0.1)
                
            // Synthesis - Smooth feedback for sound design
            case .engineAssign:
                return HapticPattern(intensity: 0.8, sharpness: 0.5, duration: 0.12)
            case .parameterChange:
                return HapticPattern(intensity: 0.2, sharpness: 0.8, duration: 0.02)
            case .presetLoad:
                return HapticPattern(intensity: 0.7, sharpness: 0.6, duration: 0.1)
            case .modConnection:
                return HapticPattern(intensity: 0.6, sharpness: 0.7, duration: 0.08)
                
            // Mix - Professional mixing feedback
            case .muteToggle:
                return HapticPattern(intensity: 0.8, sharpness: 0.6, duration: 0.08)
            case .soloToggle:
                return HapticPattern(intensity: 0.9, sharpness: 0.5, duration: 0.1)
            case .faderMove:
                return HapticPattern(intensity: 0.3, sharpness: 0.7, duration: 0.03)
            case .effectBypass:
                return HapticPattern(intensity: 0.6, sharpness: 0.8, duration: 0.06)
                
            // Audio Events - Rhythmic and musical feedback
            case .noteOn:
                return HapticPattern(intensity: 0.4, sharpness: 0.9, duration: 0.03)
            case .noteOff:
                return HapticPattern(intensity: 0.2, sharpness: 0.8, duration: 0.02)
            case .beatTick:
                return HapticPattern(intensity: 0.3, sharpness: 0.9, duration: 0.02)
            case .downbeat:
                return HapticPattern(intensity: 0.6, sharpness: 0.7, duration: 0.05)
            case .clipLimit:
                return HapticPattern(intensity: 1.0, sharpness: 1.0, duration: 0.08, pulseCount: 2)
            case .dropout:
                return HapticPattern(intensity: 0.9, sharpness: 0.9, duration: 0.1, pulseCount: 3)
                
            // Alerts - Clear feedback for notifications
            case .success:
                return HapticPattern(intensity: 0.8, sharpness: 0.5, duration: 0.12, pulseCount: 1)
            case .warning:
                return HapticPattern(intensity: 0.9, sharpness: 0.7, duration: 0.1, pulseCount: 2)
            case .error:
                return HapticPattern(intensity: 1.0, sharpness: 0.8, duration: 0.15, pulseCount: 3)
            case .notification:
                return HapticPattern(intensity: 0.6, sharpness: 0.6, duration: 0.08)
            }
        }
    }
    
    struct HapticPattern {
        let intensity: Float
        let sharpness: Float
        let duration: TimeInterval
        let pulseCount: Int
        
        init(intensity: Float, sharpness: Float, duration: TimeInterval, pulseCount: Int = 1) {
            self.intensity = intensity
            self.sharpness = sharpness
            self.duration = duration
            self.pulseCount = pulseCount
        }
    }
    
    init() {
        setupHapticEngine()
        loadSettings()
    }
    
    deinit {
        hapticEngine?.stop()
    }
    
    // MARK: - Setup and Configuration
    
    private func setupHapticEngine() {
        // Check if haptics are supported
        guard CHHapticEngine.capabilitiesForHardware().supportsHaptics else {
            isSupported = false
            return
        }
        
        isSupported = true
        
        do {
            hapticEngine = try CHHapticEngine()
            hapticEngine?.resetHandler = handleEngineReset
            hapticEngine?.stoppedHandler = handleEngineStopped
            
            try hapticEngine?.start()
            isRunning = true
            
        } catch {
            print("Failed to create haptic engine: \(error)")
            errorMessage = "Failed to initialize haptic feedback: \(error.localizedDescription)"
            isSupported = false
        }
    }
    
    private func handleEngineReset() {
        print("Haptic engine reset")
        
        do {
            try hapticEngine?.start()
            isRunning = true
        } catch {
            print("Failed to restart haptic engine: \(error)")
            errorMessage = "Failed to restart haptic feedback: \(error.localizedDescription)"
            isRunning = false
        }
    }
    
    private func handleEngineStopped(reason: CHHapticEngine.StoppedReason) {
        print("Haptic engine stopped: \(reason)")
        isRunning = false
        
        switch reason {
        case .audioSessionInterrupt:
            print("Audio session interrupt")
        case .applicationSuspended:
            print("Application suspended")
        case .idleTimeout:
            print("Idle timeout")
        case .systemError:
            print("System error")
            errorMessage = "Haptic feedback system error"
        case .notifyWhenFinished:
            print("Finished")
        case .gameControllerDisconnect:
            print("Game controller disconnect")
        case .engineDestroyed:
            print("Engine destroyed")
        @unknown default:
            print("Unknown reason")
        }
    }
    
    // MARK: - Public Interface
    
    func triggerFeedback(_ type: FeedbackType) {
        guard isHapticsEnabled && isSupported && isRunning else { return }
        
        let pattern = type.hapticPattern
        playHapticPattern(pattern)
    }
    
    func triggerCustomFeedback(intensity: Float, sharpness: Float, duration: TimeInterval) {
        guard isHapticsEnabled && isSupported && isRunning else { return }
        
        let pattern = HapticPattern(intensity: intensity, sharpness: sharpness, duration: duration)
        playHapticPattern(pattern)
    }
    
    func setEnabled(_ enabled: Bool) {
        isHapticsEnabled = enabled
        saveSettings()
    }
    
    func setIntensity(_ intensity: Float) {
        feedbackIntensity = max(0.0, min(1.0, intensity))
        saveSettings()
    }
    
    var isEnabled: Bool {
        return isHapticsEnabled
    }
    
    var intensity: Float {
        return feedbackIntensity
    }
    
    // MARK: - Haptic Pattern Playback
    
    private func playHapticPattern(_ pattern: HapticPattern) {
        guard let engine = hapticEngine else { return }
        
        do {
            let hapticEvent = try createHapticEvent(from: pattern)
            let player = try engine.makePlayer(with: hapticEvent)
            try player.start(atTime: 0)
            
        } catch {
            print("Failed to play haptic pattern: \(error)")
        }
    }
    
    private func createHapticEvent(from pattern: HapticPattern) throws -> CHHapticPattern {
        var events: [CHHapticEvent] = []
        
        let adjustedIntensity = pattern.intensity * feedbackIntensity
        let pulseDuration = pattern.duration / Double(pattern.pulseCount)
        let pulseGap = pulseDuration * 0.2 // 20% gap between pulses
        
        for i in 0..<pattern.pulseCount {
            let startTime = Double(i) * (pulseDuration + pulseGap)
            
            let intensityParameter = CHHapticEventParameter(
                parameterID: .hapticIntensity,
                value: adjustedIntensity
            )
            
            let sharpnessParameter = CHHapticEventParameter(
                parameterID: .hapticSharpness,
                value: pattern.sharpness
            )
            
            let event = CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [intensityParameter, sharpnessParameter],
                relativeTime: startTime
            )
            
            events.append(event)
        }
        
        return try CHHapticPattern(events: events, parameters: [])
    }
    
    // MARK: - Settings Persistence
    
    private func loadSettings() {
        let defaults = UserDefaults.standard
        isHapticsEnabled = defaults.bool(forKey: "HapticsEnabled")
        feedbackIntensity = defaults.float(forKey: "HapticsIntensity") != 0 ? 
                          defaults.float(forKey: "HapticsIntensity") : 0.7
    }
    
    private func saveSettings() {
        let defaults = UserDefaults.standard
        defaults.set(isHapticsEnabled, forKey: "HapticsEnabled")
        defaults.set(feedbackIntensity, forKey: "HapticsIntensity")
    }
    
    // MARK: - Convenience Methods for Common UI Elements
    
    func buttonPressed() {
        triggerFeedback(.buttonPress)
    }
    
    func toggleSwitched() {
        triggerFeedback(.toggle)
    }
    
    func sliderMoved() {
        triggerFeedback(.sliderMove)
    }
    
    func knobTurned() {
        triggerFeedback(.knobTurn)
    }
    
    func noteTriggered() {
        triggerFeedback(.noteOn)
    }
    
    func beatTicked() {
        triggerFeedback(.beatTick)
    }
    
    func downbeatHit() {
        triggerFeedback(.downbeat)
    }
    
    func playbackStarted() {
        triggerFeedback(.playStart)
    }
    
    func playbackStopped() {
        triggerFeedback(.playStop)
    }
    
    func recordingStarted() {
        triggerFeedback(.recordStart)
    }
    
    func recordingStopped() {
        triggerFeedback(.recordStop)
    }
    
    func engineAssigned() {
        triggerFeedback(.engineAssign)
    }
    
    func presetLoaded() {
        triggerFeedback(.presetLoad)
    }
    
    func errorOccurred() {
        triggerFeedback(.error)
    }
    
    func successAction() {
        triggerFeedback(.success)
    }
    
    func warningIssue() {
        triggerFeedback(.warning)
    }
}

// MARK: - SwiftUI Integration

struct HapticFeedbackModifier: ViewModifier {
    let feedbackType: HapticFeedbackManager.FeedbackType
    let hapticManager: HapticFeedbackManager
    
    func body(content: Content) -> some View {
        content
            .onTapGesture {
                hapticManager.triggerFeedback(feedbackType)
            }
    }
}

extension View {
    func hapticFeedback(
        _ type: HapticFeedbackManager.FeedbackType,
        manager: HapticFeedbackManager
    ) -> some View {
        self.modifier(HapticFeedbackModifier(feedbackType: type, hapticManager: manager))
    }
}

// MARK: - Haptic Button Component

struct HapticButton<Label: View>: View {
    let action: () -> Void
    let feedbackType: HapticFeedbackManager.FeedbackType
    let label: Label
    
    @EnvironmentObject private var hapticManager: HapticFeedbackManager
    
    init(
        feedbackType: HapticFeedbackManager.FeedbackType = .buttonPress,
        action: @escaping () -> Void,
        @ViewBuilder label: () -> Label
    ) {
        self.feedbackType = feedbackType
        self.action = action
        self.label = label()
    }
    
    var body: some View {
        Button(action: {
            hapticManager.triggerFeedback(feedbackType)
            action()
        }) {
            label
        }
    }
}

// MARK: - Haptic Toggle Component

struct HapticToggle: View {
    @Binding var isOn: Bool
    let label: String
    let feedbackType: HapticFeedbackManager.FeedbackType
    
    @EnvironmentObject private var hapticManager: HapticFeedbackManager
    
    init(
        _ label: String,
        isOn: Binding<Bool>,
        feedbackType: HapticFeedbackManager.FeedbackType = .toggle
    ) {
        self.label = label
        self._isOn = isOn
        self.feedbackType = feedbackType
    }
    
    var body: some View {
        Toggle(label, isOn: Binding(
            get: { isOn },
            set: { newValue in
                hapticManager.triggerFeedback(feedbackType)
                isOn = newValue
            }
        ))
    }
}

// MARK: - Haptic Slider Component

struct HapticSlider: View {
    @Binding var value: Float
    let range: ClosedRange<Float>
    let label: String?
    let feedbackThreshold: Float
    
    @EnvironmentObject private var hapticManager: HapticFeedbackManager
    @State private var lastFeedbackValue: Float = 0
    
    init(
        value: Binding<Float>,
        in range: ClosedRange<Float>,
        label: String? = nil,
        feedbackThreshold: Float = 0.05
    ) {
        self._value = value
        self.range = range
        self.label = label
        self.feedbackThreshold = feedbackThreshold
    }
    
    var body: some View {
        VStack {
            if let label = label {
                Text(label)
                    .font(.caption)
            }
            
            Slider(
                value: Binding(
                    get: { value },
                    set: { newValue in
                        let normalizedThreshold = feedbackThreshold * (range.upperBound - range.lowerBound)
                        
                        if abs(newValue - lastFeedbackValue) >= normalizedThreshold {
                            hapticManager.triggerFeedback(.sliderMove)
                            lastFeedbackValue = newValue
                        }
                        
                        value = newValue
                    }
                ),
                in: range
            )
        }
        .onAppear {
            lastFeedbackValue = value
        }
    }
}

// MARK: - Haptic Settings View

struct HapticSettingsView: View {
    @ObservedObject var hapticManager: HapticFeedbackManager
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Haptic Feedback Settings")
                .font(.headline)
            
            VStack(spacing: 12) {
                HStack {
                    Text("Enable Haptic Feedback")
                    Spacer()
                    HapticToggle(
                        "Enable",
                        isOn: Binding(
                            get: { hapticManager.isEnabled },
                            set: { hapticManager.setEnabled($0) }
                        )
                    )
                    .labelsHidden()
                }
                
                VStack(alignment: .leading, spacing: 4) {
                    Text("Feedback Intensity")
                        .font(.subheadline)
                    
                    HapticSlider(
                        value: Binding(
                            get: { hapticManager.intensity },
                            set: { hapticManager.setIntensity($0) }
                        ),
                        in: 0.0...1.0,
                        feedbackThreshold: 0.1
                    )
                }
                .disabled(!hapticManager.isEnabled)
                
                if !hapticManager.isSupported {
                    Text("Haptic feedback is not supported on this device")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
                
                if let errorMessage = hapticManager.errorMessage {
                    Text(errorMessage)
                        .font(.caption)
                        .foregroundColor(.red)
                }
            }
            
            // Test buttons
            VStack(spacing: 8) {
                Text("Test Haptic Feedback")
                    .font(.subheadline)
                
                LazyVGrid(columns: [
                    GridItem(.flexible()),
                    GridItem(.flexible()),
                    GridItem(.flexible())
                ], spacing: 8) {
                    HapticButton(feedbackType: .buttonPress, action: {}) {
                        Text("Button")
                            .font(.caption)
                            .padding(.horizontal, 12)
                            .padding(.vertical, 6)
                            .background(Color.blue.opacity(0.1))
                            .cornerRadius(8)
                    }
                    
                    HapticButton(feedbackType: .playStart, action: {}) {
                        Text("Play")
                            .font(.caption)
                            .padding(.horizontal, 12)
                            .padding(.vertical, 6)
                            .background(Color.green.opacity(0.1))
                            .cornerRadius(8)
                    }
                    
                    HapticButton(feedbackType: .noteOn, action: {}) {
                        Text("Note")
                            .font(.caption)
                            .padding(.horizontal, 12)
                            .padding(.vertical, 6)
                            .background(Color.orange.opacity(0.1))
                            .cornerRadius(8)
                    }
                    
                    HapticButton(feedbackType: .success, action: {}) {
                        Text("Success")
                            .font(.caption)
                            .padding(.horizontal, 12)
                            .padding(.vertical, 6)
                            .background(Color.green.opacity(0.1))
                            .cornerRadius(8)
                    }
                    
                    HapticButton(feedbackType: .warning, action: {}) {
                        Text("Warning")
                            .font(.caption)
                            .padding(.horizontal, 12)
                            .padding(.vertical, 6)
                            .background(Color.yellow.opacity(0.1))
                            .cornerRadius(8)
                    }
                    
                    HapticButton(feedbackType: .error, action: {}) {
                        Text("Error")
                            .font(.caption)
                            .padding(.horizontal, 12)
                            .padding(.vertical, 6)
                            .background(Color.red.opacity(0.1))
                            .cornerRadius(8)
                    }
                }
            }
            .disabled(!hapticManager.isEnabled)
        }
        .padding()
    }
}