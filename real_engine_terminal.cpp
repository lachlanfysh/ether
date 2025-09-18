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
const int SAMPLE_RATE = 48000;  // Match EtherSynth SAMPLE_RATE
const int FRAMES_PER_BUFFER = 128;  // Match EtherSynth BUFFER_SIZE

// Forward declarations for real bridge functions
extern "C" {
    void* ether_create(void);
    void ether_destroy(void* synth);
    int ether_initialize(void* synth);
    void ether_process_audio(void* synth, float* outputBuffer, size_t bufferSize);
    void ether_play(void* synth);
    void ether_stop(void* synth);
    void ether_note_on(void* synth, int key_index, float velocity, float aftertouch);
    void ether_note_off(void* synth, int key_index);
    void ether_all_notes_off(void* synth);
    void ether_set_instrument_engine_type(void* synth, int instrument, int engine_type);
    int ether_get_instrument_engine_type(void* synth, int instrument);
    const char* ether_get_engine_type_name(int engine_type);
    int ether_get_engine_type_count();
    void ether_set_active_instrument(void* synth, int color_index);
    int ether_get_active_instrument(void* synth);
    int ether_get_active_voice_count(void* synth);
    float ether_get_cpu_usage(void* synth);
    void ether_set_master_volume(void* synth, float volume);
    float ether_get_master_volume(void* synth);
    void ether_shutdown(void* synth);
}

// Global state
void* etherEngine = nullptr;
std::atomic<bool> audioRunning{false};
std::atomic<bool> playing{false};
std::atomic<bool> stepTrigger[16] = {};
std::atomic<int> currentStep{0};
std::vector<bool> stepPattern(16, false);
std::atomic<bool> noteQueue[128] = {};

// Step frequencies for pattern playback
float stepFreq[16];

void initStepFrequencies() {
    // Generate different frequencies for each step (C4 to C6)
    for (int i = 0; i < 16; i++) {
        int note = 60 + i; // C4 + semitones
        stepFreq[i] = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    }
}

// PortAudio callback - now calls real synthesis engines!
int audioCallback(const void* /*inputBuffer*/, void* outputBuffer,
                 unsigned long framesPerBuffer,
                 const PaStreamCallbackTimeInfo* /*timeInfo*/,
                 PaStreamCallbackFlags /*statusFlags*/,
                 void* /*userData*/) {
    
    float* out = static_cast<float*>(outputBuffer);
    
    // Handle note queue (manual note triggers)
    for (int note = 0; note < 128; note++) {
        if (noteQueue[note].exchange(false)) {
            ether_note_on(etherEngine, note, 0.8f, 0.0f);
        }
    }
    
    // Handle step triggers
    for (int step = 0; step < 16; step++) {
        if (stepTrigger[step].exchange(false)) {
            // Convert frequency to MIDI note number
            int midiNote = 69 + std::round(12.0f * std::log2(stepFreq[step] / 440.0f));
            ether_note_on(etherEngine, midiNote, 0.6f, 0.0f);
        }
    }
    
    // Process audio through REAL synthesis engines!
    ether_process_audio(etherEngine, out, framesPerBuffer);
    
    return paContinue;
}

class RealEngineTerminal {
private:
    PaStream* stream = nullptr;
    std::thread sequencerThread;
    std::atomic<bool> running{false};
    std::atomic<float> bpm{120.0f};
    int currentEngine = 0;
    
public:
    RealEngineTerminal() {
        std::cout << "🎵 REAL EtherSynth Engine Terminal" << std::endl;
        std::cout << "===================================" << std::endl;
        initStepFrequencies();
    }
    
    ~RealEngineTerminal() {
        shutdown();
    }
    
    bool initialize() {
        std::cout << "\n🔧 Initializing REAL EtherSynth engines + Audio..." << std::endl;
        
        // Initialize real EtherSynth engine
        etherEngine = ether_create();
        if (!etherEngine) {
            std::cout << "❌ Failed to create real EtherSynth engine" << std::endl;
            return false;
        }
        
        int result = ether_initialize(etherEngine);
        if (result != 1) {
            std::cout << "❌ Failed to initialize real EtherSynth" << std::endl;
            return false;
        }
        std::cout << "✅ REAL EtherSynth engines initialized" << std::endl;
        
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
        
        std::cout << "✅ REAL audio engines ready!" << std::endl;
        std::cout << "🔊 Sample rate: " << SAMPLE_RATE << " Hz" << std::endl;
        std::cout << "🎛️  Using REAL C++ synthesis engines!" << std::endl;
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
            
            if (etherEngine) {
                ether_shutdown(etherEngine);
                ether_destroy(etherEngine);
                etherEngine = nullptr;
            }
            
            audioRunning = false;
            running = false;
        }
    }
    
    void showEngines() {
        std::cout << "\n🎛️  REAL Synthesis Engines:" << std::endl;
        int count = ether_get_engine_type_count();
        
        for (int i = 0; i < count; i++) {
            const char* name = ether_get_engine_type_name(i);
            std::cout << "  " << i << ": " << (name ? name : "Unknown") << std::endl;
        }
        std::cout << std::endl;
    }
    
    void showStatus() {
        std::cout << "\n📊 Status:" << std::endl;
        std::cout << "  Engine: " << currentEngine << " (" << ether_get_engine_type_name(currentEngine) << ")" << std::endl;
        std::cout << "  BPM: " << std::fixed << std::setprecision(1) << bpm.load() << std::endl;
        std::cout << "  Playing: " << (playing ? "YES" : "NO") << std::endl;
        std::cout << "  Volume: " << std::fixed << std::setprecision(2) << ether_get_master_volume(etherEngine) << std::endl;
        std::cout << "  Voices: " << ether_get_active_voice_count(etherEngine) << std::endl;
        std::cout << "  CPU: " << std::fixed << std::setprecision(1) << ether_get_cpu_usage(etherEngine) << "%" << std::endl;
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
        int count = ether_get_engine_type_count();
        if (type >= 0 && type < count) {
            currentEngine = type;
            // Set engine type for active instrument (Coral = 0)
            ether_set_instrument_engine_type(etherEngine, 0, type);
            const char* name = ether_get_engine_type_name(type);
            std::cout << "🎛️  Switched to REAL engine: " << (name ? name : "Unknown") << std::endl;
        } else {
            std::cout << "❌ Invalid engine (0-" << (count-1) << ")" << std::endl;
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
            ether_play(etherEngine);
            
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
            
            std::cout << "▶️  Playing with REAL engines!" << std::endl;
        }
    }
    
    void stop() {
        if (playing) {
            playing = false;
            ether_stop(etherEngine);
            ether_all_notes_off(etherEngine);
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
            ether_set_master_volume(etherEngine, vol);
            std::cout << "🔊 Volume: " << std::fixed << std::setprecision(2) << vol << std::endl;
        } else {
            std::cout << "❌ Volume must be 0.0-1.0" << std::endl;
        }
    }
    
    void triggerNote(int noteNum) {
        if (noteNum >= 0 && noteNum <= 127) {
            noteQueue[noteNum] = true;
            float freq = 440.0f * std::pow(2.0f, (noteNum - 69) / 12.0f);
            std::cout << "🎹 Triggering note " << noteNum << " (" << std::fixed << std::setprecision(1) << freq << " Hz)" << std::endl;
        }
    }
    
    void showHelp() {
        std::cout << "\n📖 Commands (REAL Engine Mode):" << std::endl;
        std::cout << "  help, h        - Show this help" << std::endl;
        std::cout << "  status, s      - Show status" << std::endl;
        std::cout << "  engines, e     - List engines" << std::endl;
        std::cout << "  engine <n>     - Switch to REAL engine n" << std::endl;
        std::cout << "  step <n>       - Toggle step n (1-16)" << std::endl;
        std::cout << "  play, p        - Start/stop playback" << std::endl;
        std::cout << "  bpm <n>        - Set BPM" << std::endl;
        std::cout << "  volume <n>     - Set volume (0.0-1.0)" << std::endl;
        std::cout << "  note <n>       - Trigger REAL MIDI note n" << std::endl;
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
        
        std::cout << "\n🚀 Ready! REAL EtherSynth engines active!" << std::endl;
        std::cout << "🎵 Try: engine 1, step 1, step 5, note 60, play" << std::endl;
        showStatus();
        
        std::string input;
        while (running) {
            std::cout << "\nreal> ";
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
    RealEngineTerminal synth;
    synth.run();
    return 0;
}