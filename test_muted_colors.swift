import Foundation

// Bridge function declarations
@_silgen_name("ether_create")
func ether_create() -> UnsafeMutableRawPointer?

@_silgen_name("ether_initialize")  
func ether_initialize(_ synth: UnsafeMutableRawPointer) -> Int32

@_silgen_name("ether_destroy")
func ether_destroy(_ synth: UnsafeMutableRawPointer)

@_silgen_name("ether_get_instrument_color_name")
func ether_get_instrument_color_name(_ color_index: Int32) -> UnsafePointer<CChar>?

@_silgen_name("ether_get_engine_type_name")
func ether_get_engine_type_name(_ engine_type: Int32) -> UnsafePointer<CChar>?

@_silgen_name("ether_get_instrument_engine_type")
func ether_get_instrument_engine_type(_ synth: UnsafeMutableRawPointer, _ instrument: Int32) -> Int32

print("🎨 Testing Muted Color Mapping")

guard let engine = ether_create() else {
    print("❌ Failed to create engine")
    exit(1)
}

ether_initialize(engine)

print("\n🎨 Instrument Color Names:")
for i in 0..<8 {
    if let cString = ether_get_instrument_color_name(Int32(i)) {
        let colorName = String(cString: cString)
        let engineType = ether_get_instrument_engine_type(engine, Int32(i))
        if let engineCString = ether_get_engine_type_name(engineType) {
            let engineName = String(cString: engineCString)
            print("  \(i): \(colorName) -> \(engineName)")
        }
    }
}

ether_destroy(engine)
print("\n🎉 Color mapping test complete!")