#include "ProcessCommand.h"

#include "PluginProcess.h"
#include "PresetLoadingExtensionsVisitor.h"
#include "Utils.h"
#include "Validators.h"

#include <cstdio>
#include <juce_audio_processors/juce_audio_processors.h>
#include <print>

// Let the user know if we'll be creating input buses for them
static void
checkBusCountsAndWarn(const juce::AudioPluginInstance& plugin, const auto& audioInputFileReaders) {
    // The plugin's default number of inputs and outputs
    auto pluginInputBusCount = plugin.getBusCount(true);
    // The user-supplied number of inputs and outputs
    auto userInputBusCount = audioInputFileReaders.size();
    if (pluginInputBusCount != userInputBusCount) {
        std::println(
            stderr,
            "Plugin requires {} input buses. {} input buses provided. "
            "Other input buses will be silent.",
            pluginInputBusCount, userInputBusCount
        );
    }
}

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

    app->add_option("--preset", presetFileOpt, "Preset file path. Currently only .vstpreset files for VST3 are supported.")->check(CLI::ExistingFile);

    app->add_option("-o,--output", outputFilePath, "Output audio file path")->required();
    app->add_flag("-y,--overwrite", overwriteOutputFile, "Overwrite the output file if it exists");

    auto* sampleRateOption = app->add_option("-s,--sampleRate", sampleRate, "The sample rate to use for processing when no audio file is supplied");
    // sample rate is dictated by input audio files if they're provided
    audioInputOption->excludes(sampleRateOption);

    app->add_option("-b,--blockSize", blockSize, "The buffer size to use when processing audio");
    app->add_option("-d,--bitDepth", outputBitDepthOpt, "The output file's bit depth. Defaults to the input file's bit depth if present, or 16 bits if no input file is provided.")->check(validate::bitDepth);
    app->add_option("-c,--outChannels", outputChannelCountOpt, "The amount of channels to use for the plugin's output bus");

    app->add_option("--paramFile", paramsFileOpt, "Path to JSON file to read plugin parameters and automation data from")->check(CLI::ExistingFile);
    app->add_option("--param", params, "Plugin parameters to set. Explicitly specified parameters take precedence over parameters read from file")->check(validate::pluginParameter);

    return app;
    // clang-format on
}

void ProcessCommand::execute() {
    size_t totalInputLength;

    unsigned int bitDepth = 16;

    // create audio file readers
    auto audioInputFileReaders = createAudioFileReaders(audioInputFiles, totalInputLength);
    // use the sample rate of input audio files if provided
    if (!audioInputFileReaders.isEmpty()) {
        sampleRate = audioInputFileReaders[0]->sampleRate;
        bitDepth = audioInputFileReaders[0]->bitsPerSample;
    }

    if (outputBitDepthOpt) {
        bitDepth = *outputBitDepthOpt;
    }

    // read MIDI input file
    juce::MidiFile midiFile;
    if (midiInputFileOpt) {
        size_t midiLength;
        midiFile = readMIDIFile(*midiInputFileOpt, sampleRate, midiLength);
        totalInputLength = std::max(totalInputLength, midiLength);
    }

    // create the plugin instance
    auto plugin = PluginUtils::createPluginInstance(pluginPath, sampleRate, (int) blockSize);

    if (presetFileOpt) {
        // read preset file into memory block
        juce::MemoryBlock presetData;
        const auto presetInputStream = presetFileOpt->createInputStream();
        presetInputStream->readIntoMemoryBlock(presetData);
        // TODO: how to handle errors?

        // apply preset
        PresetLoadingExtensionsVisitor presetLoader(presetData);
        plugin->getExtensions(presetLoader);
    }

    // create and apply the bus layout
    const auto pluginOutputBusCount = plugin->getBusCount(false);
    if (pluginOutputBusCount != 1) {
        throw CLIException("Multi-output plugins currently not supported. Please write a PR!");
    }
    checkBusCountsAndWarn(*plugin, audioInputFileReaders);

    auto layout = createBusLayout(*plugin, audioInputFileReaders, outputChannelCountOpt);

    if (!plugin->setBusesLayout(layout)) {
        throw CLIException(
            "Plugin does not support requested bus layout: " + describeBusesLayout(layout)
        );
    }

    // parse plugin parameters
    auto automation = parseParameters(*plugin, sampleRate, totalInputLength, paramsFileOpt, params);

    plugin->prepareToPlay(sampleRate, (int) blockSize);
    const auto latency = plugin->getLatencySamples();

    // open output stream
    if (outputFilePath.exists() && !overwriteOutputFile) {
        throw CLIException("Output file already exists! Use --overwrite to overwrite the file");
    }

    auto totalNumInputChannels = getTotalNumInputChannels(layout);
    auto totalNumOutputChannels = getTotalNumOutputChannels(layout);
    std::unique_ptr<juce::AudioFormatWriter> outWriter;
    outputFilePath.deleteFile();
    if (std::unique_ptr<juce::OutputStream> outputStream{
            outputFilePath.createOutputStream(static_cast<size_t>(blockSize)) }) {
        juce::WavAudioFormat outFormat;
        outWriter = outFormat.createWriterFor(
            outputStream, // stream is now managed by writer
            juce::AudioFormatWriterOptions{}
                .withSampleRate(sampleRate)
                .withNumChannels(static_cast<int>(totalNumOutputChannels))
                .withBitsPerSample(static_cast<int>(bitDepth))
        );
    } else {
        throw CLIException(
            "Could not create output stream to write to file " + outputFilePath.getFullPathName()
        );
    }

    // process the input files with the plugin
    juce::AudioBuffer<float> sampleBuffer(
        (int) std::max(totalNumInputChannels, totalNumOutputChannels), (int) blockSize);

    juce::MidiBuffer midiBuffer;
    size_t sampleIndex = 0;
    int samplesSkipped = 0;
    while (sampleIndex < totalInputLength + static_cast<size_t>(latency)) {
        sampleBuffer.clear();

        // read next segment of audio input files into buffer
        unsigned int targetChannel = 0;
        for (auto* inputFileReader : audioInputFileReaders) {
            if (!inputFileReader->read(sampleBuffer.getArrayOfWritePointers() + targetChannel,
                                       (int) inputFileReader->numChannels, static_cast<juce::int64>(sampleIndex),
                                       (int) blockSize)) {
                throw CLIException("Error reading input file"); // TODO: more context, which file?
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
                if (timestampSamples >= sampleIndex && timestampSamples < sampleIndex + static_cast<size_t>(blockSize)) {
                    midiBuffer.addEvent(meh->message, (int) (timestampSamples - sampleIndex));
                }
            }
        }

        // apply automation
        Automation::applyParameters(*plugin, automation, sampleIndex);

        // process with plugin
        plugin->processBlock(sampleBuffer, midiBuffer);

        // skip the first samples that are just empty because of the plugin's latency
        int startSample = 0;
        if (samplesSkipped < latency) {
            startSample = std::min<int>(latency - samplesSkipped, blockSize);
            samplesSkipped += startSample;
        }

        // write to output
        if (startSample < blockSize) {
            outWriter->writeFromAudioSampleBuffer(sampleBuffer, startSample,
                                                  (int) blockSize - startSample);
        }

        sampleIndex += static_cast<size_t>(blockSize);
    }
}
