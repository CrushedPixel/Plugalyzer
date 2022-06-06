#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <map>

/**
 * Automation keyframes, with keys representing the timestamp of the keyframe in samples,
 * and the value representing the parameter's value at that timestamp.
 */
typedef std::map<size_t, float> AutomationKeyframes;

/**
 * Parameter automation, with keys representing the parameter's name,
 * and values representing the automation keyframes for that parameter.
 */
typedef std::map<std::string, AutomationKeyframes> ParameterAutomation;

/**
 * Functions for handling parameter automation.
 */
class Automation {
  public:
    /**
     * Parses a parameter automation definition from a JSON string.
     *
     * @param jsonStr The JSON string to parse.
     * @param plugin The plugin the parameter definition is for.
     * @param sampleRate The sample rate to use for conversion of seconds to samples.
     * @param inputLengthInSamples The input's total length to use for conversion of percentage
     * values.
     * @return The parsed parameter automation data.
     * @throws std::runtime_error If automation definition contains invalid keyframe times;
     * If it contains multiple keyframe times that resolve to the same value in samples;
     * If it contains a parameter name unknown to the plugin.
     */
    static ParameterAutomation parseAutomationDefinition(const std::string& jsonStr,
                                                         const juce::AudioPluginInstance& plugin,
                                                         double sampleRate,
                                                         size_t inputLengthInSamples);

    /**
     * Applies automation data to the given plugin.
     *
     * @param plugin The plugin to apply the automation to.
     * @param automation The automation to apply.
     * @param sampleIndex The sample index for which to evaluate parameter values.
     * @throws std::runtime_error If the automation data contains a parameter name unknown to the
     * plugin.
     */
    static void applyParameters(juce::AudioPluginInstance& plugin,
                                const ParameterAutomation& automation, size_t sampleIndex);

    /**
     * Tests whether calling the given parameter's <code>textToValue</code> function
     * with a string obtained using <code>getText</code> returns the original normalized value.
     *
     * @param param The parameter to test.
     * @return Whether the parameter supports the text-to-value conversion.
     */
    static bool parameterSupportsTextToValueConversion(const juce::AudioProcessorParameter* param);

  private:
    /**
     * Converts a keyframe time string into samples.
     *
     * <ul>
     * <li>Integer numbers are interpreted as samples.</li>
     * <li>Numbers suffixed with "s" are interpreted as seconds.</li>
     * <li>Numbers suffixed with "%" are interpreted relative to the input's total length.</li>
     * </ul>
     *
     * @param timeStr The input string.
     * @param sampleRate The sample rate to use for conversion of seconds to samples.
     * @param inputLengthInSamples The input's total length to use for conversion of percentage
     * values.
     * @return The time in samples.
     * @throws std::runtime_error If the input string couldn't be parsed.
     */
    // TODO: unit tests
    static size_t parseKeyframeTime(juce::String timeStr, double sampleRate,
                                    size_t inputLengthInSamples);
};
