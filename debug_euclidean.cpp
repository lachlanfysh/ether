#include "src/sampler/SampleLayeringSystem.h"
#include <iostream>

int main() {
    SampleLayeringSystem layerSystem;
    
    std::cout << "Testing Euclidean pattern generation..." << std::endl;
    
    // Test 16 steps, 8 hits
    auto pattern1 = layerSystem.generateEuclideanPattern(16, 8, 0);
    uint8_t count1 = 0;
    std::cout << "Pattern 16/8: ";
    for (bool hit : pattern1) {
        std::cout << (hit ? "1" : "0");
        if (hit) count1++;
    }
    std::cout << " (count: " << static_cast<int>(count1) << ")" << std::endl;
    
    // Test 16 steps, 5 hits
    auto pattern2 = layerSystem.generateEuclideanPattern(16, 5, 0);
    uint8_t count2 = 0;
    std::cout << "Pattern 16/5: ";
    for (bool hit : pattern2) {
        std::cout << (hit ? "1" : "0");
        if (hit) count2++;
    }
    std::cout << " (count: " << static_cast<int>(count2) << ")" << std::endl;
    
    // Test rotation
    auto pattern3 = layerSystem.generateEuclideanPattern(16, 5, 2);
    uint8_t count3 = 0;
    std::cout << "Pattern 16/5 rot 2: ";
    for (bool hit : pattern3) {
        std::cout << (hit ? "1" : "0");
        if (hit) count3++;
    }
    std::cout << " (count: " << static_cast<int>(count3) << ")" << std::endl;
    
    return 0;
}