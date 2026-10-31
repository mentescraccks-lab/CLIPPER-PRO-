cmake_minimum_required(VERSION 3.22)
project(CLIPPER VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(CLIPPER
    COMPANY_NAME "AndresBeats"
    PLUGIN_MANUFACTURER_CODE Abts
    PLUGIN_CODE Clp1
    FORMATS VST3
    PRODUCT_NAME "CLIPPER"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(CLIPPER PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(CLIPPER PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(CLIPPER
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# Pruebas del DSP puro (sin JUCE)
enable_testing()
add_executable(ClipperDSPTests Tests/DSPTests.cpp)
target_compile_features(ClipperDSPTests PRIVATE cxx_std_17)
add_test(NAME ClipperDSPTests COMMAND ClipperDSPTests)
