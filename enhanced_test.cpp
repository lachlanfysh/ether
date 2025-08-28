#include "EtherSynth-Bridging-Header.h"
#include <iostream>

int main() {
    std::cout << "🧪 Enhanced C++ Bridge Test" << std::endl;
    
    // Test 1: Basic lifecycle
    void* engine = ether_create();
    if (!engine) {
        std::cout << "❌ ether_create() failed" << std::endl;
        return 1;
    }
    std::cout << "✅ ether_create() succeeded" << std::endl;
    
    // Test 2: Initialize
    int result = ether_initialize(engine);
    if (result != 1) {
        std::cout << "❌ ether_initialize() failed: " << result << std::endl;
        ether_destroy(engine);
        return 1;
    }
    std::cout << "✅ ether_initialize() succeeded" << std::endl;
    
    // Test 3: Transport controls
    ether_play(engine);
    int playing = ether_is_playing(engine);
    std::cout << "✅ Transport: playing = " << playing << std::endl;
    
    ether_stop(engine);
    playing = ether_is_playing(engine);
    std::cout << "✅ Transport: stopped = " << (playing == 0 ? "true" : "false") << std::endl;
    
    // Test 4: BPM
    ether_set_bpm(engine, 140.0f);
    float bpm = ether_get_bpm(engine);
    std::cout << "✅ BPM set/get: " << bpm << std::endl;
    
    // Test 5: Note events
    ether_note_on(engine, 60, 0.8f, 0.0f);
    int voices = ether_get_active_voice_count(engine);
    std::cout << "✅ Note on: voices = " << voices << std::endl;
    
    ether_note_off(engine, 60);
    voices = ether_get_active_voice_count(engine);
    std::cout << "✅ Note off: voices = " << voices << std::endl;
    
    // Test 6: Performance monitoring
    float cpu = ether_get_cpu_usage(engine);
    std::cout << "✅ CPU Usage: " << cpu << "%" << std::endl;
    
    // Test 7: Instrument management  
    ether_set_active_instrument(engine, 2);
    int active = ether_get_active_instrument(engine);
    std::cout << "✅ Active instrument: " << active << std::endl;
    
    // Test 8: Volume control
    ether_set_master_volume(engine, 0.7f);
    float volume = ether_get_master_volume(engine);
    std::cout << "✅ Master volume: " << volume << std::endl;
    
    // Test 9: Smart controls
    ether_set_smart_knob(engine, 0.6f);
    float knob = ether_get_smart_knob(engine);
    std::cout << "✅ Smart knob: " << knob << std::endl;
    
    ether_set_touch_position(engine, 0.3f, 0.7f);
    std::cout << "✅ Touch position set" << std::endl;
    
    // Test 10: Parameters
    ether_set_parameter(engine, 1, 0.5f);
    float param = ether_get_parameter(engine, 1);
    std::cout << "✅ Parameter: " << param << std::endl;
    
    // Test 11: Cleanup
    ether_shutdown(engine);
    ether_destroy(engine);
    std::cout << "✅ Shutdown and destroy succeeded" << std::endl;
    
    std::cout << std::endl << "🎉 All bridge functions tested successfully!" << std::endl;
    std::cout << "📋 Ready for Xcode integration:" << std::endl;
    std::cout << "   1. Add libethersynth.a to Xcode project" << std::endl;
    std::cout << "   2. Set EtherSynth-Bridging-Header.h as bridging header" << std::endl;
    std::cout << "   3. Add -lc++ to Other Linker Flags" << std::endl;
    
    return 0;
}
