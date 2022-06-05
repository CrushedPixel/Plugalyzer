#pragma once

#include "CLI/CLI.hpp"

class CLICommand {
  public:
    virtual ~CLICommand() = default;

    /**
     * Creates a CLI::App that can be registered as a subcommand for a parent app.
     */
    virtual std::shared_ptr<CLI::App> createApp() = 0;

    /**
     * Executes this command.
     */
    virtual void execute() = 0;
};
