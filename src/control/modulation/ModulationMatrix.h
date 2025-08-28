#pragma once
#include "../core/Types.h"

/**
 * Modulation matrix system
 * Stub implementation for initial testing
 */
class ModulationMatrix {
public:
    ModulationMatrix();
    ~ModulationMatrix();
    
    // Processing
    void process();
    
    // Configuration
    void setConnection(size_t index, bool enabled);
    bool getConnection(size_t index) const;
    
private:
    bool initialized_ = false;
};