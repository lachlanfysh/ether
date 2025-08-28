#pragma once
#include "../synthesis/SynthEngine.h"
#include <array>
#include <memory>

/**
 * MacroChord - Chord Generation Engine with H/T/M Mapping
 * 
 * HARMONICS: detune spread + LPF cutoff (6-voice unison → wide chord)
 * TIMBRE: voicing complexity (triad → 7th → 9th → 11th progressions)
 * MORPH: chord↔single (blend between full chord and single note)
 * 
 * Features:
 * - Multi-voice chord generation with up to 6 notes
 * - Intelligent voicing system (triads to extended chords)
 * - Detuning spread for unison thickness
 * - Chord/single morphing for dynamics
 * - Low-pass filtering integrated with harmonic spread
 */
class MacroChordEngine : public SynthEngine {
public:
    MacroChordEngine();
    ~MacroChordEngine() override;
    
    // SynthEngine interface
    EngineType getType() const override { return EngineType::MACRO_CHORD; }
    const char* getName() const override { return "MacroChord"; }
    const char* getDescription() const override { return "Chord generation with H/T/M control"; }
    
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
    void setHarmonics(float harmonics);    // 0-1: detune spread + LPF cutoff
    void setTimbre(float timbre);          // 0-1: voicing complexity
    void setMorph(float morph);            // 0-1: chord↔single blend

private:
    // Chord voicing system
    struct ChordVoicing {
        static constexpr int MAX_CHORD_NOTES = 6;
        
        enum class VoicingType {
            TRIAD,      // 3 notes: root, 3rd, 5th
            SEVENTH,    // 4 notes: root, 3rd, 5th, 7th
            NINTH,      // 5 notes: root, 3rd, 5th, 7th, 9th
            ELEVENTH    // 6 notes: root, 3rd, 5th, 7th, 9th, 11th
        };
        
        struct ChordNote {
            int semitoneOffset = 0;     // Offset from root in semitones
            float detuneAmount = 0.0f;  // Individual detune in cents
            float levelScale = 1.0f;    // Level scaling for this note
        };
        
        VoicingType currentVoicing = VoicingType::TRIAD;
        std::array<ChordNote, MAX_CHORD_NOTES> notes;
        int activeNoteCount = 3;
        
        void calculateVoicing(float timbre);
        void calculateDetune(float harmonics);
        void calculateLevels(float morph, float harmonics);
        
    private:
        // Chord intervals for different voicings
        static constexpr int TRIAD_INTERVALS[3] = {0, 4, 7};        // root, maj3, 5th
        static constexpr int SEVENTH_INTERVALS[4] = {0, 4, 7, 10};  // + min7
        static constexpr int NINTH_INTERVALS[5] = {0, 4, 7, 10, 14}; // + maj9
        static constexpr int ELEVENTH_INTERVALS[6] = {0, 4, 7, 10, 14, 17}; // + 11th
        
        float detuneSpread_ = 0.0f;
    };
    
    // Chord Voice implementation
    class MacroChordVoice {
    public:
        MacroChordVoice();
        
        void noteOn(uint8_t rootNote, float velocity, float aftertouch, float sampleRate, 
                   const ChordVoicing& voicing);
        void noteOff();
        void setAftertouch(float aftertouch);
        
        AudioFrame processSample();
        
        bool isActive() const { return active_; }
        bool isReleasing() const { return envelope_.isReleasing(); }
        uint8_t getNote() const { return rootNote_; }
        uint32_t getAge() const { return age_; }
        
        // Parameter control
        void setChordParams(const ChordVoicing& voicing);
        void setFilterParams(float cutoff, float resonance);
        void setMorphParams(float chordSingleBlend);
        void setVolume(float volume);
        void setEnvelopeParams(float attack, float decay, float sustain, float release);
        
    private:
        // Individual oscillator for each chord note
        struct ChordOscillator {
            float phase = 0.0f;
            float frequency = 440.0f;
            float increment = 0.0f;
            float level = 1.0f;
            bool active = false;
            
            void setFrequency(float freq, float sampleRate) {
                frequency = freq;
                increment = 2.0f * M_PI * freq / sampleRate;
            }
            
            void setLevel(float lvl) {
                level = lvl;
            }
            
            float processSaw() {
                if (!active) return 0.0f;
                
                float output = (2.0f * phase / (2.0f * M_PI)) - 1.0f;
                output *= level;
                
                phase += increment;
                if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
                
                return output;
            }
        };
        
        // State variable filter for chord processing
        struct StateVariableFilter {
            float cutoff = 1000.0f;
            float resonance = 0.0f;
            float low = 0.0f, band = 0.0f, high = 0.0f;
            float f = 0.1f, q = 0.5f;
            float sampleRate = 48000.0f;
            
            void setCutoff(float freq) {
                cutoff = std::clamp(freq, 20.0f, sampleRate * 0.45f);
                updateCoefficients();
            }
            
            void setResonance(float res) {
                resonance = std::clamp(res, 0.0f, 0.95f);
                updateCoefficients();
            }
            
            void updateCoefficients() {
                f = 2.0f * std::sin(M_PI * cutoff / sampleRate);
                q = 1.0f - resonance;
            }
            
            float processLowpass(float input) {
                low += f * band;
                high = input - low - q * band;
                band += f * high;
                
                return low;
            }
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
        uint8_t rootNote_ = 60;
        float velocity_ = 0.8f;
        float aftertouch_ = 0.0f;
        uint32_t age_ = 0;
        
        // Chord oscillators (up to 6 notes)
        std::array<ChordOscillator, ChordVoicing::MAX_CHORD_NOTES> oscillators_;
        StateVariableFilter filter_;
        Envelope envelope_;
        
        // Voice parameters
        float chordSingleBlend_ = 0.0f;  // 0=chord, 1=single note
        float volume_ = 0.8f;
        float rootFrequency_ = 440.0f;
        int activeOscCount_ = 3;
    };
    
    // Voice management
    std::array<MacroChordVoice, MAX_VOICES> voices_;
    uint32_t voiceCounter_ = 0;
    
    MacroChordVoice* findFreeVoice();
    MacroChordVoice* findVoice(uint8_t note);
    MacroChordVoice* stealVoice();
    
    // H/T/M Parameters
    float harmonics_ = 0.0f;     // detune spread + LPF cutoff
    float timbre_ = 0.3f;        // voicing complexity
    float morph_ = 0.0f;         // chord↔single blend
    
    // Derived parameters from H/T/M
    ChordVoicing chordVoicing_;
    float filterCutoff_ = 2000.0f;
    float filterResonance_ = 0.2f;
    float chordSingleBlend_ = 0.0f;
    
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
    
    // Mapping functions
    float mapDetuneSpread(float harmonics) const;
    float mapFilterCutoff(float harmonics) const;
    float mapFilterResonance(float harmonics) const;
    ChordVoicing::VoicingType mapVoicingType(float timbre) const;
    float mapChordSingleBlend(float morph) const;
};