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

void playNote(void* synth, int note, int duration_ms) {
    ether_note_on(synth, note, 0.8f, 0.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    ether_note_off(synth, note);
}

int main() {
    std::cout << "🎉🎉🎉 VICTORY SONG - THE BASS ENGINE LIVES! 🎉🎉🎉" << std::endl;
    std::cout << "💕 A celebration for you and your girlfriend! 💕" << std::endl;
    std::cout << "" << std::endl;
    
    // Initialize engine
    void* synth = ether_create();
    if (!synth) {
        std::cout << "❌ Failed to create synth" << std::endl;
        return 1;
    }
    
    ether_initialize(synth);
    ether_play(synth);
    
    // Part 1: Percussion intro with NoiseParticles
    std::cout << "🥁 Percussion intro..." << std::endl;
    ether_set_instrument_engine_type(synth, 0, 7); // NoiseParticles
    ether_set_master_volume(synth, 0.8f);
    
    for (int i = 0; i < 4; i++) {
        playNote(synth, 36, 200);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Part 2: Your bass engine!
    std::cout << "🎸 THE BASS ENGINE - Your baby is ALIVE!" << std::endl;
    ether_set_instrument_engine_type(synth, 0, 14); // SlideAccentBass
    ether_set_master_volume(synth, 1.0f);
    
    // Bass line: C4 - Eb4 - Bb3 - G4 pattern
    int bassPattern[] = {60, 63, 58, 67};
    for (int loop = 0; loop < 4; loop++) {
        for (int note : bassPattern) {
            playNote(synth, note, 400);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Part 3: 4OP FM melody
    std::cout << "🎹 4OP FM melody finale..." << std::endl;
    ether_set_instrument_engine_type(synth, 0, 15); // Classic4OpFM
    ether_set_master_volume(synth, 0.9f);
    
    // Happy melody: C5 - D5 - E5 - C5 - G4 - C5 - F4 - C5
    int melodyPattern[] = {72, 74, 76, 72, 67, 72, 65, 72};
    for (int note : melodyPattern) {
        playNote(synth, note, 500);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "" << std::endl;
    std::cout << "🎊 CELEBRATION COMPLETE! 🎊" << std::endl;
    std::cout << "🎸 Your SlideAccentBass engine is making REAL MUSIC!" << std::endl;
    std::cout << "🎹 Classic4OpFM is singing beautifully!" << std::endl;
    std::cout << "💖 Tell your girlfriend the bass is working! 💖" << std::endl;
    
    ether_stop(synth);
    ether_destroy(synth);
    
    return 0;
}