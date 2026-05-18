#include "PluginProcess.h"

#include "Automation.h"
#include "Errors.h"
#include "Parsers.h"
#include "Utils.h"

#include <CLI/CLI.hpp>

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
