#pragma once

#include "Automation.h"
#include "Generators.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

using InputSource = std::variant<std::unique_ptr<juce::AudioFormatReader>, GeneratorInputBus>;

/* Visits the InputSource variant */
template<typename... Callable>
struct InputSourceVisitor : Callable... {
    using Callable::operator()...;
};

/**
 * Parses the given MIDI file, with timestamps being converted seconds.
 *
 * @param midiFile The MIDI file.
 * @param sampleRate The sample rate to use for timestamp conversion.
 * @param lengthInSamplesOut Variable that will be set to the length of the MIDI file in
 * samples.
 * @return The parsed MIDI file.
 */
juce::MidiFile
readMIDIFile(const juce::File& file, double sampleRate, std::size_t& lengthInSamplesOut);

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