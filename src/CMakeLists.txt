set(DEPTHORATOR_SOURCES
    DepthoratorIDs.h
    DepthoratorReverb.h
    DepthoratorReverb.cpp
    DepthoratorProcessor.h
    DepthoratorProcessor.cpp
    DepthoratorKnob.h
    DepthoratorKnob.cpp
    DepthoratorLogoView.h
    DepthoratorLogoView.cpp
    DepthoratorController.h
    DepthoratorController.cpp
    entry.cpp
)

smtg_add_vst3plugin(Depthorator ${DEPTHORATOR_SOURCES})

target_link_libraries(Depthorator
    PRIVATE
        sdk
        vstgui_support
)

if(WIN32)
    target_sources(Depthorator PRIVATE
        ${vst3sdk_SOURCE_DIR}/public.sdk/source/main/dllmain.cpp
    )
endif()

smtg_target_add_plugin_resources(Depthorator
    RESOURCES
    ../resource/Depthorator.uidesc
    ../resource/125A_Signet.png
)

smtg_target_configure_version_file(Depthorator)
