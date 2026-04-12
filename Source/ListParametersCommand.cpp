#include "ListParametersCommand.h"
#include "Automation.h"
#include "Utils.h"

#include <nlohmann/json.hpp>

#include <sstream>

std::shared_ptr<CLI::App> ListParametersCommand::createApp() {
    // don't break these lines, please
    // clang-format off
    std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>("Lists a plugin's parameters", "listParameters");

    app->add_option("-p,--plugin", pluginPath, "Plugin path")->required()->check(CLI::ExistingPath); // not ExistingFile because on macOS, these bundles are directories
    app->add_option("-s,--sampleRate", sampleRate, "The sample rate to initialize the plugin with");
    app->add_option("-b,--blockSize", blockSize, "The buffer size to initialize the plugin with");
    app->add_option("-f,--format", outputFormat, "The output format (text, json)");

    return app;
    // clang-format on
}

static size_t numDigits(size_t value) {
    char buffer[20];
    auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return static_cast<size_t>(ptr - buffer);
}

static juce::StringArray getDiscreteValueStrings(const juce::AudioProcessorParameter& param) {
    if (!param.isDiscrete()) return {};

    juce::StringArray result;

    const auto maxIndex = param.getNumSteps() - 1;

    for (int i = 0; i < param.getNumSteps(); ++i)
        result.add(param.getText((float) i / (float) maxIndex, 1024));

    return result;
}

void ListParametersCommand::execute() {
    auto outFmt = parseOutputFormat(outputFormat.getCharPointer());

    auto plugin = PluginUtils::createPluginInstance(pluginPath, sampleRate, (int) blockSize);
    auto params = plugin->getParameters();
    auto paramJson = getParametersAsJson(params);

    std::string outputText{};

    if (outFmt == OutputFormat::text) {
        outputText = getParametersAsString(paramJson);
    } else if (outFmt == OutputFormat::json) {
        outputText = paramJson.dump();
    }
    std::cout << outputText;
}

std::string ListParametersCommand::getParametersAsString(const nlohmann::json& params) {
    // calculate the amount of characters the maximum parameter index has
    // for alignment purposes
    auto maxIdxStrLength = numDigits(params.size() - 1);

    std::stringstream ss;

    ss << "Plugin parameters: \n";

    for (auto& param : params) {
        jassert(param.contains("index"));
        jassert(param.contains("name"));
        jassert(param.contains("discrete"));
        jassert((param.contains("values") && param["discrete"]) ||
                (param.contains("minValue") && param.contains("maxValue")));

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
            ss << param["minValue"].get<std::string>() << " to " << param["maxValue"].get<std::string>() << "\n";
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

nlohmann::json ListParametersCommand::getParametersAsJson(
    const juce::Array<juce::AudioProcessorParameter*>& params) {
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
