#pragma once

#include <CLI/CLI.hpp>
#include <juce_core/juce_core.h>
#include <stdexcept>
#include <string>
#include <utility>

class ParseError : public CLI::Error {
  public:
    ParseError(std::string msg, int exit_code)
        : CLI::Error("ParseError", std::move(msg), exit_code) {}
};

class FileLoadError : public CLI::Error {
  public:
    FileLoadError(std::string msg, int exit_code)
        : CLI::Error("FileLoadError", std::move(msg), exit_code) {}
};

class PluginError : public CLI::Error {
  public:
    PluginError(std::string msg, int exit_code)
        : CLI::Error("PluginError", std::move(msg), exit_code) {}
};

struct CLIException : std::runtime_error {
    explicit CLIException(const std::string& message) : std::runtime_error(message) {}
    explicit CLIException(const char* message) : std::runtime_error(message) {}
    explicit CLIException(const juce::String& message)
        : std::runtime_error(message.toStdString()) {}
};

class FailedXmlError : public CLI::Error {
  public:
    FailedXmlError(std::string msg, int exit_code)
        : CLI::Error("FailedXmlError", std::move(msg), exit_code) {}
};