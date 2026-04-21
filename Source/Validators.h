#pragma once

#include <string>

namespace validate {
/**
 * Validates that the parent directory of an output file path exists.
 * You can use this as a non-mutating validator for the CLI option->check() function
 *
 * @param arg The file path to validate
 * @return Empty string if valid, or an error message if the parent directory does
 *         not exist
 */
std::string outputPath(const std::string& arg);

/**
 * Validates that the passed argument is either 'binary' or 'xml'.
 * You can use this as a non-mutating validator for the CLI option->check() function
 *
 * @param arg The argument
 * @return Empty string if valid, or an error message
 */
std::string binaryOrXml(const std::string& arg);

/**
 * Validates the format of a plugin parameter passed via CLI to be "<key>:<value>".
 * This does not validate if the parameter exists on a plugin.
 * 
 * @param str The plugin parameter argument
 * @return Empty string if valid, or an error message
 */
std::string pluginParameter(const std::string& str);

/**
 * Supported bit depths: 8, 16, 24, or 32
 * 
 * @param str The bit depth argument
 * @return Empty string if valid, or an error message
 */
std::string bitDepth(const std::string& str);

} // namespace validate