cmake_minimum_required(VERSION 3.22)

project(KeirsChords VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

# JUCE: local checkout via -DJUCE_DIR=..., otherwise the pinned release
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} juce)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

if(APPLE)
    set(KC_FORMATS AU VST3 Standalone)
else()
    set(KC_FORMATS VST3 Standalone)     # Linux is only used for compile checks
endif()

juce_add_plugin(KeirsChords
    COMPANY_NAME "Keir"
    BUNDLE_ID "com.keir.keirschords"
    PLUGIN_MANUFACTURER_CODE Keir
    PLUGIN_CODE Kchd
    FORMATS ${KC_FORMATS}
    PRODUCT_NAME "Keir's Chords"
    DESCRIPTION "House, techno and jazz chord sequencer with 909 / MPC / human swing"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Generator
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    MICROPHONE_PERMISSION_ENABLED FALSE)

juce_add_binary_data(KeirsChordsFonts SOURCES
    Resources/BarlowCondensed-SemiBold.ttf
    Resources/BarlowCondensed-Bold.ttf
    Resources/Barlow-Medium.ttf
    Resources/VT323-Regular.ttf)

target_sources(KeirsChords PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(KeirsChords PRIVATE Source)

target_compile_definitions(KeirsChords PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_ALSA=0
    JUCE_JACK=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(KeirsChords
    PRIVATE
        KeirsChordsFonts
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# engine unit test (no JUCE)
add_executable(engine_test tests/engine_test.cpp)
target_include_directories(engine_test PRIVATE Source)

option(KC_TESTS "Build the host-simulation test" OFF)
if(KC_TESTS)
    juce_add_console_app(plugin_test PRODUCT_NAME "plugin_test")
    target_sources(plugin_test PRIVATE tests/plugin_test.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(plugin_test PRIVATE Source)
    target_compile_definitions(plugin_test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_ALSA=0 JUCE_JACK=0
        JucePlugin_Name="Keir's Chords" JucePlugin_Build_Standalone=0 JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(plugin_test PRIVATE KeirsChordsFonts juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
