# Top‑Up Spec — Drops & Pitch‑Ramped Retriggers (v1.1)

**Scope**: Adds two performance features on top of **“2×16 Grid Layout — Option Bank + Step Row (Control Spec v1)”** without changing hardware:
- **Drop** (solo/mute punch‑in) that can be *latched* and *baked*.
- **Pitch‑Ramped Retrigger** (aka “zipper roll”) as an extension of Ratchet/Note‑Repeat.

> **Terminology**: “Zipper” isn’t a universal term in samplers; it can also mean *zipper noise* (parameter stepping artifacts). In this spec we’ll use **Pitch‑Ramped Retrigger** and allow the UI subtitle **“Zipper roll”** if desired.

---

## 1) UX Additions (no new buttons)

### 1.1 FX Overlay → **Drop** tile
- Add a 5th tile to the FX overlay: **Drop**.
- Tapping **Drop** opens a mini panel with:
  - **Mode**: *Solo Armed*, *Mute Armed*, *Kill All*, *Kick‑Only*, *Drums‑Only*, *User Mask…*
  - **Length** (quantized): *¼ • ½ • 1 • 2 bars*, plus **Hold** (momentary) and **Latch**.
  - **Optional sweep**: *Filter Sweep on/off* (simple LPF breakdown).
  - **Bake** button (writes automation events; confirm toast).
- **SHIFT+FX** while picking **Drop** = **Latch** (hands‑free). **Hold FX** = **Bake**.

### 1.2 RATCHET (hold) → **Retrigger Panel**
- Long‑press **RATCHET** (or open Step Editor when `ratchet>1`) to show the **Retrigger Panel**:
  - **Shape**: *Flat • Up • Down • Up‑Down • Random*
  - **Spread (semitones)**: `±0..±12`
  - **Curve**: *Linear • Exponential* (ramp law)
  - **Gain Decay**: `0..100%` per sub‑hit
  - **Retrig Type**: *Re‑Start* (reseek sample start), *Continue* (same playhead), *Granular* (optional)
  - **Start%**: `0..100`% offset for sub‑hits (Re‑Start/Granular)
  - **Time Anchor**: *Keep Step Length* (distribute inside step) or *Fixed Rate* (respects Repeat rate)
  - **Jitter**: `0..5 ms` random micro jitter

### 1.3 FX Overlay → **Note‑Repeat** quick ramp
- In the Note‑Repeat area add a compact control:
  - **Ramp**: *Flat/Up/Down*
  - **Spread**: `0..12 st`
- When **WRITE** is armed, live repeats are recorded as steps with `ratchet>1` and corresponding retrigger parameters.

---

## 2) Data Model Deltas

### 2.1 Mixer automation for **Drop**
```c
// Song timeline event (quantized to bar grid)
typedef struct {
  uint32_t start_tick;    // song ticks
  uint32_t end_tick;      // song ticks
  uint16_t mute_mask;     // 16-bit: 1=mute, 0=play (or invert for Solo mode)
  bool     is_solo;       // if true, interpret mask as solo mask (1=solo)
  bool     lp_sweep;      // optional master LPF sweep during drop
} MixerAutomation;
```
- **Conflict rule**: last writer wins (later event overrides during overlap).
- **Safety**: apply 1.5–3.0 ms gain ramps per channel to avoid clicks when muting/unmuting.

### 2.2 Per‑step **Retrigger** extension
```c
// Only valid when ratchet > 1
typedef enum { RT_FLAT, RT_UP, RT_DOWN, RT_UPDOWN, RT_RANDOM } RetrigShape;
typedef enum { RC_LIN, RC_EXP } RetrigCurve;
typedef enum { RTYPE_RESTART, RTYPE_CONTINUE, RTYPE_GRANULAR } RetrigType;
typedef enum { TA_KEEP_STEP, TA_FIXED_RATE } TimeAnchor;

typedef struct {
  uint8_t len_steps;      // 1..8
  int8_t  micro;          // -60..+60 ticks
  uint8_t ratchet;        // 1,2,3,4 (n sub‑hits)
  bool    tie, accent;
  // Retrigger
  RetrigShape shape;      // Flat/Up/Down/UpDown/Random
  int8_t  spread_semi;    // -12..+12
  RetrigCurve curve;      // Lin/Exp (γ≈1.6)
  uint8_t gain_decay;     // 0..100 (% per sub‑hit)
  RetrigType type;        // Restart/Continue/Granular
  uint8_t start_pct;      // 0..100
  TimeAnchor anchor;      // Keep Step / Fixed Rate
  uint8_t jitter_ms;      // 0..5
} StepMeta;
```
- **Backward compatibility**: if `ratchet==1`, ignore retrigger fields on load.

---

## 3) Engine Behavior

### 3.1 Drop execution
- On trigger, compute **channel states** from `mute_mask` and `is_solo`.
- Crossfade per channel over 2 ms (linear) into mute/unmute.
- If `lp_sweep==true`, schedule a master LPF automation: `f_c: 18 kHz → 300 Hz` over the Drop window with `Q≈0.7` (exp curve). On release, sweep back to 18 kHz over 60–120 ms.

### 3.2 Retrigger expansion (per active step)
```
for k in 0..n-1 (n = ratchet):
  frac   = (n==1) ? 0 : k/(n-1)
  shaped = curve==EXP ? pow(frac, 1.6) : frac
  semis  = shape==FLAT?0 : shape==UP? +spread*shaped : shape==DOWN? -spread*shaped :
           shape==UPDOWN? spread*(2*abs(frac-0.5)) : rand(-spread, +spread)
  ratio  = 2^(semis/12)
  t_k    = base_t + k * Δt(anchor, n, step_len) + jitter()
  gain_k = base * (1 - decay)^k
  if type==RESTART:  seek sample to start + start_pct%
  if type==CONTINUE: keep playhead, retrigger amp env only
  if type==GRANULAR: schedule short grains (20 ms, 50% overlap, Hann)
  schedule voice with pitch_ratio=ratio, gain=gain_k, len=step_len/n
```
- **Interpolation**: 4‑point Lagrange (good compromise on H7). Precompute `2^(x/12)` LUT for x=−12..+12 in 0.1‑semi steps.
- **Polyphony guard**: global cap (e.g., 24 voices), per‑slot cap (e.g., 8). On overflow drop quietest retrig voice.

---

## 4) Defaults & Presets
- **Retrigger defaults** (sound like HAAi‑style zips out of the box):
  - `ratchet=3`, `shape=UP`, `spread=+7`, `curve=EXP`, `gain_decay=15%`, `type=RESTART`, `start_pct=0`, `anchor=KEEP_STEP`, `jitter=1 ms`.
- **Drop defaults**: Mode=*Solo Armed*, Length=*1 bar*, Sweep=*off*.
- **User Mask presets**: store up to 4 masks under **Scene** memory (A–D) so Scenes can recall both mixer state and mask preference.

---

## 5) UI Wiring (delta to current mock)
- **FX Overlay**: add **Drop** tile → `DropPanel()` with Mode, Length, Sweep, Bake.
- **Note‑Repeat block**: add Ramp (Flat/Up/Down) and Spread (0..12) controls; record to steps when WRITE.
- **RATCHET hold** → `RetriggerPanel()`; expose same fields as Step Editor.
- **Step Editor**: When `ratchet>1`, show **Retrigger** group (Shape, Spread, Curve, Decay, Type, Start%, Anchor, Jitter).
- **LEDs**: FX key pulses when Drop latched; meter strips grey for muted channels.

---

## 6) MIDI & Scenes (small deltas)
- **MIDI CC** (optional profile):
  - CC20–23 → performance macros (unchanged).
  - New: CC24 **Drop Mode** (0=off, 1=Solo, 2=MuteArmed, 3=Kill, 4=KickOnly, 5=DrumsOnly).
  - New: CC25 **Drop Length** (0=Hold, 32=¼, 64=½, 96=1, 127=2 bars).
  - New: CC26 **Retrig Ramp** (0=Flat, 32=Down, 96=Up) + CC27 **Retrig Spread** (0..127 ≈ 0..12 st).
- **Scenes**: snapshot macro values + mixer + sends (unchanged). Optional flag to *include current Drop mask* when saving Scene.

---

## 7) QA / Edge Cases
- **Click‑free**: Verify per‑channel mute/unmute ramps prevent clicks at low frequencies.
- **Overlap**: Triggering a second Drop before the first ends should replace the active mask (last wins). Baking should merge into non‑overlapping automation blocks.
- **Ratchet ties**: If `anchor=FIXED_RATE` and ratchet overflows step length, require step `tie=true` or clamp at pattern boundary.
- **Humanize**: Ensure Quantize/Humanize post‑edit respects sub‑hit jitter (cap at ±5 ms).
- **Persistence**: Old projects (without retrigger fields) load with `ratchet=1` and default retrigger params.

---

## 8) Dev Tasks (checklist)
- [ ] Add Drop tile + panel; implement `MixerAutomation` event and renderer.
- [ ] Add Ramp/Spread to Note‑Repeat; write retrigger params when recording repeats.
- [ ] Implement `RetriggerPanel` (RATCHET hold) + Step Editor group.
- [ ] Voice scheduler: implement retrigger expansion with LUT pitch ratios and poly cap.
- [ ] Master LPF sweep option for Drop.
- [ ] LED/meter feedback for Drop states; toast messaging for Bake/Latch.
- [ ] Unit tests: event overlap resolution; retrigger pitch accuracy ±1 cent.

---

## 9) Glossary
- **Pitch‑Ramped Retrigger**: A sequence of rapid sub‑hits whose playback rate (pitch) ramps over the step (often used for drum “zips”).
- **Zipper noise**: Audible stepping from coarse parameter resolution—**not** the effect above; avoid this term in UI unless clarified.

— **End of Top‑Up v1.1** — Ready to hand to firmware/UI devs as an additive change list.

