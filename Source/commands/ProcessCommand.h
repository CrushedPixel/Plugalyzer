#pragma once

#include "Automation.h"
#include "CLICommand.h"
#include "Errors.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

class ProcessCommand : public CLICommand {
  public:
    std::shared_ptr<CLI::App> createApp() override;

    void execute() override;

  private:
    // String from CLI to be parsed into a File object
    std::string argOutPath;
    // String from CLI to be parsed into a File object
    std::string argStatePath;

    juce::String pluginPath;
    std::vector<juce::File> audioInputFiles;
    std::optional<juce::File> midiInputFileOpt;
    std::optional<juce::File> presetFileOpt;
    juce::File statePath;
    juce::File outputFilePath;
    bool overwriteOutputFile;
    double sampleRate = 44100;
    int blockSize = 1024;
    std::optional<unsigned int> outputChannelCountOpt;
    std::optional<unsigned int> outputBitDepthOpt;
    std::optional<juce::File> paramsFileOpt;
    std::vector<std::string> params;
};
