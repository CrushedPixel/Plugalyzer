#pragma once

#include "Automation.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

/**
 * Creates readers for the given audio files, verifying that their sample rate matches.
 *
 * @param files The audio files.
 * @param maxLengthInSamplesOut Variable that will be set to the length of the longest audio
 * file, in samples.
 * @return The audio file readers.
 */
juce::OwnedArray<juce::AudioFormatReader>
createAudioFileReaders(const std::vector<juce::File>& files, size_t& maxLengthInSamplesOut);

/**
 * Parses the given MIDI file, with timestamps being converted seconds.
 *
 * @param midiFile The MIDI file.
 * @param sampleRate The sample rate to use for timestamp conversion.
 * @param lengthInSamplesOut Variable that will be set to the length of the MIDI file in
 * samples.
 * @return The parsed MIDI file.
 */
juce::MidiFile readMIDIFile(const juce::File& file, double sampleRate, size_t& lengthInSamplesOut);

/**
 * Creates a bus layout according to the user input.
 *
 * The number of input buses will match the plugin, and their channel sets will match the input
 * files. The number of output buses will be 1, and it's channel set will match (in order of
 * preference) the user-supplied number, the first input file, or the plugin's default.
 *
 * @param plugin The plugin to create the bus layout for.
 * @param audioInputFileReaders The file readers for each input file.
 * @param outputChannelCountOpt The user-supplied number of channels to use for the output bus.
 * @return The bus layout.
 */
juce::AudioPluginInstance::BusesLayout createBusLayout(
    const juce::AudioPluginInstance& plugin,
    const juce::OwnedArray<juce::AudioFormatReader>& audioInputFileReaders,
    const std::optional<unsigned int>& outputChannelCountOpt
);

/**
 * Parses and validates plugin parameters supplied via file and CLI.
 *
 * @param plugin The plugin to validate the parameters against.
 * @param sampleRate The sample rate to use for time conversion purposes.
 * @param inputLengthInSamples The total length of the input that will be supplied to the
 * plugin, in samples. Used to warn the user about keyframes that lie outside of the range of
 * input audio.
 * @param parameterFileOpt The parameter file, if supplied.
 * @param cliParameters The parameters supplied via CLI.
 * @return The parsed plugin parameters.
 */
ParameterAutomation parseParameters(
    const juce::AudioPluginInstance& plugin, double sampleRate, size_t inputLengthInSamples,
    const std::optional<juce::File>& parameterFileOpt, const std::vector<std::string>& cliParameters
);