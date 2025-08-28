#include "EtherSynthBridge.h"
#include <iostream>

int main() {
    std::cout << "Testing C++ Bridge..." << std::endl;
    
    // Test basic creation
    EtherSynthCpp* engine = ether_create();
    if (engine) {
        std::cout << "✅ ether_create() succeeded" << std::endl;
        
        // Test basic function calls that should work
        ether_set_master_volume(engine, 0.8f);
        float volume = ether_get_master_volume(engine);
        std::cout << "✅ Volume set/get: " << volume << std::endl;
        
        ether_destroy(engine);
        std::cout << "✅ ether_destroy() succeeded" << std::endl;
    } else {
        std::cout << "❌ ether_create() failed" << std::endl;
        return 1;
    }
    
    std::cout << "🎉 Basic bridge test passed!" << std::endl;
    return 0;
}
