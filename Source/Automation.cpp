#include "Automation.h"
#include "Utils.h"

ParameterAutomation Automation::parseAutomationDefinition(const std::string& jsonStr,
                                                          const juce::AudioPluginInstance& plugin,
                                                          double sampleRate,
                                                          size_t inputLengthInSamples) {
    auto json = nlohmann::json::parse(jsonStr);
    // parse JSON into a map of parameter names to automation definitions

    auto def = json.get<std::map<std::string, nlohmann::json>>();

    // convert automation definition into ParameterAutomation instance
    // by converting keyframe times from string format to samples,
    // and text values into normalized float values
    ParameterAutomation automation;
    for (const auto& [paramName, automationDefinition] : def) {
        auto* param = PluginUtils::getPluginParameterByName(plugin, paramName);

        AutomationKeyframes keyframes;

        bool usedTextFormat = false;

        if (automationDefinition.is_primitive()) {
            // the entry is a single value to use the entire time
            auto value = getParameterValueFromJSONPrimitive(param, automationDefinition);
            keyframes[0] = value;

            usedTextFormat = automationDefinition.is_string();

        } else {
            // the entry is an automation object
            auto automationObject =
                automationDefinition.get<std::map<std::string, nlohmann::json>>();

            for (const auto& [timeStr, val] : automationObject) {
                // convert keyframe time to samples
                auto timeSamples = parseKeyframeTime(timeStr, sampleRate, inputLengthInSamples);

                if (keyframes.contains(timeSamples)) {
                    // TODO: give context on the origin of the first occurrence?
                    //  requires us to keep track of all timeStr -> time mappings
                    throw std::runtime_error(
                        "Duplicate keyframe time: " + std::to_string(timeSamples) +
                        " (obtained from input string " + timeStr + ")");
                }

                // get the internal float representation of the value provided
                auto value = getParameterValueFromJSONPrimitive(param, val);
                keyframes[timeSamples] = value;

                usedTextFormat |= val.is_string();
            }
        }

        automation[paramName] = keyframes;

        if (usedTextFormat && !parameterSupportsTextToValueConversion(param)) {
            throw std::runtime_error("Text value used for parameter '" + paramName +
                                     "', but parameter only supports normalized values");
        }
    }

    return automation;
}

size_t Automation::parseKeyframeTime(juce::String timeStr, double sampleRate,
                                     size_t inputLengthInSamples) {
    // remove an excess whitespace
    timeStr = timeStr.trim();

    bool isSeconds = timeStr.endsWithChar('s');
    bool isPercentage = timeStr.endsWithChar('%');

    if (isSeconds || isPercentage) {
        // remove the suffix and any whitespace preceding it
        timeStr = timeStr.substring(0, timeStr.length() - 1).trim();

        // parse the floating-point number
        double time;
        try {
            time = std::stod(timeStr.toStdString());
        } catch (std::invalid_argument& ia) {
            throw std::runtime_error("Invalid floating-point number '" + timeStr.toStdString() +
                                     "'");
        }

        if (isSeconds) {
            return secondsToSamples(time, sampleRate);
        } else /* if (isPercentage) */ {
            return (size_t) std::round((time / 100) * (double) inputLengthInSamples);
        }
    }

    // no known suffix was detected - parse as an integer sample value
    size_t time;
    try {
        time = std::stoul(timeStr.toStdString());
    } catch (std::invalid_argument& ia) {
        throw std::runtime_error("Invalid sample index '" + timeStr.toStdString() + "'");
    }

    return time;
}

float Automation::getParameterValueFromJSONPrimitive(const juce::AudioProcessorParameter* param,
                                                     const nlohmann::json& primitive) {
    if (primitive.is_number()) {
        float val = primitive.get<float>();
        if (val < 0 || val > 1) {
            throw std::out_of_range("Normalized parameter value must be between 0 and 1, but is " +
                                    std::to_string(val));
        }

        return val;
    }

    if (primitive.is_string()) {
        return param->getValueForText(primitive.get<std::string>());
    }

    throw std::invalid_argument("Invalid parameter value type. Must be a number or string");
}

void Automation::applyParameters(juce::AudioPluginInstance& plugin,
                                 const ParameterAutomation& automation, size_t sampleIndex) {

    for (const auto& [paramName, keyframes] : automation) {
        // find parameter
        auto* param = PluginUtils::getPluginParameterByName(plugin, paramName);

        // interpolate value for current sample index based on keyframes.
        float value;
        {
            // find first keyframe with time that is greater than the sample time.
            // this works because std::map is sorted by key in ascending order
            auto nextKeyframe =
                std::upper_bound(keyframes.begin(), keyframes.end(), sampleIndex,
                                 [](size_t _sampleIndex, const std::pair<size_t, float>& keyframe) {
                                     return _sampleIndex < keyframe.first;
                                 });

            if (nextKeyframe == keyframes.begin()) {
                // use the value of the first keyframe
                value = nextKeyframe->second;

            } else if (nextKeyframe == keyframes.end()) {
                // use the value of the last keyframe
                value = std::prev(nextKeyframe)->second;

            } else {
                // linearly interpolate between the two keyframes
                auto prevKeyframe = std::prev(nextKeyframe);

                auto keyframeDistance = nextKeyframe->first - prevKeyframe->first;
                auto relativePos =
                    (float) (sampleIndex - prevKeyframe->first) / (float) keyframeDistance;

                value = std::lerp(prevKeyframe->second, nextKeyframe->second, relativePos);
            }
        }

        // apply the value
        param->setValue(value);
    }
}

float epsilon = std::powf(10, -4);

bool Automation::parameterSupportsTextToValueConversion(
    const juce::AudioProcessorParameter* param) {
    int numValuesToTry = std::min(100, param->getNumSteps());

    for (int i = 0; i < numValuesToTry; i++) {
        float normalizedValue = (float) i / (float) (numValuesToTry - 1);

        auto text = param->getText(normalizedValue, 1024);
        float normalizedValueFromText = param->getValueForText(text);

        if (std::abs(normalizedValue - normalizedValueFromText) >= epsilon) {
            return false;
        }
    }

    return true;
}
