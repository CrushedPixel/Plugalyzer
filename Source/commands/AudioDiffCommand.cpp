#include "AudioDiffCommand.h"

#include "Errors.h"
#include "Parsers.h"
#include "Validators.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <print>


std::shared_ptr<CLI::App> AudioDiffCommand::createApp() {
    // clang-format off
    std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>("Compares two audio files", "audioDiff");

    app->add_option("-t,--test", inputFilePaths.at(AudioFileRole::test), "Audio to test")
        ->required()
        ->check(CLI::ExistingFile);
    app->add_option("-r,--reference", inputFilePaths.at(AudioFileRole::reference), "Reference audio")
        ->required()
        ->check(CLI::ExistingFile);
    app->add_option("-d,--tolerance", argThreshold, "How different the audio can be before it's considered a failure, in RMS.")
        ->check(validate::amplitude)
        ->each([&](std::string arg){ rmsThreshold = parse::amplitude(arg); });

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

        audioFiles.insert({ role, juce::File{ verifiedInPath } });
    }

    auto differ = AudioDiff::create(audioFiles);

    if (!differ) {
        for (const auto& [role, errorMessage] : differ.error()) {
            std::println(std::cerr, "{}: {}", stringFromRole(role), errorMessage.toStdString());
        }
        throw FileLoadError("Couldn't read files. Aborting.", 2);
    }

    const auto actualRMS = differ->getDifferenceRMS();
    const auto actualRMSdB = juce::Decibels::gainToDecibels(actualRMS, -96.f);
    const auto actualRMSdBReadout =
        juce::Decibels::toString(actualRMSdB, 6, -96.f, true, "-inf").toStdString();
    const auto rmsThresholddB = juce::Decibels::gainToDecibels(rmsThreshold, -96.0);
    const auto rmsThresholddBReadout =
        juce::Decibels::toString(rmsThresholddB, 6, -96.0, true, "-inf").toStdString();

    std::println("{}", actualRMSdBReadout);

    if (actualRMS > rmsThreshold) {
        std::println(
            stderr, "Detected SNR of {}, which exceeds threshold of {}", actualRMSdBReadout,
            rmsThresholddBReadout
        );
        throw FailedDiffError("Cancellation test failed", 1);
    }
}
