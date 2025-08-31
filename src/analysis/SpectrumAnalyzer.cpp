#include "SpectrumAnalyzer.h"
#include <algorithm>
#include <numeric>
#include <chrono>
#include <cstring>

namespace EtherSynth {

SpectrumAnalyzer::SpectrumAnalyzer() 
    : realTimeEnabled_(true)
    , sampleRate_(SAMPLE_RATE)
    , processingLoad_(0.0f)
    , lastProcessTime_(0)
{
    // Initialize buffers
    inputBuffer_.resize(FFT_SIZE, 0.0f);
    windowBuffer_.resize(FFT_SIZE, 0.0f);
    fftBuffer_.resize(FFT_SIZE);
    magnitudeHistory_.resize(BINS * 10, 0.0f); // 10 frames of history
    
    // Initialize analysis data
    currentSpectrum_.magnitudes.fill(0.0f);
    currentSpectrum_.barkBands.fill(0.0f);
    currentSpectrum_.displayBars.fill(0.0f);
    smoothedSpectrum_ = currentSpectrum_;
    
    // Create bark band mapping
    barkBandStart_ = std::vector<int>(BARK_BANDS);
    barkBandEnd_ = std::vector<int>(BARK_BANDS);
    barkBandWeights_ = std::vector<float>(BARK_BANDS);
    
    // Initialize bark bands (0-24 bark scale mapped to 0-24kHz)
    for (int i = 0; i < BARK_BANDS; i++) {
        float bark = float(i);
        float startHz = barkToHz(bark);
        float endHz = barkToHz(bark + 1.0f);
        
        barkBandStart_[i] = getBinForFrequency(startHz);
        barkBandEnd_[i] = getBinForFrequency(endHz);
        barkBandWeights_[i] = 1.0f;
    }
    
    // Create window function
    windowFunction_ = createHannWindow(FFT_SIZE);
    windowCacheValid_ = true;
}

SpectrumAnalyzer::~SpectrumAnalyzer() {
}

void SpectrumAnalyzer::processAudioBuffer(const float* buffer, int bufferSize) {
    if (!buffer || bufferSize <= 0) return;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(analysisMutex_);
    
    // Copy input data to circular buffer
    int copySize = std::min(bufferSize, FFT_SIZE);
    
    // Shift existing data
    if (copySize < FFT_SIZE) {
        std::memmove(inputBuffer_.data(), 
                    inputBuffer_.data() + copySize, 
                    (FFT_SIZE - copySize) * sizeof(float));
    }
    
    // Copy new data
    std::memcpy(inputBuffer_.data() + (FFT_SIZE - copySize), 
               buffer, copySize * sizeof(float));
    
    // Apply window function
    windowBuffer_ = inputBuffer_;
    applyWindow(windowBuffer_);
    
    // Perform FFT
    performFFT(windowBuffer_, fftBuffer_);
    
    // Calculate magnitudes
    calculateMagnitudes(fftBuffer_);
    
    // Calculate spectral features
    calculateSpectralFeatures();
    
    // Calculate perceptual bands
    calculateBarkBands();
    calculateDisplayBars();
    
    // Update audio features
    updateAudioFeatures();
    
    // Update timestamp
    currentSpectrum_.timestamp = std::chrono::duration_cast<std::chrono::microseconds>
        (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    // Calculate processing load
    auto endTime = std::chrono::high_resolution_clock::now();
    float processTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    float bufferTime = (float(bufferSize) / sampleRate_) * 1000.0f;
    processingLoad_.store(processTime / bufferTime);
}

const SpectrumAnalyzer::SpectrumData& SpectrumAnalyzer::getLatestSpectrum() const {
    std::lock_guard<std::mutex> lock(analysisMutex_);
    return currentSpectrum_;
}

const SpectrumAnalyzer::AudioFeatures& SpectrumAnalyzer::getAudioFeatures() const {
    return currentFeatures_;
}

void SpectrumAnalyzer::performFFT(const std::vector<float>& input, 
                                 std::vector<std::complex<float>>& output) {
    // Simple DFT implementation for demonstration
    // In a real implementation, use FFTW or accelerated FFT
    
    const float two_pi = 2.0f * M_PI;
    const int N = FFT_SIZE;
    
    for (int k = 0; k < N; k++) {
        std::complex<float> sum(0.0f, 0.0f);
        
        for (int n = 0; n < N; n++) {
            float angle = -two_pi * k * n / N;
            std::complex<float> w(std::cos(angle), std::sin(angle));
            sum += input[n] * w;
        }
        
        output[k] = sum;
    }
}

void SpectrumAnalyzer::applyWindow(std::vector<float>& buffer) const {
    if (!windowCacheValid_ || windowFunction_.size() != buffer.size()) {
        return;
    }
    
    for (size_t i = 0; i < buffer.size(); i++) {
        buffer[i] *= windowFunction_[i];
    }
}

void SpectrumAnalyzer::calculateMagnitudes(const std::vector<std::complex<float>>& fftData) {
    const float scale = 2.0f / FFT_SIZE;
    
    for (int i = 0; i < BINS; i++) {
        float real = fftData[i].real();
        float imag = fftData[i].imag();
        float magnitude = std::sqrt(real * real + imag * imag) * scale;
        
        // Apply smoothing
        currentSpectrum_.magnitudes[i] = smoothingFactor_ * smoothedSpectrum_.magnitudes[i] +
                                       (1.0f - smoothingFactor_) * magnitude;
    }
    
    smoothedSpectrum_.magnitudes = currentSpectrum_.magnitudes;
}

void SpectrumAnalyzer::calculateSpectralFeatures() {
    // Total energy
    currentSpectrum_.totalEnergy = 0.0f;
    for (float mag : currentSpectrum_.magnitudes) {
        currentSpectrum_.totalEnergy += mag * mag;
    }
    
    if (currentSpectrum_.totalEnergy < 1e-10f) {
        currentSpectrum_.hasActivity = false;
        return;
    }
    
    currentSpectrum_.hasActivity = true;
    
    // Spectral centroid (brightness)
    float weightedSum = 0.0f;
    float magnitudeSum = 0.0f;
    
    for (int i = 0; i < BINS; i++) {
        float frequency = getFrequencyForBin(i);
        float magnitude = currentSpectrum_.magnitudes[i];
        
        weightedSum += frequency * magnitude;
        magnitudeSum += magnitude;
    }
    
    currentSpectrum_.spectralCentroid = (magnitudeSum > 0.0f) ? 
        weightedSum / magnitudeSum : 0.0f;
    
    // Spectral spread
    float variance = 0.0f;
    for (int i = 0; i < BINS; i++) {
        float frequency = getFrequencyForBin(i);
        float magnitude = currentSpectrum_.magnitudes[i];
        float deviation = frequency - currentSpectrum_.spectralCentroid;
        
        variance += deviation * deviation * magnitude;
    }
    
    currentSpectrum_.spectralSpread = (magnitudeSum > 0.0f) ?
        std::sqrt(variance / magnitudeSum) : 0.0f;
    
    // Spectral rolloff (95% energy point)
    float energyThreshold = 0.95f * currentSpectrum_.totalEnergy;
    float cumulativeEnergy = 0.0f;
    
    for (int i = 0; i < BINS; i++) {
        cumulativeEnergy += currentSpectrum_.magnitudes[i] * currentSpectrum_.magnitudes[i];
        if (cumulativeEnergy >= energyThreshold) {
            currentSpectrum_.spectralRolloff = getFrequencyForBin(i);
            break;
        }
    }
    
    // RMS and peak
    currentSpectrum_.rms = std::sqrt(currentSpectrum_.totalEnergy / BINS);
    currentSpectrum_.peak = *std::max_element(currentSpectrum_.magnitudes.begin(), 
                                            currentSpectrum_.magnitudes.end());
}

void SpectrumAnalyzer::calculateBarkBands() {
    currentSpectrum_.barkBands.fill(0.0f);
    
    for (int band = 0; band < BARK_BANDS; band++) {
        int startBin = barkBandStart_[band];
        int endBin = barkBandEnd_[band];
        
        float bandEnergy = 0.0f;
        int binCount = 0;
        
        for (int bin = startBin; bin < endBin && bin < BINS; bin++) {
            bandEnergy += currentSpectrum_.magnitudes[bin];
            binCount++;
        }
        
        if (binCount > 0) {
            currentSpectrum_.barkBands[band] = bandEnergy / binCount;
        }
    }
}

void SpectrumAnalyzer::calculateDisplayBars() {
    currentSpectrum_.displayBars.fill(0.0f);
    
    // Map frequency bins to display bars using logarithmic scale
    float logMin = std::log10(20.0f);      // 20 Hz
    float logMax = std::log10(20000.0f);   // 20 kHz
    float logRange = logMax - logMin;
    
    for (int bar = 0; bar < DISPLAY_BARS; bar++) {
        float logFreq = logMin + (float(bar) / DISPLAY_BARS) * logRange;
        float frequency = std::pow(10.0f, logFreq);
        
        int centerBin = getBinForFrequency(frequency);
        int startBin = std::max(0, centerBin - 2);
        int endBin = std::min(BINS - 1, centerBin + 2);
        
        float barEnergy = 0.0f;
        int binCount = 0;
        
        for (int bin = startBin; bin <= endBin; bin++) {
            barEnergy += currentSpectrum_.magnitudes[bin];
            binCount++;
        }
        
        if (binCount > 0) {
            currentSpectrum_.displayBars[bar] = barEnergy / binCount;
        }
    }
}

void SpectrumAnalyzer::updateAudioFeatures() {
    // Calculate band energies
    currentSpectrum_.bassEnergy = getBandEnergy(20.0f, 250.0f);
    currentSpectrum_.midEnergy = getBandEnergy(250.0f, 4000.0f);
    currentSpectrum_.highEnergy = getBandEnergy(4000.0f, 20000.0f);
    
    // Calculate ratios
    if (currentSpectrum_.midEnergy > 1e-10f) {
        currentSpectrum_.lowMidRatio = currentSpectrum_.bassEnergy / currentSpectrum_.midEnergy;
        currentSpectrum_.highMidRatio = currentSpectrum_.highEnergy / currentSpectrum_.midEnergy;
    }
    
    // Feature detection (simplified)
    currentFeatures_.hasKick = (currentSpectrum_.bassEnergy > 0.3f * currentSpectrum_.totalEnergy);
    currentFeatures_.hasSnare = (getBandEnergy(150.0f, 300.0f) > 0.2f * currentSpectrum_.totalEnergy &&
                                getBandEnergy(5000.0f, 8000.0f) > 0.1f * currentSpectrum_.totalEnergy);
    currentFeatures_.hasHiHat = (currentSpectrum_.highEnergy > 0.25f * currentSpectrum_.totalEnergy);
    currentFeatures_.hasBass = (currentSpectrum_.bassEnergy > 0.4f * currentSpectrum_.totalEnergy);
    
    // Musical characteristics
    currentFeatures_.isPercussive = (currentSpectrum_.spectralCentroid > 2000.0f) &&
                                   (currentSpectrum_.spectralSpread > 1000.0f);
    currentFeatures_.isMelodic = (currentSpectrum_.spectralCentroid < 3000.0f) &&
                                (currentSpectrum_.spectralSpread < 800.0f);
    
    // Harmonicity estimate
    currentFeatures_.harmonicity = calculateInharmonicity();
}

float SpectrumAnalyzer::getFrequencyForBin(int bin) const {
    return (float(bin) * sampleRate_) / (2.0f * FFT_SIZE);
}

int SpectrumAnalyzer::getBinForFrequency(float frequency) const {
    return int((frequency * 2.0f * FFT_SIZE) / sampleRate_);
}

float SpectrumAnalyzer::getBandEnergy(float lowFreq, float highFreq) const {
    int startBin = getBinForFrequency(lowFreq);
    int endBin = getBinForFrequency(highFreq);
    
    startBin = std::max(0, startBin);
    endBin = std::min(BINS - 1, endBin);
    
    float energy = 0.0f;
    for (int bin = startBin; bin <= endBin; bin++) {
        energy += currentSpectrum_.magnitudes[bin] * currentSpectrum_.magnitudes[bin];
    }
    
    return energy;
}

float SpectrumAnalyzer::calculateInharmonicity() const {
    // Simplified harmonicity calculation
    float fundamental = detectFundamental();
    if (fundamental < 50.0f) return 0.0f;
    
    float harmonicSum = 0.0f;
    float totalSum = 0.0f;
    
    for (int harmonic = 1; harmonic <= 8; harmonic++) {
        float harmonicFreq = fundamental * harmonic;
        if (harmonicFreq > sampleRate_ / 2.0f) break;
        
        float magnitude = getMagnitudeAtFrequency(harmonicFreq);
        harmonicSum += magnitude;
    }
    
    for (float mag : currentSpectrum_.magnitudes) {
        totalSum += mag;
    }
    
    return (totalSum > 0.0f) ? harmonicSum / totalSum : 0.0f;
}

float SpectrumAnalyzer::detectFundamental(float minFreq, float maxFreq) const {
    int startBin = getBinForFrequency(minFreq);
    int endBin = getBinForFrequency(maxFreq);
    
    startBin = std::max(0, startBin);
    endBin = std::min(BINS - 1, endBin);
    
    float maxMagnitude = 0.0f;
    int fundamentalBin = startBin;
    
    for (int bin = startBin; bin <= endBin; bin++) {
        if (currentSpectrum_.magnitudes[bin] > maxMagnitude) {
            maxMagnitude = currentSpectrum_.magnitudes[bin];
            fundamentalBin = bin;
        }
    }
    
    return getFrequencyForBin(fundamentalBin);
}

float SpectrumAnalyzer::getMagnitudeAtFrequency(float frequency) const {
    int bin = getBinForFrequency(frequency);
    if (bin < 0 || bin >= BINS) return 0.0f;
    
    return currentSpectrum_.magnitudes[bin];
}

float SpectrumAnalyzer::hzToBark(float hz) const {
    return 13.0f * std::atan(0.00076f * hz) + 3.5f * std::atan((hz / 7500.0f) * (hz / 7500.0f));
}

float SpectrumAnalyzer::barkToHz(float bark) const {
    // Approximate inverse (iterative solution would be more accurate)
    return 600.0f * std::sinh(bark / 4.0f);
}

std::vector<float> SpectrumAnalyzer::createHannWindow(int size) const {
    std::vector<float> window(size);
    for (int i = 0; i < size; i++) {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (size - 1)));
    }
    return window;
}

void SpectrumAnalyzer::renderSpectrum(uint32_t* displayBuffer, int width, int height,
                                    uint32_t barColor, uint32_t bgColor) const {
    if (!displayBuffer || width <= 0 || height <= 0) return;
    
    std::lock_guard<std::mutex> lock(analysisMutex_);
    
    // Clear background
    std::fill_n(displayBuffer, width * height, bgColor);
    
    // Draw spectrum bars
    int barWidth = width / DISPLAY_BARS;
    float maxMagnitude = *std::max_element(currentSpectrum_.displayBars.begin(),
                                         currentSpectrum_.displayBars.end());
    
    if (maxMagnitude < 1e-10f) return;
    
    for (int bar = 0; bar < DISPLAY_BARS && bar * barWidth < width; bar++) {
        float magnitude = currentSpectrum_.displayBars[bar];
        int barHeight = int((magnitude / maxMagnitude) * height * 0.9f);
        
        int x = bar * barWidth;
        int barPixelWidth = std::min(barWidth - 1, width - x);
        
        // Draw bar from bottom up
        for (int y = height - barHeight; y < height; y++) {
            for (int px = 0; px < barPixelWidth; px++) {
                int pixelIndex = y * width + x + px;
                if (pixelIndex >= 0 && pixelIndex < width * height) {
                    displayBuffer[pixelIndex] = barColor;
                }
            }
        }
    }
}

void SpectrumAnalyzer::setSampleRate(float sampleRate) {
    sampleRate_.store(sampleRate);
    
    // Recalculate bark band mapping
    for (int i = 0; i < BARK_BANDS; i++) {
        float bark = float(i);
        float startHz = barkToHz(bark);
        float endHz = barkToHz(bark + 1.0f);
        
        barkBandStart_[i] = getBinForFrequency(startHz);
        barkBandEnd_[i] = getBinForFrequency(endHz);
    }
}

float SpectrumAnalyzer::getProcessingLoad() const {
    return processingLoad_.load();
}

} // namespace EtherSynth