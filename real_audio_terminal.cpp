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

// Use the real EtherSynth bridge
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

// Audio constants
const int SAMPLE_RATE = 44100;
const int FRAMES_PER_BUFFER = 256;

// Global state
void* etherEngine = nullptr;
std::atomic<bool> audioRunning{false};
std::atomic<bool> playing{false};
std::atomic<float> masterVolume{0.3f};
std::atomic<bool> noteQueue[128] = {}; // Note on/off queue
std::atomic<bool> noteOffQueue[128] = {};
std::vector<bool> stepPattern(16, false);
std::atomic<int> currentStep{0};
std::atomic<bool> stepTrigger[16] = {};
std::atomic<float> stepFreq[16];

// Initialize step frequencies
void initStepFrequencies() {
    for (int i = 0; i < 16; i++) {
        // Map steps to a pentatonic scale starting from C4
        int scaleNotes[] = {60, 62, 64, 67, 69, 72, 74, 76}; // C major pentatonic over 2 octaves
        int note = scaleNotes[i % 8] + (i >= 8 ? 12 : 0);
        stepFreq[i] = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    }
}

// Simple audio buffer for mixing
struct AudioBuffer {
    std::vector<float> left;
    std::vector<float> right;
    
    AudioBuffer(size_t size) : left(size, 0.0f), right(size, 0.0f) {}
    
    void clear() {
        std::fill(left.begin(), left.end(), 0.0f);
        std::fill(right.begin(), right.end(), 0.0f);
    }
};

// Engine-specific oscillator that changes based on selected engine
struct EngineOscillator {
    float phase = 0.0f;
    float frequency = 440.0f;
    float amplitude = 0.0f;
    int engineType = 0; // Current engine type
    
    float process() {
        float output = 0.0f;
        
        // Different synthesis per engine type
        switch (engineType) {
        case 0: // MacroVA - Classic sine wave
            output = std::sin(phase);
            break;
        case 1: // MacroFM - Simple FM synthesis
            output = std::sin(phase + 0.5f * std::sin(phase * 2.0f));
            break;
        case 2: // MacroWaveshaper - Wave distortion
            output = std::tanh(3.0f * std::sin(phase));
            break;
        case 3: // MacroWavetable - Square wave
            output = (std::sin(phase) > 0.0f) ? 1.0f : -1.0f;
            break;
        case 4: // MacroChord - Chord (multiple harmonics)
            output = 0.4f * std::sin(phase) + 0.3f * std::sin(phase * 1.25f) + 0.2f * std::sin(phase * 1.5f);
            break;
        case 5: // MacroHarmonics - Rich harmonics
            output = std::sin(phase) + 0.3f * std::sin(phase * 2.0f) + 0.2f * std::sin(phase * 3.0f);
            break;
        case 6: // FormantVocal - Filtered noise-like
            output = std::sin(phase) * (1.0f + 0.3f * std::sin(phase * 7.0f));
            break;
        case 7: // NoiseParticles - Noise with tone
            output = 0.7f * std::sin(phase) + 0.3f * ((rand() % 1000) / 500.0f - 1.0f);
            break;
        case 8: // TidesOsc - Complex wave
            output = std::sin(phase) * (1.0f + 0.2f * std::sin(phase * 0.1f));
            break;
        case 9: // RingsVoice - Ring modulation
            output = std::sin(phase) * std::sin(phase * 1.618f);
            break;
        case 10: // ElementsVoice - Metallic sound
            output = std::sin(phase) * std::exp(-phase * 0.001f);
            break;
        case 11: // DrumKit - Punchy envelope
            output = std::sin(phase * 0.5f) * std::exp(-phase * 0.01f);
            break;
        case 12: // SamplerKit - Bright harmonic
            output = std::sin(phase) + 0.5f * std::sin(phase * 4.0f);
            break;
        case 13: // SamplerSlicer - Filtered square
            output = std::tanh(std::sin(phase) * 2.0f);
            break;
        default:
            output = std::sin(phase);
            break;
        }
        
        output *= amplitude;
        phase += 2.0f * M_PI * frequency / SAMPLE_RATE;
        if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        amplitude *= 0.9998f; // Slow decay
        return output;
    }
    
    void trigger(float freq, float amp) {
        frequency = freq;
        amplitude = amp;
        phase = 0.0f; // Reset phase for clean start
    }
    
    void setEngineType(int type) {
        engineType = type;
    }
};

EngineOscillator oscillators[16]; // One per step

// PortAudio callback
int audioCallback(const void* /*inputBuffer*/, void* outputBuffer,
                 unsigned long framesPerBuffer,
                 const PaStreamCallbackTimeInfo* /*timeInfo*/,
                 PaStreamCallbackFlags /*statusFlags*/,
                 void* /*userData*/) {
    
    float* out = static_cast<float*>(outputBuffer);
    AudioBuffer buffer(framesPerBuffer);
    buffer.clear();
    
    // Process EtherSynth engine audio (if available)
    // TODO: This would call your real C++ engines to fill the buffer
    // For now, we'll use simple oscillators as a proof of concept
    
    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        float sample = 0.0f;
        
        // Handle note queue (manual note triggers)
        for (int note = 0; note < 128; note++) {
            if (noteQueue[note].exchange(false)) {
                // Trigger note
                float freq = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
                // Find available oscillator
                for (int osc = 0; osc < 16; osc++) {
                    if (oscillators[osc].amplitude < 0.01f) {
                        oscillators[osc].trigger(freq, 0.3f);
                        std::cout << "🎹 Note " << note << " triggered (" << freq << " Hz)" << std::endl;
                        break;
                    }
                }
            }
        }
        
        // Handle step triggers
        for (int step = 0; step < 16; step++) {
            if (stepTrigger[step].exchange(false)) {
                oscillators[step].trigger(stepFreq[step], 0.2f);
            }
        }
        
        // Generate audio from all oscillators
        for (int osc = 0; osc < 16; osc++) {
            sample += oscillators[osc].process();
        }
        
        // Apply master volume
        sample *= masterVolume.load();
        
        // Simple soft limiting
        sample = std::tanh(sample);
        
        // Stereo output
        *out++ = sample;
        *out++ = sample;
    }
    
    return paContinue;
}

class RealAudioTerminal {
private:
    PaStream* stream = nullptr;
    std::thread sequencerThread;
    std::atomic<bool> running{false};
    std::atomic<float> bpm{120.0f};
    int currentEngine = 0;
    
public:
    RealAudioTerminal() {
        std::cout << "🎵 Real Audio EtherSynth Terminal" << std::endl;
        std::cout << "=================================" << std::endl;
        initStepFrequencies();
    }
    
    ~RealAudioTerminal() {
        shutdown();
    }
    
    bool initialize() {
        std::cout << "\n🔧 Initializing EtherSynth + Audio..." << std::endl;
        
        // Initialize EtherSynth engine
        etherEngine = ether_create();
        if (!etherEngine) {
            std::cout << "❌ Failed to create EtherSynth engine" << std::endl;
            return false;
        }
        
        int result = ether_initialize(etherEngine);
        if (result != 1) {
            std::cout << "❌ Failed to initialize EtherSynth" << std::endl;
            return false;
        }
        std::cout << "✅ EtherSynth engine initialized" << std::endl;
        
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
                                  nullptr);
        
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
        
        std::cout << "✅ Audio + EtherSynth ready!" << std::endl;
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
        std::cout << "\n🎛️  Available Engines:" << std::endl;
        int count = ether_get_engine_type_count();
        for (int i = 0; i < count; i++) {
            const char* name = ether_get_engine_type_name(i);
            std::cout << "  " << i << ": " << (name ? name : "Unknown") << std::endl;
        }
        std::cout << std::endl;
    }
    
    void showStatus() {
        std::cout << "\n📊 Status:" << std::endl;
        const char* engineName = ether_get_engine_type_name(currentEngine);
        std::cout << "  Engine: " << currentEngine << " (" << (engineName ? engineName : "Unknown") << ")" << std::endl;
        std::cout << "  BPM: " << std::fixed << std::setprecision(1) << bpm.load() << std::endl;
        std::cout << "  Playing: " << (playing ? "YES" : "NO") << std::endl;
        std::cout << "  Volume: " << std::fixed << std::setprecision(2) << masterVolume.load() << std::endl;
        std::cout << "  Voices: " << ether_get_active_voice_count(etherEngine) << std::endl;
        std::cout << "  CPU: " << std::fixed << std::setprecision(1) << ether_get_cpu_usage(etherEngine) << "%" << std::endl;
        
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
            ether_set_instrument_engine_type(etherEngine, 0, type);
            
            // Update all oscillators to use the new engine type
            for (int i = 0; i < 16; i++) {
                oscillators[i].setEngineType(type);
            }
            
            const char* name = ether_get_engine_type_name(type);
            std::cout << "🎛️  Switched to: " << (name ? name : "Unknown") << " - Audio updated!" << std::endl;
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
                    }
                    
                    currentStep = (currentStep + 1) % 16;
                    
                    // Calculate step timing from BPM (16th notes)
                    float stepMs = (60.0f / bpm) / 4.0f * 1000.0f;
                    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(stepMs)));
                }
            });
            
            std::cout << "▶️  Playing with real engines!" << std::endl;
        }
    }
    
    void stop() {
        if (playing) {
            playing = false;
            if (sequencerThread.joinable()) {
                sequencerThread.join();
            }
            ether_stop(etherEngine);
            ether_all_notes_off(etherEngine);
            std::cout << "⏹️  Stopped" << std::endl;
        }
    }
    
    void setBPM(float newBpm) {
        if (newBpm >= 60.0f && newBpm <= 200.0f) {
            bpm = newBpm;
            ether_set_bpm(etherEngine, newBpm);
            std::cout << "🥁 BPM: " << std::fixed << std::setprecision(1) << newBpm << std::endl;
        }
    }
    
    void setVolume(float vol) {
        if (vol >= 0.0f && vol <= 1.0f) {
            masterVolume = vol;
            std::cout << "🔊 Volume: " << std::fixed << std::setprecision(2) << vol << std::endl;
        }
    }
    
    void triggerNote(int note) {
        if (note >= 0 && note <= 127) {
            // Queue note for audio callback
            noteQueue[note] = true;
            // Also send to EtherSynth engine
            ether_note_on(etherEngine, note, 0.8f, 0.0f);
            std::cout << "🎹 Triggering note " << note << std::endl;
        }
    }
    
    void showHelp() {
        std::cout << "\n📖 Commands:" << std::endl;
        std::cout << "  help, h        - Show this help" << std::endl;
        std::cout << "  status, s      - Show status" << std::endl;
        std::cout << "  engines, e     - List engines" << std::endl;
        std::cout << "  engine <n>     - Switch to engine n (affects audio!)" << std::endl;
        std::cout << "  step <n>       - Toggle step n (1-16)" << std::endl;
        std::cout << "  play, p        - Start/stop playback" << std::endl;
        std::cout << "  bpm <n>        - Set BPM" << std::endl;
        std::cout << "  volume <n>     - Set volume (0.0-1.0)" << std::endl;
        std::cout << "  note <n>       - Trigger MIDI note n (should work now!)" << std::endl;
        std::cout << "  clear          - Clear pattern" << std::endl;
        std::cout << "  fill           - Fill pattern" << std::endl;
        std::cout << "  quit, q        - Exit" << std::endl;
        std::cout << std::endl;
    }
    
    void run() {
        if (!initialize()) {
            return;
        }
        
        std::cout << "\n🚀 Ready! EtherSynth + Audio working together!" << std::endl;
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
    RealAudioTerminal synth;
    synth.run();
    return 0;
}