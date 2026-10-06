# Demo Release Branch

Product version: **1.1.0**

Authoritative full-release baseline:
- branch: `RELEASE-v1.1.0-GOLD`
- commit: `b0e6d5b706458ce232d5b8527769faaeef427dd7`

This Demo implementation follows the current 125A Demo / Licensing Standard.

- One compiled VST3 binary supports Demo/Full authorization.
- Gumroad Demo package contains no valid Full license resource.
- Product-specific Depthorator license identity is retained for Full-mode verification.
- Demo behavior: 60 s normal processing / 3 s interruption / 10 ms fades.
- Demo gating is sample-count based and applied after the v1.1.0 processing path.
- v1.1.0 sample-accurate automation, denormal protection and DSP hardening remain intact.
- Full mode bypasses the DemoGate.

Verified CI evidence:
- workflow run: 37404231838
- tested source commit: `3a259f5b3de6544e16d072d9bc38bd7280f1308b`
- V1 character regression: PASS
- Windows x64 Release build: PASS
- 125A Plugin Tester: PASS
- Demo package license-absence check: PASS
- Gumroad Demo artifact assembly: PASS

The former v1.0.0 Demo branch state is preserved at `archive/Demo-v1.0.0`.
