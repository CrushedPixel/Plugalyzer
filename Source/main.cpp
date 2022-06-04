#include "ListParametersCommand.h"
#include "ProcessCommand.h"
#include <juce_events/juce_events.h>

void registerSubcommand(CLI::App& app, CLICommand &subcommand) {
    app.add_subcommand(subcommand.createApp())->callback([&subcommand]() {
        subcommand.execute();
    });
}

void runCommandLine(const std::string& commandLineParameters) {
    CLI::App app("Command-line audio plugin host");

    // set up subcommands
    ProcessCommand pc;
    registerSubcommand(app, pc);

    ListParametersCommand lpc;
    registerSubcommand(app, lpc);

    app.require_subcommand();
    app.parse(commandLineParameters, false);
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
                runCommandLine(commandLineParameters.toStdString());
                return 0;
            });

        setApplicationReturnValue(exitCode);
        quit();
    }
};

START_JUCE_APPLICATION(PlugalyzerApplication)
