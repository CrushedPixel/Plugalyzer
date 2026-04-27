#pragma once

#include "CLICommand.h"
#include "Utils.h"

#include <juce_audio_processors/juce_audio_processors.h>

class StateCommand : public CLICommand {
  public:
    std::shared_ptr<CLI::App> createApp() override;

    void execute() override;

  private:
    // String from CLI to be parsed into a File object
    std::string argPluginPath;
    // String from CLI to be parsed into a File object
    std::string argInPath;
    // String from CLI to be parsed into a File object
    std::string argOutPath;
    // String from CLI to be parsed into an outputFormat
    std::string argOutFormat;

    juce::File pluginPath;
    juce::File inputFilePath;
    juce::File outputFilePath;
    OutputFormat outputFormat{ OutputFormat::binary };
    bool overwriteOutputFile{ false };
};
