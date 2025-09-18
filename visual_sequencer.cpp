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
#include <map>
#include <portaudio.h>

// ANSI escape codes for terminal control
#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[H"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define RESET_COLOR "\033[0m"
#define BOLD "\033[1m"
#define DIM "\033[2m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define BG_RED "\033[41m"
#define BG_GREEN "\033[42m"
#define BG_YELLOW "\033[43m"
#define BG_BLUE "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN "\033[46m"
#define BG_WHITE "\033[47m"
#define BLACK "\033[30m"

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
    void ether_set_instrument_parameter(void* synth, int instrument, int param_id, float value);
    float ether_get_instrument_parameter(void* synth, int instrument, int param_id);
    void ether_shutdown(void* synth);
}

const int MAX_ENGINES = 16;

// Parameter IDs
enum ParameterID {
    HARMONICS = 0, TIMBRE, MORPH, OSC_MIX, DETUNE, SUB_LEVEL, SUB_ANCHOR,
    FILTER_CUTOFF, FILTER_RESONANCE, FILTER_TYPE,
    ATTACK, DECAY, SUSTAIN, RELEASE,
    LFO_RATE, LFO_DEPTH, LFO_SHAPE,
    REVERB_SIZE, REVERB_DAMPING, REVERB_MIX,
    DELAY_TIME, DELAY_FEEDBACK,
    VOLUME, PAN,
    PARAM_COUNT
};

std::map<int, std::string> parameterNames = {
    {HARMONICS, "harmonics"}, {TIMBRE, "timbre"}, {MORPH, "morph"},
    {OSC_MIX, "oscmix"}, {DETUNE, "detune"}, {SUB_LEVEL, "sublevel"}, {SUB_ANCHOR, "subanchor"},
    {FILTER_CUTOFF, "cutoff"}, {FILTER_RESONANCE, "resonance"}, {FILTER_TYPE, "filtertype"},
    {ATTACK, "attack"}, {DECAY, "decay"}, {SUSTAIN, "sustain"}, {RELEASE, "release"},
    {LFO_RATE, "lfo_rate"}, {LFO_DEPTH, "lfo_depth"}, {LFO_SHAPE, "lfo_shape"},
    {REVERB_SIZE, "reverb_size"}, {REVERB_DAMPING, "reverb_damp"}, {REVERB_MIX, "reverb_mix"},
    {DELAY_TIME, "delay_time"}, {DELAY_FEEDBACK, "delay_fb"},
    {VOLUME, "volume"}, {PAN, "pan"}
};

// Global state
void* etherEngine = nullptr;
std::atomic<bool> audioRunning{false};
std::atomic<bool> playing{false};
std::atomic<bool> stepTrigger[MAX_ENGINES][16] = {};
std::atomic<bool> noteOffTrigger[MAX_ENGINES][16] = {};
std::atomic<int> currentStep{0};
std::atomic<int> activeNotes[MAX_ENGINES][16] = {};
std::atomic<bool> uiUpdateNeeded{true};

// Step data per engine
struct StepData {
    bool active = false;
    int note = 60;
    float velocity = 0.6f;
};

std::array<std::vector<StepData>, MAX_ENGINES> enginePatterns;
std::array<std::map<int, float>, MAX_ENGINES> engineParameters;

// Minor scale
const std::vector<int> minorScale = {
    48, 50, 51, 53, 55, 56, 58, 59, 60, 62, 63, 65, 67, 68, 70, 72
};

// Engine colors for visual distinction
const std::string engineColors[MAX_ENGINES] = {
    RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, RED,
    GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, RED, GREEN
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

std::string noteToShortName(int midiNote) {
    const char* noteNames[] = {"C", "#", "D", "#", "E", "F", "#", "G", "#", "A", "#", "B"};
    int octave = (midiNote / 12) - 1;
    int noteIndex = midiNote % 12;
    if (noteNames[noteIndex][0] == '#') {
        // Use flat notation for display
        const char* flatNames[] = {"C", "b", "D", "b", "E", "F", "b", "G", "b", "A", "b", "B"};
        return std::string(flatNames[noteIndex]) + std::to_string(octave);
    }
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
        
        // Initialize default parameters
        engineParameters[engine][ATTACK] = 0.01f;
        engineParameters[engine][DECAY] = 0.3f;
        engineParameters[engine][SUSTAIN] = 0.7f;
        engineParameters[engine][RELEASE] = 0.8f;
        engineParameters[engine][FILTER_CUTOFF] = 0.8f;
        engineParameters[engine][FILTER_RESONANCE] = 0.2f;
        engineParameters[engine][VOLUME] = 0.8f;
        engineParameters[engine][PAN] = 0.5f;
        engineParameters[engine][REVERB_MIX] = 0.3f;
    }
}

// PortAudio callback
int audioCallback(const void* /*inputBuffer*/, void* outputBuffer,
                 unsigned long framesPerBuffer,
                 const PaStreamCallbackTimeInfo* /*timeInfo*/,
                 PaStreamCallbackFlags /*statusFlags*/,
                 void* /*userData*/) {
    
    float* out = static_cast<float*>(outputBuffer);
    
    for (unsigned long i = 0; i < framesPerBuffer * 2; i++) {
        out[i] = 0.0f;
    }
    
    for (int engine = 0; engine < MAX_ENGINES; engine++) {
        for (int step = 0; step < 16; step++) {
            if (stepTrigger[engine][step].exchange(false)) {
                if (enginePatterns[engine][step].active) {
                    ether_set_active_instrument(etherEngine, 0);
                    ether_set_instrument_engine_type(etherEngine, 0, engine);
                    ether_note_on(etherEngine, enginePatterns[engine][step].note, 
                                 enginePatterns[engine][step].velocity, 0.0f);
                    activeNotes[engine][step] = enginePatterns[engine][step].note;
                    uiUpdateNeeded = true;
                }
            }
            
            if (noteOffTrigger[engine][step].exchange(false)) {
                int note = activeNotes[engine][step].exchange(-1);
                if (note >= 0) {
                    ether_set_active_instrument(etherEngine, 0);
                    ether_set_instrument_engine_type(etherEngine, 0, engine);
                    ether_note_off(etherEngine, note);
                }
            }
        }
    }
    
    if (etherEngine) {
        ether_process_audio(etherEngine, out, framesPerBuffer);
    }
    
    return paContinue;
}

class VisualSequencer {
private:
    PaStream* stream = nullptr;
    std::thread sequencerThread;
    std::thread uiThread;
    std::atomic<bool> running{false};
    std::atomic<float> bpm{120.0f};
    int currentEngine = 14;
    bool showHelp = false;
    std::string lastCommand = "";
    
public:
    VisualSequencer() {
        initializeEnginePatterns();
    }
    
    ~VisualSequencer() {
        shutdown();
    }
    
    bool initialize() {
        // Initialize EtherSynth
        etherEngine = ether_create();
        if (!etherEngine) return false;
        
        ether_initialize(etherEngine);
        ether_set_master_volume(etherEngine, 0.8f);
        ether_play(etherEngine);
        
        // Initialize all engine parameters
        for (int engine = 0; engine < MAX_ENGINES; engine++) {
            ether_set_instrument_engine_type(etherEngine, 0, engine);
            for (auto& [paramId, value] : engineParameters[engine]) {
                ether_set_instrument_parameter(etherEngine, 0, paramId, value);
            }
        }
        
        // Initialize PortAudio
        PaError err = Pa_Initialize();
        if (err != paNoError) return false;
        
        err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, 48000, 128, audioCallback, nullptr);
        if (err != paNoError) return false;
        
        err = Pa_StartStream(stream);
        if (err != paNoError) return false;
        
        audioRunning = true;
        running = true;
        
        return true;
    }
    
    void drawUI() {
        std::cout << CURSOR_HOME;
        
        // Title
        std::cout << BOLD CYAN "🎵 EtherSynth Visual Multi-Engine Sequencer" RESET_COLOR << std::endl;
        std::cout << "==========================================" << std::endl;
        
        // Current engine and status
        const char* engineName = ether_get_engine_type_name(currentEngine);
        std::cout << BOLD "Current: " << engineColors[currentEngine] << currentEngine << ": " 
                  << (engineName ? engineName : "Unknown") << RESET_COLOR 
                  << " | BPM: " << BOLD << std::fixed << std::setprecision(1) << bpm.load() << RESET_COLOR
                  << " | " << (playing ? GREEN "▶ PLAYING" : RED "⏹ STOPPED") << RESET_COLOR << std::endl;
        
        std::cout << std::endl;
        
        // Step numbers header
        std::cout << "Eng ";
        for (int step = 0; step < 16; step++) {
            if (step == currentStep && playing) {
                std::cout << BG_YELLOW << BLACK << std::setw(4) << (step + 1) << RESET_COLOR;
            } else {
                std::cout << DIM << std::setw(4) << (step + 1) << RESET_COLOR;
            }
        }
        std::cout << std::endl;
        
        // Engine pattern grid
        for (int engine = 0; engine < MAX_ENGINES; engine++) {
            // Engine number with color
            std::cout << engineColors[engine] << std::setw(2) << engine << " " RESET_COLOR;
            
            // Pattern steps
            for (int step = 0; step < 16; step++) {
                if (enginePatterns[engine][step].active) {
                    // Active step with note
                    std::string noteStr = noteToShortName(enginePatterns[engine][step].note);
                    if (step == currentStep && playing) {
                        // Current playing step
                        std::cout << BG_GREEN << BLACK << std::setw(4) << noteStr << RESET_COLOR;
                    } else {
                        // Active step
                        std::cout << engineColors[engine] << std::setw(4) << noteStr << RESET_COLOR;
                    }
                } else {
                    // Empty step
                    if (step == currentStep && playing) {
                        std::cout << BG_YELLOW << "  · " << RESET_COLOR;
                    } else {
                        std::cout << DIM "  · " RESET_COLOR;
                    }
                }
            }
            
            // Engine name
            const char* name = ether_get_engine_type_name(engine);
            if (engine == currentEngine) {
                std::cout << " " << BOLD << engineColors[engine] << "👈 " << (name ? name : "Unknown") << RESET_COLOR;
            } else {
                std::cout << " " << DIM << (name ? name : "Unknown") << RESET_COLOR;
            }
            std::cout << std::endl;
        }
        
        std::cout << std::endl;
        
        // Current engine parameters (compact)
        std::cout << BOLD "Parameters: " << RESET_COLOR;
        std::cout << "A:" << std::fixed << std::setprecision(2) << engineParameters[currentEngine][ATTACK] << " ";
        std::cout << "D:" << engineParameters[currentEngine][DECAY] << " ";
        std::cout << "S:" << engineParameters[currentEngine][SUSTAIN] << " ";
        std::cout << "R:" << engineParameters[currentEngine][RELEASE] << " ";
        std::cout << "Cut:" << engineParameters[currentEngine][FILTER_CUTOFF] << " ";
        std::cout << "Res:" << engineParameters[currentEngine][FILTER_RESONANCE];
        std::cout << std::endl;
        
        // Command line
        std::cout << std::endl;
        if (showHelp) {
            std::cout << CYAN "Commands: " RESET_COLOR << std::endl;
            std::cout << "eng <0-15> | step <1-16> [note] | attack/decay/sustain/release <0-1>" << std::endl;
            std::cout << "cutoff/resonance <0-1> | play/stop | bpm <60-200> | clear | h (toggle help)" << std::endl;
        } else {
            std::cout << CYAN "Last: " RESET_COLOR << lastCommand << " " << DIM "(type 'h' for help)" RESET_COLOR;
        }
        std::cout << std::endl;
        std::cout << BOLD "seq[" << currentEngine << "]> " RESET_COLOR;
    }
    
    void run() {
        std::cout << CLEAR_SCREEN << HIDE_CURSOR;
        
        // Start UI update thread
        uiThread = std::thread([this]() {
            while (running) {
                if (uiUpdateNeeded.exchange(false)) {
                    drawUI();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });
        
        uiUpdateNeeded = true;
        
        std::string input;
        while (running) {
            if (!std::getline(std::cin, input)) break;
            
            lastCommand = input;
            if (input.empty()) continue;
            
            std::istringstream iss(input);
            std::string command;
            iss >> command;
            
            if (command == "quit" || command == "q") {
                break;
            }
            else if (command == "h") {
                showHelp = !showHelp;
                uiUpdateNeeded = true;
            }
            else if (command == "eng" || command == "engine") {
                int type;
                if (iss >> type && type >= 0 && type < MAX_ENGINES) {
                    currentEngine = type;
                    uiUpdateNeeded = true;
                }
            }
            else if (command == "step") {
                int step, scaleNote = -1;
                if (iss >> step) {
                    if (iss >> scaleNote) {
                        setStep(step, scaleNote);
                    } else {
                        setStep(step);
                    }
                }
            }
            else if (parameterNames.find(getParameterIdFromName(command)) != parameterNames.end()) {
                float value;
                if (iss >> value) {
                    setParameter(command, value);
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
                if (iss >> newBpm && newBpm >= 60.0f && newBpm <= 200.0f) {
                    bpm = newBpm;
                    uiUpdateNeeded = true;
                }
            }
            else if (command == "clear") {
                clearPattern();
            }
            else if (command == "clearall") {
                clearAllPatterns();
            }
            
            uiUpdateNeeded = true;
        }
        
        std::cout << SHOW_CURSOR << CLEAR_SCREEN;
    }
    
    void play() {
        if (!playing) {
            playing = true;
            currentStep = 0;
            uiUpdateNeeded = true;
            
            sequencerThread = std::thread([this]() {
                while (playing) {
                    for (int engine = 0; engine < MAX_ENGINES; engine++) {
                        if (enginePatterns[engine][currentStep].active) {
                            stepTrigger[engine][currentStep] = true;
                            
                            std::thread([this, engine]() {
                                float stepMs = (60.0f / bpm) / 4.0f * 1000.0f;
                                float releaseParam = engineParameters[engine][RELEASE];
                                float noteOffMs = stepMs * (0.1f + releaseParam * 0.8f);
                                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(noteOffMs)));
                                if (playing) {
                                    noteOffTrigger[engine][currentStep] = true;
                                }
                            }).detach();
                        }
                    }
                    
                    currentStep = (currentStep + 1) % 16;
                    uiUpdateNeeded = true;
                    
                    float stepMs = (60.0f / bpm) / 4.0f * 1000.0f;
                    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(stepMs)));
                }
            });
        }
    }
    
    void stop() {
        if (playing) {
            playing = false;
            ether_all_notes_off(etherEngine);
            if (sequencerThread.joinable()) {
                sequencerThread.join();
            }
            uiUpdateNeeded = true;
        }
    }
    
    // ... (other methods similar to previous version but without console output)
    
    int getParameterIdFromName(const std::string& name) {
        for (auto& [id, paramName] : parameterNames) {
            if (paramName == name) return id;
        }
        return -1;
    }
    
    void setParameter(const std::string& paramName, float value) {
        int paramId = getParameterIdFromName(paramName);
        if (paramId >= 0) {
            value = std::max(0.0f, std::min(1.0f, value));
            engineParameters[currentEngine][paramId] = value;
            ether_set_active_instrument(etherEngine, 0);
            ether_set_instrument_engine_type(etherEngine, 0, currentEngine);
            ether_set_instrument_parameter(etherEngine, 0, paramId, value);
            uiUpdateNeeded = true;
        }
    }
    
    void setStep(int step, int scaleNote = -1) {
        if (step < 1 || step > 16) return;
        
        int stepIndex = step - 1;
        
        if (scaleNote == -1) {
            enginePatterns[currentEngine][stepIndex].active = !enginePatterns[currentEngine][stepIndex].active;
        } else {
            if (scaleNote >= 0 && scaleNote <= 15) {
                enginePatterns[currentEngine][stepIndex].active = true;
                enginePatterns[currentEngine][stepIndex].note = scaleIndexToMidiNote(scaleNote);
            }
        }
        uiUpdateNeeded = true;
    }
    
    void clearPattern() {
        for (auto& step : enginePatterns[currentEngine]) {
            step.active = false;
        }
        uiUpdateNeeded = true;
    }
    
    void clearAllPatterns() {
        for (int engine = 0; engine < MAX_ENGINES; engine++) {
            for (auto& step : enginePatterns[engine]) {
                step.active = false;
            }
        }
        uiUpdateNeeded = true;
    }
    
    void shutdown() {
        if (running) {
            stop();
            running = false;
            
            if (uiThread.joinable()) {
                uiThread.join();
            }
            
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
        }
    }
};

int main() {
    VisualSequencer sequencer;
    
    if (!sequencer.initialize()) {
        return 1;
    }
    
    sequencer.run();
    return 0;
}