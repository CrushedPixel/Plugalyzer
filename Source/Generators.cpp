#include "Generators.h"

#include "Errors.h"
#include "Parsers.h"
#include "Utils.h"

#include <cmath>
#include <cstddef>
#include <juce_dsp/juce_dsp.h>
#include <limits>
#include <memory>
#include <string>

static std::unique_ptr<Generator> jsonToGenerator(const nlohmann::json& json) {
    auto generatorType = json["generator"].get<std::string>();

    auto ampToParse = json["amplitude"].get<std::string>();
    auto amplitude = parse::amplitude(ampToParse);

    if (generatorType == "sine") {
        auto freqToParse = json["frequency"].get<std::string>();
        auto frequency = parse::frequency(freqToParse);
        return std::make_unique<SineGenerator>(amplitude, frequency);
    }

    if (generatorType == "white noise") {
        if (json.contains("random seed")) {
            const auto randomSeed = json["random seed"].get<juce::int64>();
            return std::make_unique<WhiteNoiseGenerator>(amplitude, randomSeed);
        } else {
            return std::make_unique<WhiteNoiseGenerator>(amplitude);
        }
    }

    throw ParseError{ std::format("Unknown generator: {}", generatorType), 7 };
}

GeneratorInputBus GeneratorInputBus::fromJson(const nlohmann::json& json) {
    const std::size_t numChannels{ json["num channels"].get<std::size_t>() };
    auto durToParse = json["duration"].get<std::string>();
    auto secs = parse::seconds(durToParse);
    GeneratorInputBus ret{ numChannels, secs };

    for (const auto& [index, channel] : juce::enumerate(json["channels"])) {
        ret.channels.at(static_cast<std::size_t>(index)) = jsonToGenerator(channel);
    }
    return ret;
}
void GeneratorInputBus::prepare(Hertz sampleRate, juce::uint32 /* blockSize */) {
    for (auto& gen : channels) {
        gen->prepare(sampleRate);
    }
}

void GeneratorInputBus::processChannels(juce::dsp::AudioBlock<float>& buffer) {
    for (std::size_t channel{ 0 }; channel < buffer.getNumChannels(); ++channel) {
        auto channelBuffer = buffer.getSingleChannelBlock(channel);
        channels[channel]->render(channelBuffer);
    }
}

size_t GeneratorInputBus::getDurationInSamples(Hertz sampleRate) const {
    jassert(
        juce::approximatelyEqual(sampleRate, static_cast<double>(juce::roundToInt(sampleRate)))
    );
    jassert(sampleRate > 0.0);
    auto sr = static_cast<long long>(juce::roundToInt(sampleRate));

    jassert(duration.count() >= 0);
    const auto seconds = duration.count();
    const auto samples = std::llround(sr * seconds);

#ifdef __cpp_lib_saturate
    const auto clampedSamples = std::saturate_cast<std::size_t>(samples);
#else
    const auto clampedSamples =
        static_cast<std::size_t>(std::clamp(samples, 0LL, std::numeric_limits<long long>::max()));
#endif

    return clampedSamples;
}

juce::AudioChannelSet GeneratorInputBus::getChannelLayout() const {
    return juce::AudioChannelSet::canonicalChannelSet(static_cast<int>(channels.size()));
}

void WhiteNoiseGenerator::render(juce::dsp::AudioBlock<float>& buffer) {
    jassert(buffer.getNumChannels() == 1);
    for (std::size_t sample{ 0 }; sample < buffer.getNumSamples(); ++sample) {
        const auto val = random.nextDouble() * 2.0 - 1.0;
        const auto scaledVal = static_cast<float>(val * amplitude);
        buffer.setSample(0, static_cast<int>(sample), scaledVal);
    }
}

void SineGenerator::prepare(Hertz sampleRate) {
    phasePerSample = juce::MathConstants<double>::twoPi / (sampleRate / frequency);
}

void SineGenerator::render(juce::dsp::AudioBlock<float>& buffer) {
    jassert(buffer.getNumChannels() == 1);
    for (std::size_t sample{ 0 }; sample < buffer.getNumSamples(); ++sample) {
        const auto val = std::sin(currentPhase);
        const auto scaledVal = static_cast<float>(val * amplitude);
        buffer.setSample(0, static_cast<int>(sample), scaledVal);
        currentPhase += phasePerSample;
    }
}
