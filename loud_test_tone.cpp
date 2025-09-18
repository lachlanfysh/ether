#include <iostream>
#include <portaudio.h>
#include <cmath>

float phase = 0.0f;

int loudCallback(const void* inputBuffer, void* outputBuffer,
                unsigned long framesPerBuffer,
                const PaStreamCallbackTimeInfo* timeInfo,
                PaStreamCallbackFlags statusFlags,
                void* userData) {
    
    float* out = static_cast<float*>(outputBuffer);
    
    // Generate LOUD 440Hz sine wave
    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        float sample = 0.8f * sin(phase);  // Much louder
        out[i * 2] = sample;     // Left
        out[i * 2 + 1] = sample; // Right
        
        phase += 2.0f * M_PI * 440.0f / 48000.0f;
        if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
    }
    
    return paContinue;
}

int main() {
    std::cout << "Playing LOUD 440Hz test tone..." << std::endl;
    
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cout << "PortAudio init failed" << std::endl;
        return 1;
    }
    
    PaStream* stream;
    err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, 48000, 128, loudCallback, nullptr);
    
    if (err == paNoError) {
        Pa_StartStream(stream);
        Pa_Sleep(3000); // 3 seconds
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
    }
    
    Pa_Terminate();
    std::cout << "Test complete - did you hear it?" << std::endl;
    return 0;
}