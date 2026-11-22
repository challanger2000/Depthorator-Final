cmake_minimum_required(VERSION 3.25)

project(Depthorator VERSION 1.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES ON CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VSTGUI_SUPPORT ON CACHE BOOL "" FORCE)
set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "" FORCE)

include(FetchContent)

FetchContent_Declare(
    vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
    GIT_TAG v3.8.1_build_84
)

FetchContent_MakeAvailable(vst3sdk)

add_subdirectory(src)

include(CTest)
if(BUILD_TESTING)
    add_executable(DepthoratorAutoClearTests tests/AutoClearCoreTests.cpp)
    target_include_directories(DepthoratorAutoClearTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_features(DepthoratorAutoClearTests PRIVATE cxx_std_17)
    add_test(NAME depthorator_autoclear_core COMMAND DepthoratorAutoClearTests)
endif()
