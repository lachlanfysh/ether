#include "ModulationMatrix.h"
#include <iostream>

ModulationMatrix::ModulationMatrix() {
    initialized_ = true;
    std::cout << "ModulationMatrix created" << std::endl;
}

ModulationMatrix::~ModulationMatrix() {
    std::cout << "ModulationMatrix destroyed" << std::endl;
}

void ModulationMatrix::process() {
    // Stub implementation - just process modulation
}

void ModulationMatrix::setConnection(size_t index, bool enabled) {
    std::cout << "ModulationMatrix: Connection " << index << " " 
              << (enabled ? "enabled" : "disabled") << std::endl;
}

bool ModulationMatrix::getConnection(size_t index) const {
    return false; // Stub
}