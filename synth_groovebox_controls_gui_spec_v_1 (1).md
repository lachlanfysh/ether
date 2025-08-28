# Synth Groovebox — Controls & GUI Spec (v1.3, transport‑4)

**Config covered:** 4×4 Hall/MX pad grid • **Transport keys (1×4): PLAY, REC, WRITE, SHIFT** • 1×4 encoders (E0 main + E1–E3 performance). No dedicated INST/PATT/FX/TOOLS keys; those are entered via **SHIFT‑chords** on the pad grid. This spec supersedes prior 1×5 function‑key drafts and keeps all immediacy targets.

---

## 0) Goals & Design Principles
- **Hardware‑first immediacy:** PO‑style speed; pad + a few chords do 90% of actions.
- **One mental model:** 16 **Instrument slots** (I1–I16). Sequences (A–P) store per‑instrument events + a mixer scene.
- **Low mode cost:** Layers are **hold‑momentary, SHIFT‑latch on double‑tap**; clear on‑screen legends; PLAY confirms, SHIFT cancels.
- **Performance‑centric:** P1–P3 encoders always meaningful; easy re‑map without menus.
- **Safe iteration:** one‑gesture **Clone Pattern → Slot**; Undo for destructive ops.

---

## 1) Physical Controls & I/O

### 1.1 Pads
- **16 large pads** (Hall/MX), 19.05 mm pitch; **velocity + pressure** if Hall.
- Per‑pad RGB (or bicolor) LEDs.

### 1.2 Transport keys (1×4, top→bottom)
1. **PLAY** — Play/Stop; also **Yes** in dialogs
2. **REC** — Sampling/record flow; also opens CHOP after capture
3. **WRITE** — Sequencer/automation write toggle; also Step↔Pad mode chord
4. **SHIFT** — momentary modifier; **double‑tap = latch**; **No** in dialogs

> Transport keys are addressable by role; physical order can change in the panel.

### 1.3 Encoders (1×4, vertical)
- **E0 — Main** (detents; **press** & **long‑press** supported)
- **E1, E2, E3 — Performance** (assignable; **press/hold** supported)

### 1.4 Transport, mode & chords
- **PLAY (tap):** Play/Stop
- **REC (tap):** Arm/capture sample → enters **CHOP** → **Assign 16** → return
- **WRITE (tap):** Write/Overdub ON/OFF (notes & automation)
- **SHIFT+PLAY:** Tap‑tempo
- **SHIFT+REC:** Record source menu (Input/Mic/USB, mono/stereo, pre/post) **or** quick‑arm last used
- **SHIFT+WRITE:** Toggle **Pad‑Play ↔ Step‑Edit** mode
- **SHIFT double‑tap:** Latch modifier (sticky until tapped again)

Latency: end‑to‑end pad→sound ≤ **7 ms**; LED feedback ≤ 10 ms; hold threshold 400–600 ms.

---

## 2) Operating Modes & High‑Level State
- **PAD‑PLAY:** pads perform the **armed instrument** (velocity/AT). WRITE ON = live overdub.
- **STEP‑EDIT:** pads = **steps 1–16** of armed instrument lane (long‑press a step → mini editor: Length/Micro/Ratchet/Tie; chord support for chords in Chromatic).
- **Layers (entered via SHIFT‑chords):** INST, PATT, FX/REPEAT, TOOLS overlay pad meanings while held; latch by **SHIFT double‑tap**, exit on SHIFT release.
- **Stamps:** Accent / Ratchet / Tie / Micro are sticky **input decorators** applied to newly placed steps.

PLAY is the universal **confirm**, SHIFT is **cancel** in two‑button dialogs.

---

## 3) SHIFT Map (Pad Functions)

When **SHIFT** is held (or latched), the 4×4 pads expose four rows of functions. Labels appear on‑screen immediately. Pads are referred to as **P1..P16** (row‑major; top‑left = P1).

### 3.1 Palette (one‑shot) — top row (P1–P4)
- **P1 — Sound Select** → next pad = arm **Instrument I1–I16** (then return).
- **P2 — Pattern Select / Chain** → next pad = choose **Pattern A–P**; while holding **SHIFT**, tap more pads to **chain**; return on SHIFT release.
- **P3 — Clone Pattern → Slot** → next pad = **destination Pattern**; clones **events + mixer scene + macro lanes** (references same samples) and **auto‑queues** it; return.
- **P4 — FX / Repeat Pick** → one tap picks an **FX throw** (pads 1–4: **Stutter, Tape, Filter, Delay**) **or** a **Note‑Repeat** rate (pads 9–16: **¼, ⅛, ⅛T, 1/16, 1/16T, 1/32, 1/32T, 1/64**). **Hold SHIFT while selecting an FX = Latch** that FX; release returns.

### 3.2 Tools (sticky) — second row (P5–P8)
- **P5 — Copy** → tap **source**, then any number of **targets** to paste; **PLAY cancels**.
- **P6 — Cut** → as Copy, but **removes from source** upon first selection.
- **P7 — Delete** → taps erase at current **Scope** (see §5 “Scope”); **PLAY cancels**.
- **P8 — Nudge** → each tap nudges selected step(s) by **± preset microshift** (e.g., ±25%); **long‑press step** shows **fine slider**.

> Tools row is **sticky while SHIFT is held or latched**. Exiting SHIFT ends the tool.

### 3.3 Stamps (sticky; applied to new steps) — third row (P9–P12)
- **P9 — Accent** (toggle)
- **P10 — Ratchet** (×2 → ×3 → ×4 → off)
- **P11 — Tie** (toggle)
- **P12 — Micro** (−25% → 0 → +25%)

> Active stamps decorate **new** steps placed in STEP; they do not retro‑edit existing steps.

### 3.4 Other (quick section/feel) — bottom row (P13–P16)
- **P13 — Quantize (Apply)** → **one‑shot** apply at current strength; **long‑press** opens strength slider.
- **P14 — Swing Nudge** → **±1% per tap** (alternating +/−); **long‑press** opens slider.
- **P15 — Pattern Length** → cycles **16 → 32 → 48 → 64**; **long‑press** opens chooser; screen shows pages.
- **P16 — Scene Snapshot** → tap to **Save** current mixer scene to slot **A–D** (screen chips appear); then tap **A–D** to **Recall**.

---

## 4) Stamps (input decorators)
- **Accent** (on/off) • **Ratchet** (×2→×3→×4→off) • **Tie** (on/off) • **Micro** (−25%→0→+25%).
- Active stamps apply to **new steps** placed in STEP.
- Access: small footer chips (tap), or map **P3 press** to cycle stamps (optional firmware build).

---

## 5) Scope Resolution (what Tools operate on)
- **Default:** **STEP scope** (armed instrument’s lane in current pattern).
- If the last layer used was **INST** and it remains active (SHIFT latched) → Tools act on **instrument lanes**.
- If the last layer used was **PATT** and active → Tools act on **patterns** (whole‑pattern ops).
- **PLAY** commits tool action; **SHIFT** cancels and returns scope to default.

---

## 6) Encoders

### 6.1 E0 — Main encoder
| Context                      | Turn                          | Press                 | Long‑press            |
| ---------------------------- | ----------------------------- | --------------------- | --------------------- |
| **PAD/STEP (no focus)**     | Move step focus (±1/16)       | Toggle step (STEP)    | Open step editor      |
| **STEP (with focus)**       | Micro nudge (fine)            | Toggle step           | Open step editor      |
| **CHOP**                     | Next/prev transient           | Drop/move marker      | Zoom                  |
| **FX layer**                 | Adjust rate/wet of focused FX | Latch/unlatch FX      | Reset FX              |
| **PATT layer**               | Select/queue pattern          | Clone to selected     | Open chain editor     |

**Tap‑tempo:** via **SHIFT+PLAY** (preferred). E0 can optionally double as tap on the BPM chip.

### 6.2 E1–E3 — Performance encoders (assignable)
Same as prior spec (v1.1): **Performance Map** per encoder; up to 4 targets, depth/curve/pickup, automation write when WRITE is ON. Quick assign: **Hold P → tap pad (instrument) → E0 turn/press**.

---

## 7) Events & Data (Sequencer)
Unchanged from v1.1: per Sequence events by instrument; Scene; automation lanes; see §20.

---

## 8) FX & Note‑Repeat
- Punch‑ins as above; **latch by long‑press**.
- Note‑Repeat rates on pads 9–16 in FX layer; shows as a chip in PAD view.
- **Bake FX:** in FX layer, **WRITE long‑press** opens dialog (range, pre/post, normalize, target Replace/New). Printed audio spawns a new Instrument slot; original muted for Undo.

---

## 9) Sampling & CHOP Flow
1. **REC (tap)** from BASE (or **REC hold + I‑pad** in INST) starts capture.
2. **CHOP** opens with waveform + auto‑transients.
   - **E0 turn**: next/prev marker; **press**: drop/move marker; **long**: zoom.
   - Pads audition slices; **Assign 16** fills a **Sliced Kit** or remaps an existing kit.
3. Return to prior state (preserve SHIFT latch), armed to the new/updated slot.

Options: on‑device **Normalize/Time‑process** toggles before Assign.

---

## 10) Mixer & Scenes
Unchanged: 16 mini strips; Scene A–D (save/recall); Scene recall at pattern change unless Performance Lock is on.

---

## 11) Automation & Global LFO (optional)
Unchanged from v1.1 (automation lanes per Sequence; per‑instrument global LFOs targeting Macros). Evaluation order preserved.

---

## 12) GUI Architecture (screen feedback‑first)

### 12.1 Header (always visible, compact)
`▶/■  •  Pattern A–P  •  Tempo (tap)  •  Swing%  •  Quantize%  •  Write  •  CPU/voices  •  P1/P2/P3 labels`
- **Tap BPM/Swing/Quantize** → slider dialog; **PLAY=OK**, **SHIFT=Cancel**.
- **Tap a Perf label** → popover: targets, quick unassign, lock toggle.

### 12.2 Main views
- **STEP:** 4×4 grid (pages 1–4 if >16). Tap steps; long‑press opens mini editor (Length/Micro/Ratchet/Tie). Footer shows Stamps.
- **PAD:** Macro sliders, pad meters, armed instrument, Repeat rate chip.
- **FX:** 4 FX tiles, Rate/Wet knobs, **Bake** button.
- **MIX:** 16 faders + pan/send; Scene chips A–D.
- **SEQ:** Pattern tiles A–P; Song chain list; **Performance Lock** toggle.
- **CHOP:** Waveform, markers, **Assign 16**, Normalize/Time toggles; encoder tips.

### 12.3 Legends & overlays
- **Hold SHIFT** → **SHIFT Map legend** appears showing the four rows: **Palette (P1–P4)**, **Tools (P5–P8)**, **Stamps (P9–P12)**, **Other (P13–P16)**. Labels render within **120 ms**.
- When a **Palette** pad (P1–P4) is pressed, the UI shows the **next action hint** (e.g., “Select Instrument I1–I16…”, “Select Pattern A–P…”, “Pick FX or Repeat…”, “Pick destination Pattern…”). Return behavior: **one‑shot**, auto‑return after the selection is made (or when SHIFT is released for Chain).
- **Tools** row remains **sticky** while SHIFT is held (or latched). The active tool is highlighted; brief help text appears at the footer (“Copy: tap source, then targets; PLAY cancels”).
- **Stamps** row lights the active stamps; footer chips mirror the state.
- **Other** row actions open tiny sliders/choosers as needed (Quantize/Swing/Pattern Length), and the **Scene A–D** chips appear above the mixer for snapshot save/recall.

---

## 13) LEDs & Feedback
- **SHIFT:** dim idle → bright while held → **solid** when latched (double‑tap)
- **Pads:** palette tint differs for PAD vs STEP; Accent brighter; Ratchet double‑blink; Tie underline; Micro tick.
- **Encoders:** halo tick optional; P‑encoders blink while in assign‑hold.

---

## 14) Defaults & Persistence
Startup: Pattern A, Instrument I1 armed, **WRITE OFF**, 120 BPM, Swing 55%, Quantize 50%. Per‑instrument engine/patch/macros/scale stored; per‑Pattern events/scene/automation stored.

---

## 15) Acceptance Criteria (functional)
**Transport & chords**
- **PLAY** toggles transport; **REC** starts capture then opens **CHOP**; **WRITE** toggles write/overdub.
- **SHIFT+PLAY** performs tap‑tempo; **SHIFT+WRITE** toggles **Pad‑Play ↔ Step‑Edit**.
- **Latency**: pad→sound ≤ **7 ms**; LED ≤ 10 ms; legends/overlays ≤ 120 ms.

**SHIFT Map**
- **Palette row (P1–P4)** works as one‑shots:
  - **P1 Sound Select** → next pad arms **I1–I16** then returns.
  - **P2 Pattern Select/Chain** → first pad selects **A–P**; while **SHIFT held**, additional taps **chain**; releasing SHIFT exits. Queues at bar if playing.
  - **P3 Clone Pattern→Slot** → next pad clones **events + mixer scene + macro lanes** (same samples) to destination **A–P**, then **auto‑queues**.
  - **P4 FX/Repeat Pick** → picks **FX (1–4)** or **Repeat (9–16)**. **Holding SHIFT while selecting an FX latches** it; releasing SHIFT exits.

- **Tools row (P5–P8)** is sticky while **SHIFT** is held/latched:
  - **Copy**: tap **source**, then any number of **targets**; **PLAY cancels**.
  - **Cut**: as Copy, but removes from source on first selection.
  - **Delete**: taps erase at current **Scope**; **PLAY cancels**.
  - **Nudge**: taps nudge by preset micro; **long‑press step** opens fine slider.

- **Stamps row (P9–P12)** toggles decorators applied to **new** steps only: **Accent**, **Ratchet ×2/×3/×4/off**, **Tie**, **Micro −25%/0/+25%**.

- **Other row (P13–P16)**:
  - **Quantize (Apply)** one‑shot; long‑press opens strength slider.
  - **Swing Nudge** ±1% per tap (alternating ±); long‑press opens slider.
  - **Pattern Length** cycles 16/32/48/64; long‑press opens chooser; UI paginates STEP.
  - **Scene Snapshot**: tap to **save** to A–D (chips appear), then tap an A–D chip to **recall**.

**INST / PATT behaviors**
- **Sound Select** arms instruments immediately; velocity/AT preview when pads are struck in PAD‑PLAY mode.
- **Pattern Select/Chain** selects/queues patterns; chain plays through; **Performance Lock** respected.

**FX/Repeat**
- FX throws are **momentary** unless **latched** (by holding SHIFT while picking the FX). **E0 turn** adjusts rate/wet for focused FX; **WRITE long‑press** opens **Bake**.

**STEP**
- Tap toggles step; long‑press opens mini editor (Length/Micro/Ratchet/Tie). Stamps apply to new placements.

**Encoders**
- **E0** context bindings and **E1–E3** performance assignment flow as specified; WRITE records automation where applicable.

## 16) Icons & Color (silkscreen/legend)
- **Top‑row SHIFT legend** (when SHIFT is held): Pad1 **INST** (wave/key), Pad2 **PATT** (grid), Pad3 **FX** (spark/star), Pad4 **TOOLS** (scissors/doc). Remaining pads show layer‑specific labels.
- **Stamps** (green): Accent ›, Ratchet ≡×n, Tie slur, Micro ± clock.
- **Other chips**: Quantize magnet/grid, Swing pendulum, Length bar‑stack, Scene camera/mixer.

---

## 17) Appendix — State Chart (transport‑4)
```
States: BASE (PAD), STEP, SHIFT_MAP, CHOP, DIALOG

PLAY          -> PlayToggle
REC tap       -> StartCapture -> CHOP -> Assign -> return
WRITE tap     -> WriteToggle
SHIFT+PLAY    -> TapTempo
SHIFT+WRITE   -> ModeToggle (BASE<->STEP)

SHIFT down    -> SHIFT_MAP (legend visible)

// SHIFT Map rows
P1 (Palette)  -> SoundSelect   -> waitNextPad(I1..I16) -> arm -> return
P2 (Palette)  -> PattSelect    -> waitNextPad(A..P) -> select; while SHIFT held, additional taps chain; release -> return
P3 (Palette)  -> CloneToSlot   -> waitNextPad(A..P) -> clone(events+scene+macros refs) -> autoQueue -> return
P4 (Palette)  -> FxOrRepeat    -> tap FX (1..4) or Repeat (9..16); if SHIFT held during FX tap -> latch

P5 (Tools)    -> Copy (sticky while SHIFT) : tap source -> tap targets (multi); PLAY cancels
P6 (Tools)    -> Cut  (sticky while SHIFT) : tap source -> tap targets; removes from source on first selection; PLAY cancels
P7 (Tools)    -> Delete (sticky while SHIFT) : taps erase at Scope; PLAY cancels
P8 (Tools)    -> Nudge (sticky while SHIFT) : ± preset; long-press step -> fine slider

P9..P12       -> Stamps toggles (sticky): Accent / Ratchet / Tie / Micro
P13           -> Quantize Apply (oneshot) ; long-press -> strength slider
P14           -> Swing Nudge ±1% ; long-press -> slider
P15           -> Pattern Length 16/32/48/64 ; long-press -> chooser
P16           -> Scene Snapshot: save to A-D; A-D chips -> recall

Dialogs: PLAY = Yes; SHIFT = No (unless Tool row says PLAY cancels)
```

## 20) Song Architecture & Data Model (Developer Spec)
(unchanged from v1.1; see previous section headings for entities, timing, storage, and test matrix)

---

## 21) Screens & GUI (Developer Spec — compact 720×720)
**Target reference:** **720×720** square (≈72 mm), ultra‑compact.
- **Header**: 36 px; **Footer**: 30 px; **Mode rail**: n/a (layers via SHIFT legend)
- **Main pane**: fills remainder; grid gaps ~6 px; **STEP cells** ~80×80 px; **PAD mini‑pads** (KIT) ~54×54 px; **Knobs** 60 px box
- **Typography**: base 10 px; small 9 px; numbers mono 10 px; touch targets ≥ 40×40 px; hold=400–600 ms

**Acceptance (screens)**
- Legends appear <120 ms when SHIFT pressed; vanish <150 ms after release.
- Header chips: tapping BPM/Swing/Quant opens dialog; **PLAY=OK**, **SHIFT=Cancel**.
- CHOP zoom/pan responsive (<16 ms); Assign returns to prior view and preserves SHIFT latch.

---

**End of v1.3 (transport‑4).** This document replaces the 1×5 function‑key variant and is implementation‑ready. Any remaining references to dedicated layer buttons must be replaced by the SHIFT‑Chord activations defined above.

