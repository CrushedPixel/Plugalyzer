#pragma once

#include "CLICommand.h"
#include "PluginProcess.h"

#include <cstddef>
#include <cstdio>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class ProcessCommand : public CLICommand {
  public:
    ProcessCommand() { audioFormatManager.registerBasicFormats(); }
    std::shared_ptr<CLI::App> createApp() override;

    void execute() override;

  private:
    std::string validateInputFileSampleRate(const std::string& arg);
    std::string validateInputGeneratorSampleRate(const std::string& arg);
    std::unique_ptr<juce::AudioFormatReader> parseAudioFileInput(const std::string& audioFilePath);
    std::size_t getLengthOfLongestAudioInput(Hertz sampleRate) const;
    // Returns zero if there are no inputs
    int getBitDepthOfInput() const;
    juce::Array<juce::AudioChannelSet> getInputBusesLayoutFromAudioInputs() const;
    juce::Array<juce::AudioChannelSet> getOutputBusesLayout(const juce::AudioPluginInstance& plugin) const;
    void negotiateBusesLayout(juce::AudioPluginInstance& plugin);
    void prepareAudioInputs(Hertz currentSampleRate, int currentBlockSize);
    void renderAudioInput(juce::AudioBuffer<float>& buffer, size_t sampleIndex);

    // Strings from CLI to be parsed into audio input sources
    std::vector<std::string> argInputSources;
    // String from CLI to be parsed into a File object
    std::string argPluginPath;
    // String from CLI to be parsed into a File object
    std::string argOutPath;
    // String from CLI to be parsed into a File object
    std::string argStatePath;
    // String from CLI to be parsed into a Generator
    std::string argGeneratorMain;
    std::string argGeneratorSide;

    // Sample rate found in audio inputs for validation
    double inputSampleRate{ 0.0 };

    // Sample rate provided by user
    double argSampleRate{ 0.0 };

    juce::File pluginPath;
    std::vector<InputSource> audioInputs;
    std::optional<juce::File> midiInputFileOpt;
    std::optional<juce::File> presetFileOpt;
    juce::File statePath;
    juce::File outputFilePath;
    bool overwriteOutputFile;
    int blockSize = 1024;
    std::optional<unsigned int> outputChannelCountOpt;
    std::optional<int> outputBitDepthOpt;
    std::optional<juce::File> paramsFileOpt;
    std::vector<std::string> params;
    juce::AudioFormatManager audioFormatManager;
};
