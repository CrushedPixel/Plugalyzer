#include "AudioDiffCommand.h"

#include <print>

std::shared_ptr<CLI::App> AudioDiffCommand::createApp() {
    // don't break these lines, please
    // clang-format off
    std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>("Compares two audio files", "audioDiff");

    app->add_option("-t,--test", inputFilePaths.at(AudioFileRole::test), "Audio to test")->required()->check(CLI::ExistingFile);
    app->add_option("-r,--reference", inputFilePaths.at(AudioFileRole::reference), "Reference audio")->required()->check(CLI::ExistingFile);

    return app;
    // clang-format on
}

void AudioDiffCommand::execute() {
    std::map<AudioFileRole, juce::File> audioFiles;

    for (const auto& [role, inPath] : inputFilePaths) {
        juce::File verifiedInPath;
        if (juce::File::isAbsolutePath(inPath)) {
            verifiedInPath = inPath;
        } else {
            verifiedInPath = juce::File::getCurrentWorkingDirectory().getChildFile(inPath);
        }

        audioFiles.insert({role, juce::File{verifiedInPath}});
    }

    std::println("Reading audio files");

    auto differ = AudioDiff::create(audioFiles);

    if (!differ) {
        for (const auto& [role, errorMessage] : differ.error()) {
            std::println(std::cerr, "{}: {}", stringFromRole(role), errorMessage.toStdString());
        }
        throw FileReadError("Couldn't read files. Aborting.", 1);
    }

    const double rmsThresh = 0.005;

    const auto actualRMS = differ->getDifferenceRMS();

    std::println("Difference in RMS between test audio and reference: {}", actualRMS);

    if (actualRMS > rmsThresh) {
        std::println("Cancellation test failed: detected SNR of {}, which exceeds threshold of {}",
                     actualRMS, rmsThresh);
        throw FailedDiffError("Cancellation test failed", 2);
    }

    std::println("Cancellation test successful.");
}
