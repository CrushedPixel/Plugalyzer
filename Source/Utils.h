#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <nlohmann/json.hpp>

enum class OutputFormat { text, json };

OutputFormat parseOutputFormat(const char* formatName);
OutputFormat parseOutputFormat(const std::string& formatName);

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
 * Validates that the parent directory of an output file path exists.
 * You can use this as a non-mutating validator for the CLI option->check() function
 *
 * @param arg The file path to validate
 * @return Empty string if valid, or an error message if the parent directory does
 *         not exist
 */
std::string validateOutputPath(const std::string& arg);

struct CLIException : std::runtime_error {
    explicit CLIException(const std::string& message) : std::runtime_error(message) {}
    explicit CLIException(const char* message) : std::runtime_error(message) {}
    explicit CLIException(const juce::String& message)
        : std::runtime_error(message.toStdString()) {}
};

/**
 * Converts the given time in seconds to samples given the sample rate.
 *
 * @param sec The time to convert, in seconds.
 * @param sampleRate The sample rate.
 * @return The time in samples.
 */
size_t secondsToSamples(double sec, double sampleRate);

/**
 * Parses a string into a floating-point number.
 * If not the entire string is a number, an error is thrown,
 * unlike <code>std::stof</code> which only returns the leading number in such a case.
 *
 * @param str The string to parse.
 * @return The parsed number.
 * @throws std::invalid_argument If the input is not a valid number.
 */
float parseFloatStrict(const std::string& str);

/**
 * Parses a string into an unsigned long number.
 * If not the entire string is a number, an error is thrown,
 * unlike <code>std::stoul</code> which only returns the leading number in such a case.
 *
 * @param str The string to parse.
 * @return The parsed number.
 * @throws std::invalid_argument If the input is not a valid number.
 */
unsigned long parseULongStrict(const std::string& str);

class PluginUtils {
  public:
    /**
     * Loads and initializes an audio plugin.
     *
     * @param pluginPath The plugin's file path.
     * @param initialSampleRate The sample rate to initialize the plugin with.
     * @param initialBlockSize The buffer size to initialize the plugin with.
     * @return The initialized plugin.
     */
    static std::unique_ptr<juce::AudioPluginInstance>
    createPluginInstance(const juce::String& pluginPath, double initialSampleRate,
                         int initialBlockSize);

    /**
     * Finds the plugin's parameter with the given name.
     *
     * @param plugin The plugin.
     * @param parameterName The parameter name.
     * @return The parameter.
     * @throws std::runtime_error If the plugin has no parameter with the given name.
     */
    static juce::AudioProcessorParameter*
    getPluginParameterByName(const juce::AudioPluginInstance& plugin,
                             const std::string& parameterName);
};

/**
 * Get the possible values of a discrete parameter (choice, bool).
 *
 * @param param The plugin parameter for which to get the values
 * @return Array of values, empty if the parameter is not discrete
 */
juce::StringArray getDiscreteValueStrings(const juce::AudioProcessorParameter& param);

/**
 * Get all possible channel sets in JUCE
 *
 * @return Array of channel sets
 */
juce::Array<juce::AudioChannelSet> allChannelSets();

/**
 * Converts a BusesLayout to a JSON representation with channel information
 *
 * @param layout The AudioProcessor BusesLayout to convert
 * @return JSON object containing input and output buses with descriptions and
 *         channel counts
 */
nlohmann::json getBusLayoutJson(const juce::AudioProcessor::BusesLayout& layout);

/**
 * Converts bus layout JSON to a human-readable string description
 *
 * @param layoutJson The JSON object from getBusLayoutJson()
 * @return Formatted string describing all input and output buses with their channel
 *         counts
 */
std::string getBusLayoutHumanReadable(const nlohmann::json& layoutJson);

/**
 * Outputs text either to stdout or to a file.
 * Used for outputting the final result of a command according to the options set by the user.
 *
 * @param text The text content to output
 * @param outPath The output file path. If not supplied, outputs to stdout instead
 * @param overwrite If true, overwrites the file if it exists. If false, creates a new file with a
 *                  unique name
 */
void outputResult(const std::string& text, juce::File outPath = {}, bool overwrite = true);
