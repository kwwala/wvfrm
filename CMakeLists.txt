cmake_minimum_required(VERSION 3.22)

project(wvfrm VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.6
)

FetchContent_MakeAvailable(JUCE)

add_library(wvfrm_core STATIC
  src/JuceIncludes.h
  src/dsp/AnalysisRingBuffer.h
  src/dsp/AnalysisRingBuffer.cpp
  src/dsp/LoopClock.h
  src/dsp/LoopClock.cpp
  src/dsp/TimeWindowResolver.h
  src/dsp/TimeWindowResolver.cpp
  src/dsp/BandAnalyzer3.h
  src/dsp/BandAnalyzer3.cpp
  src/dsp/ChannelViews.h
)

add_library(wvfrm_plugin_support STATIC
  src/Parameters.h
  src/Parameters.cpp
  src/ui/ThemeEngine.h
  src/ui/ThemeEngine.cpp
)

target_link_libraries(wvfrm_core
  PUBLIC
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_graphics
)

target_include_directories(wvfrm_core PUBLIC src)

target_link_libraries(wvfrm_plugin_support
  PUBLIC
    wvfrm_core
    juce::juce_audio_processors
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_opengl
)

target_include_directories(wvfrm_plugin_support PUBLIC src)

set(WVFRM_PLUGIN_NAME "wvfrm0")

juce_add_plugin(wvfrm
  COMPANY_NAME "Kwwala"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD TRUE
  PLUGIN_MANUFACTURER_CODE WvF0
  PLUGIN_CODE WvF1
  FORMATS VST3
  PRODUCT_NAME "${WVFRM_PLUGIN_NAME}"
)

if(TARGET wvfrm_VST3)
  set_target_properties(wvfrm_VST3 PROPERTIES
    OUTPUT_NAME "${WVFRM_PLUGIN_NAME}"
  )
endif()

juce_generate_juce_header(wvfrm)

target_sources(wvfrm
  PRIVATE
    src/PluginProcessor.h
    src/PluginProcessor.cpp
    src/PluginEditor.h
    src/PluginEditor.cpp
    src/ui/WaveformView.h
    src/ui/WaveformView.cpp
)

target_link_libraries(wvfrm
  PRIVATE
    wvfrm_core
    wvfrm_plugin_support
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

target_compile_definitions(wvfrm
  PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

enable_testing()

add_executable(wvfrm_tests
  tests/main.cpp
  tests/AnalysisRingBufferTests.cpp
  tests/LoopClockTests.cpp
  tests/TimeWindowResolverTests.cpp
  tests/BandAnalyzer3Tests.cpp
  tests/ChannelViewsTests.cpp
)

target_link_libraries(wvfrm_tests
  PRIVATE
    wvfrm_core
)

add_test(NAME wvfrm_tests COMMAND wvfrm_tests)
