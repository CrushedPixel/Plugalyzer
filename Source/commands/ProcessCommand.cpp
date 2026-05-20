#include "ProcessCommand.h"

#include "Errors.h"
#include "Generators.h"
#include "Parsers.h"
#include "PluginProcess.h"
#include "PresetLoadingExtensionsVisitor.h"
#include "Utils.h"
#include "Validators.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <format>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <print>
#include <string>
#include <variant>
#include <vector>

std::shared_ptr<CLI::App> ProcessCommand::createApp() {
    // don't break these lines, please
    // clang-format off
    std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>("Processes audio using a plugin", "process");

    app->add_option("-p,--plugin", argPluginPath, "Plugin path")->required()
        ->check(CLI::ExistingPath) // not ExistingFile because on macOS, these bundles are directores
        ->each([&](std::string arg){ pluginPath = parse::stringToFile(arg); });

    auto* inputGroup = app->add_option_group("input");
    auto* audioInputOption = inputGroup->add_option("-i,--input", argInputSources, "Input audio file path")
        ->check(CLI::ExistingFile)
        ->check([&](const std::string& arg) { return this->validateInputFileSampleRate(arg); })
        ->each([&](std::string arg){ audioInputs.push_back(parseAudioFileInput(arg)); });
    inputGroup->add_option("-m,--midiInput", midiInputFileOpt, "Input MIDI file path")
        ->check(CLI::ExistingFile);
    auto* generatorInputOption = inputGroup->add_option("-g,--generatorInput", argGenerator, "JSON string or file with the configuration to generate audio input")
        ->check(validate::generator)
        ->check([&](const std::string& arg) { return this->validateInputGeneratorSampleRate(arg); })
        ->each([&](std::string arg){ audioInputs.push_back(parse::generatorInput(arg)); });
    // require at least one input of any kind
    inputGroup->require_option();

    app->add_option("--preset", presetFileOpt, "Preset file path. Currently only .vstpreset files for VST3 are supported.")
        ->check(CLI::ExistingFile);

    app->add_option("-o,--output", argOutPath, "Output audio file path")
        ->required()
        ->check(validate::outputPath)
        ->each([&](std::string arg) { outputFilePath = parse::stringToFile(arg); });
    app->add_flag("-y,--overwrite", overwriteOutputFile, "Overwrite the output file if it exists");

    auto* sampleRateOption = app->add_option("-s,--sampleRate", argSampleRate, "The sample rate to use for processing when no audio input is supplied");
    // sample rate is dictated by input audio files/generators if they're provided
    audioInputOption->excludes(sampleRateOption);
    generatorInputOption->excludes(sampleRateOption);

    app->add_option("-b,--blockSize", blockSize, "The buffer size to use when processing audio");
    app->add_option("-d,--bitDepth", outputBitDepthOpt, "The output file's bit depth. Defaults to the input file's bit depth if present, or 16 bits if no input file is provided.")
        ->check(validate::bitDepth);
    app->add_option("-c,--outChannels", outputChannelCountOpt, "The amount of channels to use for the plugin's output bus");

    app->add_option("--paramFile", argParamsFile, "Path to JSON file to read plugin parameters and automation data from")
        ->check(CLI::ExistingFile)
        ->each([&](std::string arg){ paramsFileOpt = parse::stringToFile(arg); });
    app->add_option("--param", params, "Plugin parameters to set. Explicitly specified parameters take precedence over parameters read from file")
        ->check(validate::pluginParameter);

    return app;
    // clang-format on
}

void ProcessCommand::execute() {
    const auto sampleRate = inputSampleRate != 0.0 ? inputSampleRate : argSampleRate;
    auto totalInputLength = getLengthOfLongestAudioInput(sampleRate);
    auto bitDepth = audioInputs.size() > 0 ? getBitDepthOfInput() : 16;
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
    auto plugin = PluginUtils::createPluginInstance(
        pluginPath.getFullPathName(), sampleRate, (int) blockSize
    );

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
    negotiateBusesLayout(*plugin);

    // parse plugin parameters
    auto automation = parseParameters(*plugin, sampleRate, totalInputLength, paramsFileOpt, params);

    prepareAudioInputs(sampleRate, blockSize);
    plugin->prepareToPlay(sampleRate, blockSize);

    const auto latency = plugin->getLatencySamples();

    // open output stream
    if (outputFilePath.exists() && !overwriteOutputFile) {
        throw CLIException("Output file already exists! Use --overwrite to overwrite the file");
    }

    auto totalNumInputChannels = getTotalNumInputChannels(plugin->getBusesLayout());
    auto totalNumOutputChannels = getTotalNumOutputChannels(plugin->getBusesLayout());
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
        (int) std::max(totalNumInputChannels, totalNumOutputChannels), (int) blockSize
    );

    juce::MidiBuffer midiBuffer;
    size_t sampleIndex = 0;
    int samplesSkipped = 0;
    while (sampleIndex < totalInputLength + static_cast<size_t>(latency)) {
        sampleBuffer.clear();
        renderAudioInput(sampleBuffer, sampleIndex);

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
                if (timestampSamples >= sampleIndex &&
                    timestampSamples < sampleIndex + static_cast<size_t>(blockSize)) {
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
            outWriter->writeFromAudioSampleBuffer(
                sampleBuffer, startSample, (int) blockSize - startSample
            );
        }

        sampleIndex += static_cast<size_t>(blockSize);
    }
}

std::string ProcessCommand::validateInputFileSampleRate(const std::string& arg) {
    auto f = parse::stringToFile(arg);
    if (std::unique_ptr<juce::AudioFormatReader> inputFileReader{
            audioFormatManager.createReaderFor(f) }) {
        if (inputSampleRate == 0.0) {
            inputSampleRate = inputFileReader->sampleRate;
            return {};
        }
        if (!juce::exactlyEqual(inputSampleRate, inputFileReader->sampleRate)) {
            return std::format("Mismatched sample rate in input file: {}", arg);
        }
        return {};
    } else {
        return std::format("Found the file but couldn't open it as audio: {}", arg);
    }
}

std::string ProcessCommand::validateInputGeneratorSampleRate(const std::string& arg) {
    std::string jsonString{ arg };
    auto candidateFile = parse::stringToFile(arg);
    bool readAsFile{ false };

    if (candidateFile.existsAsFile()) {
        if (!candidateFile.hasReadAccess()) {
            return std::format("No read access to {}", arg);
        }
        jsonString = candidateFile.loadFileAsString().toStdString();
        if (jsonString == juce::String{}) {
            return std::format("Didn't manage to read anything from {}", arg);
        }
        readAsFile = true;
    }
    if (!nlohmann::json::accept(jsonString)) {
        return readAsFile ? std::format("Couldn't parse json in file {}", arg)
                          : "Couldn't parse JSON string";
    }

    auto sr = parse::extractSampleRate(arg);

    if (inputSampleRate == 0.0) {
        inputSampleRate = sr;
        return {};
    }

    if (!juce::exactlyEqual(inputSampleRate, sr)) {
        std::print(
            stderr,
            "Generator sample rate {} does not match previous sample rate {}. Using previous "
            "sample rate.",
            sr, inputSampleRate
        );
    }

    return {};
}

std::unique_ptr<juce::AudioFormatReader>
ProcessCommand::parseAudioFileInput(const std::string& audioFilePath) {
    auto f = parse::stringToFile(audioFilePath);
    if (std::unique_ptr<juce::AudioFormatReader> inputFileReader{
            audioFormatManager.createReaderFor(f) }) {
        return inputFileReader;
    }
    throw FileLoadError{ std::format("Couldn't read audio file: {}", audioFilePath), 97 };
}

std::size_t ProcessCommand::getLengthOfLongestAudioInput(Hertz sampleRate) const {
    size_t maxLengthInSamples{ 0 };

    using Reader = std::unique_ptr<juce::AudioFormatReader>;
    using Gen = GeneratorInputBus;

    // clang-format off
    for (const auto& inputSource : audioInputs) {
        auto samples = std::visit(
            InputSourceVisitor{
                [&](const Reader& reader) {
                    return static_cast<std::size_t>(reader->lengthInSamples);
                },
                [&](const Gen& gen) {
                    return gen.getDurationInSamples(sampleRate);
                }
            },
            inputSource
        );
        maxLengthInSamples = std::max(maxLengthInSamples, samples);
    }
    // clang-format on

    return maxLengthInSamples;
}

int ProcessCommand::getBitDepthOfInput() const {
    int bitDepth{ 0 };

    using Reader = std::unique_ptr<juce::AudioFormatReader>;
    using Gen = GeneratorInputBus;

    // clang-format off
    for (const auto& inputSource : audioInputs) {
        std::visit(
            InputSourceVisitor{
                [&](const Reader& reader) {
                    const auto candidateBits = static_cast<int>(reader->bitsPerSample);
                    if (bitDepth == 0) {
                        bitDepth = candidateBits;
                        return;
                    }
                    if (bitDepth != candidateBits){
                        std::print(stderr, "Audio file bit depth {} does not match previous bit depth {}. Using previous bit depth.", candidateBits, bitDepth);
                        return;
                    }
                },
                [&](const Gen&) {
                    bitDepth = 16;
                }
            },
            inputSource
        );
    }
    // clang-format on

    return bitDepth;
}

juce::Array<juce::AudioChannelSet> ProcessCommand::getInputBusesLayoutFromAudioInputs() const {
    using Reader = std::unique_ptr<juce::AudioFormatReader>;
    using Gen = GeneratorInputBus;

    juce::Array<juce::AudioChannelSet> ret;

    for (const auto& inputSource : audioInputs) {
        ret.add(
            std::visit(
                InputSourceVisitor{
                    [&](const Reader& reader) { return reader->getChannelLayout(); },
                    [&](const Gen& gen) { return gen.getChannelLayout(); } },
                inputSource
            )
        );
    }
    return ret;
}

juce::Array<juce::AudioChannelSet>
ProcessCommand::getOutputBusesLayout(const juce::AudioPluginInstance& plugin) const {
    juce::Array<juce::AudioChannelSet> ret;
    if (outputChannelCountOpt.has_value()) {
        ret.add(
            juce::AudioChannelSet::canonicalChannelSet(static_cast<int>(*outputChannelCountOpt))
        );
    } else {
        const auto pluginOutputLayout = plugin.getChannelLayoutOfBus(false, 0);
        if (pluginOutputLayout.size() == 0) {
            throw PluginError{ "Couldn't get an output bus from the plugin.", 45 };
        }
        ret.add(pluginOutputLayout);
    }

    return ret;
}

void ProcessCommand::negotiateBusesLayout(juce::AudioPluginInstance& plugin) {
    auto setAndCheck = [&](const juce::AudioProcessor::BusesLayout& layout) {
        auto result = plugin.setBusesLayout(layout);
        // setBusesLayout can give different results to checkBusesLayoutSupported
        // if the plugin has overriden canApplyBusesLayout
        if (!result) {
            throw PluginError("Unable to set plugin to a buses layout it declared supported.", 61);
        }
    };

    // Only support single-output plugins
    if (!PluginUtils::pluginSupportsSingleOutputBus(plugin)) {
        throw PluginError{
            "The plugin does not support a single output bus and Plugalyzer does not "
            "support multiple outputs.",
            62
        };
    }

    // Code path of least resistance: the natural buses layout is compatible

    // clang-format off
    juce::AudioProcessor::BusesLayout candidateLayout {
        .inputBuses = getInputBusesLayoutFromAudioInputs(),
        .outputBuses = getOutputBusesLayout(plugin)
    };
    // clang-format on

    if (plugin.checkBusesLayoutSupported(candidateLayout)) {
        setAndCheck(candidateLayout);
        return;
    }
    std::println(
        stderr,
        "The inputs provided produce the following layout:\n{}\n"
        "But the plugin does not support it. Trying something else.",
        describeBusesLayout(candidateLayout).toStdString()
    );

    // Second choice: ditch extraneous user inputs
    {
        auto numInputBuses = candidateLayout.inputBuses.size();
        while (numInputBuses > 0) {
            auto inBuses = getInputBusesLayoutFromAudioInputs();
            juce::Array<juce::AudioChannelSet> inBusesSubset{ inBuses.data(), --numInputBuses };

            candidateLayout.inputBuses = inBusesSubset;
            if (plugin.checkBusesLayoutSupported(candidateLayout)) {
                setAndCheck(candidateLayout);
                const auto droppedInputs = candidateLayout.inputBuses.size() - numInputBuses;
                std::println(
                    stderr, "Dropped {} inputs. Using busesLayout: {}", droppedInputs,
                    describeBusesLayout(candidateLayout).toStdString()
                );
                return;
            }
        }
    }

    // Third choice: give the plugin its default
    {
        const auto candidateInputBuses = getInputBusesLayoutFromAudioInputs();
        const auto pluginInputBuses = plugin.getBusesLayout().inputBuses;

        // See if we can match the plugin's desired channel layouts with the inputs we have
        for (int i{ 0 }; i < pluginInputBuses.size() && i < candidateInputBuses.size(); ++i) {
            if (candidateInputBuses[i].size() != pluginInputBuses.size()) {
                throw PluginError("Can't find a buses layout to satisfy the plugin.", 63);
            }
        }
        // We can offer the plugin its default layout using the channels we have in the audio
        // inputs, padding any extra channels with silence
        if (candidateInputBuses.size() < pluginInputBuses.size()) {
            std::println(
                stderr, "Adding {} silent inputs.",
                pluginInputBuses.size() - candidateInputBuses.size()
            );
        }
        candidateLayout = plugin.getBusesLayout();
        setAndCheck(candidateLayout);
        std::println(
            stderr, "Using busesLayout {}", describeBusesLayout(candidateLayout).toStdString()
        );
    }
}

void ProcessCommand::prepareAudioInputs(Hertz currentSampleRate, int currentBlockSize) {
    using Reader = std::unique_ptr<juce::AudioFormatReader>;
    using Gen = GeneratorInputBus;

    // clang-format off
    for (auto& inputSource : audioInputs) {
        std::visit(
            InputSourceVisitor{
                [&](Reader&) {},
                [&](Gen& gen) { return gen.prepare(currentSampleRate, static_cast<juce::uint32>(currentBlockSize)); }
            },
            inputSource
        );
    }
    // clang-format on
}

void ProcessCommand::renderAudioInput(juce::AudioBuffer<float>& buffer, size_t sampleIndex) {
    using Reader = std::unique_ptr<juce::AudioFormatReader>;
    using Gen = GeneratorInputBus;

    unsigned int bufferChannelIndex{ 0 };
    unsigned int inputFileIndex{ 0 };

    for (auto& inputSource : audioInputs) {
        std::visit(
            InputSourceVisitor{
                [&](Reader& inputFile) {
                    auto success = inputFile->read(
                        buffer.getArrayOfWritePointers() + bufferChannelIndex,
                        (int) inputFile->numChannels, static_cast<juce::int64>(sampleIndex),
                        (int) blockSize
                    );
                    if (!success)
                        throw FileLoadError(
                            std::format("Error reading input file {}", inputFileIndex), 103
                        );
                    inputFileIndex++;
                    bufferChannelIndex += inputFile->numChannels;
                },
                [&](Gen& gen) {
                    const auto numChannels =
                        static_cast<std::size_t>(gen.getChannelLayout().size());
                    auto block = juce::dsp::AudioBlock<float>{ buffer }.getSubsetChannelBlock(
                        bufferChannelIndex, numChannels
                    );
                    gen.processChannels(block);
                    bufferChannelIndex += numChannels;
                } },
            inputSource
        );
    }
}
