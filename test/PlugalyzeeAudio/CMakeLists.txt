cmake_minimum_required(VERSION 3.22)

project(TEST_AUDIO_PLUGIN VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(../../JUCE juce)

juce_add_plugin(PlugalyzeeAudio
    # VERSION ...
    # ICON_BIG ...
    # ICON_SMALL ...
    COMPANY_NAME Plugalyzer
    # IS_SYNTH TRUE/FALSE
    # NEEDS_MIDI_INPUT TRUE/FALSE
    # NEEDS_MIDI_OUTPUT TRUE/FALSE
    # IS_MIDI_EFFECT TRUE/FALSE
    # EDITOR_WANTS_KEYBOARD_FOCUS TRUE/FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Plug
    PLUGIN_CODE Kal6
    FORMATS AU VST3 LV2
    PRODUCT_NAME "PlugalyzeeAudio"
    LV2URI "urn:CrushedPixel:Plugalyzee:Audio"
)

target_sources(PlugalyzeeAudio
    PRIVATE
        PluginProcessor.cpp
        PlugalyzeeAudio.cpp
)

target_compile_definitions(PlugalyzeeAudio
    PUBLIC
        JUCE_WEB_BROWSER=0  
        JUCE_USE_CURL=0     
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_LOG_ASSERTIONS=1
        JUCE_DISABLE_JUCE_VERSION_PRINTING=1
)

target_link_libraries(PlugalyzeeAudio
    PRIVATE
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
