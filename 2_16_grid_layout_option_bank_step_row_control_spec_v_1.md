# 2×16 Grid Layout — Option Bank + Step Row (Control Spec v1)

**Purpose**: Define a fast, PO-like, performance-first control scheme using **two rows of 16 keys**:
- **Top row (16)** = **Option Bank** (pattern/instrument tools, edit tools, stamps, performance/mix).
- **Bottom row (16)** = **Step Sequencer** (always live, never stolen by modes).

Designed to pair with your 720×720 UI (≈72×72 mm) and **Transport‑4**: **PLAY**, **REC**, **WRITE**, **SHIFT**. One main encoder (+ up to 3 performance encoders) is assumed but optional.

---

## 1) Design Principles
- **Immediacy**: One press = one result. Overlays appear on screen; **step row remains active**.
- **Low modal depth**: Only short overlays (Pattern/Sound/Clone/FX/Scene) and a sticky Tools layer.
- **PO muscle memory**: Stamps and tools work like PO punch‑ins; MPC‑like per‑pattern Scenes.
- **Single-source-of-truth**: Scope chip on screen indicates **STEP / INSTR / PATTERN** when a Tool is active.

---

## 2) Hardware Summary
- **Keys**: 2×16 grid. Top = Option Bank; Bottom = Steps.
- **Transport‑4**: PLAY, REC, WRITE, SHIFT (physical).
- **Encoders (optional)**: 1× Main (context-sensitive), 3× Performance (map to instrument macros M1–M4).
- **LEDs**: Per‑key RGB (recommend pastels), with Solid/Blink/Pulse semantics (see §7).

---

## 3) Top Row (Option Bank, left→right; grouped in quads)

### Quad A — Pattern / Instrument (Keys 1–4)
| # | Label  | Primary Tap | Hold | SHIFT + Key |
|---|--------|-------------|------|-------------|
| 1 | **INST** | Open **Instrument Setup** for the **armed slot** (engine/preset, poly/mono, glide, macro map). | Quick toggle Poly/Mono + Glide slider. | Jump directly to **Engine/Preset** browser; assigning loads & arms slot. |
| 2 | **SOUND** | Mini overlay of **I1–I16** to arm/select an instrument slot. | Multi-assign behavior for chords (chromatic) or pad-subselect (KIT). | **Scale & color** palette for the armed slot (scale lock, pad tint). |
| 3 | **PATTERN** | Overlay **A–P** to select/queue without stealing steps. | **Chain mode**: tap more letters; press **Done** on screen. | **Pattern Length** cycle: 16→32→48→64 (page dots appear). |
| 4 | **CLONE** | Clone current pattern (events + scene + macros) to destination **A–P**; auto‑queue. | Clone **with assets** (duplicate sample/slice refs). | Clone **shell only** (length + scene; no events). |

### Quad B — Edit Tools (Keys 5–8) — *Sticky until PLAY or another Tool*
| # | Label  | Primary Tap | Hold / Notes | SHIFT (while tool armed) |
|---|--------|-------------|--------------|--------------------------|
| 5 | **COPY** | Tap source step(s), then any number of targets to paste. | Multi-source OK; last source is previewed. | **Scope ↑**: STEP → INSTR → PATTERN. |
| 6 | **CUT** | As COPY, but first target removes from source. | — | **Scope ↑** as above. |
| 7 | **DELETE** | Taps erase at current scope. | Long‑press step opens confirm toast. | **Scope ↑** as above. |
| 8 | **NUDGE** | Each step tap applies ±preset microshift. | Hold a step to open fine micro slider. | **Scope ↑** as above. |

> **Scope** changes only when a Tool is active. Screen shows a small chip at top‑left: **STEP / INSTR / PATTERN**. SHIFT cycles the scope.

### Quad C — Stamps (Keys 9–12) — *Affect new entries until toggled off*
| # | Label  | Primary Tap | Hold | SHIFT + Key |
|---|--------|-------------|------|-------------|
| 9 | **ACCENT** | Toggle on/off; shows accent chevron on steps. | — | — |
| 10 | **RATCHET** | Cycle ×2→×3→×4→off. | Small divisor picker overlay. | — |
| 11 | **TIE** | Toggle; shows tie line across steps. | — | — |
| 12 | **MICRO** | Cycle −25% → 0 → +25%. | Fine micro grid overlay. | — |

### Quad D — Performance / Mix (Keys 13–16)
| # | Label  | Primary Tap | Hold | SHIFT + Key |
|---|--------|-------------|------|-------------|
| 13 | **FX** | Punch‑in FX overlay: **Stutter / Tape / Filter / Delay**; Note‑Repeat rates. | **Bake** FX to pattern (confirm). | **Latch** FX (stays active hands‑free). |
| 14 | **QUANT** | Apply quantize at current strength (toast). | Strength slider; **Humanize** toggle. | Humanize ±% bias. |
| 15 | **SWING** | Nudge swing ±1% per press (alternating). | Swing slider overlay. | Per‑pattern swing scene (store in scene snapshot). |
| 16 | **SCENE** | Snapshot/Recall **A–D** (mixer + macro lanes). Tap = recall. | Hold = save to A–D. | Morph time / crossfade between scenes. |

---

## 4) Bottom Row (16) — Step Sequencer
- Always active; **write** gates and notes according to the armed instrument.
- **Long‑press any step** → Step Editor (Length, Micro, Ratchet, Tie, Accent).
- **Stamps** (Accent/Ratchet/Tie/Micro) apply to **new** taps only.
- **Tools** (Copy/Cut/Delete/Nudge) act on tapped steps; **PLAY** exits Tool.
- **Page dots** appear when Pattern Length >16; tapping a dot jumps page; steps show absolute indices.
- **KIT**: step cells show the sub‑pad name (e.g., “Snare”); **CHROMA**: note or “Chord n”; **SLICED**: slice number.

---

## 5) Transport Chords
- **SHIFT+PLAY** — Tap tempo (each press nudges BPM; on-screen tap meter).
- **SHIFT+REC** — **Instrument Setup** for the currently armed slot.
- **SHIFT+WRITE** — Toggle **STEP ↔ PAD** view on the screen (does not change bottom row behavior).

---

## 6) Overlays (Non‑modal; bottom steps remain live)
- **Instrument Setup (INST / SHIFT+REC)**: engine, preset list, poly/mono, glide, macro mapping (M1–M4), sample load/edit shortcuts.
- **SOUND**: 4×4 grid of I1–I16; tap to arm; SHIFT opens Scale/Color palette.
- **PATTERN**: A–P grid; **Hold** enters chain mode; SHIFT cycles pattern length.
- **CLONE**: A–P destination grid; Hold = clone with assets; SHIFT = shell only.
- **FX**: 2×2 FX + Note‑Repeat buttons; SHIFT latches; Hold bakes to pattern.
- **SCENE**: A–D chips; Tap recall; Hold save; SHIFT morph time slider.

---

## 7) LED / Feedback Rules
- **Color coding** (recommended):
  - Quad A (Pattern/Instr) — **Lilac**
  - Quad B (Tools) — **Blue**
  - Quad C (Stamps) — **Peach**
  - Quad D (Perf/Mix) — **Mint**
- **States**:
  - **Solid** = armed (sticky tool or active stamp)
  - **Blink** = awaiting a target (COPY/CUT/CLONE)
  - **Pulse** = latched FX
- **Step LEDs**: small corner dot for **accent**, tie line, **×n** badge for ratchet, left/right micro dot for early/late.

---

## 8) Scope Model (when a Tool is active)
- **Default scope** = STEP. Press **SHIFT** while a Tool is active to cycle **STEP → INSTR → PATTERN**.
- Screen shows a chip with the current scope; actions apply accordingly (e.g., DELETE at PATTERN removes all events in current pattern; COPY at INSTR duplicates all steps for the armed instrument page).

---

## 9) Encoders (optional but recommended)
- **Main Encoder**: context‑sensitive (e.g., navigate overlays, adjust values in Step Editor).
- **Performance Encoders (1–3)**: map to M1–M3 (or M1–M4 if four encoders); **Perf Lock** toggle keeps mappings across pattern changes.

---

## 10) Common Flows
- **Clone & queue quickly**: `CLONE → tap P on screen` → keeps playing; bottom row still steps.
- **Hat rolls**: `RATCHET ×3 → tap steps 5/13` → exit by tapping RATCHET to off.
- **Late claps**: `MICRO (+25%) → tap steps` → turn off MICRO.
- **A→B→B chain**: `PATTERN (hold) → tap A,B,B → Done`.
- **Latch Filter FX then bake**: `SHIFT+FX (choose Filter) → perform → FX (hold) → Bake`.

---

## 11) Error/Conflict Handling
- **Stamp conflict**: applying TIE where no following step exists auto‑extends pattern or warns (configurable).
- **Tool safety**: DELETE at PATTERN scope prompts toast “Delete ALL events in Pat X? Hold to confirm”.
- **Clone with assets** on insufficient RAM warns; fallback to event‑only clone.

---

## 12) MIDI / Sync (outline)
- **Clock**: In/Out (24 PPQN), Start/Stop/Continue.
- **Notes**: bottom 16 steps can echo as gates; top row can send CCs for FX/Stamps (optional profile).
- **Macro CC map**: M1–M4 to CC #20–23 (configurable per slot). Scenes snapshot encoder values.

---

## 13) Persistence
- **Per‑pattern**: steps, length, page micro, stamps, FX bakes.
- **Per‑instrument slot**: engine/preset, macros, sample refs, scale/color.
- **Per‑song**: pattern bank A–P, chains, scenes A–D, global swing/quant.

---

## 14) Implementation Notes (UI hooks for your 720×720 mock)
- Map top row presses to existing overlays in the React mock:
  - **INST** → open `InstSetupOverlay()` (already stubbed in recent code).
  - **SOUND** → `SoundSelectOverlay()`; SHIFT → Scale palette overlay.
  - **PATTERN** → `PatternSelectOverlay()`; SHIFT → pattern length.
  - **CLONE** → `ClonePatternOverlay()`.
  - **FX** → `FxRepeatOverlay()`.
  - **SCENE** → `SceneOverlay()`.
- Tools and Stamps already exist logically (COPY/CUT/DELETE/NUDGE & Accent/Ratchet/Tie/Micro); wire top-row key events to set those states (and show LED/overlay toasts).

---

## 15) Testing Checklist
- [ ] Bottom steps remain live during all overlays.
- [ ] Tools respect scope cycling via SHIFT.
- [ ] Stamps only affect **new** entries.
- [ ] Pattern length paging & dots rendered.
- [ ] Scenes snapshot mixer+macros; morph time respected.
- [ ] Bake FX writes automation/events and clears latch.

---

**End of Spec v1** — Ready for firmware/UI wiring. Let me know if you want a compact 1‑pager cheat‑sheet for the top row silk. 

