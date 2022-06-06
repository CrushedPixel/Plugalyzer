#include "Utils.h"

size_t secondsToSamples(double sec, double sampleRate) { return (size_t) (sec * sampleRate); }

#define PARSE_STRICT(funName)                                         \
    size_t endPtr;                                                    \
    auto num = (funName)(str, &endPtr);                               \
    if (endPtr != str.size()) {                                       \
        throw std::invalid_argument("Invalid number: '" + str + "'"); \
    }                                                                 \
    return num


float parseFloatStrict(const std::string &str) {
    PARSE_STRICT(std::stof);
}

unsigned long parseULongStrict(const std::string &str) {
    PARSE_STRICT(std::stoul);
}

std::unique_ptr<juce::AudioPluginInstance>
PluginUtils::createPluginInstance(const juce::String& pluginPath, double initialSampleRate,
                                  int initialBlockSize) {
    juce::AudioPluginFormatManager audioPluginFormatManager;
    audioPluginFormatManager.addDefaultFormats();

    // parse the plugin path into a PluginDescription instance
    juce::PluginDescription pluginDescription;
    {
        juce::OwnedArray<juce::PluginDescription> pluginDescriptions;

        juce::KnownPluginList kpl;
        kpl.scanAndAddDragAndDroppedFiles(audioPluginFormatManager, juce::StringArray(pluginPath),
                                          pluginDescriptions);

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
        plugin = audioPluginFormatManager.createPluginInstance(pluginDescription, initialSampleRate,
                                                               initialBlockSize, err);

        if (!plugin) {
            throw CLIException("Error creating plugin instance: " + err);
        }
    }

    return plugin;
}

juce::AudioProcessorParameter*
PluginUtils::getPluginParameterByName(const juce::AudioPluginInstance& plugin,
                                      const std::string& parameterName) {

    auto* paramIt = std::find_if(plugin.getParameters().begin(), plugin.getParameters().end(),
                                 [&parameterName](juce::AudioProcessorParameter* parameter) {
                                     return parameter->getName(1024).toStdString() == parameterName;
                                 });

    if (paramIt == plugin.getParameters().end()) {
        throw std::runtime_error("Unknown parameter identifier '" + parameterName + "'");
    }

    return *paramIt;
}
