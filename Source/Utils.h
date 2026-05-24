#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <juce_audio_processors/juce_audio_processors.h>
#include <nlohmann/json.hpp>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using Hertz = double;
constexpr double operator""_Hz(long double frequency) { return static_cast<double>(frequency); }

enum class OutputFormat { text, json, binary, xml };

inline const std::unordered_map<std::string, OutputFormat> formatMap{
    { "text", OutputFormat::text },
    { "json", OutputFormat::json },
    { "binary", OutputFormat::binary },
    { "xml", OutputFormat::xml },
};

template<typename T>
concept EqualityComparable = requires(const T& a, const T& b) {
    { a == b } -> std::convertible_to<bool>;
};

template<EqualityComparable ContainedType>
bool vectorContains(const std::vector<ContainedType>& vec, ContainedType toFind) {
    return std::find(vec.begin(), vec.end(), toFind) != vec.end();
}

namespace string_utils {
/**
 * Displays a map as:
 * option1, option2 or option 3
 *
 * @param map The map to present
 * @return a human-readable string
 */
template<typename T>
std::string presentMapKeysAsOptions(const std::unordered_map<std::string, T> map) {
    if (map.size() == 0) {
        return "";
    }

    std::string presentable{};
    std::vector<std::string> keys{};

    std::size_t vecIdx{ 0 };
    for (const auto& [key, _] : map) {
        keys[vecIdx++] = key;
    }

    if (keys.size() == 1) {
        return keys[0];
    }

    for (std::size_t i{ 0 }; i < keys.size() - 2; ++i) {
        presentable += keys[i] + ", ";
    }
    presentable += keys[keys.size() - 2];
    presentable += " or ";
    presentable += keys[keys.size() - 1];

    return presentable;
}

/**
 * Get a string with all uppercase characters in the input string changed to their lowercase
 * equivalents.
 *
 * This function uses the C locale for character classification and
 * conversion. Behavior with non-ASCII characters depends on the
 * system locale.
 *
 * @return A new string with all alphabetic characters converted to lowercase.
 */
std::string lowerCase(const std::string_view str);

/**
 * @brief Return a view of the input string with leading and trailing ASCII whitespace removed.
 *
 * @param s The input string to trim
 *
 * @return std::string_view A stripped view of @p s or a null string_view if it was all whitespace.
 */
std::string_view strip(std::string_view s);

/**
 * Joins some strings into a single string with a separator.
 *
 * @param strs The strings
 * @param separator The thing to go between the strings. Can be empty.
 *
 * @return A new string containing all input strings concatenated with the separator.
 *         Returns an empty string if the input vector is empty.
 */
std::string join(std::ranges::bidirectional_range auto strs, const std::string_view separator) {
    std::stringstream ss;
    auto it = std::ranges::begin(strs);
    auto end = std::ranges::end(strs);

    if (it == end) return "";

    auto last = std::prev(end);
    while (it != last) {
        ss << *it++;
        ss << separator;
    }

    ss << *it;
    return ss.str();
}

} // namespace string_utils

/**
 * Parses JSON from either a file path or a JSON string.
 *
 * @param filePathOrJsonString A file path or JSON string to parse.
 * @return A parsed JSON object.
 *
 * @throws nlohmann::json::parse_error if the string or file contents are not valid JSON.
 */
nlohmann::json getJson(const std::string& filePathOrJsonString);

struct ParameterCLIArgument {
    std::string parameterName;

    // I'd use a union for this, but compiler says no
    // https://stackoverflow.com/a/70428826
    std::string textValue;
    float normalizedValue;

    bool isNormalizedValue;
};

/**
 * Converts the given time in seconds to samples given the sample rate.
 *
 * @param sec The time to convert, in seconds.
 * @param sampleRate The sample rate.
 * @return The time in samples.
 */
size_t secondsToSamples(double sec, double sampleRate);

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
    static std::unique_ptr<juce::AudioPluginInstance> createPluginInstance(
        const juce::String& pluginPath, double initialSampleRate, int initialBlockSize
    );

    /**
     * Finds the plugin's parameter with the given name.
     *
     * @param plugin The plugin.
     * @param parameterName The parameter name.
     * @return The parameter.
     * @throws std::runtime_error If the plugin has no parameter with the given name.
     */
    static juce::AudioProcessorParameter* getPluginParameterByName(
        const juce::AudioPluginInstance& plugin, const std::string& parameterName
    );

    static bool pluginSupportsSingleOutputBus(const juce::AudioPluginInstance& plugin);
};

/**
 * Loads a saved plugin state from file and applies it to the plugin.
 * The state should have been saved as binary using the manage state command.
 * Will throw if the file couldn't be opened, but has no idea if the plugin accepted/understood the
 * binary blob it was given.
 *
 * @param plugin The plugin.
 * @param statePath The path to the binary state
 * @param state Memory into which to write the state
 * @throws FileLoadError If the state file couldn't be opened.
 */
void loadPluginStateFromFile(
    juce::AudioPluginInstance& plugin, const juce::File& statePath, juce::MemoryBlock& state
);

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
 * Find out the total number of input channels across all buses in order to make the right sized
 * AudioBuffer
 *
 * @param layout The buses layout
 * @return The total number of input channels
 */
int getTotalNumInputChannels(const juce::AudioProcessor::BusesLayout& layout);

/**
 * Find out the total number of output channels across all buses in order to make the right sized
 * AudioBuffer
 *
 * @param layout The buses layout
 * @return The total number of input channels
 */
int getTotalNumOutputChannels(const juce::AudioProcessor::BusesLayout& layout);

/**
 * Get info about a BusesLayout
 *  - Total count of input buses
 *  - Total count of output buses
 *  - Description of each input bus
 *  - Description of each output bus
 *
 * @param layout BusesLayout
 * @return String with description
 */
juce::String describeBusesLayout(const juce::AudioProcessor::BusesLayout& layout);

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

/**
 * Outputs binary data either to stdout or to a file.
 * Used for outputting the final result of a command according to the options set by the user.
 *
 * @param data The binary data to output
 * @param outPath The output file path. If not supplied, outputs to stdout instead
 * @param overwrite If true, overwrites the file if it exists. If false, creates a new file with a
 *                  unique name
 */
void outputResult(const juce::MemoryBlock& data, juce::File outPath = {}, bool overwrite = true);
