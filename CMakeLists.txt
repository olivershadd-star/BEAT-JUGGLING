cmake_minimum_required(VERSION 3.22)
project(BEAT_JUGGLING VERSION 0.1.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(BeatJuggling
    COMPANY_NAME "Perso"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    PLUGIN_MANUFACTURER_CODE Pers
    PLUGIN_CODE BjPp
    VST3_CATEGORIES Instrument Sampler
    FORMATS VST3 Standalone
    PRODUCT_NAME "Beat Juggling")

target_sources(BeatJuggling PRIVATE
    Source/Program.h
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp)

target_compile_definitions(BeatJuggling PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_VST3_CAN_REPLACE_VST2=0)

if(MSVC)
    target_compile_options(BeatJuggling PRIVATE /utf-8)
endif()

target_link_libraries(BeatJuggling
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
