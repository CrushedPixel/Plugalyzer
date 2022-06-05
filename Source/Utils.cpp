#include "Utils.h"

std::unique_ptr<juce::AudioPluginInstance> createPluginInstance(const juce::String& pluginPath,
                                                                double initialSampleRate,
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

juce::int64 secondsToSamples(double sec, double sampleRate) {
    return (juce::int64) (sec * sampleRate);
}
