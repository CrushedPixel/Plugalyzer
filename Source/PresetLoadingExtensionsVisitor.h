#pragma once

#include "Utils.h"

#include <juce_audio_processors/juce_audio_processors.h>

/**
 * Applies the given preset data when used on a plugin.
 * Currently, JUCE only supports VST3 here.
 */
struct PresetLoadingExtensionsVisitor : juce::ExtensionsVisitor {

    const juce::MemoryBlock& presetData;

    explicit PresetLoadingExtensionsVisitor(const juce::MemoryBlock& presetData);

    void visitUnknown(const Unknown&) override;

    void visitVST3Client(const VST3Client& client) override;

    void visitVSTClient(const VSTClient&) override;

    void visitAudioUnitClient(const AudioUnitClient&) override;

    void visitARAClient(const ARAClient&) override;
};