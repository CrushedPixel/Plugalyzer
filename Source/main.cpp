#include "Plugalyzer.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

std::vector<juce::File> getInputFileArguments(const juce::ArgumentList& _args) {
    // create a copy to keep the original argument list unmodified
    // when consuming the input options
    juce::ArgumentList args = _args;

    std::vector<juce::File> inputFiles;
    args.failIfOptionIsMissing("--input");
    while (args.containsOption("--input")) {
        inputFiles.push_back(args.getExistingFileForOptionAndRemove("--input"));
    }

    return inputFiles;
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

juce::String getPluginArgument(const juce::ArgumentList& args) {
    args.failIfOptionIsMissing("--plugin");
    return args.getValueForOption("--plugin");
}

int getBlockSizeArgument(const juce::ArgumentList& args) {
    // parse optional block size option
    int blockSize = 1024;
    if (args.containsOption("--blockSize")) {
        blockSize = args.getValueForOption("--blockSize").getIntValue();
        if (blockSize <= 0) {
            juce::ConsoleApplication::fail("blockSize must be a positive number");
        }
    }

    return blockSize;
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

double getSampleRateArgument(const juce::ArgumentList& args) {
    // parse optional sample rate option
    double sampleRate = 44100;
    if (args.containsOption("--sampleRate")) {
        sampleRate = args.getValueForOption("--sampleRate").getDoubleValue();
        if (sampleRate <= 0) {
            juce::ConsoleApplication::fail("sampleRate must be a positive number");
        }
    }

    return sampleRate;
}

void runProcessCommand(const juce::ArgumentList& args) {
    // parse plugin option
    auto pluginPath = getPluginArgument(args);

    // parse input file option(s).
    // each input file will be sent to the plugin in a separate bus.
    auto inputFiles = getInputFileArguments(args);

    auto outputFile = getOutputFileArgument(args);
    auto blockSize = getBlockSizeArgument(args);

    // parse optional output channel count option
    auto numOutputChannelsOpt = getOutputChannelCountArgument(args);

    process(pluginPath, inputFiles, outputFile, blockSize, numOutputChannelsOpt);
}

void runListParametersCommand(const juce::ArgumentList& args) {
    auto pluginPath = getPluginArgument(args);
    auto sampleRate = getSampleRateArgument(args);
    auto blockSize = getBlockSizeArgument(args);

    listParameters(pluginPath, sampleRate, blockSize);
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
