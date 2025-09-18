# Grid Sequencer (C++/Terminal) — Quick Guide

This file outlines how to use the terminal grid sequencer UI and how FX selection works after the recent menu fix.

- Launch: build and run `grid_sequencer.cpp` (uses PortAudio + liblo).
- Screen: fixed terminal UI shows engine params, plus extra rows for voices and FX.
- Navigation: Up/Down arrows or `k/j` change selection. Left/Right arrows adjust values. Space toggles Play. `w` toggles Write.

Menu structure (bottom section after engine parameters):
- voices — engine voice count
- rev_send — per‑engine reverb send (FX bus A)
- del_send — per‑engine delay send (FX bus B)
- rvb_size — global reverb decay/time
- rvb_damp — global reverb damping
- rvb_mix — global reverb wet mix
- dly_time — global delay time
- dly_fb — global delay feedback
- dly_mix — global delay wet mix

Selection fix (what changed):
- The selection clamp now accounts for all “extra” rows using a single helper (`extraMenuRows()`), so you can select and edit reverb/delay items reliably.
- Arrow key and `j/k` navigation use the same bound, preventing drift when items are added later.

Known notes:
- Drum engines add one extra editable row (drum pad editor) below the parameter list.
- Voices currently point to slot 0 in code; adjust if you need per‑slot control.

LFO controls (keyboard):
- Select LFO: `[` / `]` cycles 1–8.
- Waveform: `v` cycles common shapes; `S` toggles a small settings pane.
- Rate: `r`/`R` decreases/increases Hz.
- Depth: `d`/`D` decreases/increases modulation depth.
- Assign menu: `L` opens an 8‑slot checkbox row; move with arrows; toggle with `x` or Enter. Multiple LFOs can be latched to one parameter.
- Quick assign/remove: `m`/`M` toggle the currently selected LFO for the current parameter.
- Sync modes: `k` sets Key‑sync; `e` sets Envelope mode (envelope‑shaped LFO). Use normal note triggers to latch.

Function buttons (to be configured later):
- shift, engine, pattern, FX, LFO, play, write, copy, delete
