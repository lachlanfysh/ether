//
//  EtherSynth-Bridging-Header.h
//  Created by Claude Code
//

#ifndef EtherSynth_Bridging_Header_h
#define EtherSynth_Bridging_Header_h

#ifdef __cplusplus
extern "C" {
#endif

// Core engine management  
void* ether_create(void);
int ether_initialize(void* engine);
void ether_destroy(void* engine);
void ether_shutdown(void* engine);

// Transport controls
void ether_play(void* engine);
void ether_stop(void* engine);
void ether_record(void* engine, int enable);
int ether_is_playing(void* engine);
int ether_is_recording(void* engine);

// Tempo and timing
void ether_set_bpm(void* engine, float bpm);
float ether_get_bpm(void* engine);

// Note events (using int for compatibility with Swift Int32)
void ether_note_on(void* engine, int note, float velocity, float aftertouch);
void ether_note_off(void* engine, int note);
void ether_all_notes_off(void* engine);

// Parameters
void ether_set_parameter(void* engine, int param_id, float value);
float ether_get_parameter(void* engine, int param_id);
void ether_set_instrument_parameter(void* engine, int instrument, int param_id, float value);
float ether_get_instrument_parameter(void* engine, int instrument, int param_id);

// Instrument management (using int for compatibility with Swift Int32)
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

#ifdef __cplusplus
}
#endif

#endif /* EtherSynth_Bridging_Header_h */