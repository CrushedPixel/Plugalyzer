#include "Plugalyzer.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

std::vector<juce::File> getInputFileArguments(const juce::ArgumentList& _args) {
    // create a copy to keep the original argument list unmodified
    // when consuming the input options
    juce::ArgumentList args = _args;

    std::vector<juce::File> inputFiles;
    while (args.containsOption("--input")) {
        inputFiles.push_back(args.getExistingFileForOptionAndRemove("--input"));
    }

    return inputFiles;
}

std::optional<juce::File> getMidiInputFileArgument(const juce::ArgumentList& args) {
    std::optional<juce::File> midiFile;
    if (args.containsOption("--midiInput")) {
        midiFile = args.getExistingFileForOption("--midiInput");
    }

    return midiFile;
}

juce::File getOutputFileArgument(const juce::ArgumentList& args) {
    args.failIfOptionIsMissing("--output");
    auto outputFile = args.getFileForOption("--output");
    if (outputFile.existsAsFile() && !args.containsOption("--overwrite")) {
        juce::ConsoleApplication::fail(
            "Output file already exists! Use --overwrite to overwrite the file.");
    }

    return outputFile;
}

std::vector<std::pair<juce::String, juce::String>>
getParameterArguments(const juce::ArgumentList& _args) {
    // create a copy to keep the original argument list unmodified
    // when consuming the parameter options
    juce::ArgumentList args = _args;

    std::vector<std::pair<juce::String, juce::String>> params;

    while (args.containsOption("--param")) {
        // format: --param="Parameter Name":-12.0
        // quotes are supported for both key and value
        auto paramStr = args.removeValueForOption("--param");
        juce::StringArray tokens;
        tokens.addTokens(paramStr, ":", "\"'");

        if (tokens.size() != 2) {
            juce::ConsoleApplication::fail("Invalid parameter syntax: " + paramStr);
        }

        params.emplace_back(tokens[0], tokens[1]);
    }

    return params;
}

juce::String getPluginArgument(const juce::ArgumentList& args) {
    args.failIfOptionIsMissing("--plugin");
    return args.getValueForOption("--plugin");
}

std::optional<double> getSampleRateArgument(const juce::ArgumentList& args) {
    // parse optional sample rate option
    std::optional<double> sampleRateOpt;
    if (args.containsOption("--sampleRate")) {
        sampleRateOpt = args.getValueForOption("--sampleRate").getDoubleValue();
        if (*sampleRateOpt <= 0) {
            juce::ConsoleApplication::fail("sampleRate must be a positive number");
        }
    }

    return sampleRateOpt;
}

std::optional<int> getBlockSizeArgument(const juce::ArgumentList& args) {
    // parse optional block size option
    std::optional<int> blockSizeOpt;
    if (args.containsOption("--blockSize")) {
        blockSizeOpt = args.getValueForOption("--blockSize").getIntValue();
        if (*blockSizeOpt <= 0) {
            juce::ConsoleApplication::fail("blockSize must be a positive number");
        }
    }

    return blockSizeOpt;
}

std::optional<int> getOutputChannelCountArgument(const juce::ArgumentList& args) {
    std::optional<int> numOutputChannelsOpt;
    if (args.containsOption("--outChannels")) {
        numOutputChannelsOpt = args.getValueForOption("--outChannels").getIntValue();
        if (*numOutputChannelsOpt <= 0) {
            juce::ConsoleApplication::fail("outChannels must be a positive number");
        }
    }

    return numOutputChannelsOpt;
}

void runProcessCommand(const juce::ArgumentList& args) {
    // parse plugin option
    auto pluginPath = getPluginArgument(args);

    // parse input file option(s).
    // each input file will be sent to the plugin in a separate bus.
    auto audioInputFiles = getInputFileArguments(args);
    auto midiInputFileOpt = getMidiInputFileArgument(args);

    if (audioInputFiles.empty() && !midiInputFileOpt) {
        juce::ConsoleApplication::fail("Either audio or MIDI input must be provided.");
    }

    auto outputFile = getOutputFileArgument(args);

    auto sampleRateOpt = getSampleRateArgument(args);
    if (sampleRateOpt && !audioInputFiles.empty()) {
        juce::ConsoleApplication::fail(
            "The --sampleRate argument is not allowed in combination with the --input argument. "
            "When audio input files are supplied, their sample rate is used.");
    }

    auto blockSizeOpt = getBlockSizeArgument(args);

    // parse optional output channel count option
    auto numOutputChannelsOpt = getOutputChannelCountArgument(args);

    // parse parameters to set
    auto params = getParameterArguments(args);

    process(pluginPath, audioInputFiles, midiInputFileOpt, outputFile,
            sampleRateOpt.value_or(44100), blockSizeOpt.value_or(1024), numOutputChannelsOpt,
            params);
}

void runListParametersCommand(const juce::ArgumentList& args) {
    auto pluginPath = getPluginArgument(args);
    auto sampleRateOpt = getSampleRateArgument(args);
    auto blockSizeOpt = getBlockSizeArgument(args);

    listParameters(pluginPath, sampleRateOpt.value_or(44100), blockSizeOpt.value_or(1024));
}

void runCommandLine(const juce::String& commandLineParameters) {
    juce::ArgumentList args("", commandLineParameters);

    if (args.arguments.isEmpty()) {
        juce::ConsoleApplication::fail(
            "Must have at least one argument to indicate the desired action");
    }
    auto command = args.arguments.removeAndReturn(0).text;
    if (command == "process") {
        runProcessCommand(args);
    } else if (command == "listParameters") {
        runListParametersCommand(args);
    } else {
        juce::ConsoleApplication::fail("Invalid command: " + command);
    }
}

class PlugalyzerApplication : public juce::JUCEApplicationBase {
    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }

    bool moreThanOneInstanceAllowed() override { return true; }

    void anotherInstanceStarted(const juce::String& commandLine) override {}
    void suspended() override {}
    void resumed() override {}
    void shutdown() override {}

    void systemRequestedQuit() override { quit(); }

    void unhandledException(const std::exception* exception, const juce::String& sourceFilename,
                            int lineNumber) override {
        // TODO
        std::cerr << exception << std::endl;
    }

    void initialise(const juce::String& commandLineParameters) override {
        auto exitCode =
            juce::ConsoleApplication::invokeCatchingFailures([&commandLineParameters]() {
                runCommandLine(commandLineParameters);
                return 0;
            });

        setApplicationReturnValue(exitCode);
        quit();
    }
};

START_JUCE_APPLICATION(PlugalyzerApplication)
