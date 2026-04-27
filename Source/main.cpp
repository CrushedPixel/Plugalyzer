#include "commands/AudioDiffCommand.h"
#include "commands/BusLayoutsCommand.h"
#include "commands/GenerateAutomationCommand.h"
#include "commands/ListParametersCommand.h"
#include "commands/StateCommand.h"
#include "commands/ProcessCommand.h"

#include <juce_events/juce_events.h>
#include <print>

static void registerSubcommand(CLI::App& app, CLICommand& subcommand) {
    app.add_subcommand(subcommand.createApp())->callback([&subcommand]() { subcommand.execute(); });
}

static int runCommandLine(const std::string& commandLineParameters) {
    CLI::App app("Command-line audio plugin host");

    app.add_flag_callback(
        "-v,--version",
        []() { std::println("Plugalyzer version {}", JUCE_APPLICATION_VERSION_STRING); },
        "Show version information"
    );

    app.footer("Use 'plugalyzer <subcommand> --help' to see options for each subcommand.");

    // set up subcommands
    ProcessCommand pc;
    registerSubcommand(app, pc);

    AudioDiffCommand adc;
    registerSubcommand(app, adc);

    ListParametersCommand lpc;
    registerSubcommand(app, lpc);

    GenerateAutomationCommand gac;
    registerSubcommand(app, gac);

    StateCommand msc;
    registerSubcommand(app, msc);

    BusLayoutsCommand blc;
    registerSubcommand(app, blc);

    try {
        app.parse(commandLineParameters, false);
    } catch (const CLI::Error& error) {
        return app.exit(error);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}

class PlugalyzerApplication : public juce::JUCEApplicationBase {
  public:
    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }

    bool moreThanOneInstanceAllowed() override { return true; }

    void anotherInstanceStarted(const juce::String& /* commandLine */) override {}
    void suspended() override {}
    void resumed() override {}
    void shutdown() override {}

    void systemRequestedQuit() override { quit(); }

    void unhandledException(
        const std::exception* /* exception */, const juce::String& /* sourceFilename */,
        int /* lineNumber */
    ) override {
        // for some reason, this doesn't actually get called and the runtime just terminates...
    }

    void initialise(const juce::String& commandLineParameters) override {
        int exitCode = runCommandLine(commandLineParameters.toStdString());

        setApplicationReturnValue(exitCode);
        quit();
    }
};

START_JUCE_APPLICATION(PlugalyzerApplication)
