// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "EtherSynth",
    platforms: [
        .macOS(.v13)
    ],
    products: [
        .executable(
            name: "EtherSynth",
            targets: ["EtherSynth"]
        )
    ],
    dependencies: [
        // Add dependencies here if needed
    ],
    targets: [
        .target(
            name: "CEtherSynth",
            path: "Sources/CEtherSynth",
            publicHeadersPath: "include"
        ),
        .executableTarget(
            name: "EtherSynth",
            dependencies: ["CEtherSynth"],
            path: "Sources/EtherSynth"
        )
    ]
)