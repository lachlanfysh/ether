# Synth Groovebox — Controls & GUI Spec (v1)

**Config covered:** 4×4 Hall/MX pad grid • 1×5 function-key column • 1×4 encoders (E0 main + E1–E3 performance). Transport is on **E0 press/long-press**; function keys drive layers (Instrument, Pattern, FX/Repeat, Tools, Shift). This spec captures **all control details** and **all GUI behavior** needed for a first firmware/UI implementation.

---

## 0) Goals & Design Principles
- **Hardware‑first immediacy:** PO-style speed (arm → write → place/perform) with minimal screen taps.
- **One mental model:** 16 **Instrument slots** (I1–I16). Patterns/Sequences store per‑instrument events + a mixer scene.
- **Low mode cost:** Layers are **hold‑momentary, tap‑to‑latch**; clear LEDs; PLAY cancels Tools.
- **Performance-centric:** 3 assignable **Performance encoders** (P1–P3) always do something musical; easy re‑mapping.
- **Safe iteration:** one‑gesture **Clone Pattern → Slot**; Undo for destructive ops.

---

## 1) Physical Controls & I/O
### 1.1 Pads
- **16 large pads** (Hall/MX), 19.05 mm pitch; **velocity + pressure (aftertouch)** if Hall.
- Per‑pad RGB (or bicolor) LEDs.

### 1.2 Function keys (1×5 column, top→bottom)
1) **INST** — Instrument layer
2) **PATT** — Pattern/Sequence layer
3) **FX/REPEAT** — Punch‑ins & Note‑Repeat rates
4) **TOOLS** — Copy/Cut/Delete/Nudge layer
5) **SHIFT** — momentary; **double‑tap = latch**

> If a different order is preferred on the panel, firmware treats them as addressable roles.

### 1.3 Encoders (1×4, vertical)
- **E0 — Main** (detents recommended; **push** & **long‑press**)
- **E1, E2, E3 — Performance encoders** (assignable; **push/hold** supported)

### 1.4 Transport & mode
- **E0 press**: **Play/Stop**
- **E0 long‑press**: **Rec/Write** toggle
- **SHIFT + E0 press**: **Tap‑tempo**
- **Pad‑Play ↔ Step‑Edit** mode toggle: **SHIFT + (Rec/Write)** chord *(i.e., SHIFT + E0 long‑press)* or GUI chip

### 1.5 Suggested mechanics
- Key scan ≥ 1 kHz; per‑switch diodes; Hall sampling 4–8 kHz per pad.
- End‑to‑end pad→sound latency ≤ **7 ms**; LED feedback ≤ 10 ms.

---

## 2) Operating Modes & High‑Level State
- **BASE (Pad‑Play):** pads perform the **armed instrument** (velocity/AT). Write ON = live overdub.
- **STEP (Step‑Edit):** pads = **steps 1–16** of the armed instrument’s lane (long‑press a step → mini editor: Length/Micro/Ratchet/Tie).
- **Layers (momentary/latched):** INST, PATT, FX/REPEAT, TOOLS overlay pad meanings while held; latch on tap.
- **Stamps:** Accent / Ratchet / Tie / Micro are sticky **input decorators** applied to newly placed steps.

> **PLAY** is the universal **cancel** for Tools. SHIFT LED indicates momentary vs latched.

---

## 3) Function‑Key Layers (Pad meanings while held)
### 3.1 INST — Instrument layer
- Pads **1–16 = I1–I16**: tap to **arm/preview**.
- **SHIFT + pad**: **Duplicate** instrument (copy patch/samples/params to next free slot).
- **Hold E0 long (Rec) + pad**: **Record** into that slot → enters **CHOP** → **Assign 16** → returns to prior state.

### 3.2 PATT — Pattern/Sequence layer
- Pads **1–16 = A–P**: tap to **select** (queued on bar if playing).
- While held, tap multiple = **Chain** (A→B→C…).
- **SHIFT + pad**: **Duplicate** pattern.
- **Hold E0 long (Rec) + pad**: **Clear** pattern (screen confirm: **Play=Yes**, **Shift=No**).

### 3.3 FX/REPEAT layer
- Pads **1–4**: **Punch‑ins** *(Stutter, Tape‑Stop, Filter, Delay)* — **momentary**; **hold SHIFT while selecting** to **Latch**.
- Pads **9–16**: **Note‑Repeat rates** (¼, ⅛, ⅛T, 1/16, 1/16T, 1/32, 1/32T, 1/64) — sets the default repeat rate used in Pad‑Play.
- **E0 turn** adjusts selected FX rate/wet; **E0 press** toggles latch when an FX is focused.

### 3.4 TOOLS layer (tap to select tool, hold to engage)
- Tool order: **Copy → Cut → Delete → Nudge** (TOOLS LED/icon shows active selection).
- **Copy/Cut:** tap a **source** (a step in STEP or a whole lane in INST scope), then any **targets**; **PLAY** cancels.
- **Delete:** taps erase at current **Scope** (see §5). Confirm only for **Pattern** scope.
- **Nudge:** taps microshift ± preset (e.g., ±25%); **E0 turn** = fine micro.

---

## 4) Stamps (input decorators)
- **Accent (on/off)**
- **Ratchet (×2→×3→×4→off)**
- **Tie (on/off)**
- **Micro (−25%→0→+25%)**

**Behavior:** Active stamps apply to **new steps** placed in STEP. One‑offs editable via step long‑press.

**Access:**
- Quick: **E3 press** can be set as a **Stamp cycle** (optional), or keep stamps within a small on‑screen footer with tap‑to‑toggle.
- Always mirrored as icons in the GUI footer; stamp LEDs solid when active.

---

## 5) Scope Resolution (what Tools operate on)
- **Default:** **STEP scope** (armed instrument’s lane in current pattern).
- If the last layer used was **INST** and SHIFT is still held/latched → Tools act on **instrument lanes**.
- If the last layer used was **PATT** and SHIFT is held/latched → Tools act on **patterns** (whole‑pattern ops).
- **PLAY** exits Tool and returns scope to default.

---

## 6) Encoders
### 6.1 E0 — Main encoder
| Context | Turn | Press | Long‑press |
|---|---|---|---|
| **BASE/STEP** | Move step focus (±1/16) | **Play/Stop** | **Rec/Write** toggle |
| **STEP (on a focused step)** | Micro nudge (fine) | Toggle step | Open step editor |
| **CHOP** | Next/prev transient | Drop/move marker | Zoom |
| **FX layer** | Adjust rate/wet of focused FX | Latch/unlatch | Reset FX |
| **PATT layer** | Select/queue | Clone to selected | Open chain editor |

- **SHIFT + E0 press**: Tap‑tempo.
- **Soft‑takeover** for parameter edits where applicable; smoothing 3–10 ms.

### 6.2 E1–E3 — Performance encoders (assignable)
Each P‑encoder holds a **Performance Map**:
- **Target:** `(Instrument I1–I16)` + `(Parameter)`; up to **4 stacked targets** (macro‑style) per encoder.
- **Depth:** −200%…+200%; **Curve:** Linear/Exp/S‑curve; **Pickup:** soft‑takeover on/off.
- **Write behavior:** If **Write** is ON and target is automatable → record **automation lane** (per Sequence). If target is a mixer/scene value → update the **Scene** live (Undoable). Optionally **SHIFT+press** on a P‑encoder toggles “write as automation” force.

**Assign (no menu):**
1) **Hold** E1/E2/E3.  
2) **Tap a pad** (instrument).  
3) **Turn E0** to browse that instrument’s parameters; **press E0** to confirm.  
4) **Release** the P‑encoder. (Turn P to play; **SHIFT+P turn** adjusts depth; **P long‑press** opens tiny overlay for depth/curve/pickup.)

**Add targets:** **Double‑tap** P‑encoder, repeat steps to append (max 4). **Hold P + PLAY** removes last target.

**Per‑Sequence mappings:** Each Pattern/Sequence can **override** P1–P3 mappings + current values.
- **Lock Performance** across patterns: **SHIFT + E0 long‑press** (header shows lock icon).

**Header labels:** `P1  I4:Cut`, `P2  I3:Start`, `P3  I7:Send`; bars show value & pickup status (arrow until caught).

**Suggested defaults:**
- **P1:** armed instrument **Filter Cutoff**  
- **P2:** armed instrument **Decay/Release** (Pad engines) or **Env Amt** (Synth)  
- **P3:** armed instrument **Send** (Delay/Verb)

---

## 7) Events & Data (Sequencer)
- **Per Pattern/Sequence:** `events[16]` (one lane per instrument): note, length, velocity, micro, accent, ratchet, tie.  
- **Mixer Scene:** level/pan/send for I1–I16 (+ FX params if exposed).  
- **Automation lanes:** 2 lanes reserved per instrument for **Macro 1/2** (continuous curves), written via encoders or GUI.

> No per‑step parameter locks in v1 (keeps the surface fast). Encoders/automation cover motion.

---

## 8) FX & Note‑Repeat
- **Punch‑ins:** Stutter, Tape‑stop, Filter (LP/HP), Delay send. **Momentary**; latch with SHIFT during selection.
- **Note‑Repeat:** the selected rate applies to Pad‑Play; respects Swing; latchable via FX layer if desired.
- **Bake FX:** in FX layer, **SHIFT + E0 long** opens **Bake** dialog (range bar/sequence; target replace/new slot; normalize). Printed audio becomes a new Instrument slot (original muted for Undo).

---

## 9) Sampling & CHOP Flow
1) **Hold E0 long (Rec) + pad** in INST layer (or on armed instrument) → record.  
2) **CHOP** opens with waveform + auto‑transients.
   - **E0 turn**: next/prev marker; **press**: drop marker; **long**: zoom.
   - Pads audition slices; **Assign 16** fills a **Sliced Kit** slot or remaps an existing kit.
3) Return to prior state (preserve SHIFT latch), armed to the new/updated slot.

Options: offline **Normalize/Time‑process** (stretch/pitch) before Assign.

---

## 10) Mixer & Scenes
- **MIX view:** 16 mini channel strips: fader, pan, send, mute/solo.
- **Scene Snapshot (per Pattern):** save/recall **A–D**; invoked from **Other → P16** (or as on‑screen chips).
- Scenes recalled on Pattern change unless **Lock Performance** is on; Undoable.

---

## 11) Automation & Global LFO (optional, recommended)
- **Per‑Sequence automation**: 2 lanes per instrument (Macro1/Macro2). Written by moving macro sliders or P‑encoders while Write is ON. Smoothing 3–10 ms.
- **Global LFOs per instrument** (persist across Sequences): LFO1, LFO2, and **Follower** (sidechain). Target **Macros** by default.
  - **Sync** (1/1…1/64 + dotted/triplet) or **Hz** (0.01–20 Hz). Shapes: Sine, Triangle, Square (PW), Ramp, S&H, Jitter, Chaos.
  - **Phase modes:** Free‑run / Reset on bar / Reset on note.
  - **Per‑Sequence depth offset** slider (SEQ page) lets A/B sections breathe differently without duplicating instruments.

**Evaluation order:** `Param = Base + Automation + LFO1 + LFO2 + Follower → clamp → smooth`.

---

## 12) GUI Architecture (screen is feedback‑first)
### 12.1 Header (always visible)
`▶/■  •  Pattern A–P  •  Tempo (tap)  •  Swing%  •  Quantize%  •  Write  •  CPU/voices  •  P1/P2/P3 labels`

### 12.2 Main view
- **STEP** (when in Step‑Edit): 16‑step lane (pages appear at 16/32/48/64); tap steps; long‑press opens mini editor. Footer shows active stamps.
- **PAD** (when in Pad‑Play): macro 1/2 sliders, pad meters, armed instrument name; Repeat rate chip.
- **MIX** (toggle chip): 16 faders + pan/send; Scene A–D chips (Save/Recall).
- **SEQ** (Pattern list/chain): A–P tiles; current chain; small “Motion depth” sliders per instrument if LFOs are enabled.
- **CHOP** (modal after record/import): waveform, markers, Assign 16 button, Normalize/Time options; encoder tips on the side.

### 12.3 Overlays/legends
- Holding any function key shows a compact **legend overlay** relabeling pads for that layer.
- Tool/FX/bake confirmations are small, two‑button popups: **Play = Yes**, **Shift = No**.

---

## 13) LED & Feedback Rules
- **Function keys:** dim idle → bright while held → **solid** when latched (tap to unlatch).
- **TOOLS:** LED/icon indicates chosen tool; **slow‑blink** while engaged.
- **Pads:**
  - Palette tint differs for BASE vs STEP.
  - **Accent** brighter; **Ratchet** double‑blink on that step; **Tie** underline/continuation; **Micro** small left/right tick.
- **Encoders:** optional halo tick flash on detents; P‑encoders blink while in **assign‑hold**.

---

## 14) Defaults & Persistence
- Startup: Pattern A, Instrument I1 armed, **Write OFF**, 120 BPM, Swing 55%, Quantize 50%.
- Pattern length 16 (expandable to 32/48/64; preserves notes).
- Per‑instrument stored: engine type, patch, macro targets, scale/root (for tone engines).
- Per‑Pattern stored: events[16], mixer scene, automation lanes, (optional) Performance Map overrides.
- P‑encoder **default maps** live at project level; per‑Pattern overrides when present.

---

## 15) Acceptance Criteria (functional)
**Layers & transport**
- E0 press toggles Play/Stop; long‑press toggles Rec/Write; SHIFT+E0 press Tap‑tempo.
- Holding INST/PATT/FX/TOOLS immediately repurposes pads; tap latches; LEDs indicate state.

**INST**
- Pad selects/arms instrument; SHIFT+pad duplicates; E0 long+pad records → CHOP → Assign → return.

**PATT**
- Pad selects/queues; multi‑tap chains; SHIFT+pad duplicates; E0 long+pad clears with confirm.

**FX/REPEAT**
- Pads 1–4 throw FX (momentary or latched with SHIFT); pads 9–16 set Repeat rate. E0 turn edits wet/rate.

**TOOLS**
- Copy/Cut select source then paste to multiple targets; PLAY cancels.
- Delete erases at scope; Pattern‑scope delete confirms.
- Nudge performs ± micro; E0 turn provides fine.

**STEP**
- Tap to toggle step; long‑press edits Length/Micro/Ratchet/Tie. Stamps apply to new placements.

**Encoders**
- P1–P3 assign via **hold P → pad → E0 turn/press**; multi‑target via double‑tap P; soft‑takeover works; Write records automation where applicable.

**Performance Lock**
- SHIFT+E0 long toggles lock; header shows lock; mappings persist across Pattern changes when locked.

**Latency**
- Pad→sound ≤ 7 ms; LED ≤ 10 ms; UI overlays ≤ 100 ms.

---

## 16) Icons & Color (silkscreen/legend)
- **INST** (blue) — wave/key icon; **PATT** (blue) — grid; **FX** (blue) — spark/star; **TOOLS** (orange) — scissors/doc; **SHIFT** (neutral).
- **Stamps** (green) — Accent “>”, Ratchet “≡×n”, Tie slur, Micro ± clock.
- **Other chips** (purple) — Quantize magnet/grid, Swing pendulum, Length bar‑stack, Scene camera/mixer.

---

## 17) Appendix A — Alternate Minimal (3 hard keys) Shift‑Map
If panel constraints force only **SHIFT + PLAY/STOP + REC/WRITE**, use the previously provided **Palette/Tool/Stamp/Other** Shift map (P1–P16 while holding SHIFT). Firmware can compile either map.

---

## 18) Appendix B — Timing, Smoothing, Storage
- **Debounce:** 3–5 ms; **hold** threshold 350–500 ms; **double‑tap** 250–350 ms.
- **Smoothing:** 3–10 ms one‑pole on mod/FX; clamp at bounds.
- **Storage:** efficient diffs for Scenes & automation; samples referenced, not duplicated, by Clone Pattern.

---

## 19) Appendix C — Small State Chart (2×3 layout)
```
States: BASE, STEP, LAYER_INST, LAYER_PATT, LAYER_FX, LAYER_TOOLS
(STAMP is a flag bitmask)

E0 press   -> PlayToggle
E0 long    -> RecWriteToggle
SHIFT+E0   -> TapTempo
SHIFT+Rec  -> ModeToggle (BASE<->STEP)

Hold INST  -> LAYER_INST (pads=I1..I16)
Hold PATT  -> LAYER_PATT (pads=A..P)
Hold FX    -> LAYER_FX   (pads FX/Repeat)
TOOLS tap  -> select tool; TOOLS hold -> engage tool
PLAY       -> cancel tool / confirm Yes in dialogs
SHIFT      -> No in dialogs / latch modifier
```

---

**End of v1.** This document is implementation‑ready; ambiguities should be resolved in firmware notes as comments in code or tracked in the issue list (e.g., exact FX set, microstep quantization granularity).


---

## 20) Song Architecture & Data Model (Developer Spec)

> This section formalizes **entities, timing, persistence**, and **runtime rules** so multiple devs can implement UI, sequencer, and storage in parallel.

### 20.1 Entities & IDs
- **Project** (root)
  - `project_id` (UUID v4)
  - `meta` { name, author, created_utc, last_modified_utc, sample_rate, tempo_default_bpm }
  - `instruments[16]` (fixed-size array of **InstrumentSlot**)
  - `sequences[]` (ordered list of **Sequence** objects; max 99)
  - `song` (**Song**)
  - `globals` (**GlobalSettings**)
  - `patch_library[]` (optional cached patches)
  - `media/` (samples, rendered stems, etc.)

- **InstrumentSlot** (I1–I16)
  - `slot_ix` (0..15)
  - `engine_type` (enum: DRUM_KIT, SLICED_KIT, DRUM_SYNTH_KIT, CHROMATIC_SAMPLER, SYNTH_[MI_*], …)
  - `patch_ref` (id or inline patch object)
  - `perf_defaults` (default P1–P3 perf maps; see 20.6)
  - `scale_root` (0..11), `scale_mask` (12-bit), `mono_legato` (bool)
  - `lfo_config` { lfo1, lfo2, follower } (global; see 20.7)

- **Sequence** (aka Pattern; label usually A–P for the first 16)
  - `seq_id` (UUID)
  - `label` (string, e.g., "A")
  - `time_sig` { num:4, den:4 } (v1 fixed 4/4; structure ready for future)
  - `length_steps` (16 | 32 | 48 | 64)  
  - `swing_pct` (0..75, default 55)
  - `quantize_strength_pct` (0..100, default 50)
  - `events[16]` (array of **Lane**; one per instrument slot)
  - `automation[16]` (per instrument: Macro1/Macro2 lanes)
  - `mixer_scene` (**Scene**)
  - `perf_overrides` (optional **PerfMapSet** for P1–P3; see 20.6)

- **Song** (arranger)
  - `chain[]` (list of **ChainItem**: `{ seq_id, repeats }`)
  - `loop` { start_ix, end_ix, enabled }  
  - `next_queue` (runtime-only: pending next seq; cleared once taken)

- **Lane** (per instrument in a Sequence)
  - `notes[]` (list of **NoteEvent**) sorted by time

- **NoteEvent**
  - `tick` (int, absolute within sequence; see 20.3 timing)
  - `len_ticks` (int ≥ 1)
  - `vel` (0..127)
  - `accent` (bool)
  - `ratchet` (1|2|3|4) // 1 = none
  - `micro` (int; −MICRO_MAX..+MICRO_MAX, default 0)
  - `tie` (bool)

- **AutomationLane** (per instrument per Sequence; 2 lanes M1/M2)
  - `points[]` (time/value breakpoints; see 20.5)

- **Scene** (per Sequence)
  - `ch[16]` { level_db, pan_0to1, send_0to1, mute, solo }
  - `fx` { master_glue, delay_time, delay_fb, verb_amt, … (minimal in v1) }

- **PerfMapSet** (per Sequence overrides)
  - `P1`, `P2`, `P3` → each a **PerfMap** (targets, depths, curves, locked?)

- **GlobalSettings**
  - `velocity_curve`, `pad_calibration`, `latency_offsets`, `ui_theme`, …

**ID policy**: immutable `seq_id`/`project_id` are UUIDs; instrument slots are referenced by **index** (0..15) for compactness.

---

### 20.2 Engine Limits & Voice Model
- **Global voices**: 24–32 (build-time constant).  
- **Per-instrument**: soft cap 2–4; **choke groups** honored for kits.  
- **Voice steal**: last-note priority per instrument; drums prefer the oldest in same choke group.

---

### 20.3 Timing & Transport
- **Master tick resolution**: **TPQN = 480** (ticks per quarter note).  
- **Bar ticks (4/4)**: 4 × 480 = **1920**.  
- **Step ticks** at 16 steps/bar: 1920 / 16 = **120**.
- **Microshift**: **±60** ticks (±½ step). Stored in `NoteEvent.micro`.
- **Ratchet** rendering**: ratchet n=2/3/4 emits n gates spaced evenly across the step; each child gets `len_ticks / n` with minimum 15 ticks (clamp).  
- **Swing**: apply to off-beat 1/8th grid: tick′ = tick + swing_pct * swing_table[pos] (table precalc; default 55%).
- **Quantize strength**: on record, move `tick` toward nearest grid by `strength%`.
- **Automation sample rate**: evaluate lanes at **250–500 Hz** control tick; linear segment interp.
- **Transport states**: STOP, PLAY, RECORD (Write ON).  
- **Pattern change quantization**: at **bar boundary**; `next_queue` consumed on boundary.

---

### 20.4 Editing Semantics
- **Pad-Play** adds live notes; **Step-Edit** toggles steps at centers (`tick = step_ix * 120`).  
- **Stamps** decorate *new* events (accent/ratchet/tie/micro).  
- **Delete Tool** removes:  
  - STEP scope: events overlapping selected step window `[center−60, center+60)` for the **armed instrument**.  
  - INSTR scope: clears entire lane.  
  - PATTERN scope: clears all lanes (+ automation + scene reset to defaults; confirm needed).

---

### 20.5 Automation Lanes (Macro1/Macro2)
- **Lane model**: sparse breakpoints `points[]` with `{ tick:int, value:float[0..1] }`, sorted.  
- **Interp**: linear; extrapolation clamps to nearest endpoint.  
- **Record**: when Write ON and macro moved, sample at control tick with **point merge**: if last point < 12 ticks apart, replace; else append.  
- **Erase**: holding Delete Tool and touching lane removes points in step window; full-lane clear exposed via Tools (INSTR scope).

---

### 20.6 Performance Encoders (P1–P3)
- **PerfTarget** = `{ instr_ix, param_id, depth:-2..+2, curve:LIN|EXP|SCURVE, pickup:bool }`.  
- **PerfMap** = up to 4 `PerfTarget` stacked.  
- **PerfMapSet** = `{ P1, P2, P3, locked:bool }`.
- **Evaluation** (per param): `value = base + Σ curve(depth_k * knob_norm)` → clamp → smooth.  
- **Write**: if `param_id` automatable → record into Macro lane (mapped Macro dest), else mutate Scene value.  
- **Per-Sequence overrides**: if present, replace defaults unless `locked` is true (then keep current maps across Sequence changes).

---

### 20.7 Global LFOs (per Instrument, optional in v1)
- **LFO**: `{ shape, rate_sync|hz, phase_mode(FREE|BAR|NOTE), depth_to_macro[4] }`
- **Follower**: `{ source_instr_ix|-1(master), attack_ms, release_ms, gain }` → typically depth-to Macro4 (send).  
- **Per-Sequence depth offset**: stored in `Sequence.perf_overrides` or a `sequence_motion[]` table if enabled.

---

### 20.8 Persistence (JSON outline)
```json
{
  "project_id": "…",
  "meta": { "name": "…", "sample_rate": 48000, "tempo_default_bpm": 120.0 },
  "instruments": [ {"slot_ix":0, "engine_type":"SLICED_KIT", "patch_ref":"p/01", "scale_root":2, "scale_mask":4095, "perf_defaults": {"P1":{…}, "P2":{…}, "P3":{…}} }, … ],
  "sequences": [
    { "seq_id":"…", "label":"A", "length_steps":16, "swing_pct":55, "quantize_strength_pct":50,
      "events": [ {"notes":[{"tick":0, "len_ticks":120, "vel":110, "accent":true, "ratchet":1, "micro":0, "tie":false} ]}, … ],
      "automation": [ {"M1":{"points":[{"tick":0,"value":0.3}]} , "M2":{"points":[]}}, … ],
      "mixer_scene": { "ch": [ {"level_db":-6.0, "pan_0to1":0.5, "send_0to1":0.2, "mute":false, "solo":false}, … ] }
    }
  ],
  "song": { "chain": [ {"seq_id":"…","repeats":4}, {"seq_id":"…","repeats":8} ], "loop": {"enabled":false} }
}
```

---

### 20.9 Undo/Redo Journal
- **Operation types**: note add/remove/edit, lane clear, pattern clone, scene change, perf-map change, tool ops (copy/cut), chop assign, tempo/swing/quantize change, song chain edit.  
- **Journal entry**: `{ op_type, entity_ref, before, after, timestamp }`.  
- **Depth**: ≥ 32 ops; ring buffer.

---

### 20.10 Acceptance (Song/Data)
- Changing `length_steps` preserves existing note ticks; out-of-range events are kept but not shown until length grows again.  
- Pattern clone copies events, automation, scene; samples are referenced; new `seq_id` assigned.  
- Song chain plays with correct bar-quantized transitions; `next_queue` respected even during chain playback.

---

## 21) Screens & GUI (Developer Spec)

> Target reference: **480×480** square; adapt with a scalable layout grid.

### 21.1 Layout Grid & Style
- **Safe areas**: 16 px outer margin.  
- **Header**: 480×54 px (11% height).  
- **Footer** (contextual): 480×44 px.  
- **Main pane**: 480×382 px.  
- **Typography**: UI base 12 px; small 10 px; heading 14–16 px; numeric fields 14 px monospaced.  
- **Touch targets**: ≥ 44×44 px; long‑press threshold 400 ms.

### 21.2 Header (always visible)
- Left cluster: **▶/■**, **Pattern label**, **BPM** (tap to open big slider dialog), **Swing%**, **Quantize%**, **Write badge**.  
- Right cluster: **CPU%**, **Voices**, **Perf labels** `P1 I4:Cut | P2 I3:Start | P3 I7:Send` (each has a tiny value bar + pickup arrow if not caught).

**Interactions**
- Tap **BPM/Swing/Quantize** → full‑screen slider with detents, **E0** also adjusts; **Play=OK**, **Shift=Cancel**.  
- Tap a **Perf label** → small popover: View targets, quick unassign, lock toggle.

---

### 21.3 STEP View (Step‑Edit)
- **Grid**: 16 steps horizontally; if `length_steps > 16`, show page dots (1–4).  
- **Step cell**: 20–24 px wide; shows: on/off, accent (bright), ratchet (double blink), tie underline, micro tick (left/right).  
- **Cursor** (E0): thin vertical line; E0 press toggles step.  
- **Stamp footer**: four pill buttons (Accent, Ratchet, Tie, Micro) mirroring hardware state.

**Long‑press on step** → popover (240×160 px):
- **Length** (1–8 steps) slider; **Micro** (−60..+60) dial; **Ratchet** selector (1/2/3/4); **Tie** checkbox.  
- Buttons: **Delete**, **Duplicate to +1 bar** (if length ≥ 32).

**Performance**
- Redraw budget < 2 ms/frame; grid pre‑rasterized; LED state diff only.

---

### 21.4 PAD View (Performance)
- **Macro sliders** M1/M2 (vertical) with value and target label; **P encoders** reflect value bars.  
- **Pad meters** (per‑pad mini VU for armed instrument).  
- **Scale/Root chip** (tap → quick picker: scales row, roots grid).  
- **Repeat chip** (shows current rate; tap to change quickly).

**Interactions**
- Drag a macro → writes automation if Write ON.  
- Tap instrument name → opens **Patch summary** (read-only essentials; full edit on long‑press INST if desired).

---

### 21.5 FX View
- **Punch‑ins**: 4 large buttons (stutter, tape, filter, delay); indicate latch.  
- **Focused FX** params: **Rate/Wet** big knobs; **E0** maps to Rate/Wet via focus; **Reset** button.

**Bake dialog** (SHIFT+E0 long in FX):
- Range: Bar | Sequence; Source: Pre/Post; Normalize: On/Off; Target: Replace | New slot.  
- **Play=Confirm**, **Shift=Cancel**.

---

### 21.6 MIX View
- **16 channels** (scroll if needed): fader (−∞..+6 dB, log curve), pan (−1..+1 with −3 dB center law), send (0..1), Mute/Solo toggles.  
- **Scene A–D chips**: Save (long‑press) / Recall (tap).  
- **Master** mini: output meter, limiter on/off.

**Acceptance**: faders snap at −12/−6/0/+3/+6 dB detents; pan snaps to center.

---

### 21.7 SEQ View (Patterns & Song)
- **Pattern tiles** (A–P visible; scroll for more). Tap to select; **E0** scrolls list.  
- **Chain editor**: list of `{ label, repeats }`; tap to edit repeats (×1..×16 with detents at 2/4/8).  
- **Queue Next** button: adds selected to `next_queue`.  
- **Lock Performance** toggle appears here; when ON, P‑encoder maps don’t change with pattern.

**Acceptance**: switching patterns takes effect on next bar; chain playback honors repeats exactly; cancel queue clears `next_queue`.

---

### 21.8 CHOP View
- **Waveform** with zoom; markers as draggable handles; zero‑cross snap toggle.  
- **Auto‑detect** sensitivity; **Normalize** toggle; **Time‑process** (coarse) optional.  
- **Assign 16** button; pads audition slices; E0: next/prev marker; press: drop marker; long: zoom.  
- **Exit** returns to previous view with the new/updated slot armed.

**Performance**: waveform cached to mipmap layers; zoom/pan under 16 ms.

---

### 21.9 GLOBAL/Settings
- **Tempo/Swing/Quantize defaults**, **Velocity curve** picker (graph), **Pad calibration wizard**, **Brightness**.  
- **Storage**: project size, free space, export.  
- **Engine options**: poly cap, oversampling modes (if applicable).

---

### 21.10 Overlays, Toasts, Confirms
- **Legends** appear when holding INST/PATT/FX/TOOLS; disappear <150 ms after release.  
- **Confirms**: Pattern delete and Bake; **Play=Yes**, **Shift=No**.  
- **Toasts**: 1.5 s info (e.g., "Cloned to C", "Scene A saved").

---

### 21.11 Acceptance (Screens)
- Header sliders open/close within 120 ms; encoder E0 controls them when open.  
- STEP operations (tap/long‑press) never block audio; UI thread ≤ 30% CPU budget.  
- P labels always reflect pickup state; automation dot shows while writing.  
- CHOP Assign returns to the exact prior layer/mode; SHIFT latch preserved.

---

## 22) Threading, Audio/GUI Bridge
- **Audio thread**: sample rendering; receives **scheduled note-ons** (tick‑aligned), automation samples, and Scene snapshots via lock‑free ring buffers.  
- **UI thread**: edits data structures; pushes changes as **delta messages** (`NoteAdd`, `LaneClear`, `SceneSet`, `PerfMapSet`, …).  
- **Clock**: high‑res timer; `tick_now` advanced by (BPM, TPQN); bar boundary callback enqueues Sequence switch.

**Message timing**
- Note events for the next **2 audio blocks** pre‑scheduled; late edits slip to next grid where needed.

---

## 23) Developer Test Matrix (excerpt)
- **Pattern clone**: clone A→B; verify events/automation/scene copied; seq_id differs.  
- **Next queue**: while A plays, queue C; confirm switch on next bar boundary; voices do not click.  
- **Perf lock**: lock ON in A; switch B; P labels and mappings unchanged; unlock → mappings follow B.  
- **Stamps**: enable Ratchet×3; place steps; verify 3 evenly spaced gates, min 15 ticks each.  
- **Automation write**: Write ON; move P1 mapped to cutoff; lane points created with merge rule; playback smooth.  
- **Scene recall**: save Scene A in pattern; tweak faders; recall A → exact restore.

---

End of extended developer spec (v1.1).
