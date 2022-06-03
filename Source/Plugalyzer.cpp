#include "Plugalyzer.h"

std::unique_ptr<juce::AudioPluginInstance> createPluginInstance(const juce::String& pluginPath,
                                                                double initialSampleRate,
                                                                int initialBlockSize) {
    juce::AudioPluginFormatManager audioPluginFormatManager;
    audioPluginFormatManager.addDefaultFormats();

    juce::PluginDescription pluginDescription;
    {
        // parse the plugin path into a PluginDescription instance
        juce::OwnedArray<juce::PluginDescription> pluginDescriptions;

        juce::KnownPluginList kpl;
        kpl.scanAndAddDragAndDroppedFiles(audioPluginFormatManager, juce::StringArray(pluginPath),
                                          pluginDescriptions);

        // check if the requested plugin was found
        if (pluginDescriptions.isEmpty()) {
            juce::ConsoleApplication::fail("Invalid plugin identifier: " + pluginPath);
        }

        pluginDescription = *pluginDescriptions[0];
    }

    // create plugin instance
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    {
        juce::String err;
        plugin = audioPluginFormatManager.createPluginInstance(pluginDescription, initialSampleRate,
                                                               initialBlockSize, err);

        if (!plugin) {
            juce::ConsoleApplication::fail("Error creating plugin instance: " + err);
        }
    }

    return plugin;
}

void printPluginInfo(const juce::AudioPluginInstance& plugin) {
    std::cout << "Loaded plugin \"" << plugin.getName() << "\"." << std::endl << std::endl;
}

/**
 * Converts the given duration in seconds to samples given the sample rate.
 */
inline juce::int64 secondsToSamples(double sec, double sampleRate) {
    return (juce::int64) (sec * sampleRate);
}

void process(const juce::String& pluginPath, const std::vector<juce::File>& audioInputFiles,
             const std::optional<juce::File>& midiInputFileOpt, const juce::File& outputFile,
             double fallbackSampleRate, int blockSize,
             const std::optional<int>& numOutputChannelsOpt,
             const std::vector<std::pair<juce::String, juce::String>>& params) {
    // parse the input files
    juce::AudioFormatManager audioFormatManager;
    audioFormatManager.registerBasicFormats();

    juce::OwnedArray<juce::AudioFormatReader> audioInputFileReaders;
    juce::int64 maxInputLength = 0;

    double sampleRate = fallbackSampleRate;

    for (size_t i = 0; i < audioInputFiles.size(); i++) {
        auto& inputFile = audioInputFiles[i];

        auto inputFileReader = audioFormatManager.createReaderFor(inputFile);
        if (!inputFileReader) {
            juce::ConsoleApplication::fail("Could not read input file " +
                                           inputFile.getFullPathName());
        }

        audioInputFileReaders.add(inputFileReader);

        // ensure the sample rate of all input files is the same
        if (i == 0) {
            sampleRate = inputFileReader->sampleRate;
        } else if (inputFileReader->sampleRate != sampleRate) {
            juce::ConsoleApplication::fail("Mismatched sample rate between input files");
        }

        maxInputLength = std::max(maxInputLength, inputFileReader->lengthInSamples);
    }

    // create the plugin instance
    auto plugin = createPluginInstance(pluginPath, sampleRate, blockSize);
    printPluginInfo(*plugin);

    // create the plugin's bus layout with an input bus for each input file
    unsigned int totalNumInputChannels = 0;
    juce::AudioPluginInstance::BusesLayout layout;
    for (auto* inputFileReader : audioInputFileReaders) {
        layout.inputBuses.add(
            juce::AudioChannelSet::canonicalChannelSet(inputFileReader->numChannels));
        totalNumInputChannels += inputFileReader->numChannels;
    }

    // create an output bus with the desired amount of channels,
    // defaulting to the same amount of channels as the main input file if one exists,
    // or the plugin's default bus layout otherwise.
    unsigned int totalNumOutputChannels = (numOutputChannelsOpt.value_or(
        audioInputFileReaders.isEmpty() ? plugin->getBusesLayout().getMainOutputChannels()
                                        : audioInputFileReaders[0]->numChannels));
    layout.outputBuses.add(juce::AudioChannelSet::canonicalChannelSet(totalNumOutputChannels));

    // apply the channel layout
    if (!plugin->setBusesLayout(layout)) {
        juce::ConsoleApplication::fail("Plugin does not support requested bus layout");
    }

    // apply plugin parameters
    for (auto& p : params) {
        auto& paramId = p.first;

        auto* paramIt =
            std::find_if(plugin->getParameters().begin(), plugin->getParameters().end(),
                         [paramId](juce::AudioProcessorParameter* parameter) {
                             return parameter->getName(1024) == paramId ||
                                    juce::String(parameter->getParameterIndex()) == paramId;
                         });

        if (paramIt == plugin->getParameters().end()) {
            juce::ConsoleApplication::fail("Unknown parameter identifier " + paramId);
        }

        auto* param = *paramIt;
        param->setValueNotifyingHost(param->getValueForText(p.second));
    }

    // read MIDI input file
    juce::MidiFile midiFile;
    if (midiInputFileOpt) {
        auto inputStream = midiInputFileOpt->createInputStream();
        if (!midiFile.readFrom(*inputStream, true)) {
            juce::ConsoleApplication::fail("Error reading MIDI input file");
        }

        // since MIDI tick length is defined in the file header,
        // let JUCE take care of the conversion for us and work with timestamps in seconds
        midiFile.convertTimestampTicksToSeconds();

        // find the timestamp of the last MIDI event in the file
        // to ensure we process until that MIDI event is reached
        for (int i = 0; i < midiFile.getNumTracks(); i++) {
            auto midiTrack = midiFile.getTrack(i);
            for (auto& meh : *midiTrack) {
                auto timestampSamples = secondsToSamples(meh->message.getTimeStamp(), sampleRate);
                maxInputLength = std::max(maxInputLength, timestampSamples);
            }
        }
    }

    // TODO: is this needed?
    plugin->prepareToPlay(sampleRate, blockSize);

    // open output stream
    std::unique_ptr<juce::AudioFormatWriter> outWriter;
    {
        outputFile.deleteFile();
        auto outputStream = outputFile.createOutputStream(blockSize);
        if (!outputStream) {
            juce::ConsoleApplication::fail("Could not create output stream to write to file " +
                                           outputFile.getFullPathName());
        }

        juce::WavAudioFormat outFormat;
        outWriter.reset(outFormat.createWriterFor(
            outputStream.release() /* stream is now managed by writer */, sampleRate,
            totalNumOutputChannels, 16, juce::StringPairArray(), 0));
    }

    // process the input files with the plugin
    juce::AudioBuffer<float> sampleBuffer(std::max(totalNumInputChannels, totalNumOutputChannels),
                                          blockSize);

    juce::MidiBuffer midiBuffer;
    juce::int64 sampleIndex = 0;
    while (sampleIndex < maxInputLength) {
        // read next segment of audio input files into buffer
        unsigned int targetChannel = 0;
        for (auto* inputFileReader : audioInputFileReaders) {
            if (!inputFileReader->read(sampleBuffer.getArrayOfWritePointers() + targetChannel,
                                       inputFileReader->numChannels, sampleIndex, blockSize)) {
                juce::ConsoleApplication::fail(
                    "Error reading input file"); // TODO: more context, which file?
            }

            targetChannel += inputFileReader->numChannels;
        }

        // populate MIDI buffer with the MIDI events
        // falling into the current processing block.
        // we simply take MIDI events from all tracks and supply them to the buffer -
        // if the user only wants a single track of a multi-track MIDI file,
        // they should extract that track into a separate MIDI file.
        midiBuffer.clear();
        for (int i = 0; i < midiFile.getNumTracks(); i++) {
            auto midiTrack = midiFile.getTrack(i);

            for (auto& meh : *midiTrack) {
                auto timestampSamples = secondsToSamples(meh->message.getTimeStamp(), sampleRate);
                if (timestampSamples >= sampleIndex && timestampSamples < sampleIndex + blockSize) {
                    midiBuffer.addEvent(meh->message, (int) (timestampSamples - sampleIndex));
                }
            }
        }

        // process with plugin
        plugin->processBlock(sampleBuffer, midiBuffer);

        // write to output
        outWriter->writeFromAudioSampleBuffer(sampleBuffer, 0, blockSize);

        sampleIndex += blockSize;
    }
}

void listParameters(const juce::String& pluginPath, double initialSampleRate,
                    int initialBlockSize) {
    auto plugin = createPluginInstance(pluginPath, initialSampleRate, initialBlockSize);
    printPluginInfo(*plugin);

    std::cout << "Plugin parameters: " << std::endl;

    auto params = plugin->getParameters();

    // calculate the amount of characters the maximum parameter index has
    // for alignment purposes
    auto maxIdxStrLength = juce::String(params.size() - 1).length();

    for (auto* param : params) {
        // calculate the indent of the index to align all entries nicely
        auto idxStrLength = juce::String(param->getParameterIndex()).length();
        std::string idxIndent(maxIdxStrLength - idxStrLength, ' ');

        // index: name
        std::cout << idxIndent << param->getParameterIndex() << ": " << param->getName(100)
                  << std::endl;

        // calculate the indent of entries for alignment
        std::string indent(2 + maxIdxStrLength, ' ');

        // print the parameter's values
        std::cout << indent << "Values:  ";

        if (auto valueStrings = param->getAllValueStrings(); !valueStrings.isEmpty()) {
            // list all discrete values
            for (int i = 0; i < valueStrings.size(); i++) {
                std::cout << valueStrings[i];
                if (i != valueStrings.size() - 1) {
                    std::cout << ", ";
                } else {
                    std::cout << std::endl;
                }
            }
        } else {
            // print the range of values that is supported
            std::cout << param->getText(0, 1024) << param->getLabel() << " to "
                      << param->getText(1, 1024) << param->getLabel() << std::endl;
        }

        // print the default value
        std::cout << indent << "Default: " << param->getText(param->getDefaultValue(), 1024)
                  << param->getLabel() << std::endl;
    }
}
