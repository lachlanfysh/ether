#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <array>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <portaudio.h>

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

const int MAX_ENGINES = 16;

// Global state
void* etherEngine = nullptr;
std::atomic<bool> audioRunning{false};
std::atomic<bool> playing{false};
std::atomic<bool> stepTrigger[MAX_ENGINES][16] = {}; // Per-engine step triggers
std::atomic<bool> noteOffTrigger[MAX_ENGINES][16] = {}; // Per-engine note-offs
std::atomic<int> currentStep{0};
std::atomic<int> activeNotes[MAX_ENGINES][16] = {}; // Per-engine active notes

// Step data per engine
struct StepData {
    bool active = false;
    int note = 60;
    float velocity = 0.6f;
};

// Each engine has its own 16-step pattern
std::array<std::vector<StepData>, MAX_ENGINES> enginePatterns;

// Minor scale with C4 at position 8
const std::vector<int> minorScale = {
    48, 50, 51, 53, 55, 56, 58, 59, 60, 62, 63, 65, 67, 68, 70, 72
};

int scaleIndexToMidiNote(int scaleIndex) {
    scaleIndex = std::max(0, std::min(15, scaleIndex));
    return minorScale[scaleIndex];
}

std::string midiNoteToName(int midiNote) {
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (midiNote / 12) - 1;
    int noteIndex = midiNote % 12;
    return std::string(noteNames[noteIndex]) + std::to_string(octave);
}

void initializeEnginePatterns() {
    for (int engine = 0; engine < MAX_ENGINES; engine++) {
        enginePatterns[engine].resize(16);
        for (int step = 0; step < 16; step++) {
            enginePatterns[engine][step].active = false;
            enginePatterns[engine][step].note = 60;
            enginePatterns[engine][step].velocity = 0.6f;
            activeNotes[engine][step] = -1;
        }
    }
}

// PortAudio callback with multi-engine step sequencing
int audioCallback(const void* /*inputBuffer*/, void* outputBuffer,
                 unsigned long framesPerBuffer,
                 const PaStreamCallbackTimeInfo* /*timeInfo*/,
                 PaStreamCallbackFlags /*statusFlags*/,
                 void* /*userData*/) {
    
    float* out = static_cast<float*>(outputBuffer);
    
    // Clear buffer
    for (unsigned long i = 0; i < framesPerBuffer * 2; i++) {
        out[i] = 0.0f;
    }
    
    // Handle step triggers for ALL engines concurrently
    for (int engine = 0; engine < MAX_ENGINES; engine++) {
        for (int step = 0; step < 16; step++) {
            if (stepTrigger[engine][step].exchange(false)) {
                if (enginePatterns[engine][step].active) {
                    // Set active instrument to this engine and trigger note
                    ether_set_active_instrument(etherEngine, 0); // Use slot 0
                    ether_set_instrument_engine_type(etherEngine, 0, engine);
                    ether_note_on(etherEngine, enginePatterns[engine][step].note, 
                                 enginePatterns[engine][step].velocity, 0.0f);
                    activeNotes[engine][step] = enginePatterns[engine][step].note;
                }
            }
            
            // Handle note-off triggers for ADSR
            if (noteOffTrigger[engine][step].exchange(false)) {
                int note = activeNotes[engine][step].exchange(-1);
                if (note >= 0) {
                    // Set active instrument to this engine and send note off
                    ether_set_active_instrument(etherEngine, 0);
                    ether_set_instrument_engine_type(etherEngine, 0, engine);
                    ether_note_off(etherEngine, note);
                }
            }
        }
    }
    
    // Process audio through synthesis engines
    if (etherEngine) {
        ether_process_audio(etherEngine, out, framesPerBuffer);
    }
    
    return paContinue;
}

class MultiEngineSequencer {
private:
    PaStream* stream = nullptr;
    std::thread sequencerThread;
    std::atomic<bool> running{false};
    std::atomic<float> bpm{120.0f};
    int currentEngine = 14; // Start with SlideAccentBass
    
public:
    MultiEngineSequencer() {
        std::cout << "🎵 EtherSynth Multi-Engine Step Sequencer!" << std::endl;
        std::cout << "===========================================" << std::endl;
        std::cout << "🎛️  Each engine has its own 16-step pattern!" << std::endl;
        std::cout << "🎼 Layer multiple engines for rich sequences!" << std::endl;
        initializeEnginePatterns();
    }
    
    ~MultiEngineSequencer() {
        shutdown();
    }
    
    bool initialize() {
        std::cout << "\n🔧 Initializing Multi-Engine EtherSynth..." << std::endl;
        
        // Initialize EtherSynth
        etherEngine = ether_create();
        if (!etherEngine) {
            std::cout << "❌ Failed to create EtherSynth" << std::endl;
            return false;
        }
        
        ether_initialize(etherEngine);
        
        // Initialize ALL engine slots
        for (int i = 0; i < MAX_ENGINES; i++) {
            ether_set_instrument_engine_type(etherEngine, 0, i); // We'll switch dynamically
        }
        
        ether_set_master_volume(etherEngine, 0.8f);
        ether_play(etherEngine);
        
        // Initialize PortAudio
        PaError err = Pa_Initialize();
        if (err != paNoError) {
            std::cout << "❌ PortAudio initialization failed" << std::endl;
            return false;
        }
        
        // Open audio stream
        err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, 48000, 128, audioCallback, nullptr);
        if (err != paNoError) {
            std::cout << "❌ Failed to open audio stream" << std::endl;
            return false;
        }
        
        // Start audio
        err = Pa_StartStream(stream);
        if (err != paNoError) {
            std::cout << "❌ Failed to start audio stream" << std::endl;
            return false;
        }
        
        audioRunning = true;
        running = true;
        
        std::cout << "✅ Multi-Engine EtherSynth initialized!" << std::endl;
        std::cout << "🔊 Audio callback running - ready for layered sequences!" << std::endl;
        
        return true;
    }
    
    void run() {
        showEngines();
        showHelp();
        
        std::string input;
        while (running) {
            std::cout << "\nseq[" << currentEngine << "]> ";
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
            else if (command == "engine" || command == "eng") {
                int type;
                if (iss >> type) {
                    setEngine(type);
                } else {
                    std::cout << "❌ Usage: engine <0-15>" << std::endl;
                }
            }
            else if (command == "step") {
                int step, scaleNote = -1;
                if (iss >> step >> scaleNote) {
                    setStep(step, scaleNote);
                } else if (iss.clear(), iss.seekg(0), iss >> command >> step) {
                    setStep(step);
                } else {
                    std::cout << "❌ Usage: step <1-16> [scale_note_0-15]" << std::endl;
                }
            }
            else if (command == "clear") {
                clearPattern();
            }
            else if (command == "clearall") {
                clearAllPatterns();
            }
            else if (command == "copy") {
                int fromEngine;
                if (iss >> fromEngine) {
                    copyPattern(fromEngine);
                } else {
                    std::cout << "❌ Usage: copy <engine_0-15>" << std::endl;
                }
            }
            else if (command == "play") {
                play();
            }
            else if (command == "stop") {
                stop();
            }
            else if (command == "bpm") {
                float newBpm;
                if (iss >> newBpm) {
                    setBPM(newBpm);
                } else {
                    std::cout << "❌ Usage: bpm <60-200>" << std::endl;
                }
            }
            else if (command == "volume" || command == "vol") {
                float vol;
                if (iss >> vol) {
                    setVolume(vol);
                } else {
                    std::cout << "❌ Usage: volume <0.0-1.0>" << std::endl;
                }
            }
            else if (command == "patterns") {
                showAllPatterns();
            }
            else {
                std::cout << "❌ Unknown command. Type 'help'" << std::endl;
            }
        }
        
        std::cout << "\n👋 Goodbye!" << std::endl;
    }
    
    void play() {
        if (!playing) {
            playing = true;
            currentStep = 0;
            
            // Start sequencer thread - triggers ALL active engines simultaneously
            sequencerThread = std::thread([this]() {
                while (playing) {
                    // Trigger current step for ALL engines that have active steps
                    for (int engine = 0; engine < MAX_ENGINES; engine++) {
                        if (enginePatterns[engine][currentStep].active) {
                            stepTrigger[engine][currentStep] = true;
                            
                            // Schedule note-off for this engine's step
                            std::thread([this, engine]() {
                                float stepMs = (60.0f / bpm) / 4.0f * 1000.0f;
                                float noteOffMs = stepMs * 0.125f; // 1/8 of step = tight envelope
                                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(noteOffMs)));
                                if (playing) {
                                    noteOffTrigger[engine][currentStep] = true;
                                }
                            }).detach();
                        }
                    }
                    
                    currentStep = (currentStep + 1) % 16;
                    
                    // Calculate step timing from BPM (16th notes)
                    float stepMs = (60.0f / bpm) / 4.0f * 1000.0f;
                    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(stepMs)));
                }
            });
            
            std::cout << "▶️  Playing ALL engine patterns simultaneously!" << std::endl;
        }
    }
    
    void stop() {
        if (playing) {
            playing = false;
            ether_all_notes_off(etherEngine);
            if (sequencerThread.joinable()) {
                sequencerThread.join();
            }
            std::cout << "⏹️  Stopped all engines" << std::endl;
        }
    }
    
    void setEngine(int type) {
        int count = ether_get_engine_type_count();
        if (type >= 0 && type < count) {
            currentEngine = type;
            const char* name = ether_get_engine_type_name(type);
            std::cout << "🎛️  Now editing engine " << type << ": " << (name ? name : "Unknown") << std::endl;
            std::cout << "💡 Use 'step' commands to program this engine's pattern" << std::endl;
        } else {
            std::cout << "❌ Invalid engine (0-" << (count-1) << ")" << std::endl;
        }
    }
    
    void setStep(int step, int scaleNote = -1) {
        if (step < 1 || step > 16) {
            std::cout << "❌ Step must be 1-16" << std::endl;
            return;
        }
        
        int stepIndex = step - 1;
        
        if (scaleNote == -1) {
            // Toggle step on/off for current engine
            enginePatterns[currentEngine][stepIndex].active = !enginePatterns[currentEngine][stepIndex].active;
            std::cout << "🎵 Engine " << currentEngine << " step " << step 
                      << (enginePatterns[currentEngine][stepIndex].active ? " ON" : " OFF") << std::endl;
        } else {
            // Set step to specific scale note for current engine
            if (scaleNote >= 0 && scaleNote <= 15) {
                enginePatterns[currentEngine][stepIndex].active = true;
                enginePatterns[currentEngine][stepIndex].note = scaleIndexToMidiNote(scaleNote);
                std::cout << "🎵 Engine " << currentEngine << " step " << step << " set to scale note " << scaleNote 
                          << " (" << midiNoteToName(enginePatterns[currentEngine][stepIndex].note) << ")" << std::endl;
            } else {
                std::cout << "❌ Scale note must be 0-15" << std::endl;
            }
        }
    }
    
    void clearPattern() {
        for (auto& step : enginePatterns[currentEngine]) {
            step.active = false;
        }
        const char* name = ether_get_engine_type_name(currentEngine);
        std::cout << "🧹 Cleared pattern for engine " << currentEngine << " (" << (name ? name : "Unknown") << ")" << std::endl;
    }
    
    void clearAllPatterns() {
        for (int engine = 0; engine < MAX_ENGINES; engine++) {
            for (auto& step : enginePatterns[engine]) {
                step.active = false;
            }
        }
        std::cout << "🧹 Cleared ALL engine patterns" << std::endl;
    }
    
    void copyPattern(int fromEngine) {
        if (fromEngine >= 0 && fromEngine < MAX_ENGINES) {
            enginePatterns[currentEngine] = enginePatterns[fromEngine];
            const char* fromName = ether_get_engine_type_name(fromEngine);
            const char* toName = ether_get_engine_type_name(currentEngine);
            std::cout << "📋 Copied pattern from engine " << fromEngine << " (" << (fromName ? fromName : "Unknown") 
                      << ") to engine " << currentEngine << " (" << (toName ? toName : "Unknown") << ")" << std::endl;
        } else {
            std::cout << "❌ Invalid source engine (0-" << (MAX_ENGINES-1) << ")" << std::endl;
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
    
    void showEngines() {
        std::cout << "\n🎛️  REAL Synthesis Engines:" << std::endl;
        int count = ether_get_engine_type_count();
        
        for (int i = 0; i < count; i++) {
            const char* name = ether_get_engine_type_name(i);
            std::string marker = (i == currentEngine) ? " 👈 (editing)" : "";
            
            // Count active steps for this engine
            int activeSteps = 0;
            for (const auto& step : enginePatterns[i]) {
                if (step.active) activeSteps++;
            }
            
            std::cout << "  " << i << ": " << (name ? name : "Unknown") 
                      << " [" << activeSteps << " steps]" << marker << std::endl;
        }
        std::cout << std::endl;
    }
    
    void showStatus() {
        std::cout << "\n📊 Status:" << std::endl;
        std::cout << "  Current Engine: " << currentEngine << " (" << ether_get_engine_type_name(currentEngine) << ")" << std::endl;
        std::cout << "  BPM: " << std::fixed << std::setprecision(1) << bpm.load() << std::endl;
        std::cout << "  Playing: " << (playing ? "YES" : "NO") << std::endl;
        std::cout << "  Volume: " << std::fixed << std::setprecision(2) << ether_get_master_volume(etherEngine) << std::endl;
        std::cout << "  Audio: " << (audioRunning ? "RUNNING" : "STOPPED") << std::endl;
        
        // Show current engine's pattern
        std::cout << "\n🎵 Current Engine Pattern:" << std::endl;
        for (int i = 0; i < 16; i++) {
            if (i == currentStep && playing) {
                if (enginePatterns[currentEngine][i].active) {
                    std::cout << "[" << std::setw(2) << (i+1) << ":" << midiNoteToName(enginePatterns[currentEngine][i].note) << "]";
                } else {
                    std::cout << "[" << std::setw(2) << (i+1) << ": - ]";
                }
            } else {
                if (enginePatterns[currentEngine][i].active) {
                    std::cout << " " << std::setw(2) << (i+1) << ":" << midiNoteToName(enginePatterns[currentEngine][i].note) << " ";
                } else {
                    std::cout << " " << std::setw(2) << (i+1) << ": - " << " ";
                }
            }
        }
        std::cout << std::endl;
    }
    
    void showAllPatterns() {
        std::cout << "\n🎼 ALL Engine Patterns:" << std::endl;
        for (int engine = 0; engine < MAX_ENGINES; engine++) {
            const char* name = ether_get_engine_type_name(engine);
            
            // Count active steps
            int activeSteps = 0;
            for (const auto& step : enginePatterns[engine]) {
                if (step.active) activeSteps++;
            }
            
            if (activeSteps > 0) {
                std::cout << "\n" << engine << ": " << (name ? name : "Unknown") 
                          << " [" << activeSteps << " steps]" << std::endl;
                std::cout << "  ";
                for (int i = 0; i < 16; i++) {
                    if (enginePatterns[engine][i].active) {
                        std::cout << " " << std::setw(2) << (i+1) << ":" << midiNoteToName(enginePatterns[engine][i].note) << " ";
                    } else {
                        std::cout << " " << std::setw(2) << (i+1) << ": - " << " ";
                    }
                }
                std::cout << std::endl;
            }
        }
        if (playing) {
            std::cout << "\n▶️  Currently playing step " << (currentStep + 1) << "/16" << std::endl;
        }
    }
    
    void showHelp() {
        std::cout << "\n🎵 MULTI-ENGINE COMMANDS:" << std::endl;
        std::cout << "  engine <0-15>     - Switch to engine for editing" << std::endl;
        std::cout << "  step <1-16> [0-15] - Set step note for current engine" << std::endl;
        std::cout << "  step <1-16>       - Toggle step on/off for current engine" << std::endl;
        std::cout << "  clear             - Clear current engine's pattern" << std::endl;
        std::cout << "  clearall          - Clear ALL engine patterns" << std::endl;
        std::cout << "  copy <engine>     - Copy pattern from another engine" << std::endl;
        std::cout << "  patterns          - Show all active patterns" << std::endl;
        std::cout << "  play              - Start ALL engines simultaneously" << std::endl;
        std::cout << "  stop              - Stop all engines" << std::endl;
        std::cout << "  bpm <60-200>      - Set tempo for all engines" << std::endl;
        std::cout << "  volume <0-1>      - Set master volume" << std::endl;
        std::cout << "  status (s)        - Show current engine status" << std::endl;
        std::cout << "  engines (e)       - List all engines" << std::endl;
        std::cout << "  help (h)          - Show this help" << std::endl;
        std::cout << "  quit (q)          - Exit" << std::endl;
        std::cout << "\n🎼 Scale: 8=C4 (middle), 0=C3 (low), 15=C5 (high)" << std::endl;
        std::cout << "💡 Workflow: 'engine 14', 'step 1 8', 'engine 15', 'step 1 12', 'play'!" << std::endl;
    }
    
    void shutdown() {
        if (running) {
            std::cout << "\n🛑 Shutting down multi-engine sequencer..." << std::endl;
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
};

int main() {
    MultiEngineSequencer sequencer;
    
    if (!sequencer.initialize()) {
        return 1;
    }
    
    sequencer.run();
    return 0;
}