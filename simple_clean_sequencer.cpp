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

class SimpleCleanSequencer {
private:
    PaStream* stream = nullptr;
    std::thread sequencerThread;
    std::atomic<bool> running{false};
    std::atomic<float> bpm{120.0f};
    int currentEngine = 14;
    
public:
    SimpleCleanSequencer() {
        initializeEnginePatterns();
    }
    
    ~SimpleCleanSequencer() {
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
    
    void showStatus() {
        std::cout << "\n=== EtherSynth Multi-Engine Sequencer ===" << std::endl;
        const char* engineName = ether_get_engine_type_name(currentEngine);
        std::cout << "Current Engine: " << currentEngine << " (" << (engineName ? engineName : "Unknown") << ")" << std::endl;
        std::cout << "BPM: " << std::fixed << std::setprecision(1) << bpm.load();
        std::cout << " | " << (playing ? "PLAYING" : "STOPPED");
        if (playing) {
            std::cout << " | Step: " << (currentStep + 1) << "/16";
        }
        std::cout << std::endl;
        
        // Show current engine's pattern in one line
        std::cout << "Pattern [" << currentEngine << "]: ";
        for (int i = 0; i < 16; i++) {
            if (enginePatterns[currentEngine][i].active) {
                if (i == currentStep && playing) {
                    std::cout << "[" << (i+1) << "]";
                } else {
                    std::cout << " " << (i+1) << " ";
                }
            } else {
                if (i == currentStep && playing) {
                    std::cout << "[·]";
                } else {
                    std::cout << " · ";
                }
            }
        }
        std::cout << std::endl;
        
        // Show parameters in one line
        std::cout << "Params: A:" << std::fixed << std::setprecision(2) << engineParameters[currentEngine][ATTACK];
        std::cout << " D:" << engineParameters[currentEngine][DECAY];
        std::cout << " S:" << engineParameters[currentEngine][SUSTAIN];
        std::cout << " R:" << engineParameters[currentEngine][RELEASE];
        std::cout << " Cut:" << engineParameters[currentEngine][FILTER_CUTOFF];
        std::cout << " Res:" << engineParameters[currentEngine][FILTER_RESONANCE];
        std::cout << std::endl;
    }
    
    void showAllPatterns() {
        std::cout << "\nAll Active Patterns:" << std::endl;
        for (int engine = 0; engine < MAX_ENGINES; engine++) {
            int activeSteps = 0;
            for (const auto& step : enginePatterns[engine]) {
                if (step.active) activeSteps++;
            }
            
            if (activeSteps > 0) {
                const char* name = ether_get_engine_type_name(engine);
                std::cout << engine << ":" << (name ? name : "Unknown") << " [" << activeSteps << "] ";
                for (int i = 0; i < 16; i++) {
                    if (enginePatterns[engine][i].active) {
                        std::cout << (i+1) << ":" << midiNoteToName(enginePatterns[engine][i].note) << " ";
                    }
                }
                std::cout << std::endl;
            }
        }
    }
    
    void run() {
        std::cout << "🎵 EtherSynth Multi-Engine Sequencer (Clean Mode)" << std::endl;
        std::cout << "Commands: eng <0-15>, step <1-16> [note], play, stop, status, patterns, params, quit" << std::endl;
        
        showStatus();
        
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
            else if (command == "status" || command == "s") {
                showStatus();
            }
            else if (command == "patterns") {
                showAllPatterns();
            }
            else if (command == "params") {
                showParameters();
            }
            else if (command == "eng" || command == "engine") {
                int type;
                if (iss >> type && type >= 0 && type < MAX_ENGINES) {
                    currentEngine = type;
                    const char* name = ether_get_engine_type_name(currentEngine);
                    std::cout << "✓ Switched to engine " << type << ": " << (name ? name : "Unknown") << std::endl;
                } else {
                    std::cout << "Usage: eng <0-15>" << std::endl;
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
                } else {
                    std::cout << "Usage: step <1-16> [scale_note_0-15]" << std::endl;
                }
            }
            else if (parameterNames.find(getParameterIdFromName(command)) != parameterNames.end()) {
                float value;
                if (iss >> value) {
                    setParameter(command, value);
                } else {
                    showParameter(command);
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
                    std::cout << "✓ BPM set to " << (int)newBpm << std::endl;
                } else {
                    std::cout << "Usage: bpm <60-200>" << std::endl;
                }
            }
            else if (command == "clear") {
                clearPattern();
            }
            else if (command == "clearall") {
                clearAllPatterns();
            }
            else if (command == "engines") {
                showEngines();
            }
            else {
                std::cout << "Unknown command: " << command << std::endl;
                std::cout << "Try: eng, step, play, stop, status, patterns, params, quit" << std::endl;
            }
        }
        
        std::cout << "\nGoodbye!" << std::endl;
    }
    
    void play() {
        if (!playing) {
            playing = true;
            currentStep = 0;
            std::cout << "✓ Playing all engines" << std::endl;
            
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
            std::cout << "✓ Stopped" << std::endl;
        }
    }
    
    // ... (other helper methods remain the same but with fixed variable names)
    
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
            std::cout << "✓ " << paramName << " = " << std::fixed << std::setprecision(2) << value << std::endl;
        }
    }
    
    void showParameter(const std::string& paramName) {
        int paramId = getParameterIdFromName(paramName);
        if (paramId >= 0) {
            float value = engineParameters[currentEngine][paramId];
            std::cout << paramName << " = " << std::fixed << std::setprecision(2) << value << std::endl;
        }
    }
    
    void showParameters() {
        const char* engineName = ether_get_engine_type_name(currentEngine);
        std::cout << "Parameters for Engine " << currentEngine << " (" << (engineName ? engineName : "Unknown") << "):" << std::endl;
        std::cout << "attack=" << std::fixed << std::setprecision(2) << engineParameters[currentEngine][ATTACK];
        std::cout << " decay=" << engineParameters[currentEngine][DECAY];
        std::cout << " sustain=" << engineParameters[currentEngine][SUSTAIN];
        std::cout << " release=" << engineParameters[currentEngine][RELEASE];
        std::cout << " cutoff=" << engineParameters[currentEngine][FILTER_CUTOFF];
        std::cout << " resonance=" << engineParameters[currentEngine][FILTER_RESONANCE] << std::endl;
    }
    
    void setStep(int step, int scaleNote = -1) {
        if (step < 1 || step > 16) {
            std::cout << "Step must be 1-16" << std::endl;
            return;
        }
        
        int stepIndex = step - 1;
        
        if (scaleNote == -1) {
            enginePatterns[currentEngine][stepIndex].active = !enginePatterns[currentEngine][stepIndex].active;
            std::cout << "✓ Step " << step << (enginePatterns[currentEngine][stepIndex].active ? " ON" : " OFF") << std::endl;
        } else {
            if (scaleNote >= 0 && scaleNote <= 15) {
                enginePatterns[currentEngine][stepIndex].active = true;
                enginePatterns[currentEngine][stepIndex].note = scaleIndexToMidiNote(scaleNote);
                std::cout << "✓ Step " << step << " = " << midiNoteToName(enginePatterns[currentEngine][stepIndex].note) << std::endl;
            } else {
                std::cout << "Scale note must be 0-15" << std::endl;
            }
        }
    }
    
    void clearPattern() {
        for (auto& step : enginePatterns[currentEngine]) {
            step.active = false;
        }
        const char* name = ether_get_engine_type_name(currentEngine);
        std::cout << "✓ Cleared pattern for " << (name ? name : "Unknown") << std::endl;
    }
    
    void clearAllPatterns() {
        for (int engine = 0; engine < MAX_ENGINES; engine++) {
            for (auto& step : enginePatterns[engine]) {
                step.active = false;
            }
        }
        std::cout << "✓ Cleared all patterns" << std::endl;
    }
    
    void showEngines() {
        std::cout << "Available Engines:" << std::endl;
        int count = ether_get_engine_type_count();
        for (int i = 0; i < count; i++) {
            const char* name = ether_get_engine_type_name(i);
            std::string marker = (i == currentEngine) ? " <-- current" : "";
            std::cout << "  " << i << ": " << (name ? name : "Unknown") << marker << std::endl;
        }
    }
    
    void shutdown() {
        if (running) {
            stop();
            running = false;
            
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
    SimpleCleanSequencer sequencer;
    
    if (!sequencer.initialize()) {
        return 1;
    }
    
    sequencer.run();
    return 0;
}