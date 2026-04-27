#include "StateCommand.h"

#include "Automation.h"
#include "Errors.h"
#include "Parsers.h"
#include "PluginProcess.h"
#include "Utils.h"
#include "Validators.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <optional>

using ParamArray = juce::Array<juce::AudioProcessorParameter*>;

static nlohmann::json getParameterValuesAsJson(const ParamArray& params) {
    nlohmann::json paramJson;

    for (const auto* param : params) {
        const auto paramName = param->getName(100).toStdString();
        paramJson[paramName] = param->getCurrentValueAsText().toStdString();
    }

    return paramJson;
}

static std::unique_ptr<juce::XmlElement> decodeState(juce::XmlElement* stateXml) {
    using namespace juce;
    constexpr const char* kJucePrivateDataIdentifier = "JUCEPrivateData";

    auto rootElement = std::make_unique<XmlElement>("VST3PluginState");
    auto iComponentElement = std::make_unique<XmlElement>("IComponent");
    juce::MemoryBlock mb;
    mb.fromBase64Encoding(stateXml->getAllSubText());

    auto buffer = static_cast<const char*>(mb.getData());
    auto size = static_cast<uint64>(mb.getSize());

    // Check for JUCE private data section
    auto jucePrivDataIdentifierSize = std::strlen(kJucePrivateDataIdentifier);
    uint64 privateDataSize = 0;
    uint64 pluginStateSize = size;

    if ((size_t) size >= jucePrivDataIdentifierSize + sizeof(int64)) {
        juce::String magic(
            juce::CharPointer_UTF8(buffer + size - jucePrivDataIdentifierSize),
            juce::CharPointer_UTF8(buffer + size)
        );

        if (magic == kJucePrivateDataIdentifier) {
            std::memcpy(
                &privateDataSize,
                buffer + ((size_t) size - jucePrivDataIdentifierSize - sizeof(uint64)),
                sizeof(uint64)
            );

            privateDataSize = juce::ByteOrder::swapIfBigEndian(privateDataSize);
            pluginStateSize -= privateDataSize + jucePrivDataIdentifierSize + sizeof(uint64);
        }
    }

    if (pluginStateSize > 0) {
        if (auto pluginStateXml =
                juce::AudioProcessor::getXmlFromBinary(buffer, static_cast<int>(pluginStateSize))) {
            iComponentElement->addChildElement(pluginStateXml.release());
        } else {
            throw FailedXmlError{ "Cannot decode the binary data into XML.", 210 };
        }
    }

    if ((size_t) size >= jucePrivDataIdentifierSize + sizeof(int64)) {
        juce::String magic(
            juce::CharPointer_UTF8(buffer + size - jucePrivDataIdentifierSize),
            juce::CharPointer_UTF8(buffer + size)
        );

        if (magic == kJucePrivateDataIdentifier && privateDataSize > 0) {
            auto privateTree = juce::ValueTree::readFromData(
                buffer + pluginStateSize, static_cast<size_t>(privateDataSize)
            );
            auto privateElement = std::make_unique<XmlElement>("JucePrivateData");
            privateElement->addChildElement(privateTree.createXml().release());
            iComponentElement->addChildElement(privateElement.release());
        }
    }

    rootElement->addChildElement(iComponentElement.release());

    return rootElement;
}

std::shared_ptr<CLI::App> StateCommand::createApp() {
    std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>(
        "Load parameter automation and save as binary plugin state, load plugin binary state and "
        "save as (static) parameter automation, or simply output the default state. Binary state "
        "can sometimes be decoded into readable XML format, but currently only for VST3 using JUCE "
        "APVTS.",
        "state"
    );

    // don't break these lines, please
    // clang-format off
    app->add_option("-p,--plugin", argPluginPath, "Plugin path")
        ->required()
        ->check(CLI::ExistingPath)
        ->each([&](std::string arg){ pluginPath = parse::stringToFile(arg); });
    app->add_option("-i,--input", argInPath, "Input file path. Can be an automation file in json format (must have a .json extension), or a state file in binary format.")
        ->check(CLI::ExistingPath)
        ->each([&](std::string arg) { inputFilePath = parse::stringToFile(arg); });
    app->add_option("-o,--output", argOutPath, "Output file path. Will output to stdout if not supplied.")
        ->check(validate::outputPath)
        ->each([&](std::string arg) { outputFilePath = parse::stringToFile(arg); });
    app->add_option("-f,--format", argOutFormat, "The output format (json, binary, xml). If the input is JSON, only binary or XML are valid output formats and vice versa.")
        ->check([](const std::string& arg) { return (arg == "binary" || arg == "xml" || arg == "json") ? std::string{} : "Output format must be 'binary', 'xml' or 'json'"; } )
        ->each([&](std::string arg) { outputFormat = parse::outputFormat(arg); });
    app->add_flag("-y,--overwrite", overwriteOutputFile, "Overwrite the output file if it exists");

    // clang-format on
    return app;
}

void StateCommand::execute() {
    const double dummySampleRate{ 48000.0 };
    const int dummyBlockSize{ 1024 };
    const size_t dummyInputLength{ 1024 };
    const size_t firstSampleIndex{ 0 };
    const auto plugin = PluginUtils::createPluginInstance(
        pluginPath.getFullPathName(), dummySampleRate, dummyBlockSize
    );
    juce::MemoryBlock state;

    if (inputFilePath == juce::File{}) {
        // Default state only
        plugin->getStateInformation(state);
    } else if (inputFilePath.hasFileExtension("json")) {
        // Parse the automation file and output the state
        auto automation = parseParameters(
            *plugin, dummySampleRate, dummyInputLength, std::optional{ inputFilePath }, {}
        );
        Automation::applyParameters(*plugin, automation, firstSampleIndex);
        plugin->getStateInformation(state);

    } else {
        // Load the state and return the plugin's parameter values
        loadPluginStateFromFile(*plugin, inputFilePath, state);
        auto params = getParameterValuesAsJson(plugin->getParameters());
        outputResult(params.dump(4), outputFilePath, overwriteOutputFile);
        return;
    }

    if (outputFormat == OutputFormat::binary) {
        outputResult(state, outputFilePath, overwriteOutputFile);
    } else if (outputFormat == OutputFormat::xml) {
        std::unique_ptr<juce::XmlElement> decodedState{};
        if (auto wrappedState =
                juce::AudioProcessor::getXmlFromBinary(state.getData(), state.getSize())) {
            decodedState = decodeState(wrappedState.get());
            outputResult(
                decodedState->toString().toStdString(), outputFilePath, overwriteOutputFile
            );
        } else {
            outputResult(
                state.toString().toStdString(), outputFilePath, overwriteOutputFile
            );
            throw FailedXmlError{ "Couldn't decode the state into XML format.", 200 };
        };
    }
}
