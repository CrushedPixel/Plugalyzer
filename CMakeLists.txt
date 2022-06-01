cmake_minimum_required(VERSION 3.15)
project(PLUGALYZER VERSION 0.0.1)

# embrace the future, baby!
set(CMAKE_CXX_STANDARD 20)

# include JUCE from submodule
add_subdirectory(JUCE)

juce_add_console_app(Plugalyzer
    COMPANY_NAME CrushedPixel
    )

target_sources(Plugalyzer
    PRIVATE
    Source/main.cpp
    Source/Plugalyzer.cpp
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
    )

target_link_libraries(Plugalyzer
    PRIVATE
    # GuiAppData            # If we'd created a binary data target, we'd link to it here
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_audio_devices
    juce::juce_audio_utils

    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)