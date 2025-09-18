#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <portaudio.h>

// Audio constants
const int SAMPLE_RATE = 44100;
const int FRAMES_PER_BUFFER = 256;

// Global audio state
std::atomic<bool> audioRunning{false};
std::atomic<bool> playing{false};
std::atomic<float> volume{0.5f};
std::atomic<float> frequency{440.0f};
std::atomic<bool> stepTrigger[16] = {};
std::atomic<int> currentStep{0};
std::vector<bool> stepPattern(16, false);

// Simple oscillator state
struct Oscillator {
    float phase = 0.0f;
    float frequency = 440.0f;
    float amplitude = 0.0f;
    
    float process() {
        float output = std::sin(phase) * amplitude;
        phase += 2.0f * M_PI * frequency / SAMPLE_RATE;
        if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        return output;
    }
};

Oscillator osc;

// PortAudio callback
int audioCallback(const void* /*inputBuffer*/, void* outputBuffer,
                 unsigned long framesPerBuffer,
                 const PaStreamCallbackTimeInfo* /*timeInfo*/,
                 PaStreamCallbackFlags /*statusFlags*/,
                 void* /*userData*/) {
    
    float* out = static_cast<float*>(outputBuffer);
    
    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        float sample = 0.0f;
        
        // Check for step triggers
        for (int step = 0; step < 16; step++) {
            if (stepTrigger[step].exchange(false)) {
                // Trigger note for this step
                osc.frequency = 220.0f + (step * 50.0f); // Different frequencies per step
                osc.amplitude = 0.3f;
            }
        }
        
        // Generate audio
        if (playing) {
            sample = osc.process();
            osc.amplitude *= 0.9995f; // Decay
        }
        
        sample *= volume.load();
        
        // Stereo output
        *out++ = sample;
        *out++ = sample;
    }
    
    return paContinue;
}

class AudioTerminalSynth {
private:
    PaStream* stream = nullptr;
    std::thread sequencerThread;
    std::atomic<bool> running{false};
    std::atomic<float> bpm{120.0f};
    int engineType = 0;
    
public:
    AudioTerminalSynth() {
        std::cout << "🎵 Audio Terminal EtherSynth" << std::endl;
        std::cout << "============================" << std::endl;
    }
    
    ~AudioTerminalSynth() {
        shutdown();
    }
    
    bool initialize() {
        std::cout << "\n🔧 Initializing audio..." << std::endl;
        
        // Initialize PortAudio
        PaError err = Pa_Initialize();
        if (err != paNoError) {
            std::cout << "❌ PortAudio init failed: " << Pa_GetErrorText(err) << std::endl;
            return false;
        }
        
        // Open audio stream
        err = Pa_OpenDefaultStream(&stream,
                                  0,          // input channels
                                  2,          // output channels (stereo)
                                  paFloat32,  // sample format
                                  SAMPLE_RATE,
                                  FRAMES_PER_BUFFER,
                                  audioCallback,
                                  nullptr);   // user data
        
        if (err != paNoError) {
            std::cout << "❌ Failed to open stream: " << Pa_GetErrorText(err) << std::endl;
            return false;
        }
        
        // Start audio stream
        err = Pa_StartStream(stream);
        if (err != paNoError) {
            std::cout << "❌ Failed to start stream: " << Pa_GetErrorText(err) << std::endl;
            return false;
        }
        
        audioRunning = true;
        running = true;
        
        std::cout << "✅ Audio initialized successfully!" << std::endl;
        std::cout << "🔊 Sample rate: " << SAMPLE_RATE << " Hz" << std::endl;
        showEngines();
        return true;
    }
    
    void shutdown() {
        if (running) {
            std::cout << "\n🛑 Shutting down..." << std::endl;
            stop();
            
            if (sequencerThread.joinable()) {
                sequencerThread.join();
            }
            
            if (stream) {
                Pa_CloseStream(stream);
                stream = nullptr;
            }
            
            Pa_Terminate();
            audioRunning = false;
            running = false;
        }
    }
    
    void showEngines() {
        std::cout << "\n🎛️  Available Engines:" << std::endl;
        std::vector<std::string> engines = {
            "MacroVA", "MacroFM", "MacroWaveshaper", "MacroWavetable",
            "MacroChord", "MacroHarmonics", "FormantVocal", "NoiseParticles",
            "TidesOsc", "RingsVoice", "ElementsVoice", "DrumKit",
            "SamplerKit", "SamplerSlicer"
        };
        
        for (size_t i = 0; i < engines.size(); i++) {
            std::cout << "  " << i << ": " << engines[i] << std::endl;
        }
        std::cout << std::endl;
    }
    
    void showStatus() {
        std::cout << "\n📊 Status:" << std::endl;
        std::cout << "  Engine: " << engineType << std::endl;
        std::cout << "  BPM: " << std::fixed << std::setprecision(1) << bpm.load() << std::endl;
        std::cout << "  Playing: " << (playing ? "YES" : "NO") << std::endl;
        std::cout << "  Volume: " << std::fixed << std::setprecision(2) << volume.load() << std::endl;
        std::cout << "  Audio: " << (audioRunning ? "RUNNING" : "STOPPED") << std::endl;
        
        // Show pattern
        std::cout << "\n🎵 Pattern: ";
        for (int i = 0; i < 16; i++) {
            if (i == currentStep && playing) {
                std::cout << (stepPattern[i] ? "[●]" : "[ ]");
            } else {
                std::cout << (stepPattern[i] ? " ● " : " ○ ");
            }
        }
        std::cout << std::endl;
    }
    
    void setEngine(int type) {
        if (type >= 0 && type < 14) {
            engineType = type;
            std::cout << "🎛️  Switched to engine " << type << std::endl;
        } else {
            std::cout << "❌ Invalid engine (0-13)" << std::endl;
        }
    }
    
    void toggleStep(int step) {
        if (step >= 0 && step < 16) {
            stepPattern[step] = !stepPattern[step];
            std::cout << "🎵 Step " << (step + 1) << ": " << (stepPattern[step] ? "ON" : "OFF") << std::endl;
        }
    }
    
    void play() {
        if (!playing) {
            playing = true;
            currentStep = 0;
            
            // Start sequencer thread
            sequencerThread = std::thread([this]() {
                while (playing) {
                    // Trigger active steps
                    if (stepPattern[currentStep]) {
                        stepTrigger[currentStep] = true;
                        std::cout << "🎵 Step " << (currentStep + 1) << " triggered" << std::endl;
                    }
                    
                    currentStep = (currentStep + 1) % 16;
                    
                    // Calculate step timing from BPM (16th notes)
                    float stepMs = (60.0f / bpm) / 4.0f * 1000.0f;
                    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(stepMs)));
                }
            });
            
            std::cout << "▶️  Playing with audio!" << std::endl;
        }
    }
    
    void stop() {
        if (playing) {
            playing = false;
            if (sequencerThread.joinable()) {
                sequencerThread.join();
            }
            std::cout << "⏹️  Stopped" << std::endl;
        }
    }
    
    void setBPM(float newBpm) {
        if (newBpm >= 60.0f && newBpm <= 200.0f) {
            bpm = newBpm;
            std::cout << "🥁 BPM: " << std::fixed << std::setprecision(1) << newBpm << std::endl;
        } else {
            std::cout << "❌ BPM must be 60-200" << std::endl;
        }
    }
    
    void setVolume(float vol) {
        if (vol >= 0.0f && vol <= 1.0f) {
            volume = vol;
            std::cout << "🔊 Volume: " << std::fixed << std::setprecision(2) << vol << std::endl;
        } else {
            std::cout << "❌ Volume must be 0.0-1.0" << std::endl;
        }
    }
    
    void triggerNote(int noteNum) {
        if (noteNum >= 0 && noteNum <= 127) {
            osc.frequency = 440.0f * std::pow(2.0f, (noteNum - 69) / 12.0f);
            osc.amplitude = 0.5f;
            std::cout << "🎹 Note " << noteNum << " (" << std::fixed << std::setprecision(1) << osc.frequency << " Hz)" << std::endl;
        }
    }
    
    void showHelp() {
        std::cout << "\n📖 Commands:" << std::endl;
        std::cout << "  help, h        - Show this help" << std::endl;
        std::cout << "  status, s      - Show status" << std::endl;
        std::cout << "  engines, e     - List engines" << std::endl;
        std::cout << "  engine <n>     - Switch to engine n" << std::endl;
        std::cout << "  step <n>       - Toggle step n (1-16)" << std::endl;
        std::cout << "  play, p        - Start/stop playback" << std::endl;
        std::cout << "  bpm <n>        - Set BPM" << std::endl;
        std::cout << "  volume <n>     - Set volume (0.0-1.0)" << std::endl;
        std::cout << "  note <n>       - Trigger MIDI note n" << std::endl;
        std::cout << "  clear          - Clear pattern" << std::endl;
        std::cout << "  fill           - Fill pattern" << std::endl;
        std::cout << "  quit, q        - Exit" << std::endl;
        std::cout << std::endl;
    }
    
    void run() {
        if (!initialize()) {
            std::cout << "❌ Failed to initialize" << std::endl;
            return;
        }
        
        std::cout << "\n🚀 Ready! Type 'help' for commands" << std::endl;
        std::cout << "🎵 Try: step 1, step 5, step 9, step 13, then 'play'!" << std::endl;
        showStatus();
        
        std::string input;
        while (running) {
            std::cout << "\naudio> ";
            if (!std::getline(std::cin, input)) break;
            
            if (input.empty()) continue;
            
            std::istringstream iss(input);
            std::string command;
            iss >> command;
            
            if (command == "quit" || command == "q") {
                break;
            }
            else if (command == "help" || command == "h") {
                showHelp();
            }
            else if (command == "status" || command == "s") {
                showStatus();
            }
            else if (command == "engines" || command == "e") {
                showEngines();
            }
            else if (command == "engine") {
                int n;
                if (iss >> n) {
                    setEngine(n);
                } else {
                    std::cout << "❌ Usage: engine <number>" << std::endl;
                }
            }
            else if (command == "step") {
                int n;
                if (iss >> n) {
                    toggleStep(n - 1);
                } else {
                    std::cout << "❌ Usage: step <1-16>" << std::endl;
                }
            }
            else if (command == "play" || command == "p") {
                if (playing) {
                    stop();
                } else {
                    play();
                }
            }
            else if (command == "bpm") {
                float bpmVal;
                if (iss >> bpmVal) {
                    setBPM(bpmVal);
                } else {
                    std::cout << "❌ Usage: bpm <60-200>" << std::endl;
                }
            }
            else if (command == "volume") {
                float vol;
                if (iss >> vol) {
                    setVolume(vol);
                } else {
                    std::cout << "❌ Usage: volume <0.0-1.0>" << std::endl;
                }
            }
            else if (command == "note") {
                int note;
                if (iss >> note) {
                    triggerNote(note);
                } else {
                    std::cout << "❌ Usage: note <0-127>" << std::endl;
                }
            }
            else if (command == "clear") {
                std::fill(stepPattern.begin(), stepPattern.end(), false);
                std::cout << "🧹 Pattern cleared" << std::endl;
            }
            else if (command == "fill") {
                std::fill(stepPattern.begin(), stepPattern.end(), true);
                std::cout << "✨ Pattern filled" << std::endl;
            }
            else {
                std::cout << "❌ Unknown command. Type 'help'" << std::endl;
            }
        }
        
        std::cout << "\n👋 Goodbye!" << std::endl;
    }
};

int main() {
    AudioTerminalSynth synth;
    synth.run();
    return 0;
}