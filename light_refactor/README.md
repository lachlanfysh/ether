Light Refactor Scaffolding
==========================

This folder contains minimal, header-only facades to introduce clean seams into
the existing monolithic grid sequencer without changing runtime behavior.

Goals
- Zero allocation or locking in hot paths.
- No dependencies on OSC/PortAudio inside facades.
- Simple, explicit composition (no DI container).

Components
- EngineBridge.h: Thin wrappers over the `ether_*` C bridge calls.
- GridLEDManager.h: Local LED state buffer + batched flush API.
- ParameterCache.h: Atomic read-mostly parameter cache for UI.
- AppContext.h: Simple struct for wiring components together.

These files are not yet wired into the build; integrate gradually by including
the headers in the monolith and replacing call sites incrementally.

