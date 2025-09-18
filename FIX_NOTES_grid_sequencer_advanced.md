
# Ether – Terminal Grid UI Parameter Fix Notes

These instructions patch `grid_sequencer_advanced.cpp` so keyboard-driven parameter changes behave correctly:

- **LPF** no longer jumps to max or tie to other params.
- **Resonance** increments/decrements in small, sane steps (no giant jumps).
- **HPF** sweeps smoothly (not just 0↔1).
- **Amp** and **Clip** move off 0 and adjust normally.
- FM-specific handling (TIMBRE/algorithm stepping) uses the **actual instrument engine type**, not the row index.

---

## 0) Make a backup

```bash
cp grid_sequencer_advanced.cpp grid_sequencer_advanced.cpp.bak
```

---

## 1) Patch `adjustParameter(...)` to use normalized 0..1 steps and correct FM detection

**Apply this unified diff (safe to paste into a patch tool).**  
If any hunk fails, see **Section 4** for fallback edits.

```diff
*** a/grid_sequencer_advanced.cpp
--- b/grid_sequencer_advanced.cpp
***************
*** 23891,23933 ****
  void adjustParameter(int pid, bool increment) {
      // Handle pseudo-parameters
      if (pid == PSEUDO_PARAM_OCTAVE) {
          if (increment) {
              octaveOffset = std::min(4, octaveOffset.load() + 1);
          } else {
              octaveOffset = std::max(-4, octaveOffset.load() - 1);
          }
          return;
      } else if (pid == PSEUDO_PARAM_PITCH) {
          if (increment) {
              pitchOffset = std::min(12.0f, pitchOffset.load() + 0.5f);
          } else {
              pitchOffset = std::max(-12.0f, pitchOffset.load() - 0.5f);
          }
          return;
      }

      // Regular engine parameters - get current value from engine
      int slot = rowToSlot[currentEngineRow];
      if (slot < 0) slot = 0;
      float currentValue = ether_get_instrument_parameter(etherEngine, slot, pid);

      // Special handling for FM algorithm (TIMBRE parameter)
-     bool fm = std::string(ether_get_engine_type_name(currentEngineRow)).find("FM") != std::string::npos;
+     int engType = ether_get_instrument_engine_type(etherEngine, slot);
+     const char* etn = ether_get_engine_type_name(engType);
+     bool fm = (etn && std::string(etn).find("FM") != std::string::npos);
      if (pid == static_cast<int>(ParameterID::TIMBRE) && fm) {
          int algo = (int)std::floor(currentValue * 8.0f + 1e-6);
          if (increment) {
              algo = std::min(7, algo + 1);
          } else {
              algo = std::max(0, algo - 1);
          }
          currentValue = (algo + 0.5f) / 8.0f;
      } else {
          // Parameter-specific increment/decrement values
-         float delta = 0.02f; // Default for most parameters
- 
-         // Special cases for problematic parameters
-         if (pid == static_cast<int>(ParameterID::FILTER_CUTOFF)) {
-             delta = 100.0f; // LPF Hz
-         } else if (pid == static_cast<int>(ParameterID::FILTER_RESONANCE)) {
-             delta = 0.01f; // Small resonance steps
-         } else if (pid == static_cast<int>(ParameterID::HPF)) {
-             delta = 50.0f; // HPF Hz
-         } else if (pid == static_cast<int>(ParameterID::AMPLITUDE) ||
-                    pid == static_cast<int>(ParameterID::CLIP)) {
-             delta = (currentValue < 2.0f) ? 0.01f : 0.1f; // Adaptive
-         }
+         // ALWAYS work in normalized 0..1 domain for UI steps; engine maps to Hz/Q/etc.
+         float delta = 0.02f; // default
+         if (pid == static_cast<int>(ParameterID::FILTER_CUTOFF) ||
+             pid == static_cast<int>(ParameterID::HPF)) {
+             // small smooth steps; SHIFT speeds up
+             delta = shiftHeld.load() ? 0.05f : 0.01f;
+         } else if (pid == static_cast<int>(ParameterID::FILTER_RESONANCE)) {
+             delta = 0.01f;
+         } else if (pid == static_cast<int>(ParameterID::AMPLITUDE) ||
+                    pid == static_cast<int>(ParameterID::CLIP)) {
+             delta = 0.02f;
+         }

          if (increment) {
              currentValue += delta;
          } else {
              currentValue -= delta;
-             // Apply minimum bounds
-             if (pid == static_cast<int>(ParameterID::FILTER_CUTOFF)) {
-                 currentValue = std::max(20.0f, currentValue); // LPF minimum 20 Hz
-             } else {
-                 currentValue = std::max(0.0f, currentValue); // General minimum
-             }
          }
+         // Clamp to normalized range expected by engine
+         currentValue = std::clamp(currentValue, 0.0f, 1.0f);
      }

      // Set the new value directly to the engine
      printf("DEBUG: Setting pid=%d to value=%.3f\n", pid, currentValue);
      ether_set_instrument_parameter(etherEngine, slot, pid, currentValue);

      // Verify the value was set
      float verifyValue = ether_get_instrument_parameter(etherEngine, slot, pid);
      printf("DEBUG: Verified value=%.3f\n", verifyValue);
+     // Keep local cache coherent for UI reads
+     engineParameters[currentEngineRow][pid] = verifyValue;
  }
```

**What this patch does**

- Removes giant **Hz-sized** steps (e.g., `+100`/`+50`) and uses normalized 0..1 deltas.
- Adds clamping to `[0,1]`.
- Correctly detects FM **by instrument engine type**, not by row index.

---

## 2) Patch engine-type lookups (shared helper + drum detection + UI header title)

Add a helper near your other utilities (top-level, not inside a function):

```diff
+ // Helper: current row's instrument engine type name
+ static inline const char* currentInstrumentTypeName() {
+     int slot = rowToSlot[currentEngineRow];
+     if (slot < 0) slot = 0;
+     int t = ether_get_instrument_engine_type(etherEngine, slot);
+     return ether_get_engine_type_name(t);
+ }
```

Replace `isCurrentEngineDrum()` and `isEngineDrum(int row)` with slot-aware versions:

```diff
*** a/grid_sequencer_advanced.cpp
--- b/grid_sequencer_advanced.cpp
***************
*** 41234,41255 ****
- bool isCurrentEngineDrum() {
-     const char* name = ether_get_engine_type_name(currentEngineRow);
-     if (!name) return false;
-     std::string n(name);
-     for (auto &c : n) c = std::tolower(c);
-     return n.find("drum") != std::string::npos;
- }
- 
- bool isEngineDrum(int row) {
-     const char* name = ether_get_engine_type_name(row);
-     if (!name) return false;
-     std::string n(name);
-     for (auto &c : n) c = std::tolower(c);
-     return n.find("drum") != std::string::npos;
- }
+ bool isCurrentEngineDrum() {
+     const char* name = currentInstrumentTypeName();
+     if (!name) return false;
+     std::string n(name);
+     for (auto &c : n) c = std::tolower(c);
+     return n.find("drum") != std::string::npos;
+ }
+ 
+ bool isEngineDrum(int row) {
+     int slot = rowToSlot[row];
+     if (slot < 0) slot = 0;
+     int t = ether_get_instrument_engine_type(etherEngine, slot);
+     const char* name = ether_get_engine_type_name(t);
+     if (!name) return false;
+     std::string n(name);
+     for (auto &c : n) c = std::tolower(c);
+     return n.find("drum") != std::string::npos;
+ }
```

Fix the engine name used in your header `drawFixedUI()` (one-liner swap):

```diff
*** a/grid_sequencer_advanced.cpp
--- b/grid_sequencer_advanced.cpp
***************
*** 26868,26874 ****
-     const char* techName = ether_get_engine_type_name(currentEngineRow);
+     const char* techName = currentInstrumentTypeName();
      const char* name = getDisplayName(techName);
```

Also **globally replace** any remaining occurrences of  
`ether_get_engine_type_name(currentEngineRow)` → `currentInstrumentTypeName()`.

---

## 3) Rebuild with extra warnings (catches int/float issues)

```bash
# Example; adapt to your build system
CXXFLAGS+=" -Wall -Wextra -Wconversion -Wimplicit-fallthrough -Wswitch-enum " make -j
```

---

## 4) If any diff hunk fails to apply – manual fallback edits

**4.1 FM detection inside `adjustParameter(...)`:**

Replace:
```cpp
bool fm = std::string(ether_get_engine_type_name(currentEngineRow)).find("FM") != std::string::npos;
```
with:
```cpp
int engType = ether_get_instrument_engine_type(etherEngine, slot);
const char* etn = ether_get_engine_type_name(engType);
bool fm = (etn && std::string(etn).find("FM") != std::string::npos);
```

**4.2 LPF/HPF/RES/AMP/CLIP step sizes + clamp:**  
Remove any `delta = 100.0f` / `50.0f` or Hz-based math. Replace the whole delta block with:

```cpp
float delta = 0.02f; // default
if (pid == (int)ParameterID::FILTER_CUTOFF || pid == (int)ParameterID::HPF) {
    delta = shiftHeld.load() ? 0.05f : 0.01f;
} else if (pid == (int)ParameterID::FILTER_RESONANCE) {
    delta = 0.01f;
} else if (pid == (int)ParameterID::AMPLITUDE || pid == (int)ParameterID::CLIP) {
    delta = 0.02f;
}
if (increment) currentValue += delta; else currentValue -= delta;
currentValue = std::clamp(currentValue, 0.0f, 1.0f);
```

**4.3 Cache coherence after set:**  
After calling `ether_set_instrument_parameter(...)`, ensure your UI cache is updated:

```cpp
engineParameters[currentEngineRow][pid] =
    ether_get_instrument_parameter(etherEngine, slot, pid);
```

**4.4 Drum checks & header:**  
Create `currentInstrumentTypeName()` (see Section 2) and replace any direct `ether_get_engine_type_name(currentEngineRow)` usage with it.

---

## 5) Quick runtime sanity checks

- **LPF**: arrow keys change smoothly by ~0.01 (hold **SHIFT** for ~0.05).
- **Resonance**: small ±0.01 steps, no huge jumps.
- **HPF**: smooth sweep (no 0↔1 toggling).
- **Amp & Clip**: move off 0 and adjust as expected.
- **Macro/FM engines**: TIMBRE/algorithm stepping still works, but LPF/others are no longer tied or misrouted.

---

### Notes

- These changes keep UI math in normalized **0..1**. If you want to **display** Hz/Q/dB, do that only in the formatting layer (don’t change the stored value domain).
- You can extend the “fast step” behavior to other params by checking `shiftHeld` in the same way.

Good luck! ✨
