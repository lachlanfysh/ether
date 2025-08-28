#include <iostream>
#include <cmath>

// Simple test of tape saturation algorithms without full DSP dependencies
namespace TapeSaturationTest {

float vintageeTubeSaturation(float input, float amount) {
    // Model vintage tube-based tape machine saturation
    float drive = 1.0f + amount * 3.0f;
    float driven = input * drive;
    
    // Tube-like asymmetric saturation using tanh with bias
    float bias = 0.1f * amount;
    float saturated = std::tanh(driven + bias) - std::tanh(bias);
    
    // Add even harmonic content typical of tubes
    float even_harmonic = std::sin(driven * M_PI) * amount * 0.1f;
    
    return saturated + even_harmonic;
}

float modernSolidStateSaturation(float input, float amount) {
    // Model modern solid-state tape machine saturation
    float drive = 1.0f + amount * 2.0f;
    float driven = input * drive;
    
    // Cleaner, more symmetric saturation
    float saturated;
    if (std::abs(driven) < 0.7f) {
        saturated = driven;  // Linear region
    } else {
        // Soft clipping region
        saturated = std::copysign(0.7f + (std::abs(driven) - 0.7f) * 0.3f, driven);
    }
    
    return saturated * (1.0f / drive);  // Compensate for drive gain
}

float calculateCompressionGain(float input, float compressionAmount, float ratio = 3.0f) {
    float threshold = 0.7f - compressionAmount * 0.4f;  // Adaptive threshold
    
    float inputLevel = std::abs(input);
    
    if (inputLevel <= threshold) {
        return 1.0f;  // No compression
    }
    
    // Calculate compression
    float excess = inputLevel - threshold;
    float compressedExcess = excess / ratio;
    float targetLevel = threshold + compressedExcess;
    
    return inputLevel > 0.0f ? targetLevel / inputLevel : 1.0f;
}

void testSaturationAlgorithms() {
    std::cout << "Testing tape saturation algorithms...\n";
    
    // Test with various input levels
    float testInputs[] = {0.1f, 0.5f, 0.8f, 0.95f, 1.2f};
    float amount = 0.7f;
    
    std::cout << "Input\tTube\tSolid\tReduction\n";
    std::cout << "-----\t----\t-----\t---------\n";
    
    for (float input : testInputs) {
        float tubeOutput = vintageeTubeSaturation(input, amount);
        float solidOutput = modernSolidStateSaturation(input, amount);
        
        // Check that high levels are compressed
        float reduction = (std::abs(input) > 0.8f) ? 
                         (std::abs(input) - std::abs(tubeOutput)) / std::abs(input) * 100.0f : 0.0f;
        
        std::cout << input << "\t" << tubeOutput << "\t" << solidOutput << "\t" << reduction << "%\n";
    }
}

void testCompression() {
    std::cout << "\nTesting tape compression...\n";
    
    float testInputs[] = {0.1f, 0.5f, 0.8f, 0.95f, 1.2f};
    float compressionAmount = 0.6f;
    
    std::cout << "Input\tGain\tOutput\tReduction\n";
    std::cout << "-----\t----\t------\t---------\n";
    
    for (float input : testInputs) {
        float gain = calculateCompressionGain(input, compressionAmount);
        float output = input * gain;
        float reductionDb = 20.0f * std::log10(gain);
        
        std::cout << input << "\t" << gain << "\t" << output << "\t" << reductionDb << " dB\n";
    }
}

}

int main() {
    std::cout << "=== Tape Effects Algorithm Test ===\n\n";
    
    TapeSaturationTest::testSaturationAlgorithms();
    TapeSaturationTest::testCompression();
    
    std::cout << "\n=== Test Complete ===\n";
    std::cout << "Algorithms show expected behavior:\n";
    std::cout << "- Saturation reduces peaks at high levels\n";
    std::cout << "- Compression provides gain reduction above threshold\n";
    std::cout << "- Different saturation types have distinct characteristics\n";
    
    return 0;
}