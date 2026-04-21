#pragma once

#include <CLI/CLI.hpp>
#include <juce_audio_processors/juce_audio_processors.h>

#include <stdexcept>

class FileLoadError : public CLI::Error {
  public:
    FileLoadError(std::string msg, int exit_code)
        : CLI::Error("FileLoadError", std::move(msg), exit_code) {}
};

struct CLIException : std::runtime_error {
    explicit CLIException(const std::string& message) : std::runtime_error(message) {}
    explicit CLIException(const char* message) : std::runtime_error(message) {}
    explicit CLIException(const juce::String& message)
        : std::runtime_error(message.toStdString()) {}
};