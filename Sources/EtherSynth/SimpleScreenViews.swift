import SwiftUI

// MARK: - Simple Screen Views for Demo

struct InstrumentScreenView: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack {
            Text("INSTRUMENT I\(synthController.uiArmedInstrumentIndex + 1)")
                .font(.system(size: 16, weight: .bold))
            Text("Parameter editing goes here")
                .foregroundColor(.secondary)
            Spacer()
        }
        .padding()
    }
}

struct SongScreenView: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack {
            Text("SONG ARRANGEMENT")
                .font(.system(size: 16, weight: .bold))
            Text("Pattern: \(synthController.currentPatternID)")
            Text("Chain editing goes here")
                .foregroundColor(.secondary)
            Spacer()
        }
        .padding()
    }
}

struct MixScreenView: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack {
            Text("16-CHANNEL MIXER")
                .font(.system(size: 16, weight: .bold))
            Text("BPM: \(Int(synthController.uiBPM))")
            Text("CPU: \(synthController.uiCpuUsage)%")
            Text("Voices: \(synthController.uiActiveVoices)")
            Text("Mixer controls go here")
                .foregroundColor(.secondary)
            Spacer()
        }
        .padding()
    }
}

// MARK: - Simple Overlay Views

struct SoundOverlayView: View {
    let onClose: () -> Void
    
    var body: some View {
        VStack {
            Text("SOUND SELECTION")
                .font(.system(size: 14, weight: .semibold))
            Text("Choose instrument engine")
                .foregroundColor(.secondary)
            Button("Close", action: onClose)
        }
        .frame(width: 300, height: 200)
        .background(Color.white)
        .cornerRadius(12)
    }
}

struct PatternOverlayView: View {
    let onClose: () -> Void
    
    var body: some View {
        VStack {
            Text("PATTERN TOOLS")
                .font(.system(size: 14, weight: .semibold))
            Text("Pattern operations")
                .foregroundColor(.secondary)
            Button("Close", action: onClose)
        }
        .frame(width: 300, height: 200)
        .background(Color.white)
        .cornerRadius(12)
    }
}

struct FXOverlayView: View {
    let onClose: () -> Void
    
    var body: some View {
        VStack {
            Text("FX CONTROLS")
                .font(.system(size: 14, weight: .semibold))
            Text("Effects processing")
                .foregroundColor(.secondary)
            Button("Close", action: onClose)
        }
        .frame(width: 300, height: 200)
        .background(Color.white)
        .cornerRadius(12)
    }
}