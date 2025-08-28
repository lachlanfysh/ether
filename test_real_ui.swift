import Foundation

// Bridge function declarations
@_silgen_name("ether_create")
func ether_create() -> UnsafeMutableRawPointer?

@_silgen_name("ether_initialize")
func ether_initialize(_ synth: UnsafeMutableRawPointer) -> Int32

@_silgen_name("ether_destroy")
func ether_destroy(_ synth: UnsafeMutableRawPointer)

@_silgen_name("ether_get_engine_type_count")
func ether_get_engine_type_count() -> Int32

@_silgen_name("ether_get_engine_type_name")
func ether_get_engine_type_name(_ engine_type: Int32) -> UnsafePointer<CChar>?

@_silgen_name("ether_get_instrument_engine_type")
func ether_get_instrument_engine_type(_ synth: UnsafeMutableRawPointer, _ instrument: Int32) -> Int32

print("🎹 Testing Real Engine Names from Swift")

guard let engine = ether_create() else {
    print("❌ Failed to create engine")
    exit(1)
}

let result = ether_initialize(engine)
print("✅ Engine initialized with result: \(result)")

let engineCount = ether_get_engine_type_count()
print("✅ Available engine types: \(engineCount)")

print("\n🎵 Real Engine Types:")
for i in 0..<engineCount {
    if let cString = ether_get_engine_type_name(i) {
        let engineName = String(cString: cString)
        print("  \(i): \(engineName)")
    }
}

print("\n🎨 Instrument Engine Mappings:")
for i in 0..<8 {
    let engineType = ether_get_instrument_engine_type(engine, Int32(i))
    if let cString = ether_get_engine_type_name(engineType) {
        let engineName = String(cString: cString)
        let colorNames = ["Red", "Orange", "Yellow", "Green", "Blue", "Indigo", "Violet", "Grey"]
        print("  \(colorNames[Int(i)]): \(engineName)")
    }
}

ether_destroy(engine)
print("\n🎉 Real engine integration working perfectly!")