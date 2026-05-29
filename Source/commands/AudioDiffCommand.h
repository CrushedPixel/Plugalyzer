#pragma once

#include "AudioDiff.h"
#include "CLICommand.h"

#include <juce_audio_formats/juce_audio_formats.h>

class FailedDiffError : public CLI::Error {
  public:
    FailedDiffError(std::string msg, int exit_code)
        : CLI::Error("FailedDiffError", std::move(msg), exit_code) {}
};

class AudioDiffCommand : public CLICommand {
  public:
    std::shared_ptr<CLI::App> createApp() override;

    void execute() override;

  private:
    // String from CLI to be parsed into a double
    std::string argThreshold;

    double rmsThreshold{ juce::Decibels::decibelsToGain(-50.0, -96.0) };
    std::map<AudioFileRole, juce::String> inputFilePaths{
        { AudioFileRole::test, {} },
        { AudioFileRole::reference, {} },
    };
};
