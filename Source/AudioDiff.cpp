#include "AudioDiff.h"

std::expected<AudioDiff, std::map<AudioFileRole, juce::String>>
AudioDiff::create(const std::map<AudioFileRole, juce::File>& audioFiles) {
    AudioDiff diff{};
    diff.formatManager->registerBasicFormats();
    std::map<AudioFileRole, juce::String> errorMessages{};

    auto results = diff.remap(audioFiles);

    for (const auto& [role, res] : results) {
        if (res.failed()) {
            errorMessages.insert({role, res.getErrorMessage()});
        }
    }

    if (errorMessages.empty()) {
        return diff;
    } else {
        return std::unexpected(errorMessages);
    }
}

std::map<AudioFileRole, juce::Result>
AudioDiff::remap(const std::map<AudioFileRole, juce::File>& audioFiles) {
    std::map<AudioFileRole, juce::Result> results;
    for (const auto& [role, file] : audioFiles) {
        results.insert({role, readBufferFromFile(file, role)});
    }
    return results;
}

[[nodiscard]] juce::Result AudioDiff::readBufferFromFile(const juce::File& file,
                                                         AudioFileRole role) {
    using namespace juce;

    if (!file.existsAsFile()) {
        return Result::fail("Input file '" + file.getFullPathName() + "' not found.");
    }

    if (std::unique_ptr<juce::AudioFormatReader> reader{formatManager->createReaderFor(file)}) {
        const auto numChannels = static_cast<int>(reader->numChannels);
        const auto numSamples = static_cast<int>(reader->lengthInSamples);

        audio[role].setSize(numChannels, numSamples,
                            false, // keepExistingContent
                            true,  // clearExtraSpace
                            false  // avoidReallocating
        );

        reader->read(&audio[role], 0, numSamples, 0, true, true);

        return Result::ok();
    } else {
        return Result::fail("Could not read file: " + file.getFullPathName());
    }
}

[[nodiscard]] float AudioDiff::getDifferenceRMS() const {
    auto& reference = audio.at(AudioFileRole::reference);
    auto& test = audio.at(AudioFileRole::test);

    const auto numSamples = std::min(reference.getNumSamples(), test.getNumSamples());
    const auto numChannels = std::min(reference.getNumChannels(), test.getNumChannels());

    juce::AudioBuffer<float> difference{numChannels, numSamples};

    for (auto chan = 0; chan < numChannels; ++chan) {
        difference.copyFrom(chan, 0, test, chan, 0, numSamples);

        juce::FloatVectorOperations::subtract(difference.getWritePointer(chan),
                                              reference.getReadPointer(chan), numSamples);
    }

    float totalRMS = 0.f;

    for (auto chan = 0; chan < numChannels; ++chan)
        totalRMS += difference.getRMSLevel(chan, 0, numSamples);

    return totalRMS / static_cast<float>(numChannels);
}