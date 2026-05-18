#pragma once

#include "Utils.h"

#include <chrono>
#include <cstddef>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <nlohmann/json.hpp>

using namespace std::chrono_literals;

/* Base class for generating a channel of audio input for a plugin to process */
class Generator {
  public:
    Generator(double howLoud) : amplitude(howLoud) {}
    virtual ~Generator() = default;

    virtual void prepare(Hertz /* sampleRate */) {}
    virtual void render(juce::dsp::AudioBlock<float>& buffer) { buffer.clear(); }

  protected:
    double amplitude;
};

/* An input bus containing a generator per channel */
class GeneratorInputBus {
  public:
    GeneratorInputBus(std::size_t numChannelsToGenerate, std::chrono::seconds howLong)
        : channels(numChannelsToGenerate), duration(howLong) {
        for (auto& channel : channels) {
            channel = std::make_unique<Generator>(0.0);
        }
    }

    static GeneratorInputBus fromJson(const nlohmann::json& json);

    void prepare(Hertz sampleRate, juce::uint32 blockSize);
    void processChannels(juce::dsp::AudioBlock<float>& buffer);
    std::size_t getDurationInSamples(Hertz sampleRate) const;
    juce::AudioChannelSet getChannelLayout() const;

  private:
    std::vector<std::unique_ptr<Generator>> channels;
    std::chrono::seconds duration;
};

class WhiteNoiseGenerator : public Generator {
  public:
    WhiteNoiseGenerator(double howLoud, juce::int64 randomSeed = juce::Time::currentTimeMillis())
        : Generator(howLoud), random(randomSeed) {}

    void render(juce::dsp::AudioBlock<float>& buffer) override;

  private:
    juce::Random random;
};

class SineGenerator : public Generator {
  public:
    SineGenerator(double howLoud, Hertz freq = 1000.0_Hz) : Generator(howLoud), frequency(freq) {}

    void prepare(Hertz sampleRate) override;
    void render(juce::dsp::AudioBlock<float>& buffer) override;

  private:
    double frequency;
    double currentPhase{ 0.0 };
    double phasePerSample{ 0.0 };
};
