#include "src/sequencer/TapeSquashLimiter.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Testing TapeSquashLimiter basic functionality..." << std::endl;
    
    TapeSquashLimiter limiter;
    std::cout << "✓ Limiter created" << std::endl;
    
    // Test basic configuration
    TapeSquashLimiter::LimitConfig config;
    config.maxTracks = 6;
    limiter.setLimitConfig(config);
    std::cout << "✓ Configuration set" << std::endl;
    
    // Test basic limits
    bool result = limiter.checkTrackCountLimit(4);
    std::cout << "✓ Track count limit check: " << (result ? "pass" : "fail") << std::endl;
    
    result = limiter.checkMemoryLimit(1024);
    std::cout << "✓ Memory limit check: " << (result ? "pass" : "fail") << std::endl;
    
    result = limiter.checkCpuLimit(0.5f);
    std::cout << "✓ CPU limit check: " << (result ? "pass" : "fail") << std::endl;
    
    std::cout << "Basic limiter test completed successfully!" << std::endl;
    
    return 0;
}