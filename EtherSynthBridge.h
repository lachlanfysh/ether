#ifndef EtherSynthBridge_h
#define EtherSynthBridge_h

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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

// Engine Parameter Mapping Functions (for 16-key system)
// Parameter control by key index (0-15) for current engine
void ether_set_parameter_by_key(EtherSynthCpp* synth, int instrument, int keyIndex, float value);
float ether_get_parameter_by_key(EtherSynthCpp* synth, int instrument, int keyIndex);
const char* ether_get_parameter_name(EtherSynthCpp* synth, int engineType, int keyIndex);
const char* ether_get_parameter_unit(EtherSynthCpp* synth, int engineType, int keyIndex);
float ether_get_parameter_min(EtherSynthCpp* synth, int engineType, int keyIndex);
float ether_get_parameter_max(EtherSynthCpp* synth, int engineType, int keyIndex);
int ether_get_parameter_group(EtherSynthCpp* synth, int engineType, int keyIndex);

// Engine management
void ether_set_instrument_engine(EtherSynthCpp* synth, int instrument, int engineType);
int ether_get_instrument_engine(EtherSynthCpp* synth, int instrument);
int ether_get_engine_count(EtherSynthCpp* synth);
const char* ether_get_engine_name(EtherSynthCpp* synth, int engineType);

// SmartKnob parameter control
void ether_set_smart_knob_parameter(EtherSynthCpp* synth, int parameterIndex);
int ether_get_smart_knob_parameter(EtherSynthCpp* synth);

// LFO Control Functions (for advanced modulation)
void ether_assign_lfo_to_parameter(EtherSynthCpp* synth, int instrument, int lfoIndex, int keyIndex, float depth);
void ether_remove_lfo_assignment(EtherSynthCpp* synth, int instrument, int lfoIndex, int keyIndex);
// Convenience: assign by ParameterID instead of key index (grid UI)
void ether_assign_lfo_to_param_id(EtherSynthCpp* synth, int instrument, int lfoIndex, int paramId, float depth);
void ether_remove_lfo_assignment_by_param(EtherSynthCpp* synth, int instrument, int lfoIndex, int paramId);
void ether_set_lfo_waveform(EtherSynthCpp* synth, int instrument, int lfoIndex, int waveform);
void ether_set_lfo_rate(EtherSynthCpp* synth, int instrument, int lfoIndex, float rate);
void ether_set_lfo_depth(EtherSynthCpp* synth, int instrument, int lfoIndex, float depth);
void ether_set_lfo_sync(EtherSynthCpp* synth, int instrument, int lfoIndex, int syncMode);
int ether_get_parameter_lfo_count(EtherSynthCpp* synth, int instrument, int keyIndex);
int ether_get_parameter_lfo_info(EtherSynthCpp* synth, int instrument, int keyIndex, int* activeLFOs, float* currentValue);
void ether_trigger_instrument_lfos(EtherSynthCpp* synth, int instrument);
void ether_apply_lfo_template(EtherSynthCpp* synth, int instrument, int templateType);

// Effects Control Functions
uint32_t ether_add_effect(EtherSynthCpp* synth, int effectType, int effectSlot);
void ether_remove_effect(EtherSynthCpp* synth, uint32_t effectId);
void ether_set_effect_parameter(EtherSynthCpp* synth, uint32_t effectId, int keyIndex, float value);
float ether_get_effect_parameter(EtherSynthCpp* synth, uint32_t effectId, int keyIndex);
void ether_get_effect_parameter_name(EtherSynthCpp* synth, uint32_t effectId, int keyIndex, char* name, size_t nameSize);
void ether_set_effect_enabled(EtherSynthCpp* synth, uint32_t effectId, bool enabled);
void ether_set_effect_wet_dry_mix(EtherSynthCpp* synth, uint32_t effectId, float mix);

// Performance Effects Functions
void ether_trigger_reverb_throw(EtherSynthCpp* synth);
void ether_trigger_delay_throw(EtherSynthCpp* synth);
void ether_set_performance_filter(EtherSynthCpp* synth, float cutoff, float resonance, int filterType);
void ether_toggle_note_repeat(EtherSynthCpp* synth, int division);
void ether_set_reverb_send(EtherSynthCpp* synth, float sendLevel);
void ether_set_delay_send(EtherSynthCpp* synth, float sendLevel);

// Effects Preset Functions
void ether_save_effects_preset(EtherSynthCpp* synth, int slot, const char* name);
bool ether_load_effects_preset(EtherSynthCpp* synth, int slot);
void ether_get_effects_preset_names(EtherSynthCpp* synth, char* names, size_t namesSize);

// Effects Metering Functions
float ether_get_reverb_level(EtherSynthCpp* synth);
float ether_get_delay_level(EtherSynthCpp* synth);
float ether_get_compression_reduction(EtherSynthCpp* synth);
float ether_get_lufs_level(EtherSynthCpp* synth);
float ether_get_peak_level(EtherSynthCpp* synth);
bool ether_is_limiter_active(EtherSynthCpp* synth);

// Pattern Chain Management Functions
void ether_create_pattern_chain(EtherSynthCpp* synth, uint32_t startPatternId, const uint32_t* patternIds, int patternCount);
void ether_queue_pattern(EtherSynthCpp* synth, uint32_t patternId, int trackIndex, int triggerMode);
void ether_trigger_pattern(EtherSynthCpp* synth, uint32_t patternId, int trackIndex, bool immediate);
uint32_t ether_get_current_pattern(EtherSynthCpp* synth, int trackIndex);
uint32_t ether_get_queued_pattern(EtherSynthCpp* synth, int trackIndex);
void ether_set_chain_mode(EtherSynthCpp* synth, int trackIndex, int chainMode);
int ether_get_chain_mode(EtherSynthCpp* synth, int trackIndex);

// Live Performance Functions  
void ether_arm_pattern(EtherSynthCpp* synth, uint32_t patternId, int trackIndex);
void ether_launch_armed_patterns(EtherSynthCpp* synth);
void ether_set_performance_mode(EtherSynthCpp* synth, bool enabled);
void ether_set_global_quantization(EtherSynthCpp* synth, int bars);

// Pattern Variations and Mutations
void ether_generate_pattern_variation(EtherSynthCpp* synth, uint32_t sourcePatternId, float mutationAmount);
void ether_apply_euclidean_rhythm(EtherSynthCpp* synth, uint32_t patternId, int steps, int pulses, int rotation);
void ether_morph_pattern_timing(EtherSynthCpp* synth, uint32_t patternId, float swingAmount, float humanizeAmount);

// Scene Management Functions
uint32_t ether_save_scene(EtherSynthCpp* synth, const char* name);
bool ether_load_scene(EtherSynthCpp* synth, uint32_t sceneId);
void ether_delete_scene(EtherSynthCpp* synth, uint32_t sceneId);
void ether_get_scene_names(EtherSynthCpp* synth, char* names, size_t namesSize);

// Song Arrangement Functions
uint32_t ether_create_section(EtherSynthCpp* synth, int sectionType, const char* name, const uint32_t* patternIds, int patternCount);
void ether_arrange_section(EtherSynthCpp* synth, uint32_t sectionId, int position);
void ether_set_arrangement_mode(EtherSynthCpp* synth, bool enabled);
void ether_play_arrangement(EtherSynthCpp* synth, int startSection);

// Pattern Intelligence Functions
void ether_get_suggested_patterns(EtherSynthCpp* synth, uint32_t currentPattern, uint32_t* suggestions, int maxCount);
void ether_generate_intelligent_chain(EtherSynthCpp* synth, uint32_t startPattern, int chainLength);
float ether_get_pattern_compatibility(EtherSynthCpp* synth, uint32_t pattern1, uint32_t pattern2);

// Hardware Integration Functions
void ether_process_pattern_key(EtherSynthCpp* synth, int keyIndex, bool pressed, int trackIndex);
void ether_process_chain_knob(EtherSynthCpp* synth, float value, int trackIndex);

// Pattern Chain Templates
void ether_create_basic_loop(EtherSynthCpp* synth, const uint32_t* patternIds, int patternCount);
void ether_create_verse_chorus(EtherSynthCpp* synth, uint32_t versePattern, uint32_t chorusPattern);
void ether_create_build_drop(EtherSynthCpp* synth, const uint32_t* buildPatterns, int buildCount, uint32_t dropPattern);

// AI Generative Sequencer Functions
uint32_t ether_generate_pattern(EtherSynthCpp* synth, int generationMode, int musicalStyle, int complexity, int trackIndex);
void ether_set_generation_params(EtherSynthCpp* synth, float density, float tension, float creativity, float responsiveness);
void ether_evolve_pattern(EtherSynthCpp* synth, uint32_t patternId, float evolutionAmount);
uint32_t ether_generate_harmony(EtherSynthCpp* synth, uint32_t sourcePatternId);
uint32_t ether_generate_rhythm_variation(EtherSynthCpp* synth, uint32_t sourcePatternId, float variationAmount);

// AI Analysis and Learning Functions
void ether_analyze_user_performance(EtherSynthCpp* synth, const void* noteEvents, int eventCount);
void ether_set_adaptive_mode(EtherSynthCpp* synth, bool enabled);
void ether_reset_learning_model(EtherSynthCpp* synth);
float ether_get_pattern_complexity(EtherSynthCpp* synth, uint32_t patternId);
float ether_get_pattern_interest(EtherSynthCpp* synth, uint32_t patternId);

// Style and Scale Analysis
int ether_detect_musical_style(EtherSynthCpp* synth, const void* noteEvents, int eventCount);
void ether_get_scale_analysis(EtherSynthCpp* synth, int* rootNote, int* scaleType, float* confidence);
void ether_set_musical_style(EtherSynthCpp* synth, int style);
void ether_load_style_template(EtherSynthCpp* synth, int styleType);

// Real-time Generative Control
void ether_process_generative_key(EtherSynthCpp* synth, int keyIndex, bool pressed, float velocity);
void ether_process_generative_knob(EtherSynthCpp* synth, float value, int paramIndex);
void ether_get_generative_suggestions(EtherSynthCpp* synth, uint32_t* suggestions, int maxCount);
void ether_trigger_generative_event(EtherSynthCpp* synth, int eventType);

// Pattern Intelligence and Optimization
void ether_optimize_pattern_for_hardware(EtherSynthCpp* synth, uint32_t patternId);
bool ether_is_pattern_hardware_friendly(EtherSynthCpp* synth, uint32_t patternId);
void ether_quantize_pattern(EtherSynthCpp* synth, uint32_t patternId, float strength);
void ether_add_pattern_swing(EtherSynthCpp* synth, uint32_t patternId, float swingAmount);
void ether_humanize_pattern(EtherSynthCpp* synth, uint32_t patternId, float humanizeAmount);

// Generation Mode Constants
typedef enum {
    GENERATION_ASSIST = 0,
    GENERATION_GENERATE = 1,
    GENERATION_EVOLVE = 2,
    GENERATION_RESPOND = 3,
    GENERATION_HARMONIZE = 4,
    GENERATION_RHYTHMIZE = 5
} EtherGenerationMode;

// Musical Style Constants
typedef enum {
    STYLE_ELECTRONIC = 0,
    STYLE_TECHNO = 1,
    STYLE_HOUSE = 2,
    STYLE_AMBIENT = 3,
    STYLE_DRUM_AND_BASS = 4,
    STYLE_ACID = 5,
    STYLE_INDUSTRIAL = 6,
    STYLE_MELODIC = 7,
    STYLE_EXPERIMENTAL = 8,
    STYLE_CUSTOM = 9
} EtherMusicalStyle;

// Generation Complexity Constants
typedef enum {
    COMPLEXITY_SIMPLE = 0,
    COMPLEXITY_MODERATE = 1,
    COMPLEXITY_COMPLEX = 2,
    COMPLEXITY_ADAPTIVE = 3
} EtherGenerationComplexity;

// Performance Macros Functions
uint32_t ether_create_macro(EtherSynthCpp* synth, const char* name, int macroType, int triggerMode);
bool ether_delete_macro(EtherSynthCpp* synth, uint32_t macroId);
void ether_execute_macro(EtherSynthCpp* synth, uint32_t macroId, float intensity);
void ether_stop_macro(EtherSynthCpp* synth, uint32_t macroId);
void ether_bind_macro_to_key(EtherSynthCpp* synth, uint32_t macroId, int keyIndex, bool requiresShift, bool requiresAlt);
void ether_unbind_macro_from_key(EtherSynthCpp* synth, uint32_t macroId);

// Scene Management Functions
uint32_t ether_capture_scene(EtherSynthCpp* synth, const char* name);
bool ether_recall_scene(EtherSynthCpp* synth, uint32_t sceneId, float morphTime);
void ether_morph_between_scenes(EtherSynthCpp* synth, uint32_t fromSceneId, uint32_t toSceneId, float morphPosition);
bool ether_delete_scene_macro(EtherSynthCpp* synth, uint32_t sceneId);

// Live Looping Functions
uint32_t ether_create_live_loop(EtherSynthCpp* synth, const char* name, int recordingTrack);
void ether_start_loop_recording(EtherSynthCpp* synth, uint32_t loopId);
void ether_stop_loop_recording(EtherSynthCpp* synth, uint32_t loopId);
void ether_start_loop_playback(EtherSynthCpp* synth, uint32_t loopId, int targetTrack);
void ether_stop_loop_playback(EtherSynthCpp* synth, uint32_t loopId);
void ether_clear_loop(EtherSynthCpp* synth, uint32_t loopId);

// Performance Hardware Integration
void ether_process_performance_key(EtherSynthCpp* synth, int keyIndex, bool pressed, bool shiftHeld, bool altHeld);
void ether_process_performance_knob(EtherSynthCpp* synth, int knobIndex, float value);
void ether_set_performance_mode(EtherSynthCpp* synth, bool enabled);
bool ether_is_performance_mode(EtherSynthCpp* synth);

// Factory Macros
void ether_load_factory_macros(EtherSynthCpp* synth);
void ether_create_filter_sweep_macro(EtherSynthCpp* synth, const char* name, float startCutoff, float endCutoff, float duration);
void ether_create_volume_fade_macro(EtherSynthCpp* synth, const char* name, float targetVolume, float fadeTime);
void ether_create_tempo_ramp_macro(EtherSynthCpp* synth, const char* name, float targetTempo, float rampTime);

// Performance Statistics
void ether_get_performance_stats(EtherSynthCpp* synth, int* macrosExecuted, int* scenesRecalled, float* averageRecallTime);
void ether_reset_performance_stats(EtherSynthCpp* synth);

// Macro Type Constants
typedef enum {
    MACRO_PARAMETER_SET = 0,
    MACRO_PATTERN_TRIGGER = 1,
    MACRO_EFFECT_CHAIN = 2,
    MACRO_SCENE_MORPH = 3,
    MACRO_FILTER_SWEEP = 4,
    MACRO_VOLUME_FADE = 5,
    MACRO_TEMPO_RAMP = 6,
    MACRO_HARMONY_STACK = 7,
    MACRO_RHYTHM_FILL = 8,
    MACRO_LOOP_CAPTURE = 9,
    MACRO_CUSTOM = 10
} EtherMacroType;

// Trigger Mode Constants
typedef enum {
    TRIGGER_IMMEDIATE = 0,
    TRIGGER_QUANTIZED = 1,
    TRIGGER_HOLD = 2,
    TRIGGER_TOGGLE = 3,
    TRIGGER_TIMED = 4
} EtherTriggerMode;

// Euclidean Sequencer Functions
void ether_set_euclidean_pattern(EtherSynthCpp* synth, int trackIndex, int pulses, int rotation);
void ether_set_euclidean_probability(EtherSynthCpp* synth, int trackIndex, float probability);
void ether_set_euclidean_swing(EtherSynthCpp* synth, int trackIndex, float swing);
void ether_set_euclidean_humanization(EtherSynthCpp* synth, int trackIndex, float humanization);
bool ether_should_trigger_euclidean_step(EtherSynthCpp* synth, int trackIndex, int stepIndex);
float ether_get_euclidean_step_velocity(EtherSynthCpp* synth, int trackIndex, int stepIndex);
float ether_get_euclidean_step_timing(EtherSynthCpp* synth, int trackIndex, int stepIndex);

// Euclidean Pattern Analysis
float ether_get_euclidean_density(EtherSynthCpp* synth, int trackIndex);
int ether_get_euclidean_complexity(EtherSynthCpp* synth, int trackIndex);
void ether_get_euclidean_active_steps(EtherSynthCpp* synth, int trackIndex, int* activeSteps, int* stepCount);

// Euclidean Presets
void ether_load_euclidean_preset(EtherSynthCpp* synth, int trackIndex, const char* presetName);
void ether_save_euclidean_preset(EtherSynthCpp* synth, int trackIndex, const char* presetName);
void ether_get_euclidean_preset_names(EtherSynthCpp* synth, char* names, size_t namesSize);

// Euclidean Hardware Integration
void ether_process_euclidean_key(EtherSynthCpp* synth, int keyIndex, bool pressed, int trackIndex);
void ether_visualize_euclidean_pattern(EtherSynthCpp* synth, int trackIndex, uint32_t* displayBuffer, int width, int height);

// Euclidean Advanced Features
void ether_enable_euclidean_polyrhythm(EtherSynthCpp* synth, int trackIndex, bool enabled);
void ether_set_euclidean_pattern_offset(EtherSynthCpp* synth, int trackIndex, int offset);
void ether_link_euclidean_patterns(EtherSynthCpp* synth, int track1, int track2, bool linked);
void ether_regenerate_all_euclidean_patterns(EtherSynthCpp* synth);

// Euclidean Algorithm Constants
typedef enum {
    EUCLIDEAN_BJORKLUND = 0,
    EUCLIDEAN_BRESENHAM = 1,
    EUCLIDEAN_FRACTIONAL = 2,
    EUCLIDEAN_GOLDEN_RATIO = 3
} EtherEuclideanAlgorithm;

// Enhanced chord system - Bicep mode multi-engine chord distribution
void ether_chord_set_type(EtherSynthCpp* synth, int chordType);
void ether_chord_set_root(EtherSynthCpp* synth, int rootNote);
void ether_chord_play(EtherSynthCpp* synth, int rootNote, float velocity);
void ether_chord_release(EtherSynthCpp* synth);

// Voice configuration
void ether_chord_set_voice_engine(EtherSynthCpp* synth, int voiceIndex, int engineType);
void ether_chord_set_voice_level(EtherSynthCpp* synth, int voiceIndex, float level);
void ether_chord_set_voice_pan(EtherSynthCpp* synth, int voiceIndex, float pan);
void ether_chord_enable_voice(EtherSynthCpp* synth, int voiceIndex, bool enabled);

// Multi-instrument assignment (Bicep mode)
void ether_chord_assign_instrument(EtherSynthCpp* synth, int instrumentIndex, const int* voiceIndices, int voiceCount);
void ether_chord_set_instrument_strum(EtherSynthCpp* synth, int instrumentIndex, float offsetMs);
void ether_chord_set_instrument_arpeggio(EtherSynthCpp* synth, int instrumentIndex, bool enabled, float rate);
void ether_chord_clear_assignments(EtherSynthCpp* synth);

// Chord parameters
void ether_chord_set_spread(EtherSynthCpp* synth, float spread);
void ether_chord_set_strum_time(EtherSynthCpp* synth, float strumMs);
void ether_chord_set_voice_leading(EtherSynthCpp* synth, bool enabled);

// Arrangement presets
void ether_chord_load_arrangement(EtherSynthCpp* synth, const char* presetName);
void ether_chord_save_arrangement(EtherSynthCpp* synth, const char* presetName);
int ether_chord_get_arrangement_count(EtherSynthCpp* synth);
void ether_chord_get_arrangement_name(EtherSynthCpp* synth, int index, char* name, size_t nameSize);

// Chord analysis
void ether_chord_get_symbol(EtherSynthCpp* synth, int chordType, int rootNote, char* symbol, size_t symbolSize);
int ether_chord_get_tone_count(EtherSynthCpp* synth, int chordType);
void ether_chord_get_tone_names(EtherSynthCpp* synth, int chordType, int rootNote, char* names, size_t namesSize);

// Performance monitoring
float ether_chord_get_cpu_usage(EtherSynthCpp* synth);
int ether_chord_get_active_voice_count(EtherSynthCpp* synth);

// Chord type constants
typedef enum {
    CHORD_MAJOR = 0, CHORD_MAJOR_6, CHORD_MAJOR_7, CHORD_MAJOR_9, CHORD_MAJOR_ADD9,
    CHORD_MAJOR_11, CHORD_MAJOR_13, CHORD_MAJOR_6_9,
    CHORD_MINOR, CHORD_MINOR_6, CHORD_MINOR_7, CHORD_MINOR_9, CHORD_MINOR_ADD9,
    CHORD_MINOR_11, CHORD_MINOR_13, CHORD_MINOR_MAJ7,
    CHORD_DOMINANT_7, CHORD_DOMINANT_9, CHORD_DOMINANT_11, CHORD_DOMINANT_13,
    CHORD_DOMINANT_7_SHARP_5, CHORD_DOMINANT_7_FLAT_5,
    CHORD_DIMINISHED, CHORD_DIMINISHED_7, CHORD_HALF_DIMINISHED_7,
    CHORD_AUGMENTED, CHORD_AUGMENTED_7, CHORD_AUGMENTED_MAJ7,
    CHORD_SUS_2, CHORD_SUS_4, CHORD_SEVEN_SUS_4,
    CHORD_COUNT
} EtherChordType;

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

// Engine management functions
int ether_get_engine_type_count(EtherSynthCpp* synth);
const char* ether_get_engine_type_name(EtherSynthCpp* synth, int engineType);
int ether_get_instrument_engine_type(EtherSynthCpp* synth, int instrumentIndex);
void ether_set_instrument_engine_type(EtherSynthCpp* synth, int instrumentIndex, int engineType);

#ifdef __cplusplus
}
#endif

#endif /* EtherSynthBridge_h */
