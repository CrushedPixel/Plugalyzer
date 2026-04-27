#include "Parsers.h"

#include "Errors.h"
#include "Utils.h"

#define PARSE_STRICT(funName)                                                                      \
    size_t endPtr;                                                                                 \
    auto num = (funName) (str, &endPtr);                                                           \
    if (endPtr != str.size()) {                                                                    \
        throw std::invalid_argument("Invalid number: '" + str + "'");                              \
    }                                                                                              \
    return num


namespace parse
{
OutputFormat outputFormat(const std::string& formatName) {
    if (formatMap.contains(formatName)) {
        return formatMap.at(formatName);
    } else {
        return OutputFormat::text;
    }
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
            throw CLIException("Normalized parameter value must be a number, but is '" + valueStr +
                               "'");
        }

        if (normalizedValue < 0 || normalizedValue > 1) {
            throw CLIException("Normalized parameter value must be between 0 and 1, but is " +
                               std::to_string(normalizedValue));
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
