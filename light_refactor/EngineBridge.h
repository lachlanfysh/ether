// Header-only thin bridge over ether_* C API.
// Intent: provide a stable, testable surface without changing behavior.

#pragma once

#include <cstddef>
#include <cstdint>

extern "C" {
    void* ether_create(void);
    void ether_destroy(void* synth);
    int   ether_initialize(void* synth);
    void  ether_process_audio(void* synth, float* outputBuffer, size_t bufferSize);
    void  ether_play(void* synth);
    void  ether_stop(void* synth);
    void  ether_note_on(void* synth, int key_index, float velocity, float aftertouch);
    void  ether_note_off(void* synth, int key_index);
    void  ether_all_notes_off(void* synth);
    void  ether_set_instrument_engine_type(void* synth, int instrument, int engine_type);
    int   ether_get_instrument_engine_type(void* synth, int instrument);
    const char* ether_get_engine_type_name(int engine_type);
    int   ether_get_engine_type_count();
    void  ether_set_active_instrument(void* synth, int color_index);
    int   ether_get_active_instrument(void* synth);
    int   ether_get_active_voice_count(void* synth);
    float ether_get_cpu_usage(void* synth);
    void  ether_set_master_volume(void* synth, float volume);
    float ether_get_master_volume(void* synth);
    void  ether_set_instrument_parameter(void* synth, int instrument, int param_id, float value);
    float ether_get_instrument_parameter(void* synth, int instrument, int param_id);
    void  ether_shutdown(void* synth);
    void  ether_set_engine_voice_count(void* synth, int instrument, int voices);
    int   ether_get_engine_voice_count(void* synth, int instrument);
    bool  ether_engine_has_parameter(void* synth, int instrument, int param_id);
    float ether_get_memory_usage_kb(void* synth);
    float ether_get_cycles_480_per_buffer(void* synth);
    float ether_get_cycles_480_per_sample(void* synth);
    float ether_get_engine_cpu_pct(void* synth, int instrument);
    float ether_get_engine_cycles_480_buf(void* synth, int instrument);
    float ether_get_engine_cycles_480_smp(void* synth, int instrument);
    void  ether_set_engine_fx_send(void* synth, int instrument, int which, float value);
    float ether_get_engine_fx_send(void* synth, int instrument, int which);
    void  ether_set_fx_global(void* synth, int which, int param, float value);
    float ether_get_fx_global(void* synth, int which, int param);
    float ether_get_bpm(void* synth);
    int   ether_get_parameter_lfo_info(void* synth, int instrument, int keyIndex, int* activeLFOs, float* currentValue);
    void  ether_set_lfo_rate(void* synth, unsigned char lfo_id, float rate);
    void  ether_set_lfo_depth(void* synth, unsigned char lfo_id, float depth);
    void  ether_set_lfo_waveform(void* synth, unsigned char lfo_id, unsigned char waveform);
    void  ether_set_lfo_sync(void* synth, int instrument, int lfoIndex, int syncMode);
    void  ether_trigger_instrument_lfos(void* synth, int instrument);
    void  ether_assign_lfo_to_param_id(void* synth, int instrument, int lfoIndex, int paramId, float depth);
    void  ether_remove_lfo_assignment_by_param(void* synth, int instrument, int lfoIndex, int paramId);
}

namespace light {
struct EngineBridge {
    using Handle = void*;

    // Lifecycle
    static inline Handle create() noexcept { return ether_create(); }
    static inline void destroy(Handle h) noexcept { if (h) ether_destroy(h); }
    static inline int initialize(Handle h) noexcept { return ether_initialize(h); }
    static inline void shutdown(Handle h) noexcept { if (h) ether_shutdown(h); }

    // Transport
    static inline void play(Handle h) noexcept { ether_play(h); }
    static inline void stop(Handle h) noexcept { ether_stop(h); }

    // Audio processing
    static inline void process(Handle h, float* out, size_t n) noexcept { ether_process_audio(h, out, n); }

    // Notes
    static inline void noteOn(Handle h, int key, float vel, float at = 0.0f) noexcept { ether_note_on(h, key, vel, at); }
    static inline void noteOff(Handle h, int key) noexcept { ether_note_off(h, key); }
    static inline void allNotesOff(Handle h) noexcept { ether_all_notes_off(h); }

    // Instruments / Engines
    static inline void setEngineType(Handle h, int inst, int engineType) noexcept { ether_set_instrument_engine_type(h, inst, engineType); }
    static inline int  getEngineType(Handle h, int inst) noexcept { return ether_get_instrument_engine_type(h, inst); }
    static inline const char* engineTypeName(int engineType) noexcept { return ether_get_engine_type_name(engineType); }
    static inline int  engineTypeCount() noexcept { return ether_get_engine_type_count(); }
    static inline void setActiveInstrument(Handle h, int idx) noexcept { ether_set_active_instrument(h, idx); }
    static inline int  activeInstrument(Handle h) noexcept { return ether_get_active_instrument(h); }
    static inline int  activeVoiceCount(Handle h) noexcept { return ether_get_active_voice_count(h); }

    // Parameters
    static inline void setParam(Handle h, int inst, int paramId, float value) noexcept { ether_set_instrument_parameter(h, inst, paramId, value); }
    static inline float getParam(Handle h, int inst, int paramId) noexcept { return ether_get_instrument_parameter(h, inst, paramId); }
    static inline bool hasParam(Handle h, int inst, int paramId) noexcept { return ether_engine_has_parameter(h, inst, paramId); }

    // FX
    static inline void setFxSend(Handle h, int inst, int which, float value) noexcept { ether_set_engine_fx_send(h, inst, which, value); }
    static inline float getFxSend(Handle h, int inst, int which) noexcept { return ether_get_engine_fx_send(h, inst, which); }
    static inline void setFxGlobal(Handle h, int which, int param, float value) noexcept { ether_set_fx_global(h, which, param, value); }
    static inline float getFxGlobal(Handle h, int which, int param) noexcept { return ether_get_fx_global(h, which, param); }

    // System metrics
    static inline float cpuUsage(Handle h) noexcept { return ether_get_cpu_usage(h); }
    static inline float memoryKB(Handle h) noexcept { return ether_get_memory_usage_kb(h); }
    static inline float cyclesPerBuffer(Handle h) noexcept { return ether_get_cycles_480_per_buffer(h); }
    static inline float cyclesPerSample(Handle h) noexcept { return ether_get_cycles_480_per_sample(h); }
    static inline float engineCpuPct(Handle h, int inst) noexcept { return ether_get_engine_cpu_pct(h, inst); }
    static inline float engineCyclesBuf(Handle h, int inst) noexcept { return ether_get_engine_cycles_480_buf(h, inst); }
    static inline float engineCyclesSmp(Handle h, int inst) noexcept { return ether_get_engine_cycles_480_smp(h, inst); }
    static inline float bpm(Handle h) noexcept { return ether_get_bpm(h); }

    // LFO
    static inline void setLfoRate(Handle h, unsigned char id, float rate) noexcept { ether_set_lfo_rate(h, id, rate); }
    static inline void setLfoDepth(Handle h, unsigned char id, float depth) noexcept { ether_set_lfo_depth(h, id, depth); }
    static inline void setLfoWaveform(Handle h, unsigned char id, unsigned char waveform) noexcept { ether_set_lfo_waveform(h, id, waveform); }
    static inline void setLfoSync(Handle h, int inst, int lfoIndex, int syncMode) noexcept { ether_set_lfo_sync(h, inst, lfoIndex, syncMode); }
    static inline void triggerInstrumentLfos(Handle h, int inst) noexcept { ether_trigger_instrument_lfos(h, inst); }
    static inline void assignLfoToParam(Handle h, int inst, int lfoIndex, int paramId, float depth) noexcept { ether_assign_lfo_to_param_id(h, inst, lfoIndex, paramId, depth); }
    static inline void removeLfoAssignment(Handle h, int inst, int lfoIndex, int paramId) noexcept { ether_remove_lfo_assignment_by_param(h, inst, lfoIndex, paramId); }
    static inline int  getParamLfoInfo(Handle h, int inst, int keyIndex, int* activeLFOs, float* currentValue) noexcept { return ether_get_parameter_lfo_info(h, inst, keyIndex, activeLFOs, currentValue); }
};
} // namespace light

