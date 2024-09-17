#pragma once

#include "Automation.h"
#include "CLICommand.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

class ProcessCommand : public CLICommand {
  public:
    std::shared_ptr<CLI::App> createApp() override;

    void execute() override;

  private:
    juce::String pluginPath;
    std::vector<juce::File> audioInputFiles;
    std::optional<juce::File> midiInputFileOpt;
    std::optional<juce::File> presetFileOpt;
    juce::File outputFilePath;
    bool overwriteOutputFile;
    double sampleRate = 44100;
    unsigned int blockSize = 1024;
    std::optional<unsigned int> outputChannelCountOpt;
    std::optional<juce::File> paramsFileOpt;
    std::vector<std::string> params;

    /**
     * Creates readers for the given audio files, verifying that their sample rate matches.
     *
     * @param files The audio files.
     * @param maxLengthInSamplesOut Variable that will be set to the length of the longest audio
     * file, in samples.
     * @return The audio file readers.
     */
    static juce::OwnedArray<juce::AudioFormatReader>
    createAudioFileReaders(const std::vector<juce::File>& files, size_t& maxLengthInSamplesOut);

    /**
     * Parses the given MIDI file, with timestamps being converted seconds.
     *
     * @param midiFile The MIDI file.
     * @param sampleRate The sample rate to use for timestamp conversion.
     * @param lengthInSamplesOut Variable that will be set to the length of the MIDI file in
     * samples.
     * @return The parsed MIDI file.
     */
    static juce::MidiFile readMIDIFile(const juce::File& file, double sampleRate,
                                       size_t& lengthInSamplesOut);

    /**
     * Creates a bus layout with one input bus for each input file.
     *
     * @param plugin The plugin to create the bus layout for.
     * @param audioInputFileReaders The file readers for each input file.
     * @param outputChannelCountOpt The amount of channels to use for the output bus. If not
     * supplied, the same amount of channels as the main input file is used. If no such file exists,
     * the plugin's default bus layout is used.
     * @param totalNumInputChannelsOut Variable that will be populated with the total amount of
     * input channels the returned layout has.
     * @param totalNumOutputChannelsOut Variable that will be populated with the total amount of
     * output channels the returned layout has.
     * @return The bus layout.
     */
    static juce::AudioPluginInstance::BusesLayout
    createBusLayout(const juce::AudioPluginInstance& plugin,
                    const juce::OwnedArray<juce::AudioFormatReader>& audioInputFileReaders,
                    const std::optional<unsigned int>& outputChannelCountOpt,
                    unsigned int& totalNumInputChannelsOut,
                    unsigned int& totalNumOutputChannelsOut);

    /**
     * Parses and validates plugin parameters supplied via file and CLI.
     *
     * @param plugin The plugin to validate the parameters against.
     * @param sampleRate The sample rate to use for time conversion purposes.
     * @param inputLengthInSamples The total length of the input that will be supplied to the
     * plugin, in samples. Used to warn the user about keyframes that lie outside of the range of
     * input audio.
     * @param parameterFileOpt The parameter file, if supplied.
     * @param cliParameters The parameters supplied via CLI.
     * @return The parsed plugin parameters.
     */
    static ParameterAutomation parseParameters(const juce::AudioPluginInstance& plugin,
                                               double sampleRate, size_t inputLengthInSamples,
                                               const std::optional<juce::File>& parameterFileOpt,
                                               const std::vector<std::string>& cliParameters);
};
