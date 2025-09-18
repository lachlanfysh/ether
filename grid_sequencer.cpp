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
#include <atomic>
#include <fstream>
#include <cstdlib>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <errno.h>
#include <chrono>
// #include "encoder_control_system.h" // Removed - using simple direct approach

// Serial communication for encoder controller
class SerialPort {
private:
    int fd;
public:
    SerialPort() : fd(-1) {}

    bool open(const std::string& device) {
        fd = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd == -1) return false;

        struct termios tty;
        if (tcgetattr(fd, &tty) != 0) return false;

        cfsetospeed(&tty, B115200);
        cfsetispeed(&tty, B115200);
        tty.c_cflag &= ~PARENB;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;
        tty.c_cflag &= ~CRTSCTS;
        tty.c_cflag |= CREAD | CLOCAL;
        tty.c_lflag &= ~ICANON;
        tty.c_lflag &= ~ECHO;
        tty.c_lflag &= ~ECHOE;
        tty.c_lflag &= ~ECHONL;
        tty.c_lflag &= ~ISIG;
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
        tty.c_oflag &= ~OPOST;
        tty.c_oflag &= ~ONLCR;
        tty.c_cc[VTIME] = 1;
        tty.c_cc[VMIN] = 0;

        return tcsetattr(fd, TCSANOW, &tty) == 0;
    }

    int readData(char* buffer, int maxBytes) {
        if (fd == -1) return -1;
        return read(fd, buffer, maxBytes);
    }

    void close() {
        if (fd != -1) {
            ::close(fd);
            fd = -1;
        }
    }

    ~SerialPort() { close(); }
};

// Forward decls
bool isCurrentEngineDrum();
const char* getDisplayName(const char* technicalName);
#include <portaudio.h>
#include <lo/lo.h>
#include "src/core/Types.h"

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
    void ether_set_engine_voice_count(void* synth, int instrument, int voices);
    int ether_get_engine_voice_count(void* synth, int instrument);
    bool ether_engine_has_parameter(void* synth, int instrument, int param_id);
    float ether_get_memory_usage_kb(void* synth);
    float ether_get_cycles_480_per_buffer(void* synth);
    float ether_get_cycles_480_per_sample(void* synth);
    float ether_get_engine_cpu_pct(void* synth, int instrument);
    float ether_get_engine_cycles_480_buf(void* synth, int instrument);
    float ether_get_engine_cycles_480_smp(void* synth, int instrument);
    void ether_set_engine_fx_send(void* synth, int instrument, int which, float value);
    float ether_get_engine_fx_send(void* synth, int instrument, int which);
    void ether_set_fx_global(void* synth, int which, int param, float value);
    float ether_get_fx_global(void* synth, int which, int param);
    float ether_get_bpm(void* synth);
    int ether_get_parameter_lfo_info(void* synth, int instrument, int keyIndex, int* activeLFOs, float* currentValue);
    // LFO bridge
    // LFO controls (use active instrument internally)
    void ether_set_lfo_rate(void* synth, unsigned char lfo_id, float rate);
    void ether_set_lfo_depth(void* synth, unsigned char lfo_id, float depth);
    void ether_set_lfo_waveform(void* synth, unsigned char lfo_id, unsigned char waveform);
    void ether_set_lfo_sync(void* synth, int instrument, int lfoIndex, int syncMode);
    void ether_trigger_instrument_lfos(void* synth, int instrument);
    void ether_assign_lfo_to_param_id(void* synth, int instrument, int lfoIndex, int paramId, float depth);
    void ether_remove_lfo_assignment_by_param(void* synth, int instrument, int lfoIndex, int paramId);
}

const int MAX_ENGINES = 17;  // 13 real + 4 fallback = 17 total engines
const int GRID_WIDTH = 16;
const int GRID_HEIGHT = 8;
#ifndef BUILD_VERSION
#define BUILD_VERSION "Grid dev"
#endif
static const char* BUILD_VERSION_STR = BUILD_VERSION;

using ::ParameterID;

std::map<int, std::string> parameterNames = {
    {static_cast<int>(ParameterID::HARMONICS), "harmonics"}, {static_cast<int>(ParameterID::TIMBRE), "timbre"}, {static_cast<int>(ParameterID::MORPH), "morph"},
    {static_cast<int>(ParameterID::OSC_MIX), "oscmix"}, {static_cast<int>(ParameterID::DETUNE), "detune"}, {static_cast<int>(ParameterID::SUB_LEVEL), "sublevel"}, {static_cast<int>(ParameterID::SUB_ANCHOR), "subanchor"},
    {static_cast<int>(ParameterID::FILTER_CUTOFF), "lpf"}, {static_cast<int>(ParameterID::FILTER_RESONANCE), "resonance"},
    {static_cast<int>(ParameterID::ATTACK), "attack"}, {static_cast<int>(ParameterID::DECAY), "decay"}, {static_cast<int>(ParameterID::SUSTAIN), "sustain"}, {static_cast<int>(ParameterID::RELEASE), "release"},
    // LFO controls are global, not engine parameters — omitted from engine param list
    {static_cast<int>(ParameterID::REVERB_SIZE), "reverb_size"}, {static_cast<int>(ParameterID::REVERB_DAMPING), "reverb_damp"}, {static_cast<int>(ParameterID::REVERB_MIX), "reverb_mix"},
    {static_cast<int>(ParameterID::DELAY_TIME), "delay_time"}, {static_cast<int>(ParameterID::DELAY_FEEDBACK), "delay_fb"},
    {static_cast<int>(ParameterID::VOLUME), "volume"}, {static_cast<int>(ParameterID::PAN), "pan"}, {static_cast<int>(ParameterID::HPF), "hpf"}, {static_cast<int>(ParameterID::ACCENT_AMOUNT), "accent"}, {static_cast<int>(ParameterID::GLIDE_TIME), "glide"},
    {static_cast<int>(ParameterID::AMPLITUDE), "amp"}, {static_cast<int>(ParameterID::CLIP), "clip"}
};

// (moved method definitions appear near the end of this file)

// Global state
void* etherEngine = nullptr;
std::atomic<bool> audioRunning{false};
std::atomic<bool> playing{false};
std::atomic<bool> stepTrigger[MAX_ENGINES][16] = {};
std::atomic<bool> noteOffTrigger[MAX_ENGINES][16] = {};
std::atomic<int> currentStep{0};
std::atomic<int> activeNotes[MAX_ENGINES][16] = {};
// Suppress double-trigger when live-writing: remember previewed step
std::array<std::atomic<int>,16> drumPreviewStep;   // per pad: last step previewed
std::array<std::atomic<int>,MAX_ENGINES> melodicPreviewStep; // per engine: last step previewed
// Debounce pad presses within the 4x4 region
std::array<std::atomic<bool>,16> padIsDown; // true while key is held

// Step data per engine
struct StepData {
    bool active = false;
    int note = 60;
    float velocity = 0.6f;
};

std::array<std::vector<StepData>, MAX_ENGINES> enginePatterns;
std::array<std::map<int, float>, MAX_ENGINES> engineParameters;

// Drum engine multi-lane pattern: per-drum 16-step bitmask
std::array<uint16_t, 16> drumMasks = {0};

// Minor scale
const std::vector<int> minorScale = {
    48, 50, 51, 53, 55, 56, 58, 59, 60, 62, 63, 65, 67, 68, 70, 72
};

// Grid OSC variables
lo_server_thread grid_server = nullptr;
lo_address grid_addr = nullptr;
std::atomic<int> currentEngineRow{0}; // Which engine we're editing/playing (0..MAX_ENGINES-1)
std::atomic<bool> gridConnected{false};
std::string grid_prefix = "/monome";
int local_grid_osc_port = 7001;

// 4x4 pad UI state
std::atomic<bool> writeMode{false};
std::atomic<bool> engineHold{false};
int lastLiveNote = -1;
int liveHeldNoteByPad[16];
std::atomic<bool> playAllEngines{true};
std::atomic<bool> accentLatch{false};
int selectedDrumPad = 0;

// Map engine rows to instrument slots (0..7). -1 = unmapped
int rowToSlot[MAX_ENGINES];
int slotToRow[16];
bool rowMuted[MAX_ENGINES] = {false};
int soloEngine = -1; // -1 = no solo
bool muteHold = false;
std::chrono::steady_clock::time_point lastMutePress;

// UI requests from OSC handler (handled on engine thread)
std::atomic<bool> reqTogglePlay{false};
std::atomic<bool> reqClear{false};

// Terminal UI state
std::atomic<int> selectedParamIndex{0};
static int selectedLFOIndex = 0; // 0..7
static int lfoWaveform[8] = {0};
static float lfoRate[8] = {1,1,1,1,1,1,1,1};
static float lfoDepth[8] = {0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,0.5f};
static bool showLFOAssign = false;
static bool showLFOSettings = false;
static int lfoAssignCursor = 0; // 0..7 within assign row
static uint32_t lfoAssignMask = 0; // local UI mirror of assignments for current parameter (lower 8 bits)
std::vector<ParameterID> uiParams = {
    // Core synth & env
    ParameterID::HARMONICS, ParameterID::TIMBRE, ParameterID::MORPH,
    ParameterID::ATTACK, ParameterID::DECAY, ParameterID::SUSTAIN, ParameterID::RELEASE,
    // Tone & mix
    ParameterID::FILTER_CUTOFF, ParameterID::FILTER_RESONANCE, ParameterID::HPF,
    ParameterID::VOLUME, ParameterID::PAN, ParameterID::AMPLITUDE, ParameterID::CLIP,
    // Performance
    ParameterID::ACCENT_AMOUNT, ParameterID::GLIDE_TIME
};
// Computed each frame based on current engine support
std::vector<ParameterID> visibleParams;

// Number of non-engine-parameter rows shown after the main parameter list:
//  voices (1) + per-engine sends (2) + global reverb (3) + global delay (3) = 9
static inline int extraMenuRows() {
    int extra = 9;
    if (isCurrentEngineDrum()) extra += 1; // drum edit row
    return extra;
}

static void rebuildVisibleParams() {
    visibleParams.clear();
    int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
    for (auto pid : uiParams) {
        int pidInt = static_cast<int>(pid);
        // Always show global post filters
        if (pid == ParameterID::FILTER_CUTOFF || pid == ParameterID::FILTER_RESONANCE || pid == ParameterID::HPF) {
            visibleParams.push_back(pid);
            continue;
        }
        // Query engine support for others
        if (ether_engine_has_parameter(etherEngine, slot, pidInt)) {
            visibleParams.push_back(pid);
        }
    }
    // Clamp selection index to full menu range (params + extras)
    int maxIndex = static_cast<int>(visibleParams.size()) + extraMenuRows();
    if (selectedParamIndex < 0) selectedParamIndex = 0;
    if (selectedParamIndex > maxIndex) selectedParamIndex = maxIndex;
}

// Drum edit UI state
int drumEditPad = 0; // 0..15
int drumEditField = 0; // 0=decay,1=tune,2=level,3=pan

extern "C" {
    void ether_drum_set_param(void* synth, int instrument, int pad, int which, float value);
}

// Terminal raw mode helpers
termios orig_termios;
void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}
void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1; // 100ms timeout
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void setStdinNonblocking() {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void applyParamToEngine(int engine, ParameterID pid, float value) {
    value = std::max(0.0f, std::min(1.0f, value));
    engineParameters[engine][static_cast<int>(pid)] = value;
    if (etherEngine) {
        int slot = (engine >= 0 && engine < MAX_ENGINES) ? rowToSlot[engine] : 0;
        if (slot < 0) slot = 0;
        ether_set_active_instrument(etherEngine, slot);
        ether_set_instrument_parameter(etherEngine, slot, static_cast<int>(pid), value);
    }
}

void drawFixedUI() {
    // Clear screen and home cursor
    printf("\x1b[2J\x1b[H");
    rebuildVisibleParams();
    // Use friendly display name
    const char* techName = ether_get_engine_type_name(currentEngineRow);
    const char* name = getDisplayName(techName);
    const char* cat = nullptr;
    float cpu = ether_get_cpu_usage(etherEngine);
    float memMB = ether_get_memory_usage_kb(etherEngine) / 1024.0f;
    float cycBuf = ether_get_cycles_480_per_buffer(etherEngine);
    float cycSmp = ether_get_cycles_480_per_sample(etherEngine);
    printf("Ether Grid Sequencer | %s | Engine: %d %s%s%s%s | BPM: %3.0f | %s | CPU: %4.1f%% | MEM: %4.1f MB | Cyc@480: %6.0f/buf, %5.0f/smp\n",
           BUILD_VERSION_STR,
           currentEngineRow.load(),
           name ? name : "?",
           cat ? " [" : "",
           cat ? cat : "",
           cat ? "]" : "",
           ether_get_bpm(etherEngine),
           playing ? "PLAY" : "STOP", cpu, memMB, cycBuf, cycSmp);
    // Per-slot CPU% line (first 8 slots)
    printf("CPU slots: ");
    for (int s=0;s<8;s++){ float p = ether_get_engine_cpu_pct(etherEngine, s); printf("%d:%3.0f%% ", s, p); }
    printf("\n");
    
    // Show FM algo if applicable (uses TIMBRE as algo selector 0..7)
    int algo = (int)std::floor(engineParameters[currentEngineRow][static_cast<int>(ParameterID::TIMBRE)] * 8.0f);
    if (algo < 0) algo = 0; if (algo > 7) algo = 7;
    bool isFM = (ether_get_engine_type_name(currentEngineRow) && std::string(ether_get_engine_type_name(currentEngineRow)).find("FM") != std::string::npos);
    if (isFM) {
        static const char* fmAlgoNames[8] = {
            "Stack 1-2-3-4", "Stack 1-2-2-3", "Bright 1-3-2-5", "Mellow 1-1.5-2-3",
            "FB 1-2-1-2", "Sub 0.5-1-2-3", "Clang 1-2.5-3.5-5", "Organ 1-1-1-1"
        };
        printf("FM Algo: %d/8 - %s (TIMBRE)\n", algo+1, fmAlgoNames[algo]);
    } else {
        printf("\n");
    }
    
    // Parameter table (no scroll)
    printf("Params (↑/↓ select, ←/→ adjust, space play/stop, w write, c clear, q quit)\n");
    for (size_t i = 0; i < visibleParams.size(); ++i) {
        ParameterID pid = visibleParams[i];
        float v = engineParameters[currentEngineRow][static_cast<int>(pid)];
        const char* sel = (selectedParamIndex == (int)i) ? ">" : " ";
        std::string label = parameterNames[static_cast<int>(pid)];
        // Rename LPF cutoff to "brightness" for Classic 4-Op FM
        bool isFM4Op = (ether_get_engine_type_name(currentEngineRow) && std::string(ether_get_engine_type_name(currentEngineRow)).find("Classic4OpFM") != std::string::npos);
        if (isFM4Op && pid == ParameterID::FILTER_CUTOFF) label = "brightness";
        printf("%s %-12s : %0.2f\n", sel, label.c_str(), v);
    }
    // Extra: Voices control + FX controls
    int baseIdx = (int)visibleParams.size();
    int voices = ether_get_engine_voice_count(etherEngine, 0);
    const char* selv = (selectedParamIndex == baseIdx) ? ">" : " ";
    printf("%s %-12s : %d\n", selv, "voices", voices);
    // Per-engine FX sends
    float sRev = ether_get_engine_fx_send(etherEngine, currentEngineRow, 0);
    float sDel = ether_get_engine_fx_send(etherEngine, currentEngineRow, 1);
    const char* sels1 = (selectedParamIndex == baseIdx+1) ? ">" : " ";
    const char* sels2 = (selectedParamIndex == baseIdx+2) ? ">" : " ";
    printf("%s %-12s : %0.2f\n", sels1, "rev_send", sRev);
    printf("%s %-12s : %0.2f\n", sels2, "del_send", sDel);
    // Global FX
    float rvTime = ether_get_fx_global(etherEngine, 0, 0);
    float rvDamp = ether_get_fx_global(etherEngine, 0, 1);
    float rvMix  = ether_get_fx_global(etherEngine, 0, 2);
    float dlTime = ether_get_fx_global(etherEngine, 1, 0);
    float dlFB   = ether_get_fx_global(etherEngine, 1, 1);
    float dlMix  = ether_get_fx_global(etherEngine, 1, 2);
    const char* selg0 = (selectedParamIndex == baseIdx+3) ? ">" : " ";
    const char* selg1 = (selectedParamIndex == baseIdx+4) ? ">" : " ";
    const char* selg2 = (selectedParamIndex == baseIdx+5) ? ">" : " ";
    const char* selg3 = (selectedParamIndex == baseIdx+6) ? ">" : " ";
    const char* selg4 = (selectedParamIndex == baseIdx+7) ? ">" : " ";
    const char* selg5 = (selectedParamIndex == baseIdx+8) ? ">" : " ";
    printf("%s %-12s : %0.2f\n", selg0, "rvb_size", rvTime);
    printf("%s %-12s : %0.2f\n", selg1, "rvb_damp", rvDamp);
    printf("%s %-12s : %0.2f\n", selg2, "rvb_mix",  rvMix);
    printf("%s %-12s : %0.2f\n", selg3, "dly_time", dlTime);
    printf("%s %-12s : %0.2f\n", selg4, "dly_fb",   dlFB);
    printf("%s %-12s : %0.2f\n", selg5, "dly_mix",  dlMix);
    // LFO quick status
    printf("LFO sel: %2d  wf=%2d  rate=%4.2fHz  depth=%4.2f  ([/]=select  v=wave  r/R=rate  d/D=depth  L=assign menu  S=settings)\n",
           selectedLFOIndex+1, lfoWaveform[selectedLFOIndex], lfoRate[selectedLFOIndex], lfoDepth[selectedLFOIndex]);

    if (showLFOAssign) {
        printf("\nLFO Assign — toggle with X, arrows move, L to close\n");
        for (int idx=0; idx<8; ++idx) {
            bool on = (lfoAssignMask >> idx) & 1u;
            bool sel = (lfoAssignCursor == idx);
            printf("%s[%c]%2d ", sel?">":" ", on?'x':' ', idx+1);
        }
        printf("\n");
    }

    if (showLFOSettings) {
        printf("\nLFO Settings — %2d  wf=%d  rate=%4.2fHz  depth=%4.2f  (v/r/R/d/D/k=KeySync e=Env)\n",
               selectedLFOIndex+1, lfoWaveform[selectedLFOIndex], lfoRate[selectedLFOIndex], lfoDepth[selectedLFOIndex]);
    }
    printf("  play mode     : %s (press 'a' to toggle)\n", playAllEngines ? "ALL" : "CURRENT");
    // Mute/Solo
    printf("  mute/solo     : hold grid y0x4 for mute view; double-tap to solo current row\n");
    size_t idxAfter = uiParams.size()+1;
    if (isCurrentEngineDrum()) {
        const char* seld = (selectedParamIndex == (int)idxAfter) ? ">" : " ";
        const char* fieldNames[4] = {"decay","tune","level","pan"};
        printf("%s drum pad    : %d\n", seld, drumEditPad);
        printf("  edit field   : %s\n", fieldNames[drumEditField]);
        printf("  tip: press a drum pad to select; enter cycles field\n");
        printf("  ←/→ adjust, [/] pad-, ] pad+  (level/pan 0..1, tune -1..1)\n");
    }
    
    // Pattern line
    printf("\nPattern: ");
    for (int i = 0; i < 16; ++i) {
        bool on = enginePatterns[currentEngineRow][i].active;
        if (playing && i == currentStep) {
            printf("[%c]", on ? '#' : '.');
        } else {
            printf(" %c ", on ? '#' : '.');
        }
    }
    printf("\n");
        if (isCurrentEngineDrum()) {
            printf("Drum hits at step %2d: ", currentStep.load()+1);
            for (int pad = 0; pad < 16; ++pad) {
                bool on = (drumMasks[pad] >> currentStep) & 1u;
                printf("%c", on ? '#' : '.');
            }
            printf("\n");
        }
    fflush(stdout);
}

bool isCurrentEngineDrum() {
    const char* name = ether_get_engine_type_name(currentEngineRow);
    if (!name) return false;
    std::string n(name);
    for (auto &c : n) c = std::tolower(c);
    return n.find("drum") != std::string::npos;
}

bool isEngineDrum(int row) {
    const char* name = ether_get_engine_type_name(row);
    if (!name) return false;
    std::string n(name);
    for (auto &c : n) c = std::tolower(c);
    return n.find("drum") != std::string::npos;
}

const char* getDisplayName(const char* technicalName) {
    if (!technicalName) return "Unknown";
    
    static const std::map<std::string, const char*> displayNames = {
        {"MacroVA", "Analog VA"},
        {"MacroFM", "FM Synth"},
        {"MacroWaveshaper", "Waveshaper"},
        {"MacroWavetable", "Wavetable"},
        {"MacroChord", "Multi-Voice"},
        {"MacroHarmonics", "Morph"},
        {"FormantVocal", "Vocal"},
        {"NoiseParticles", "Noise"},
        {"TidesOsc", "Morph"},
        {"RingsVoice", "Modal"},
        {"ElementsVoice", "Exciter"},
        {"SlideAccentBass", "Acid"},
        {"Classic4OpFM", "Classic FM"},
        {"Granular", "Granular"},
        {"DrumKit(fallback)", "Drum Kit"},
        {"SamplerKit(fallback)", "Sampler"},
        {"SamplerSlicer(fallback)", "Sampler"},
        {"SerialHPLP(fallback)", "Filter"}
    };
    
    auto it = displayNames.find(technicalName);
    return (it != displayNames.end()) ? it->second : technicalName;
}

// General MIDI-ish 16-pad map: K S Rim Clap Toms HH Crash Ride
const std::array<int,16> DRUM_PAD_NOTES = {
    36, // 0 Kick A (Bass Drum 1)
    38, // 1 Snare A (Acoustic Snare)
    49, // 2 Crash Cymbal 1
    39, // 3 Hand Clap
    41, // 4 Low Tom
    45, // 5 Mid Tom
    48, // 6 High Tom
    37, // 7 Rimshot / Side Stick
    42, // 8 Closed Hat
    44, // 9 Pedal Hat
    46, // 10 Open Hat
    51, // 11 Ride Cymbal 1
    56, // 12 Cowbell
    35, // 13 Kick B (Acoustic Bass Drum)
    40, // 14 Snare B (Electric Snare)
    70  // 15 Shaker (Maracas)
};

// Forward declare
void register_grid_with_device(int device_port) {
    if (grid_addr) {
        lo_address_free(grid_addr);
        grid_addr = nullptr;
    }
    grid_addr = lo_address_new("127.0.0.1", std::to_string(device_port).c_str());
    if (!grid_addr) {
        std::cout << "Grid: failed to create address for device port " << device_port << std::endl;
        return;
    }
    // sys messages are NOT prefixed
    lo_send(grid_addr, "/sys/host", "s", "127.0.0.1");
    lo_send(grid_addr, "/sys/port", "i", local_grid_osc_port);
    lo_send(grid_addr, "/sys/prefix", "s", grid_prefix.c_str());
    lo_send(grid_addr, "/sys/info", "");
    gridConnected = true;
    std::cout << "Grid: registered with device on port " << device_port << " using prefix " << grid_prefix << std::endl;
}

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
        // Tighter defaults to avoid long sustains across engines
        engineParameters[engine][static_cast<int>(ParameterID::ATTACK)]  = 0.10f;
        engineParameters[engine][static_cast<int>(ParameterID::DECAY)]   = 0.10f;
        engineParameters[engine][static_cast<int>(ParameterID::SUSTAIN)] = 0.10f;
        engineParameters[engine][static_cast<int>(ParameterID::RELEASE)] = 0.10f;
        engineParameters[engine][static_cast<int>(ParameterID::FILTER_CUTOFF)] = 0.8f;
        engineParameters[engine][static_cast<int>(ParameterID::FILTER_RESONANCE)] = 0.2f;
        engineParameters[engine][static_cast<int>(ParameterID::VOLUME)] = 0.8f;
        engineParameters[engine][static_cast<int>(ParameterID::PAN)] = 0.5f;
        engineParameters[engine][static_cast<int>(ParameterID::REVERB_MIX)] = 0.3f;
    }
    // Initialize preview suppression to -1 (none)
    for (int p = 0; p < 16; ++p) drumPreviewStep[p] = -1;
    for (int e = 0; e < MAX_ENGINES; ++e) melodicPreviewStep[e] = -1;
    for (int p = 0; p < 16; ++p) padIsDown[p] = false;
}

// OSC handlers
int grid_key_handler(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data) {
    static bool firstMessage = true;
    if (firstMessage) {
        std::cout << "Grid: Received first OSC message from grid device: " << path << std::endl;
        firstMessage = false;
    }

    // Layout: top row y=0 function keys; 4x4 pad at x=0..3, y=1..4
    static const int PAD_ORIGIN_X = 0;
    static const int PAD_ORIGIN_Y = 1;
    static const int PAD_W = 4;
    static const int PAD_H = 4;
    static bool liveHeldInit = false;
    if (!liveHeldInit) { for (int i=0;i<PAD_W*PAD_H;i++) liveHeldNoteByPad[i] = -1; liveHeldInit = true; }

    auto padIndexFromXY = [&](int x, int y) -> int {
        if (x < PAD_ORIGIN_X || x >= PAD_ORIGIN_X + PAD_W) return -1;
        if (y < PAD_ORIGIN_Y || y >= PAD_ORIGIN_Y + PAD_H) return -1;
        int px = x - PAD_ORIGIN_X;
        int py = y - PAD_ORIGIN_Y;
        return py * PAD_W + px; // 0..15
    };

    auto noteFromPadIndex = [&](int idx) -> int {
        if (idx < 0) return 60;
        int scaleIndex = std::max(0, std::min((int)minorScale.size()-1, idx));
        return minorScale[scaleIndex];
    };

    // Handle /monome/grid/key messages
    if (std::string(path).find("/grid/key") != std::string::npos && argc >= 3) {
        int x = argv[0]->i;
        int y = argv[1]->i;
        int state = argv[2]->i;

        // Function row (y==0)
        if (y == 0) {
            if (state == 1) { // on press actions
                if (x == 0) {
                    reqTogglePlay = true;
                } else if (x == 1) {
                    writeMode = !writeMode.load();
                    std::cout << (writeMode ? "Write mode ON" : "Write mode OFF") << std::endl;
                } else if (x == 2) {
                    engineHold = true;
                } else if (x == 3) {
                    reqClear = true;
                } else if (x == 4) {
                    auto now = std::chrono::steady_clock::now();
                    if (lastMutePress.time_since_epoch().count() != 0) {
                        auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastMutePress).count();
                        if (dt < 300) {
                            if (soloEngine == currentEngineRow) {
                                soloEngine = -1;
                                std::cout << "Solo OFF" << std::endl;
                            } else {
                                soloEngine = currentEngineRow;
                                std::cout << "Solo engine " << currentEngineRow << std::endl;
                            }
                        }
                    }
                    lastMutePress = now;
                    muteHold = true;
                }
            } else if (state == 0) {
                if (x == 2) engineHold = false; // release engine hold
                if (x == 4) muteHold = false;    // release mute view
            }
            return 0;
        }

        // Inside 4x4 pad region
        int padIdx = padIndexFromXY(x, y);
        // Accent toggle button (below mute): x=4,y=1
        if (x == 4 && y == 1) {
            if (state == 1) {
                accentLatch = !accentLatch.load();
                std::cout << (accentLatch ? "Accent ON" : "Accent OFF") << std::endl;
            }
            return 0;
        }
        if (padIdx >= 0) {
            // Debounce: ignore repeat press events while held
            if (state == 1) {
                bool wasDown = padIsDown[padIdx].exchange(true);
                if (wasDown) return 0;
            } else if (state == 0) {
                padIsDown[padIdx] = false;
            }
            // PO-style live write: when playing and write is ON, pad presses place hits at current step
            if (playing && writeMode && state == 1) {
                if (isCurrentEngineDrum()) {
                    drumMasks[padIdx] |= (1u << currentStep);
                    drumPreviewStep[padIdx] = currentStep.load();
                    int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                    ether_set_active_instrument(etherEngine, slot);
                    float vel = accentLatch ? 1.0f : 0.9f;
                    ether_note_on(etherEngine, DRUM_PAD_NOTES[padIdx], vel, 0.0f);
                    ether_trigger_instrument_lfos(etherEngine, slot);
                    return 0;
                } else {
                    int engine = currentEngineRow;
                    int liveNote = noteFromPadIndex(padIdx);
                    lastLiveNote = liveNote;
                    enginePatterns[engine][currentStep].active = true;
                    enginePatterns[engine][currentStep].note = liveNote;
                    melodicPreviewStep[engine] = currentStep.load();
                    int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                    ether_set_active_instrument(etherEngine, slot);
                    float vel = accentLatch ? 1.0f : 0.8f;
                    ether_note_on(etherEngine, liveNote, vel, 0.0f);
                    ether_trigger_instrument_lfos(etherEngine, slot);
                    return 0;
                }
            }
            if (muteHold) {
                if (state == 1) {
                    int eng = std::min(MAX_ENGINES - 1, padIdx);
                    rowMuted[eng] = !rowMuted[eng];
                    std::cout << "Row " << eng << (rowMuted[eng]?" muted":" unmuted") << std::endl;
                }
                return 0;
            }
            if (engineHold) {
                if (state == 1) {
                    int newEngine = std::min(MAX_ENGINES - 1, padIdx);
                    currentEngineRow = newEngine;
                    if (etherEngine) {
                        int slot = rowToSlot[newEngine]; if (slot < 0) slot = 0;
                        ether_set_active_instrument(etherEngine, slot);
                    }
                    const char* techName = ether_get_engine_type_name(newEngine);
                    const char* name = getDisplayName(techName);
                    std::cout << "Engine -> " << newEngine << ": " << (name ? name : "Unknown") << std::endl;
                }
                return 0;
            }

            // Drum engine behavior
            if (isCurrentEngineDrum()) {
                if (writeMode) {
                    if (state == 1) {
                        int stepIdx = padIdx;
                        drumMasks[selectedDrumPad] ^= (1u << stepIdx);
                    }
                    return 0;
                }
                else {
                    int note = DRUM_PAD_NOTES[padIdx];
                    if (state == 1) {
                        selectedDrumPad = padIdx; // arm this drum
                        // Jump the terminal menu to the drum pad editor row
                        selectedParamIndex = (int)uiParams.size()+1;
                        int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                        ether_set_active_instrument(etherEngine, slot);
                        float vel = accentLatch ? 1.0f : 0.9f;
                        ether_note_on(etherEngine, note, vel, 0.0f);
                    } else if (state == 0) {
                        int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                        ether_set_active_instrument(etherEngine, slot);
                        ether_note_off(etherEngine, note);
                    }
                    return 0;
                }
            }

            // Melodic live play
            {
                int liveNote = noteFromPadIndex(padIdx);
                if (writeMode && state == 1) {
                    // Toggle step and bake lastLiveNote if available
                    int engine = currentEngineRow;
                    enginePatterns[engine][padIdx].active = !enginePatterns[engine][padIdx].active;
                    if (lastLiveNote >= 0) {
                        enginePatterns[engine][padIdx].note = lastLiveNote;
                    } else {
                        enginePatterns[engine][padIdx].note = liveNote;
                    }
                    std::cout << "Step " << (padIdx+1) << (enginePatterns[engine][padIdx].active?" ON":" OFF") << std::endl;
                    return 0;
                }
                if (state == 1) {
                    lastLiveNote = liveNote;
                    int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                    ether_set_active_instrument(etherEngine, slot);
                    float vel = accentLatch ? 1.0f : 0.8f;
                    ether_note_on(etherEngine, liveNote, vel, 0.0f);
                    ether_trigger_instrument_lfos(etherEngine, slot);
                    liveHeldNoteByPad[padIdx] = liveNote;
                    return 0;
                } else if (state == 0) {
                    int held = liveHeldNoteByPad[padIdx];
                    if (held >= 0) {
                        int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                        ether_set_active_instrument(etherEngine, slot);
                        ether_note_off(etherEngine, held);
                        liveHeldNoteByPad[padIdx] = -1;
                    }
                    return 0;
                }
            }

            if (writeMode) {
                if (state == 1) {
                    // Toggle step and bake lastLiveNote if available
                    int engine = currentEngineRow;
                    enginePatterns[engine][padIdx].active = !enginePatterns[engine][padIdx].active;
                    if (lastLiveNote >= 0) {
                        enginePatterns[engine][padIdx].note = lastLiveNote;
                    }
                    std::cout << "Step " << (padIdx+1) << (enginePatterns[engine][padIdx].active?" ON":" OFF");
                    if (enginePatterns[engine][padIdx].active) {
                        std::cout << " note=" << enginePatterns[engine][padIdx].note;
                    }
                    std::cout << std::endl;
                }
                return 0;
            }

            // Notes mode (live play)
            int note = noteFromPadIndex(padIdx);
            if (state == 1) {
                lastLiveNote = note;
                int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                ether_set_active_instrument(etherEngine, slot);
                ether_note_on(etherEngine, note, 0.8f, 0.0f);
                ether_trigger_instrument_lfos(etherEngine, slot);
                liveHeldNoteByPad[padIdx] = note;
            } else if (state == 0) {
                int held = liveHeldNoteByPad[padIdx];
                if (held >= 0) {
                    int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                    ether_set_active_instrument(etherEngine, slot);
                    ether_note_off(etherEngine, held);
                    liveHeldNoteByPad[padIdx] = -1;
                }
            }
            return 0;
        }
    }
    return 0;
}

// Handle serialosc device announcements: /serialosc/device ssi (id, type, port)
int serialosc_device_handler(const char *path, const char *types, lo_arg **argv, int argc, lo_message /*msg*/, void * /*user_data*/) {
    std::cout << "serialosc: " << path << " types=" << (types ? types : "") << " argc=" << argc << std::endl;
    if (argc >= 3 && types && std::string(types).rfind("ssi", 0) == 0) {
        const char* id = &argv[0]->s;
        const char* type = &argv[1]->s;
        int port = argv[2]->i;
        std::cout << "serialosc device: id=" << (id ? id : "?") << " type=" << (type ? type : "?") << " port=" << port << std::endl;
        register_grid_with_device(port);
    } else if (argc >= 3) {
        int port = argv[2]->i;
        std::cout << "serialosc device: (untyped) port=" << port << std::endl;
        register_grid_with_device(port);
    }
    return 0;
}

void updateGridLEDs() {
    if (!gridConnected || !grid_addr) return;

    // Clear all LEDs first
    lo_send(grid_addr, (grid_prefix + "/grid/led/all").c_str(), "i", 0);

    static const int PAD_ORIGIN_X = 0;
    static const int PAD_ORIGIN_Y = 1;
    static const int PAD_W = 4;
    static const int PAD_H = 4;

    // Mute hold view: show 4x4 engine mute states
    if (muteHold) {
        for (int i = 0; i < PAD_W * PAD_H; i++) {
            int x = PAD_ORIGIN_X + (i % PAD_W);
            int y = PAD_ORIGIN_Y + (i / PAD_W);
            int brightness = 0;
            int eng = i;
            if (eng < MAX_ENGINES) {
                if (soloEngine >= 0) {
                    brightness = (eng == soloEngine) ? 15 : 2;
                } else {
                    brightness = rowMuted[eng] ? 2 : 12;
                }
            }
            if (brightness > 0) {
                lo_send(grid_addr, (grid_prefix + "/grid/led/level/set").c_str(), "iii", x, y, brightness);
            }
        }
        // Function row indicators
        lo_send(grid_addr, (grid_prefix + "/grid/led/level/set").c_str(), "iii", 4, 0, 15);
        return;
    }

    // Accent toggle indicator (below mute key): x=4,y=1
    {
        int ax = 4, ay = 1;
        int b = accentLatch ? 12 : 3;
        lo_send(grid_addr, (grid_prefix + "/grid/led/level/set").c_str(), "iii", ax, ay, b);
    }

    // Function row indicators
    // Play button at (0,0)
    lo_send(grid_addr, (grid_prefix + "/grid/led/level/set").c_str(), "iii", 0, 0, playing ? 15 : 4);
    // Write toggle at (1,0)
    lo_send(grid_addr, (grid_prefix + "/grid/led/level/set").c_str(), "iii", 1, 0, writeMode ? 15 : 4);
    // Engine hold at (2,0)
    lo_send(grid_addr, (grid_prefix + "/grid/led/level/set").c_str(), "iii", 2, 0, engineHold ? 15 : 4);
    // Clear at (3,0)
    lo_send(grid_addr, (grid_prefix + "/grid/led/level/set").c_str(), "iii", 3, 0, 4);

    // 4x4 panel
    if (engineHold) {
        // Show engine selection across 4x4
        for (int i = 0; i < PAD_W * PAD_H; i++) {
            int x = PAD_ORIGIN_X + (i % PAD_W);
            int y = PAD_ORIGIN_Y + (i / PAD_W);
            int b = (i == currentEngineRow) ? 15 : 4;
            lo_send(grid_addr, (grid_prefix + "/grid/led/level/set").c_str(), "iii", x, y, b);
        }
        return;
    }

    if (writeMode) {
        int engine = currentEngineRow;
        for (int i = 0; i < PAD_W * PAD_H; i++) {
            int x = PAD_ORIGIN_X + (i % PAD_W);
            int y = PAD_ORIGIN_Y + (i / PAD_W);
            int b = 0;
            if (isCurrentEngineDrum()) {
                // In write mode, show selected drum's 16-step pattern across time
                bool on = (drumMasks[selectedDrumPad] >> i) & 1u;
                b = on ? 12 : ((playing && i == currentStep) ? 2 : 0);
            } else {
                // Ghost steps from other engines dim
                bool ghost = false;
                for (int e = 0; e < MAX_ENGINES; ++e) {
                    if (e == engine) continue;
                    if (enginePatterns[e][i].active) { ghost = true; break; }
                }
                if (ghost) b = 3; // dim ghost
                // Current engine step brighter
                if (enginePatterns[engine][i].active) {
                    b = (playing && i == currentStep) ? 15 : 8;
                } else if (playing && i == currentStep) {
                    b = std::max(b, 2); // ensure playhead visible
                }
            }
            if (b > 0) {
                lo_send(grid_addr, (grid_prefix + "/grid/led/level/set").c_str(), "iii", x, y, b);
            }
        }
        return;
    }

    // Notes mode: show current engine pattern brightly, others as ghost; playhead dimly
    for (int i = 0; i < PAD_W * PAD_H; i++) {
        int x = PAD_ORIGIN_X + (i % PAD_W);
        int y = PAD_ORIGIN_Y + (i / PAD_W);
        int b = 0;
        if (isCurrentEngineDrum()) {
            // Bright for steps written to selected drum pad
            bool on = ((drumMasks[selectedDrumPad] >> i) & 1u) != 0;
            if (on) b = 12;
        } else {
            // Bright for current engine's own steps
            if (enginePatterns[currentEngineRow][i].active) {
                b = 12;
            } else {
                // Dim ghost if any other engine has a step here
                bool ghost = false;
                for (int e = 0; e < MAX_ENGINES; ++e) {
                    if (e == currentEngineRow) continue;
                    if (enginePatterns[e][i].active) { ghost = true; break; }
                }
                if (ghost) b = 3;
            }
        }
        if (playing && i == currentStep) b = std::max(b, 2);
        if (b > 0) {
            lo_send(grid_addr, (grid_prefix + "/grid/led/level/set").c_str(), "iii", x, y, b);
        }
    }
}

// PortAudio callback (same as before)
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
                    int slot = rowToSlot[engine]; if (slot < 0) slot = 0;
                    ether_set_active_instrument(etherEngine, slot);
                    ether_note_on(etherEngine, enginePatterns[engine][step].note, 
                                 enginePatterns[engine][step].velocity, 0.0f);
                    activeNotes[engine][step] = enginePatterns[engine][step].note;
                }
            }
            
            if (noteOffTrigger[engine][step].exchange(false)) {
                int note = activeNotes[engine][step].exchange(-1);
                if (note >= 0) {
                    int slot = rowToSlot[engine]; if (slot < 0) slot = 0;
                    ether_set_active_instrument(etherEngine, slot);
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

class GridSequencer {
private:
    PaStream* stream = nullptr;
    std::thread sequencerThread;
    std::thread ledUpdateThread;
    std::atomic<bool> running{false};
    std::atomic<float> bpm{120.0f};

    // Encoder state - implementing your control architecture
    SerialPort encoderSerial;
    std::string serialLineBuffer;

    // Encoder 4 state (menu navigation + edit mode)
    bool editMode = false;

    // Encoders 1-3 state (parameter latching)
    struct ParameterLatch {
        bool active = false;
        ParameterID paramId;
        std::string paramName;
        int engineRow = -1;  // Store which engine row this parameter is latched to
    };
    ParameterLatch paramLatches[3]; // For encoders 1-3

    // Double-press detection
    struct ButtonState {
        std::chrono::steady_clock::time_point lastPressTime;
        bool pendingSinglePress = false;
    };
    ButtonState buttonStates[4]; // For encoders 1-4
    static constexpr int DOUBLE_PRESS_TIMEOUT_MS = 300;
public:
    // (prototypes consolidated above)
    
    GridSequencer() {
        initializeEnginePatterns();
        setupEncoderSystem(); // Simple connection like encoder_demo
    }
    
    ~GridSequencer() {
        shutdownSequencer();
    }
    
    bool initializeGrid();
    
    bool initialize();
    void showStatus();
    void run();
    void play();
    void stop();
    void clearPattern();
    void shutdownSequencer();

    // Encoder control system methods
    void setupEncoderSystem();
    void updateEngineFromEncoderChange(const std::string& param_id, float delta);
    void syncMenuWithEncoder(const std::string& param_id);
    void processEncoderInput();
    void handleEncoder4Turn(int delta);
    void handleParameterEncoderTurn(int encoder_id, int delta);
    void handleEncoderButtonPress(int encoder_id);
    void updateButtonTimers();
    void processPendingButtonPress(int encoder_id);
    void handleEncoderLatch(int encoder_id);
    void adjustLatchedParameter(int enc_index, int delta);

    // Direct encoder parameter control
    struct EncoderLatch {
        bool active = false;
        int paramIndex = -1;  // Index into visibleParams
        std::string paramName;
    };
    EncoderLatch encoderLatches[4];  // For encoders 1-4
};

    /* BEGIN_OLD_INCLASS
        std::cout << "\n=== EtherSynth Grid Sequencer ===" << std::endl;
        std::cout << "Grid: " << (gridConnected ? "Connected" : "Disconnected") << std::endl;
        
        const char* engineName = ether_get_engine_type_name(currentEngineRow);
        std::cout << "Current Engine Row: " << currentEngineRow << " (" << (engineName ? engineName : "Unknown") << ")" << std::endl;
        std::cout << "BPM: " << std::fixed << std::setprecision(1) << bpm.load();
        std::cout << " | " << (playing ? "PLAYING" : "STOPPED");
        if (playing) {
            std::cout << " | Step: " << (currentStep + 1) << "/16";
        }
        std::cout << std::endl;
        
        // Show current engine's pattern
        std::cout << "Pattern [" << currentEngineRow << "]: ";
        for (int i = 0; i < 16; i++) {
            if (enginePatterns[currentEngineRow][i].active) {
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
    }
    
    // Removed broken duplicate run() function - using GridSequencer::run() instead
                if (c == 'q') { quit = true; break; }
                if (c == ' ') { reqTogglePlay = true; }
                if (c == 'w' || c == 'W') { writeMode = !writeMode.load(); }
                if (c == 'c' || c == 'C') { reqClear = true; }
                if (c == 'a' || c == 'A') { playAllEngines = !playAllEngines.load(); }
                if (c == '[') { if (--selectedLFOIndex < 0) selectedLFOIndex = 0; }
                if (c == ']') { if (++selectedLFOIndex > 7) selectedLFOIndex = 7; }
                if (c == 'L') {
                    showLFOAssign = !showLFOAssign;
                    // refresh local mask from engine
                    if (showLFOAssign && selectedParamIndex < (int)visibleParams.size()) {
                        int activeMask = 0; float cur = 0.0f;
                        int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                        ParameterID pid = visibleParams[selectedParamIndex];
                        ether_get_parameter_lfo_info(etherEngine, slot, static_cast<int>(pid), &activeMask, &cur);
                        lfoAssignMask = static_cast<uint32_t>(activeMask);
                        lfoAssignCursor = 0;
                    }
                }
                if (c == 'S') { showLFOSettings = !showLFOSettings; }
                if (c == 'v' || c == 'V') {
                    int wf = (lfoWaveform[selectedLFOIndex] + 1) % 12; // cycle 0..11
                    lfoWaveform[selectedLFOIndex] = wf;
                    int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                    ether_set_lfo_waveform(etherEngine, (unsigned char)selectedLFOIndex, (unsigned char)wf);
                }
                if (c == 'r') {
                    lfoRate[selectedLFOIndex] = std::max(0.01f, lfoRate[selectedLFOIndex] * 0.9f);
                    int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                    ether_set_lfo_rate(etherEngine, (unsigned char)selectedLFOIndex, lfoRate[selectedLFOIndex]);
                }
                if (c == 'R') {
                    lfoRate[selectedLFOIndex] = std::min(50.0f, lfoRate[selectedLFOIndex] * 1.1f);
                    int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                    ether_set_lfo_rate(etherEngine, (unsigned char)selectedLFOIndex, lfoRate[selectedLFOIndex]);
                }
                if (c == 'd') {
                    lfoDepth[selectedLFOIndex] = std::max(0.0f, lfoDepth[selectedLFOIndex] - 0.05f);
                    int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                    ether_set_lfo_depth(etherEngine, (unsigned char)selectedLFOIndex, lfoDepth[selectedLFOIndex]);
                }
                if (c == 'D') {
                    lfoDepth[selectedLFOIndex] = std::min(1.0f, lfoDepth[selectedLFOIndex] + 0.05f);
                    int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                    ether_set_lfo_depth(etherEngine, (unsigned char)selectedLFOIndex, lfoDepth[selectedLFOIndex]);
                }
                if (c == 'm') {
                    // Quick-assign selected LFO to current parameter
                    if (selectedParamIndex < (int)visibleParams.size()) {
                        ParameterID pid = visibleParams[selectedParamIndex];
                        int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                        ether_assign_lfo_to_param_id(etherEngine, slot, selectedLFOIndex, static_cast<int>(pid), lfoDepth[selectedLFOIndex]);
                        lfoAssignMask |= (1u << selectedLFOIndex);
                    }
                }
                if (c == 'M') {
                    // Quick-remove selected LFO from current parameter
                    if (selectedParamIndex < (int)visibleParams.size()) {
                        ParameterID pid = visibleParams[selectedParamIndex];
                        int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                        ether_remove_lfo_assignment_by_param(etherEngine, slot, selectedLFOIndex, static_cast<int>(pid));
                        lfoAssignMask &= ~(1u << selectedLFOIndex);
                    }
                }
                if (c == 'e' || c == 'E') {
                    // Envelope mode latch for selected LFO
                    int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                    ether_set_lfo_sync(etherEngine, slot, selectedLFOIndex, 4); // ENVELOPE
                }
                if (c == 'k' || c == 'K') {
                    // Key-sync mode for selected LFO (restarts on key)
                    int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                    ether_set_lfo_sync(etherEngine, slot, selectedLFOIndex, 2); // KEY_SYNC
                }
                if (isCurrentEngineDrum()) {
                    if (c == '[') { if (--drumEditPad < 0) drumEditPad = 0; }
                    if (c == ']') { if (++drumEditPad > 15) drumEditPad = 15; }
                    if (c == '\n') { drumEditField = (drumEditField+1)%4; }
                }
                // Vim-style unified navigation (matches arrow key behavior)
                if (c == 'j') {
                    if (showLFOAssign) {
                        lfoAssignCursor = std::min(7, lfoAssignCursor + 1);
                    } else {
                        int maxIdx = (int)visibleParams.size() + extraMenuRows();
                        int idx = selectedParamIndex + 1; if (idx > maxIdx) idx = maxIdx; selectedParamIndex = idx;
                    }
                } else if (c == 'k') {
                    if (showLFOAssign) {
                        lfoAssignCursor = std::max(0, lfoAssignCursor - 1);
                    } else {
                        int maxIdx = (int)visibleParams.size() + extraMenuRows();
                        int idx = selectedParamIndex - 1; if (idx < 0) idx = 0; selectedParamIndex = idx;
                    }
                } else if (c == '\x1b') { // ESC sequence
                    char seq[2];
                    if (read(STDIN_FILENO, &seq[0], 1) != 1) break;
                    if (read(STDIN_FILENO, &seq[1], 1) != 1) break;
                    if (seq[0] == '[') {
                if (seq[1] == 'A') { // Up - unified menu navigation
                    if (showLFOAssign) {
                        lfoAssignCursor = std::max(0, lfoAssignCursor - 1);
                    } else {
                        int maxIdx = (int)visibleParams.size() + extraMenuRows();
                        int idx = selectedParamIndex - 1; if (idx < 0) idx = 0; selectedParamIndex = idx;
                    }
                } else if (seq[1] == 'B') { // Down - unified menu navigation
                    if (showLFOAssign) {
                        lfoAssignCursor = std::min(7, lfoAssignCursor + 1);
                    } else {
                        int maxIdx = (int)visibleParams.size() + extraMenuRows();
                        int idx = selectedParamIndex + 1; if (idx > maxIdx) idx = maxIdx; selectedParamIndex = idx;
                    }
                        } else if (seq[1] == 'C') { // Right
                            if (showLFOAssign) {
                                lfoAssignCursor = std::min(7, lfoAssignCursor + 1);
                                break;
                            }
                            if (isCurrentEngineDrum() && selectedParamIndex == (int)visibleParams.size()+1) {
                                // adjust drum field
                                float step = (drumEditField==1) ? 0.05f : 0.02f;
                                ether_drum_set_param(etherEngine, 0, drumEditPad, drumEditField, step +  (drumEditField==1?0.0f:0.0f));
                            } else if (selectedParamIndex < (int)visibleParams.size()) {
                                ParameterID pid = visibleParams[selectedParamIndex];
                                float v = engineParameters[currentEngineRow][static_cast<int>(pid)];
                                bool fm = std::string(ether_get_engine_type_name(currentEngineRow)).find("FM") != std::string::npos;
                                if (pid == ParameterID::TIMBRE && fm) {
                                    int algo = (int)std::floor(v * 8.0f + 1e-6);
                                    algo = std::min(7, algo + 1);
                                    v = (algo + 0.5f) / 8.0f;
                                } else {
                                    v += 0.02f;
                                }
                                applyParamToEngine(currentEngineRow, pid, v);
                            } else {
                                int base = (int)visibleParams.size();
                                if (selectedParamIndex == base) {
                                    int voices = ether_get_engine_voice_count(etherEngine, 0);
                                    voices = std::min(16, voices + 1);
                                    ether_set_engine_voice_count(etherEngine, 0, voices);
                                } else if (selectedParamIndex == base+1) {
                                    float v = std::min(1.0f, ether_get_engine_fx_send(etherEngine, currentEngineRow, 0) + 0.02f);
                                    ether_set_engine_fx_send(etherEngine, currentEngineRow, 0, v);
                                } else if (selectedParamIndex == base+2) {
                                    float v = std::min(1.0f, ether_get_engine_fx_send(etherEngine, currentEngineRow, 1) + 0.02f);
                                    ether_set_engine_fx_send(etherEngine, currentEngineRow, 1, v);
                                } else if (selectedParamIndex >= base+3 && selectedParamIndex <= base+8) {
                                    int which = (selectedParamIndex<=base+5)?0:1;
                                    int param = (selectedParamIndex-(base+3)) % 3;
                                    float cur = ether_get_fx_global(etherEngine, which, param);
                                    ether_set_fx_global(etherEngine, which, param, std::min(1.0f, cur+0.02f));
                                }
                            }
                        } else if (seq[1] == 'D') { // Left
                            if (showLFOAssign) {
                                lfoAssignCursor = std::max(0, lfoAssignCursor - 1);
                                break;
                            }
                            if (isCurrentEngineDrum() && selectedParamIndex == (int)visibleParams.size()+1) {
                                float step = (drumEditField==1) ? -0.05f : -0.02f;
                                ether_drum_set_param(etherEngine, 0, drumEditPad, drumEditField, step + 0.0f);
                            } else if (selectedParamIndex < (int)visibleParams.size()) {
                                ParameterID pid = visibleParams[selectedParamIndex];
                                float v = engineParameters[currentEngineRow][static_cast<int>(pid)];
                                bool fm = std::string(ether_get_engine_type_name(currentEngineRow)).find("FM") != std::string::npos;
                                if (pid == ParameterID::TIMBRE && fm) {
                                    int algo = (int)std::floor(v * 8.0f + 1e-6);
                                    algo = std::max(0, algo - 1);
                                    v = (algo + 0.5f) / 8.0f;
                                } else {
                                    v -= 0.02f;
                                }
                                applyParamToEngine(currentEngineRow, pid, v);
                            } else {
                                int base = (int)visibleParams.size();
                                if (selectedParamIndex == base) {
                                    int voices = ether_get_engine_voice_count(etherEngine, 0);
                                    voices = std::max(1, voices - 1);
                                    ether_set_engine_voice_count(etherEngine, 0, voices);
                                } else if (selectedParamIndex == base+1) {
                                    float v = std::max(0.0f, ether_get_engine_fx_send(etherEngine, currentEngineRow, 0) - 0.02f);
                                    ether_set_engine_fx_send(etherEngine, currentEngineRow, 0, v);
                                } else if (selectedParamIndex == base+2) {
                                    float v = std::max(0.0f, ether_get_engine_fx_send(etherEngine, currentEngineRow, 1) - 0.02f);
                                    ether_set_engine_fx_send(etherEngine, currentEngineRow, 1, v);
                                } else if (selectedParamIndex >= base+3 && selectedParamIndex <= base+8) {
                                    int which = (selectedParamIndex<=base+5)?0:1;
                                    int param = (selectedParamIndex-(base+3)) % 3;
                                    float cur = ether_get_fx_global(etherEngine, which, param);
                                    ether_set_fx_global(etherEngine, which, param, std::max(0.0f, cur-0.02f));
                                }
                            }
                        }
                        }
                    }
                }
                if (showLFOAssign && (c == 'x' || c == 'X' || c == '\n')) {
                    if (selectedParamIndex < (int)visibleParams.size()) {
                        ParameterID pid = visibleParams[selectedParamIndex];
                        int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                        bool on = ((lfoAssignMask >> lfoAssignCursor) & 1u) != 0;
                        if (on) {
                            ether_remove_lfo_assignment_by_param(etherEngine, slot, lfoAssignCursor, static_cast<int>(pid));
                            lfoAssignMask &= ~(1u << lfoAssignCursor);
                        } else {
                            ether_assign_lfo_to_param_id(etherEngine, slot, lfoAssignCursor, static_cast<int>(pid), lfoDepth[lfoAssignCursor]);
                            lfoAssignMask |= (1u << lfoAssignCursor);
                        }
                    }
                }
            }
            drawFixedUI();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        disableRawMode();
        std::cout << "\nGoodbye!" << std::endl;
    }
    
    void play();
        if (!playing) {
            playing = true;
            currentStep = 0;
            std::cout << "✓ Playing all engines" << std::endl;
            
            sequencerThread = std::thread([this]() {
                while (playing) {
                    if (isCurrentEngineDrum()) {
                        // Trigger any drum whose bit at currentStep is set; let engine manage decay
                        int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                        if (!(soloEngine>=0 && currentEngineRow!=soloEngine) && !rowMuted[currentEngineRow]) {
                            ether_set_active_instrument(etherEngine, slot);
                            bool chNow = ((drumMasks[8] >> currentStep) & 1u) || ((drumMasks[9] >> currentStep) & 1u);
                            for (int pad = 0; pad < 16; ++pad) {
                                if ((drumMasks[pad] >> currentStep) & 1u) {
                                    // Skip if we just previewed this pad at this step
                                    int prev = drumPreviewStep[pad].load();
                                    if (prev == currentStep) { drumPreviewStep[pad] = -1; continue; }
                                    if (chNow && pad == 10) continue; // choke OH when CH/PH hit same step
                                    float vel = accentLatch ? 1.0f : 0.9f;
                                    ether_note_on(etherEngine, DRUM_PAD_NOTES[pad], vel, 0.0f);
                                }
                            }
                        }
                    } else {
                        if (playAllEngines) {
                            // Trigger all rows (drums + melodic)
                            for (int s = 0; s < 16; ++s) {
                                int row = slotToRow[s];
                                if (row < 0) continue;
                                if (soloEngine >= 0 && row != soloEngine) continue;
                                if (rowMuted[row]) continue;
                                // Determine if this slot is a drum row by engine type
                                int etype = ether_get_instrument_engine_type(etherEngine, s);
                                bool isDrum = (etype == static_cast<int>(EngineType::DRUM_KIT));
                                if (isDrum) {
                                    // Trigger any drum whose bit at currentStep is set on this row's slot
                                    int slot = s;
                                    ether_set_active_instrument(etherEngine, slot);
                                    bool chNow = ((drumMasks[8] >> currentStep) & 1u) || ((drumMasks[9] >> currentStep) & 1u);
                                    for (int pad = 0; pad < 16; ++pad) {
                                        if ((drumMasks[pad] >> currentStep) & 1u) {
                                            // Skip if we just previewed this pad at this step
                                            int prev = drumPreviewStep[pad].load();
                                            if (prev == currentStep) { drumPreviewStep[pad] = -1; continue; }
                                            if (chNow && pad == 10) continue; // choke OH when CH/PH hit same step
                                            float vel = accentLatch ? 1.0f : 0.9f;
                                            ether_note_on(etherEngine, DRUM_PAD_NOTES[pad], vel, 0.0f);
                                        }
                                    }
                                } else {
                                    if (enginePatterns[row][currentStep].active) {
                                        // Skip if just previewed live in this step
                                        int prev = melodicPreviewStep[row].load();
                                        if (prev == currentStep) { melodicPreviewStep[row] = -1; }
                                        else { stepTrigger[row][currentStep] = true; }
                                    }
                                }
                            }
                            // Schedule melodic note offs only (drums manage decay)
                            std::thread([this]() {
                                float stepMs = (60.0f / bpm) / 4.0f * 1000.0f;
                                for (int s = 0; s < 16; ++s) {
                                    int row = slotToRow[s]; if (row < 0) continue;
                                    if (soloEngine >= 0 && row != soloEngine) continue;
                                    if (rowMuted[row]) continue;
                                    int etype = ether_get_instrument_engine_type(etherEngine, s);
                                    bool isDrum = (etype == static_cast<int>(EngineType::DRUM_KIT));
                                    if (!isDrum) {
                                        float releaseParam = engineParameters[row][static_cast<int>(ParameterID::RELEASE)];
                                        float noteOffMs = stepMs * (0.1f + releaseParam * 0.8f);
                                        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(noteOffMs)));
                                        if (playing) {
                                            noteOffTrigger[row][currentStep] = true;
                                        }
                                    }
                                }
                            }).detach();
                        } else {
                            int engine = currentEngineRow;
                            if (soloEngine >= 0 && engine != soloEngine) {
                                // skip
                            } else if (!rowMuted[engine] && enginePatterns[engine][currentStep].active) {
                                stepTrigger[engine][currentStep] = true;
                                std::thread([this, engine]() {
                                    float stepMs = (60.0f / bpm) / 4.0f * 1000.0f;
                                    float releaseParam = engineParameters[engine][static_cast<int>(ParameterID::RELEASE)];
                                    float noteOffMs = stepMs * (0.1f + releaseParam * 0.8f);
                                    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(noteOffMs)));
                                    if (playing) {
                                        noteOffTrigger[engine][currentStep] = true;
                                    }
                                }).detach();
                            }
                        }
                    }
                    
                    currentStep = (currentStep + 1) % 16;
                    
                    float stepMs = (60.0f / bpm) / 4.0f * 1000.0f;
                    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(stepMs)));
                }
            });
        }
    }
    
    void stop();
        if (playing) {
            playing = false;
            ether_all_notes_off(etherEngine);
            if (sequencerThread.joinable()) {
                sequencerThread.join();
            }
            std::cout << "✓ Stopped" << std::endl;
        }
    }
    
    void clearPattern();
        if (isCurrentEngineDrum()) {
            for (auto &m : drumMasks) m = 0;
        } else {
            for (auto& step : enginePatterns[currentEngineRow]) {
                step.active = false;
            }
        }
        const char* name = ether_get_engine_type_name(currentEngineRow);
        std::cout << "✓ Cleared pattern for " << (name ? name : "Unknown") << std::endl;
    }
    
    void shutdownSequencer();
        if (running) {
            stop();
            running = false;
            
            if (sequencerThread.joinable()) {
                sequencerThread.join();
            }
            
            if (ledUpdateThread.joinable()) {
                ledUpdateThread.join();
            }
            
            if (grid_server) {
                lo_server_thread_stop(grid_server);
                lo_server_thread_free(grid_server);
                grid_server = nullptr;
            }
            
            if (grid_addr) {
                lo_address_free(grid_addr);
                grid_addr = nullptr;
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

};
*/

// ===== GridSequencer method definitions (moved from class) =====

bool GridSequencer::initializeGrid() {
    grid_server = lo_server_thread_new(std::to_string(local_grid_osc_port).c_str(), nullptr);
    if (!grid_server) { std::cout << "Failed to create OSC server" << std::endl; return false; }
    lo_server_thread_add_method(grid_server, (grid_prefix + "/grid/key").c_str(), "iii", grid_key_handler, nullptr);
    lo_server_thread_add_method(grid_server, "/serialosc/device", "ssi", serialosc_device_handler, nullptr);
    lo_server_thread_add_method(grid_server, "/serialosc/add", "ssi", serialosc_device_handler, nullptr);
    lo_server_thread_add_method(grid_server, (grid_prefix + "/sys/port").c_str(), "", [](const char*,const char*, lo_arg**,int, lo_message, void*)->int{ return 0; }, nullptr);
    lo_server_thread_add_method(grid_server, (grid_prefix + "/sys/host").c_str(), "", [](const char*,const char*, lo_arg**,int, lo_message, void*)->int{ return 0; }, nullptr);
    lo_server_thread_add_method(grid_server, (grid_prefix + "/sys/id").c_str(), "", [](const char*,const char*, lo_arg**,int, lo_message, void*)->int{ return 0; }, nullptr);
    lo_server_thread_add_method(grid_server, (grid_prefix + "/sys/size").c_str(), "", [](const char*,const char*, lo_arg**,int, lo_message, void*)->int{ return 0; }, nullptr);
    lo_server_thread_add_method(grid_server, (grid_prefix + "/*").c_str(), nullptr, [](const char*,const char*, lo_arg**,int, lo_message, void*)->int{ return 0; }, nullptr);
    lo_server_thread_start(grid_server);
    grid_addr = lo_address_new("127.0.0.1", "12002");
    lo_send(grid_addr, "/serialosc/list", "si", "127.0.0.1", local_grid_osc_port);
    lo_send(grid_addr, "/serialosc/notify", "si", "127.0.0.1", local_grid_osc_port);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "Grid setup complete - listening on port " << local_grid_osc_port << std::endl;
    return true;
}

bool GridSequencer::initialize() {
    etherEngine = ether_create();
    if (!etherEngine) return false;
    ether_initialize(etherEngine);
    ether_set_master_volume(etherEngine, 0.8f);
    ether_play(etherEngine);
    for (int r = 0; r < MAX_ENGINES; ++r) rowToSlot[r] = -1;
    for (int s = 0; s < 8; ++s) slotToRow[s] = -1;
    for (int r = 0; r < MAX_ENGINES; ++r) rowToSlot[r] = -1;
    for (int s = 0; s < 16; ++s) slotToRow[s] = -1;
    for (int r = 0; r < 16 && r < MAX_ENGINES; ++r) { rowToSlot[r] = r; slotToRow[r] = r; ether_set_active_instrument(etherEngine, r); ether_set_instrument_engine_type(etherEngine, r, r); }
    int curSlot = (currentEngineRow >= 0 && currentEngineRow < MAX_ENGINES) ? rowToSlot[currentEngineRow] : 0;
    if (curSlot < 0) curSlot = 0; ether_set_active_instrument(etherEngine, curSlot);
    for (int engine = 0; engine < MAX_ENGINES; engine++) { int slot = rowToSlot[engine]; if (slot < 0) continue; ether_set_instrument_engine_type(etherEngine, slot, engine); for (auto& kv : engineParameters[engine]) { ether_set_instrument_parameter(etherEngine, slot, kv.first, kv.second); } }
    PaError err = Pa_Initialize(); if (err != paNoError) return false;
    err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, 48000, 128, audioCallback, nullptr); if (err != paNoError) return false;
    err = Pa_StartStream(stream); if (err != paNoError) return false;
    audioRunning = true; running = true;
    initializeGrid();
    ledUpdateThread = std::thread([this]() {
        while (running) { if (reqTogglePlay.exchange(false)) { if (!playing) this->play(); else this->stop(); } if (reqClear.exchange(false)) { this->clearPattern(); } updateGridLEDs(); std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
    });
    return true;
}

void GridSequencer::showStatus() {
    std::cout << "\n=== EtherSynth Grid Sequencer ===" << std::endl;
    std::cout << "Grid: " << (gridConnected ? "Connected" : "Disconnected") << std::endl;
    const char* techName = ether_get_engine_type_name(currentEngineRow);
    const char* engineName = getDisplayName(techName);
    std::cout << "Current Engine Row: " << currentEngineRow << " (" << (engineName ? engineName : "Unknown") << ")" << std::endl;
    std::cout << "BPM: " << std::fixed << std::setprecision(1) << bpm.load();
    std::cout << " | " << (playing ? "PLAYING" : "STOPPED");
    if (playing) { std::cout << " | Step: " << (currentStep + 1) << "/16"; }
    std::cout << std::endl;
    std::cout << "Pattern [" << currentEngineRow << "]: ";
    for (int i = 0; i < 16; i++) { if (enginePatterns[currentEngineRow][i].active) { if (i == currentStep && playing) std::cout << "[" << (i+1) << "]"; else std::cout << " " << (i+1) << " "; } else { if (i == currentStep && playing) std::cout << "[·]"; else std::cout << " · "; } }
    std::cout << std::endl;
}

void GridSequencer::run() {
    enableRawMode(); setStdinNonblocking(); running = true; bool quit = false;
    while (running && !quit) {
        // Process encoder input and handle button timing
        processEncoderInput();
        updateButtonTimers();

        usleep(1000); // 1ms delay like encoder_demo

        char c; while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == 'q') { quit = true; break; }
        if (c == ' ') { reqTogglePlay = true; }
        if (c == 'w' || c == 'W') { writeMode = !writeMode.load(); }
        if (c == 'c' || c == 'C') { reqClear = true; }
        if (c == 'a' || c == 'A') { playAllEngines = !playAllEngines.load(); }
        if (c == '[') { if (--selectedLFOIndex < 0) selectedLFOIndex = 0; }
        if (c == ']') { if (++selectedLFOIndex > 7) selectedLFOIndex = 7; }
        if (c == 'L') { showLFOAssign = !showLFOAssign; if (showLFOAssign && selectedParamIndex < (int)visibleParams.size()) { int activeMask = 0; float cur = 0.0f; int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0; ParameterID pid = visibleParams[selectedParamIndex]; ether_get_parameter_lfo_info(etherEngine, slot, static_cast<int>(pid), &activeMask, &cur); lfoAssignMask = static_cast<uint32_t>(activeMask); lfoAssignCursor = 0; } }
        if (c == 'S') { showLFOSettings = !showLFOSettings; }
        if (c == 'v' || c == 'V') { int wf = (lfoWaveform[selectedLFOIndex] + 1) % 12; lfoWaveform[selectedLFOIndex] = wf; ether_set_lfo_waveform(etherEngine, (unsigned char)selectedLFOIndex, (unsigned char)wf); }
        if (c == 'r') { lfoRate[selectedLFOIndex] = std::max(0.01f, lfoRate[selectedLFOIndex] * 0.9f); ether_set_lfo_rate(etherEngine, (unsigned char)selectedLFOIndex, lfoRate[selectedLFOIndex]); }
        if (c == 'R') { lfoRate[selectedLFOIndex] = std::min(50.0f, lfoRate[selectedLFOIndex] * 1.1f); ether_set_lfo_rate(etherEngine, (unsigned char)selectedLFOIndex, lfoRate[selectedLFOIndex]); }
        if (c == 'd') { lfoDepth[selectedLFOIndex] = std::max(0.0f, lfoDepth[selectedLFOIndex] - 0.05f); ether_set_lfo_depth(etherEngine, (unsigned char)selectedLFOIndex, lfoDepth[selectedLFOIndex]); }
        if (c == 'D') { lfoDepth[selectedLFOIndex] = std::min(1.0f, lfoDepth[selectedLFOIndex] + 0.05f); ether_set_lfo_depth(etherEngine, (unsigned char)selectedLFOIndex, lfoDepth[selectedLFOIndex]); }
        
        // LFO assignment trigger (x/X/Enter when in LFO assign mode)
        if (showLFOAssign && (c == 'x' || c == 'X' || c == '\n')) {
            if (selectedParamIndex < (int)visibleParams.size()) {
                ParameterID pid = visibleParams[selectedParamIndex];
                int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                bool on = ((lfoAssignMask >> lfoAssignCursor) & 1u) != 0;
                if (on) {
                    ether_remove_lfo_assignment_by_param(etherEngine, slot, lfoAssignCursor, static_cast<int>(pid));
                    lfoAssignMask &= ~(1u << lfoAssignCursor);
                } else {
                    ether_assign_lfo_to_param_id(etherEngine, slot, lfoAssignCursor, static_cast<int>(pid), lfoDepth[lfoAssignCursor]);
                    lfoAssignMask |= (1u << lfoAssignCursor);
                }
            }
        }
        
        // Unified menu navigation - vim keys and arrows
        if (c == 'j') {
            if (showLFOAssign) {
                lfoAssignCursor = std::min(7, lfoAssignCursor + 1);
            } else {
                int maxIdx = (int)visibleParams.size() + extraMenuRows();
                int idx = selectedParamIndex + 1; if (idx > maxIdx) idx = maxIdx; selectedParamIndex = idx;
            }
        } else if (c == 'k') {
            if (showLFOAssign) {
                lfoAssignCursor = std::max(0, lfoAssignCursor - 1);
            } else {
                int maxIdx = (int)visibleParams.size() + extraMenuRows();
                int idx = selectedParamIndex - 1; if (idx < 0) idx = 0; selectedParamIndex = idx;
            }
        } else if (c == '\x1b') { // ESC sequence for arrow keys
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;
            if (seq[0] == '[') {
                if (seq[1] == 'A') { // Up - unified menu navigation
                    if (showLFOAssign) {
                        lfoAssignCursor = std::max(0, lfoAssignCursor - 1);
                    } else {
                        int maxIdx = (int)visibleParams.size() + extraMenuRows();
                        int idx = selectedParamIndex - 1; if (idx < 0) idx = 0; selectedParamIndex = idx;
                    }
                } else if (seq[1] == 'B') { // Down - unified menu navigation  
                    if (showLFOAssign) {
                        lfoAssignCursor = std::min(7, lfoAssignCursor + 1);
                    } else {
                        int maxIdx = (int)visibleParams.size() + extraMenuRows();
                        int idx = selectedParamIndex + 1; if (idx > maxIdx) idx = maxIdx; selectedParamIndex = idx;
                    }
                } else if (seq[1] == 'C') { // Right - parameter adjustment
                    if (showLFOAssign) {
                        lfoAssignCursor = std::min(7, lfoAssignCursor + 1);
                    } else if (selectedParamIndex < (int)visibleParams.size()) {
                        ParameterID pid = visibleParams[selectedParamIndex];
                        float v = engineParameters[currentEngineRow][static_cast<int>(pid)];
                        engineParameters[currentEngineRow][static_cast<int>(pid)] = std::min(1.0f, v + 0.02f);
                        int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                        ether_set_instrument_parameter(etherEngine, slot, static_cast<int>(pid), engineParameters[currentEngineRow][static_cast<int>(pid)]);
                    } else {
                        // FX section adjustment (voices, sends, global FX)
                        int base = (int)visibleParams.size();
                        if (selectedParamIndex == base) {
                            int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                            int voices = ether_get_engine_voice_count(etherEngine, slot);
                            voices = std::min(16, voices + 1);
                            ether_set_engine_voice_count(etherEngine, slot, voices);
                        } else if (selectedParamIndex == base+1) {
                            float v = std::min(1.0f, ether_get_engine_fx_send(etherEngine, currentEngineRow, 0) + 0.02f);
                            ether_set_engine_fx_send(etherEngine, currentEngineRow, 0, v);
                        } else if (selectedParamIndex == base+2) {
                            float v = std::min(1.0f, ether_get_engine_fx_send(etherEngine, currentEngineRow, 1) + 0.02f);
                            ether_set_engine_fx_send(etherEngine, currentEngineRow, 1, v);
                        } else if (selectedParamIndex >= base+3 && selectedParamIndex <= base+8) {
                            int which = (selectedParamIndex<=base+5)?0:1;
                            int param = (selectedParamIndex-(base+3)) % 3;
                            float cur = ether_get_fx_global(etherEngine, which, param);
                            ether_set_fx_global(etherEngine, which, param, std::min(1.0f, cur+0.02f));
                        }
                    }
                } else if (seq[1] == 'D') { // Left - parameter adjustment
                    if (showLFOAssign) {
                        lfoAssignCursor = std::max(0, lfoAssignCursor - 1);
                    } else if (selectedParamIndex < (int)visibleParams.size()) {
                        ParameterID pid = visibleParams[selectedParamIndex];
                        float v = engineParameters[currentEngineRow][static_cast<int>(pid)];
                        engineParameters[currentEngineRow][static_cast<int>(pid)] = std::max(0.0f, v - 0.02f);
                        int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                        ether_set_instrument_parameter(etherEngine, slot, static_cast<int>(pid), engineParameters[currentEngineRow][static_cast<int>(pid)]);
                    } else {
                        // FX section adjustment (voices, sends, global FX)
                        int base = (int)visibleParams.size();
                        if (selectedParamIndex == base) {
                            int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                            int voices = ether_get_engine_voice_count(etherEngine, slot);
                            voices = std::max(1, voices - 1);
                            ether_set_engine_voice_count(etherEngine, slot, voices);
                        } else if (selectedParamIndex == base+1) {
                            float v = std::max(0.0f, ether_get_engine_fx_send(etherEngine, currentEngineRow, 0) - 0.02f);
                            ether_set_engine_fx_send(etherEngine, currentEngineRow, 0, v);
                        } else if (selectedParamIndex == base+2) {
                            float v = std::max(0.0f, ether_get_engine_fx_send(etherEngine, currentEngineRow, 1) - 0.02f);
                            ether_set_engine_fx_send(etherEngine, currentEngineRow, 1, v);
                        } else if (selectedParamIndex >= base+3 && selectedParamIndex <= base+8) {
                            int which = (selectedParamIndex<=base+5)?0:1;
                            int param = (selectedParamIndex-(base+3)) % 3;
                            float cur = ether_get_fx_global(etherEngine, which, param);
                            ether_set_fx_global(etherEngine, which, param, std::max(0.0f, cur-0.02f));
                        }
                    }
                }
            }
        }
    } drawFixedUI(); std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
    disableRawMode(); std::cout << "\nGoodbye!" << std::endl;
}

void GridSequencer::play() {
    if (!playing) {
        std::cout << "[DEBUG] CORRECT GridSequencer::play() called - setting playing=true, currentStep=0" << std::endl;
        // Log to file for crash analysis
        std::ofstream logFile("grid_sequencer_debug.log", std::ios::app);
        if (logFile.is_open()) {
            logFile << "[" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << "] Starting play() - setting playing=true, currentStep=0" << std::endl;
            logFile.close();
        }
        playing = true;
        currentStep = 0;
        std::cout << "✓ Playing all engines" << std::endl;
        
        std::cout << "[DEBUG] Creating sequencer thread..." << std::endl;
        sequencerThread = std::thread([this]() {
            std::cout << "[DEBUG] Sequencer thread started, entering loop" << std::endl;
            while (playing) {
                if (isCurrentEngineDrum()) {
                    // Trigger any drum whose bit at currentStep is set; let engine manage decay
                    int slot = rowToSlot[currentEngineRow]; if (slot < 0) slot = 0;
                    if (!(soloEngine>=0 && currentEngineRow!=soloEngine) && !rowMuted[currentEngineRow]) {
                        ether_set_active_instrument(etherEngine, slot);
                        bool chNow = ((drumMasks[8] >> currentStep) & 1u) || ((drumMasks[9] >> currentStep) & 1u);
                        for (int pad = 0; pad < 16; ++pad) {
                            if ((drumMasks[pad] >> currentStep) & 1u) {
                                // Skip if we just previewed this pad at this step
                                int prev = drumPreviewStep[pad].load();
                                if (prev == currentStep) { drumPreviewStep[pad] = -1; continue; }
                                if (chNow && pad == 10) continue; // choke OH when CH/PH hit same step
                                float vel = accentLatch ? 1.0f : 0.9f;
                                ether_note_on(etherEngine, DRUM_PAD_NOTES[pad], vel, 0.0f);
                            }
                        }
                    }
                } else {
                    if (playAllEngines) {
                        // Trigger all rows (drums + melodic)
                        for (int s = 0; s < 16; ++s) {
                            int row = slotToRow[s]; if (row < 0) continue;
                            if (soloEngine >= 0 && row != soloEngine) continue;
                            if (rowMuted[row]) continue;
                            
                            int slot = rowToSlot[row]; if (slot < 0) slot = 0;
                            ether_set_active_instrument(etherEngine, slot);
                            bool isDrum = isEngineDrum(row);
                            
                            if (isDrum) {
                                bool chNow = ((drumMasks[8] >> currentStep) & 1u) || ((drumMasks[9] >> currentStep) & 1u);
                                for (int pad = 0; pad < 16; ++pad) {
                                    if ((drumMasks[pad] >> currentStep) & 1u) {
                                        // Skip if we just previewed this pad at this step
                                        int prev = drumPreviewStep[pad].load();
                                        if (prev == currentStep) { drumPreviewStep[pad] = -1; continue; }
                                        if (chNow && pad == 10) continue; // choke OH when CH/PH hit same step
                                        float vel = accentLatch ? 1.0f : 0.9f;
                                        ether_note_on(etherEngine, DRUM_PAD_NOTES[pad], vel, 0.0f);
                                    }
                                }
                            } else {
                                if (enginePatterns[row][currentStep].active) {
                                    // Skip if just previewed live in this step
                                    int prev = melodicPreviewStep[row].load();
                                    if (prev == currentStep) { melodicPreviewStep[row] = -1; }
                                    else { stepTrigger[row][currentStep] = true; }
                                }
                            }
                        }
                        // Schedule melodic note offs only (drums manage decay)
                        std::thread([this]() {
                            float stepMs = (60.0f / bpm) / 4.0f * 1000.0f;
                            for (int s = 0; s < 16; ++s) {
                                int row = slotToRow[s]; if (row < 0) continue;
                                if (soloEngine >= 0 && row != soloEngine) continue;
                                if (rowMuted[row]) continue;
                                
                                int slot = rowToSlot[row]; if (slot < 0) slot = 0;
                                bool isDrum = isEngineDrum(row);
                                if (!isDrum) {
                                    float releaseParam = engineParameters[row][static_cast<int>(ParameterID::RELEASE)];
                                    float noteOffMs = stepMs * (0.1f + releaseParam * 0.8f);
                                    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(noteOffMs)));
                                    if (playing) {
                                        noteOffTrigger[row][currentStep] = true;
                                    }
                                }
                            }
                        }).detach();
                    } else {
                        int engine = currentEngineRow;
                        if (soloEngine >= 0 && engine != soloEngine) {
                            // skip
                        } else if (!rowMuted[engine] && enginePatterns[engine][currentStep].active) {
                            stepTrigger[engine][currentStep] = true;
                            std::thread([this, engine]() {
                                float stepMs = (60.0f / bpm) / 4.0f * 1000.0f;
                                float releaseParam = engineParameters[engine][static_cast<int>(ParameterID::RELEASE)];
                                float noteOffMs = stepMs * (0.1f + releaseParam * 0.8f);
                                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(noteOffMs)));
                                if (playing) {
                                    noteOffTrigger[engine][currentStep] = true;
                                }
                            }).detach();
                        }
                    }
                }
                
                currentStep = (currentStep + 1) % 16;
                
                float stepMs = (60.0f / bpm) / 4.0f * 1000.0f;
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(stepMs)));
            }
            std::cout << "[DEBUG] Sequencer thread exiting loop" << std::endl;
        });
        std::cout << "[DEBUG] play() complete - thread started" << std::endl;
    }
}

void GridSequencer::stop() {
    if (playing) { 
        std::cout << "[DEBUG] Starting stop() - setting playing=false" << std::endl;
        playing = false; 
        std::cout << "[DEBUG] Calling ether_all_notes_off" << std::endl;
        ether_all_notes_off(etherEngine); 
        std::cout << "[DEBUG] Checking if sequencer thread is joinable..." << std::endl;
        if (sequencerThread.joinable()) {
            std::cout << "[DEBUG] Joining sequencer thread..." << std::endl;
            sequencerThread.join();
            std::cout << "[DEBUG] Sequencer thread joined successfully" << std::endl;
        }
        std::cout << "✓ Stopped" << std::endl; 
    }
}

void GridSequencer::clearPattern() {
    if (isCurrentEngineDrum()) { for (auto &m : drumMasks) m = 0; } else { for (auto& step : enginePatterns[currentEngineRow]) step.active = false; } const char* techName = ether_get_engine_type_name(currentEngineRow); const char* name = getDisplayName(techName); std::cout << "✓ Cleared pattern for " << (name ? name : "Unknown") << std::endl;
}

void GridSequencer::shutdownSequencer() {
    if (running) { stop(); running = false; if (sequencerThread.joinable()) sequencerThread.join(); if (ledUpdateThread.joinable()) ledUpdateThread.join(); if (grid_server) { lo_server_thread_stop(grid_server); lo_server_thread_free(grid_server); grid_server = nullptr; } if (grid_addr) { lo_address_free(grid_addr); grid_addr = nullptr; } if (stream) { Pa_CloseStream(stream); stream = nullptr; } Pa_Terminate(); if (etherEngine) { ether_shutdown(etherEngine); ether_destroy(etherEngine); etherEngine = nullptr; } audioRunning = false; }
}

/* END_OLD_INCLASS */

// Encoder control system implementation
void GridSequencer::setupEncoderSystem() {
    // Simple encoder setup - just connect to serial like encoder_demo
    std::cout << "Waiting for encoder controller..." << std::endl;
    sleep(2);  // Give QTPY time to boot and run script

    std::vector<std::string> devices = {"/dev/tty.usbmodem101", "/dev/tty.usbmodemm59111127381"};
    for (const auto& device : devices) {
        std::cout << "Trying to connect to: " << device << std::endl;
        if (encoderSerial.open(device)) {
            std::cout << "📡 Connected to encoder controller: " << device << std::endl;
            break;
        }
    }
}

void GridSequencer::updateEngineFromEncoderChange(const std::string& param_id, float delta) {
    // Parse param_id like "engine2_cutoff"
    size_t underscore = param_id.find('_');
    if (underscore == std::string::npos) return;

    std::string engine_part = param_id.substr(0, underscore);
    std::string param_part = param_id.substr(underscore + 1);

    if (engine_part.substr(0, 6) != "engine") return;

    int engine_num = std::stoi(engine_part.substr(6));
    if (engine_num < 0 || engine_num >= MAX_ENGINES) return;

    // Map parameter name to ParameterID
    ParameterID pid;
    if (param_part == "cutoff") pid = ParameterID::FILTER_CUTOFF;
    else if (param_part == "resonance") pid = ParameterID::FILTER_RESONANCE;
    else if (param_part == "attack") pid = ParameterID::ATTACK;
    else if (param_part == "decay") pid = ParameterID::DECAY;
    else if (param_part == "sustain") pid = ParameterID::SUSTAIN;
    else if (param_part == "release") pid = ParameterID::RELEASE;
    else if (param_part == "volume") pid = ParameterID::VOLUME;
    else if (param_part == "pan") pid = ParameterID::PAN;
    else if (param_part == "reverb") pid = ParameterID::REVERB_MIX;
    else return;

    // Update the actual synthesizer engine
    int slot = rowToSlot[engine_num];
    if (slot < 0) slot = 0;

    // Get current value, add delta, and clamp
    float current_value = engineParameters[engine_num][static_cast<int>(pid)];
    float new_value = std::clamp(current_value + delta, 0.0f, 1.0f);

    // Update both our local copy and the engine
    engineParameters[engine_num][static_cast<int>(pid)] = new_value;
    ether_set_instrument_parameter(etherEngine, slot, static_cast<int>(pid), new_value);
}

std::string getParameterName(ParameterID pid) {
    switch (pid) {
        case ParameterID::HARMONICS: return "harmonics";
        case ParameterID::TIMBRE: return "timbre";
        case ParameterID::MORPH: return "morph";
        case ParameterID::ATTACK: return "attack";
        case ParameterID::DECAY: return "decay";
        case ParameterID::SUSTAIN: return "sustain";
        case ParameterID::RELEASE: return "release";
        case ParameterID::FILTER_CUTOFF: return "lpf";
        case ParameterID::FILTER_RESONANCE: return "resonance";
        case ParameterID::HPF: return "hpf";
        case ParameterID::VOLUME: return "volume";
        case ParameterID::PAN: return "pan";
        case ParameterID::REVERB_MIX: return "reverb";
        default: return "unknown";
    }
}

void GridSequencer::syncMenuWithEncoder(const std::string& param_id) {
    // Instead of using encoder's own menu system, directly control the grid sequencer's selectedParamIndex
    // This makes encoder navigation work exactly like arrow keys

    // We don't need to parse param_id - just move the cursor directly
    // The encoder control system will call this when it wants to move the cursor
}

void GridSequencer::handleEncoderLatch(int encoder_id) {
    if (encoder_id < 1 || encoder_id > 3) return;

    int enc_index = encoder_id - 1;

    // Latch to currently selected parameter
    if (selectedParamIndex >= 0 && selectedParamIndex < (int)visibleParams.size()) {
        encoderLatches[enc_index].active = true;
        encoderLatches[enc_index].paramIndex = selectedParamIndex;
        encoderLatches[enc_index].paramName = getParameterName(visibleParams[selectedParamIndex]);

        std::cout << "🔒 Encoder " << encoder_id << " latched to " << encoderLatches[enc_index].paramName << std::endl;
    }
}

void GridSequencer::adjustLatchedParameter(int enc_index, int delta) {
    if (!encoderLatches[enc_index].active) return;
    if (encoderLatches[enc_index].paramIndex < 0 || encoderLatches[enc_index].paramIndex >= (int)visibleParams.size()) return;

    ParameterID pid = visibleParams[encoderLatches[enc_index].paramIndex];
    int slot = rowToSlot[currentEngineRow];
    if (slot < 0) slot = 0;

    // Get current parameter value
    float currentValue = ether_get_instrument_parameter(etherEngine, slot, static_cast<int>(pid));

    // Adjust by small increments (0.01 per encoder step)
    float newValue = currentValue + (delta * 0.01f);
    newValue = std::max(0.0f, std::min(1.0f, newValue));  // Clamp to 0-1

    // Set the new value
    ether_set_instrument_parameter(etherEngine, slot, static_cast<int>(pid), newValue);

    std::cout << "🎛️ " << encoderLatches[enc_index].paramName << ": " << std::fixed << std::setprecision(2) << newValue << std::endl;
}

void GridSequencer::processEncoderInput() {
    // Read serial data - replicate encoder_demo approach exactly
    char buffer[256];
    int bytesRead = encoderSerial.readData(buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        serialLineBuffer += buffer;

        // Process complete lines
        size_t pos;
        while ((pos = serialLineBuffer.find('\n')) != std::string::npos) {
            std::string line = serialLineBuffer.substr(0, pos);
            serialLineBuffer = serialLineBuffer.substr(pos + 1);

            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            // Parse encoder commands - direct approach like encoder_demo
            if (!line.empty() && line[0] == 'E') {
                // Format: E1:+1 or E2:-1
                if (line.size() >= 4 && line[2] == ':') {
                    int encoder_id = line[1] - '0';
                    int delta = std::stoi(line.substr(3));
                    std::cout << "🎛️ Encoder " << encoder_id << " turned " << delta << std::endl;

                    // Handle encoder directly like encoder_demo
                    if (encoder_id == 4) {
                        // Encoder 4: Menu navigation
                        handleEncoder4Turn(delta);
                    } else {
                        // Encoders 1-3: Parameter control (for now, adjust current param)
                        handleParameterEncoderTurn(encoder_id, delta);
                    }
                }
            } else if (!line.empty() && line[0] == 'B') {
                // Format: B1:PRESS or B1:RELEASE
                if (line.size() >= 4 && line[2] == ':') {
                    int encoder_id = line[1] - '0';
                    std::string action = line.substr(3);
                    if (action == "PRESS") {
                        std::cout << "🔘 Button " << encoder_id << " PRESSED" << std::endl;
                        handleEncoderButtonPress(encoder_id);
                    } else if (action == "RELEASE") {
                        std::cout << "🔘 Button " << encoder_id << " RELEASED" << std::endl;
                    }
                }
            }
        }
    }
}

// Proper control architecture implementation
void GridSequencer::handleEncoder4Turn(int delta) {
    if (editMode) {
        // Edit mode: adjust currently selected parameter
        std::cout << ">>> EDIT MODE: Adjusting parameter " << (delta > 0 ? "UP" : "DOWN") << std::endl;

        if (selectedParamIndex >= 0 && selectedParamIndex < (int)visibleParams.size()) {
            ParameterID pid = visibleParams[selectedParamIndex];
            float current_value = engineParameters[currentEngineRow][static_cast<int>(pid)];
            float new_value = std::clamp(current_value + (delta * 0.01f), 0.0f, 1.0f);
            engineParameters[currentEngineRow][static_cast<int>(pid)] = new_value;
            int slot = rowToSlot[currentEngineRow];
            if (slot < 0) slot = 0;
            ether_set_instrument_parameter(etherEngine, slot, static_cast<int>(pid), new_value);

            std::cout << ">>> PARAM UPDATE: " << getParameterName(pid) << " = " << new_value << std::endl;
        }
    } else {
        // Browse mode: navigate menu
        std::cout << ">>> MENU NAVIGATION: " << (delta > 0 ? "DOWN" : "UP") << std::endl;

        int maxIdx = (int)visibleParams.size() + extraMenuRows();
        if (delta > 0) {
            selectedParamIndex = std::min(maxIdx, selectedParamIndex + 1);
        } else {
            selectedParamIndex = std::max(0, selectedParamIndex - 1);
        }
    }
}

void GridSequencer::handleParameterEncoderTurn(int encoder_id, int delta) {
    int enc_index = encoder_id - 1; // Convert to 0-2 index
    std::cout << ">>> ENCODER " << encoder_id << " TURN: " << (delta > 0 ? "CW" : "CCW") << std::endl;

    if (paramLatches[enc_index].active) {
        // Adjust latched parameter on the specific engine it was latched to
        ParameterID pid = paramLatches[enc_index].paramId;
        int latchedEngineRow = paramLatches[enc_index].engineRow;

        float current_value = engineParameters[latchedEngineRow][static_cast<int>(pid)];
        float new_value = std::clamp(current_value + (delta * 0.01f), 0.0f, 1.0f);
        engineParameters[latchedEngineRow][static_cast<int>(pid)] = new_value;

        int slot = rowToSlot[latchedEngineRow];
        if (slot < 0) slot = 0;
        ether_set_instrument_parameter(etherEngine, slot, static_cast<int>(pid), new_value);

        std::cout << ">>> LATCHED PARAM: " << paramLatches[enc_index].paramName << " (Engine Row " << latchedEngineRow << ") = " << new_value << std::endl;
    } else {
        std::cout << ">>> No parameter latched to encoder " << encoder_id << std::endl;
    }
}

void GridSequencer::handleEncoderButtonPress(int encoder_id) {
    int enc_index = encoder_id - 1; // Convert to 0-3 index
    auto now = std::chrono::steady_clock::now();
    auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - buttonStates[enc_index].lastPressTime).count();

    if (time_since_last < DOUBLE_PRESS_TIMEOUT_MS && buttonStates[enc_index].pendingSinglePress) {
        // Double press detected
        buttonStates[enc_index].pendingSinglePress = false;
        std::cout << ">>> DOUBLE PRESS: Encoder " << encoder_id << std::endl;

        if (encoder_id == 4) {
            // Encoder 4 double press: could add special behavior
            std::cout << ">>> Encoder 4 double press action" << std::endl;
        } else {
            // Encoders 1-3 double press: clear all latches
            std::cout << ">>> CLEAR ALL LATCHES" << std::endl;
            for (int i = 0; i < 3; i++) {
                paramLatches[i].active = false;
            }
        }
    } else {
        // Potential single press - start timer
        buttonStates[enc_index].pendingSinglePress = true;
        buttonStates[enc_index].lastPressTime = now;
    }
}

void GridSequencer::updateButtonTimers() {
    auto now = std::chrono::steady_clock::now();

    for (int i = 0; i < 4; i++) {
        if (buttonStates[i].pendingSinglePress) {
            auto time_since_press = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - buttonStates[i].lastPressTime).count();

            if (time_since_press >= DOUBLE_PRESS_TIMEOUT_MS) {
                buttonStates[i].pendingSinglePress = false;
                processPendingButtonPress(i + 1); // Convert back to 1-4 encoder ID
            }
        }
    }
}

void GridSequencer::processPendingButtonPress(int encoder_id) {
    std::cout << ">>> SINGLE PRESS: Encoder " << encoder_id << std::endl;

    if (encoder_id == 4) {
        // Encoder 4 single press: toggle edit mode
        editMode = !editMode;
        if (editMode) {
            std::cout << ">>> ENTERED EDIT MODE" << std::endl;
        } else {
            std::cout << ">>> EXITED EDIT MODE" << std::endl;
        }
    } else {
        // Encoders 1-3 single press: latch current parameter
        int enc_index = encoder_id - 1;

        if (selectedParamIndex >= 0 && selectedParamIndex < (int)visibleParams.size()) {
            ParameterID pid = visibleParams[selectedParamIndex];
            paramLatches[enc_index].active = true;
            paramLatches[enc_index].paramId = pid;
            paramLatches[enc_index].paramName = getParameterName(pid);
            paramLatches[enc_index].engineRow = currentEngineRow;  // Store which engine this parameter belongs to

            std::cout << ">>> LATCH: Encoder " << encoder_id << " -> " << paramLatches[enc_index].paramName << " (Engine Row " << currentEngineRow << ")" << std::endl;
        } else {
            std::cout << ">>> No parameter selected to latch" << std::endl;
        }
    }
}

int main() {
    GridSequencer sequencer;

    if (!sequencer.initialize()) {
        return 1;
    }

    sequencer.run();
    return 0;
}
