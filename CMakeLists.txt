cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")

project(ResoOG VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.10
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

if(APPLE)
    set(RESOOG_FORMATS VST3 AU Standalone)
    set(RESOOG_COPY TRUE)
else()
    set(RESOOG_FORMATS VST3 Standalone)
    set(RESOOG_COPY FALSE) # copying into Program Files needs admin rights
endif()

juce_add_plugin(ResoOG
    COMPANY_NAME "Gavr"
    COMPANY_WEBSITE "https://gavr.cz"
    BUNDLE_ID "cz.gavr.resoog"
    PLUGIN_MANUFACTURER_CODE Gavr
    PLUGIN_CODE RsOG
    PRODUCT_NAME "ResoOG"
    FORMATS ${RESOOG_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD ${RESOOG_COPY}
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice)

set(RESOOG_SOURCES
    Source/Params/Parameters.cpp
    Source/DSP/SynthLayer.cpp
    Source/DSP/Effects.cpp
    Source/DSP/ModMatrix.cpp
    Source/State/StateHistory.cpp
    Source/State/GlobalSettings.cpp
    Source/State/FactoryPresets.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/UI/Theme.cpp
    Source/UI/Knob.cpp
    Source/UI/Widgets.cpp
    Source/UI/Pages.cpp
    Source/UI/Keyboard.cpp
    Source/UI/Bars.cpp
    Source/UI/ModDrawer.cpp)

target_sources(ResoOG PRIVATE ${RESOOG_SOURCES})
target_include_directories(ResoOG PRIVATE Source)

target_compile_definitions(ResoOG PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ResoOG
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Offline test harness (not part of the plug-in): cmake -DRESOOG_TESTS=ON
option(RESOOG_TESTS "Build the offline test host" OFF)
if(RESOOG_TESTS)
    juce_add_console_app(ResoOGTests PRODUCT_NAME "ResoOGTests")
    target_sources(ResoOGTests PRIVATE tools/TestHost/Main.cpp ${RESOOG_SOURCES})
    target_include_directories(ResoOGTests PRIVATE Source)
    target_compile_definitions(ResoOGTests PRIVATE
        JUCE_PLUGINHOST_VST3=1 JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JucePlugin_Name="ResoOG" JucePlugin_VersionString="0.2.0" JucePlugin_IsSynth=1)
    target_link_libraries(ResoOGTests PRIVATE juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags)
endif()
