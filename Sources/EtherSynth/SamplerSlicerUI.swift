import SwiftUI

/**
 * SamplerSlicer UI - Loop slicing interface with waveform view and slice markers
 * Supports up to 25 slices with visual editing and real-time preview
 */

// MARK: - SamplerSlicer Models
struct AudioSlice {
    var id: Int
    var startTime: Float // 0.0 to 1.0 of total sample length
    var endTime: Float
    var volume: Float = 0.8
    var pitch: Float = 0.0 // -24 to +24 semitones
    var pan: Float = 0.0 // -1 to +1
    var reverse: Bool = false
    var isActive: Bool = true
    var chokeGroup: Int = 0
    
    var duration: Float {
        return endTime - startTime
    }
    
    var displayName: String {
        return "Slice \(id + 1)"
    }
}

struct WaveformData {
    var samples: [Float] = []
    var sampleRate: Float = 44100
    var duration: Float = 0.0
    var peaks: [Float] = [] // For visual display
    
    var isEmpty: Bool {
        return samples.isEmpty
    }
}

enum SliceMode {
    case manual      // User creates slices manually
    case grid        // Evenly spaced grid slices
    case transient   // Auto-detect based on transients
    case beat        // Beat-synchronized slices
}

// MARK: - Main SamplerSlicer UI
struct SamplerSlicerUI: View {
    let trackId: Int
    let trackColor: InstrumentColor
    
    @Environment(\.presentationMode) var presentationMode
    @State private var waveformData = WaveformData()
    @State private var slices: [AudioSlice] = []
    @State private var selectedSlice: Int? = nil
    @State private var sliceMode: SliceMode = .manual
    @State private var numGridSlices: Int = 8
    @State private var playheadPosition: Float = 0.0
    @State private var isPlaying: Bool = false
    @State private var zoomLevel: Float = 1.0
    @State private var scrollOffset: Float = 0.0
    @State private var showingFileBrowser: Bool = false
    @State private var currentSampleName: String = "No sample loaded"
    @State private var previewSlice: Int? = nil
    
    private let waveformHeight: CGFloat = 200
    private let maxSlices = 25
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            SamplerSlicerHeader(
                trackId: trackId,
                trackColor: trackColor,
                sampleName: currentSampleName,
                onClose: { presentationMode.wrappedValue.dismiss() },
                onLoadSample: { showingFileBrowser = true }
            )
            .padding(.horizontal, 20)
            .padding(.vertical, 16)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(trackColor.pastelColor.opacity(0.1))
            )
            
            // Slice Mode Controls
            SliceModeControls(
                sliceMode: $sliceMode,
                numGridSlices: $numGridSlices,
                trackColor: trackColor,
                onApplySlicing: { applySlicing() },
                onClearSlices: { clearAllSlices() }
            )
            .padding(.horizontal, 20)
            .padding(.vertical, 12)
            
            // Waveform and Slice Editor
            VStack(spacing: 12) {
                // Zoom and Navigation Controls
                WaveformNavigationControls(
                    zoomLevel: $zoomLevel,
                    scrollOffset: $scrollOffset,
                    trackColor: trackColor
                )
                .padding(.horizontal, 20)
                
                // Main Waveform View
                WaveformSliceEditor(
                    waveformData: waveformData,
                    slices: $slices,
                    selectedSlice: $selectedSlice,
                    playheadPosition: playheadPosition,
                    zoomLevel: zoomLevel,
                    scrollOffset: scrollOffset,
                    trackColor: trackColor,
                    onSliceSelect: { index in selectSlice(index) },
                    onSliceMove: { index, newStart, newEnd in moveSlice(index: index, newStart: newStart, newEnd: newEnd) },
                    onSliceCreate: { position in createSliceAt(position: position) },
                    onSliceDelete: { index in deleteSlice(index: index) }
                )
                .frame(height: waveformHeight)
                .padding(.horizontal, 20)
                
                // Slice List
                SliceListView(
                    slices: $slices,
                    selectedSlice: $selectedSlice,
                    trackColor: trackColor,
                    onSliceSelect: { index in selectSlice(index) },
                    onSlicePreview: { index in previewSlice(index: index) }
                )
                .padding(.horizontal, 20)
            }
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(Color.white)
                    .shadow(color: Color.black.opacity(0.05), radius: 2, x: 1, y: 1)
            )
            .padding(.horizontal, 20)
            
            Spacer()
            
            // Selected Slice Inspector
            if let sliceIndex = selectedSlice {
                SliceInspector(
                    slice: $slices[sliceIndex],
                    trackColor: trackColor,
                    onParameterChange: { paramName, value in
                        updateSliceParameter(sliceIndex: sliceIndex, paramName: paramName, value: value)
                    }
                )
                .padding(.horizontal, 20)
                .padding(.vertical, 12)
            }
            
            // Transport Controls
            SlicerTransportControls(
                isPlaying: $isPlaying,
                onPlay: { playEntireSample() },
                onStop: { stopPlayback() },
                onPreviewSlice: { 
                    if let selected = selectedSlice {
                        previewSlice(index: selected)
                    }
                }
            )
            .padding(.horizontal, 20)
            .padding(.bottom, 20)
        }
        .frame(width: 900, height: 700)
        .background(
            RoundedRectangle(cornerRadius: 16)
                .fill(Color.white)
                .shadow(color: Color.black.opacity(0.3), radius: 12, x: 0, y: 8)
        )
        .sheet(isPresented: $showingFileBrowser) {
            SampleFileBrowser(
                onSampleSelect: { samplePath, sampleName in
                    loadSample(path: samplePath, name: sampleName)
                }
            )
        }
        .onAppear {
            initializeDemo()
        }
    }
    
    // MARK: - Helper Functions
    private func initializeDemo() {
        // Load demo waveform
        waveformData = generateDemoWaveform()
        currentSampleName = "Demo_Loop.wav"
        
        // Create initial slices
        slices = [
            AudioSlice(id: 0, startTime: 0.0, endTime: 0.25),
            AudioSlice(id: 1, startTime: 0.25, endTime: 0.5),
            AudioSlice(id: 2, startTime: 0.5, endTime: 0.75),
            AudioSlice(id: 3, startTime: 0.75, endTime: 1.0)
        ]
    }
    
    private func generateDemoWaveform() -> WaveformData {
        var data = WaveformData()
        data.duration = 4.0 // 4 seconds
        data.sampleRate = 44100
        
        // Generate demo waveform data (simplified)
        let numSamples = Int(data.duration * data.sampleRate)
        data.samples = (0..<numSamples).map { i in
            let time = Float(i) / data.sampleRate
            return sin(time * 2 * Float.pi * 440) * 0.5 * exp(-time * 0.5) // Decaying sine
        }
        
        // Generate peaks for display (downsample)
        let peakCount = 1000
        let samplesPerPeak = numSamples / peakCount
        data.peaks = (0..<peakCount).map { peakIndex in
            let startIndex = peakIndex * samplesPerPeak
            let endIndex = min(startIndex + samplesPerPeak, numSamples)
            let peakValue = data.samples[startIndex..<endIndex].map { abs($0) }.max() ?? 0.0
            return peakValue
        }
        
        return data
    }
    
    private func applySlicing() {
        switch sliceMode {
        case .manual:
            break // User creates slices manually
        case .grid:
            createGridSlices()
        case .transient:
            detectTransientSlices()
        case .beat:
            createBeatSlices()
        }
    }
    
    private func createGridSlices() {
        slices.removeAll()
        let sliceCount = min(numGridSlices, maxSlices)
        let sliceSize = 1.0 / Float(sliceCount)
        
        for i in 0..<sliceCount {
            let startTime = Float(i) * sliceSize
            let endTime = Float(i + 1) * sliceSize
            slices.append(AudioSlice(id: i, startTime: startTime, endTime: endTime))
        }
    }
    
    private func detectTransientSlices() {
        // Simplified transient detection
        slices.removeAll()
        var currentSliceStart: Float = 0.0
        var sliceId = 0
        
        // Find peaks in the waveform that could indicate transients
        for (index, peak) in waveformData.peaks.enumerated() {
            if peak > 0.3 && sliceId < maxSlices { // Threshold for transient detection
                let position = Float(index) / Float(waveformData.peaks.count)
                
                if position - currentSliceStart > 0.05 { // Minimum slice length
                    if sliceId > 0 {
                        slices[sliceId - 1].endTime = position
                    }
                    
                    slices.append(AudioSlice(id: sliceId, startTime: currentSliceStart, endTime: 1.0))
                    currentSliceStart = position
                    sliceId += 1
                }
            }
        }
        
        // Ensure the last slice ends at 1.0
        if !slices.isEmpty {
            slices[slices.count - 1].endTime = 1.0
        }
    }
    
    private func createBeatSlices() {
        // Create beat-synchronized slices (assuming 4/4 time)
        createGridSlices() // For now, use grid slicing
    }
    
    private func clearAllSlices() {
        slices.removeAll()
        selectedSlice = nil
    }
    
    private func selectSlice(_ index: Int) {
        selectedSlice = index
    }
    
    private func moveSlice(index: Int, newStart: Float, newEnd: Float) {
        guard index < slices.count else { return }
        slices[index].startTime = max(0.0, newStart)
        slices[index].endTime = min(1.0, newEnd)
    }
    
    private func createSliceAt(position: Float) {
        guard slices.count < maxSlices else { return }
        
        let newSlice = AudioSlice(
            id: slices.count,
            startTime: max(0.0, position - 0.05),
            endTime: min(1.0, position + 0.05)
        )
        slices.append(newSlice)
        selectedSlice = slices.count - 1
    }
    
    private func deleteSlice(index: Int) {
        guard index < slices.count else { return }
        slices.remove(at: index)
        
        // Renumber remaining slices
        for i in 0..<slices.count {
            slices[i].id = i
        }
        
        if selectedSlice == index {
            selectedSlice = nil
        } else if let selected = selectedSlice, selected > index {
            selectedSlice = selected - 1
        }
    }
    
    private func loadSample(path: String, name: String) {
        currentSampleName = name
        // TODO: Load actual audio file and analyze waveform
        waveformData = generateDemoWaveform()
        showingFileBrowser = false
    }
    
    private func playEntireSample() {
        isPlaying = true
        // TODO: Play entire sample and update playhead
    }
    
    private func stopPlayback() {
        isPlaying = false
        playheadPosition = 0.0
    }
    
    private func previewSlice(index: Int) {
        guard index < slices.count else { return }
        previewSlice = index
        // TODO: Play specific slice
        
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
            previewSlice = nil
        }
    }
    
    private func updateSliceParameter(sliceIndex: Int, paramName: String, value: Float) {
        // TODO: Send parameter update to synth controller
        // synthController.setSliceParameter(slice: sliceIndex, param: paramName, value: value)
    }
}

// MARK: - SamplerSlicer Header
struct SamplerSlicerHeader: View {
    let trackId: Int
    let trackColor: InstrumentColor
    let sampleName: String
    let onClose: () -> Void
    let onLoadSample: () -> Void
    
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
                    Text("SamplerSlicer • Track \(trackId + 1)")
                        .font(.system(size: 16, weight: .bold))
                        .foregroundColor(.primary)
                    
                    Text(sampleName)
                        .font(.system(size: 12))
                        .foregroundColor(.secondary)
                        .lineLimit(1)
                }
            }
            
            Spacer()
            
            // Load Sample Button
            Button(action: onLoadSample) {
                HStack(spacing: 4) {
                    Image(systemName: "folder.badge.plus")
                        .font(.system(size: 12))
                    Text("Load Sample")
                        .font(.system(size: 12, weight: .medium))
                }
                .foregroundColor(.blue)
                .padding(.horizontal, 12)
                .padding(.vertical, 6)
                .background(
                    RoundedRectangle(cornerRadius: 16)
                        .fill(Color.blue.opacity(0.1))
                )
            }
            .buttonStyle(PlainButtonStyle())
            
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

// MARK: - Slice Mode Controls
struct SliceModeControls: View {
    @Binding var sliceMode: SliceMode
    @Binding var numGridSlices: Int
    let trackColor: InstrumentColor
    let onApplySlicing: () -> Void
    let onClearSlices: () -> Void
    
    var body: some View {
        HStack(spacing: 16) {
            // Slice Mode Picker
            HStack(spacing: 8) {
                Text("Mode:")
                    .font(.system(size: 12, weight: .medium))
                    .foregroundColor(.secondary)
                
                Picker("Slice Mode", selection: $sliceMode) {
                    Text("Manual").tag(SliceMode.manual)
                    Text("Grid").tag(SliceMode.grid)
                    Text("Transient").tag(SliceMode.transient)
                    Text("Beat").tag(SliceMode.beat)
                }
                .pickerStyle(SegmentedPickerStyle())
                .frame(width: 240)
            }
            
            // Grid Slices Stepper (only for grid mode)
            if sliceMode == .grid {
                HStack(spacing: 8) {
                    Text("Slices:")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundColor(.secondary)
                    
                    Stepper("\(numGridSlices)", value: $numGridSlices, in: 2...25)
                        .labelsHidden()
                        .frame(width: 80)
                    
                    Text("\(numGridSlices)")
                        .font(.system(size: 12, weight: .bold))
                        .foregroundColor(trackColor.pastelColor)
                        .frame(width: 24)
                }
            }
            
            Spacer()
            
            // Action Buttons
            HStack(spacing: 8) {
                Button(action: onApplySlicing) {
                    Text("Apply")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundColor(.white)
                        .padding(.horizontal, 12)
                        .padding(.vertical, 6)
                        .background(
                            RoundedRectangle(cornerRadius: 16)
                                .fill(trackColor.pastelColor)
                        )
                }
                .buttonStyle(PlainButtonStyle())
                
                Button(action: onClearSlices) {
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
            }
        }
    }
}

// MARK: - Waveform Navigation Controls
struct WaveformNavigationControls: View {
    @Binding var zoomLevel: Float
    @Binding var scrollOffset: Float
    let trackColor: InstrumentColor
    
    var body: some View {
        HStack {
            Text("Zoom:")
                .font(.system(size: 12, weight: .medium))
                .foregroundColor(.secondary)
            
            Slider(value: $zoomLevel, in: 1.0...10.0)
                .frame(width: 120)
                .accentColor(trackColor.pastelColor)
            
            Text("\(Int(zoomLevel))x")
                .font(.system(size: 12, weight: .bold))
                .foregroundColor(trackColor.pastelColor)
                .frame(width: 24)
            
            Spacer()
            
            if zoomLevel > 1.0 {
                Text("Scroll:")
                    .font(.system(size: 12, weight: .medium))
                    .foregroundColor(.secondary)
                
                Slider(value: $scrollOffset, in: 0.0...1.0)
                    .frame(width: 120)
                    .accentColor(trackColor.pastelColor)
            }
        }
    }
}

// MARK: - Waveform Slice Editor
struct WaveformSliceEditor: View {
    let waveformData: WaveformData
    @Binding var slices: [AudioSlice]
    @Binding var selectedSlice: Int?
    let playheadPosition: Float
    let zoomLevel: Float
    let scrollOffset: Float
    let trackColor: InstrumentColor
    let onSliceSelect: (Int) -> Void
    let onSliceMove: (Int, Float, Float) -> Void
    let onSliceCreate: (Float) -> Void
    let onSliceDelete: (Int) -> Void
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Background
                Rectangle()
                    .fill(Color.gray.opacity(0.1))
                
                // Waveform
                if !waveformData.isEmpty {
                    WaveformPath(
                        peaks: waveformData.peaks,
                        width: geometry.size.width,
                        height: geometry.size.height,
                        color: Color.primary.opacity(0.6)
                    )
                }
                
                // Slice boundaries
                ForEach(Array(slices.enumerated()), id: \.element.id) { index, slice in
                    SliceMarker(
                        slice: slice,
                        isSelected: selectedSlice == index,
                        width: geometry.size.width,
                        height: geometry.size.height,
                        color: trackColor.pastelColor,
                        onSelect: { onSliceSelect(index) }
                    )
                }
                
                // Playhead
                if playheadPosition > 0 {
                    Rectangle()
                        .fill(Color.red)
                        .frame(width: 2)
                        .offset(x: CGFloat(playheadPosition) * geometry.size.width - geometry.size.width/2)
                }
            }
            .clipped()
            .onTapGesture { location in
                let position = Float(location.x / geometry.size.width)
                onSliceCreate(position)
            }
        }
        .background(
            RoundedRectangle(cornerRadius: 8)
                .stroke(Color.gray.opacity(0.3), lineWidth: 1)
        )
    }
}

// MARK: - Waveform Path
struct WaveformPath: View {
    let peaks: [Float]
    let width: CGFloat
    let height: CGFloat
    let color: Color
    
    var body: some View {
        Path { path in
            guard !peaks.isEmpty else { return }
            
            let stepWidth = width / CGFloat(peaks.count)
            let centerY = height / 2
            
            path.move(to: CGPoint(x: 0, y: centerY))
            
            for (index, peak) in peaks.enumerated() {
                let x = CGFloat(index) * stepWidth
                let y = centerY - CGFloat(peak) * centerY * 0.8
                path.addLine(to: CGPoint(x: x, y: y))
            }
            
            // Draw bottom half (mirrored)
            for (index, peak) in peaks.enumerated().reversed() {
                let x = CGFloat(index) * stepWidth
                let y = centerY + CGFloat(peak) * centerY * 0.8
                path.addLine(to: CGPoint(x: x, y: y))
            }
            
            path.closeSubpath()
        }
        .fill(color)
    }
}

// MARK: - Slice Marker
struct SliceMarker: View {
    let slice: AudioSlice
    let isSelected: Bool
    let width: CGFloat
    let height: CGFloat
    let color: Color
    let onSelect: () -> Void
    
    var body: some View {
        let startX = CGFloat(slice.startTime) * width
        let endX = CGFloat(slice.endTime) * width
        let sliceWidth = endX - startX
        
        ZStack {
            // Slice region highlight
            Rectangle()
                .fill(color.opacity(isSelected ? 0.3 : 0.1))
                .frame(width: sliceWidth, height: height)
                .position(x: startX + sliceWidth/2, y: height/2)
            
            // Start boundary
            Rectangle()
                .fill(color)
                .frame(width: 2, height: height)
                .position(x: startX, y: height/2)
            
            // End boundary
            Rectangle()
                .fill(color)
                .frame(width: 2, height: height)
                .position(x: endX, y: height/2)
            
            // Slice label
            Text(slice.displayName)
                .font(.system(size: 10, weight: .medium))
                .foregroundColor(isSelected ? .white : color)
                .padding(.horizontal, 4)
                .padding(.vertical, 2)
                .background(
                    RoundedRectangle(cornerRadius: 4)
                        .fill(isSelected ? color : Color.white.opacity(0.8))
                )
                .position(x: startX + sliceWidth/2, y: 16)
        }
        .onTapGesture {
            onSelect()
        }
    }
}

// MARK: - Slice List View
struct SliceListView: View {
    @Binding var slices: [AudioSlice]
    @Binding var selectedSlice: Int?
    let trackColor: InstrumentColor
    let onSliceSelect: (Int) -> Void
    let onSlicePreview: (Int) -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Slices (\(slices.count)/25)")
                .font(.system(size: 12, weight: .bold))
                .foregroundColor(.primary)
            
            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: 8) {
                    ForEach(Array(slices.enumerated()), id: \.element.id) { index, slice in
                        SliceItemView(
                            slice: slice,
                            isSelected: selectedSlice == index,
                            trackColor: trackColor,
                            onSelect: { onSliceSelect(index) },
                            onPreview: { onSlicePreview(index) }
                        )
                    }
                    
                    // Add slice button
                    if slices.count < 25 {
                        Button(action: {
                            // Create new slice at end
                            let newSlice = AudioSlice(
                                id: slices.count,
                                startTime: slices.isEmpty ? 0.0 : slices.last!.endTime,
                                endTime: 1.0
                            )
                            slices.append(newSlice)
                        }) {
                            VStack(spacing: 4) {
                                Image(systemName: "plus")
                                    .font(.system(size: 16))
                                    .foregroundColor(.secondary)
                                
                                Text("Add")
                                    .font(.system(size: 10))
                                    .foregroundColor(.secondary)
                            }
                            .frame(width: 60, height: 50)
                            .background(
                                RoundedRectangle(cornerRadius: 8)
                                    .stroke(Color.gray.opacity(0.3), lineWidth: 1)
                                    .background(Color.white)
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

// MARK: - Slice Item View
struct SliceItemView: View {
    let slice: AudioSlice
    let isSelected: Bool
    let trackColor: InstrumentColor
    let onSelect: () -> Void
    let onPreview: () -> Void
    
    var body: some View {
        Button(action: onSelect) {
            VStack(spacing: 4) {
                // Slice number
                Text("\(slice.id + 1)")
                    .font(.system(size: 12, weight: .bold))
                    .foregroundColor(isSelected ? .white : trackColor.pastelColor)
                
                // Duration
                Text("\(Int(slice.duration * 1000))ms")
                    .font(.system(size: 8))
                    .foregroundColor(isSelected ? .white.opacity(0.8) : .secondary)
                
                // Active indicator
                Circle()
                    .fill(slice.isActive ? Color.green : Color.gray)
                    .frame(width: 6, height: 6)
            }
            .frame(width: 60, height: 50)
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .fill(isSelected ? trackColor.pastelColor : Color.white)
                    .shadow(color: Color.black.opacity(0.1), radius: 2, x: 1, y: 1)
            )
        }
        .buttonStyle(PlainButtonStyle())
        .onLongPressGesture {
            onPreview()
        }
    }
}

// MARK: - Slice Inspector
struct SliceInspector: View {
    @Binding var slice: AudioSlice
    let trackColor: InstrumentColor
    let onParameterChange: (String, Float) -> Void
    
    var body: some View {
        VStack(spacing: 12) {
            Text("Slice \(slice.id + 1) Inspector")
                .font(.system(size: 14, weight: .bold))
                .foregroundColor(.primary)
            
            HStack(spacing: 20) {
                // Timing
                VStack(spacing: 8) {
                    Text("Timing")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundColor(.secondary)
                    
                    HStack(spacing: 8) {
                        VStack(spacing: 4) {
                            Text("Start")
                                .font(.system(size: 10))
                                .foregroundColor(.secondary)
                            Text("\(Int(slice.startTime * 100))%")
                                .font(.system(size: 10, weight: .bold))
                                .foregroundColor(.primary)
                        }
                        
                        VStack(spacing: 4) {
                            Text("End")
                                .font(.system(size: 10))
                                .foregroundColor(.secondary)
                            Text("\(Int(slice.endTime * 100))%")
                                .font(.system(size: 10, weight: .bold))
                                .foregroundColor(.primary)
                        }
                        
                        VStack(spacing: 4) {
                            Text("Duration")
                                .font(.system(size: 10))
                                .foregroundColor(.secondary)
                            Text("\(Int(slice.duration * 1000))ms")
                                .font(.system(size: 10, weight: .bold))
                                .foregroundColor(trackColor.pastelColor)
                        }
                    }
                }
                
                // Level & Pitch
                VStack(spacing: 8) {
                    Text("Level & Pitch")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundColor(.secondary)
                    
                    HStack(spacing: 16) {
                        SliceParameterSlider(name: "Volume", value: $slice.volume, range: 0...1, unit: "%", color: trackColor.pastelColor)
                        SliceParameterSlider(name: "Pitch", value: $slice.pitch, range: -24...24, unit: "st", color: trackColor.pastelColor)
                        SliceParameterSlider(name: "Pan", value: $slice.pan, range: -1...1, unit: "", color: trackColor.pastelColor)
                    }
                }
                
                // Options
                VStack(spacing: 8) {
                    Text("Options")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundColor(.secondary)
                    
                    VStack(spacing: 8) {
                        HStack(spacing: 16) {
                            Toggle("Active", isOn: $slice.isActive)
                                .font(.system(size: 10))
                            
                            Toggle("Reverse", isOn: $slice.reverse)
                                .font(.system(size: 10))
                        }
                        .toggleStyle(SwitchToggleStyle(tint: trackColor.pastelColor))
                        
                        HStack {
                            Text("Choke")
                                .font(.system(size: 10))
                                .foregroundColor(.secondary)
                            Stepper("\(slice.chokeGroup)", value: $slice.chokeGroup, in: 0...8)
                                .labelsHidden()
                            Text("\(slice.chokeGroup == 0 ? "None" : "\(slice.chokeGroup)")")
                                .font(.system(size: 10))
                                .foregroundColor(.primary)
                        }
                    }
                }
            }
        }
        .padding(.vertical, 12)
        .padding(.horizontal, 16)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(trackColor.pastelColor.opacity(0.05))
        )
    }
}

// MARK: - Slice Parameter Slider
struct SliceParameterSlider: View {
    let name: String
    @Binding var value: Float
    let range: ClosedRange<Float>
    let unit: String
    let color: Color
    
    var body: some View {
        VStack(spacing: 4) {
            Text(name)
                .font(.system(size: 10))
                .foregroundColor(.secondary)
            
            Slider(value: $value, in: range)
                .frame(width: 60)
                .accentColor(color)
                .controlSize(.mini)
            
            Text(valueString)
                .font(.system(size: 8))
                .foregroundColor(.primary)
        }
        .frame(width: 80)
    }
    
    private var valueString: String {
        switch unit {
        case "%":
            return "\(Int(value * 100))%"
        case "st":
            return "\(value > 0 ? "+" : "")\(String(format: "%.1f", value))st"
        default:
            if value == 0 {
                return "Center"
            } else {
                return value > 0 ? "R\(Int(abs(value) * 100))" : "L\(Int(abs(value) * 100))"
            }
        }
    }
}

// MARK: - Slicer Transport Controls
struct SlicerTransportControls: View {
    @Binding var isPlaying: Bool
    let onPlay: () -> Void
    let onStop: () -> Void
    let onPreviewSlice: () -> Void
    
    var body: some View {
        HStack(spacing: 16) {
            // Transport
            HStack(spacing: 8) {
                Button(action: isPlaying ? onStop : onPlay) {
                    HStack(spacing: 4) {
                        Image(systemName: isPlaying ? "pause.fill" : "play.fill")
                            .font(.system(size: 12))
                        Text(isPlaying ? "Pause" : "Play All")
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
            
            // Preview Slice
            Button(action: onPreviewSlice) {
                HStack(spacing: 4) {
                    Image(systemName: "speaker.wave.2")
                        .font(.system(size: 12))
                    Text("Preview Slice")
                        .font(.system(size: 12, weight: .medium))
                }
                .foregroundColor(.blue)
                .padding(.horizontal, 16)
                .padding(.vertical, 8)
                .background(
                    RoundedRectangle(cornerRadius: 20)
                        .fill(Color.blue.opacity(0.1))
                )
            }
            .buttonStyle(PlainButtonStyle())
        }
    }
}

// MARK: - Sample File Browser (Placeholder)
struct SampleFileBrowser: View {
    let onSampleSelect: (String, String) -> Void
    
    var body: some View {
        VStack {
            Text("Sample File Browser")
                .font(.headline)
            
            Text("Sample browser with audio preview would go here")
                .foregroundColor(.secondary)
            
            Button("Select Demo Sample") {
                onSampleSelect("/path/to/demo_loop.wav", "Demo_Loop.wav")
            }
        }
        .frame(width: 600, height: 400)
        .padding()
    }
}