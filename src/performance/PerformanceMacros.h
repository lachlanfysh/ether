#pragma once
#include "../core/Types.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <functional>

/**
 * PerformanceMacros - Advanced performance shortcuts and workflow automation
 * 
 * Professional live performance system for EtherSynth V1.0:
 * - Hardware-mapped performance shortcuts for instant access
 * - Multi-parameter macros with intelligent scaling
 * - Scene snapshots with morphing capabilities
 * - Live looping and pattern capture
 * - Performance mode optimizations for stage use
 * - Hardware integration for 960×320 + 2×16 key interface
 */

namespace EtherSynth {

class PerformanceMacros {
public:
    enum class MacroType : uint8_t {
        PARAMETER_SET = 0,    // Set multiple parameters instantly
        PATTERN_TRIGGER,      // Complex pattern triggering sequence
        EFFECT_CHAIN,         // Effects chain manipulation
        SCENE_MORPH,          // Scene morphing between states
        FILTER_SWEEP,         // Automated filter sweeps
        VOLUME_FADE,          // Volume fade in/out
        TEMPO_RAMP,           // BPM ramping
        HARMONY_STACK,        // Instant harmony generation
        RHYTHM_FILL,          // Drum fills and variations
        LOOP_CAPTURE,         // Live loop recording/playback
        CUSTOM,               // User-defined macro
        COUNT
    };
    
    enum class TriggerMode : uint8_t {
        IMMEDIATE = 0,        // Execute immediately
        QUANTIZED,           // Execute on next bar/beat
        HOLD,                // Execute while held
        TOGGLE,              // Toggle on/off state
        TIMED,               // Execute for specific duration
        COUNT
    };
    
    // Performance macro definition
    struct PerformanceMacro {
        uint32_t id;
        std::string name;
        MacroType type;
        TriggerMode triggerMode;
        
        // Key binding
        int keyIndex = -1;            // Hardware key (0-31) or -1 for unbound
        bool requiresShift = false;   // Requires SHIFT modifier
        bool requiresAlt = false;     // Requires ALT modifier
        
        // Parameters
        std::map<std::string, float> parameters;  // Parameter name -> value
        std::vector<uint32_t> targetTracks;       // Which tracks to affect
        
        // Timing
        float duration = 0.0f;        // Duration for timed macros (seconds)
        float fadeTime = 0.1f;        // Fade/transition time
        bool quantizeToBar = true;    // Quantize to bar boundaries
        
        // Advanced options
        float probability = 1.0f;     // Execution probability
        int maxExecutions = -1;       // Max times to execute (-1 = unlimited)
        int currentExecutions = 0;    // Current execution count
        
        // State
        bool isActive = false;        // Currently executing
        bool isEnabled = true;        // Macro enabled
        float progress = 0.0f;        // Execution progress (0-1)
        
        // Visual feedback
        uint32_t color = 0xFF6B73;    // Macro color for UI
        std::string category = "General"; // Category for organization
        
        PerformanceMacro() : id(generateMacroId()) {}
        
    private:
        static uint32_t generateMacroId() {
            static uint32_t counter = 1;
            return counter++;
        }
    };
    
    // Scene snapshot with morphing capability
    struct SceneSnapshot {
        uint32_t id;
        std::string name;
        
        // Complete synth state
        std::map<int, float> trackVolumes;         // Track volumes
        std::map<int, float> trackPans;            // Track pans
        std::map<int, bool> trackMutes;            // Track mute states
        std::map<int, uint32_t> trackPatterns;     // Active patterns
        std::map<int, int> trackEngines;           // Engine assignments
        
        // Parameter snapshots per track/engine
        std::map<std::string, float> globalParameters; // Global parameters
        std::map<int, std::map<std::string, float>> trackParameters; // Per-track parameters
        
        // Effects state
        std::map<uint32_t, std::map<std::string, float>> effectParameters;
        std::map<uint32_t, bool> effectStates;     // Effect enabled states
        
        // Performance state
        float masterVolume = 0.8f;
        float masterTempo = 120.0f;
        bool performanceMode = false;
        
        // Metadata
        uint32_t color = 0x4ECDC4;
        std::string description = "";
        float recallTime = 0.5f;      // Time to recall scene
        
        SceneSnapshot() : id(generateSceneId()) {}
        
    private:
        static uint32_t generateSceneId() {
            static uint32_t counter = 1000;
            return counter++;
        }
    };
    
    // Live loop for performance capture
    struct LiveLoop {
        uint32_t id;
        std::string name;
        
        // Loop data
        std::vector<NoteEvent> recordedEvents;
        float loopLength = 4.0f;      // Length in bars
        bool isRecording = false;
        bool isPlaying = false;
        bool overdubEnabled = false;
        
        // Playback parameters
        float volume = 1.0f;
        int trackAssignment = -1;     // Which track to play on (-1 = free)
        bool quantizePlayback = true;
        
        // Recording parameters
        int recordingTrack = -1;      // Which track to record from
        float recordThreshold = 0.1f; // Minimum velocity to record
        bool autoStart = true;        // Auto-start recording on first note
        
        // Performance options
        bool syncToTempo = true;
        float playbackSpeed = 1.0f;   // Playback speed multiplier
        int loopCount = -1;           // How many times to loop (-1 = infinite)
        
        LiveLoop() : id(generateLoopId()) {}
        
    private:
        static uint32_t generateLoopId() {
            static uint32_t counter = 2000;
            return counter++;
        }
    };
    
    PerformanceMacros();
    ~PerformanceMacros() = default;
    
    // Macro Management
    uint32_t createMacro(const std::string& name, MacroType type, TriggerMode triggerMode);
    bool deleteMacro(uint32_t macroId);
    PerformanceMacro* getMacro(uint32_t macroId);
    std::vector<PerformanceMacro> getAllMacros() const;
    std::vector<PerformanceMacro> getMacrosByCategory(const std::string& category) const;
    
    // Macro Execution
    void executeMacro(uint32_t macroId, float intensity = 1.0f);
    void stopMacro(uint32_t macroId);
    void stopAllMacros();
    void processMacroUpdates(float deltaTime);
    
    // Key Binding
    void bindMacroToKey(uint32_t macroId, int keyIndex, bool requiresShift = false, bool requiresAlt = false);
    void unbindMacroFromKey(uint32_t macroId);
    uint32_t getMacroForKey(int keyIndex, bool shiftHeld = false, bool altHeld = false) const;
    
    // Hardware Integration
    void processPerformanceKey(int keyIndex, bool pressed, bool shiftHeld = false, bool altHeld = false);
    void processPerformanceKnob(int knobIndex, float value);
    void setPerformanceMode(bool enabled) { performanceMode_ = enabled; }
    bool isPerformanceMode() const { return performanceMode_; }
    
    // Scene Management
    uint32_t captureScene(const std::string& name);
    bool recallScene(uint32_t sceneId, float morphTime = 0.5f);
    void morphBetweenScenes(uint32_t fromSceneId, uint32_t toSceneId, float morphPosition);
    bool deleteScene(uint32_t sceneId);
    std::vector<SceneSnapshot> getAllScenes() const;
    
    // Live Looping
    uint32_t createLiveLoop(const std::string& name, int recordingTrack);
    void startLoopRecording(uint32_t loopId);
    void stopLoopRecording(uint32_t loopId);
    void startLoopPlayback(uint32_t loopId, int targetTrack = -1);
    void stopLoopPlayback(uint32_t loopId);
    void clearLoop(uint32_t loopId);
    std::vector<LiveLoop> getAllLoops() const;
    
    // Preset Macros (Factory defaults)
    void loadFactoryMacros();
    void createFilterSweepMacro(const std::string& name, float startCutoff, float endCutoff, float duration);
    void createVolumeFadeMacro(const std::string& name, float targetVolume, float fadeTime);
    void createTempoRampMacro(const std::string& name, float targetTempo, float rampTime);
    void createHarmonyStackMacro(const std::string& name, const std::vector<int>& intervals);
    
    // Performance Statistics
    struct PerformanceStats {
        int macrosExecuted = 0;
        int scenesRecalled = 0;
        int loopsRecorded = 0;
        float averageRecallTime = 0.0f;
        int keyPressesPerMinute = 0;
        std::map<uint32_t, int> macroUsageCount;
    };
    
    PerformanceStats getPerformanceStats() const { return stats_; }
    void resetPerformanceStats() { stats_ = PerformanceStats(); }
    
    // Advanced Features
    void setMacroChaining(uint32_t primaryMacroId, const std::vector<uint32_t>& chainedMacroIds);
    void enableMacroRandomization(uint32_t macroId, float randomAmount);
    void setMacroCondition(uint32_t macroId, std::function<bool()> condition);
    
private:
    // Core data
    std::map<uint32_t, PerformanceMacro> macros_;
    std::map<uint32_t, SceneSnapshot> scenes_;
    std::map<uint32_t, LiveLoop> liveLoops_;
    
    // Key bindings
    std::map<int, uint32_t> keyBindings_;         // keyIndex -> macroId
    std::map<int, uint32_t> shiftKeyBindings_;    // SHIFT + keyIndex -> macroId
    std::map<int, uint32_t> altKeyBindings_;      // ALT + keyIndex -> macroId
    
    // Performance state
    bool performanceMode_ = false;
    std::map<uint32_t, float> activeMacroTimers_; // macroId -> remaining time
    std::map<uint32_t, bool> macroHoldStates_;    // macroId -> hold state
    
    // Scene morphing state
    bool morphingActive_ = false;
    uint32_t morphFromScene_ = 0;
    uint32_t morphToScene_ = 0;
    float morphProgress_ = 0.0f;
    float morphDuration_ = 0.5f;
    
    // Live loop state
    std::map<uint32_t, float> loopTimers_;        // loopId -> recording time
    uint32_t activeRecordingLoop_ = 0;            // Currently recording loop
    
    // Performance statistics
    mutable PerformanceStats stats_;
    
    // Hardware state
    std::array<bool, 32> keyStates_;              // Current key states
    std::array<float, 16> knobValues_;            // Current knob values
    bool shiftHeld_ = false;
    bool altHeld_ = false;
    
    // Timing
    float sampleRate_ = 48000.0f;
    float currentTempo_ = 120.0f;
    uint64_t sampleCounter_ = 0;
    
    // Helper methods
    void executePareameterSetMacro(const PerformanceMacro& macro, float intensity);
    void executeFilterSweepMacro(const PerformanceMacro& macro, float intensity);
    void executeVolumeFadeMacro(const PerformanceMacro& macro, float intensity);
    void executeSceneMorphMacro(const PerformanceMacro& macro, float intensity);
    void executeLoopCaptureMacro(const PerformanceMacro& macro, float intensity);
    
    void updateMacroProgress(uint32_t macroId, float deltaTime);
    bool shouldExecuteMacro(const PerformanceMacro& macro) const;
    void applyQuantization(PerformanceMacro& macro);
    
    // Scene morphing helpers
    float interpolateParameter(float from, float to, float position) const;
    void applySceneParameters(const SceneSnapshot& scene, float weight = 1.0f);
    
    // Live loop helpers
    void processLoopRecording(uint32_t loopId, const std::vector<NoteEvent>& events);
    void quantizeLoop(LiveLoop& loop);
    std::vector<NoteEvent> generateLoopPlayback(const LiveLoop& loop, float currentTime);
};

// Factory macro templates
namespace MacroTemplates {
    PerformanceMacros::PerformanceMacro createFilterSweep(const std::string& name, float duration = 2.0f);
    PerformanceMacros::PerformanceMacro createVolumeSwell(const std::string& name, float duration = 1.0f);
    PerformanceMacros::PerformanceMacro createTempoHalftime(const std::string& name);
    PerformanceMacros::PerformanceMacro createTempoDoubletime(const std::string& name);
    PerformanceMacros::PerformanceMacro createHarmonyChord(const std::string& name, const std::vector<int>& intervals);
    PerformanceMacros::PerformanceMacro createRhythmicFill(const std::string& name, int complexity = 2);
    PerformanceMacros::PerformanceMacro createReverbThrow(const std::string& name, float throwAmount = 0.8f);
    PerformanceMacros::PerformanceMacro createDelayFeedback(const std::string& name, float feedbackAmount = 0.6f);
}

// Hardware mapping utilities
namespace PerformanceHardware {
    void mapMacrosToKeys(PerformanceMacros& macros, const std::vector<uint32_t>& macroIds);
    void setupDefaultKeyLayout(PerformanceMacros& macros);
    void visualizeMacroMapping(const PerformanceMacros& macros, uint32_t* displayBuffer, int width, int height);
    std::string getKeyDescription(int keyIndex, bool shiftHeld = false, bool altHeld = false);
}

} // namespace EtherSynth