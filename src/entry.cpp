#include "public.sdk/source/main/pluginfactory.h"
#include "DepthoratorController.h"
#include "DepthoratorIDs.h"
#include "DepthoratorProcessor.h"

#define stringPluginName "Depthorator"
#define stringPluginVersion "1.0.0"

BEGIN_FACTORY_DEF("125A", "https://github.com/challanger2000/Depthorator-Final", "")

DEF_CLASS2(
    INLINE_UID_FROM_FUID(Depthorator::kProcessorUID),
    Steinberg::PClassInfo::kManyInstances,
    kVstAudioEffectClass,
    stringPluginName,
    Steinberg::Vst::kDistributable,
    "Fx|Delay|Reverb",
    stringPluginVersion,
    kVstVersionString,
    Depthorator::Processor::createInstance)

DEF_CLASS2(
    INLINE_UID_FROM_FUID(Depthorator::kControllerUID),
    Steinberg::PClassInfo::kManyInstances,
    kVstComponentControllerClass,
    "Depthorator Controller",
    0,
    "",
    stringPluginVersion,
    kVstVersionString,
    Depthorator::Controller::createInstance)

END_FACTORY
