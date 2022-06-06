#pragma once

#include "CLICommand.h"
#include <juce_audio_processors/juce_audio_processors.h>

class ListParametersCommand : public CLICommand {
  public:
    std::shared_ptr<CLI::App> createApp() override;

    void execute() override;

  private:
    juce::String pluginPath;
    double sampleRate = 44100;
    unsigned int blockSize = 1024;
};
