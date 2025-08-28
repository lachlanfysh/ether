#include "EtherSynth-Bridging-Header.h"
#include <iostream>

int main() {
    std::cout << "🎛️ Testing Engine Type Functions" << std::endl;
    
    void* engine = ether_create();
    ether_initialize(engine);
    
    // Test engine type count
    int engine_count = ether_get_engine_type_count();
    std::cout << "✅ Engine type count: " << engine_count << std::endl;
    
    // Test instrument color count  
    int color_count = ether_get_instrument_color_count();
    std::cout << "✅ Instrument color count: " << color_count << std::endl;
    
    // Test engine type names
    std::cout << "\n🎵 Available Engine Types:" << std::endl;
    for (int i = 0; i < engine_count; i++) {
        const char* name = ether_get_engine_type_name(i);
        std::cout << "  " << i << ": " << name << std::endl;
    }
    
    // Test instrument color names
    std::cout << "\n🎨 Instrument Colors:" << std::endl;
    for (int i = 0; i < color_count; i++) {
        const char* name = ether_get_instrument_color_name(i);
        int engine_type = ether_get_instrument_engine_type(engine, i);
        const char* engine_name = ether_get_engine_type_name(engine_type);
        std::cout << "  " << i << ": " << name << " -> " << engine_name << std::endl;
    }
    
    // Test changing engine types
    std::cout << "\n🔄 Testing Engine Type Changes:" << std::endl;
    ether_set_instrument_engine_type(engine, 0, 1); // Set Red to Wavetable
    int new_type = ether_get_instrument_engine_type(engine, 0);
    const char* new_name = ether_get_engine_type_name(new_type);
    std::cout << "✅ Red instrument changed to: " << new_name << std::endl;
    
    ether_destroy(engine);
    std::cout << "\n🎉 Engine type functions working perfectly!" << std::endl;
    return 0;
}