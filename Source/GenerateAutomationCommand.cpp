#include "GenerateAutomationCommand.h"

#include "Automation.h"
#include "Utils.h"

#include <format>
#include <nlohmann/json.hpp>

static nlohmann::json getParameterAutomation(const juce::AudioProcessorParameter& param) {
    const auto supportsTextValues = Automation::parameterSupportsTextToValueConversion(&param);
    const auto automatable = param.isAutomatable();
    const auto numSteps = param.getNumSteps();
    const auto discrete = param.isDiscrete();
    const auto name = param.getName(100).toStdString();

    if (!automatable) {
        return param.getDefaultValue();
    }

    nlohmann::json returnValues;

    if (!supportsTextValues) {
        returnValues["0%"] = 0.0;
        returnValues["100%"] = 1.0;
        return returnValues;
    }

    if (discrete) {
        // For discrete parameters, we explicitly automate from one value to another
        // so the final file can be more easily edited and the user sees all possible values
        const auto valueStrings = getDiscreteValueStrings(param);
        const double percentageIncrement{ 100.0 / numSteps };
        double percentage{ 0.0 };
        for (auto& value : valueStrings) {
            returnValues[std::format("{:.0f}%", percentage)] = value.toStdString();
            percentage += percentageIncrement;
        }
    } else {
        const auto minValue = param.getText(0, 1024).toStdString();
        const auto maxValue = param.getText(1, 1024).toStdString();
        returnValues["0%"] = minValue;
        returnValues["100%"] = maxValue;
    }

    return returnValues;
}

using ParamArray = juce::Array<juce::AudioProcessorParameter*>;

static nlohmann::json getParameterAutomation(const ParamArray& params) {
    nlohmann::json paramJson;

    for (const auto* param : params) {
        const auto paramName = param->getName(100).toStdString();
        if (paramName == "Bypass") {
            // No one wants bypass automated
            paramJson[paramName] = param->getDefaultValue();
        } else {
            paramJson[paramName] = getParameterAutomation(*param);
        }
    }

    return paramJson;
}

std::shared_ptr<CLI::App> GenerateAutomationCommand::createApp() {
    std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>(
        "Generates an example automation file you can modify", "generateAutomation"
    );

    // don't break these lines, please
    // clang-format off
    app->add_option("-p,--plugin", argPluginPath, "Plugin path")
        ->required()
        ->check(CLI::ExistingPath)
        ->each([&](std::string arg){ pluginPath = stringToFile(arg); });
    app->add_option("-o,--output", argOutPath, "Output automation file path (json). Will output to stdout if not supplied.")
        ->check(validateOutputPath)
        ->each([&](std::string arg) { outputFilePath = stringToFile(arg); });
    app->add_flag("-y,--overwrite", overwriteOutputFile, "Overwrite the output file if it exists");

    // clang-format on
    return app;
}

void GenerateAutomationCommand::execute() {
    const double dummySampleRate{ 48000.0 };
    const int dummyBlockSize{ 1024 };
    const auto plugin = PluginUtils::createPluginInstance(
        pluginPath.getFullPathName(), dummySampleRate, dummyBlockSize
    );
    const auto params = plugin->getParameters();
    const auto paramJson = getParameterAutomation(params);

    if (argOutPath.empty()) {
        std::cout << paramJson.dump();
    } else {
        if (overwriteOutputFile) {
            outputFilePath.replaceWithText(paramJson.dump(4));
        } else {
            outputFilePath.getNonexistentSibling(false).replaceWithText(paramJson.dump(4));
        }
    }
}
