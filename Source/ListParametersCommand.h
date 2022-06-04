#pragma once

#include "CLICommand.h"
#include <juce_core/juce_core.h>

class ListParametersCommand : public CLICommand {
  public:
    std::shared_ptr<CLI::App> createApp() override;

    void execute() override;

  private:
    juce::String pluginPath;
    double sampleRate = 44100;
    unsigned int blockSize = 1024;
};
