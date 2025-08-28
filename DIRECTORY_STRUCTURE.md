# EtherSynth Directory Structure

## Overview
This document describes the reorganized directory structure of EtherSynth V1.0, implemented during the architectural cleanup phase.

## New Directory Organization

### Core System (`src/`)
```
src/
├── core/                          # Core system components
│   ├── EtherSynth.h/.cpp         # Main synthesizer class
│   ├── ParameterSystem.h/.cpp     # Unified parameter management
│   ├── ParameterSystemAdapter.h/.cpp # Backward compatibility layer
│   ├── ParameterSystemJSON.cpp   # JSON serialization
│   ├── PresetManager.h/.cpp       # Preset management
│   ├── SmartKnobSystem.h         # Smart knob interface
│   └── Types.h                    # Global type definitions
│
├── audio/                         # Audio processing components
│   ├── AudioEngine.h/.cpp         # Main audio engine
│   ├── AdvancedParameterSmoother.h/.cpp # Parameter smoothing
│   ├── DCBlocker.h/.cpp          # DC blocking filter
│   ├── VoiceManager.h/.cpp       # Voice allocation
│   └── ...                       # Other audio processors
│
├── engines/                       # Synthesis engines (consolidated)
│   ├── Classic4OpFM.h/.cpp       # 4-operator FM engine
│   ├── ElementsVoice.h/.cpp      # Physical modeling engine
│   ├── MacroVA.h/.cpp           # Virtual analog engine
│   ├── MacroFM.h/.cpp           # Macro FM engine
│   ├── MacroWavetable.h/.cpp    # Wavetable engine
│   ├── SynthEngineBase.h        # Base engine interface
│   └── ...                      # Other engines
│
├── control/                      # Control and modulation systems
│   ├── modulation/               # Modulation systems
│   │   ├── VelocityParameterScaling.h/.cpp
│   │   ├── ModulationMatrix.h/.cpp
│   │   ├── AdvancedModulationMatrix.h/.cpp
│   │   └── ...
│   └── velocity/                 # Velocity-specific controls
│       ├── VelocityCaptureSystem.h/.cpp
│       ├── VelocityLatchSystem.h/.cpp
│       ├── EngineVelocityMapping.h/.cpp
│       └── ...
│
├── processing/                   # Signal processing
│   └── effects/                  # Audio effects
│       ├── EffectsChain.h/.cpp
│       ├── TapeEffectsProcessor.h/.cpp
│       ├── ReverbEffect.h
│       └── ...
│
├── interface/                    # User interface
│   └── ui/                      # UI components
│       ├── MacroHUD.h/.cpp
│       ├── VelocityModulationUI.h
│       ├── TapeSquashingUI.h/.cpp
│       └── ...
│
├── platform/                    # Platform-specific code
│   └── hardware/                # Hardware interfaces
│       ├── HardwareInterface.h/.cpp
│       ├── MacHardware.h/.cpp
│       ├── SmartKnob.h/.cpp
│       └── ...
│
├── storage/                     # Data storage systems
│   ├── presets/                # Preset management
│   │   └── EnginePresetLibrary.h/.cpp
│   └── sampling/               # Sample management
│       ├── MultiSampleTrack.h/.cpp
│       ├── AutoSampleLoader.h/.cpp
│       └── ...
│
├── sequencer/                   # Sequencing and timing
│   ├── Timeline.h/.cpp
│   ├── SequencerPattern.h/.cpp
│   ├── EuclideanRhythm.h/.cpp
│   └── ...
│
├── midi/                       # MIDI processing
│   └── MIDIManager.h/.cpp
│
├── instruments/                # Instrument definitions
│   └── InstrumentSlot.h/.cpp
│
├── synthesis/                  # Additional synthesis components
│   ├── SynthEngine.h/.cpp
│   ├── BaseEngine.h/.cpp
│   └── ...
│
└── tests/                     # Unit tests
    └── EtherSynthTestSuite.h
```

## Migration Summary

### Consolidated Directories
- **`src/engines/` + `src/synthesis/engines/`** → **`src/engines/`**
  - Merged duplicate engine systems
  - All synthesis engines now in one location
  - Removed duplicate header/implementation separation

### Renamed Directories
- **`src/modulation/` + `src/velocity/`** → **`src/control/modulation/` + `src/control/velocity/`**
  - Logical grouping of control systems
  - Clear separation of general vs. velocity-specific modulation

- **`src/effects/`** → **`src/processing/effects/`**
  - Groups all signal processing under `processing/`
  - Allows for future expansion (e.g., `processing/filters/`)

- **`src/ui/`** → **`src/interface/ui/`**
  - Clearer interface designation
  - Allows for future non-UI interfaces

- **`src/hardware/`** → **`src/platform/hardware/`**
  - Platform-specific code grouping
  - Allows for OS-specific platform code

- **`src/sampler/`** → **`src/storage/sampling/`**
  - Groups all data storage systems
  - Logical pairing with `storage/presets/`

- **`src/presets/`** → **`src/storage/presets/`**
  - Consistent with sampling storage
  - Clear data management grouping

## Benefits of New Structure

### 1. **Logical Grouping**
- Related functionality is grouped together
- Clear separation of concerns
- Easier to find relevant code

### 2. **Reduced Duplication**
- Eliminated `src/engines/` vs `src/synthesis/engines/` confusion
- Single source of truth for each component type
- No more scattered modulation code

### 3. **Scalability**
- Room for expansion within each category
- Clear place for new components
- Hierarchical organization supports growth

### 4. **Maintainability**
- Easier code navigation
- Clear dependency relationships
- Simplified build system

### 5. **Professional Structure**
- Industry-standard organization patterns
- Clear separation of layers
- Documentation-friendly layout

## Include Path Updates

All `#include` statements have been systematically updated:

### Old Paths → New Paths
```cpp
// Old
#include "modulation/VelocityParameterScaling.h"
#include "velocity/VelocityLatchSystem.h"
#include "ui/MacroHUD.h"
#include "effects/EffectsChain.h"

// New
#include "control/modulation/VelocityParameterScaling.h"
#include "control/velocity/VelocityLatchSystem.h"
#include "interface/ui/MacroHUD.h"
#include "processing/effects/EffectsChain.h"
```

### Relative Path Examples
```cpp
// From src/core/
#include "../control/modulation/VelocityParameterScaling.h"
#include "../interface/ui/VelocityModulationUI.h"

// From src/control/modulation/
#include "../../interface/ui/VelocityModulationUI.h"
#include "../velocity/VelocityLatchSystem.h"
```

## Verification

### Compilation Tested ✅
- Core parameter system compiles successfully
- Velocity modulation system compiles
- Adapter system compiles
- Test files compile with updated paths

### Functionality Tested ✅
- Parameter system core functionality verified
- JSON serialization working
- Thread-safe atomic operations confirmed
- Backward compatibility maintained

## Migration Commands

The reorganization was performed using these commands:

```bash
# Create new directory structure
mkdir -p src/control/{modulation,velocity}
mkdir -p src/processing/effects
mkdir -p src/interface/ui
mkdir -p src/platform/hardware
mkdir -p src/storage/{sampling,presets}

# Move files to new locations
mv src/modulation/* src/control/modulation/
mv src/velocity/* src/control/velocity/
mv src/effects/* src/processing/effects/
mv src/ui/* src/interface/ui/
mv src/hardware/* src/platform/hardware/
mv src/sampler/* src/storage/sampling/
mv src/presets/* src/storage/presets/

# Consolidate engines
mv src/synthesis/engines/* src/engines/

# Update all include paths
./update_includes.sh
```

## Next Steps

With the directory structure reorganization complete, the next phase focuses on:

1. **Day 3: Naming Standardization** - Consistent class and file naming
2. **Memory Management Audit** - RAII patterns and smart pointers
3. **Header Dependencies Cleanup** - Forward declarations and compilation optimization

The new directory structure provides a solid foundation for these improvements and future development.

---

*This directory structure is part of the EtherSynth V1.0 release preparation.*