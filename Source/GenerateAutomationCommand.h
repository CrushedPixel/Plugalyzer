#pragma once

#include "CLICommand.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <nlohmann/json.hpp>

class GenerateAutomationCommand : public CLICommand {
  public:
    std::shared_ptr<CLI::App> createApp() override;

    void execute() override;

  private:
    // String from CLI to be parsed into a File object
    std::string argPluginPath;
    // String from CLI to be parsed into a File object
    std::string argOutPath;

    juce::File pluginPath;
    juce::File outputFilePath;
    bool overwriteOutputFile;
};