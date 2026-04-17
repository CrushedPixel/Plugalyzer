#include "ListParametersCommand.h"

#include "Automation.h"
#include "Utils.h"

#include <nlohmann/json.hpp>
#include <sstream>

static size_t numDigits(size_t value) {
    char buffer[20];
    auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return static_cast<size_t>(ptr - buffer);
}

static std::string getParametersAsString(const nlohmann::json& params) {
    // calculate the amount of characters the maximum parameter index has
    // for alignment purposes
    auto maxIdxStrLength = numDigits(params.size() - 1);

    std::stringstream ss;

    ss << "Plugin parameters: \n";

    for (auto& param : params) {
        jassert(param.contains("index"));
        jassert(param.contains("name"));
        jassert(param.contains("discrete"));
        jassert(
            (param.contains("values") && param["discrete"]) ||
            (param.contains("minValue") && param.contains("maxValue"))
        );

        auto idxStrLength = numDigits(param["index"]);
        std::string idxIndent(maxIdxStrLength - idxStrLength, ' ');
        std::string indent(2 + maxIdxStrLength, ' ');

        ss << idxIndent << param["index"] << ": " << param["name"].get<std::string>() << "\n";

        ss << indent << "values: ";
        if (param["discrete"]) {
            // List values separated by commas
            for (auto i = 0uz; i < param["values"].size(); i++) {
                ss << param["values"][i].get<std::string>();
                if (i != param["values"].size() - 1) {
                    ss << ", ";
                } else {
                    ss << "\n";
                }
            }
        } else {
            // Show range of values
            ss << param["minValue"].get<std::string>() << " to "
               << param["maxValue"].get<std::string>() << "\n";
        }

        for (auto& [key, value] : param.items()) {
            if (key == "index" || key == "name" || key == "minValue" || key == "maxValue" ||
                key == "values") {
                continue;
            }

            ss << indent << key << ": " << value << "\n";
        }
    }

    return ss.str();
}

using ParamArray = juce::Array<juce::AudioProcessorParameter*>;

static nlohmann::json getParametersAsJson(const ParamArray& params) {
    nlohmann::json json;

    for (auto* param : params) {
        nlohmann::json paramJson;
        paramJson["index"] = param->getParameterIndex();
        paramJson["name"] = param->getName(100).toStdString();
        paramJson["label"] = param->getLabel().toStdString();
        paramJson["defaultValue"] = param->getText(param->getDefaultValue(), 1024).toStdString();
        paramJson["numSteps"] = param->getNumSteps();
        paramJson["discrete"] = param->isDiscrete();
        paramJson["boolean"] = param->isBoolean();
        paramJson["automatable"] = param->isAutomatable();
        paramJson["metaParameter"] = param->isMetaParameter();
        paramJson["versionHint"] = param->getVersionHint();
        paramJson["supportsTextValues"] = Automation::parameterSupportsTextToValueConversion(param);

        if (param->isDiscrete()) {
            auto valueStrings = param->getAllValueStrings();
            // getAllValueStrings returns empty array for VST3
            if (valueStrings.isEmpty()) {
                valueStrings = getDiscreteValueStrings(*param);
            }
            jassert(!valueStrings.isEmpty());
            nlohmann::json values = nlohmann::json::array();
            for (int i = 0; i < valueStrings.size(); i++) {
                values.push_back(valueStrings[i].toStdString());
            }
            paramJson["values"] = values;
        } else {
            paramJson["minValue"] = param->getText(0, 1024).toStdString();
            paramJson["maxValue"] = param->getText(1, 1024).toStdString();
        }

        json.push_back(paramJson);
    }

    return json;
}

std::shared_ptr<CLI::App> ListParametersCommand::createApp() {
    std::shared_ptr<CLI::App> app =
        std::make_shared<CLI::App>("Lists a plugin's parameters", "listParameters");

    // don't break these lines, please
    // clang-format off
    app->add_option("-p,--plugin", argPluginPath, "Plugin path")
        ->required()
        ->check(CLI::ExistingPath)
        ->each([&](std::string arg){ pluginPath = stringToFile(arg); });
    app->add_option("-o,--output", argOutPath, "Output automation file path (json). Will output to stdout if not supplied.")
        ->check(validateOutputPath)
        ->each([&](std::string arg) { outputFilePath = stringToFile(arg); });
    app->add_option("-f,--format", argOutFormat, "The output format (text, json)")
        ->each([&](std::string arg) { outputFormat = parseOutputFormat(arg); });
    app->add_flag("-y,--overwrite", overwriteOutputFile, "Overwrite the output file if it exists");

    // clang-format on
    return app;
}

void ListParametersCommand::execute() {
    const double dummySampleRate{ 48000.0 };
    const int dummyBlockSize{ 1024 };
    const auto plugin = PluginUtils::createPluginInstance(
        pluginPath.getFullPathName(), dummySampleRate, dummyBlockSize
    );
    auto params = plugin->getParameters();
    auto paramJson = getParametersAsJson(params);

    if (outputFormat == OutputFormat::text) {
        const auto paramsText = getParametersAsString(paramJson);
        outputResult(paramsText, outputFilePath, overwriteOutputFile);
    } else if (outputFormat == OutputFormat::json) {
        outputResult(paramJson, outputFilePath, overwriteOutputFile);
    }
}
