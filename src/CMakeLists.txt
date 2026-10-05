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

# PACKAGE_NAME is the Steinberg-supported bundle/package name. This keeps
# Depthorator V2.vst3 separate from the V1 Depthorator.vst3 installation.
smtg_add_vst3plugin(Depthorator
    PACKAGE_NAME "Depthorator V2"
    SOURCES_LIST ${DEPTHORATOR_SOURCES}
)

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
)

smtg_target_configure_version_file(Depthorator)
