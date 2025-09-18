# Post‑Chain Tuning Guide (Core Param Fallbacks)

This guide defines the exact mappings and ranges used to guarantee that the core 16 parameters audibly affect every engine via the per‑slot post‑chain in the harmonized bridge. The post‑chain runs for all engines, so these mappings are always effective, regardless of engine internals.

Core mappings
- HARMONICS → HPF tilt
  - Map: 0.0 → 10 Hz, 1.0 → 600 Hz (log‑like control via exponential map)
  - Curve: f = 10.0 * pow(2, norm * 5.9)
  - Notes: Emulates “thickness → thinness” tilt; keep Q broad (one‑pole).

- TIMBRE → LPF cutoff (brightness)
  - Map: 0.0 → 300 Hz, 1.0 → ~18 kHz (exponential)
  - Curve: f = 300.0 * pow(2, norm * 6.5)
  - Notes: Primary brightness control; engines with their own bright control should add, not replace.

- MORPH → LPF resonance (focus)
  - Map: 0.0 → Q 0.5, 1.0 → Q 10.0 (linear in Q domain)
  - Curve: Q = 0.5 + norm * 9.5
  - Notes: Adds focus/emphasis; avoid self‑oscillation.

- AMPLITUDE → pre‑gain (+ headroom)
  - Map: 0.0 → 0 dB, 1.0 → +6 dB
  - Curve: gain = 1.0 + norm
  - Notes: Applied pre‑filters; helps loudness sweeps and RMS tests.

- CLIP → tanh drive amount
  - Map: 0.0 → clean, 1.0 → strong soft clip
  - Curve: y = tanh((1 + 5*norm) * x)
  - Notes: Keep post‑clip LPF/HPF active to tame harmonics.

- PAN → equal‑power panning
  - Map: −1.0..+1.0 → angle 0..π/2
  - Gains: gL = cos(θ), gR = sin(θ) where θ = (pan+1)*π/4
  - Notes: Apply before global LPF for consistent tone.

- FILTER_CUTOFF / FILTER_RESONANCE / HPF
  - Cutoff: same LPF curve as TIMBRE; RES maps to Q (0.5..10.0)
  - HPF: same HARMONICS curve (10..600 Hz)

Ranges and defaults
- Defaults: TIMBRE ~0.7, MORPH ~0.2, AMPLITUDE ~0.5, CLIP ~0.0, PAN 0.0, HPF 0.0
- Clamp behaviors: All mapped values clamped conservatively (HPF ≤ sampleRate*0.45, LPF ≤ sampleRate*0.45, Q ≤ 10.0)
- Stability: One‑pole HPF + RBJ LPF; coefficient updates once per block.

Implementation hints
- Order: pre‑gain → drive → pan → HPF → LPF (current chain)
- Avoid double brightness: If engine also brightens internally, keep TIMBRE ≤0.8 or damp drive tone post‑clip.
- RMS deltas: For test sweeps, use 3 segments with ±15–30% expected RMS movement.

