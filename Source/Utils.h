#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

/**
 * Loads and initializes an audio plugin.
 *
 * @param pluginPath The plugin's file path.
 * @param initialSampleRate The sample rate to initialize the plugin with.
 * @param initialBlockSize The buffer size to initialize the plugin with.
 * @return The initialized plugin.
 */
std::unique_ptr<juce::AudioPluginInstance>
createPluginInstance(const juce::String& pluginPath, double initialSampleRate, int initialBlockSize);

/**
 * Converts the given time in seconds to samples given the sample rate.
 *
 * @param sec The time to convert, in seconds.
 * @param sampleRate The sample rate.
 * @return The time in samples.
 */
juce::int64 secondsToSamples(double sec, double sampleRate);