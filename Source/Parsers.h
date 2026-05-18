#pragma once

#include "Generators.h"
#include "Utils.h"

#include <chrono>
#include <juce_audio_formats/juce_audio_formats.h>
#include <type_traits>
#include <utility>

// Parsers for CLI11
// Signature:
// YourObject parse(const std::string& arg)
// Usage:
// option->each([&](std::string arg){ memberVariable = parse(arg); });

namespace parse {

template<typename T>
concept Numeric = std::is_same_v<T, int> || std::is_same_v<T, long> || std::is_same_v<T, double> ||
    std::is_same_v<T, float>;

/**
 * Parses a string into a number and units.
 *
 * If the input contains only digits with no units, the units string
 * will be empty.
 *
 * @param str The input string to parse.
 *
 * @return A pair where:
 *         - @c first is the parsed number
 *         - @c second is the units suffix as a string
 *
 * @throws std::invalid_argument if no digits are found in the input
 */
template<Numeric T>
std::pair<T, std::string> numberAndUnits(const std::string& str) {
    if (str.empty()) {
        throw std::invalid_argument("Input string is empty");
    }

    size_t pos{ 0 };
    T value;

    if constexpr (std::is_same_v<T, int>) {
        value = std::stoi(str, &pos);
    } else if constexpr (std::is_same_v<T, long>) {
        value = std::stol(str, &pos);
    } else if constexpr (std::is_same_v<T, double>) {
        value = std::stod(str, &pos);
    } else if constexpr (std::is_same_v<T, float>) {
        value = std::stof(str, &pos);
    }

    const auto units{ str.substr(pos) };
    const auto unitsStripped = string_utils::strip(units);

    return { value, std::string{ unitsStripped } };
}

std::chrono::seconds seconds(const std::string& secondsString);

double amplitude(const std::string& amplitudeString);

Hertz frequency(const std::string& freqString);

GeneratorInputBus generatorInput(const std::string& jsonStringOrFilePath);

OutputFormat outputFormat(const std::string& formatName);

/* Find the sample rate in the top level of the JSON object and return it */
double extractSampleRate(const std::string& jsonString);

/**
 * Converts a string file path to a juce::File object.
 * Useful for accepting relatve file path CLI arguments and
 * making a juce Path without getting an assertion.
 *
 * @param filePath The file path as a string. Can be absolute or relative.
 * @return The file path.
 *         If the path is absolute, it's used directly.
 *         If the path is relative, it's resolved relative to the current working
 *         directory.
 */
juce::File stringToFile(const std::string& filePath);

/**
 * Parses a string into a floating-point number.
 * If not the entire string is a number, an error is thrown,
 * unlike <code>std::stof</code> which only returns the leading number in such a case.
 *
 * @param str The string to parse.
 * @return The parsed number.
 * @throws std::invalid_argument If the input is not a valid number.
 */
float floatStrict(const std::string& str);

/**
 * Parses a string into an unsigned long number.
 * If not the entire string is a number, an error is thrown,
 * unlike <code>std::stoul</code> which only returns the leading number in such a case.
 *
 * @param str The string to parse.
 * @return The parsed number.
 * @throws std::invalid_argument If the input is not a valid number.
 */
unsigned long uLongStrict(const std::string& str);

/**
 * Parses a plugin parameter string in the format <key>:<value>[:n].
 *
 * @param str The string to parse.
 * @return The parsed parameter argument.
 * @throws CLIException If the input string is not formatted correctly.
 */
ParameterCLIArgument pluginParameterArgument(const std::string& str);

} // namespace parse
