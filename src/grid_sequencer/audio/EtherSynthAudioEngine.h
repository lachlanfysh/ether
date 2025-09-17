#pragma once
#include "IAudioEngine.h"
#include "../utils/Logger.h"
#include <atomic>

namespace GridSequencer {
namespace Audio {

// Forward declarations for EtherSynth C bridge functions
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
    void ether_set_lfo_rate(void* synth, unsigned char lfo_id, float rate);
    void ether_set_lfo_depth(void* synth, unsigned char lfo_id, float depth);
    void ether_set_lfo_waveform(void* synth, unsigned char lfo_id, unsigned char waveform);
    void ether_set_lfo_sync(void* synth, int instrument, int lfoIndex, int syncMode);
    void ether_trigger_instrument_lfos(void* synth, int instrument);
    void ether_assign_lfo_to_param_id(void* synth, int instrument, int lfoIndex, int paramId, float depth);
    void ether_remove_lfo_assignment_by_param(void* synth, int instrument, int lfoIndex, int paramId);
}

// Concrete implementation of the audio engine interface using EtherSynth
class EtherSynthAudioEngine : public IAudioEngine {
public:
    EtherSynthAudioEngine();
    virtual ~EtherSynthAudioEngine();

    // Engine management
    bool initialize() override;
    void shutdown() override;
    bool isInitialized() const override { return initialized_.load(); }

    // Audio processing
    void processAudio(float* outputBuffer, size_t bufferSize) override;
    void play() override;
    void stop() override;

    // Instrument management
    Result<bool> setInstrumentEngineType(int instrument, int engineType) override;
    Result<int> getInstrumentEngineType(int instrument) override;
    Result<std::string> getEngineTypeName(int engineType) override;
    int getEngineTypeCount() override;

    // Parameter management
    Result<bool> setParameter(int instrument, ParameterID param, float value) override;
    Result<float> getParameter(int instrument, ParameterID param) override;
    bool hasParameter(int instrument, ParameterID param) override;
    ParamRoute getParameterRoute(int instrument, ParameterID param) override;

    // Note triggering
    Result<bool> noteOn(int keyIndex, float velocity, float aftertouch = 0.0f) override;
    Result<bool> noteOff(int keyIndex) override;
    void allNotesOff() override;

    // Engine state
    void setActiveInstrument(int instrument) override;
    int getActiveInstrument() override;
    int getActiveVoiceCount() override;
    float getCPUUsage() override;
    float getMemoryUsageKB() override;

    // Master controls
    void setMasterVolume(float volume) override;
    float getMasterVolume() override;

    // Voice management
    void setEngineVoiceCount(int instrument, int voices) override;
    int getEngineVoiceCount(int instrument) override;

    // Effects
    void setEngineFXSend(int instrument, int which, float value) override;
    float getEngineFXSend(int instrument, int which) override;
    void setGlobalFX(int which, int param, float value) override;
    float getGlobalFX(int which, int param) override;

    // LFO system
    void setLFORate(unsigned char lfoId, float rate) override;
    void setLFODepth(unsigned char lfoId, float depth) override;
    void setLFOWaveform(unsigned char lfoId, unsigned char waveform) override;
    void triggerInstrumentLFOs(int instrument) override;

    // Get raw synth pointer (for migration period only)
    void* getRawSynthPointer() const { return synth_; }

private:
    void* synth_;
    std::atomic<bool> initialized_;

    // Helper methods
    bool isValidInstrument(int instrument) const;
    bool isValidParameter(ParameterID param) const;
    ParamRoute resolveParameterRoute(int instrument, ParameterID param);
};

} // namespace Audio
} // namespace GridSequencer