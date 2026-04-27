#include "Errors.h"
#include "Utils.h"

#include <format>
#include <iostream>
#include <nlohmann/json.hpp>

size_t secondsToSamples(double sec, double sampleRate) { return (size_t) (sec * sampleRate); }

std::unique_ptr<juce::AudioPluginInstance> PluginUtils::createPluginInstance(
    const juce::String& pluginPath, double initialSampleRate, int initialBlockSize
) {
    juce::AudioPluginFormatManager audioPluginFormatManager;
    addDefaultFormatsToManager(audioPluginFormatManager);

    // parse the plugin path into a PluginDescription instance
    juce::PluginDescription pluginDescription;
    {
        juce::OwnedArray<juce::PluginDescription> pluginDescriptions;

        juce::KnownPluginList kpl;
        kpl.scanAndAddDragAndDroppedFiles(
            audioPluginFormatManager, juce::StringArray(pluginPath), pluginDescriptions
        );

        // check if the requested plugin was found
        if (pluginDescriptions.isEmpty()) {
            throw CLIException("Invalid plugin identifier: " + pluginPath);
        }

        pluginDescription = *pluginDescriptions[0];
    }

    // create plugin instance
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    {
        juce::String err;
        plugin = audioPluginFormatManager.createPluginInstance(
            pluginDescription, initialSampleRate, initialBlockSize, err
        );

        if (!plugin) {
            throw CLIException("Error creating plugin instance: " + err);
        }
    }

    return plugin;
}

juce::AudioProcessorParameter* PluginUtils::getPluginParameterByName(
    const juce::AudioPluginInstance& plugin, const std::string& parameterName
) {

    auto* paramIt = std::find_if(
        plugin.getParameters().begin(), plugin.getParameters().end(),
        [&parameterName](juce::AudioProcessorParameter* parameter) {
            return parameter->getName(1024).toStdString() == parameterName;
        }
    );

    if (paramIt == plugin.getParameters().end()) {
        throw std::runtime_error("Unknown parameter identifier '" + parameterName + "'");
    }

    return *paramIt;
}

void loadPluginStateFromFile(juce::AudioPluginInstance& plugin, const juce::File& statePath, juce::MemoryBlock& state) {
    if (statePath != juce::File{}) {
        if (statePath.loadFileAsData(state)) {
            plugin.setStateInformation(state.getData(), state.getSize());
        } else {
            throw FileLoadError{
                std::format(
                    "Couldn't read file: {}", statePath.getFullPathName().toStdString()
                ),
                150
            };
        }
    }
}

juce::StringArray getDiscreteValueStrings(const juce::AudioProcessorParameter& param) {
    if (!param.isDiscrete()) return {};

    auto valueStrings = param.getAllValueStrings();
    // getAllValueStrings returns empty array for VST3
    if (valueStrings.isEmpty()) {
        const auto maxIndex = param.getNumSteps() - 1;
        for (int i = 0; i < param.getNumSteps(); ++i)
            valueStrings.add(param.getText((float) i / (float) maxIndex, 1024));
    }

    jassert(!valueStrings.isEmpty());

    return valueStrings;
}

juce::Array<juce::AudioChannelSet> allChannelSets() {
    juce::Array<juce::AudioChannelSet> layouts;

    layouts.add(juce::AudioChannelSet::disabled());
    layouts.add(juce::AudioChannelSet::mono());
    layouts.add(juce::AudioChannelSet::stereo());
    layouts.add(juce::AudioChannelSet::createLCR());
    layouts.add(juce::AudioChannelSet::createLRS());
    layouts.add(juce::AudioChannelSet::createLCRS());
    layouts.add(juce::AudioChannelSet::create5point0());
    layouts.add(juce::AudioChannelSet::create5point1());
    layouts.add(juce::AudioChannelSet::create6point0());
    layouts.add(juce::AudioChannelSet::create6point1());
    layouts.add(juce::AudioChannelSet::create6point0Music());
    layouts.add(juce::AudioChannelSet::create6point1Music());
    layouts.add(juce::AudioChannelSet::create7point0());
    layouts.add(juce::AudioChannelSet::create7point0SDDS());
    layouts.add(juce::AudioChannelSet::create7point1());
    layouts.add(juce::AudioChannelSet::create7point1SDDS());
    layouts.add(juce::AudioChannelSet::quadraphonic());
    layouts.add(juce::AudioChannelSet::pentagonal());
    layouts.add(juce::AudioChannelSet::hexagonal());
    layouts.add(juce::AudioChannelSet::octagonal());
    layouts.add(juce::AudioChannelSet::create5point0point2());
    layouts.add(juce::AudioChannelSet::create5point1point2());
    layouts.add(juce::AudioChannelSet::create5point0point4());
    layouts.add(juce::AudioChannelSet::create5point1point4());
    layouts.add(juce::AudioChannelSet::create7point0point2());
    layouts.add(juce::AudioChannelSet::create7point1point2());
    layouts.add(juce::AudioChannelSet::create7point0point4());
    layouts.add(juce::AudioChannelSet::create7point1point4());
    layouts.add(juce::AudioChannelSet::create7point0point6());
    layouts.add(juce::AudioChannelSet::create7point1point6());
    layouts.add(juce::AudioChannelSet::create9point0point4());
    layouts.add(juce::AudioChannelSet::create9point1point4());
    layouts.add(juce::AudioChannelSet::create9point0point6());
    layouts.add(juce::AudioChannelSet::create9point1point6());
    layouts.add(juce::AudioChannelSet::create9point0point4ITU());
    layouts.add(juce::AudioChannelSet::create9point1point4ITU());
    layouts.add(juce::AudioChannelSet::create9point0point6ITU());
    layouts.add(juce::AudioChannelSet::create9point1point6ITU());

    for (int order = 0; order < 8; ++order) {
        layouts.add(juce::AudioChannelSet::ambisonic(order));
    }

    for (int channels = 1; channels <= 64; ++channels) {
        layouts.add(juce::AudioChannelSet::discreteChannels(channels));
    }

    return layouts;
}

nlohmann::json getBusLayoutJson(const juce::AudioProcessor::BusesLayout& layout) {
    nlohmann::json json;

    for (auto [i, channelSet] : juce::enumerate(layout.inputBuses)) {
        json["inputBuses"][static_cast<size_t>(i)]["description"] =
            channelSet.getDescription().toStdString();
        json["inputBuses"][static_cast<size_t>(i)]["channels"] = channelSet.size();
    }

    for (auto [i, channelSet] : juce::enumerate(layout.outputBuses)) {
        json["outputBuses"][static_cast<size_t>(i)]["description"] =
            channelSet.getDescription().toStdString();
        json["outputBuses"][static_cast<size_t>(i)]["channels"] = channelSet.size();
    }

    return json;
}

std::string getBusLayoutHumanReadable(const nlohmann::json& layoutJson) {
    // Input is a list of objects of lists of objects

    std::string result;

    for (const auto& busLayout : layoutJson) {
        // Iterate through list
        for (const auto& [inOrOut, channelSets] : busLayout.items()) {
            // Open each object to get its list of channel sets
            for (const auto& [i, cs] : juce::enumerate(channelSets)) {
                // Print each channel set of each bus
                auto direction = inOrOut == "inputBuses" ? "In" : "Out";
                result += std::format(
                    "{} {}: {}ch {:24} ", direction, i + 1, cs["channels"].get<int>(),
                    cs["description"].get<std::string>()
                );
            }
        }
        result += "\n";
    }

    return result;
}

void outputResult(const std::string& text, juce::File outPath, bool overwrite) {
    if (outPath == juce::File{}) {
        std::cout << text;
    } else {
        if (overwrite) {
            outPath.replaceWithText(text);
        } else {
            outPath.getNonexistentSibling(false).replaceWithText(text);
        }
    }
}

void outputResult(const juce::MemoryBlock& data, juce::File outPath, bool overwrite) {
    if (outPath == juce::File{}) {
        std::cout.write(
            static_cast<const char*>(data.getData()), static_cast<std::streamsize>(data.getSize())
        );
    } else {
        if (overwrite) {
            outPath.replaceWithData(data.getData(), data.getSize());
        } else {
            outPath.getNonexistentSibling(false).replaceWithData(data.getData(), data.getSize());
        }
    }
}
