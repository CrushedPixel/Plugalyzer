#pragma once

#include "CLICommand.h"
#include "Utils.h"

#include <juce_audio_processors/juce_audio_processors.h>

class FailedXmlError : public CLI::Error {
  public:
    FailedXmlError(std::string msg, int exit_code)
        : CLI::Error("FailedXmlError", std::move(msg), exit_code) {}
};

class FileLoadError : public CLI::Error {
  public:
    FileLoadError(std::string msg, int exit_code)
        : CLI::Error("FileLoadError", std::move(msg), exit_code) {}
};

class ManageStateCommand : public CLICommand {
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
    juce::File outputFilePath;
    OutputFormat outputFormat{ OutputFormat::binary };
    bool overwriteOutputFile{ false };
};
