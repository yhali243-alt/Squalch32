cmake_minimum_required(VERSION 3.22)
project(Squalch32 VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(JUCE_PATH "" CACHE PATH "Path to a local JUCE checkout (optional)")
if(JUCE_PATH)
  add_subdirectory(${JUCE_PATH} JUCE)
else()
  include(FetchContent)
  FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.10
    GIT_SHALLOW ON)
  FetchContent_MakeAvailable(JUCE)
endif()

set(SQ_FORMATS VST3 CACHE STRING "Plugin formats to build (e.g. 'VST3;Standalone')")

juce_add_plugin(Squalch32
  COMPANY_NAME "PsyLab"
  PLUGIN_MANUFACTURER_CODE Psyl
  PLUGIN_CODE Sq32
  PRODUCT_NAME "Squalch32"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES Instrument Synth
  FORMATS ${SQ_FORMATS})

target_sources(Squalch32 PRIVATE PluginProcessor.cpp PluginEditor.cpp)
target_compile_definitions(Squalch32 PUBLIC
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)

set(SQ_LINK juce::juce_audio_processors juce::juce_gui_basics)
if("Standalone" IN_LIST SQ_FORMATS)
  list(APPEND SQ_LINK juce::juce_audio_utils)
endif()
target_link_libraries(Squalch32
  PRIVATE ${SQ_LINK}
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

add_executable(render_demo render_demo.cpp)
