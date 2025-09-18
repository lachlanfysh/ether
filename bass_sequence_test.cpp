#include <iostream>
#include <thread>
#include <chrono>

// Forward declarations for bridge
extern "C" {
    void* ether_create(void);
    void ether_destroy(void* synth);
    int ether_initialize(void* synth);
    void ether_note_on(void* synth, int key_index, float velocity, float aftertouch);
    void ether_note_off(void* synth, int key_index);
    void ether_set_instrument_engine_type(void* synth, int instrument, int engine_type);
    void ether_play(void* synth);
    void ether_stop(void* synth);
    void ether_set_master_volume(void* synth, float volume);
}

int main() {
    std::cout << "🎸 Direct Bass Sequence Test" << std::endl;
    
    // Initialize engine
    void* synth = ether_create();
    if (!synth) {
        std::cout << "❌ Failed to create synth" << std::endl;
        return 1;
    }
    
    ether_initialize(synth);
    ether_set_instrument_engine_type(synth, 0, 14); // SlideAccentBass
    ether_set_master_volume(synth, 1.0f);
    ether_play(synth);
    
    std::cout << "🎵 Playing bass sequence..." << std::endl;
    
    // Play a simple bass pattern
    int pattern[] = {60, 63, 58, 67, 60, 63, 58, 67}; // C4, Eb4, Bb3, G4
    
    for (int loop = 0; loop < 2; loop++) {
        std::cout << "Loop " << (loop + 1) << std::endl;
        
        for (int i = 0; i < 8; i++) {
            // Note on
            ether_note_on(synth, pattern[i], 0.8f, 0.0f);
            std::cout << "🎵 Note " << pattern[i] << std::endl;
            
            // Let note play for 200ms
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            
            // Note off
            ether_note_off(synth, pattern[i]);
            
            // Gap between notes
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    ether_stop(synth);
    ether_destroy(synth);
    
    std::cout << "🎵 Test complete!" << std::endl;
    return 0;
}