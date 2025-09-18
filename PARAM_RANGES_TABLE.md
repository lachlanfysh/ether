# Core Parameter Ranges & Scales

Use these ranges/scales for UI, tests, and fallback mapping. All values are normalized 0..1 in the public API and mapped to engineering units here.

- HARMONICS: 0..1 (maps to HPF cutoff)
  - Curve: exponential; 10 Hz → 600 Hz
- TIMBRE: 0..1 (maps to LPF cutoff)
  - Curve: exponential; 300 Hz → ~18 kHz
- MORPH: 0..1 (maps to LPF Q)
  - Curve: linear in Q; 0.5 → 10.0
- ATTACK: 0..1
  - Curve: exponential; 0.5 ms → 2.0 s (engine‑internal if available)
- DECAY: 0..1
  - Curve: exponential; 2 ms → 4.0 s
- SUSTAIN: 0..1
  - Curve: linear; 0..1
- RELEASE: 0..1
  - Curve: exponential; 2 ms → 6.0 s
- FILTER_CUTOFF: 0..1 (LPF cutoff)
  - Curve: exponential; 300 Hz → ~18 kHz
- FILTER_RESONANCE: 0..1 (LPF Q)
  - Curve: linear; 0.5 → 10.0
- HPF: 0..1 (HPF cutoff)
  - Curve: exponential; 10 Hz → 600 Hz
- VOLUME: 0..1 (engine volume + post pre‑gain assist)
  - Curve: linear; 0 → 1 (+ mirror to pre‑gain)
- PAN: −1..+1 (equal‑power)
  - Curve: angle θ = (pan+1)*π/4; gL=cosθ, gR=sinθ
- AMPLITUDE: 0..1 (post pre‑gain)
  - Curve: linear; 1.0 → 2.0 (0 dB → +6 dB)
- CLIP: 0..1 (tanh drive)
  - Curve: y = tanh((1 + 5*norm) * x)
- ACCENT_AMOUNT: 0..1 (performance)
  - Curve: linear; recommended 0.75 default
- GLIDE_TIME: 0..1 (performance)
  - Curve: exponential; 0 → 250 ms

Notes
- Clamp all filter frequencies to ≤ sampleRate*0.45; keep Q ≤ 10.0.
- For tests, compute RMS across three segments of a sweep and assert a ≥15% delta.

