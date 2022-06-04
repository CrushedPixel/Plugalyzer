#include "ProcessCommand.h"
#include "Utils.h"
#include <juce_audio_formats/juce_audio_formats.h>

/**
 * Parses a plugin parameter string in the format <key>:<value> into a key-value pair.
 *
 * @param str The string to parse.
 * @return The parsed key-value pair.
 * @throws std::runtime_error If the input string is not formatted correctly.
 */
std::pair<juce::String, juce::String> parsePluginParameterArgument(const std::string& str) {
    juce::StringArray tokens;
    tokens.addTokens(str, ":", "\"'");

    if (tokens.size() != 2) {
        throw std::runtime_error("'" + str + "' is not a colon-separated key-value pair");
    }

    return {tokens[0], tokens[1]};
}

/**
 * Validates the format of a plugin parameter passed via CLI to be "<key>:<value>".
 * This does not validate if the parameter exists on a plugin.
 */
struct PluginParameterValidator : public CLI::Validator {
    PluginParameterValidator() {
        name_ = "PLUGIN_PARAM";
        func_ = [](const std::string& str) {
            try {
                parsePluginParameterArgument(str);
            } catch (const std::runtime_error& e) {
                return std::string(e.what());
            }

            return std::string();
        };
    }
};

const static PluginParameterValidator PluginParameter;

std::shared_ptr<CLI::App> ProcessCommand::createApp() {
    // don't break these lines, please
    // clang-format off
    std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>("Processes audio using a plugin", "process");

    app->add_option("-p,--plugin", pluginPath, "Plugin path")->required()->check(CLI::ExistingPath); // not ExistingFile because on macOS, these bundles are directores

    auto* inputGroup = app->add_option_group("input");
    auto* audioInputOption = inputGroup->add_option("-i,--input", audioInputFiles, "Input audio file path")->check(CLI::ExistingFile);
    inputGroup->add_option("-m,--midiInput", midiInputFileOpt, "Input MIDI file path")->check(CLI::ExistingFile);
    // require at least one input of either kind
    inputGroup->require_option();

    app->add_option("-o,--output", outputFilePath, "Output audio file path")->required();
    app->add_flag("-y,--overwrite", overwriteOutputFile, "Overwrite the output file if it exists");

    auto* sampleRateOption = app->add_option("-s,--sampleRate", sampleRate, "The sample rate to use for processing when no audio file is supplied");
    // sample rate is dictated by input audio files if they're provided
    audioInputOption->excludes(sampleRateOption);

    app->add_option("-b,--blockSize", blockSize, "The buffer size to use when processing audio");
    app->add_option("-c,--outChannels", outputChannelCountOpt, "The amount of channels to use for the plugin's output bus");
    app->add_option("--param", parameters, "Plugin parameters to set")->check(PluginParameter);

    return app;
    // clang-format on
}

void ProcessCommand::execute() {
    // parse the input files
    juce::AudioFormatManager audioFormatManager;
    audioFormatManager.registerBasicFormats();

    juce::OwnedArray<juce::AudioFormatReader> audioInputFileReaders;
    juce::int64 maxInputLength = 0;

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
    auto plugin = createPluginInstance(pluginPath, sampleRate, (int) blockSize);

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
    unsigned int totalNumOutputChannels = (outputChannelCountOpt.value_or(
        audioInputFileReaders.isEmpty() ? plugin->getBusesLayout().getMainOutputChannels()
                                        : audioInputFileReaders[0]->numChannels));
    layout.outputBuses.add(juce::AudioChannelSet::canonicalChannelSet(totalNumOutputChannels));

    // apply the channel layout
    if (!plugin->setBusesLayout(layout)) {
        juce::ConsoleApplication::fail("Plugin does not support requested bus layout");
    }

    // apply plugin parameters
    for (auto& arg : parameters) {
        // can't use destructuring because the destructured value can't be captured in a lambda...
        auto p = parsePluginParameterArgument(arg);
        auto& paramId = p.first;
        auto& value = p.second;

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
        param->setValueNotifyingHost(param->getValueForText(value));
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
    if (outputFilePath.exists() && !overwriteOutputFile) {
        juce::ConsoleApplication::fail(
            "Output file already exists! Use --overwrite to overwrite the file");
    }

    std::unique_ptr<juce::AudioFormatWriter> outWriter;
    {
        outputFilePath.deleteFile();
        auto outputStream = outputFilePath.createOutputStream(blockSize);
        if (!outputStream) {
            juce::ConsoleApplication::fail("Could not create output stream to write to file " +
                                           outputFilePath.getFullPathName());
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
