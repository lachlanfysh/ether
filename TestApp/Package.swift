// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "EtherSynthTestApp",
    platforms: [
        .macOS(.v12)
    ],
    products: [
        .executable(name: "EtherSynthTestApp", targets: ["EtherSynthTestApp"])
    ],
    targets: [
        .executableTarget(
            name: "EtherSynthTestApp",
            dependencies: []
        )
    ]
)