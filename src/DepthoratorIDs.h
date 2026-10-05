#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace Depthorator {

// V2 has a distinct component identity so V1.0.0 and V2 can coexist.
// Parameter IDs remain stable inside the V2 product line.
static const Steinberg::FUID kProcessorUID(0x4D8BCE21, 0xA1F6478E, 0xB35C29D4, 0x7E9206AF);
static const Steinberg::FUID kControllerUID(0x91A57CD3, 0x2F4B4E68, 0xA6D18B30, 0xC54FE729);

static constexpr Steinberg::int32 kProcessorStateMagic = 0x44503253; // "DP2S"
static constexpr Steinberg::int32 kProcessorStateVersion = 1;
static constexpr Steinberg::int32 kControllerStateMagic = 0x44503255; // "DP2U"
static constexpr Steinberg::int32 kControllerStateVersion = 1;

enum ParamID : Steinberg::Vst::ParamID {
    kTime = 100,
    kFeedback,
    kDepth,
    kCurve,
    kSize,
    kDecay,
    kDamping,
    kWidth,
    kDuck,
    kMix,
    kSync,
    kMode
};

} // namespace Depthorator
