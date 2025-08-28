#pragma once
#include "../synthesis/SynthEngine.h"
#include <array>
#include <memory>
#include <vector>

/**
 * MacroWavetable - Wavetable Engine with Vector Path Scrubbing
 * 
 * HARMONICS: position scan (band-limited interpolation)
 * TIMBRE: formant shift −6 → +6 st; spectral tilt ±3 dB
 * MORPH: Vector Path Scrub (see Vector Path system)
 * 
 * Features:
 * - Band-limited wavetable interpolation
 * - Formant shifting and spectral tilt
 * - Vector Path scrubbing with 4 corner sources (A/B/C/D)
 * - Catmull-Rom interpolation between waypoints
 * - Equal-power bilinear blending
 * - Latch mode for automatic path traversal
 */
class MacroWavetableEngine : public SynthEngine {
public:
    MacroWavetableEngine();
    ~MacroWavetableEngine() override;
    
    // SynthEngine interface
    EngineType getType() const override { return EngineType::MACRO_WAVETABLE; }
    const char* getName() const override { return "MacroWavetable"; }
    const char* getDescription() const override { return "Wavetable with Vector Path control"; }
    
    void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f) override;
    void noteOff(uint8_t note) override;
    void setAftertouch(uint8_t note, float aftertouch) override;
    void allNotesOff() override;
    
    void setParameter(ParameterID param, float value) override;
    float getParameter(ParameterID param) const override;
    bool hasParameter(ParameterID param) const override;
    
    void processAudio(EtherAudioBuffer& outputBuffer) override;
    
    size_t getActiveVoiceCount() const override;
    size_t getMaxVoiceCount() const override { return MAX_VOICES; }
    void setVoiceCount(size_t maxVoices) override;
    
    float getCPUUsage() const override { return cpuUsage_; }
    
    void savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const override;
    bool loadPreset(const uint8_t* data, size_t size) override;
    
    void setSampleRate(float sampleRate) override;
    void setBufferSize(size_t bufferSize) override;
    
    bool supportsPolyAftertouch() const override { return true; }
    bool supportsModulation(ParameterID target) const override;
    void setModulation(ParameterID target, float amount) override;
    
    // H/T/M Macro Controls
    void setHarmonics(float harmonics);    // 0-1: wavetable position scan
    void setTimbre(float timbre);          // 0-1: formant shift + spectral tilt
    void setMorph(float morph);            // 0-1: Vector Path position
    
    // Vector Path Controls
    void setVectorPathEnabled(bool enabled);
    void setVectorPathPosition(float position); // 0-1 along path
    void setVectorPathLatch(bool latched);
    void setVectorPathRate(float rate);         // Hz for automatic traversal
    
private:
    // Vector Path System
    struct VectorPathPoint {
        float x = 0.5f;  // 0-1
        float y = 0.5f;  // 0-1
    };
    
    struct VectorPath {
        std::vector<VectorPathPoint> waypoints;
        std::vector<float> arcLengthLUT;  // For uniform motion
        bool enabled = false;
        bool latched = false;
        float position = 0.0f;  // 0-1 along path
        float rate = 0.25f;     // Hz
        float phase = 0.0f;     // For LFO-style motion
        
        void addWaypoint(float x, float y);
        void removeWaypoint(int index);
        void clearWaypoints();
        VectorPathPoint interpolatePosition(float t) const;
        void buildArcLengthLUT();
        float getUniformPosition(float t) const;
        
    private:
        VectorPathPoint catmullRomInterp(const VectorPathPoint& p0, const VectorPathPoint& p1,
                                       const VectorPathPoint& p2, const VectorPathPoint& p3, float t) const;
    };
    
    // Corner Sources for Vector Path
    struct CornerSources {
        int sourceA = 0;  // Top-left
        int sourceB = 1;  // Top-right  
        int sourceC = 2;  // Bottom-left
        int sourceD = 3;  // Bottom-right
        
        // Equal-power bilinear blend weights
        struct BlendWeights {
            float wA, wB, wC, wD;
            float gA, gB, gC, gD; // Gain coefficients
        };
        
        BlendWeights calculateWeights(float x, float y) const;
    };
    
    // Wavetable Voice implementation
    class MacroWavetableVoice {
    public:
        MacroWavetableVoice();
        
        void noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate);
        void noteOff();
        void setAftertouch(float aftertouch);
        
        AudioFrame processSample();
        
        bool isActive() const { return active_; }
        bool isReleasing() const { return envelope_.isReleasing(); }
        uint8_t getNote() const { return note_; }
        uint32_t getAge() const { return age_; }
        
        // Parameter control
        void setWavetableParams(float position, const CornerSources::BlendWeights& blend);
        void setFormantParams(float formantShift, float spectralTilt);
        void setVolume(float volume);
        void setEnvelopeParams(float attack, float decay, float sustain, float release);
        
    private:
        // Wavetable oscillator with band-limited interpolation
        struct WavetableOscillator {
            static constexpr int WAVETABLE_SIZE = 2048;
            static constexpr int NUM_WAVETABLES = 64;
            
            float phase = 0.0f;
            float frequency = 440.0f;
            float increment = 0.0f;
            float position = 0.0f;        // 0-1 through wavetables
            float sampleRate = 48000.0f;
            
            // Simplified wavetable data (in practice, loaded from files)
            static float wavetables[NUM_WAVETABLES][WAVETABLE_SIZE];
            static bool wavetablesInitialized;
            
            void setFrequency(float freq, float sampleRate);
            void setPosition(float pos);
            float process();
            
            static void initializeWavetables();
            
        private:
            float interpolateWavetable(int tableA, int tableB, float fraction, float phase) const;
            float bandLimitedInterpolation(const float* table, float phase, float freq) const;
        };
        
        // Formant shifter (simplified implementation)
        struct FormantShifter {
            float shiftSemitones = 0.0f;
            float spectralTilt = 0.0f;
            
            // Ring buffer for pitch shifting
            static constexpr int BUFFER_SIZE = 4096;
            float buffer[BUFFER_SIZE];
            int writeIndex = 0;
            int readIndex = 0;
            float crossfade = 0.0f;
            
            void setShift(float semitones);
            void setTilt(float tiltDb);
            float process(float input);
            
        private:
            float applySpectralTilt(float input);
        };
        
        // ADSR envelope
        struct Envelope {
            enum class Stage { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };
            
            Stage stage = Stage::IDLE;
            float level = 0.0f;
            float attack = 0.01f;
            float decay = 0.3f;
            float sustain = 0.8f;
            float release = 0.5f;
            float sampleRate = 48000.0f;
            
            void noteOn() {
                stage = Stage::ATTACK;
            }
            
            void noteOff() {
                if (stage != Stage::IDLE) {
                    stage = Stage::RELEASE;
                }
            }
            
            bool isReleasing() const {
                return stage == Stage::RELEASE;
            }
            
            bool isActive() const {
                return stage != Stage::IDLE;
            }
            
            float process();
        };
        
        bool active_ = false;
        uint8_t note_ = 60;
        float velocity_ = 0.8f;
        float aftertouch_ = 0.0f;
        uint32_t age_ = 0;
        
        // Four wavetable oscillators for corner blending
        WavetableOscillator oscA_, oscB_, oscC_, oscD_;
        FormantShifter formantShifter_;
        Envelope envelope_;
        
        // Voice parameters
        float wavetablePosition_ = 0.0f;
        CornerSources::BlendWeights currentBlend_;
        float formantShift_ = 0.0f;    // -6 to +6 semitones
        float spectralTilt_ = 0.0f;    // ±3 dB
        float volume_ = 0.8f;
        float noteFrequency_ = 440.0f;
    };
    
    // Voice management
    std::array<MacroWavetableVoice, MAX_VOICES> voices_;
    uint32_t voiceCounter_ = 0;
    
    MacroWavetableVoice* findFreeVoice();
    MacroWavetableVoice* findVoice(uint8_t note);
    MacroWavetableVoice* stealVoice();
    
    // H/T/M Parameters
    float harmonics_ = 0.0f;     // wavetable position scan
    float timbre_ = 0.5f;        // formant shift + spectral tilt
    float morph_ = 0.0f;         // Vector Path position
    
    // Derived parameters from H/T/M
    float wavetablePosition_ = 0.0f;
    float formantShift_ = 0.0f;
    float spectralTilt_ = 0.0f;
    
    // Vector Path system
    VectorPath vectorPath_;
    CornerSources cornerSources_;
    CornerSources::BlendWeights currentBlend_;
    
    // Additional parameters
    float volume_ = 0.8f;
    float attack_ = 0.01f;
    float decay_ = 0.3f;
    float sustain_ = 0.8f;
    float release_ = 0.5f;
    
    // Performance monitoring
    float cpuUsage_ = 0.0f;
    void updateCPUUsage(float processingTime);
    
    // Modulation
    std::array<float, static_cast<size_t>(ParameterID::COUNT)> modulation_;
    
    // Parameter calculation and voice updates
    void calculateDerivedParams();
    void updateAllVoices();
    void updateVectorPath(float deltaTime);
    
    // Mapping functions
    float mapWavetablePosition(float harmonics) const;
    float mapFormantShift(float timbre) const;
    float mapSpectralTilt(float timbre) const;
    
public:
    // Vector Path Editor Interface (for TouchGFX)
    struct VectorPathEditor {
        bool isOpen = false;
        int selectedPointIndex = -1;
        float editX = 0.5f;
        float editY = 0.5f;
        float editCurvature = 0.5f;
        
        void openEditor();
        void closeEditor();
        void selectPoint(int index);
        void addPoint(float x, float y);
        void deleteSelectedPoint();
        void quantizeToCorners();
        void setSelectedPointPosition(float x, float y);
    } vectorPathEditor_;
    
    // Vector Path interface for UI
    const VectorPath& getVectorPath() const { return vectorPath_; }
    VectorPath& getVectorPath() { return vectorPath_; }
    VectorPathEditor& getVectorPathEditor() { return vectorPathEditor_; }
};