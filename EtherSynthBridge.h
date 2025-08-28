#ifndef EtherSynthBridge_h
#define EtherSynthBridge_h

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for C++ types
typedef struct EtherSynthCpp EtherSynthCpp;

// Core lifecycle
EtherSynthCpp* ether_create(void);
void ether_destroy(EtherSynthCpp* synth);
int ether_initialize(EtherSynthCpp* synth);
void ether_shutdown(EtherSynthCpp* synth);

// Instrument management
void ether_set_active_instrument(EtherSynthCpp* synth, int color_index);
int ether_get_active_instrument(EtherSynthCpp* synth);

// Note events
void ether_note_on(EtherSynthCpp* synth, int key_index, float velocity, float aftertouch);
void ether_note_off(EtherSynthCpp* synth, int key_index);
void ether_all_notes_off(EtherSynthCpp* synth);

// Transport
void ether_play(EtherSynthCpp* synth);
void ether_stop(EtherSynthCpp* synth);
void ether_record(EtherSynthCpp* synth, int enable);
int ether_is_playing(EtherSynthCpp* synth);
int ether_is_recording(EtherSynthCpp* synth);

// Parameters
void ether_set_parameter(EtherSynthCpp* synth, int param_id, float value);
float ether_get_parameter(EtherSynthCpp* synth, int param_id);
void ether_set_instrument_parameter(EtherSynthCpp* synth, int instrument, int param_id, float value);
float ether_get_instrument_parameter(EtherSynthCpp* synth, int instrument, int param_id);

// Smart knob and touch interface
void ether_set_smart_knob(EtherSynthCpp* synth, float value);
float ether_get_smart_knob(EtherSynthCpp* synth);
void ether_set_touch_position(EtherSynthCpp* synth, float x, float y);

// BPM and timing
void ether_set_bpm(EtherSynthCpp* synth, float bpm);
float ether_get_bpm(EtherSynthCpp* synth);

// Performance metrics
float ether_get_cpu_usage(EtherSynthCpp* synth);
int ether_get_active_voice_count(EtherSynthCpp* synth);
float ether_get_master_volume(EtherSynthCpp* synth);
void ether_set_master_volume(EtherSynthCpp* synth, float volume);

// Parameter IDs (matching Types.h)
typedef enum {
    PARAM_VOLUME = 0,
    PARAM_ATTACK = 1,
    PARAM_DECAY = 2,
    PARAM_SUSTAIN = 3,
    PARAM_RELEASE = 4,
    PARAM_FILTER_CUTOFF = 5,
    PARAM_FILTER_RESONANCE = 6,
    PARAM_OSC1_FREQ = 7,
    PARAM_OSC2_FREQ = 8,
    PARAM_OSC_MIX = 9,
    PARAM_LFO_RATE = 10,
    PARAM_LFO_DEPTH = 11,
    PARAM_COUNT = 12
} EtherParameterID;

// Instrument colors (matching Types.h)
typedef enum {
    INSTRUMENT_RED = 0,
    INSTRUMENT_ORANGE = 1,
    INSTRUMENT_YELLOW = 2,
    INSTRUMENT_GREEN = 3,
    INSTRUMENT_BLUE = 4,
    INSTRUMENT_INDIGO = 5,
    INSTRUMENT_VIOLET = 6,
    INSTRUMENT_GREY = 7,
    INSTRUMENT_COUNT = 8
} EtherInstrumentColor;

#ifdef __cplusplus
}
#endif

#endif /* EtherSynthBridge_h */