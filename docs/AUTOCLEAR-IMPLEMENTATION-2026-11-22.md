# Depthorator 1.2 Auto Clear — isolated engineering candidate

Status at source creation: **IMPLEMENTED / WINDOWS TEST NOT YET RUN**. Do not
represent this branch as the owner-approved FULL, a Gumroad release, a completed
125A QA audit, or a Studio One-tested binary.

## Provenance and baseline

- Destination: `challanger2000/Depthorator-Final`
- Isolated source: `feature/autoclear-qdelay-20261122`
- Exact development base: `RELEASE-v1.1.0-GOLD`, commit
  `b0e6d5b706458ce232d5b8527769faaeef427dd7`.
- This commit is the verified matching-version GitHub Gumroad package-workflow
  source for run 37402944687. The owner's local customer ZIP and its exact
  binary/source match have **not** been independently reverified.
- Approved FULL 1.1.0 remains unchanged.
- Behavioral reference: `coolvegan/qdelay-autoclear`, commit
  `5ad0ba70a998d561d9c46d55301a1f66c2284259`,
  `src/dsp/AutoClear.h` and `src/PluginProcessor.cpp`. That project is GPLv3.
  No JUCE/GPL source is included in Depthorator; the independently written
  `src/AutoClearCore.h` reproduces the observed public behavioral sequence.

## Functional design

This feature is audio-triggered only: unlike QDelay's desktop MIDI mode,
Depthorator has no MIDI input bus. Scope excludes modifying the existing
mix, delay time, feedback law, reverb character, plugin class IDs, or
released package.

- Enable (OFF by default): disables trigger/gain processing exactly.
- Amount (0–100%, default 100%): partial duck at intermediate values;
  100% causes a full prior-tail invalidation.
- Threshold (-60 to 0 dBFS, default -30 dBFS): audio-onset detection.
- Fade (2–500 ms, default 40 ms): fade-in duration, with the full-kill
  fade-out limited to 12 ms. Feedback writes are muted during full-kill fade-out; only frames written
  since the new onset survive, preventing old feedback from re-entering the
  cleared loop while retaining the new attack.
- A dual-timescale onset detector (fast 1/50 ms and slow 60/400 ms), with a
  90-ms refractory interval, is the reviewed reference behavior.
- 100% clear does not allocate, lock, or traverse long buffers in the callback:
  a ring-write frame cutoff invalidates old stereo delay data and a generation
  counter invalidates the reverb FDN/diffusion memory.
- The wet-only gain multiplies the existing Depthorator duck gain.
  The dry path is never ducked by Auto Clear.
- V1.1's 12 stored values are decoded with Auto Clear disabled. Four new
  values are appended, with no class-ID or previous parameter-ID changes.

## Traceability and outstanding gates

| Contract | Evidence method | Status until Windows results |
|---|---|---|
| Disabled/0 amount is exactly 1.0 gain | Always-active C++ core test | NOT RUN |
| New onset causes single full kill | Always-active C++ core test | NOT RUN |
| Amount 50% ducks without hard clear | Always-active C++ core test | NOT RUN |
| Initial wet attack data preserved | DSP timestamp cutoff logic; packaged PCM A/B needed | NOT VERIFIED |
| Full old delay/reverb tail disappears | Timestamp/generation state design; real VST3 PCM A/B needed | NOT VERIFIED |
| 44.1/48/96 kHz and 32/64 sample processing | Native hosted VST3 matrix needed | NOT RUN |
| VST3 project save/reopen, automation, malformed-state rejection | Native host/DAW test needed | NOT RUN |
| GUI clicks, type-in, reset, 100/150% rendering | Native editor/Studio One test needed | NOT RUN |
| CPU deadlines, no callback allocation | Paced live Windows QA and instrumentation needed | NOT RUN |
| Official Steinberg VST3 Validator | Windows workflow | NOT RUN |
| 125A Plugin Tester and Fender Studio Pro | Owner/native QA | NOT RUN |
| Independent real-audio level-matched listening | Owner/audio fixtures | NOT RUN |

The Windows workflow is limited to the development branch, protects the
approved FULL and stages an installable VST3 directory, not a customer ZIP.
