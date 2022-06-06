#include "ListParametersCommand.h"
#include "Automation.h"
#include "Utils.h"

std::shared_ptr<CLI::App> ListParametersCommand::createApp() {
    // don't break these lines, please
    // clang-format off
    std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>("Lists a plugin's parameters", "listParameters");

    app->add_option("-p,--plugin", pluginPath, "Plugin path")->required()->check(CLI::ExistingPath); // not ExistingFile because on macOS, these bundles are directories
    app->add_option("-s,--sampleRate", sampleRate, "The sample rate to initialize the plugin with");
    app->add_option("-b,--blockSize", blockSize, "The buffer size to initialize the plugin with");

    return app;
    // clang-format on
}

void ListParametersCommand::execute() {
    // load the plugin
    auto plugin = PluginUtils::createPluginInstance(pluginPath, sampleRate, (int) blockSize);

    std::cout << "Plugin parameters: " << std::endl;

    auto params = plugin->getParameters();

    // calculate the amount of characters the maximum parameter index has
    // for alignment purposes
    auto maxIdxStrLength = juce::String(params.size() - 1).length();

    for (auto* param : params) {
        // calculate the indent of the index to align all entries nicely
        auto idxStrLength = juce::String(param->getParameterIndex()).length();
        std::string idxIndent(maxIdxStrLength - idxStrLength, ' ');

        // index: name
        std::cout << idxIndent << param->getParameterIndex() << ": " << param->getName(100)
                  << std::endl;

        // calculate the indent of entries for alignment
        std::string indent(2 + maxIdxStrLength, ' ');

        // print the parameter's values
        std::cout << indent << "Values:  ";

        if (auto valueStrings = param->getAllValueStrings(); !valueStrings.isEmpty()) {
            // list all discrete values
            for (int i = 0; i < valueStrings.size(); i++) {
                std::cout << valueStrings[i];
                if (i != valueStrings.size() - 1) {
                    std::cout << ", ";
                } else {
                    std::cout << std::endl;
                }
            }
        } else {
            // print the range of values that is supported
            std::cout << param->getText(0, 1024) << param->getLabel() << " to "
                      << param->getText(1, 1024) << param->getLabel() << std::endl;
        }

        // print the default value
        std::cout << indent << "Default: " << param->getText(param->getDefaultValue(), 1024)
                  << param->getLabel() << std::endl;

        // print symmetry of parameter's string <-> normalized value conversion
        std::cout << std::boolalpha; // print bools as true/false
        std::cout << indent << "Supports text values: "
                  << Automation::parameterSupportsTextToValueConversion(param) << std::endl;
    }
}
