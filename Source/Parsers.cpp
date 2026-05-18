#include "Parsers.h"

#include "Errors.h"
#include "Generators.h"
#include "Utils.h"

#include <chrono>
#include <cstddef>
#include <format>
#include <juce_audio_basics/juce_audio_basics.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

#define PARSE_STRICT(funName)                                                                      \
    size_t endPtr;                                                                                 \
    auto num = (funName) (str, &endPtr);                                                           \
    if (endPtr != str.size()) {                                                                    \
        throw std::invalid_argument("Invalid number: '" + str + "'");                              \
    }                                                                                              \
    return num

namespace parse {
std::chrono::seconds seconds(const std::string& secondsString) {
    auto [val, unit] = parse::numberAndUnits<int>(secondsString);
    if (string_utils::lowerCase(unit) == "s") {
        return std::chrono::seconds{ val };
    }
    if (string_utils::lowerCase(unit) == "ms") {
        return std::chrono::seconds{ val / 1000 };
    }
    throw ParseError{ std::format("Unknown time unit: {}", unit), 168 };
}

double amplitude(const std::string& amplitudeString) {
    auto [val, unit] = parse::numberAndUnits<double>(amplitudeString);
    if (string_utils::lowerCase(unit) == "db") {
        auto linearVal = juce::Decibels::decibelsToGain(static_cast<double>(val), -96.0);
        return linearVal;
    }
    if (unit == "") {
        return static_cast<double>(val);
    }
    throw ParseError{ std::format("Unknown amplitude unit: {}", unit), 169 };
}

Hertz frequency(const std::string& freqString) {
    auto [val, unit] = parse::numberAndUnits<double>(freqString);
    if (string_utils::lowerCase(unit) == "hz") {
        return val;
    }
    if (string_utils::lowerCase(unit) == "khz") {
        return val * 1000.0;
    }
    throw ParseError{ std::format("Unknown time unit: {}", unit), 170 };
}

GeneratorInputBus generatorInput(const std::string& jsonStringOrFilePath) {
    auto json = getJson(jsonStringOrFilePath);
    return GeneratorInputBus::fromJson(json);
}

OutputFormat outputFormat(const std::string& formatName) {
    if (formatMap.contains(formatName)) {
        return formatMap.at(formatName);
    } else {
        // Should be validated already
        jassertfalse;
        return OutputFormat::text;
    }
}

double extractSampleRate(const std::string& jsonStringOrFilePath) {
    auto json = getJson(jsonStringOrFilePath);
    return json["sample rate"].get<double>();
}

juce::File stringToFile(const std::string& filePath) {
    if (juce::File::isAbsolutePath(filePath)) {
        return juce::File(filePath);
    } else {
        return juce::File::getCurrentWorkingDirectory().getChildFile(filePath);
    }
}

float floatStrict(const std::string& str) { PARSE_STRICT(std::stof); }

unsigned long uLongStrict(const std::string& str) { PARSE_STRICT(std::stoul); }

ParameterCLIArgument pluginParameterArgument(const std::string& str) {
    juce::StringArray tokens;
    tokens.addTokens(str, ":", "\"'");

    bool isNormalizedValue = tokens.size() == 3;

    if (isNormalizedValue && tokens[2] != "n") {
        throw CLIException("Invalid parameter modifier: '" + tokens[2] + "'. Only 'n' is allowed");
    }

    if (tokens.size() != 2 && tokens.size() != 3) {
        throw CLIException("'" + str + "' is not a colon-separated key-value pair");
    }

    std::string valueStr = tokens[1].toStdString();
    if (isNormalizedValue) {
        float normalizedValue;
        try {
            normalizedValue = parse::floatStrict(valueStr);
        } catch (const std::invalid_argument& ia) {
            throw CLIException(
                "Normalized parameter value must be a number, but is '" + valueStr + "'"
            );
        }

        if (normalizedValue < 0 || normalizedValue > 1) {
            throw CLIException(
                "Normalized parameter value must be between 0 and 1, but is " +
                std::to_string(normalizedValue)
            );
        }

        return {
            .parameterName = tokens[0].toStdString(),
            .textValue = {},
            .normalizedValue = normalizedValue,
            .isNormalizedValue = true,
        };
    }

    return {
        .parameterName = tokens[0].toStdString(),
        .textValue = valueStr,
        .normalizedValue = 0.f,
        .isNormalizedValue = false,
    };
}

} // namespace parse
