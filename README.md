# 125A Depthorator 1.2.0

**Each repeat goes deeper.**

Depthorator is a 125A VST3 spatial delay/reverb effect for Windows. Instead of keeping every delay repeat at the same perceived distance, Depthorator progressively moves the repeating signal deeper into a darker, softer and more diffuse room field.

The interface is intentionally compact: delay, depth progression, internal algorithmic reverb, stereo behaviour and ducking are coupled into one focused workflow.

## Controls

### DELAY
- **TIME** - delay time. In SYNC mode it selects musical note divisions; in FREE mode it runs from 20 to 2000 ms.
- **FEEDBACK** - amount and persistence of delay repeats.
- **SYNC / FREE** - tempo-synchronised or free delay timing.

SYNC divisions:
`1/32`, `1/32 T`, `1/32 D`, `1/16`, `1/16 T`, `1/16 D`, `1/8`, `1/8 T`, `1/8 D`, `1/4`, `1/4 T`, `1/4 D`, `1/2`, `1/2 T`, `1/2 D`, `1/1`.

### DEPTH
- **DEPTH** - main front-to-back control. Higher values make the repeating signal increasingly absorbed into the room field.
- **CURVE** - controls how quickly the repeats develop into depth.

### SPACE
- **SIZE** - perceived room size and internal spatial scale.
- **DECAY** - reverb decay time, approximately 0.3 to 12 seconds.
- **DAMPING** - high-frequency absorption/darkness of the room.

### IMAGE / CONTROL
- **WIDTH** - processed stereo width: 0% collapses the wet side component to mono; 100% preserves normal stereo width.
- **DUCK** - reduces the processed signal around incoming transients so the direct sound stays clear and the delay/reverb can bloom behind it.
- **MONO / STEREO / PING-PONG** - delay routing mode.

### AUTO CLEAR
- **AUTO CLEAR ON/OFF** — when a clearly separated new input note arrives, the preceding delay/reverb tail fades out, is cleared, and the new note builds its own repeats. The first note is unchanged; there are no extra controls. Detection uses audio transients and a brief input gap, not MIDI note events.

### OUTPUT
- **MIX** - equal-power dry/wet balance.

## GUI

The editor offers **100%** and **150%** zoom. The selected zoom level is stored with the plug-in state.

## Technical characteristics

- VST3
- Windows x64
- Stereo input / stereo output
- 32-bit and 64-bit audio processing
- Zero reported processing latency
- Tempo sync plus free-time operation
- Integrated algorithmic stereo reverb
- Smooth, peak-safe delay-time transitions
- Parameter/state persistence

## Version 1.2.0

New optional **AUTO CLEAR OFF/ON**, implemented for incoming audio.
The first sound and its original delay/reverb tail are left unchanged.
On a clearly detected later input note, the old tail is smoothly
suppressed and removed while the new note is retained and generates
its own delay repeats. This is a full tail clear, not an ordinary
temporary sidechain duck.

All previous delay, depth, space, routing and mix controls remain
available with their existing defaults. Previously saved V1.1 projects
load with AUTO CLEAR OFF. The component/controller VST3 IDs are preserved.

### Development QA evidence

- Windows x64 VST3 build and Steinberg validator 47/47 completed.
- Deterministic audio tests cover first-note OFF/ON parity, second-note
  old-tail removal and audible new-note repeats.
- The complete independent 125A Plugin Tester and physical DAW/practical
  listening gates must be recorded separately before any FINAL release.
- This documentation does not claim unexecuted Studio Pro tests.

## Notes

Depthorator intentionally has no plug-in-owned bypass parameter; use the host bypass when required. The plug-in is designed as a stereo effect and does not advertise a mono bus layout.

Factory presets are not included. The plug-in is intended to be quick to dial in directly from its controls.

## Installation

Copy `125A Depthorator.vst3` to a VST3 plug-in location used by your host, typically:

`C:\Program Files\Common Files\VST3\`

Then rescan VST3 plug-ins in your DAW if necessary.

## Documentation

The Windows package provides installation instructions in German and English. The V1.2 Auto Clear behavior is documented above.

## License

Depthorator is commercial software. Use is subject to the license included with the purchased release package. Redistribution or resale of the plug-in is not permitted.

## Project

125A / AUDIO SOFTWARE
