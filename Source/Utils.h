#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <nlohmann/json.hpp>

enum class OutputFormat { text, json, binary, xml };

inline const std::unordered_map<std::string, OutputFormat> formatMap{
    { "text", OutputFormat::text },
    { "json", OutputFormat::json },
    { "binary", OutputFormat::binary },
    { "xml", OutputFormat::xml },
};


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
};

/**
* Loads a saved plugin state from file and applies it to the plugin.
* The state should have been saved as binary using the manage state command.
* Will throw if the file couldn't be opened, but has no idea if the plugin accepted/understood the binary blob it was given.
*
* @param plugin The plugin.
* @param statePath The path to the binary state
* @param state Memory into which to write the state
* @throws FileLoadError If the state file couldn't be opened. 
*/
void loadPluginStateFromFile(juce::AudioPluginInstance& plugin, const juce::File& statePath, juce::MemoryBlock& state);

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
