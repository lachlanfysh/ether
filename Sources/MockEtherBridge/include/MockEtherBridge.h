#ifndef MOCK_ETHER_BRIDGE_H
#define MOCK_ETHER_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Mock engine pointer type
typedef void* EtherEngine;

// Core engine management
EtherEngine ether_create(void);
void ether_destroy(EtherEngine engine);
int32_t ether_initialize(EtherEngine engine, float sampleRate, int32_t bufferSize);
void ether_shutdown(EtherEngine engine);

// Transport
void ether_play(EtherEngine engine);
void ether_stop(EtherEngine engine);
void ether_record(EtherEngine engine, int32_t enable);

// Performance monitoring
float ether_get_cpu_usage(EtherEngine engine);
int32_t ether_get_active_voice_count(EtherEngine engine);
float ether_get_bpm(EtherEngine engine);

// Master controls
float ether_get_master_volume(EtherEngine engine);
void ether_set_master_volume(EtherEngine engine, float volume);
void ether_set_bpm(EtherEngine engine, float bpm);

// Instrument management
void ether_set_active_instrument(EtherEngine engine, int32_t colorIndex);
int32_t ether_get_active_instrument(EtherEngine engine);
void ether_set_instrument_engine_type(EtherEngine engine, int32_t instrument, int32_t engineType);

// Parameters
void ether_set_parameter(EtherEngine engine, int32_t paramId, float value);
float ether_get_parameter(EtherEngine engine, int32_t paramId);
void ether_set_instrument_parameter(EtherEngine engine, int32_t instrument, int32_t param, float value);
float ether_get_instrument_parameter(EtherEngine engine, int32_t instrument, int32_t param);

// Note events
void ether_trigger_note(EtherEngine engine, int32_t instrument, int32_t pitch, float velocity);
void ether_trigger_note_with_length(EtherEngine engine, int32_t instrument, int32_t pitch, float velocity, float length);
void ether_note_off(EtherEngine engine, int32_t instrument, int32_t pitch);
void ether_all_notes_off(EtherEngine engine);

// Touch and control
void ether_set_touch_position(EtherEngine engine, float x, float y);
void ether_set_smart_knob(EtherEngine engine, float value);

// Engine types
int32_t ether_get_engine_type_count(void);
const char* ether_get_engine_type_name(int32_t index);

// Pattern management
void ether_add_note_to_pattern(EtherEngine engine, const char* patternId, int32_t step, 
                               int32_t instrument, int32_t pitch, float velocity, float length);
void ether_clear_pattern_step(EtherEngine engine, const char* patternId, int32_t step);
void ether_set_pattern_length(EtherEngine engine, const char* patternId, int32_t length);
int32_t ether_get_pattern_length(EtherEngine engine, const char* patternId);

// Additional functions
int32_t ether_is_playing(EtherEngine engine);
int32_t ether_is_recording(EtherEngine engine);
void ether_note_on(EtherEngine engine, int32_t note, float velocity, float aftertouch);
int32_t ether_get_instrument_engine_type(EtherEngine engine, int32_t instrument);

#ifdef __cplusplus
}
#endif

#endif // MOCK_ETHER_BRIDGE_H