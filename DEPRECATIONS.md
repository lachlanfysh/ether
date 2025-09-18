# Deprecations for Grid Sequencer Focus

The grid sequencer (`grid_sequencer.cpp`) is the active, supported path for running EtherSynth in this workspace. The following assets are not required for this path and are considered deprecated for the purposes of the C++ grid build. They are retained for reference and future UI work but should not affect the grid build.

Deprecated (not used by grid_sequencer.cpp build):
- `TestApp/` — SwiftUI test app scaffolding.
- `EtherSynth/EtherSynth.swift` and related Swift sources — macOS/iOS SwiftUI implementation.
- `GUI/` HTML/JS prototypes — design prototypes for UI flows.

Recommendations:
- Exclude the above paths from your C++ build system or project files.
- Avoid editing these while iterating on the grid sequencer unless you plan to revive the Swift/HTML UIs.
- If desired, these can be archived or removed in a future cleanup PR.

