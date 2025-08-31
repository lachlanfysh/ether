// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "EtherSynth",
    platforms: [
        .macOS(.v14)
    ],
    products: [
        .executable(
            name: "EtherSynth",
            targets: ["EtherSynth"]
        )
    ],
    targets: [
        .target(
            name: "EtherBridge",
            path: ".",
            sources: [
                "EtherSynthBridge.cpp",
                
                // Core System
                "src/core/EtherSynth.cpp",
                "src/audio/AudioEngine.cpp",
                "src/audio/VoiceManager.cpp",
                
                // Missing classes causing linker errors
                "src/sequencer/Timeline.cpp",
                "src/sequencer/EuclideanRhythm.cpp",
                "src/synthesis/DSPUtils.cpp",
                "src/platform/hardware/HardwareInterface.cpp",
                "src/platform/hardware/MacHardware.cpp",
                
                // Synthesis Engines
                "src/engines/EngineParameterLayouts.cpp",
                "src/synthesis/BaseEngine.cpp",
                "src/synthesis/SynthEngine.cpp",
                "src/synthesis/EngineFactory.cpp",
                
                // Basic Engines
                "src/synthesis/SubtractiveEngine.cpp",
                "src/synthesis/WavetableEngine.cpp", 
                "src/synthesis/FMEngine.cpp",
                "src/synthesis/GranularEngine.cpp",
                
                // Minimal working engines for now
                // TODO: Add back other engines after fixing compilation issues
                
                // Instruments & Audio Processing
                "src/instruments/InstrumentSlot.cpp",
                "src/audio/DCBlocker.cpp",
                "src/audio/LUFSNormalizer.cpp",
                "src/control/modulation/ModulationMatrix.cpp",
                "src/processing/effects/EffectsChain.cpp"
            ],
            publicHeadersPath: ".",
            cxxSettings: [
                .headerSearchPath("src"),
                .headerSearchPath("src/core"),
                .headerSearchPath("src/audio"),
                .headerSearchPath("src/engines"),
                .headerSearchPath("src/synthesis"),
                .headerSearchPath("src/instruments"),
                .headerSearchPath("src/control/modulation"),
                .headerSearchPath("src/processing/effects"),
                .headerSearchPath("src/sequencer"),
                .headerSearchPath("src/platform/hardware"),
                .define("PLATFORM_MAC"),
                .unsafeFlags(["-std=c++17", "-I/opt/homebrew/include"])
            ],
            linkerSettings: [
                .linkedLibrary("portaudio"),
                .linkedFramework("CoreMIDI"),
                .linkedFramework("CoreFoundation"),
                .unsafeFlags(["-L/opt/homebrew/lib"])
            ]
        ),
        .executableTarget(
            name: "EtherSynth",
            dependencies: ["EtherBridge"],
            path: "Sources/EtherSynth",
            sources: [
                "EtherSynthApp.swift",
                "EtherSynthMainView.swift", 
                "PatternScreenView.swift",
                "ScreenViews.swift",
                "OverlayViews.swift",
                "SynthController.swift",
                "SynthControllerExtensions.swift",
                "AudioEngineConnector.swift",
                "RealDataModels.swift"
            ]
        )
    ]
)