import SwiftUI

// MARK: - Instrument Screen View
struct InstrumentScreenView: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 16) {
            // Header
            HStack {
                Text("INSTRUMENT I\(synthController.uiArmedInstrumentIndex + 1) — \(synthController.getInstrumentTypeName())")
                    .font(.system(size: 12, weight: .semibold, design: .monospaced))
                    .foregroundColor(.primary)
                
                Spacer()
                
                Text("Page \(synthController.currentParameterPage + 1)/\(synthController.getTotalParameterPages())")
                    .font(.system(size: 9, weight: .regular, design: .monospaced))
                    .foregroundColor(.secondary)
            }
            .padding(.horizontal, 16)
            .padding(.top, 12)
            
            // Engine and preset selection (always show)
            // if synthController.isInstrumentUnassigned() {
                HStack {
                    Text("Engine:")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.secondary)
                    
                    Button(action: {
                        synthController.showEngineSelector()
                    }) {
                        Text(synthController.getSelectedEngine() ?? "Choose...")
                            .font(.system(size: 10, weight: .medium))
                            .foregroundColor(.blue)
                            .underline()
                    }
                    
                    Spacer()
                    
                    Text("Preset:")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.secondary)
                    
                    Button(action: {
                        synthController.showPresetSelector()
                    }) {
                        Text(synthController.getSelectedPreset() ?? "Choose...")
                            .font(.system(size: 10, weight: .medium))
                            .foregroundColor(.blue)
                            .underline()
                    }
                }
                .padding(.horizontal, 16)
            // }
            
            // 1×16 Parameter Grid
            ParameterGridView()
            
            Spacer()
        }
    }
}

// MARK: - Parameter Grid View
struct ParameterGridView: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 8) {
            // Parameter grid - 1×16 layout
            HStack(spacing: 4) {
                ForEach(0..<16, id: \.self) { paramIndex in
                    ParameterCellView(paramIndex: paramIndex)
                        .onTapGesture {
                            synthController.selectParameter(paramIndex)
                        }
                }
            }
            .padding(.horizontal, 16)
            
            // Page indicators (if multiple pages)
            if synthController.getTotalParameterPages() > 1 {
                HStack(spacing: 4) {
                    ForEach(0..<synthController.getTotalParameterPages(), id: \.self) { pageIndex in
                        Circle()
                            .fill(pageIndex == synthController.currentParameterPage ? Color.blue : Color.gray.opacity(0.3))
                            .frame(width: 6, height: 6)
                    }
                }
                .padding(.top, 4)
            }
            
            // Instructions
            HStack {
                Text("Press bottom buttons 1-16 to select parameter • Turn SmartKnob to adjust")
                    .font(.system(size: 9, weight: .regular))
                    .foregroundColor(.secondary)
                
                Spacer()
                
                if synthController.getTotalParameterPages() > 1 {
                    Text("SHIFT + button for page \(synthController.currentParameterPage + 1 == synthController.getTotalParameterPages() ? 1 : synthController.currentParameterPage + 2)")
                        .font(.system(size: 9, weight: .regular))
                        .foregroundColor(.secondary)
                }
            }
            .padding(.horizontal, 16)
        }
        .onReceive(synthController.hardwareButtonPublisher) { buttonPress in
            if case .bottomButton(let buttonIndex) = buttonPress.buttonType, buttonPress.isPressed {
                synthController.handleBottomButtonPress(buttonIndex, isShiftHeld: synthController.uiIsShiftHeld)
            }
        }
    }
}

// MARK: - Parameter Cell View
struct ParameterCellView: View {
    let paramIndex: Int
    @EnvironmentObject var synthController: SynthController
    
    var parameterData: ParameterData {
        let absoluteIndex = (synthController.currentParameterPage * 16) + paramIndex
        return synthController.getParameterData(absoluteIndex)
    }
    
    var isSelected: Bool {
        let absoluteIndex = (synthController.currentParameterPage * 16) + paramIndex
        return synthController.selectedParameterIndex == absoluteIndex
    }
    
    var body: some View {
        VStack(spacing: 2) {
            // Parameter name (abbreviated)
            Text(parameterData.name)
                .font(.system(size: 8, weight: .medium, design: .monospaced))
                .foregroundColor(isSelected ? .white : .primary)
                .lineLimit(1)
                .minimumScaleFactor(0.7)
            
            // Parameter value
            Text(parameterData.displayValue)
                .font(.system(size: 10, weight: .bold, design: .monospaced))
                .foregroundColor(isSelected ? .white : .primary)
                .lineLimit(1)
            
            // Value bar indicator (only show for selected parameter)
            if isSelected {
                Rectangle()
                    .fill(Color.white.opacity(0.8))
                    .frame(width: max(2, CGFloat(parameterData.value) * 48), height: 2)
                    .animation(.easeInOut(duration: 0.1), value: parameterData.value)
            }
        }
        .frame(width: 54, height: 32)
        .background(
            RoundedRectangle(cornerRadius: 6)
                .fill(isSelected ? Color.blue : Color.gray.opacity(0.1))
                .overlay(
                    RoundedRectangle(cornerRadius: 6)
                        .stroke(isSelected ? Color.blue : Color.gray.opacity(0.3), lineWidth: 1)
                )
        )
        .animation(.easeInOut(duration: 0.15), value: isSelected)
    }
}

// MARK: - Song Screen View  
struct SongScreenView: View {
    @EnvironmentObject var synthController: SynthController
    @State private var selectedSectionIndex: Int? = nil
    
    var body: some View {
        VStack(spacing: 0) {
            // Song header
            SongHeaderView()
                .padding(.horizontal, 16)
                .padding(.top, 12)
            
            // Main arrangement area
            HStack(alignment: .top, spacing: 20) {
                // Song sections (left panel)  
                SongSectionsPanel(selectedIndex: $selectedSectionIndex)
                    .frame(width: 140)
                
                // Pattern chain timeline (main area)
                PatternChainTimelineView()
            }
            .padding(.horizontal, 16)
            .padding(.vertical, 16)
            
            Spacer()
            
            // Global song controls
            GlobalSongControlsView()
                .padding(.horizontal, 16)
                .padding(.bottom, 12)
        }
    }
}

// MARK: - Song Header View
struct SongHeaderView: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 2) {
                Text("SONG — \"\(synthController.songName)\" — \(synthController.getTotalSongBars()) bars")
                    .font(.system(size: 12, weight: .semibold, design: .monospaced))
                    .foregroundColor(.primary)
                
                Text("Current: Bar \(synthController.getCurrentSongBar()) • \(synthController.chainMode) chain mode")
                    .font(.system(size: 9, weight: .regular, design: .monospaced))
                    .foregroundColor(.secondary)
            }
            
            Spacer()
            
            // Quick play controls
            HStack(spacing: 8) {
                Button("◀◀") {
                    synthController.jumpToPreviousSection()
                }
                .font(.system(size: 12, weight: .bold))
                .foregroundColor(.blue)
                
                Button("▶▶") {
                    synthController.jumpToNextSection()
                }
                .font(.system(size: 12, weight: .bold))
                .foregroundColor(.blue)
            }
        }
    }
}

// MARK: - Song Sections Panel
struct SongSectionsPanel: View {
    @Binding var selectedIndex: Int?
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Sections")
                .font(.system(size: 10, weight: .semibold))
                .foregroundColor(.primary)
            
            VStack(spacing: 6) {
                ForEach(Array(synthController.getSongSections().enumerated()), id: \.offset) { index, section in
                    EnhancedSongSectionView(
                        section: section,
                        isSelected: selectedIndex == index,
                        onTap: { selectedIndex = selectedIndex == index ? nil : index }
                    )
                }
            }
            
            Spacer()
        }
    }
}

// MARK: - Enhanced Song Section View
struct EnhancedSongSectionView: View {
    let section: SongSection
    let isSelected: Bool
    let onTap: () -> Void
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        Button(action: onTap) {
            HStack {
                // Section color indicator
                RoundedRectangle(cornerRadius: 2)
                    .fill(section.color.color)
                    .frame(width: 4, height: 24)
                
                VStack(alignment: .leading, spacing: 2) {
                    Text(section.name)
                        .font(.system(size: 9, weight: .medium))
                        .foregroundColor(isSelected ? .white : .primary)
                    
                    Text("\(section.patternBlocks.first?.patternId ?? "?") • \(section.totalLength) bars")
                        .font(.system(size: 8, weight: .regular, design: .monospaced))
                        .foregroundColor(isSelected ? .white.opacity(0.8) : .secondary)
                }
                
                Spacer()
                
                // Edit button
                Button("⋯") {
                    synthController.editSongSection(Array(synthController.getSongSections()).firstIndex(where: { $0.name == section.name }) ?? 0)
                }
                .font(.system(size: 10, weight: .bold))
                .foregroundColor(isSelected ? .white.opacity(0.8) : .secondary)
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 6)
            .background(
                RoundedRectangle(cornerRadius: 6)
                    .fill(isSelected ? section.color.color.opacity(0.8) : Color.gray.opacity(0.1))
                    .overlay(
                        RoundedRectangle(cornerRadius: 6)
                            .stroke(section.color.color.opacity(0.3), lineWidth: 1)
                    )
            )
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - Pattern Chain Timeline View
struct PatternChainTimelineView: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Pattern Chain Timeline")
                .font(.system(size: 10, weight: .semibold))
                .foregroundColor(.primary)
            
            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: 3) {
                    ForEach(Array(synthController.getSongPatternSequence().enumerated()), id: \.offset) { index, block in
                        EnhancedPatternBlockView(block: block, blockIndex: index)
                    }
                }
                .padding(.horizontal, 8)
                .padding(.vertical, 8)
            }
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .fill(Color.gray.opacity(0.05))
                    .overlay(
                        RoundedRectangle(cornerRadius: 8)
                            .stroke(Color.gray.opacity(0.2), lineWidth: 1)
                    )
            )
            
            // Timeline controls
            HStack {
                Text("Drag blocks to rearrange • Click to select pattern • Double-click to edit")
                    .font(.system(size: 8, weight: .regular))
                    .foregroundColor(.secondary)
                
                Spacer()
                
                Button("Add Section") {
                    synthController.addSongSection()
                }
                .font(.system(size: 9))
                .foregroundColor(.blue)
            }
        }
    }
}

// MARK: - Enhanced Pattern Block View  
struct EnhancedPatternBlockView: View {
    let block: SongPatternBlock
    let blockIndex: Int
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 2) {
            // Pattern ID
            Text(block.patternID)
                .font(.system(size: 10, weight: .bold, design: .monospaced))
                .foregroundColor(block.isCurrentlyPlaying ? .white : .primary)
            
            // Bar number
            Text("\(block.barIndex + 1)")
                .font(.system(size: 7, weight: .medium, design: .monospaced))
                .foregroundColor(block.isCurrentlyPlaying ? .white.opacity(0.8) : .secondary)
            
            // Section indicator
            Text(String(block.sectionName.prefix(3)))
                .font(.system(size: 6, weight: .medium))
                .foregroundColor(block.isCurrentlyPlaying ? .white.opacity(0.7) : .secondary)
        }
        .frame(width: 32, height: 40)
        .background(
            RoundedRectangle(cornerRadius: 4)
                .fill(
                    block.isCurrentlyPlaying ? 
                    Color.blue : 
                    getSectionColor(for: block.sectionName).opacity(0.3)
                )
                .overlay(
                    RoundedRectangle(cornerRadius: 4)
                        .stroke(
                            block.isCurrentlyPlaying ? 
                            Color.blue : 
                            getSectionColor(for: block.sectionName), 
                            lineWidth: block.isCurrentlyPlaying ? 2 : 1
                        )
                )
        )
        .onTapGesture {
            synthController.switchPattern(block.patternID)
        }
    }
    
    private func getSectionColor(for sectionName: String) -> Color {
        switch sectionName {
        case "Intro", "Outro": return .gray
        case "Verse": return .blue
        case "Chorus": return .green
        case "Bridge": return .orange
        default: return .gray
        }
    }
}

// MARK: - Global Song Controls View
struct GlobalSongControlsView: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 12) {
            Divider()
            
            // Primary controls
            HStack {
                // BPM with tap tempo
                HStack(spacing: 8) {
                    Text("Master BPM:")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.secondary)
                    Text("\(Int(synthController.uiBPM))")
                        .font(.system(size: 10, weight: .bold))
                        .foregroundColor(.primary)
                    Button("Tap") {
                        synthController.tapTempo()
                    }
                    .font(.system(size: 9))
                    .foregroundColor(.blue)
                }
                
                Spacer()
                
                // Key and Scale
                HStack(spacing: 12) {
                    HStack(spacing: 4) {
                        Text("Key:")
                            .font(.system(size: 10, weight: .medium))
                            .foregroundColor(.secondary)
                        Text(synthController.songKey)
                            .font(.system(size: 10, weight: .bold))
                            .foregroundColor(.primary)
                    }
                    
                    HStack(spacing: 4) {
                        Text("Scale:")
                            .font(.system(size: 10, weight: .medium))
                            .foregroundColor(.secondary)
                        Text(synthController.songScale)
                            .font(.system(size: 10, weight: .bold))
                            .foregroundColor(.primary)
                    }
                }
            }
            
            // Secondary controls
            HStack {
                // Chain mode
                HStack(spacing: 8) {
                    Text("Chain Mode:")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.secondary)
                    
                    ForEach(["Auto", "Manual", "OneShot"], id: \.self) { mode in
                        Button(mode) {
                            synthController.setChainMode(mode)
                        }
                        .font(.system(size: 9, weight: synthController.chainMode == mode ? .bold : .regular))
                        .foregroundColor(synthController.chainMode == mode ? .white : .secondary)
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(
                            RoundedRectangle(cornerRadius: 4)
                                .fill(synthController.chainMode == mode ? Color.blue : Color.clear)
                        )
                    }
                }
                
                Spacer()
                
                // Global swing and quantize (SmartKnob adjustable)
                HStack(spacing: 12) {
                    HStack(spacing: 4) {
                        Text("Swing:")
                            .font(.system(size: 10, weight: .medium))
                            .foregroundColor(.secondary)
                        Text("\(Int(synthController.globalSwing * 100))%")
                            .font(.system(size: 10, weight: .bold))
                            .foregroundColor(.primary)
                            .padding(.horizontal, 6)
                            .padding(.vertical, 2)
                            .background(
                                RoundedRectangle(cornerRadius: 3)
                                    .fill(Color.blue.opacity(0.1))
                                    .overlay(
                                        RoundedRectangle(cornerRadius: 3)
                                            .stroke(Color.blue.opacity(0.3), lineWidth: 1)
                                    )
                            )
                            .gesture(
                                DragGesture()
                                    .onChanged { value in
                                        let delta = Float(-value.translation.height * 0.002)
                                        synthController.adjustGlobalSwing(delta)
                                    }
                            )
                    }
                    
                    HStack(spacing: 4) {
                        Text("Quantize:")
                            .font(.system(size: 10, weight: .medium))
                            .foregroundColor(.secondary)
                        Text("\(Int(synthController.globalQuantize * 100))%")
                            .font(.system(size: 10, weight: .bold))
                            .foregroundColor(.primary)
                            .padding(.horizontal, 6)
                            .padding(.vertical, 2)
                            .background(
                                RoundedRectangle(cornerRadius: 3)
                                    .fill(Color.green.opacity(0.1))
                                    .overlay(
                                        RoundedRectangle(cornerRadius: 3)
                                            .stroke(Color.green.opacity(0.3), lineWidth: 1)
                                    )
                            )
                            .gesture(
                                DragGesture()
                                    .onChanged { value in
                                        let delta = Float(-value.translation.height * 0.002)
                                        synthController.adjustGlobalQuantize(delta)
                                    }
                            )
                    }
                }
            }
        }
    }
}

// MARK: - Song Section View
struct SongSectionView: View {
    let name: String
    let pattern: String
    let bars: Int
    
    var body: some View {
        HStack {
            Text(name)
                .font(.system(size: 9, weight: .medium))
                .foregroundColor(.primary)
                .frame(width: 40, alignment: .leading)
            
            Text("\(pattern)\(bars)")
                .font(.system(size: 9, weight: .bold, design: .monospaced))
                .foregroundColor(.secondary)
        }
        .padding(.horizontal, 8)
        .padding(.vertical, 2)
        .background(
            RoundedRectangle(cornerRadius: 4)
                .fill(Color.gray.opacity(0.1))
        )
    }
}

// MARK: - Pattern Block View
struct PatternBlockView: View {
    let pattern: String
    
    var body: some View {
        Text(pattern)
            .font(.system(size: 10, weight: .bold, design: .monospaced))
            .foregroundColor(.white)
            .frame(width: 24, height: 24)
            .background(
                RoundedRectangle(cornerRadius: 4)
                    .fill(Color.blue)
            )
    }
}

// MARK: - Mix Screen View
struct MixScreenView: View {
    @EnvironmentObject var synthController: SynthController
    
    var selectedStripIndex: Int? {
        synthController.selectedMixerChannel
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Mixer header with global controls
            MixerHeaderView(selectedStrip: .constant(selectedStripIndex))
                .padding(.horizontal, 16)
                .padding(.top, 12)
            
            // Main mixer area
            HStack(spacing: 0) {
                // 16-channel mixer strips
                ScrollView(.horizontal, showsIndicators: false) {
                    HStack(spacing: 6) {
                        ForEach(0..<16, id: \.self) { instrumentIndex in
                            EnhancedMixerStripView(
                                instrumentIndex: instrumentIndex,
                                isSelected: selectedStripIndex == instrumentIndex,
                                onSelect: { synthController.selectMixerChannel(instrumentIndex) }
                            )
                        }
                    }
                    .padding(.horizontal, 16)
                    .padding(.vertical, 16)
                }
                .background(
                    RoundedRectangle(cornerRadius: 8)
                        .fill(Color.gray.opacity(0.05))
                        .overlay(
                            RoundedRectangle(cornerRadius: 8)
                                .stroke(Color.gray.opacity(0.2), lineWidth: 1)
                        )
                )
                .padding(.horizontal, 16)
            }
            
            Spacer()
            
            // Master section
            MasterSectionView()
                .padding(.horizontal, 16)
                .padding(.bottom, 12)
        }
    }
}

// MARK: - Mixer Header View
struct MixerHeaderView: View {
    @Binding var selectedStrip: Int?
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 2) {
                Text("MIXER — 16 Instruments")
                    .font(.system(size: 12, weight: .semibold, design: .monospaced))
                    .foregroundColor(.primary)
                
                if let selectedIndex = selectedStrip {
                    Text("Selected: I\(selectedIndex + 1) \(synthController.getInstrumentName(selectedIndex))")
                        .font(.system(size: 9, weight: .regular, design: .monospaced))
                        .foregroundColor(.secondary)
                } else {
                    Text("Select a channel strip to edit • SmartKnob adjusts volume")
                        .font(.system(size: 9, weight: .regular))
                        .foregroundColor(.secondary)
                }
            }
            
            Spacer()
            
            // Mixer quick actions
            HStack(spacing: 8) {
                Button("Solo Clear") {
                    synthController.clearAllSolo()
                }
                .font(.system(size: 9, weight: .medium))
                .foregroundColor(.blue)
                
                Button("Mute Clear") {
                    synthController.clearAllMute()
                }
                .font(.system(size: 9, weight: .medium))
                .foregroundColor(.blue)
            }
        }
    }
}

// MARK: - Enhanced Mixer Strip View
struct EnhancedMixerStripView: View {
    let instrumentIndex: Int
    let isSelected: Bool
    let onSelect: () -> Void
    @EnvironmentObject var synthController: SynthController
    @State private var isDraggingVolume = false
    
    var instrumentName: String {
        synthController.getInstrumentName(instrumentIndex)
    }
    
    var instrumentVolume: Float {
        synthController.getInstrumentVolume(instrumentIndex)
    }
    
    var isArmed: Bool {
        instrumentIndex == synthController.activeInstrument.index
    }
    
    private var instrumentHeader: some View {
        VStack(spacing: 2) {
            instrumentCircle
            instrumentNameText
        }
    }
    
    private var instrumentCircle: some View {
        ZStack {
            Circle()
                .fill(synthController.getInstrumentColor(instrumentIndex))
                .frame(width: 20, height: 20)
            
            Text("I\(instrumentIndex + 1)")
                .font(.system(size: 7, weight: .black, design: .monospaced))
                .foregroundColor(.white)
        }
        .overlay(
            Circle()
                .stroke(isArmed ? Color.primary : Color.clear, lineWidth: 2)
        )
    }
    
    private var instrumentNameText: some View {
        Text(instrumentName)
            .font(.system(size: 7, weight: .medium))
            .foregroundColor(isSelected ? .white : .primary)
            .lineLimit(1)
            .frame(width: 50)
            .minimumScaleFactor(0.7)
    }
    
    private var volumeSection: some View {
        VStack(spacing: 4) {
            volumeText
            volumeBar
        }
    }
    
    private var volumeText: some View {
        Text("\(Int(instrumentVolume * 100))")
            .font(.system(size: 8, weight: .bold, design: .monospaced))
            .foregroundColor(isSelected ? .white : .primary)
    }
    
    private var volumeBar: some View {
        VStack(spacing: 1) {
            ForEach(0..<8, id: \.self) { segment in
                Rectangle()
                    .fill(volumeBarColor(for: segment))
                    .frame(width: 6, height: 4)
            }
        }
        .background(
            RoundedRectangle(cornerRadius: 2)
                .fill(Color.black.opacity(0.1))
        )
    }
    
    private func volumeBarColor(for segment: Int) -> Color {
        let segmentLevel = Float(8 - segment) / 8.0
        return instrumentVolume >= segmentLevel ? 
               (segmentLevel > 0.8 ? Color.red : Color.green) : 
               Color.gray.opacity(0.3)
    }
    
    var body: some View {
        Button(action: onSelect) {
            VStack(spacing: 6) {
                instrumentHeader
                volumeSection
                    .gesture(
                        DragGesture()
                            .onChanged { value in
                                isDraggingVolume = true
                                let newLevel = max(0, min(1, Float(1.0 - (value.location.y / 36))))
                                synthController.setInstrumentVolume(instrumentIndex, volume: newLevel)
                            }
                            .onEnded { _ in
                                isDraggingVolume = false
                            }
                    )
                muteSoloButtons
                activityIndicator
            }
            .frame(width: 54, height: 140)
            .padding(.vertical, 8)
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .fill(isSelected ? Color.blue.opacity(0.15) : Color.gray.opacity(0.08))
                    .overlay(
                        RoundedRectangle(cornerRadius: 8)
                            .stroke(
                                isSelected ? Color.blue : 
                                (isArmed ? synthController.getInstrumentColor(instrumentIndex) : Color.gray.opacity(0.3)), 
                                lineWidth: isSelected ? 2 : (isArmed ? 2 : 1)
                            )
                    )
            )
        }
        .buttonStyle(PlainButtonStyle())
        .scaleEffect(isDraggingVolume ? 1.02 : 1.0)
        .animation(.easeInOut(duration: 0.15), value: isDraggingVolume)
    }
    
    private var muteSoloButtons: some View {
        HStack(spacing: 4) {
            muteButton
            soloButton
        }
    }
    
    private var muteButton: some View {
        Button("M") {
            synthController.toggleInstrumentMute(instrumentIndex)
        }
        .font(.system(size: 7, weight: .black))
        .foregroundColor(synthController.isInstrumentMuted(instrumentIndex) ? .white : .primary)
        .frame(width: 22, height: 18)
        .background(
            RoundedRectangle(cornerRadius: 3)
                .fill(synthController.isInstrumentMuted(instrumentIndex) ? Color.red : Color.gray.opacity(0.2))
        )
    }
    
    private var soloButton: some View {
        Button("S") {
            synthController.toggleInstrumentSolo(instrumentIndex)
        }
        .font(.system(size: 7, weight: .black))
        .foregroundColor(synthController.isInstrumentSoloed(instrumentIndex) ? .black : .primary)
        .frame(width: 22, height: 18)
        .background(
            RoundedRectangle(cornerRadius: 3)
                .fill(synthController.isInstrumentSoloed(instrumentIndex) ? Color.yellow : Color.gray.opacity(0.2))
        )
    }
    
    private var activityIndicator: some View {
        Rectangle()
            .fill(synthController.getInstrumentActivityLevel(instrumentIndex) > 0.1 ? 
                 synthController.getInstrumentColor(instrumentIndex) : 
                 Color.gray.opacity(0.3))
            .frame(width: 46, height: 3)
            .cornerRadius(1.5)
    }
}

// MARK: - Master Section View
struct MasterSectionView: View {
    @EnvironmentObject var synthController: SynthController
    @State private var isDraggingMaster = false
    
    var body: some View {
        VStack(spacing: 12) {
            Divider()
            
            HStack {
                // Master volume section
                VStack(alignment: .leading, spacing: 8) {
                    Text("Master")
                        .font(.system(size: 10, weight: .semibold))
                        .foregroundColor(.primary)
                    
                    HStack(spacing: 12) {
                        // Master volume control
                        VStack(spacing: 4) {
                            Text("Volume")
                                .font(.system(size: 8, weight: .medium))
                                .foregroundColor(.secondary)
                            
                            Text("\(Int(synthController.uiMasterVolume * 100))%")
                                .font(.system(size: 10, weight: .bold, design: .monospaced))
                                .foregroundColor(.primary)
                                .padding(.horizontal, 8)
                                .padding(.vertical, 4)
                                .background(
                                    RoundedRectangle(cornerRadius: 4)
                                        .fill(Color.blue.opacity(0.1))
                                        .overlay(
                                            RoundedRectangle(cornerRadius: 4)
                                                .stroke(Color.blue.opacity(0.3), lineWidth: 1)
                                        )
                                )
                                .gesture(
                                    DragGesture()
                                        .onChanged { value in
                                            isDraggingMaster = true
                                            let delta = Float(-value.translation.height * 0.003)
                                            synthController.adjustMasterVolume(delta)
                                        }
                                        .onEnded { _ in
                                            isDraggingMaster = false
                                        }
                                )
                        }
                        
                        // Master VU meter
                        VStack(spacing: 2) {
                            Text("Level")
                                .font(.system(size: 8, weight: .medium))
                                .foregroundColor(.secondary)
                            
                            HStack(spacing: 2) {
                                ForEach(0..<10, id: \.self) { segment in
                                    let level = Float(segment) / 10.0
                                    Rectangle()
                                        .fill(synthController.getMasterLevel() >= level ? 
                                             (level > 0.8 ? Color.red : (level > 0.6 ? Color.yellow : Color.green)) : 
                                             Color.gray.opacity(0.3))
                                        .frame(width: 4, height: 20)
                                }
                            }
                            .background(
                                RoundedRectangle(cornerRadius: 2)
                                    .fill(Color.black.opacity(0.1))
                            )
                        }
                    }
                }
                
                Spacer()
                
                // Global mixer controls
                VStack(alignment: .trailing, spacing: 8) {
                    Text("Global")
                        .font(.system(size: 10, weight: .semibold))
                        .foregroundColor(.primary)
                    
                    HStack(spacing: 16) {
                        // Send/Return level
                        VStack(spacing: 2) {
                            Text("Send")
                                .font(.system(size: 8, weight: .medium))
                                .foregroundColor(.secondary)
                            Text("42%")
                                .font(.system(size: 9, weight: .bold, design: .monospaced))
                                .foregroundColor(.primary)
                        }
                        
                        // Compressor
                        VStack(spacing: 2) {
                            Text("Comp")
                                .font(.system(size: 8, weight: .medium))
                                .foregroundColor(.secondary)
                            Text("ON")
                                .font(.system(size: 9, weight: .bold))
                                .foregroundColor(.green)
                        }
                        
                        // EQ
                        VStack(spacing: 2) {
                            Text("EQ")
                                .font(.system(size: 8, weight: .medium))
                                .foregroundColor(.secondary)
                            Text("ON")
                                .font(.system(size: 9, weight: .bold))
                                .foregroundColor(.blue)
                        }
                    }
                }
            }
        }
        .scaleEffect(isDraggingMaster ? 1.01 : 1.0)
        .animation(.easeInOut(duration: 0.15), value: isDraggingMaster)
    }
}

// MARK: - Mixer Strip View
struct MixerStripView: View {
    let instrumentIndex: Int
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 4) {
            // Instrument name
            Text("I\(instrumentIndex + 1)")
                .font(.system(size: 8, weight: .bold, design: .monospaced))
                .foregroundColor(.primary)
            
            Text(synthController.getInstrumentName(instrumentIndex))
                .font(.system(size: 7, weight: .regular))
                .foregroundColor(.secondary)
                .lineLimit(1)
                .frame(width: 40)
            
            // Volume level
            Text("\(Int(synthController.getInstrumentVolume(instrumentIndex) * 100))")
                .font(.system(size: 8, weight: .bold, design: .monospaced))
                .foregroundColor(.primary)
            
            // Mute/Solo buttons
            VStack(spacing: 2) {
                Button("M") {
                    synthController.toggleInstrumentMute(instrumentIndex)
                }
                .font(.system(size: 7, weight: .bold))
                .foregroundColor(synthController.isInstrumentMuted(instrumentIndex) ? .white : .primary)
                .frame(width: 20, height: 16)
                .background(
                    RoundedRectangle(cornerRadius: 3)
                        .fill(synthController.isInstrumentMuted(instrumentIndex) ? Color.red : Color.gray.opacity(0.2))
                )
                
                Button("S") {
                    synthController.toggleInstrumentSolo(instrumentIndex)
                }
                .font(.system(size: 7, weight: .bold))
                .foregroundColor(synthController.isInstrumentSoloed(instrumentIndex) ? .white : .primary)
                .frame(width: 20, height: 16)
                .background(
                    RoundedRectangle(cornerRadius: 3)
                        .fill(synthController.isInstrumentSoloed(instrumentIndex) ? Color.yellow : Color.gray.opacity(0.2))
                )
            }
        }
        .padding(.vertical, 8)
        .frame(width: 50)
        .background(
            RoundedRectangle(cornerRadius: 6)
                .fill(Color.gray.opacity(0.1))
                .overlay(
                    RoundedRectangle(cornerRadius: 6)
                        .stroke(instrumentIndex == synthController.uiArmedInstrumentIndex ? Color.blue : Color.gray.opacity(0.3), lineWidth: 1)
                )
        )
    }
}