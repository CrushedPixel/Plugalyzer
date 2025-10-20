#pragma once

#include <expected>
#include <map>
#include <string>

#include <juce_audio_formats/juce_audio_formats.h>

/* The meaning of the audio passed in */
enum class AudioFileRole {
    /* The audio to be tested */
    test,
    /* The audio against which the test audio will be compared */
    reference
};

constexpr std::string stringFromRole(AudioFileRole role) {
    if (role == AudioFileRole::test) {
        return "test audio";
    }
    if (role == AudioFileRole::reference) {
        return "reference audio";
    } else {
        return "Unknown audio file role.";
    }
}

/**
 * Compares two audio files
 */
class AudioDiff {
  public:
    /**
     * Create an Audio Differ with a map of files and their roles in the testing.
     * If successful, an AudioDiff will be created.
     * On failure, a corresponding map of error messages will be returned.
     */
    [[nodiscard]] static std::expected<AudioDiff, std::map<AudioFileRole, juce::String>>
    create(const std::map<AudioFileRole, juce::File>& audioFiles);

    [[nodiscard]] std::map<AudioFileRole, juce::Result>
    remap(const std::map<AudioFileRole, juce::File>& audioFiles);

    [[nodiscard]] float getDifferenceRMS() const;

  private:
    [[nodiscard]] juce::Result readBufferFromFile(const juce::File& file, AudioFileRole role);

    std::map<AudioFileRole, juce::AudioBuffer<float>> audio{};
    juce::SharedResourcePointer<juce::AudioFormatManager> formatManager;
};