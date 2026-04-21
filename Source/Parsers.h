#pragma once

#include "Utils.h"

// Parsers for CLI11
// Signature:
// YourObject parse(const std::string& arg)
// Usage:
// option->each([&](std::string arg){ memberVariable = parse(arg); });

namespace parse
{
OutputFormat outputFormat(const std::string& formatName);

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

} // namespace Parse

