cmake_minimum_required(VERSION 3.15)
project(PLUGALYZER VERSION 0.0.1)

# embrace the future, baby!
set(CMAKE_CXX_STANDARD 20)

# include JUCE from submodule
add_subdirectory(JUCE)
juce_add_console_app(Plugalyzer
    COMPANY_NAME CrushedPixel
    )

# include CLI11 from submodule
add_subdirectory(CLI11)
target_include_directories(Plugalyzer PRIVATE CLI11/include)

#include nlohmann/json from submodule
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory(json)
target_include_directories(Plugalyzer PRIVATE json/include)

target_sources(Plugalyzer
    PRIVATE
    Source/main.cpp
    Source/Utils.cpp
    Source/ProcessCommand.cpp
    Source/ListParametersCommand.cpp
    Source/Automation.cpp
    )

target_compile_definitions(Plugalyzer
    PRIVATE
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Plugalyzer,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Plugalyzer,JUCE_VERSION>"

    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
    JUCE_PLUGINHOST_LADSPA=1
    JUCE_PLUGINHOST_LV2=1
    )

target_link_libraries(Plugalyzer
    PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_audio_devices
    juce::juce_audio_utils

    CLI11::CLI11
    nlohmann_json::nlohmann_json

    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)