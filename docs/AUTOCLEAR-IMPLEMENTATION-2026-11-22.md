# Depthorator v1.2.0 — Auto Clear / Sidechain-inspired Wet Duck

**Engineering candidate only; NOT a released 125A FULL build.**
The approved 1.1.0 remains untouched.

## Contract agreed with the owner

- One **AUTO CLEAR ON/OFF** switch in the original 1000×500 editor.
- OFF is the original V1.1 Delay/Reverb engine. No DSP modifications to
  original delay buffer, feedback filter, reverb, timing or topology.
- ON must render a **single first note identically** to OFF, across its
  entire delay/reverb tail, not just the first few milliseconds.
- A distinct later dry-input onset (after a brief gap) ducks ONLY the
  existing wet output by ~12 dB, in 8 ms, holds for 65 ms, then returns
  to unity smoothly over 250 ms. The dry input and delay/reverb internal
  signals are not cleared.
- This is explicitly **ducking, not the original QDelay hard-clear
  operation**. QDelay upstream is a behavior reference and does not
  guarantee first-note immunity. Hence the detector is changed
  intentionally to honor the owner's observed desired result.
- Input-audio-only signal detection cannot perfectly identify overlapping
  MIDI notes, as this audio effect has no MIDI note-on input. Use silence-gap
  and attack conditions to reduce false retriggering.
- Existing VST3 processor and controller IDs, earlier 12-parameter
  state, and version 1.2.0 packaging remain stable. Auto Clear appends
  one parameter, ID 112, with OFF as default.

## Native tests added

- `tests/AutoClearCoreTests.cpp`: three sample rates, first-note
  protection including a sustained note, one later note with duck/release,
  unity bypass and finite invalid-input handling.
- `tests/DepthoratorNativeAudioTests.cpp`: instantiates the actual
  full Depthorator audio processor and exercises 32-bit / 64-bit
  processing across complete note-and-echo fixtures, asserting
  sample-exact OFF/ON first-note parity and wet attenuation/recovery after
  the second note. Builds the same processor/reverb implementation as
  the VST3.
- `tests/verify_autoclear_layout.py`: one ON/OFF switch, no extra controls
  or collisions.

**The build and QA run are authoritative only after CI results pass.**
The fixture is not identical to the owner's original musical FLAC,
which contains an audible OFF/ON difference even before note two.
Independent DAW listening and actual VST3 hosted PCM tests remain pending.
