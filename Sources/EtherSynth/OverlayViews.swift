import SwiftUI

// MARK: - Sound Overlay View
struct SoundOverlayView: View {
    let onClose: () -> Void
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 12) {
            // Header
            HStack {
                Text("Select Instrument I1–I16")
                    .font(.system(size: 12, weight: .semibold))
                    .foregroundColor(.primary)
                
                Spacer()
                
                Button("Close") {
                    onClose()
                }
                .font(.system(size: 10))
                .foregroundColor(.blue)
            }
            
            // Instrument grid (4×4)
            LazyVGrid(columns: Array(repeating: GridItem(.flexible(), spacing: 8), count: 4), spacing: 8) {
                ForEach(0..<16, id: \.self) { instrumentIndex in
                    SoundInstrumentButton(instrumentIndex: instrumentIndex, onClose: onClose)
                }
            }
            
            // Engine selection for armed instrument (if unassigned)
            if synthController.isInstrumentUnassigned() {
                Divider()
                
                HStack {
                    Text("Engine for I\(synthController.uiArmedInstrumentIndex + 1):")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.secondary)
                    
                    Spacer()
                    
                    Button(synthController.getSelectedEngine() ?? "Choose...") {
                        synthController.showEngineSelector()
                    }
                    .font(.system(size: 10))
                    .foregroundColor(.blue)
                    .overlay(
                        Rectangle()
                            .frame(height: 1)
                            .offset(y: 5),
                        alignment: .bottom
                    )
                }
            }
        }
        .padding(16)
        .frame(width: 320, height: 200)
        .background(
            RoundedRectangle(cornerRadius: 12)
                .fill(Color.white)
                .shadow(color: .black.opacity(0.2), radius: 8, x: 0, y: 4)
        )
    }
}

// MARK: - Sound Instrument Button
struct SoundInstrumentButton: View {
    let instrumentIndex: Int
    let onClose: () -> Void
    @EnvironmentObject var synthController: SynthController
    
    var isArmed: Bool {
        synthController.uiArmedInstrumentIndex == instrumentIndex
    }
    
    var instrumentName: String {
        synthController.getInstrumentName(instrumentIndex)
    }
    
    var hasActivity: Bool {
        synthController.getInstrumentActivityLevel(instrumentIndex) > 0
    }
    
    var body: some View {
        Button(action: {
            synthController.uiArmInstrument(instrumentIndex)
            onClose()
        }) {
            VStack(spacing: 4) {
                // Instrument indicator
                Circle()
                    .fill(synthController.getInstrumentColor(instrumentIndex))
                    .frame(width: 24, height: 24)
                    .overlay(
                        Circle()
                            .stroke(isArmed ? Color.primary : Color.clear, lineWidth: 2)
                    )
                
                // Instrument label
                Text("I\(instrumentIndex + 1)")
                    .font(.system(size: 8, weight: .bold, design: .monospaced))
                    .foregroundColor(isArmed ? .primary : .secondary)
                
                // Instrument type/name
                Text(instrumentName == "Empty slot" ? "Empty" : instrumentName)
                    .font(.system(size: 7, weight: .regular))
                    .foregroundColor(.secondary)
                    .lineLimit(1)
                    .minimumScaleFactor(0.8)
                
                // Activity indicator
                if hasActivity {
                    Text("●")
                        .font(.system(size: 6))
                        .foregroundColor(synthController.getInstrumentColor(instrumentIndex))
                } else {
                    Text("")
                        .font(.system(size: 6))
                        .frame(height: 8)
                }
            }
            .frame(width: 60, height: 60)
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .fill(isArmed ? synthController.getInstrumentColor(instrumentIndex).opacity(0.2) : Color.gray.opacity(0.1))
                    .overlay(
                        RoundedRectangle(cornerRadius: 8)
                            .stroke(isArmed ? synthController.getInstrumentColor(instrumentIndex) : Color.gray.opacity(0.3), lineWidth: 1)
                    )
            )
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - Pattern Overlay View
struct PatternOverlayView: View {
    let onClose: () -> Void
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 12) {
            // Header
            HStack {
                Text("Select Pattern A–P")
                    .font(.system(size: 12, weight: .semibold))
                    .foregroundColor(.primary)
                
                Spacer()
                
                Button("Close") {
                    onClose()
                }
                .font(.system(size: 10))
                .foregroundColor(.blue)
            }
            
            // Pattern grid (4×4)
            LazyVGrid(columns: Array(repeating: GridItem(.flexible(), spacing: 8), count: 4), spacing: 8) {
                ForEach(Array("ABCDEFGHIJKLMNOP".enumerated()), id: \.offset) { index, patternChar in
                    PatternButton(patternIndex: index, patternName: String(patternChar), onClose: onClose)
                }
            }
            
            // Pattern length control
            Divider()
            
            HStack {
                Text("Length:")
                    .font(.system(size: 10, weight: .medium))
                    .foregroundColor(.secondary)
                
                ForEach([16, 32, 48, 64], id: \.self) { length in
                    Button("\(length)") {
                        synthController.setPatternLength(length)
                    }
                    .font(.system(size: 9, weight: synthController.getPatternLength() == length ? .bold : .regular))
                    .foregroundColor(synthController.getPatternLength() == length ? .blue : .secondary)
                }
                
                Spacer()
                
                Text("Pages: \(synthController.getTotalPatternPages())")
                    .font(.system(size: 9, weight: .regular))
                    .foregroundColor(.secondary)
            }
        }
        .padding(16)
        .frame(width: 320, height: 240)
        .background(
            RoundedRectangle(cornerRadius: 12)
                .fill(Color.white)
                .shadow(color: .black.opacity(0.2), radius: 8, x: 0, y: 4)
        )
    }
}

// MARK: - Pattern Button
struct PatternButton: View {
    let patternIndex: Int
    let patternName: String
    let onClose: () -> Void
    @EnvironmentObject var synthController: SynthController
    
    var isCurrent: Bool {
        synthController.currentPatternID == patternName
    }
    
    var patternLength: Int {
        synthController.getPatternLength(patternIndex)
    }
    
    var hasContent: Bool {
        synthController.getPatternContentLevel(patternIndex) > 0
    }
    
    var body: some View {
        Button(action: {
            synthController.switchPattern(patternName)
            onClose()
        }) {
            VStack(spacing: 4) {
                // Pattern name
                Text(patternName)
                    .font(.system(size: 14, weight: .bold, design: .monospaced))
                    .foregroundColor(isCurrent ? .white : .primary)
                
                // Pattern info
                Text("\(patternLength)st")
                    .font(.system(size: 7, weight: .regular, design: .monospaced))
                    .foregroundColor(isCurrent ? .white.opacity(0.8) : .secondary)
                
                // Content level indicator
                if hasContent {
                    Text("●")
                        .font(.system(size: 8))
                        .foregroundColor(isCurrent ? .white.opacity(0.8) : .blue)
                } else {
                    Text("○")
                        .font(.system(size: 8))
                        .foregroundColor(isCurrent ? .white.opacity(0.8) : .secondary)
                }
            }
            .frame(width: 50, height: 50)
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .fill(isCurrent ? Color.blue : Color.gray.opacity(0.1))
                    .overlay(
                        RoundedRectangle(cornerRadius: 8)
                            .stroke(isCurrent ? Color.blue : Color.gray.opacity(0.3), lineWidth: 1)
                    )
            )
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - FX Overlay View
struct FXOverlayView: View {
    let onClose: () -> Void
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 12) {
            // Header
            HStack {
                Text("FX / Note Repeat")
                    .font(.system(size: 12, weight: .semibold))
                    .foregroundColor(.primary)
                
                Spacer()
                
                Button("Close") {
                    onClose()
                }
                .font(.system(size: 10))
                .foregroundColor(.blue)
            }
            
            // Effects grid
            VStack(spacing: 8) {
                Text("Effects")
                    .font(.system(size: 10, weight: .semibold))
                    .foregroundColor(.primary)
                
                LazyVGrid(columns: Array(repeating: GridItem(.flexible(), spacing: 8), count: 3), spacing: 8) {
                    ForEach(["Stutter", "Filter", "Tape", "Delay", "Drop"], id: \.self) { effectName in
                        FXButton(effectName: effectName)
                    }
                }
            }
            
            // Note Repeat section
            VStack(spacing: 8) {
                Text("Note Repeat")
                    .font(.system(size: 10, weight: .semibold))
                    .foregroundColor(.primary)
                
                // Repeat rates
                HStack(spacing: 4) {
                    ForEach(["1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/64"], id: \.self) { rate in
                        Button(rate) {
                            synthController.setNoteRepeatRate(rate)
                        }
                        .font(.system(size: 8, weight: synthController.noteRepeatRate == rate ? .bold : .regular))
                        .foregroundColor(synthController.noteRepeatRate == rate ? .white : .primary)
                        .padding(.horizontal, 4)
                        .padding(.vertical, 2)
                        .background(
                            RoundedRectangle(cornerRadius: 3)
                                .fill(synthController.noteRepeatRate == rate ? Color.blue : Color.gray.opacity(0.2))
                        )
                    }
                }
                
                // Ramp and Spread controls
                HStack {
                    Text("Ramp:")
                        .font(.system(size: 9, weight: .medium))
                        .foregroundColor(.secondary)
                    
                    ForEach(["Flat", "Up", "Down"], id: \.self) { ramp in
                        Button(ramp) {
                            synthController.setNoteRepeatRamp(ramp)
                        }
                        .font(.system(size: 8, weight: synthController.noteRepeatRamp == ramp ? .bold : .regular))
                        .foregroundColor(synthController.noteRepeatRamp == ramp ? .blue : .secondary)
                    }
                    
                    Spacer()
                    
                    Text("Spread: \(synthController.noteRepeatSpread)st")
                        .font(.system(size: 9, weight: .medium))
                        .foregroundColor(.secondary)
                }
            }
            
            // FX Controls
            HStack {
                Button(synthController.isFXLatched ? "Latched" : "Latch") {
                    synthController.toggleFXLatch()
                }
                .font(.system(size: 9, weight: .medium))
                .foregroundColor(synthController.isFXLatched ? .white : .blue)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(
                    RoundedRectangle(cornerRadius: 4)
                        .fill(synthController.isFXLatched ? Color.blue : Color.blue.opacity(0.1))
                )
                
                Spacer()
                
                Button("Bake...") {
                    synthController.bakeFX()
                }
                .font(.system(size: 9, weight: .medium))
                .foregroundColor(.blue)
            }
        }
        .padding(16)
        .frame(width: 360, height: 280)
        .background(
            RoundedRectangle(cornerRadius: 12)
                .fill(Color.white)
                .shadow(color: .black.opacity(0.2), radius: 8, x: 0, y: 4)
        )
    }
}

// MARK: - FX Button
struct FXButton: View {
    let effectName: String
    @EnvironmentObject var synthController: SynthController
    
    var isActive: Bool {
        synthController.activeFX == effectName
    }
    
    var body: some View {
        Button(action: {
            synthController.setActiveFX(effectName)
        }) {
            VStack(spacing: 4) {
                // Effect icon (placeholder)
                Text(getEffectIcon(effectName))
                    .font(.system(size: 16))
                    .foregroundColor(isActive ? .white : .primary)
                
                // Effect name
                Text(effectName)
                    .font(.system(size: 8, weight: .medium))
                    .foregroundColor(isActive ? .white : .primary)
            }
            .frame(width: 60, height: 40)
            .background(
                RoundedRectangle(cornerRadius: 6)
                    .fill(isActive ? Color.blue : Color.gray.opacity(0.1))
                    .overlay(
                        RoundedRectangle(cornerRadius: 6)
                            .stroke(isActive ? Color.blue : Color.gray.opacity(0.3), lineWidth: 1)
                    )
            )
        }
        .buttonStyle(PlainButtonStyle())
    }
    
    private func getEffectIcon(_ name: String) -> String {
        switch name {
        case "Stutter": return "⚡"
        case "Filter": return "🎛️"
        case "Tape": return "⏺"
        case "Delay": return "🔄"
        case "Drop": return "📉"
        default: return "🎵"
        }
    }
}