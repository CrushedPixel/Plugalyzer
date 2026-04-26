#pragma once

#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

// Gain/trim measured in dB
using decibelFloat = float;
// Gain/trim measure in linear scale
using linearFloat = float;
// Indivisible milliseconds
using millisInt = int;
// Parameter Value normalized to 0-1
using normalizedParameterValue = float;
// Parameter Value full scale
using denormalizedParameterValue = float;

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

static constexpr auto pluginName{ "Plugalyzee Audio" };
static constexpr int parameterVersion{ 1 };

namespace id
{
// Parameters
static constexpr auto inGainDecibels{ "In_Gain" };
static constexpr auto ratio{ "Ratio" };
static constexpr auto threshold{ "Threshold" };
static constexpr auto attack{ "Attack" };
static constexpr auto release{ "Release" };
static constexpr auto outGainDecibels{ "Out_Gain" };

constexpr std::array paramIds{
    inGainDecibels,
    ratio,
    threshold,
    attack,
    release,
    outGainDecibels
};

// State
static const juce::Identifier version{ "v" };
static const juce::Identifier apvtsRoot{ "PlugalyzeeAPVTS" };
static const juce::Identifier extraState{ "extra" };
static const juce::Identifier extraA{ "extraA" };
static const juce::Identifier extraB{ "extraB" };

// Prefixes/Suffixes
static constexpr auto ratioPrefix{ "1:" };
static constexpr auto dB{ "dB" };
static constexpr auto millis{ "ms" };
} // namespace id

namespace constants
{
static constexpr auto minusInfinitydB{ -96.f };
} // constants

struct PlugalyzeeParams
{
    juce::AudioParameterFloat* inGain{};
    juce::AudioParameterFloat* ratio{};
    juce::AudioParameterFloat* threshold{};
    juce::AudioParameterInt* attack{};
    juce::AudioParameterInt* release{};
    juce::AudioParameterFloat* outGain{};
};

struct ParameterValues
{
    decibelFloat inGain{};
    float ratio{};
    float threshold{};
    millisInt attack{};
    millisInt release{};
    decibelFloat outGain{};
};

class ParameterUpdateDebugger :
    public juce::AudioProcessorParameter::Listener,
    public juce::AudioProcessorValueTreeState::Listener
{
public:
    ParameterUpdateDebugger(
        juce::AudioProcessorValueTreeState& apvtsToUse,
        PlugalyzeeParams& paramsToListenTo
    );
    ~ParameterUpdateDebugger();

    // Parameter Listener
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int, bool) override {};
    // APVTS Listener
    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    std::vector<juce::AudioProcessorParameter*> params;
    std::vector<juce::String> apvtsParams;
    juce::AudioProcessorValueTreeState& apvts;
};

class PlugalyzeeDSP
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void setParams(const ParameterValues paramVals);
    void process(juce::dsp::ProcessContextReplacing<float>& context);

private:
    juce::dsp::Gain<float> procInGain;
    juce::dsp::Compressor<float> procComp;
    juce::dsp::Gain<float> procOutGain;
};
