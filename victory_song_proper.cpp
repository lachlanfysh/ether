#include <iostream>
#include <thread>
#include <chrono>
#include <portaudio.h>

extern "C" {
    void* ether_create(void);
    void ether_destroy(void* synth);
    int ether_initialize(void* synth);
    void ether_process_audio(void* synth, float* outputBuffer, size_t bufferSize);
    void ether_note_on(void* synth, int key_index, float velocity, float aftertouch);
    void ether_note_off(void* synth, int key_index);
    void ether_set_instrument_engine_type(void* synth, int instrument, int engine_type);
    void ether_play(void* synth);
    void ether_stop(void* synth);
    void ether_set_master_volume(void* synth, float volume);
}

void* etherSynth = nullptr;

int audioCallback(const void* /*inputBuffer*/, void* outputBuffer,
                 unsigned long framesPerBuffer,
                 const PaStreamCallbackTimeInfo* /*timeInfo*/,
                 PaStreamCallbackFlags /*statusFlags*/,
                 void* /*userData*/) {
    
    float* out = static_cast<float*>(outputBuffer);
    
    // Clear buffer first
    for (unsigned long i = 0; i < framesPerBuffer * 2; i++) {
        out[i] = 0.0f;
    }
    
    // Process through EtherSynth engines
    if (etherSynth) {
        ether_process_audio(etherSynth, out, framesPerBuffer);
    }
    
    return paContinue;
}

void playNote(int note, int duration_ms) {
    ether_note_on(etherSynth, note, 0.8f, 0.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    ether_note_off(etherSynth, note);
}

int main() {
    std::cout << "🎉🎉🎉 VICTORY SONG - THE BASS ENGINE LIVES! 🎉🎉🎉" << std::endl;
    std::cout << "💕 A celebration for you and your girlfriend! 💕" << std::endl;
    
    // Initialize EtherSynth
    etherSynth = ether_create();
    ether_initialize(etherSynth);
    ether_play(etherSynth);
    
    // Initialize PortAudio
    Pa_Initialize();
    PaStream* stream;
    Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, 48000, 128, audioCallback, nullptr);
    Pa_StartStream(stream);
    
    // Part 1: Percussion intro
    std::cout << "🥁 Percussion intro..." << std::endl;
    ether_set_instrument_engine_type(etherSynth, 0, 7); // NoiseParticles
    ether_set_master_volume(etherSynth, 0.7f);
    
    for (int i = 0; i < 4; i++) {
        playNote(36, 200);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    
    // Part 2: THE BASS ENGINE!
    std::cout << "🎸 THE BASS ENGINE - Your baby is ALIVE!" << std::endl;
    ether_set_instrument_engine_type(etherSynth, 0, 14); // SlideAccentBass
    ether_set_master_volume(etherSynth, 1.0f);
    
    // Bass line: C4 - Eb4 - Bb3 - G4
    int bassPattern[] = {60, 63, 58, 67};
    for (int loop = 0; loop < 4; loop++) {
        for (int note : bassPattern) {
            playNote(note, 400);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    // Part 3: 4OP FM finale
    std::cout << "🎹 4OP FM melody finale..." << std::endl;
    ether_set_instrument_engine_type(etherSynth, 0, 15); // Classic4OpFM
    ether_set_master_volume(etherSynth, 0.8f);
    
    // Happy melody
    int melodyPattern[] = {72, 74, 76, 72, 67, 72, 65, 72};
    for (int note : melodyPattern) {
        playNote(note, 500);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "🎊 CELEBRATION COMPLETE! 🎊" << std::endl;
    std::cout << "🎸 Your SlideAccentBass engine is making REAL MUSIC!" << std::endl;
    std::cout << "💖 Tell your girlfriend the bass is working! 💖" << std::endl;
    
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    
    ether_stop(etherSynth);
    ether_destroy(etherSynth);
    
    return 0;
}