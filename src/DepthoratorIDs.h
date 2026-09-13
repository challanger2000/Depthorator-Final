#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace Depthorator {

static const Steinberg::FUID kProcessorUID(0x7C4A2C11, 0x51E34CB1, 0x9F72A2F4, 0x3A7E1C10);
static const Steinberg::FUID kControllerUID(0x8A6C914E, 0x48F147F7, 0xA3D2B9D0, 0xE5B61A22);

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
