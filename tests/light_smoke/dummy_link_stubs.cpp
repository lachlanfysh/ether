// Dummy stubs to allow linking the monolith without external libs.
// Provides: ether_* bridge functions, minimal PortAudio and liblo symbols.

#include <cstddef>
extern "C" {

// -------------------- EtherSynth bridge stubs --------------------
static float g_bpm = 120.0f;
static int   g_active_inst = 0;
static float g_master_vol = 0.8f;
static int   g_engine_count = 16;
static bool  g_playing = false;

void* ether_create(void) { return (void*)0xDEADBEEF; }
void  ether_destroy(void* ) {}
int   ether_initialize(void* ) { return 1; }
void  ether_shutdown(void* ) { g_playing = false; }
void  ether_play(void* ) { g_playing = true; }
void  ether_stop(void* ) { g_playing = false; }
void  ether_process_audio(void*, float*, size_t) {}
void  ether_note_on(void*, int, float, float) {}
void  ether_note_off(void*, int) {}
void  ether_all_notes_off(void*) {}
void  ether_set_active_instrument(void*, int idx) { g_active_inst = idx; }
int   ether_get_active_instrument(void*) { return g_active_inst; }
int   ether_get_active_voice_count(void*) { return 0; }
float ether_get_cpu_usage(void*) { return 10.0f; }
void  ether_set_master_volume(void*, float v) { g_master_vol = v; }
float ether_get_master_volume(void*) { return g_master_vol; }
void  ether_set_instrument_engine_type(void*, int, int) {}
int   ether_get_instrument_engine_type(void*, int instrument) { return instrument % g_engine_count; }
const char* ether_get_engine_type_name(int engine_type) { (void)engine_type; return "DummyEngine"; }
int   ether_get_engine_type_count() { return g_engine_count; }
void  ether_set_engine_voice_count(void*, int, int) {}
int   ether_get_engine_voice_count(void*, int) { return 4; }
bool  ether_engine_has_parameter(void*, int, int) { return true; }
float ether_get_memory_usage_kb(void*) { return 1024.0f; }
float ether_get_cycles_480_per_buffer(void*) { return 1000.0f; }
float ether_get_cycles_480_per_sample(void*) { return 2.0f; }
float ether_get_engine_cpu_pct(void*, int) { return 1.0f; }
float ether_get_engine_cycles_480_buf(void*, int) { return 10.0f; }
float ether_get_engine_cycles_480_smp(void*, int) { return 0.1f; }
void  ether_set_engine_fx_send(void*, int, int, float) {}
float ether_get_engine_fx_send(void*, int, int) { return 0.0f; }
void  ether_set_fx_global(void*, int, int, float) {}
float ether_get_fx_global(void*, int, int) { return 0.0f; }
float ether_get_bpm(void*) { return g_bpm; }
int   ether_get_parameter_lfo_info(void*, int, int, int* activeLFOs, float* currentValue) { if (activeLFOs) *activeLFOs = 0; if (currentValue) *currentValue = 0.0f; return 0; }
void  ether_set_lfo_rate(void*, unsigned char, float) {}
void  ether_set_lfo_depth(void*, unsigned char, float) {}
void  ether_set_lfo_waveform(void*, unsigned char, unsigned char) {}
void  ether_set_lfo_sync(void*, int, int, int) {}
void  ether_trigger_instrument_lfos(void*, int) {}
void  ether_assign_lfo_to_param_id(void*, int, int, int, float) {}
void  ether_remove_lfo_assignment_by_param(void*, int, int, int) {}
void  ether_set_instrument_parameter(void*, int, int, float) {}
float ether_get_instrument_parameter(void*, int, int) { return 0.5f; }

// Non-existent in our search but referenced in some builds
void  ether_drum_set_param(void*, int, int, float) {}

// -------------------- PortAudio minimal stubs --------------------
struct PaStream { int _dummy; };
typedef int PaError;
typedef unsigned long PaSampleFormat;
typedef int PaStreamCallback(const void*, void*, unsigned long, const void*, void*);

PaError Pa_Initialize(void) { return 0; }
PaError Pa_Terminate(void) { return 0; }
PaError Pa_OpenDefaultStream(PaStream** stream,
                             int, int, PaSampleFormat,
                             double, unsigned long,
                             PaStreamCallback*, void*) {
    static PaStream dummy;
    if (stream) *stream = &dummy;
    return 0;
}
PaError Pa_CloseStream(PaStream*) { return 0; }
PaError Pa_StartStream(PaStream*) { return 0; }

// -------------------- liblo minimal stubs --------------------
typedef void lo_server_thread;
typedef void lo_address;
typedef void* lo_message;
typedef void lo_arg;

lo_server_thread* lo_server_thread_new(const char*, void*) { return (lo_server_thread*)0x1; }
void lo_server_thread_free(lo_server_thread*) {}
int  lo_server_thread_start(lo_server_thread*) { return 0; }
int  lo_server_thread_stop(lo_server_thread*) { return 0; }
void lo_server_thread_add_method(lo_server_thread*, const char*, const char*, int(*)(const char*, const char*, lo_arg**, int, lo_message, void*), void*) { (void)0; }
lo_address* lo_address_new(const char*, const char*) { return (lo_address*)0x2; }
void lo_address_free(lo_address*) {}
int  lo_send(lo_address*, const char*, const char*, ...) { return 0; }
int  lo_send_internal(void*, const char*, const char*, ...) { return 0; }

} // extern "C"
