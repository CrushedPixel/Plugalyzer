#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

/**
 * Processes audio using the given plugin, writing the audio output to file.
 *
 * @param pluginPath The path or identifier of the plugin to process.
 * @param audioInputFiles A list of audio input files to supply to the plugin. May be empty.
 * @param midiInputFileOpt An optional MIDI input file to supply to the plugin.
 * @param outputFile The file to write the plugin's audio output to.
 * @param fallbackSampleRate The sample rate to use when no audio input file is supplied.
 * @param blockSize The block size to use for processing.
 * @param numOutputChannelsOpt The amount of channels the plugin's output bus should have.
 * @param params Pairs of name and value for each parameter to apply to the plugin when processing.
 */
void process(const juce::String& pluginPath, const std::vector<juce::File>& audioInputFiles,
             const std::optional<juce::File>& midiInputFileOpt, const juce::File& outputFile,
             double fallbackSampleRate, int blockSize,
             const std::optional<int>& numOutputChannelsOpt,
             const std::vector<std::pair<juce::String, juce::String>>& params);

void listParameters(const juce::String& pluginPath, double initialSampleRate, int initialBlockSize);