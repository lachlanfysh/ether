#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <iomanip>

// Include the EtherSynth bridge
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

class TerminalEtherSynth {
private:
    void* engine;
    std::atomic<bool> running{false};
    std::atomic<bool> playing{false};
    int currentInstrument = 0;
    int currentEngineType = 0;
    float currentBPM = 120.0f;
    std::vector<bool> stepPattern;
    int currentStep = 0;
    std::thread sequencerThread;
    
public:
    TerminalEtherSynth() : stepPattern(16, false) {
        std::cout << "🎵 Terminal EtherSynth v1.0" << std::endl;
        std::cout << "============================" << std::endl;
    }
    
    ~TerminalEtherSynth() {
        shutdown();
    }
    
    bool initialize() {
        std::cout << "\n🔧 Initializing EtherSynth engine..." << std::endl;
        
        // Create engine
        engine = ether_create();
        if (!engine) {
            std::cout << "❌ Failed to create EtherSynth engine" << std::endl;
            return false;
        }
        std::cout << "✅ Engine created successfully" << std::endl;
        
        // Initialize engine
        if (ether_initialize(engine) != 1) {
            std::cout << "❌ Failed to initialize EtherSynth engine" << std::endl;
            return false;
        }
        std::cout << "✅ Engine initialized successfully" << std::endl;
        
        running = true;
        
        // Show available engines
        showEngineInfo();
        
        return true;
    }
    
    void shutdown() {
        if (running) {
            std::cout << "\n🛑 Shutting down..." << std::endl;
            stop();
            if (sequencerThread.joinable()) {
                sequencerThread.join();
            }
            ether_shutdown(engine);
            ether_destroy(engine);
            running = false;
        }
    }
    
    void showEngineInfo() {
        std::cout << "\n🎛️  Available Synthesis Engines:" << std::endl;
        int engineCount = ether_get_engine_type_count();
        for (int i = 0; i < engineCount; i++) {
            const char* name = ether_get_engine_type_name(i);
            std::cout << "  " << i << ": " << (name ? name : "Unknown") << std::endl;
        }
        std::cout << std::endl;
    }
    
    void showStatus() {
        std::cout << "\n📊 Status:" << std::endl;
        std::cout << "  Engine: " << ether_get_engine_type_name(currentEngineType) << std::endl;
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
    
    void setEngineType(int engineType) {
        if (engineType >= 0 && engineType < ether_get_engine_type_count()) {
            currentEngineType = engineType;
            ether_set_instrument_engine_type(engine, currentInstrument, engineType);
            std::cout << "🎛️  Switched to engine: " << ether_get_engine_type_name(engineType) << std::endl;
        } else {
            std::cout << "❌ Invalid engine type" << std::endl;
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
            
            // Start sequencer thread
            sequencerThread = std::thread([this]() {
                while (playing) {
                    // Trigger active steps
                    if (stepPattern[currentStep]) {
                        int note = 60 + (currentStep % 12); // Map steps to notes
                        ether_note_on(engine, note, 0.8f, 0.0f);
                        
                        // Schedule note off after 50ms
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        ether_note_off(engine, note);
                    }
                    
                    // Advance step
                    currentStep = (currentStep + 1) % 16;
                    
                    // Calculate step interval from BPM
                    float stepInterval = (60.0f / currentBPM) / 4.0f * 1000.0f; // Quarter note steps in ms
                    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(stepInterval - 50)));
                }
            });
            
            std::cout << "▶️  Started playback" << std::endl;
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
            std::cout << "⏹️  Stopped playback" << std::endl;
        }
    }
    
    void setBPM(float bpm) {
        if (bpm >= 60.0f && bpm <= 200.0f) {
            currentBPM = bpm;
            ether_set_bpm(engine, bpm);
            std::cout << "🥁 BPM set to " << std::fixed << std::setprecision(1) << bpm << std::endl;
        } else {
            std::cout << "❌ BPM must be between 60 and 200" << std::endl;
        }
    }
    
    void triggerNote(int note) {
        if (note >= 0 && note <= 127) {
            std::cout << "🎹 Triggering note " << note << std::endl;
            ether_note_on(engine, note, 0.8f, 0.0f);
            
            // Auto note-off after 500ms
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
        std::cout << "  engines, e     - List available engines" << std::endl;
        std::cout << "  engine <n>     - Switch to engine n" << std::endl;
        std::cout << "  step <n>       - Toggle step n (1-16)" << std::endl;
        std::cout << "  play, p        - Start/stop playback" << std::endl;
        std::cout << "  bpm <n>        - Set BPM (60-200)" << std::endl;
        std::cout << "  note <n>       - Trigger MIDI note n (0-127)" << std::endl;
        std::cout << "  clear          - Clear all steps" << std::endl;
        std::cout << "  fill           - Fill all steps" << std::endl;
        std::cout << "  quit, q        - Exit" << std::endl;
        std::cout << std::endl;
    }
    
    void clearPattern() {
        std::fill(stepPattern.begin(), stepPattern.end(), false);
        std::cout << "🧹 Pattern cleared" << std::endl;
    }
    
    void fillPattern() {
        std::fill(stepPattern.begin(), stepPattern.end(), true);
        std::cout << "✨ Pattern filled" << std::endl;
    }
    
    void run() {
        if (!initialize()) {
            return;
        }
        
        std::cout << "\n🚀 Terminal EtherSynth Ready!" << std::endl;
        std::cout << "Type 'help' for commands, 'quit' to exit" << std::endl;
        
        showHelp();
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
                showEngineInfo();
            }
            else if (command == "engine") {
                int engineType;
                if (iss >> engineType) {
                    setEngineType(engineType);
                } else {
                    std::cout << "❌ Usage: engine <number>" << std::endl;
                }
            }
            else if (command == "step") {
                int step;
                if (iss >> step) {
                    toggleStep(step - 1); // Convert to 0-based
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
                clearPattern();
            }
            else if (command == "fill") {
                fillPattern();
            }
            else {
                std::cout << "❌ Unknown command. Type 'help' for available commands." << std::endl;
            }
        }
        
        std::cout << "\n👋 Goodbye!" << std::endl;
    }
};

int main() {
    TerminalEtherSynth synth;
    synth.run();
    return 0;
}