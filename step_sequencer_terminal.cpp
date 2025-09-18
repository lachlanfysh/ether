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
const int SAMPLE_RATE = 48000;
const int FRAMES_PER_BUFFER = 128;

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
std::atomic<bool> noteOffTrigger[16] = {};  // For note-off events
std::atomic<int> currentStep{0};
std::atomic<bool> noteQueue[128] = {};
std::atomic<int> activeNotes[16] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}; // Track active notes per step

// Step sequencer data - proper step programming
struct StepData {
    bool active = false;    // Is this step on?
    int note = 60;         // What note should this step play? (C4 = 60)
    float velocity = 0.6f; // Velocity for this step
};

std::vector<StepData> stepPattern(16);

// Minor scale with C4 (MIDI note 60) at position 8 (middle of range)
// Positions: 0=C3, 1=D3, 2=Eb3, 3=F3, 4=G3, 5=Ab3, 6=Bb3, 7=C4-1oct, 8=C4, 9=D4, 10=Eb4, 11=F4, 12=G4, 13=Ab4, 14=Bb4, 15=C5
const std::vector<int> minorScale = {
    48, 50, 51, 53, 55, 56, 58, 59, 60, 62, 63, 65, 67, 68, 70, 72
};

int scaleIndexToMidiNote(int scaleIndex) {
    // Scale index 8 = C4 (middle of range)
    // Clamp to available scale notes
    scaleIndex = std::max(0, std::min(15, scaleIndex));
    return minorScale[scaleIndex];
}

std::string midiNoteToName(int midiNote) {
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (midiNote / 12) - 1;
    int noteIndex = midiNote % 12;
    return std::string(noteNames[noteIndex]) + std::to_string(octave);
}

void initializeSteps() {
    // Initialize all steps to off, with C4 as default note
    for (int i = 0; i < 16; i++) {
        stepPattern[i].active = false;
        stepPattern[i].note = 60; // C4
        stepPattern[i].velocity = 0.6f;
    }
}

// PortAudio callback - now with proper step sequencing
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
    
    // Handle step triggers - now plays the programmed note for each step
    for (int step = 0; step < 16; step++) {
        if (stepTrigger[step].exchange(false)) {
            if (stepPattern[step].active) {
                ether_note_on(etherEngine, stepPattern[step].note, stepPattern[step].velocity, 0.0f);
                activeNotes[step] = stepPattern[step].note; // Remember which note is playing
            }
        }
        
        // Handle note-off triggers
        if (noteOffTrigger[step].exchange(false)) {
            int note = activeNotes[step].exchange(-1);
            if (note >= 0) {
                ether_note_off(etherEngine, note);
            }
        }
    }
    
    // Process audio through REAL synthesis engines!
    ether_process_audio(etherEngine, out, framesPerBuffer);
    
    return paContinue;
}

class StepSequencerTerminal {
private:
    PaStream* stream = nullptr;
    std::thread sequencerThread;
    std::atomic<bool> running{false};
    std::atomic<float> bpm{120.0f};
    int currentEngine = 0;
    
public:
    StepSequencerTerminal() {
        std::cout << "🎵 EtherSynth Step Sequencer Terminal" << std::endl;
        std::cout << "=====================================" << std::endl;
        initializeSteps();
    }
    
    ~StepSequencerTerminal() {
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
        
        // Show pattern with notes
        std::cout << "\n🎵 Pattern (Minor Scale, 8=C4):" << std::endl;
        for (int i = 0; i < 16; i++) {
            if (i == currentStep && playing) {
                if (stepPattern[i].active) {
                    std::cout << "[" << std::setw(2) << (i+1) << ":" << midiNoteToName(stepPattern[i].note) << "]";
                } else {
                    std::cout << "[" << std::setw(2) << (i+1) << ": - ]";
                }
            } else {
                if (stepPattern[i].active) {
                    std::cout << " " << std::setw(2) << (i+1) << ":" << midiNoteToName(stepPattern[i].note) << " ";
                } else {
                    std::cout << " " << std::setw(2) << (i+1) << ": - " << " ";
                }
            }
        }
        std::cout << std::endl;
        
        // Show scale reference
        std::cout << "\n🎼 Scale Reference (0-15):" << std::endl;
        for (int i = 0; i < 16; i++) {
            std::cout << "  " << std::setw(2) << i << ": " << midiNoteToName(minorScale[i]) << std::endl;
        }
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
    
    void setStep(int step, int scaleNote = -1) {
        if (step < 1 || step > 16) {
            std::cout << "❌ Step must be 1-16" << std::endl;
            return;
        }
        
        int stepIndex = step - 1; // Convert to 0-based
        
        if (scaleNote == -1) {
            // Toggle step on/off
            stepPattern[stepIndex].active = !stepPattern[stepIndex].active;
            if (stepPattern[stepIndex].active) {
                std::cout << "🎵 Step " << step << " ON (" << midiNoteToName(stepPattern[stepIndex].note) << ")" << std::endl;
            } else {
                std::cout << "🎵 Step " << step << " OFF" << std::endl;
            }
        } else {
            // Set step note
            if (scaleNote >= 0 && scaleNote <= 15) {
                stepPattern[stepIndex].note = scaleIndexToMidiNote(scaleNote);
                stepPattern[stepIndex].active = true; // Automatically turn on when setting note
                std::cout << "🎵 Step " << step << " set to scale note " << scaleNote 
                          << " (" << midiNoteToName(stepPattern[stepIndex].note) << ")" << std::endl;
            } else {
                std::cout << "❌ Scale note must be 0-15 (8=C4)" << std::endl;
            }
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
                    // Trigger current step if active
                    if (stepPattern[currentStep].active) {
                        stepTrigger[currentStep] = true;
                        std::cout << "🎵 Step " << (currentStep + 1) << " triggered (" 
                                  << midiNoteToName(stepPattern[currentStep].note) << ")" << std::endl;
                        
                        // Schedule note-off after a short duration (1/32 note for tight steps)
                        int stepForNoteOff = currentStep; // Capture the correct step for note-off
                        std::thread([this, stepForNoteOff]() {
                            float noteOffMs = (60.0f / bpm) / 8.0f * 1000.0f; // 1/32 note duration
                            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(noteOffMs)));
                            if (playing) { // Only send note-off if still playing
                                noteOffTrigger[stepForNoteOff] = true;
                            }
                        }).detach();
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
            std::cout << "🎹 Triggering note " << noteNum << " (" << midiNoteToName(noteNum) 
                      << ", " << std::fixed << std::setprecision(1) << freq << " Hz)" << std::endl;
        }
    }
    
    void clearPattern() {
        for (auto& step : stepPattern) {
            step.active = false;
        }
        std::cout << "🧹 Pattern cleared" << std::endl;
    }
    
    void fillPattern() {
        for (int i = 0; i < 16; i++) {
            stepPattern[i].active = true;
            stepPattern[i].note = scaleIndexToMidiNote(i % 8 + 4); // Fill with scale pattern
        }
        std::cout << "✨ Pattern filled with scale" << std::endl;
    }
    
    void showHelp() {
        std::cout << "\n📖 Commands (Step Sequencer Mode):" << std::endl;
        std::cout << "  help, h          - Show this help" << std::endl;
        std::cout << "  status, s        - Show status and pattern" << std::endl;
        std::cout << "  engines, e       - List engines" << std::endl;
        std::cout << "  engine <n>       - Switch to REAL engine n" << std::endl;
        std::cout << "  step <n>         - Toggle step n (1-16) on/off" << std::endl;
        std::cout << "  step <n> <note>  - Set step n to scale note (0-15, 8=C4)" << std::endl;
        std::cout << "  play, p          - Start/stop playback" << std::endl;
        std::cout << "  bpm <n>          - Set BPM" << std::endl;
        std::cout << "  volume <n>       - Set volume (0.0-1.0)" << std::endl;
        std::cout << "  note <n>         - Trigger MIDI note n directly" << std::endl;
        std::cout << "  clear            - Clear pattern" << std::endl;
        std::cout << "  fill             - Fill pattern with scale" << std::endl;
        std::cout << "  quit, q          - Exit" << std::endl;
        std::cout << "\n🎼 Example: 'step 1 8' sets step 1 to C4 (middle of scale)" << std::endl;
        std::cout << "🎼 Scale: 0=C4, 1=D4, 2=Eb4, 3=F4, 4=G4, 5=Ab4, 6=Bb4, 7=C5..." << std::endl;
        std::cout << std::endl;
    }
    
    void run() {
        if (!initialize()) {
            std::cout << "❌ Failed to initialize" << std::endl;
            return;
        }
        
        std::cout << "\n🚀 Ready! Step Sequencer with REAL engines active!" << std::endl;
        std::cout << "🎵 Try: 'step 1 8', 'step 5 12', 'step 9 4', then 'play'!" << std::endl;
        showStatus();
        
        std::string input;
        while (running) {
            std::cout << "\nseq> ";
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
                int n, note = -1;
                if (iss >> n) {
                    iss >> note; // This will be -1 if no second argument
                    setStep(n, note);
                } else {
                    std::cout << "❌ Usage: step <1-16> [scale_note_0-15]" << std::endl;
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
                clearPattern();
            }
            else if (command == "fill") {
                fillPattern();
            }
            else {
                std::cout << "❌ Unknown command. Type 'help'" << std::endl;
            }
        }
        
        std::cout << "\n👋 Goodbye!" << std::endl;
    }
};

int main() {
    StepSequencerTerminal synth;
    synth.run();
    return 0;
}