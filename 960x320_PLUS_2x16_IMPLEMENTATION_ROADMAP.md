# 960×320 + 2×16 Keys Implementation Roadmap

**Target**: Professional groovebox with 960×320 screen + 32 velocity-sensitive keys  
**Timeline**: 4-6 days of focused work  
**Current Status**: Timeline system 95% complete, hardware spec finalized  
**Date**: August 28, 2025

## 🎯 **Vision Summary**

**Hardware Layout:**
```
[SmartKnob] [Enc1] [Enc2] [Enc3]
[PLAY] [REC] [WRITE] [SHIFT]

[INST][SOUND][PATT][CLONE][COPY][CUT][DEL][NUDGE][ACC][RATCH][TIE][MICRO][FX][QNT][SWG][SCENE]
[1]   [2]   [3]   [4]   [5]   [6]   [7]   [8]   [9]   [10] [11] [12] [13] [14] [15]  [16]

                    [960×320 Touch Screen]
```

**Software Features:**
- **Top 16 keys**: Option bank (pattern/instrument/tools/stamps/performance)
- **Bottom 16 keys**: Step sequencer (always live) + parameter control (when INST held)
- **14 professional engines**: MacroVA, MacroFM, MacroWT, etc.
- **Advanced parameter system**: 16 parameters per engine via SmartKnob
- **Pattern-centric workflow**: Timeline view + step editing + live performance

---

## 📋 **Phase 1: SwiftUI Interface Redesign (Day 1-2)**

### **Day 1: Core Layout & Structure (8 hours)**

#### **Morning (4h): Layout Foundation**
- [ ] **Adapt existing SwiftUI app to new dimensions**
  - Update UI constants for 960×320 exact sizing
  - Implement header (transport + globals)
  - Create option line with 16 button grid
  - Add compact view area with mode switching
  - Design always-live step row
  - Build responsive footer
  - File: `EtherSynth/EtherSynth.swift` (modify existing)

#### **Afternoon (4h): Option Line Implementation**
- [ ] **Implement 16-button option system**
  - Quad A: INST, SOUND, PATTERN, CLONE
  - Quad B: COPY, CUT, DELETE, NUDGE (tools)
  - Quad C: ACCENT, RATCHET, TIE, MICRO (stamps)
  - Quad D: FX, QUANT, SWING, SCENE (performance)
  - Visual feedback with proper LED states
  - Tool/stamp state management

### **Day 2: Advanced Interactions (8 hours)**

#### **Morning (4h): Parameter Control System**
- [ ] **INST Hold + Parameter Selection**
  - Hold INST → bottom keys become parameter selectors
  - Visual overlay showing parameter names
  - SmartKnob integration for real-time control
  - Context switching: parameter mode ↔ step mode
  - Live audio feedback during parameter changes

#### **Afternoon (4h): Overlay System**
- [ ] **Non-modal overlays (don't steal step row)**
  - SOUND overlay: I1-I16 instrument selection
  - PATTERN overlay: A-P pattern selection + length
  - CLONE overlay: destination selection + options
  - FX overlay: professional effects + note-repeat
  - SCENE overlay: A-D scene management
  - INST overlay: comprehensive instrument setup

---

## 📋 **Phase 2: Engine Integration (Day 3)**

### **Day 3: Real Engine System (8 hours)**

#### **Morning (4h): Engine Parameter Mapping**
- [ ] **Standardize 16 parameters per engine**
  ```cpp
  struct EngineParameterLayout {
      std::array<ParameterID, 16> parameters;
      std::array<std::string, 16> displayNames;  
      std::array<std::pair<float, float>, 16> ranges;
  };
  ```
  - Map all 14 engines to 16 key parameters
  - Create engine-specific parameter layouts
  - Integrate with unified parameter system
  - File: `src/engines/EngineParameterLayouts.h/.cpp`

#### **Afternoon (4h): Real Engine Integration**
- [ ] **Replace mockup engines with real ones**
  - Update SwiftUI engine list with actual 14 engines
  - Connect to EnginePresetLibrary for real presets
  - Integrate engine switching with C++ bridge
  - Add proper engine colors from existing palette
  - Connect preset loading/saving system

---

## 📋 **Phase 3: Advanced Systems (Day 4)**

### **Day 4: Professional Features (8 hours)**

#### **Morning (4h): Effects & Processing**
- [ ] **Integrate professional effects chain**
  - Replace basic FX with TapeEffectsProcessor
  - Add comprehensive reverb system
  - Integrate delay with feedback/filtering
  - Connect LUFS normalization and limiter
  - Real-time effects processing with low latency
  - File: Update existing effects integration

#### **Afternoon (4h): Advanced Modulation**
- [ ] **LFO & Modulation System**
  - Integrate existing LFO system with UI
  - Multiple LFO destinations per engine
  - Visual LFO rate/waveform selection
  - Connect velocity parameter system
  - Real-time modulation feedback
  - Files: Connect to existing `src/control/modulation/`

---

## 📋 **Phase 4: Pattern & Performance (Day 5)**

### **Day 5: Pattern Management & Performance (8 hours)**

#### **Morning (4h): Advanced Pattern System**
- [ ] **Enhanced pattern operations**
  - Pattern chaining (A→B→B→C)
  - Pattern length switching (16→32→48→64)
  - Pattern quantization and humanization
  - Advanced copy/paste with timing preservation
  - Multi-pattern selection and operations
  - Integration with existing timeline backend

#### **Afternoon (4h): Performance Features**
- [ ] **Live performance enhancements**
  - Scene snapshots (mixer + macro states)
  - Real-time FX throws and latching
  - Note-repeat with multiple subdivisions
  - Swing and quantization per-pattern
  - Performance encoder mapping system
  - Integration with SmartKnob navigation

---

## 📋 **Phase 5: Hardware Integration & Polish (Day 6)**

### **Day 6: Hardware & Final Integration (8 hours)**

#### **Morning (4h): Hardware Communication**
- [ ] **Complete hardware integration**
  - 32-key velocity capture and LED control
  - SmartKnob parameter control with detents
  - Performance encoder mapping and feedback
  - Transport button integration
  - Real-time latency optimization (<7ms)
  - Hardware calibration and response curves

#### **Afternoon (4h): Final Polish & Testing**
- [ ] **System integration and optimization**
  - Full system stress testing
  - Memory management validation
  - Real-time performance optimization
  - Error handling and edge cases
  - User feedback polish (LEDs, visual responses)
  - Documentation and help system

---

## 📋 **Technical Architecture**

### **Key Files to Create/Modify:**

#### **SwiftUI Application:**
- `EtherSynth/EtherSynth.swift` - Main application (adapt existing)
- `EtherSynth/OptionLineView.swift` - 16-button option system (new)
- `EtherSynth/ParameterControlView.swift` - INST hold parameter system (new)
- `EtherSynth/OverlaySystem.swift` - Non-modal overlay management (new)

#### **Engine Integration:**
- `src/engines/EngineParameterLayouts.h/.cpp` - 16-param mapping per engine (new)
- `src/engines/EngineManager.cpp` - Enhanced engine management (modify existing)
- `src/synthesis/ParameterMapping.h/.cpp` - SmartKnob to parameter routing (new)

#### **Hardware Interface:**
- `src/platform/hardware/KeyMatrix32.h/.cpp` - 2×16 key management (new)
- `src/platform/hardware/SmartKnobInterface.h/.cpp` - Enhanced knob control (modify)
- `src/platform/hardware/LEDController.h/.cpp` - 32-key LED feedback (new)

#### **Pattern System:**
- Reuse existing timeline backend with enhanced UI
- `src/storage/patterns/PatternChaining.h/.cpp` - Advanced pattern operations (new)

### **Integration Points:**

#### **C++ Bridge Extensions:**
```cpp
// Parameter control
void ether_set_parameter_by_key(void* engine, int instrument, int keyIndex, float value);
float ether_get_parameter_by_key(void* engine, int instrument, int keyIndex);
const char* ether_get_parameter_name(int engineType, int keyIndex);

// Hardware interface  
void ether_set_key_led(int keyIndex, float r, float g, float b);
void ether_process_key_velocity(int keyIndex, float velocity, bool pressed);
void ether_set_smart_knob_parameter(void* engine, int parameterIndex);
```

---

## 🎯 **Success Metrics**

### **Core Functionality:**
- [ ] All 32 keys respond with proper velocity and LED feedback
- [ ] SmartKnob controls any of 16 parameters per engine seamlessly
- [ ] INST hold → parameter mode switching works flawlessly
- [ ] All 14 engines integrate with real presets and parameters
- [ ] Pattern timeline + step editing + live performance all functional

### **Performance Targets:**
- [ ] <7ms pad-to-sound latency
- [ ] <10ms LED feedback response
- [ ] Real-time parameter changes without audio dropouts
- [ ] Smooth 60fps UI with professional visual feedback
- [ ] Memory usage optimized for embedded hardware

### **User Experience:**
- [ ] Intuitive workflow: hold button → keys change function → immediate results
- [ ] Professional feel: responsive, predictable, immediate feedback
- [ ] Deep sound design: 16 parameters × 14 engines = 224 unique controls
- [ ] Pattern-centric composition workflow fully supported

---

## 🚨 **Risk Mitigation**

### **High-Risk Items:**
1. **32-key hardware integration** - Extensive testing needed
2. **Real-time parameter control** - Audio thread safety critical
3. **SmartKnob parameter mapping** - Smooth value changes essential
4. **Memory management** - Embedded hardware constraints

### **Contingency Plans:**
1. **Hardware issues**: Implement software-only testing mode
2. **Performance problems**: Profile and optimize critical paths
3. **Integration complexity**: Build in phases with working fallbacks
4. **Timeline overrun**: Prioritize core functionality over polish features

---

## 📈 **Development Approach**

### **Incremental Implementation:**
- **Phase 1**: Get basic layout working with mockup-level functionality
- **Phase 2**: Integrate one real engine completely as template
- **Phase 3**: Scale to all engines with systematic approach
- **Phase 4**: Add advanced features incrementally
- **Phase 5**: Hardware integration and polish

### **Testing Strategy:**
- **Unit tests**: Each component tested in isolation
- **Integration tests**: Full signal path validation
- **Hardware tests**: Real hardware response validation
- **User tests**: Workflow and ergonomics validation

---

**This roadmap transforms your 95%-complete timeline system into a professional groovebox with immediate hardware control and deep synthesis capabilities. The combination of pattern-centric workflow + parameter immediacy + professional engines creates something genuinely unique in the market.** 🚀

## 🎯 **Ready to Begin?**

**Recommendation**: Start with Phase 1 to get the visual system working, then systematically build out the depth. The foundation is solid - now we're adding the professional interface and hardware integration that makes it special.

**Current Assets**: 
- ✅ 95% complete timeline system
- ✅ Professional synthesis engines  
- ✅ Advanced parameter system
- ✅ Comprehensive effects chain
- ✅ Pattern management backend

**Missing Pieces**:
- 🔄 Hardware-optimized UI layout
- 🔄 32-key integration  
- 🔄 Parameter control system
- 🔄 Professional visual feedback

**The architecture is proven - now we're building the dream interface!** ✨