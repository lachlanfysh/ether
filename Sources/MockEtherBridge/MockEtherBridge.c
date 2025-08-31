// Mock C++ Bridge for EtherSynth UI Testing
// This allows the complete GUI to run without the full audio engine

#include <stdint.h>

// Mock engine pointer type
typedef void* EtherEngine;

// Core engine management
EtherEngine ether_create() {
    return (void*)0x12345678; // Mock pointer
}

void ether_destroy(EtherEngine engine) {
    // No-op for mock
}

int32_t ether_initialize(EtherEngine engine, float sampleRate, int32_t bufferSize) {
    return 1; // Success
}

void ether_shutdown(EtherEngine engine) {
    // No-op for mock
}

// Transport
void ether_play(EtherEngine engine) {
    // No-op for mock
}

void ether_stop(EtherEngine engine) {
    // No-op for mock  
}

void ether_record(EtherEngine engine, int32_t enable) {
    // No-op for mock
}

// Performance monitoring
float ether_get_cpu_usage(EtherEngine engine) {
    return 15.5f; // Mock CPU usage
}

int32_t ether_get_active_voice_count(EtherEngine engine) {
    return 6; // Mock voice count
}

float ether_get_bpm(EtherEngine engine) {
    return 128.0f; // Mock BPM
}

// Master controls
float ether_get_master_volume(EtherEngine engine) {
    return 0.8f; // Mock volume
}

void ether_set_master_volume(EtherEngine engine, float volume) {
    // No-op for mock
}

void ether_set_bpm(EtherEngine engine, float bpm) {
    // No-op for mock
}

// Instrument management
void ether_set_active_instrument(EtherEngine engine, int32_t colorIndex) {
    // No-op for mock
}

int32_t ether_get_active_instrument(EtherEngine engine) {
    return 0; // Mock active instrument
}

void ether_set_instrument_engine_type(EtherEngine engine, int32_t instrument, int32_t engineType) {
    // No-op for mock
}

// Parameters
void ether_set_parameter(EtherEngine engine, int32_t paramId, float value) {
    // No-op for mock
}

float ether_get_parameter(EtherEngine engine, int32_t paramId) {
    return 0.5f; // Mock parameter value
}

void ether_set_instrument_parameter(EtherEngine engine, int32_t instrument, int32_t param, float value) {
    // No-op for mock
}

float ether_get_instrument_parameter(EtherEngine engine, int32_t instrument, int32_t param) {
    return 0.5f; // Mock parameter value
}

// Note events
void ether_trigger_note(EtherEngine engine, int32_t instrument, int32_t pitch, float velocity) {
    // No-op for mock
}

void ether_trigger_note_with_length(EtherEngine engine, int32_t instrument, int32_t pitch, float velocity, float length) {
    // No-op for mock
}

void ether_note_off(EtherEngine engine, int32_t instrument, int32_t pitch) {
    // No-op for mock
}

void ether_all_notes_off(EtherEngine engine) {
    // No-op for mock
}

// Touch and control
void ether_set_touch_position(EtherEngine engine, float x, float y) {
    // No-op for mock
}

void ether_set_smart_knob(EtherEngine engine, float value) {
    // No-op for mock
}

// Engine types
int32_t ether_get_engine_type_count() {
    return 8; // Mock engine count
}

const char* ether_get_engine_type_name(int32_t index) {
    static const char* names[] = {
        "MacroVA", "MacroFM", "Kick", "Snare", "Hihat", "Sampler", "Noise", "Filter"
    };
    if (index >= 0 && index < 8) {
        return names[index];
    }
    return "Unknown";
}

// Pattern management  
void ether_add_note_to_pattern(EtherEngine engine, const char* patternId, int32_t step, 
                               int32_t instrument, int32_t pitch, float velocity, float length) {
    // No-op for mock
}

void ether_clear_pattern_step(EtherEngine engine, const char* patternId, int32_t step) {
    // No-op for mock
}

void ether_set_pattern_length(EtherEngine engine, const char* patternId, int32_t length) {
    // No-op for mock
}

int32_t ether_get_pattern_length(EtherEngine engine, const char* patternId) {
    return 16; // Mock pattern length
}

// Additional functions needed
int32_t ether_is_playing(EtherEngine engine) {
    return 0; // Mock not playing
}

int32_t ether_is_recording(EtherEngine engine) {
    return 0; // Mock not recording
}

void ether_note_on(EtherEngine engine, int32_t note, float velocity, float aftertouch) {
    // No-op for mock
}

int32_t ether_get_instrument_engine_type(EtherEngine engine, int32_t instrument) {
    return 0; // Mock engine type
}