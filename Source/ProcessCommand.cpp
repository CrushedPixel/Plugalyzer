#include "ProcessCommand.h"

#include "PresetLoadingExtensionsVisitor.h"
#include "Utils.h"

struct ParameterCLIArgument {
    std::string parameterName;

    // I'd use a union for this, but compiler says no
    // https://stackoverflow.com/a/70428826
    std::string textValue;
    float normalizedValue;

    bool isNormalizedValue;
};

/**
 * Parses a plugin parameter string in the format <key>:<value>[:n].
 *
 * @param str The string to parse.
 * @return The parsed parameter argument.
 * @throws CLIException If the input string is not formatted correctly.
 */
ParameterCLIArgument parsePluginParameterArgument(const std::string& str) {
    juce::StringArray tokens;
    tokens.addTokens(str, ":", "\"'");

    bool isNormalizedValue = tokens.size() == 3;

    if (isNormalizedValue && tokens[2] != "n") {
        throw CLIException("Invalid parameter modifier: '" + tokens[2] + "'. Only 'n' is allowed");
    }

    if (tokens.size() != 2 && tokens.size() != 3) {
        throw CLIException("'" + str + "' is not a colon-separated key-value pair");
    }

    std::string valueStr = tokens[1].toStdString();
    if (isNormalizedValue) {
        float normalizedValue;
        try {
            normalizedValue = parseFloatStrict(valueStr);
        } catch (const std::invalid_argument& ia) {
            throw CLIException("Normalized parameter value must be a number, but is '" + valueStr +
                               "'");
        }

        if (normalizedValue < 0 || normalizedValue > 1) {
            throw CLIException("Normalized parameter value must be between 0 and 1, but is " +
                               std::to_string(normalizedValue));
        }

        return {
            .parameterName = tokens[0].toStdString(),
            .normalizedValue = normalizedValue,
            .isNormalizedValue = true,
        };
    }

    return {
        .parameterName = tokens[0].toStdString(),
        .textValue = valueStr,
        .isNormalizedValue = false,
    };
}

namespace Validator {
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

struct BitDepthValidator : public CLI::Validator {
    BitDepthValidator() {
        name_ = "BIT_DEPTH";
        func_ = [](const std::string& str) {
            try {
                int value = std::stoi(str);
                if (value != 8 && value != 16 && value != 24 && value != 32) {
                    return std::string("Bit depth must be 8, 16, 24, or 32");
                }
            } catch (const std::exception& e) {
                return std::string("Bit depth must be a valid integer");
            }
            return std::string();
        };
    }
};

const static BitDepthValidator BitDepth;
} // namespace Validator

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
    app->add_option("-d,--bitDepth", outputBitDepthOpt, "The output file's bit depth. Defaults to the input file's bit depth if present, or 16 bits if no input file is provided.")->check(Validator::BitDepth);
    app->add_option("-c,--outChannels", outputChannelCountOpt, "The amount of channels to use for the plugin's output bus");

    app->add_option("--paramFile", paramsFileOpt, "Path to JSON file to read plugin parameters and automation data from")->check(CLI::ExistingFile);
    app->add_option("--param", params, "Plugin parameters to set. Explicitly specified parameters take precedence over parameters read from file")->check(Validator::PluginParameter);

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
    unsigned int totalNumInputChannels, totalNumOutputChannels;
    auto layout = createBusLayout(*plugin, audioInputFileReaders, outputChannelCountOpt,
                                  totalNumInputChannels, totalNumOutputChannels);
    if (!plugin->setBusesLayout(layout)) {
        throw CLIException("Plugin does not support requested bus layout");
    }

    // parse plugin parameters
    auto automation = parseParameters(*plugin, sampleRate, totalInputLength, paramsFileOpt, params);

    plugin->prepareToPlay(sampleRate, (int) blockSize);
    const auto latency = plugin->getLatencySamples();

    // open output stream
    if (outputFilePath.exists() && !overwriteOutputFile) {
        throw CLIException("Output file already exists! Use --overwrite to overwrite the file");
    }

    std::unique_ptr<juce::AudioFormatWriter> outWriter;
    {
        outputFilePath.deleteFile();
        auto outputStream = outputFilePath.createOutputStream(blockSize);
        if (!outputStream) {
            throw CLIException("Could not create output stream to write to file " +
                               outputFilePath.getFullPathName());
        }

        juce::WavAudioFormat outFormat;
        outWriter.reset(outFormat.createWriterFor(
            outputStream.release() /* stream is now managed by writer */, sampleRate,
            totalNumOutputChannels, bitDepth, juce::StringPairArray(), 0));
    }

    // process the input files with the plugin
    juce::AudioBuffer<float> sampleBuffer(
        (int) std::max(totalNumInputChannels, totalNumOutputChannels), (int) blockSize);

    juce::MidiBuffer midiBuffer;
    size_t sampleIndex = 0;
    int samplesSkipped = 0;
    while (sampleIndex < totalInputLength + latency) {
        sampleBuffer.clear();

        // read next segment of audio input files into buffer
        unsigned int targetChannel = 0;
        for (auto* inputFileReader : audioInputFileReaders) {
            if (!inputFileReader->read(sampleBuffer.getArrayOfWritePointers() + targetChannel,
                                       (int) inputFileReader->numChannels, sampleIndex,
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
                if (timestampSamples >= sampleIndex && timestampSamples < sampleIndex + blockSize) {
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

        sampleIndex += blockSize;
    }
}

juce::OwnedArray<juce::AudioFormatReader>
ProcessCommand::createAudioFileReaders(const std::vector<juce::File>& files,
                                       size_t& maxLengthInSamplesOut) {
    maxLengthInSamplesOut = 0;

    // parse the input files
    juce::AudioFormatManager audioFormatManager;
    audioFormatManager.registerBasicFormats();

    juce::OwnedArray<juce::AudioFormatReader> audioInputFileReaders;

    double sampleRate;
    for (size_t i = 0; i < files.size(); i++) {
        auto& inputFile = files[i];

        auto inputFileReader = audioFormatManager.createReaderFor(inputFile);
        if (!inputFileReader) {
            throw CLIException("Could not read input file " + inputFile.getFullPathName());
        }

        audioInputFileReaders.add(inputFileReader);

        // ensure the sample rate of all input files is the same
        if (i == 0) {
            sampleRate = inputFileReader->sampleRate;
        } else if (inputFileReader->sampleRate != sampleRate) {
            throw CLIException("Mismatched sample rate between input files");
        }

        maxLengthInSamplesOut =
            std::max(maxLengthInSamplesOut, (size_t) inputFileReader->lengthInSamples);
    }

    return audioInputFileReaders;
}

juce::MidiFile ProcessCommand::readMIDIFile(const juce::File& file, double sampleRate,
                                            size_t& lengthInSamplesOut) {
    juce::MidiFile midiFile;
    lengthInSamplesOut = 0;

    auto inputStream = file.createInputStream();
    if (!midiFile.readFrom(*inputStream, true)) {
        throw CLIException("Error reading MIDI input file");
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
            lengthInSamplesOut = std::max(lengthInSamplesOut, timestampSamples);
        }
    }

    return midiFile;
}

juce::AudioPluginInstance::BusesLayout ProcessCommand::createBusLayout(
    const juce::AudioPluginInstance& plugin,
    const juce::OwnedArray<juce::AudioFormatReader>& audioInputFileReaders,
    const std::optional<unsigned int>& outputChannelCountOpt,
    unsigned int& totalNumInputChannelsOut, unsigned int& totalNumOutputChannelsOut) {

    totalNumInputChannelsOut = 0;
    juce::AudioPluginInstance::BusesLayout layout;
    if (audioInputFileReaders.isEmpty()) {
        // if no input files are provided, use the plugin's default input bus layout
        // to maximize compatibility with synths that expect an input
        layout.inputBuses = plugin.getBusesLayout().inputBuses;
        for (auto& cs : layout.inputBuses) {
            totalNumInputChannelsOut += (unsigned int) cs.size();
        }

    } else {
        for (auto* inputFileReader : audioInputFileReaders) {
            layout.inputBuses.add(
                juce::AudioChannelSet::canonicalChannelSet((int) inputFileReader->numChannels));
            totalNumInputChannelsOut += inputFileReader->numChannels;
        }
    }

    // create an output bus with the desired amount of channels,
    // defaulting to the same amount of channels as the main input file if one exists,
    // or the plugin's default bus layout otherwise.
    totalNumOutputChannelsOut = (outputChannelCountOpt.value_or(
        audioInputFileReaders.isEmpty()
            ? (unsigned int) plugin.getBusesLayout().getMainOutputChannels()
            : audioInputFileReaders[0]->numChannels));
    layout.outputBuses.add(
        juce::AudioChannelSet::canonicalChannelSet((int) totalNumOutputChannelsOut));

    return layout;
}

ParameterAutomation
ProcessCommand::parseParameters(const juce::AudioPluginInstance& plugin, double sampleRate,
                                size_t inputLengthInSamples,
                                const std::optional<juce::File>& parameterFileOpt,
                                const std::vector<std::string>& cliParameters) {
    ParameterAutomation automation;

    // read automation from file
    if (parameterFileOpt) {
        automation = Automation::parseAutomationDefinition(
            parameterFileOpt->loadFileAsString().toStdString(), plugin, sampleRate,
            inputLengthInSamples);
    }

    // parse command-line supplied parameters
    for (auto& arg : cliParameters) {
        auto [paramName, textValue, normalizedValue, isNormalizedValue] =
            parsePluginParameterArgument(arg);

        // convert parameter value from text representation to a single keyframe,
        // which causes the same value to be applied over the entire duration
        auto* param = PluginUtils::getPluginParameterByName(plugin, paramName);

        if (!isNormalizedValue) {
            if (!Automation::parameterSupportsTextToValueConversion(param)) {
                throw CLIException("Parameter '" + paramName +
                                   "' does not support text values. Use :n suffix to supply "
                                   "a normalized value instead");
            }

            normalizedValue = param->getValueForText(textValue);
        }

        // warn the user if the parameter overrides a parameter specified in the file
        if (automation.contains(paramName)) {
            std::cout << "Plugin parameter '" << paramName
                      << "' is specified in the parameter file and overridden by a command-line "
                         "parameter."
                      << std::endl;
        }

        automation[paramName] = AutomationKeyframes({{0, normalizedValue}});
    }

    return automation;
}
