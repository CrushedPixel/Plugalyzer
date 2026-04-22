#include "Automation.h"
#include "PluginProcess.h"
#include "Parsers.h"
#include "Errors.h"
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
    const std::optional<unsigned int>& outputChannelCountOpt,
    unsigned int& totalNumInputChannelsOut, unsigned int& totalNumOutputChannelsOut
) {

    totalNumInputChannelsOut = 0;
    juce::AudioPluginInstance::BusesLayout layout;
    if (audioInputFileReaders.isEmpty()) {
        // if no input files are provided, use the plugin's default input bus layout
        // to maximize compatibility with synths that expect an input
        layout.inputBuses = plugin.getBusesLayout().inputBuses;
        for (auto& cs : layout.inputBuses) {
            totalNumInputChannelsOut += (unsigned int) cs.size();
        }

    } else {
        for (auto* inputFileReader : audioInputFileReaders) {
            layout.inputBuses.add(
                juce::AudioChannelSet::canonicalChannelSet((int) inputFileReader->numChannels));
            totalNumInputChannelsOut += inputFileReader->numChannels;
        }
    }

    // create an output bus with the desired amount of channels,
    // defaulting to the same amount of channels as the main input file if one exists,
    // or the plugin's default bus layout otherwise.
    totalNumOutputChannelsOut = (outputChannelCountOpt.value_or(
        audioInputFileReaders.isEmpty()
            ? (unsigned int) plugin.getBusesLayout().getMainOutputChannels()
            : audioInputFileReaders[0]->numChannels));
    layout.outputBuses.add(
        juce::AudioChannelSet::canonicalChannelSet((int) totalNumOutputChannelsOut));

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
            inputLengthInSamples);
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
                throw CLIException("Parameter '" + paramName +
                                   "' does not support text values. Use :n suffix to supply "
                                   "a normalized value instead");
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

        automation[paramName] = AutomationKeyframes({{0, normalizedValue}});
    }

    return automation;
}
