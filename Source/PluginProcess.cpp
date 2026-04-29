#include "PluginProcess.h"

#include "Automation.h"
#include "Errors.h"
#include "Parsers.h"
#include "Utils.h"

#include <CLI/CLI.hpp>

juce::OwnedArray<juce::AudioFormatReader>
createAudioFileReaders(const std::vector<juce::File>& files, size_t& maxLengthInSamplesOut) {
    maxLengthInSamplesOut = 0;

    // parse the input files
    juce::AudioFormatManager audioFormatManager;
    audioFormatManager.registerBasicFormats();

    juce::OwnedArray<juce::AudioFormatReader> audioInputFileReaders;

    double sampleRate{ 0.0 };
    for (size_t i = 0; i < files.size(); i++) {
        auto& inputFile = files[i];

        auto inputFileReader = audioFormatManager.createReaderFor(inputFile);
        if (!inputFileReader) {
            throw CLIException("Could not read input file " + inputFile.getFullPathName());
        }

        audioInputFileReaders.add(inputFileReader);

        // ensure the sample rate of all input files is the same
        if (i == 0) {
            sampleRate = inputFileReader->sampleRate;
        } else if (!juce::exactlyEqual(inputFileReader->sampleRate, sampleRate)) {
            throw CLIException("Mismatched sample rate between input files");
        }

        maxLengthInSamplesOut =
            std::max(maxLengthInSamplesOut, (size_t) inputFileReader->lengthInSamples);
    }

    return audioInputFileReaders;
}

juce::MidiFile readMIDIFile(const juce::File& file, double sampleRate, size_t& lengthInSamplesOut) {
    juce::MidiFile midiFile;
    lengthInSamplesOut = 0;

    auto inputStream = file.createInputStream();
    if (!midiFile.readFrom(*inputStream, true)) {
        throw CLIException("Error reading MIDI input file");
    }

    // since MIDI tick length is defined in the file header,
    // let JUCE take care of the conversion for us and work with timestamps in seconds
    midiFile.convertTimestampTicksToSeconds();

    // find the timestamp of the last MIDI event in the file
    // to ensure we process until that MIDI event is reached
    for (int i = 0; i < midiFile.getNumTracks(); i++) {
        auto midiTrack = midiFile.getTrack(i);
        for (auto& meh : *midiTrack) {
            auto timestampSamples = secondsToSamples(meh->message.getTimeStamp(), sampleRate);
            lengthInSamplesOut = std::max(lengthInSamplesOut, timestampSamples);
        }
    }

    return midiFile;
}

juce::AudioPluginInstance::BusesLayout createBusLayout(
    const juce::AudioPluginInstance& plugin,
    const juce::OwnedArray<juce::AudioFormatReader>& audioInputFileReaders,
    const std::optional<unsigned int>& outputChannelCount
) {
    // Start with compatible bus layout
    juce::AudioPluginInstance::BusesLayout layout{ plugin.getBusesLayout() };
    jassert(layout.outputBuses.size() == 1);

    // Inputs: Change channel formats to match input files
    for (const auto& [index, reader] : juce::enumerate(audioInputFileReaders)) {
        layout.inputBuses[index] =
            juce::AudioChannelSet::canonicalChannelSet((int) reader->numChannels);
    }

    // Outputs: Change channel formats to match user-supplied option or first input file
    if (outputChannelCount.has_value()) {
        layout.outputBuses[0] =
            juce::AudioChannelSet::canonicalChannelSet((int) *outputChannelCount);
    } else if (!audioInputFileReaders.isEmpty()) {
        layout.outputBuses[0] =
            juce::AudioChannelSet::canonicalChannelSet((int) audioInputFileReaders[0]->numChannels);
    }

    return layout;
}

ParameterAutomation parseParameters(
    const juce::AudioPluginInstance& plugin, double sampleRate, size_t inputLengthInSamples,
    const std::optional<juce::File>& parameterFileOpt, const std::vector<std::string>& cliParameters
) {
    ParameterAutomation automation;

    // read automation from file
    if (parameterFileOpt) {
        automation = Automation::parseAutomationDefinition(
            parameterFileOpt->loadFileAsString().toStdString(), plugin, sampleRate,
            inputLengthInSamples
        );
    }

    // parse command-line supplied parameters
    for (auto& arg : cliParameters) {
        auto [paramName, textValue, normalizedValue, isNormalizedValue] =
            parse::pluginParameterArgument(arg);

        // convert parameter value from text representation to a single keyframe,
        // which causes the same value to be applied over the entire duration
        auto* param = PluginUtils::getPluginParameterByName(plugin, paramName);

        if (!isNormalizedValue) {
            if (!Automation::parameterSupportsTextToValueConversion(param)) {
                throw CLIException(
                    "Parameter '" + paramName +
                    "' does not support text values. Use :n suffix to supply "
                    "a normalized value instead"
                );
            }

            normalizedValue = param->getValueForText(textValue);
        }

        // warn the user if the parameter overrides a parameter specified in the file
        if (automation.contains(paramName)) {
            std::cerr << "Plugin parameter '" << paramName
                      << "' is specified in the parameter file and overridden by a command-line "
                         "parameter."
                      << std::endl;
        }

        automation[paramName] = AutomationKeyframes({ { 0, normalizedValue } });
    }

    return automation;
}
