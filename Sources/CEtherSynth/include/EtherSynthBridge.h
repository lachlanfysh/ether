//
//  EtherSynthBridge.h
//  C Bridge for EtherSynth
//

#ifndef ETHER_SYNTH_BRIDGE_H
#define ETHER_SYNTH_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// Core engine management
void* ether_create(void);
int ether_initialize(void* engine);
void ether_destroy(void* engine);

// Transport controls
void ether_play(void* engine);
void ether_stop(void* engine);
void ether_record(void* engine, int enable);
int ether_is_playing(void* engine);
int ether_is_recording(void* engine);

// Tempo and timing
void ether_set_bpm(void* engine, float bpm);
float ether_get_bpm(void* engine);

// Note events
void ether_note_on(void* engine, int note, float velocity, float aftertouch);
void ether_note_off(void* engine, int note);
void ether_all_notes_off(void* engine);

// Parameters
void ether_set_parameter(void* engine, int param_id, float value);
float ether_get_parameter(void* engine, int param_id);
void ether_set_instrument_parameter(void* engine, int instrument, int param_id, float value);
float ether_get_instrument_parameter(void* engine, int instrument, int param_id);

// Instrument management  
void ether_set_active_instrument(void* engine, int color_index);
int ether_get_active_instrument(void* engine);

// Performance monitoring
float ether_get_cpu_usage(void* engine);
int ether_get_active_voice_count(void* engine);
float ether_get_master_volume(void* engine);
void ether_set_master_volume(void* engine, float volume);

// Smart controls
void ether_set_smart_knob(void* engine, float value);
float ether_get_smart_knob(void* engine);
void ether_set_touch_position(void* engine, float x, float y);

// Engine type management (NEW - exposes real C++ engine types)
int ether_get_instrument_engine_type(void* engine, int instrument);
void ether_set_instrument_engine_type(void* engine, int instrument, int engine_type);
const char* ether_get_engine_type_name(int engine_type);
const char* ether_get_instrument_color_name(int color_index);
int ether_get_engine_type_count(void);
int ether_get_instrument_color_count(void);

// NEW: Enhanced engine information for UI
void ether_get_available_engines(int* engine_types, char** engine_names, char** engine_categories, int max_count);
int ether_get_engine_info_batch(int* out_types, const char** out_names, const char** out_categories, int max_engines);

// LFO controls
void ether_set_lfo_rate(void* engine, unsigned char lfo_id, float rate);
void ether_set_lfo_depth(void* engine, unsigned char lfo_id, float depth);
void ether_set_lfo_waveform(void* engine, unsigned char lfo_id, unsigned char waveform);

// Sequencer controls
void ether_set_pattern_length(void* engine, unsigned char length);
void ether_set_pattern_step(void* engine, unsigned char step, unsigned char note, float velocity);

#ifdef __cplusplus
}
#endif

#endif // ETHER_SYNTH_BRIDGE_H