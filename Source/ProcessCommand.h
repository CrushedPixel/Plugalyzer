#pragma once

#include "CLICommand.h"
#include <juce_core/juce_core.h>

class ProcessCommand : public CLICommand {
  public:
    std::shared_ptr<CLI::App> createApp() override;

    void execute() override;

  private:
    juce::String pluginPath;
    std::vector<juce::File> audioInputFiles;
    std::optional<juce::File> midiInputFileOpt;
    juce::File outputFilePath;
    bool overwriteOutputFile;
    double sampleRate = 44100;
    unsigned int blockSize = 1024;
    std::optional<unsigned int> outputChannelCountOpt;
    std::vector<std::string> parameters;
};
