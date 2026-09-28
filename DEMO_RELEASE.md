# Demo Release Branch

Product version: **1.0.0**

Baseline: `main` from the authoritative Final repository.

This `Demo` branch implements the current 125A Demo / Licensing Standard.

- One compiled VST3 binary is used for Demo and Full.
- Demo package: no valid Full license resource.
- Full package: same binary plus `125A_Depthorator.license`.
- License identity is product-specific.
- Demo behavior: audio: 60 s normal / 3 s interruption / 10 ms fades.
- Full mode bypasses the demo restriction.
- Stable release branches remain unchanged.

Build/QA evidence must come from the Demo-branch workflow before artifacts are treated as publishable.
