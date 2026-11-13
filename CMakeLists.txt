cmake_minimum_required(VERSION 3.22)

# Muss VOR project() stehen, sonst ignoriert CMake diese Werte.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures")

project(Raumchoreographie VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: entweder lokaler Checkout (-DJUCE_DIR=/pfad/zu/JUCE) oder automatischer Download.
# Version 8.0.12 entspricht dem JUCE-Stand der ICST Ambisonics Plugins 3.2.0.9.
set(JUCE_DIR "" CACHE PATH "Pfad zu einem lokalen JUCE-Checkout (leer = Download)")
if(JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory("${JUCE_DIR}" JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.12
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

option(CHOREO_BUILD_TESTS "Konsolentest (Export, Rundlauf) mitbauen" ON)

juce_add_binary_data(ChoreoFonts
    NAMESPACE ChoreoFonts
    SOURCES
        Resources/Fonts/Inter-Regular.ttf
        Resources/Fonts/Inter-Medium.ttf
        Resources/Fonts/Inter-SemiBold.ttf
        Resources/Fonts/JetBrainsMono-Regular.ttf
        Resources/Fonts/JetBrainsMono-Medium.ttf)

set(CHOREO_MODEL_SOURCES
    Source/Model/Plan.cpp
    Source/Model/Scene.cpp
    Source/Model/Generator.cpp
    Source/Model/Preview.cpp
    Source/Model/RegionImport.cpp
    Source/Model/EncoderImport.cpp
    Source/Model/SceneToPlan.cpp
    Source/AI/PlanAI.cpp
    Source/AI/PromptPlanner.cpp
    third_party/icst/Common/ScalingInfo.cpp)

set(CHOREO_INCLUDE_DIRS
    Source
    third_party/icst/Encoder/Source
    third_party/icst/Common)

set(CHOREO_DEFINES
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

# ------------------------------------------------------------------ App
juce_add_gui_app(Raumchoreographie
    PRODUCT_NAME "Animator Choreographer"
    COMPANY_NAME "ICST"
    BUNDLE_ID "ch.zhdk.icst.animatorchoreographer"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icon/AnimatorChoreographer-icon.png"
    VERSION ${PROJECT_VERSION})

juce_generate_juce_header(Raumchoreographie)

target_sources(Raumchoreographie PRIVATE
    ${CHOREO_MODEL_SOURCES}
    Source/Main.cpp
    Source/UI/Theme.cpp
    Source/UI/AppState.cpp
    Source/UI/Widgets.cpp
    Source/UI/StageViews.cpp
    Source/UI/StepPanel.cpp
    Source/UI/AiPanel.cpp
    Source/UI/MainComponent.cpp)

target_include_directories(Raumchoreographie PRIVATE ${CHOREO_INCLUDE_DIRS})
target_compile_definitions(Raumchoreographie PRIVATE
    ${CHOREO_DEFINES}
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Raumchoreographie,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Raumchoreographie,JUCE_VERSION>")

target_link_libraries(Raumchoreographie
    PRIVATE
        ChoreoFonts
        juce::juce_gui_basics
        juce::juce_data_structures
        juce::juce_audio_processors_headless
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ------------------------------------------------------------------ Tests
if(CHOREO_BUILD_TESTS)
    juce_add_console_app(ChoreoTests PRODUCT_NAME "ChoreoTests")
    juce_generate_juce_header(ChoreoTests)
    target_sources(ChoreoTests PRIVATE ${CHOREO_MODEL_SOURCES} tests/ChoreoTests.cpp)
    target_include_directories(ChoreoTests PRIVATE ${CHOREO_INCLUDE_DIRS})
    target_compile_definitions(ChoreoTests PRIVATE ${CHOREO_DEFINES})
    target_link_libraries(ChoreoTests
        PRIVATE
            juce::juce_gui_basics
            juce::juce_data_structures
            juce::juce_audio_processors_headless
            juce::juce_opengl
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endif()

# ------------------------------------------------------------------ Kommandozeile: Plan aus Beschreibung (Ollama)
juce_add_console_app(ChoreoAI PRODUCT_NAME "ChoreoAI")
juce_generate_juce_header(ChoreoAI)
target_sources(ChoreoAI PRIVATE ${CHOREO_MODEL_SOURCES} Source/AI/ChoreoAIMain.cpp)
target_include_directories(ChoreoAI PRIVATE ${CHOREO_INCLUDE_DIRS})
target_compile_definitions(ChoreoAI PRIVATE ${CHOREO_DEFINES})
target_link_libraries(ChoreoAI
    PRIVATE
        juce::juce_gui_basics
        juce::juce_data_structures
        juce::juce_audio_processors_headless
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
