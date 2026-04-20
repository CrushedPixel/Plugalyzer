#include "BusLayoutsCommand.h"

#include "Utils.h"

#include <format>
#include <nlohmann/json.hpp>

static nlohmann::json checkPossibleBusLayouts(const juce::AudioPluginInstance& plugin) {
    using blo = juce::AudioProcessor::BusesLayout;

    nlohmann::json result{};

    auto checkLayout = [&](const blo& layout) {
        if (plugin.checkBusesLayoutSupported(layout)) {
            result.push_back(getBusLayoutJson(layout));
        }
    };

    const auto channelSets = allChannelSets();

    // 0x input, 0x output
    checkLayout(blo{});

    // 0x input, 1x output
    for (const auto& cs : channelSets) {
        checkLayout(blo{ .inputBuses = {}, .outputBuses = juce::Array{ cs } });
    }

    // 1x input, 0x output
    for (const auto& cs : channelSets) {
        checkLayout(blo{ .inputBuses = juce::Array{ cs }, .outputBuses = {} });
    }

    // 1x input, 1x output
    for (const auto& cs_outer : channelSets) {
        for (const auto& cs_inner : channelSets) {
            checkLayout(
                blo{ .inputBuses = juce::Array{ cs_outer }, .outputBuses = juce::Array{ cs_inner } }
            );
        }
    }

    // 2x input, 0x output
    for (const auto& cs_outer : channelSets) {
        for (const auto& cs_inner : channelSets) {
            checkLayout(blo{ .inputBuses = juce::Array{ cs_outer, cs_inner }, .outputBuses = {} });
        }
    }

    // 2x input, 1x output
    for (const auto& cs_outer : channelSets) {
        for (const auto& cs_inner : channelSets) {
            for (const auto& cs : channelSets) {
                checkLayout(
                    blo{
                        .inputBuses = juce::Array{ cs_outer, cs_inner },
                        .outputBuses = juce::Array{ cs },
                    }
                );
            }
        }
    }

    return result;
}

std::shared_ptr<CLI::App> BusLayoutsCommand::createApp() {
    std::shared_ptr<CLI::App> app =
        std::make_shared<CLI::App>("Outputs bus layouts supported by the plugin", "busLayouts");

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

void BusLayoutsCommand::execute() {
    const double dummySampleRate{ 48000.0 };
    const int dummyBlockSize{ 1024 };
    const auto plugin = PluginUtils::createPluginInstance(
        pluginPath.getFullPathName(), dummySampleRate, dummyBlockSize
    );
    const auto busLayoutsJson = checkPossibleBusLayouts(*plugin);

    if (outputFormat == OutputFormat::text) {
        const auto busLayoutsText = getBusLayoutHumanReadable(busLayoutsJson);
        outputResult(busLayoutsText, outputFilePath, overwriteOutputFile);
    } else if (outputFormat == OutputFormat::json) {
        outputResult(busLayoutsJson.dump(4), outputFilePath, overwriteOutputFile);
    }
}
