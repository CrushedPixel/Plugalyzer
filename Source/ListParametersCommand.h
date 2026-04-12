#pragma once

#include "CLICommand.h"
#include "Utils.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <nlohmann/json.hpp>

class ListParametersCommand : public CLICommand {
  public:
    std::shared_ptr<CLI::App> createApp() override;

    void execute() override;

  private:
    std::string getParametersAsJson(const juce::Array<juce::AudioProcessorParameter*>& params);
    std::string getParametersAsString(const juce::Array<juce::AudioProcessorParameter*>& params);

    juce::String pluginPath;
    double sampleRate = 44100;
    unsigned int blockSize = 1024;
    juce::String outputFormat;
};
