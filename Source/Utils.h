#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

struct CLIException : std::runtime_error {
    explicit CLIException(const std::string& message) : std::runtime_error(message) {}
    explicit CLIException(const char* message) : std::runtime_error(message) {}
    explicit CLIException(const juce::String& message)
        : std::runtime_error(message.toStdString()) {}
};

/**
 * Converts the given time in seconds to samples given the sample rate.
 *
 * @param sec The time to convert, in seconds.
 * @param sampleRate The sample rate.
 * @return The time in samples.
 */
size_t secondsToSamples(double sec, double sampleRate);

class PluginUtils {
  public:
    /**
     * Loads and initializes an audio plugin.
     *
     * @param pluginPath The plugin's file path.
     * @param initialSampleRate The sample rate to initialize the plugin with.
     * @param initialBlockSize The buffer size to initialize the plugin with.
     * @return The initialized plugin.
     */
    static std::unique_ptr<juce::AudioPluginInstance>
    createPluginInstance(const juce::String& pluginPath, double initialSampleRate,
                         int initialBlockSize);

    /**
     * Finds the plugin's parameter with the given name.
     *
     * @param plugin The plugin.
     * @param parameterName The parameter name.
     * @return The parameter.
     * @throws std::runtime_error If the plugin has no parameter with the given name.
     */
    static juce::AudioProcessorParameter*
    getPluginParameterByName(const juce::AudioPluginInstance& plugin,
                             const std::string& parameterName);
};
