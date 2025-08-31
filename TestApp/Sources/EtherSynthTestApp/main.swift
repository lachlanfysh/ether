import SwiftUI

// Simplified test app for EtherSynth UI testing
@main
struct EtherSynthTestApp: App {
    @StateObject private var appState = TestAppState()
    
    var body: some Scene {
        WindowGroup {
            TestMainView()
                .environmentObject(appState)
        }
    }
}

// Simplified state for testing
class TestAppState: ObservableObject {
    @Published var chordSystemEnabled: Bool = false
    @Published var currentChordType: Int = 0
    @Published var showingOverlay: Bool = false
    @Published var overlayType: TestOverlayType?
    
    func openChordOverlay() {
        overlayType = .chord
        showingOverlay = true
    }
    
    func closeOverlay() {
        showingOverlay = false
        overlayType = nil
    }
}

enum TestOverlayType {
    case chord
    case fx
    case pattern
}

// Main test view mimicking the 960x320 EtherSynth layout
struct TestMainView: View {
    @EnvironmentObject var appState: TestAppState
    
    var body: some View {
        ZStack {
            VStack(spacing: 0) {
                // Header (32px)
                TestHeaderView()
                    .frame(height: 32)
                    .background(Color.gray.opacity(0.1))
                
                // Option Line (48px) - 16 buttons in quads
                TestOptionLineView()
                    .frame(height: 48)
                    .background(Color.gray.opacity(0.2))
                
                // Main View Area (156px)
                TestViewArea()
                    .frame(height: 156)
                    .background(Color.black.opacity(0.05))
                
                // Step Row (56px) - 16 step buttons
                TestStepRowView()
                    .frame(height: 56)
                    .background(Color.gray.opacity(0.2))
                
                // Footer (28px)
                TestFooterView()
                    .frame(height: 28)
                    .background(Color.gray.opacity(0.1))
            }
            
            // Modal Overlay
            if appState.showingOverlay {
                Color.black.opacity(0.3)
                    .ignoresSafeArea()
                    .onTapGesture {
                        appState.closeOverlay()
                    }
                
                if let overlayType = appState.overlayType {
                    switch overlayType {
                    case .chord:
                        TestChordOverlay()
                    case .fx:
                        Text("FX Overlay")
                            .padding()
                            .background(Color.white)
                            .cornerRadius(8)
                    case .pattern:
                        Text("Pattern Overlay")
                            .padding()
                            .background(Color.white)
                            .cornerRadius(8)
                    }
                }
            }
        }
        .frame(width: 960, height: 320)
    }
}

// Test Header
struct TestHeaderView: View {
    var body: some View {
        HStack {
            Text("ETHER")
                .font(.system(size: 14, weight: .bold))
                .foregroundColor(.primary)
            
            Spacer()
            
            // Transport controls
            HStack(spacing: 8) {
                Button("⏸") { /* play/pause */ }
                Button("⏹") { /* stop */ }
                Button("⏺") { /* record */ }
            }
            .font(.system(size: 12))
            
            Spacer()
            
            Text("120 BPM")
                .font(.system(size: 12))
                .foregroundColor(.secondary)
        }
        .padding(.horizontal, 16)
    }
}

// Test Option Line with CHORD button
struct TestOptionLineView: View {
    @EnvironmentObject var appState: TestAppState
    
    private let optionLabels = [
        "INST", "SOUND", "PATT", "CLONE",  // Quad A
        "COPY", "CUT", "DEL", "NUDGE",     // Quad B  
        "ACC", "RATCH", "TIE", "MICRO",   // Quad C
        "FX", "CHORD", "SWG", "SCENE"      // Quad D - CHORD button is here!
    ]
    
    var body: some View {
        HStack(spacing: 2) {
            ForEach(0..<16, id: \.self) { index in
                Button(action: {
                    handleButtonPress(index)
                }) {
                    Text(optionLabels[index])
                        .font(.system(size: 9, weight: .medium))
                        .foregroundColor(.black)
                        .frame(maxWidth: .infinity, maxHeight: .infinity)
                }
                .frame(width: 56, height: 36)
                .background(getButtonColor(index))
                .cornerRadius(6)
            }
        }
        .padding(.horizontal, 8)
    }
    
    private func handleButtonPress(_ index: Int) {
        switch index {
        case 12: // FX
            appState.overlayType = .fx
            appState.showingOverlay = true
        case 13: // CHORD - This is our new chord button!
            appState.openChordOverlay()
            print("🎵 CHORD button pressed - opening chord overlay!")
        case 15: // SCENE
            appState.overlayType = .pattern
            appState.showingOverlay = true
        default:
            print("Button \(index) (\(optionLabels[index])) pressed")
        }
    }
    
    private func getButtonColor(_ index: Int) -> Color {
        let quadColors = [
            Color(red: 0.94, green: 0.89, blue: 0.95), // Lilac
            Color(red: 0.89, green: 0.93, blue: 0.98), // Blue
            Color(red: 0.99, green: 0.93, blue: 0.89), // Peach
            Color(red: 0.89, green: 0.96, blue: 0.93)  // Mint
        ]
        return quadColors[index / 4].opacity(0.8)
    }
}

// Test View Area
struct TestViewArea: View {
    var body: some View {
        ZStack {
            RoundedRectangle(cornerRadius: 8)
                .foregroundColor(Color.blue.opacity(0.1))
                .overlay(
                    RoundedRectangle(cornerRadius: 8)
                        .stroke(Color.blue.opacity(0.3), lineWidth: 1)
                )
            
            VStack {
                Text("🎛️ MAIN VIEW AREA")
                    .font(.title2)
                    .foregroundColor(.primary)
                
                Text("960×156 pixels")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Text("All overlays, parameter controls, and")
                Text("visual feedback appear here")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
        .padding(8)
    }
}

// Test Step Row
struct TestStepRowView: View {
    var body: some View {
        HStack(spacing: 2) {
            ForEach(0..<16, id: \.self) { step in
                Button(action: {
                    print("Step \(step + 1) pressed")
                }) {
                    VStack(spacing: 1) {
                        Text("\(step + 1)")
                            .font(.system(size: 10, weight: .bold))
                            .foregroundColor(.black)
                        
                        Circle()
                            .fill(step % 4 == 0 ? Color.red : Color.gray)
                            .frame(width: 6, height: 6)
                    }
                }
                .frame(width: 56, height: 48)
                .background(step % 4 == 0 ? Color.orange.opacity(0.3) : Color.gray.opacity(0.2))
                .cornerRadius(4)
            }
        }
        .padding(.horizontal, 8)
    }
}

// Test Footer
struct TestFooterView: View {
    var body: some View {
        HStack {
            Text("CPU: ▓▓░░")
                .font(.system(size: 10))
                .foregroundColor(.secondary)
            
            Spacer()
            
            Text("Pattern A1 - Bar 05:03")
                .font(.system(size: 10))
                .foregroundColor(.secondary)
            
            Spacer()
            
            Text("Project: Test UI")
                .font(.system(size: 10))
                .foregroundColor(.secondary)
        }
        .padding(.horizontal, 16)
    }
}

// Test Chord Overlay (simplified version of our chord system)
struct TestChordOverlay: View {
    @EnvironmentObject var appState: TestAppState
    
    var body: some View {
        VStack(spacing: 16) {
            // Header
            HStack {
                Text("🎵 CHORD SYSTEM (BICEP MODE)")
                    .font(.title2)
                    .fontWeight(.bold)
                Spacer()
                Button("×") {
                    appState.closeOverlay()
                }
                .font(.title)
            }
            
            // Enable/Disable
            HStack {
                Text("Chord System:")
                Button(appState.chordSystemEnabled ? "ENABLED" : "DISABLED") {
                    appState.chordSystemEnabled.toggle()
                    print("Chord system: \(appState.chordSystemEnabled ? "ON" : "OFF")")
                }
                .foregroundColor(appState.chordSystemEnabled ? .green : .gray)
                .padding(.horizontal, 12)
                .padding(.vertical, 4)
                .background(appState.chordSystemEnabled ? Color.green.opacity(0.2) : Color.gray.opacity(0.2))
                .cornerRadius(6)
            }
            
            if appState.chordSystemEnabled {
                VStack(alignment: .leading, spacing: 12) {
                    // Chord Selection
                    VStack(alignment: .leading) {
                        Text("CHORD SELECTION")
                            .font(.headline)
                        HStack {
                            Text("Type:")
                            Menu {
                                Button("Major") { appState.currentChordType = 0 }
                                Button("Minor") { appState.currentChordType = 1 }
                                Button("Major 7") { appState.currentChordType = 2 }
                                Button("Minor 7") { appState.currentChordType = 3 }
                            } label: {
                                Text(chordTypeName(appState.currentChordType))
                                    .padding(.horizontal, 8)
                                    .padding(.vertical, 4)
                                    .background(Color.blue.opacity(0.2))
                                    .cornerRadius(4)
                            }
                            
                            Spacer()
                            
                            Button("PLAY CHORD") {
                                print("🎵 Playing chord: \(chordTypeName(appState.currentChordType))")
                            }
                            .foregroundColor(.white)
                            .padding(.horizontal, 12)
                            .padding(.vertical, 6)
                            .background(Color.green)
                            .cornerRadius(6)
                        }
                    }
                    
                    // Voice Configuration Preview
                    VStack(alignment: .leading) {
                        Text("VOICE CONFIGURATION (5 VOICES)")
                            .font(.headline)
                        
                        ForEach(0..<3, id: \.self) { voice in
                            HStack {
                                Circle()
                                    .fill(Color.green)
                                    .frame(width: 8, height: 8)
                                Text("Voice \(voice + 1): MacroVA")
                                Spacer()
                                Text("80%")
                                    .font(.caption)
                                    .foregroundColor(.secondary)
                            }
                        }
                        
                        Text("+ 2 more voices...")
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                    
                    // Stats
                    HStack {
                        Text("CPU: 15%")
                        Spacer()
                        Text("Voices: 3")
                    }
                    .font(.caption)
                    .foregroundColor(.secondary)
                }
            }
            
            Text("This is a test overlay demonstrating the chord system UI!")
                .font(.caption)
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)
        }
        .padding(20)
        .frame(width: 400, height: 500)
        .background(Color.white)
        .cornerRadius(12)
        .shadow(radius: 10)
    }
    
    private func chordTypeName(_ type: Int) -> String {
        let names = ["Major", "Minor", "Major 7", "Minor 7"]
        return names[min(type, names.count - 1)]
    }
}