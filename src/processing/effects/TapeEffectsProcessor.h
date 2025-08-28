#pragma once
#include <cmath>
#include <array>
#include <vector>
#include <map>
#include <string>
#include "../synthesis/DSPUtils.h"

/**
 * TapeEffectsProcessor - Comprehensive analog tape saturation and dynamics
 * 
 * Models the sonic characteristics of analog tape recording including:
 * - Non-linear saturation with program-dependent behavior
 * - Tape compression with automatic gain control
 * - Harmonic generation and frequency response coloration
 * - Multiple tape machine types (vintage, modern, exotic)
 * - Wow/flutter simulation for authentic tape movement
 * - Bias and equalization modeling
 * 
 * STM32 H7 optimized with lookup tables and efficient algorithms.
 */
class TapeEffectsProcessor {
public:
    enum class TapeType {
        VINTAGE_TUBE,       // Classic tube-based tape machine warmth
        MODERN_SOLID,       // Clean modern tape machine character
        VINTAGE_TRANSISTOR, // 70s/80s transistor-based machines
        EXOTIC_DIGITAL,     // Digital tape simulation with artifacts
        CUSTOM              // User-defined tape characteristics
    };
    
    enum class TapeMaterial {
        TYPE_I_NORMAL,      // Standard ferric oxide tape
        TYPE_II_CHROME,     // Chromium dioxide high-bias tape
        TYPE_III_FERRICHROME, // Dual-layer ferric/chrome tape
        TYPE_IV_METAL,      // Metal particle tape (high output)
        VINTAGE_ACETATE     // Early acetate-based tape (lo-fi)
    };
    
    enum class TapeSpeed {
        IPS_1_875,          // 1⅞ ips - Very lo-fi, heavy saturation
        IPS_3_75,           // 3¾ ips - Standard cassette speed
        IPS_7_5,            // 7½ ips - Professional quality
        IPS_15,             // 15 ips - High-quality mastering
        IPS_30              // 30 ips - Pristine quality
    };
    
    struct TapeConfig {
        TapeType machineType = TapeType::VINTAGE_TUBE;
        TapeMaterial material = TapeMaterial::TYPE_I_NORMAL;
        TapeSpeed speed = TapeSpeed::IPS_7_5;
        
        // Saturation parameters
        float saturationAmount = 0.3f;     // Overall saturation level (0.0-1.0)
        float saturationAsymmetry = 0.1f;  // Positive/negative clipping asymmetry
        float harmonicContent = 0.2f;      // Amount of harmonic generation
        float transientResponse = 0.7f;    // Transient preservation vs saturation
        
        // Compression parameters
        float compressionAmount = 0.4f;    // Overall compression level
        float attackTime = 5.0f;           // Attack time in milliseconds
        float releaseTime = 100.0f;        // Release time in milliseconds
        float compressionRatio = 3.0f;     // Compression ratio
        bool programDependentTiming = true; // Adaptive attack/release
        
        // Frequency response
        float lowFreqBoost = 0.2f;         // Low-frequency warmth boost
        float highFreqRolloff = 0.3f;      // High-frequency rolloff
        float midFreqColoration = 0.1f;    // Mid-frequency character
        float biasLevel = 0.5f;            // Tape bias level
        
        // Modulation and artifacts
        float wowAmount = 0.05f;           // Slow pitch variations
        float flutterAmount = 0.03f;       // Fast pitch variations
        float noiseFloor = -60.0f;         // Tape noise floor in dB
        float dropoutRate = 0.001f;        // Occasional dropouts per second
        
        // Advanced modeling
        float tapeWidth = 0.25f;           // Tape width in inches (affects headroom)
        float headGap = 2.5f;              // Playback head gap in micrometers
        float hysteresis = 0.15f;          // Magnetic hysteresis modeling
        float printThrough = 0.02f;        // Print-through between layers
        
        bool bypassable = true;            // Allow complete bypass
        float wetDryMix = 1.0f;           // Wet/dry mix (0.0=dry, 1.0=wet)
    };
    
    struct TapeState {
        // Saturation state
        float lastSaturationInput = 0.0f;
        float lastSaturationOutput = 0.0f;
        DSP::SmoothParam saturationSmoothing;
        
        // Compression state
        float compressorEnvelope = 0.0f;
        float gainReduction = 0.0f;
        DSP::SmoothParam attackSmoothing;
        DSP::SmoothParam releaseSmoothing;
        
        // Frequency response filters
        DSP::SVF lowShelfFilter;
        DSP::SVF highShelfFilter;
        DSP::SVF presenceFilter;
        DSP::Audio::DCBlocker dcBlocker;
        
        // Modulation oscillators
        float wowPhase = 0.0f;
        float flutterPhase = 0.0f;
        DSP::Random noiseGenerator;
        
        // Delay lines for modulation
        std::array<float, 1024> delayBuffer;
        int delayWritePtr = 0;
        
        // Harmonic generation state
        std::array<float, 8> harmonicPhases;
        std::array<float, 8> harmonicGains;
        
        // Advanced modeling state
        float hysteresisHistory = 0.0f;
        float printThroughDelay = 0.0f;
        uint32_t dropoutTimer = 0;
        bool inDropout = false;
        
        TapeState() : saturationSmoothing(0.0f, 1.0f), 
                     attackSmoothing(0.0f, 5.0f),
                     releaseSmoothing(0.0f, 5.0f) {
            harmonicPhases.fill(0.0f);
            harmonicGains.fill(0.0f);
            delayBuffer.fill(0.0f);
        }
    };
    
    TapeEffectsProcessor();
    ~TapeEffectsProcessor() = default;
    
    // Configuration
    void setTapeConfig(const TapeConfig& config);
    const TapeConfig& getTapeConfig() const { return config_; }
    void setTapeType(TapeType type);
    void setTapeMaterial(TapeMaterial material);
    void setTapeSpeed(TapeSpeed speed);
    
    // Runtime control
    void setSaturationAmount(float amount);
    void setCompressionAmount(float amount);
    void setWetDryMix(float mix);
    void setBypassed(bool bypassed);
    
    // Processing
    float processSample(float input);
    void processBlock(const float* input, float* output, int numSamples);
    void processStereo(const float* inputL, const float* inputR, 
                      float* outputL, float* outputR, int numSamples);
    
    // Preset management
    void loadPreset(const std::string& presetName);
    std::vector<std::string> getAvailablePresets() const;
    void savePreset(const std::string& name, const TapeConfig& config);
    
    // Analysis and metering
    float getSaturationAmount() const;
    float getCompressionReduction() const;
    float getHarmonicContent() const;
    float getOutputLevel() const;
    
    // System control
    void reset();
    void setSampleRate(float sampleRate);
    float getSampleRate() const { return sampleRate_; }
    
    // Utility functions
    static std::string tapeTypeToString(TapeType type);
    static std::string tapeMaterialToString(TapeMaterial material);
    static std::string tapeSpeedToString(TapeSpeed speed);
    
private:
    // Configuration and state
    TapeConfig config_;
    TapeState state_;
    float sampleRate_ = 48000.0f;
    bool bypassed_ = false;
    
    // Lookup tables for efficiency
    static constexpr int SATURATION_TABLE_SIZE = 1024;
    std::array<float, SATURATION_TABLE_SIZE> saturationLUT_;
    std::array<float, SATURATION_TABLE_SIZE> harmonicLUT_;
    
    // Preset storage
    std::map<std::string, TapeConfig> presets_;
    
    // Processing components
    float processSaturation(float input);
    float processCompression(float input, float& envelope);
    float processFrequencyResponse(float input);
    float processModulation(float input);
    float processHarmonicGeneration(float input, float fundamental);
    float processAdvancedModeling(float input);
    
    // Saturation algorithms
    float vintageeTubeSaturation(float input, float amount);
    float modernSolidStateSaturation(float input, float amount);
    float transistorSaturation(float input, float amount);
    float digitalTapeSaturation(float input, float amount);
    
    // Compression algorithms
    float calculateCompressionGain(float input, float envelope);
    void updateCompressionEnvelope(float input, float& envelope);
    float calculateProgramDependentTiming(float input, bool isAttack);
    
    // Frequency response modeling
    void updateFrequencyFilters();
    float calculateBiasEffect(float input);
    float calculateHeadGapEffect(float input, float frequency);
    
    // Modulation and artifacts
    void updateModulationOscillators();
    float generateTapeNoise();
    bool shouldGenerateDropout();
    
    // Harmonic generation
    void updateHarmonicGenerators(float fundamental);
    float calculateEvenHarmonics(float input);
    float calculateOddHarmonics(float input);
    
    // Advanced modeling
    float processHysteresis(float input);
    float processPrintThrough(float input);
    float processNonlinearity(float input, float level);
    
    // Lookup table generation
    void generateSaturationLUT();
    void generateHarmonicLUT();
    void initializePresets();
    
    // Utility functions
    float dbToLinear(float db) const { return DSP::Audio::dbToLinear(db); }
    float linearToDb(float linear) const { return DSP::Audio::linearToDb(linear); }
    float interpolateTable(const std::array<float, SATURATION_TABLE_SIZE>& table, float index) const;
    
    // Constants
    static constexpr float PI = M_PI;
    static constexpr float TWO_PI = 2.0f * PI;
    static constexpr float WOW_FREQUENCY = 0.5f;        // Hz
    static constexpr float FLUTTER_FREQUENCY = 6.0f;    // Hz
    static constexpr float DROPOUT_MIN_DURATION = 0.001f; // seconds
    static constexpr float DROPOUT_MAX_DURATION = 0.01f;  // seconds
    static constexpr float HYSTERESIS_AMOUNT = 0.1f;
    static constexpr float PRINT_THROUGH_DELAY = 0.001f; // seconds
};