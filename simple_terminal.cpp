#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <sstream>
#include <iomanip>

// Use the existing working bridge
extern "C" {
    void* ether_create();
    void ether_destroy(void* engine);
    int ether_initialize(void* engine);
    void ether_shutdown(void* engine);
    void ether_play(void* engine);
    void ether_stop(void* engine);
    void ether_note_on(void* engine, int note, float velocity, float aftertouch);
    void ether_note_off(void* engine, int note);
    void ether_all_notes_off(void* engine);
    int ether_get_engine_type_count();
    const char* ether_get_engine_type_name(int engine_type);
    void ether_set_instrument_engine_type(void* engine, int instrument, int engine_type);
    int ether_get_instrument_engine_type(void* engine, int instrument);
    float ether_get_bpm(void* engine);
    void ether_set_bpm(void* engine, float bpm);
    int ether_get_active_voice_count(void* engine);
    float ether_get_cpu_usage(void* engine);
}

class SimpleTerminalSynth {
private:
    void* engine = nullptr;
    std::atomic<bool> running{false};
    std::atomic<bool> playing{false};
    int currentInstrument = 0;
    int currentEngineType = 0;
    float currentBPM = 120.0f;
    std::vector<bool> stepPattern;
    int currentStep = 0;
    std::thread sequencerThread;
    
public:
    SimpleTerminalSynth() : stepPattern(16, false) {
        std::cout << "🎵 Simple Terminal EtherSynth" << std::endl;
        std::cout << "=============================" << std::endl;
    }
    
    ~SimpleTerminalSynth() {
        shutdown();
    }
    
    bool initialize() {
        std::cout << "\n🔧 Initializing..." << std::endl;
        
        // Create engine
        engine = ether_create();
        if (!engine) {
            std::cout << "❌ Failed to create engine" << std::endl;
            return false;
        }
        std::cout << "✅ Engine created" << std::endl;
        
        // Initialize engine
        int result = ether_initialize(engine);
        if (result != 1) {
            std::cout << "❌ Failed to initialize engine (result: " << result << ")" << std::endl;
            return false;
        }
        std::cout << "✅ Engine initialized" << std::endl;
        
        running = true;
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
            if (engine) {
                ether_shutdown(engine);
                ether_destroy(engine);
                engine = nullptr;
            }
            running = false;
        }
    }
    
    void showEngines() {
        std::cout << "\n🎛️  Available Engines:" << std::endl;
        int count = ether_get_engine_type_count();
        for (int i = 0; i < count; i++) {
            const char* name = ether_get_engine_type_name(i);
            std::cout << "  " << i << ": " << (name ? name : "Unknown") << std::endl;
        }
        std::cout << std::endl;
    }
    
    void showStatus() {
        if (!engine) return;
        
        std::cout << "\n📊 Status:" << std::endl;
        const char* engineName = ether_get_engine_type_name(currentEngineType);
        std::cout << "  Engine: " << (engineName ? engineName : "Unknown") << std::endl;
        std::cout << "  Instrument: " << currentInstrument << std::endl;
        std::cout << "  BPM: " << std::fixed << std::setprecision(1) << currentBPM << std::endl;
        std::cout << "  Playing: " << (playing ? "YES" : "NO") << std::endl;
        std::cout << "  Voices: " << ether_get_active_voice_count(engine) << std::endl;
        std::cout << "  CPU: " << std::fixed << std::setprecision(1) << ether_get_cpu_usage(engine) << "%" << std::endl;
        
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
    
    void setEngine(int engineType) {
        int count = ether_get_engine_type_count();
        if (engineType >= 0 && engineType < count) {
            currentEngineType = engineType;
            ether_set_instrument_engine_type(engine, currentInstrument, engineType);
            const char* name = ether_get_engine_type_name(engineType);
            std::cout << "🎛️  Switched to: " << (name ? name : "Unknown") << std::endl;
        } else {
            std::cout << "❌ Invalid engine type (0-" << (count-1) << ")" << std::endl;
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
            ether_play(engine);
            
            // Simple sequencer
            sequencerThread = std::thread([this]() {
                while (playing) {
                    if (stepPattern[currentStep]) {
                        int note = 60 + (currentStep % 12);
                        ether_note_on(engine, note, 0.8f, 0.0f);
                        
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        ether_note_off(engine, note);
                    }
                    
                    currentStep = (currentStep + 1) % 16;
                    
                    // Step timing
                    float stepMs = (60.0f / currentBPM) / 4.0f * 1000.0f;
                    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(stepMs - 50)));
                }
            });
            
            std::cout << "▶️  Playing" << std::endl;
        }
    }
    
    void stop() {
        if (playing) {
            playing = false;
            if (sequencerThread.joinable()) {
                sequencerThread.join();
            }
            ether_stop(engine);
            ether_all_notes_off(engine);
            std::cout << "⏹️  Stopped" << std::endl;
        }
    }
    
    void setBPM(float bpm) {
        if (bpm >= 60.0f && bpm <= 200.0f) {
            currentBPM = bpm;
            ether_set_bpm(engine, bpm);
            std::cout << "🥁 BPM: " << std::fixed << std::setprecision(1) << bpm << std::endl;
        } else {
            std::cout << "❌ BPM must be 60-200" << std::endl;
        }
    }
    
    void triggerNote(int note) {
        if (note >= 0 && note <= 127) {
            std::cout << "🎹 Note " << note << std::endl;
            ether_note_on(engine, note, 0.8f, 0.0f);
            
            std::thread([this, note]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                ether_note_off(engine, note);
            }).detach();
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
        std::cout << "  note <n>       - Trigger note n" << std::endl;
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
        showStatus();
        
        std::string input;
        while (running) {
            std::cout << "\nether> ";
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
                float bpm;
                if (iss >> bpm) {
                    setBPM(bpm);
                } else {
                    std::cout << "❌ Usage: bpm <60-200>" << std::endl;
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
    SimpleTerminalSynth synth;
    synth.run();
    return 0;
}