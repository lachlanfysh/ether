import SwiftUI

/**
 * Loop Browser - Folder navigation for audio files with real-time preview
 * Supports various audio formats with metadata display and batch import
 */

// MARK: - Loop Browser Models
struct AudioFile {
    let id: String
    let name: String
    let path: String
    let url: URL
    let size: Int64
    let duration: Float?
    let sampleRate: Float?
    let channels: Int?
    let bitDepth: Int?
    let bpm: Float?
    let key: String?
    let format: AudioFormat
    let lastModified: Date
    let isSelected: Bool = false
    
    var displayName: String {
        return url.deletingPathExtension().lastPathComponent
    }
    
    var sizeString: String {
        let formatter = ByteCountFormatter()
        formatter.allowedUnits = [.useKB, .useMB]
        formatter.countStyle = .file
        return formatter.string(fromByteCount: size)
    }
    
    var durationString: String {
        guard let duration = duration else { return "--" }
        let minutes = Int(duration) / 60
        let seconds = Int(duration) % 60
        return String(format: "%d:%02d", minutes, seconds)
    }
    
    var formatString: String {
        return format.rawValue.uppercased()
    }
}

enum AudioFormat: String, CaseIterable {
    case wav = "wav"
    case aiff = "aiff"
    case mp3 = "mp3"
    case m4a = "m4a"
    case flac = "flac"
    case ogg = "ogg"
    
    var icon: String {
        switch self {
        case .wav, .aiff: return "waveform"
        case .mp3, .m4a: return "music.note"
        case .flac: return "music.note.list"
        case .ogg: return "doc.audio"
        }
    }
    
    var color: Color {
        switch self {
        case .wav, .aiff: return .blue
        case .mp3, .m4a: return .green
        case .flac: return .purple
        case .ogg: return .orange
        }
    }
}

struct FolderItem {
    let id: String
    let name: String
    let path: String
    let url: URL
    let isFolder: Bool
    let audioFileCount: Int
    let lastModified: Date
    
    var displayName: String {
        return url.lastPathComponent
    }
}

struct BrowserState {
    var currentPath: String = ""
    var pathComponents: [String] = []
    var folders: [FolderItem] = []
    var audioFiles: [AudioFile] = []
    var selectedFiles: Set<String> = []
    var isLoading: Bool = false
    var sortOrder: SortOrder = .name
    var showHiddenFiles: Bool = false
    var searchText: String = ""
    
    enum SortOrder: String, CaseIterable {
        case name = "Name"
        case date = "Date"
        case size = "Size"
        case duration = "Duration"
        case format = "Format"
    }
}

// MARK: - Main Loop Browser
struct LoopBrowser: View {
    let onSampleSelect: (String, String) -> Void
    let onBatchSelect: ([String]) -> Void
    
    @State private var browserState = BrowserState()
    @State private var previewingFile: AudioFile? = nil
    @State private var showingFormatFilter: Bool = false
    @State private var selectedFormats: Set<AudioFormat> = Set(AudioFormat.allCases)
    @State private var showingImportOptions: Bool = false
    @State private var autoAnalyze: Bool = true
    @State private var copyToLibrary: Bool = false
    
    private let homeDirectory = FileManager.default.homeDirectoryForCurrentUser
    
    var filteredAudioFiles: [AudioFile] {
        browserState.audioFiles.filter { file in
            let formatMatch = selectedFormats.contains(file.format)
            let searchMatch = browserState.searchText.isEmpty || 
                             file.displayName.localizedCaseInsensitiveContains(browserState.searchText)
            return formatMatch && searchMatch
        }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            LoopBrowserHeader(
                currentPath: browserState.currentPath,
                selectedCount: browserState.selectedFiles.count,
                onHome: { navigateToHome() },
                onImport: { showingImportOptions = true },
                onClose: { /* Close browser */ }
            )
            .padding(.horizontal, 20)
            .padding(.vertical, 16)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(Color.green.opacity(0.1))
            )
            
            HStack(spacing: 0) {
                // Left Panel - Navigation & Controls
                VStack(spacing: 16) {
                    // Breadcrumb Navigation
                    FileBreadcrumbNavigation(
                        pathComponents: browserState.pathComponents,
                        onNavigate: { path in navigateToPath(path) }
                    )
                    
                    // Search & Filter
                    VStack(spacing: 12) {
                        SearchBar(searchText: $browserState.searchText)
                        
                        FilterControls(
                            sortOrder: $browserState.sortOrder,
                            selectedFormats: $selectedFormats,
                            showHiddenFiles: $browserState.showHiddenFiles,
                            onSortChange: { sortFiles() }
                        )
                    }
                    
                    // Quick Navigation
                    QuickNavigation(
                        onNavigate: { path in navigateToPath(path) }
                    )
                    
                    Spacer()
                    
                    // Preview Controls
                    if let previewing = previewingFile {
                        PreviewControls(
                            file: previewing,
                            onStopPreview: { stopPreview() },
                            onSelect: { 
                                onSampleSelect(previewing.path, previewing.name)
                            }
                        )
                    }
                }
                .frame(width: 280)
                .padding(.horizontal, 16)
                .padding(.vertical, 16)
                .background(Color.gray.opacity(0.05))
                
                // Main Browser Area
                VStack(spacing: 12) {
                    // Folder List
                    if !browserState.folders.isEmpty {
                        FolderListView(
                            folders: browserState.folders,
                            onFolderSelect: { folder in 
                                navigateToFolder(folder)
                            }
                        )
                        .frame(height: 120)
                    }
                    
                    // File List
                    FileListView(
                        audioFiles: filteredAudioFiles,
                        selectedFiles: browserState.selectedFiles,
                        previewingFile: previewingFile,
                        onFileSelect: { file in selectFile(file) },
                        onFilePreview: { file in startPreview(file) },
                        onFileDoubleClick: { file in 
                            onSampleSelect(file.path, file.name)
                        },
                        onSelectionChange: { fileId, isSelected in
                            updateSelection(fileId: fileId, isSelected: isSelected)
                        }
                    )
                    
                    // Status Bar
                    BrowserStatusBar(
                        folderCount: browserState.folders.count,
                        fileCount: filteredAudioFiles.count,
                        selectedCount: browserState.selectedFiles.count,
                        isLoading: browserState.isLoading
                    )
                    .padding(.horizontal, 20)
                }
                .padding(.vertical, 16)
            }
        }
        .frame(width: 1000, height: 700)
        .background(
            RoundedRectangle(cornerRadius: 16)
                .fill(Color.white)
                .shadow(color: Color.black.opacity(0.3), radius: 12, x: 0, y: 8)
        )
        .sheet(isPresented: $showingImportOptions) {
            ImportOptionsSheet(
                selectedFiles: Array(browserState.selectedFiles),
                autoAnalyze: $autoAnalyze,
                copyToLibrary: $copyToLibrary,
                onImport: { files in
                    onBatchSelect(files)
                }
            )
        }
        .onAppear {
            navigateToHome()
        }
    }
    
    // MARK: - Helper Functions
    private func navigateToHome() {
        let musicDirectory = homeDirectory.appendingPathComponent("Music")
        navigateToPath(musicDirectory.path)
    }
    
    private func navigateToPath(_ path: String) {
        browserState.isLoading = true
        browserState.currentPath = path
        browserState.pathComponents = path.components(separatedBy: "/").filter { !$0.isEmpty }
        
        // Load directory contents
        loadDirectoryContents(path: path)
        
        browserState.isLoading = false
    }
    
    private func navigateToFolder(_ folder: FolderItem) {
        navigateToPath(folder.path)
    }
    
    private func loadDirectoryContents(path: String) {
        let fileManager = FileManager.default
        let url = URL(fileURLWithPath: path)
        
        do {
            let contents = try fileManager.contentsOfDirectory(at: url, includingPropertiesForKeys: [
                .isDirectoryKey,
                .fileSizeKey,
                .contentModificationDateKey
            ], options: browserState.showHiddenFiles ? [] : [.skipsHiddenFiles])
            
            var folders: [FolderItem] = []
            var audioFiles: [AudioFile] = []
            
            for item in contents {
                let resourceValues = try item.resourceValues(forKeys: [
                    .isDirectoryKey,
                    .fileSizeKey,
                    .contentModificationDateKey
                ])
                
                let isDirectory = resourceValues.isDirectory ?? false
                let size = resourceValues.fileSize ?? 0
                let lastModified = resourceValues.contentModificationDate ?? Date()
                
                if isDirectory {
                    // Count audio files in subdirectory
                    let audioFileCount = countAudioFiles(in: item)
                    
                    folders.append(FolderItem(
                        id: item.path,
                        name: item.lastPathComponent,
                        path: item.path,
                        url: item,
                        isFolder: true,
                        audioFileCount: audioFileCount,
                        lastModified: lastModified
                    ))
                } else {
                    // Check if it's an audio file
                    if let format = AudioFormat(rawValue: item.pathExtension.lowercased()) {
                        let audioFile = AudioFile(
                            id: item.path,
                            name: item.lastPathComponent,
                            path: item.path,
                            url: item,
                            size: Int64(size),
                            duration: analyzeAudioDuration(url: item),
                            sampleRate: 44100, // TODO: Analyze actual sample rate
                            channels: 2, // TODO: Analyze actual channels
                            bitDepth: 16, // TODO: Analyze actual bit depth
                            bpm: nil, // TODO: Analyze BPM
                            key: nil, // TODO: Analyze key
                            format: format,
                            lastModified: lastModified
                        )
                        audioFiles.append(audioFile)
                    }
                }
            }
            
            browserState.folders = folders.sorted { $0.name < $1.name }
            browserState.audioFiles = audioFiles
            sortFiles()
            
        } catch {
            print("Error loading directory contents: \(error)")
            browserState.folders = []
            browserState.audioFiles = []
        }
    }
    
    private func countAudioFiles(in directory: URL) -> Int {
        let fileManager = FileManager.default
        guard let enumerator = fileManager.enumerator(at: directory, includingPropertiesForKeys: nil) else {
            return 0
        }
        
        var count = 0
        for case let fileURL as URL in enumerator {
            if AudioFormat(rawValue: fileURL.pathExtension.lowercased()) != nil {
                count += 1
            }
        }
        return count
    }
    
    private func analyzeAudioDuration(url: URL) -> Float? {
        // TODO: Implement actual audio analysis
        // For now, return random duration for demo
        return Float.random(in: 10.0...300.0)
    }
    
    private func sortFiles() {
        switch browserState.sortOrder {
        case .name:
            browserState.audioFiles.sort { $0.displayName < $1.displayName }
        case .date:
            browserState.audioFiles.sort { $0.lastModified > $1.lastModified }
        case .size:
            browserState.audioFiles.sort { $0.size > $1.size }
        case .duration:
            browserState.audioFiles.sort { ($0.duration ?? 0) > ($1.duration ?? 0) }
        case .format:
            browserState.audioFiles.sort { $0.format.rawValue < $1.format.rawValue }
        }
    }
    
    private func selectFile(_ file: AudioFile) {
        // Single selection for preview
        previewingFile = file
    }
    
    private func updateSelection(fileId: String, isSelected: Bool) {
        if isSelected {
            browserState.selectedFiles.insert(fileId)
        } else {
            browserState.selectedFiles.remove(fileId)
        }
    }
    
    private func startPreview(_ file: AudioFile) {
        previewingFile = file
        // TODO: Start audio preview
    }
    
    private func stopPreview() {
        previewingFile = nil
        // TODO: Stop audio preview
    }
}

// MARK: - Loop Browser Header
struct LoopBrowserHeader: View {
    let currentPath: String
    let selectedCount: Int
    let onHome: () -> Void
    let onImport: () -> Void
    let onClose: () -> Void
    
    var body: some View {
        HStack {
            // Title
            HStack(spacing: 12) {
                Circle()
                    .fill(Color.green)
                    .frame(width: 32, height: 32)
                    .overlay(
                        Image(systemName: "folder.circle.fill")
                            .font(.system(size: 16))
                            .foregroundColor(.white)
                    )
                
                VStack(alignment: .leading, spacing: 2) {
                    Text("Loop Browser")
                        .font(.system(size: 16, weight: .bold))
                        .foregroundColor(.primary)
                    
                    Text("Browse audio files")
                        .font(.system(size: 12))
                        .foregroundColor(.secondary)
                }
            }
            
            Spacer()
            
            // Path Info
            if !currentPath.isEmpty {
                Text(URL(fileURLWithPath: currentPath).lastPathComponent)
                    .font(.system(size: 12, weight: .medium))
                    .foregroundColor(.secondary)
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(
                        RoundedRectangle(cornerRadius: 8)
                            .fill(Color.white.opacity(0.5))
                    )
            }
            
            // Actions
            HStack(spacing: 8) {
                Button(action: onHome) {
                    Image(systemName: "house")
                        .font(.system(size: 12))
                        .foregroundColor(.green)
                        .padding(.horizontal, 8)
                        .padding(.vertical, 6)
                        .background(
                            RoundedRectangle(cornerRadius: 16)
                                .fill(Color.green.opacity(0.1))
                        )
                }
                .buttonStyle(PlainButtonStyle())
                
                if selectedCount > 0 {
                    Button(action: onImport) {
                        HStack(spacing: 4) {
                            Image(systemName: "square.and.arrow.down")
                                .font(.system(size: 12))
                            Text("Import (\(selectedCount))")
                                .font(.system(size: 12, weight: .medium))
                        }
                        .foregroundColor(.white)
                        .padding(.horizontal, 12)
                        .padding(.vertical, 6)
                        .background(
                            RoundedRectangle(cornerRadius: 16)
                                .fill(Color.green)
                        )
                    }
                    .buttonStyle(PlainButtonStyle())
                }
                
                Button(action: onClose) {
                    Image(systemName: "xmark.circle.fill")
                        .font(.system(size: 20))
                        .foregroundColor(.secondary)
                }
                .buttonStyle(PlainButtonStyle())
            }
        }
    }
}

// MARK: - Breadcrumb Navigation
struct FileBreadcrumbNavigation: View {
    let pathComponents: [String]
    let onNavigate: (String) -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Location")
                .font(.system(size: 12, weight: .bold))
                .foregroundColor(.primary)
            
            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: 4) {
                    // Root
                    Button(action: { onNavigate("/") }) {
                        Image(systemName: "externaldrive")
                            .font(.system(size: 10))
                            .foregroundColor(.blue)
                    }
                    .buttonStyle(PlainButtonStyle())
                    
                    // Path components
                    ForEach(Array(pathComponents.enumerated()), id: \.offset) { index, component in
                        HStack(spacing: 4) {
                            Image(systemName: "chevron.right")
                                .font(.system(size: 8))
                                .foregroundColor(.secondary)
                            
                            Button(action: {
                                let path = "/" + pathComponents.prefix(index + 1).joined(separator: "/")
                                onNavigate(path)
                            }) {
                                Text(component)
                                    .font(.system(size: 10))
                                    .foregroundColor(index == pathComponents.count - 1 ? .primary : .blue)
                                    .lineLimit(1)
                            }
                            .buttonStyle(PlainButtonStyle())
                        }
                    }
                }
            }
        }
    }
}

// MARK: - Search Bar
struct SearchBar: View {
    @Binding var searchText: String
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Search")
                .font(.system(size: 12, weight: .bold))
                .foregroundColor(.primary)
            
            HStack {
                Image(systemName: "magnifyingglass")
                    .font(.system(size: 12))
                    .foregroundColor(.secondary)
                
                TextField("File name...", text: $searchText)
                    .textFieldStyle(PlainTextFieldStyle())
                    .font(.system(size: 12))
                
                if !searchText.isEmpty {
                    Button(action: { searchText = "" }) {
                        Image(systemName: "xmark.circle.fill")
                            .font(.system(size: 12))
                            .foregroundColor(.secondary)
                    }
                    .buttonStyle(PlainButtonStyle())
                }
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 6)
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .stroke(Color.gray.opacity(0.3), lineWidth: 1)
                    .background(Color.white)
            )
        }
    }
}

// MARK: - Filter Controls
struct FilterControls: View {
    @Binding var sortOrder: BrowserState.SortOrder
    @Binding var selectedFormats: Set<AudioFormat>
    @Binding var showHiddenFiles: Bool
    let onSortChange: () -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            // Sort Order
            VStack(alignment: .leading, spacing: 8) {
                Text("Sort by")
                    .font(.system(size: 12, weight: .bold))
                    .foregroundColor(.primary)
                
                Picker("Sort Order", selection: $sortOrder) {
                    ForEach(BrowserState.SortOrder.allCases, id: \.self) { order in
                        Text(order.rawValue).tag(order)
                    }
                }
                .pickerStyle(MenuPickerStyle())
                .onChange(of: sortOrder) { _ in onSortChange() }
            }
            
            // Format Filter
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Text("Formats")
                        .font(.system(size: 12, weight: .bold))
                        .foregroundColor(.primary)
                    
                    Spacer()
                    
                    Button(action: toggleAllFormats) {
                        Text(selectedFormats.count == AudioFormat.allCases.count ? "None" : "All")
                            .font(.system(size: 10, weight: .medium))
                            .foregroundColor(.blue)
                    }
                    .buttonStyle(PlainButtonStyle())
                }
                
                LazyVGrid(columns: [
                    GridItem(.flexible()),
                    GridItem(.flexible())
                ], spacing: 4) {
                    ForEach(AudioFormat.allCases, id: \.self) { format in
                        FormatToggle(
                            format: format,
                            isSelected: selectedFormats.contains(format),
                            onToggle: { toggleFormat(format) }
                        )
                    }
                }
            }
            
            // Options
            VStack(alignment: .leading, spacing: 8) {
                Text("Options")
                    .font(.system(size: 12, weight: .bold))
                    .foregroundColor(.primary)
                
                Toggle("Show hidden files", isOn: $showHiddenFiles)
                    .font(.system(size: 10))
                    .toggleStyle(SwitchToggleStyle(tint: .green))
            }
        }
    }
    
    private func toggleFormat(_ format: AudioFormat) {
        if selectedFormats.contains(format) {
            selectedFormats.remove(format)
        } else {
            selectedFormats.insert(format)
        }
    }
    
    private func toggleAllFormats() {
        if selectedFormats.count == AudioFormat.allCases.count {
            selectedFormats.removeAll()
        } else {
            selectedFormats = Set(AudioFormat.allCases)
        }
    }
}

// MARK: - Format Toggle
struct FormatToggle: View {
    let format: AudioFormat
    let isSelected: Bool
    let onToggle: () -> Void
    
    var body: some View {
        Button(action: onToggle) {
            HStack(spacing: 4) {
                Image(systemName: format.icon)
                    .font(.system(size: 8))
                    .foregroundColor(isSelected ? .white : format.color)
                
                Text(format.rawValue.uppercased())
                    .font(.system(size: 8, weight: .medium))
                    .foregroundColor(isSelected ? .white : format.color)
            }
            .padding(.horizontal, 6)
            .padding(.vertical, 4)
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .fill(isSelected ? format.color : format.color.opacity(0.1))
            )
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - Quick Navigation
struct QuickNavigation: View {
    let onNavigate: (String) -> Void
    
    private let quickPaths = [
        ("Desktop", "~/Desktop"),
        ("Downloads", "~/Downloads"),
        ("Documents", "~/Documents"),
        ("Music", "~/Music")
    ]
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Quick Access")
                .font(.system(size: 12, weight: .bold))
                .foregroundColor(.primary)
            
            VStack(spacing: 4) {
                ForEach(quickPaths, id: \.0) { name, path in
                    Button(action: {
                        let expandedPath = NSString(string: path).expandingTildeInPath
                        onNavigate(expandedPath)
                    }) {
                        HStack {
                            Image(systemName: iconForPath(name))
                                .font(.system(size: 10))
                                .foregroundColor(.blue)
                                .frame(width: 16)
                            
                            Text(name)
                                .font(.system(size: 10))
                                .foregroundColor(.primary)
                            
                            Spacer()
                        }
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(
                            RoundedRectangle(cornerRadius: 6)
                                .fill(Color.clear)
                        )
                    }
                    .buttonStyle(PlainButtonStyle())
                }
            }
        }
    }
    
    private func iconForPath(_ name: String) -> String {
        switch name {
        case "Desktop": return "desktopcomputer"
        case "Downloads": return "arrow.down.circle"
        case "Documents": return "doc.folder"
        case "Music": return "music.note.house"
        default: return "folder"
        }
    }
}

// MARK: - Folder List View
struct FolderListView: View {
    let folders: [FolderItem]
    let onFolderSelect: (FolderItem) -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Folders (\(folders.count))")
                .font(.system(size: 12, weight: .bold))
                .foregroundColor(.primary)
                .padding(.horizontal, 20)
            
            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: 8) {
                    ForEach(folders, id: \.id) { folder in
                        FolderItemView(
                            folder: folder,
                            onSelect: { onFolderSelect(folder) }
                        )
                    }
                }
                .padding(.horizontal, 20)
            }
        }
    }
}

// MARK: - Folder Item View
struct FolderItemView: View {
    let folder: FolderItem
    let onSelect: () -> Void
    
    var body: some View {
        Button(action: onSelect) {
            VStack(spacing: 8) {
                Image(systemName: "folder.fill")
                    .font(.system(size: 24))
                    .foregroundColor(.blue)
                
                VStack(spacing: 2) {
                    Text(folder.displayName)
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.primary)
                        .lineLimit(2)
                        .multilineTextAlignment(.center)
                    
                    if folder.audioFileCount > 0 {
                        Text("\(folder.audioFileCount) files")
                            .font(.system(size: 8))
                            .foregroundColor(.secondary)
                    }
                }
            }
            .frame(width: 80, height: 80)
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .fill(Color.white)
                    .shadow(color: Color.black.opacity(0.1), radius: 2, x: 1, y: 1)
            )
        }
        .buttonStyle(PlainButtonStyle())
    }
}

// MARK: - File List View
struct FileListView: View {
    let audioFiles: [AudioFile]
    let selectedFiles: Set<String>
    let previewingFile: AudioFile?
    let onFileSelect: (AudioFile) -> Void
    let onFilePreview: (AudioFile) -> Void
    let onFileDoubleClick: (AudioFile) -> Void
    let onSelectionChange: (String, Bool) -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Audio Files (\(audioFiles.count))")
                .font(.system(size: 12, weight: .bold))
                .foregroundColor(.primary)
                .padding(.horizontal, 20)
            
            if audioFiles.isEmpty {
                VStack(spacing: 12) {
                    Image(systemName: "music.note.list")
                        .font(.system(size: 32))
                        .foregroundColor(.secondary)
                    
                    Text("No audio files found")
                        .font(.system(size: 14))
                        .foregroundColor(.secondary)
                    
                    Text("Try adjusting your filters or navigating to a different folder")
                        .font(.system(size: 12))
                        .foregroundColor(.secondary)
                        .multilineTextAlignment(.center)
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            } else {
                List(audioFiles, id: \.id) { file in
                    AudioFileRow(
                        file: file,
                        isSelected: selectedFiles.contains(file.id),
                        isPreviewing: previewingFile?.id == file.id,
                        onSelect: { onFileSelect(file) },
                        onPreview: { onFilePreview(file) },
                        onDoubleClick: { onFileDoubleClick(file) },
                        onSelectionChange: { isSelected in
                            onSelectionChange(file.id, isSelected)
                        }
                    )
                }
                .listStyle(PlainListStyle())
            }
        }
    }
}

// MARK: - Audio File Row
struct AudioFileRow: View {
    let file: AudioFile
    let isSelected: Bool
    let isPreviewing: Bool
    let onSelect: () -> Void
    let onPreview: () -> Void
    let onDoubleClick: () -> Void
    let onSelectionChange: (Bool) -> Void
    
    var body: some View {
        HStack(spacing: 12) {
            // Selection checkbox
            Button(action: { onSelectionChange(!isSelected) }) {
                Image(systemName: isSelected ? "checkmark.square.fill" : "square")
                    .font(.system(size: 14))
                    .foregroundColor(isSelected ? .blue : .secondary)
            }
            .buttonStyle(PlainButtonStyle())
            
            // Format icon
            Image(systemName: file.format.icon)
                .font(.system(size: 14))
                .foregroundColor(file.format.color)
                .frame(width: 20)
            
            // File info
            VStack(alignment: .leading, spacing: 2) {
                Text(file.displayName)
                    .font(.system(size: 12, weight: .medium))
                    .foregroundColor(.primary)
                    .lineLimit(1)
                
                HStack(spacing: 8) {
                    Text(file.formatString)
                        .font(.system(size: 10))
                        .foregroundColor(file.format.color)
                    
                    Text(file.durationString)
                        .font(.system(size: 10))
                        .foregroundColor(.secondary)
                    
                    Text(file.sizeString)
                        .font(.system(size: 10))
                        .foregroundColor(.secondary)
                    
                    if let bpm = file.bpm {
                        Text("\(Int(bpm)) BPM")
                            .font(.system(size: 10))
                            .foregroundColor(.secondary)
                    }
                    
                    if let key = file.key {
                        Text(key)
                            .font(.system(size: 10))
                            .foregroundColor(.secondary)
                    }
                }
            }
            
            Spacer()
            
            // Preview button
            Button(action: onPreview) {
                Image(systemName: isPreviewing ? "speaker.wave.3.fill" : "speaker.wave.2")
                    .font(.system(size: 12))
                    .foregroundColor(isPreviewing ? .green : .blue)
            }
            .buttonStyle(PlainButtonStyle())
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
        .background(
            RoundedRectangle(cornerRadius: 6)
                .fill(isPreviewing ? Color.green.opacity(0.1) : Color.clear)
        )
        .onTapGesture {
            onSelect()
        }
        .onDoubleClick {
            onDoubleClick()
        }
    }
}

// MARK: - Preview Controls
struct PreviewControls: View {
    let file: AudioFile
    let onStopPreview: () -> Void
    let onSelect: () -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Now Playing")
                .font(.system(size: 12, weight: .bold))
                .foregroundColor(.primary)
            
            VStack(alignment: .leading, spacing: 8) {
                Text(file.displayName)
                    .font(.system(size: 11, weight: .medium))
                    .foregroundColor(.primary)
                    .lineLimit(2)
                
                HStack {
                    Image(systemName: file.format.icon)
                        .font(.system(size: 10))
                        .foregroundColor(file.format.color)
                    
                    Text(file.formatString)
                        .font(.system(size: 10))
                        .foregroundColor(file.format.color)
                    
                    Spacer()
                }
                
                Text(file.durationString)
                    .font(.system(size: 10))
                    .foregroundColor(.secondary)
                
                // Mock playback progress
                ProgressView(value: 0.3)
                    .progressViewStyle(LinearProgressViewStyle(tint: .green))
                    .controlSize(.mini)
            }
            .padding(.vertical, 8)
            .padding(.horizontal, 10)
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .stroke(Color.green.opacity(0.2), lineWidth: 1)
                    .background(Color.green.opacity(0.05))
            )
            
            // Controls
            VStack(spacing: 8) {
                Button(action: onStopPreview) {
                    HStack(spacing: 4) {
                        Image(systemName: "stop.fill")
                            .font(.system(size: 10))
                        Text("Stop")
                            .font(.system(size: 10, weight: .medium))
                    }
                    .foregroundColor(.red)
                    .frame(maxWidth: .infinity)
                    .padding(.vertical, 6)
                    .background(
                        RoundedRectangle(cornerRadius: 6)
                            .fill(Color.red.opacity(0.1))
                    )
                }
                .buttonStyle(PlainButtonStyle())
                
                Button(action: onSelect) {
                    HStack(spacing: 4) {
                        Image(systemName: "checkmark.circle")
                            .font(.system(size: 10))
                        Text("Select")
                            .font(.system(size: 10, weight: .medium))
                    }
                    .foregroundColor(.white)
                    .frame(maxWidth: .infinity)
                    .padding(.vertical, 6)
                    .background(
                        RoundedRectangle(cornerRadius: 6)
                            .fill(Color.green)
                    )
                }
                .buttonStyle(PlainButtonStyle())
            }
        }
    }
}

// MARK: - Browser Status Bar
struct BrowserStatusBar: View {
    let folderCount: Int
    let fileCount: Int
    let selectedCount: Int
    let isLoading: Bool
    
    var body: some View {
        HStack {
            if isLoading {
                HStack(spacing: 8) {
                    ProgressView()
                        .controlSize(.mini)
                    Text("Loading...")
                        .font(.system(size: 10))
                        .foregroundColor(.secondary)
                }
            } else {
                Text("\(folderCount) folders, \(fileCount) files")
                    .font(.system(size: 10))
                    .foregroundColor(.secondary)
                
                if selectedCount > 0 {
                    Text("•")
                        .font(.system(size: 10))
                        .foregroundColor(.secondary)
                    
                    Text("\(selectedCount) selected")
                        .font(.system(size: 10))
                        .foregroundColor(.blue)
                }
            }
            
            Spacer()
        }
    }
}

// MARK: - Import Options Sheet
struct ImportOptionsSheet: View {
    let selectedFiles: [String]
    @Binding var autoAnalyze: Bool
    @Binding var copyToLibrary: Bool
    let onImport: ([String]) -> Void
    
    @Environment(\.presentationMode) var presentationMode
    
    var body: some View {
        VStack(spacing: 20) {
            Text("Import Options")
                .font(.headline)
            
            Text("Importing \(selectedFiles.count) audio files")
                .font(.subheadline)
                .foregroundColor(.secondary)
            
            VStack(alignment: .leading, spacing: 12) {
                Toggle("Auto-analyze audio properties", isOn: $autoAnalyze)
                    .font(.system(size: 12))
                
                Toggle("Copy files to library", isOn: $copyToLibrary)
                    .font(.system(size: 12))
                
                Text("Auto-analysis will detect BPM, key, and other metadata")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            
            HStack(spacing: 12) {
                Button("Cancel") {
                    presentationMode.wrappedValue.dismiss()
                }
                
                Button("Import") {
                    onImport(selectedFiles)
                    presentationMode.wrappedValue.dismiss()
                }
                .buttonStyle(.borderedProminent)
            }
        }
        .padding()
        .frame(width: 350, height: 250)
    }
}

// Double click extension already defined elsewhere