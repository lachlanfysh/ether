extern "C" {

// Global state
static float current_bpm = 120.0f;
static int active_instrument = 0;
static float smart_knob_value = 0.5f;
static bool is_playing = false;

void* ether_create(void) { return (void*)0x1234; }
int ether_initialize(void* engine) { return 1; }
void ether_destroy(void* engine) { }
void ether_shutdown(void* engine) { is_playing = false; }

void ether_play(void* engine) { is_playing = true; }
void ether_stop(void* engine) { is_playing = false; }
void ether_record(void* engine, int enable) { }
int ether_is_playing(void* engine) { return is_playing ? 1 : 0; }
int ether_is_recording(void* engine) { return 0; }

void ether_set_bpm(void* engine, float bpm) { current_bpm = bpm; }
float ether_get_bpm(void* engine) { return current_bpm; }

void ether_note_on(void* engine, int note, float velocity, float aftertouch) { }
void ether_note_off(void* engine, int note) { }
void ether_all_notes_off(void* engine) { }

void ether_set_parameter(void* engine, int param_id, float value) { }
float ether_get_parameter(void* engine, int param_id) { return 0.5f; }

void ether_set_active_instrument(void* engine, int color_index) { active_instrument = color_index; }
int ether_get_active_instrument(void* engine) { return active_instrument; }

float ether_get_cpu_usage(void* engine) { return 25.5f; }
int ether_get_active_voice_count(void* engine) { return 3; }

void ether_set_smart_knob(void* engine, float value) { smart_knob_value = value; }
float ether_get_smart_knob(void* engine) { return smart_knob_value; }

int ether_get_instrument_engine_type(void* engine, int instrument) { return instrument % 15; }
void ether_set_instrument_engine_type(void* engine, int instrument, int engine_type) { }

const char* ether_get_engine_type_name(int engine_type) {
    static const char* names[] = {"MacroVA", "MacroFM", "Wavetable", "Chord", "Harmonics"};
    return (engine_type >= 0 && engine_type < 5) ? names[engine_type] : "Unknown";
}

int ether_get_engine_type_count(void) { return 15; }

}
