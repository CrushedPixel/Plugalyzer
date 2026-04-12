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

void ListParametersCommand::execute() {
    auto outFmt = parseOutputFormat(outputFormat.getCharPointer());

    auto plugin = PluginUtils::createPluginInstance(pluginPath, sampleRate, (int) blockSize);
    auto params = plugin->getParameters();
    std::string outputText{};

    if (outFmt == OutputFormat::text) {
        outputText = getParametersAsString(params);
    } else if (outFmt == OutputFormat::json) {
        outputText = getParametersAsJson(params);
    }
    std::cout << outputText;
}

std::string ListParametersCommand::getParametersAsString(
    const juce::Array<juce::AudioProcessorParameter*>& params) {
    // calculate the amount of characters the maximum parameter index has
    // for alignment purposes
    auto maxIdxStrLength = juce::String(params.size() - 1).length();

    std::stringstream ss;

    ss << "Plugin parameters: \n";

    for (auto* param : params) {
        auto idxStrLength = juce::String(param->getParameterIndex()).length();
        std::string idxIndent(maxIdxStrLength - idxStrLength, ' ');

        ss << idxIndent << param->getParameterIndex() << ": " << param->getName(100) << "\n";

        std::string indent(2 + maxIdxStrLength, ' ');

        ss << indent << "Values:  ";

        if (auto valueStrings = param->getAllValueStrings(); !valueStrings.isEmpty()) {
            for (int i = 0; i < valueStrings.size(); i++) {
                ss << valueStrings[i];
                if (i != valueStrings.size() - 1) {
                    ss << ", ";
                } else {
                    ss << "\n";
                }
            }
        } else {
            ss << param->getText(0, 1024) << param->getLabel() << " to " << param->getText(1, 1024)
               << param->getLabel() << "\n";
        }

        ss << indent << "Default: " << param->getText(param->getDefaultValue(), 1024)
           << param->getLabel() << "\n";

        ss << indent << "Supports text values: " << std::boolalpha
           << Automation::parameterSupportsTextToValueConversion(param) << "\n";
    }

    return ss.str();
}

std::string ListParametersCommand::getParametersAsJson(
    const juce::Array<juce::AudioProcessorParameter*>& params) {
    nlohmann::json json;

    for (auto* param : params) {
        nlohmann::json paramJson;
        paramJson["index"] = param->getParameterIndex();
        paramJson["name"] = param->getName(100).toStdString();
        paramJson["label"] = param->getLabel().toStdString();
        paramJson["defaultValue"] = param->getText(param->getDefaultValue(), 1024).toStdString();
        paramJson["supportsTextValues"] = Automation::parameterSupportsTextToValueConversion(param);

        if (auto valueStrings = param->getAllValueStrings(); !valueStrings.isEmpty()) {
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

    return json.dump();
}
