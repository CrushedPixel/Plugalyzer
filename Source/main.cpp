#include "Plugalyzer.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

void runCommandLine(const juce::String& commandLineParameters) {
    juce::ArgumentList args("", commandLineParameters);

    // parse plugin option
    args.failIfOptionIsMissing("--plugin");
    auto pluginPath = args.getValueForOption("--plugin");

    // parse input file option(s).
    // each input file will be sent to the plugin in a separate bus.
    std::vector<juce::File> inputFiles;
    args.failIfOptionIsMissing("--input");
    while (args.containsOption("--input")) {
        inputFiles.push_back(args.getExistingFileForOptionAndRemove("--input"));
    }

    // parse output file option
    args.failIfOptionIsMissing("--output");
    auto outputFile = args.getFileForOption("--output");
    if (outputFile.existsAsFile() && !args.containsOption("--overwrite")) {
        juce::ConsoleApplication::fail(
            "Output file already exists! Use --overwrite to overwrite the file.");
    }

    // parse optional block size option
    int blockSize = 1024;
    if (args.containsOption("--blockSize")) {
        blockSize = args.getValueForOption("--blockSize").getIntValue();
        if (blockSize <= 0) {
            juce::ConsoleApplication::fail("blockSize must be a positive number");
        }
    }

    // parse optional output channel count option
    std::optional<int> numOutputChannelsOpt;
    if (args.containsOption("--outChannels")) {
        numOutputChannelsOpt = args.getValueForOption("--outChannels").getIntValue();
        if (*numOutputChannelsOpt <= 0) {
            juce::ConsoleApplication::fail("outChannels must be a positive number");
        }
    }

    plugalyze(pluginPath, inputFiles, outputFile, blockSize, numOutputChannelsOpt);
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
