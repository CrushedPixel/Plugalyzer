#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

void process(const juce::String& pluginPath, const std::vector<juce::File>& inputFiles,
             const juce::File& outputFile, int blockSize, std::optional<int> numOutputChannelsOpt);

void listParameters(const juce::String& pluginPath, double initialSampleRate, int initialBlockSize);