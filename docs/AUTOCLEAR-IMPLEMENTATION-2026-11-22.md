# Depthorator Auto Clear — isolated development branch

Scope corrected 2026-11-22 at the owner's explicit request:
**Auto Clear is one ON/OFF switch. No Amount, Threshold, Hold or Fade
controls are exposed.** The editor is restored to its original 1000x500
footprint; the switch sits aligned beneath the left-hand routing controls.

## Implementation and provenance

- Product: Depthorator FULL, branch `feature/autoclear-qdelay-20261122`.
- Derived from owner's approved 1.1.0 GOLD source commit
  `b0e6d5b706458ce232d5b8527769faaeef427dd7`.
- Source reference: `coolvegan/qdelay-autoclear` at
  `5ad0ba70a998d561d9c46d55301a1f66c2284259`. Reference algorithm
  behavior is studied; no GPL source copied into commercial Depthorator.
- Audio-onset detection uses fast/slow envelope comparison and a refractory
  interval. ON enables a full clear at a fixed internal threshold of -30 dBFS,
  a fixed fade-in time of 40 ms and a brief ≤12 ms fade-out.
- The gain/fade and old-tail invalidation are realtime bounded. The write
  path suppresses old feedback during the fade. Old delay and reverb memory
  is logically invalidated without clearing giant buffers per note-on.
- OFF keeps the existing dry/wet audio path unaffected by the new wet
  gain controller.
- One automatable VST3 parameter ID is appended: `kAutoClear` (112);
  default OFF. All 12 existing IDs remain unchanged.
- The extension's state contains 13 doubles; 12-double V1.1 states
  initialize Auto Clear as OFF.
- Parameters to configure amount, threshold or fade are **not exported**.
  Internal tests may exercise partial duck paths, but the user cannot
  select these modes on this plugin.

## Verification

The branch-specific Windows CI builds the executable and Windows x64 VST3,
runs a deterministic core regression, verifies the one-switch/no-overlap
GUI layout contract, builds and runs the official Steinberg validator,
and checks the staged VST3 bundle and version metadata.

These tests do not by themselves demonstrate real Fender Studio Pro use,
1.2 project recall or full-chain audio-tail behavior. Until separately
measured, those remain NOT VERIFIED. Keep the approved 1.1.0 FULL untouched.
