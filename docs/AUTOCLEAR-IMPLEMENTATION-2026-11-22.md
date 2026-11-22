# Depthorator v1.2.0 — QDelay-style Auto Clear

Status: **Development candidate / not yet approved as 125A FULL**.
The owner-approved 1.1.0 remains untouched.

## Owner's intended behavior

Exactly ONE additional GUI and automation control, **AUTO CLEAR OFF/ON**.
There are no Amount, Threshold, Fade, Hold, or other new visible controls.

- **OFF:** Original delay/reverb behavior, including old tails.
- **ON, first note:** Normal delay and reverb, sample-for-sample identical
  to OFF for the entire note and its subsequent tail. The first input
  attack merely arms the onset detector.
- **ON, second distinct note:** As a new input attack arrives, fade the
  existing wet signal to zero in 12 ms, invalidate OLD delay and reverb
  tail memory, then fade the wet signal back over 40 ms. Preserve input
  from the new note in the delay buffer so its own repeats can play.
  Never mute the dry input signal.
- **Not equivalent to ordinary ducking:** After a full clear the old
  delay feedback is gone, not restored when wet gain reaches unity.
  The short fade envelope is only used to avoid pops at the clear.
- Gate at -30 dBFS, 22 ms rearm, 90 ms refractory, audio input only,
  all hard-coded internal values; no extra parameters.
- As an insert FX without an external MIDI note-on bus, reliably
  identifying *overlapping* notes without an input gap is inherently
  limited. Do not claim full polyphonic MIDI note detection.

## Implementation

QDelay `coolvegan/qdelay-autoclear` was used as behavioral reference;
its algorithm blends down, clears delayed memory, and blends back.
The Depthorator implementation is separately authored C++ for its
existing stereo ring buffer and reverb; no upstream GPL code was copied.

- Processor and controller FUIDs and original parameter IDs remain unchanged.
- 1.1.0 preset state (12 doubles) loads with Auto Clear OFF.
- 1.2.0 state (13 doubles) includes the single Auto Clear switch at ID 112.
- Legacy delay/reverb coefficients, feedback, and dry output are unchanged
  before Auto Clear triggers.
- Delay samples have generation/timestamp validity checks; old feedback
  is suspended during fade-out, and old buffer history is invalidated in
  O(1) at the clear point, without allocating in the audio callback.
- Reverb memories have generations, allowing a real-time clear without
  iterating multi-thousand-element buffers in the audio callback.

## QA gates

- C++ AutoClearCore tests: first note, sustained first note, second-note
  trigger exactly once, clear exactly once, smooth fade and recovery,
  OFF unity, invalid-input.
- Full Depthorator processor native audio test (32-bit, 44.1 kHz and
  64-bit, 48 kHz): first-note OFF/ON bit-identical through the whole
  original echo tail, second-note difference due to old-tail clearance,
  and new note audible at its first delayed repeat using a solo-note
  reference processor.
- One-switch GUI geometry and internal-parameter surface regression.
- Windows VST3 packaging/Steinberg Validator on exact staged canonical
  125A-named binary and its unzipped shipping form. Plugin `moduleinfo`
  must be syntactically valid JSON.
- Independent Fender Studio Pro listening, full 125A Plugin Tester and
  release sign-off remain separate requirements.

CI results must be confirmed at the exact commit before any PASS claim.
