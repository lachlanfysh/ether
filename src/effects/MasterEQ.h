#pragma once
#include "../core/Types.h"
#include "../synthesis/DSPUtils.h"
#include "../audio/SIMDOptimizations.h"
#include <array>
#include <memory>
#include <cstdint>
#include <cmath>
#include <complex>

namespace EtherSynth {

/**
 * Professional 7-Band Master EQ for EtherSynth V1.0
 * 
 * Features:
 * - 7 bands: Sub, Low, Low-Mid, Mid, High-Mid, High, Air
 * - Multiple filter types per band (Bell, Shelf, HP/LP)
 * - Real-time frequency analysis with FFT
 * - Per-band bypass and solo
 * - Auto-gain compensation
 * - Spectrum analyzer integration
 * - Low-latency processing optimized for STM32H7
 */

class MasterEQ {
public:
    // EQ bands with musical frequency ranges
    enum class Band : uint8_t {
        SUB = 0,         // 20-80 Hz (High-pass/Low-shelf)
        LOW,             // 80-250 Hz (Bell/Shelf)
        LOW_MID,         // 250-800 Hz (Bell)
        MID,             // 800-2.5k Hz (Bell)
        HIGH_MID,        // 2.5k-8k Hz (Bell)
        HIGH,            // 8k-16k Hz (Bell/Shelf)
        AIR,             // 16k-20k Hz (High-shelf)
        COUNT
    };
    
    // Filter types available per band
    enum class FilterType : uint8_t {
        BELL = 0,        // Parametric bell (boost/cut)
        HIGH_SHELF,      // High frequency shelf
        LOW_SHELF,       // Low frequency shelf
        HIGH_PASS,       // High-pass filter
        LOW_PASS,        // Low-pass filter
        NOTCH,           // Notch filter (narrow cut)
        COUNT
    };
    
    // EQ curve presets
    enum class Preset : uint8_t {
        FLAT = 0,        // No EQ applied
        WARM,            // Warm analog sound
        BRIGHT,          // Enhanced highs
        PUNCHY,          // Enhanced lows and presence
        VOCAL,           // Optimized for vocals
        MASTER,          // Mastering curve
        VINTAGE,         // Vintage console emulation
        MODERN,          // Modern digital sound
        COUNT
    };
    
    struct BandSettings {
        FilterType type = FilterType::BELL;
        float frequency = 1000.0f;     // 20 Hz - 20 kHz
        float gain = 0.0f;             // -24 to +24 dB
        float q = 0.707f;              // 0.1 - 30.0 (bandwidth)
        bool enabled = true;           // Band enabled
        bool solo = false;             // Solo this band
        
        BandSettings() = default;
        BandSettings(FilterType t, float f, float g = 0.0f, float quality = 0.707f)
            : type(t), frequency(f), gain(g), q(quality) {}
    };
    
    struct EQSettings {
        std::array<BandSettings, static_cast<size_t>(Band::COUNT)> bands;
        
        float inputGain = 0.0f;        // -24 to +24 dB pre-gain
        float outputGain = 0.0f;       // -24 to +24 dB post-gain
        bool autoGain = true;          // Auto-compensate for EQ changes
        bool enabled = true;           // Master bypass
        
        // Analysis settings
        bool spectrumEnabled = true;   // Enable spectrum analyzer
        float analysisSmooth = 0.8f;   // Spectrum smoothing
        
        EQSettings() {
            // Initialize default frequencies for each band
            bands[static_cast<size_t>(Band::SUB)] = 
                BandSettings(FilterType::HIGH_PASS, 40.0f, 0.0f, 0.707f);
            bands[static_cast<size_t>(Band::LOW)] = 
                BandSettings(FilterType::LOW_SHELF, 120.0f, 0.0f, 0.707f);
            bands[static_cast<size_t>(Band::LOW_MID)] = 
                BandSettings(FilterType::BELL, 400.0f, 0.0f, 1.0f);
            bands[static_cast<size_t>(Band::MID)] = 
                BandSettings(FilterType::BELL, 1200.0f, 0.0f, 1.0f);
            bands[static_cast<size_t>(Band::HIGH_MID)] = 
                BandSettings(FilterType::BELL, 3500.0f, 0.0f, 1.0f);
            bands[static_cast<size_t>(Band::HIGH)] = 
                BandSettings(FilterType::HIGH_SHELF, 10000.0f, 0.0f, 0.707f);
            bands[static_cast<size_t>(Band::AIR)] = 
                BandSettings(FilterType::HIGH_SHELF, 18000.0f, 0.0f, 0.707f);
        }
    };
    
    MasterEQ();
    ~MasterEQ() = default;
    
    // Configuration
    void setSettings(const EQSettings& settings);
    const EQSettings& getSettings() const { return settings_; }
    void setSampleRate(float sampleRate);
    void setEnabled(bool enabled);
    
    // Band control
    void setBandSettings(Band band, const BandSettings& settings);
    void setBandGain(Band band, float gainDb);
    void setBandFrequency(Band band, float freqHz);
    void setBandQ(Band band, float q);
    void setBandType(Band band, FilterType type);
    void setBandEnabled(Band band, bool enabled);
    void setBandSolo(Band band, bool solo);
    
    // Presets
    void loadPreset(Preset preset);
    void saveCurrentAsPreset(int slot, const char* name);
    bool loadUserPreset(int slot);
    
    // Processing
    float process(float input);              // Single sample
    void processBlock(const float* input, float* output, int blockSize);
    void processStereo(const float* inputL, const float* inputR,
                      float* outputL, float* outputR, int blockSize);
    
    // Analysis
    struct SpectrumData {
        static constexpr int FFT_SIZE = 512;
        static constexpr int NUM_BINS = FFT_SIZE / 2;
        
        std::array<float, NUM_BINS> magnitude;
        std::array<float, NUM_BINS> phase;
        std::array<float, NUM_BINS> frequency;
        float peakFrequency;
        float rmsLevel;
        bool dataReady;
        
        SpectrumData() : peakFrequency(0.0f), rmsLevel(0.0f), dataReady(false) {}
    };
    
    const SpectrumData& getSpectrumData() const { return spectrumData_; }
    void enableSpectrum(bool enabled);
    void updateSpectrum(); // Call at display rate (30-60 Hz)
    
    // Frequency response
    float getFrequencyResponse(float frequency) const;
    void getFrequencyResponse(const float* frequencies, float* response, int count) const;
    
    // Utility
    bool isActive() const { return settings_.enabled && hasActiveEQ(); }
    float getInputLevel() const { return inputLevel_; }
    float getOutputLevel() const { return outputLevel_; }
    float getGainReduction() const { return gainReduction_; }
    void reset();
    
private:
    EQSettings settings_;
    float sampleRate_;
    
    // Biquad filter coefficients per band
    struct BiquadCoeffs {
        float b0, b1, b2; // Numerator coefficients
        float a1, a2;     // Denominator coefficients (a0 = 1)
        
        BiquadCoeffs() : b0(1.0f), b1(0.0f), b2(0.0f), a1(0.0f), a2(0.0f) {}
    };
    
    struct BiquadState {
        float x1 = 0.0f, x2 = 0.0f; // Input delay line
        float y1 = 0.0f, y2 = 0.0f; // Output delay line
        
        void reset() {
            x1 = x2 = y1 = y2 = 0.0f;
        }
    };
    
    std::array<BiquadCoeffs, static_cast<size_t>(Band::COUNT)> coeffs_;
    std::array<BiquadState, static_cast<size_t>(Band::COUNT)> states_;
    
    // Gain smoothing
    DSP::SmoothParam inputGainSmooth_, outputGainSmooth_;
    std::array<DSP::SmoothParam, static_cast<size_t>(Band::COUNT)> bandGainSmooth_;
    
    // Level monitoring
    float inputLevel_, outputLevel_, gainReduction_;
    DSP::Audio::PeakFollower inputPeak_, outputPeak_;
    
    // Auto-gain compensation
    float autoGainCompensation_;
    void updateAutoGain();
    
    // Spectrum analysis
    SpectrumData spectrumData_;
    std::array<float, SpectrumData::FFT_SIZE> fftInputBuffer_;
    std::array<float, SpectrumData::FFT_SIZE> fftWindow_;
    int fftBufferIndex_;
    bool spectrumNeedsUpdate_;
    
    // Filter coefficient calculation
    void updateCoefficients(Band band);
    void calculateBellFilter(Band band, float freq, float gain, float q);
    void calculateShelfFilter(Band band, float freq, float gain, float q, bool highShelf);
    void calculateHighPassFilter(Band band, float freq, float q);
    void calculateLowPassFilter(Band band, float freq, float q);
    void calculateNotchFilter(Band band, float freq, float q);
    
    // Processing helpers
    inline float processBand(Band band, float input) {
        const auto& c = coeffs_[static_cast<size_t>(band)];
        auto& s = states_[static_cast<size_t>(band)];
        
        // Biquad filter implementation
        float output = c.b0 * input + c.b1 * s.x1 + c.b2 * s.x2 - c.a1 * s.y1 - c.a2 * s.y2;
        
        // Update delay lines
        s.x2 = s.x1;
        s.x1 = input;
        s.y2 = s.y1;
        s.y1 = output;
        
        return output;
    }
    
    bool hasActiveEQ() const;
    void initializeFFTWindow();
    void performFFTAnalysis();
    
    // Preset data
    struct PresetData {
        EQSettings settings;
        char name[32];
        bool used;
        
        PresetData() : used(false) {
            std::memset(name, 0, sizeof(name));
        }
    };
    
    std::array<PresetData, 16> userPresets_;
    
    // Utility functions
    static float dbToLinear(float db) {
        return std::pow(10.0f, db / 20.0f);
    }
    
    static float linearToDb(float linear) {
        return 20.0f * std::log10(std::max(linear, 1e-10f));
    }
    
    static float frequencyToMel(float freq) {
        return 2595.0f * std::log10(1.0f + freq / 700.0f);
    }
};

/**
 * Factory function for easy creation
 */
inline std::unique_ptr<MasterEQ> createMasterEQ() {
    return std::make_unique<MasterEQ>();
}

// Utility functions for UI display
const char* getBandName(MasterEQ::Band band);
const char* getFilterTypeName(MasterEQ::FilterType type);
const char* getPresetName(MasterEQ::Preset preset);
float getBandCenterFrequency(MasterEQ::Band band);
std::pair<float, float> getBandFrequencyRange(MasterEQ::Band band);

} // namespace EtherSynth