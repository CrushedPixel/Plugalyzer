#include "ManageStateCommand.h"

#include "Utils.h"

#include <format>
#include <juce_audio_processors/juce_audio_processors.h>

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

std::shared_ptr<CLI::App> ManageStateCommand::createApp() {
    std::shared_ptr<CLI::App> app =
        std::make_shared<CLI::App>("Save and load the plugin's state", "manageState");

    // don't break these lines, please
    // clang-format off
    app->add_option("-p,--plugin", argPluginPath, "Plugin path")
        ->required()
        ->check(CLI::ExistingPath)
        ->each([&](std::string arg){ pluginPath = stringToFile(arg); });
    app->add_option("-o,--output", argOutPath, "Output plugin state file path. Will output to stdout if not supplied.")
        ->check(validateOutputPath)
        ->each([&](std::string arg) { outputFilePath = stringToFile(arg); });
    app->add_option("-f,--format", argOutFormat, "The output format (binary, xml)")
        ->check(validateBinaryOrXml)
        ->each([&](std::string arg) { outputFormat = parseOutputFormat(arg); });
    app->add_flag("-y,--overwrite", overwriteOutputFile, "Overwrite the output file if it exists");

    // clang-format on
    return app;
}

void ManageStateCommand::execute() {
    const double dummySampleRate{ 48000.0 };
    const int dummyBlockSize{ 1024 };
    juce::MemoryBlock state;

    const auto plugin = PluginUtils::createPluginInstance(
        pluginPath.getFullPathName(), dummySampleRate, dummyBlockSize
    );
    plugin->getStateInformation(state);

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
            throw FailedXmlError{ "Couldn't decode the state into XML format.", 200 };
        };
    }
}
