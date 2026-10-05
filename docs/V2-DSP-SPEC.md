# Depthorator V2 DSP specification

Status: development specification for branch `v2.0.0`.

## Product intent

Depthorator V2 keeps the compact V1 workflow but makes the core promise more literal:
successive repeats should move progressively farther behind the source instead of receiving only one
global depth coloration.

V1.0.0 is the immutable compatibility baseline. V2 has a distinct VST3 component identity and bundle
name so both versions can be installed side by side.

## Evidence basis

Auditory distance is multi-cue. V2 therefore couples:
- direct-to-reverberant energy relationship;
- repeat/direct energy;
- spectral detail;
- temporal/transient definition;
- stereo coherence where appropriate.

This is explicitly **not** a geometric room or physical distance simulator. The target is a cool,
musically useful depth effect whose behaviour borrows credible perceptual cues. The user-facing macro
mapping is EMPIRICALLY TUNED for musical impact, constrained by measured stability and predictable
control behaviour rather than realism.

## Control invariants

- No additional front-panel controls are required for the initial V2 revision.
- DEPTH controls the intended maximum front-to-back effect strength. 20–50% is the primary musical working range, 50–75% clearly audible, and 75–100% intentionally strong/creative.
- CURVE controls how quickly successive feedback circulations approach that displacement; it must
  not merely behave as a second DEPTH amount control.
- WIDTH = 0% collapses the complete wet output (echo + room) to mono.
- WIDTH = 100% preserves the internally generated wet stereo image.
- MIX remains equal-power dry/wet.
- Delay-time changes remain click-safe without pitch-slew artefacts.

## Required measurable behaviour

Before release:

1. With DEPTH > 0, later repeats must show a monotonic reduction in defined directness metrics versus
   earlier repeats. At minimum measure high-band energy and direct/reverberant energy.
2. At fixed DEPTH, CURVE must measurably change progression rate while substantially preserving the
   intended final depth character.
3. WIDTH = 0% must leave numerically negligible wet-side energy after the complete wet chain.
4. Rapid automation must not create avoidable clicks, invalid samples or block-size-dependent jumps.
5. Feedback must remain bounded at all valid settings over supported sample rates and block sizes.
6. 32-/64-bit processing, realtime/offline, state recall and editor lifecycle must regress cleanly.
7. Real-audio fixtures must include transient material, guitar/synth/vocal material and a dense mix.

## Engineering sequence

1. Side-by-side V2 identity/versioning.
2. Correct full-wet WIDTH semantics and lifecycle compliance.
3. Decouple DEPTH amount from CURVE progression rate in the recursive feedback path.
4. Add automation-safe sample-offset handling/smoothing.
5. Measure repeat-by-repeat depth metrics and tune from measurements.
6. Improve FDN density/decorrelation only if measured/listening evidence shows a remaining limitation.
7. Run the full 125A release QA only after the DSP acceptance tests pass.
