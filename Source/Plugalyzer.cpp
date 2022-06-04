#include "Plugalyzer.h"



void printPluginInfo(const juce::AudioPluginInstance& plugin) {
    std::cout << "Loaded plugin \"" << plugin.getName() << "\"." << std::endl << std::endl;
}

void process(const juce::String& pluginPath, const std::vector<juce::File>& audioInputFiles,
             const std::optional<juce::File>& midiInputFileOpt, const juce::File& outputFile,
             double fallbackSampleRate, int blockSize,
             const std::optional<int>& numOutputChannelsOpt,
             const std::vector<std::pair<juce::String, juce::String>>& params) {


}

