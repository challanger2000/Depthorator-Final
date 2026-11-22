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

smtg_add_vst3plugin(Depthorator
    PACKAGE_NAME "125A Depthorator"
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

# The pinned Steinberg CMake helper's Windows/MSBuild resource-copy rule
# expands $(TargetFileName) to the *logical target* name, not PACKAGE_NAME.
# With "125A Depthorator.vst3" this incorrectly copies into the old
# "Depthorator.vst3" path. Copy to the SDK-provided canonical package path
# at build time; never rename a compiled/staged VST3 by hand.
get_target_property(DEPTHORATOR_PACKAGE_PATH Depthorator SMTG_PLUGIN_PACKAGE_PATH)
if(NOT DEPTHORATOR_PACKAGE_PATH)
    message(FATAL_ERROR "Missing Steinberg VST3 package path for Depthorator")
endif()
set(DEPTHORATOR_RESOURCE_PATH "${DEPTHORATOR_PACKAGE_PATH}/Contents/Resources")
add_custom_command(TARGET Depthorator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DEPTHORATOR_RESOURCE_PATH}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../resource/Depthorator.uidesc"
        "${DEPTHORATOR_RESOURCE_PATH}/Depthorator.uidesc"
    COMMENT "Copy GUI resource to the official 125A Depthorator VST3 package"
    VERBATIM
)

smtg_target_configure_version_file(Depthorator)
