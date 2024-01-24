#include "PresetLoadingExtensionsVisitor.h"

PresetLoadingExtensionsVisitor::PresetLoadingExtensionsVisitor(const juce::MemoryBlock& presetData)
    : presetData(presetData) {}

void PresetLoadingExtensionsVisitor::visitUnknown(const Unknown&) {
    throw CLIException("Unknown plugin format for preset loading");
}

void PresetLoadingExtensionsVisitor::visitVST3Client(const VST3Client& client) {
    if (!client.setPreset(presetData)) {
        throw CLIException("Error applying VST3 preset");
    }
}

void PresetLoadingExtensionsVisitor::visitVSTClient(const VSTClient&) {
    throw CLIException("VST preset loading is unsupported");
}

void PresetLoadingExtensionsVisitor::visitAudioUnitClient(const AudioUnitClient&) {
    throw CLIException("AU preset loading is unsupported");
}

void PresetLoadingExtensionsVisitor::visitARAClient(const ARAClient&) {
    throw CLIException("ARA preset loading is unsupported");
}