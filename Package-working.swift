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
        .executableTarget(
            name: "EtherSynth",
            path: "Sources/EtherSynth",
            sources: [
                "EtherSynthApp.swift",
                "EtherSynthMainView.swift", 
                "PatternScreenView.swift"
            ]
        )
    ]
)