#include "PlugalyzeeAudio.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <format>

using namespace juce;

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;

    const auto makeGainParam = [](auto paramId) {
        auto humanName = String{ paramId }.replaceCharacter('_', ' ');
        return std::make_unique<AudioParameterFloat>(
            ParameterID{ paramId, parameterVersion },
            humanName,
            NormalisableRange<float>{ -96.f, 12.f },
            0.0f,
            AudioParameterFloatAttributes().withLabel(id::dB)
        );
    };

    const auto makeThresholdParam = [](auto paramId) {
        auto humanName = String{ paramId }.replaceCharacter('_', ' ');
        return std::make_unique<AudioParameterFloat>(
            ParameterID{ paramId, parameterVersion },
            humanName,
            NormalisableRange<float>{ -96.f, 0.f },
            0.1f,
            AudioParameterFloatAttributes().withLabel(id::dB)
        );
    };

    const auto makeMillisParam = [](auto paramId) {
        auto humanName = String{ paramId }.replaceCharacter('_', ' ');
        return std::make_unique<AudioParameterInt>(
            ParameterID{ paramId, parameterVersion },
            humanName,
            0,
            200,
            0,
            AudioParameterIntAttributes().withLabel(id::millis)
        );
    };

    const auto makeRatioParam = [](auto paramId) {
        auto humanName = String{ paramId }.replaceCharacter('_', ' ');

        auto sfv = [](float val, int /* maxLength */) -> String {
            return String{ id::ratioPrefix } + String{ val };
        };

        auto vfs = [](String str) -> float {
            auto val = str.replaceFirstOccurrenceOf(id::ratioPrefix, "");
            auto valRet = val.getFloatValue();
            return valRet;
        };

        return std::make_unique<AudioParameterFloat>(
            ParameterID{ paramId, parameterVersion },
            humanName,
            NormalisableRange<float>{ 1.f, 100.f, 1.f },
            2.0f,
            AudioParameterFloatAttributes().withStringFromValueFunction(sfv).withValueFromStringFunction(vfs)
        );
    };

    std::vector<std::unique_ptr<juce::AudioProcessorParameterGroup>> params;

    auto group{ std::make_unique<AudioProcessorParameterGroup>(
        String{ "maingroup" }, // groupID
        String{ "all" }, // groupName
        String{ "|" } // subgroupSeparator
    ) };

    group->addChild(makeGainParam(id::inGainDecibels));
    group->addChild(makeThresholdParam(id::threshold));
    group->addChild(makeRatioParam(id::ratio));
    group->addChild(makeMillisParam(id::attack));
    group->addChild(makeMillisParam(id::release));
    group->addChild(makeGainParam(id::outGainDecibels));

    params.push_back(std::move(group));

    return { params.begin(), params.end() };
}

ParameterUpdateDebugger::ParameterUpdateDebugger(
    juce::AudioProcessorValueTreeState& apvtsToUse,
    PlugalyzeeParams& paramsToListenTo
) : apvts(apvtsToUse)
{
    paramsToListenTo.inGain->addListener(this);
    params.push_back(paramsToListenTo.inGain);
    paramsToListenTo.ratio->addListener(this);
    params.push_back(paramsToListenTo.ratio);
    paramsToListenTo.threshold->addListener(this);
    params.push_back(paramsToListenTo.threshold);
    paramsToListenTo.attack->addListener(this);
    params.push_back(paramsToListenTo.attack);
    paramsToListenTo.release->addListener(this);
    params.push_back(paramsToListenTo.release);
    paramsToListenTo.outGain->addListener(this);
    params.push_back(paramsToListenTo.outGain);

    for (auto* p : id::paramIds)
    {
        apvts.addParameterListener(p, this);
    }
}

ParameterUpdateDebugger::~ParameterUpdateDebugger()
{
    for (auto* param : params)
    {
        param->removeListener(this);
    }
    for (auto* p : id::paramIds)
    {
        apvts.removeParameterListener(p, this);
    }
}

void ParameterUpdateDebugger::parameterValueChanged(int parameterIndex, float newValue)
{
    DBG(std::format(
        "{} [{}]\tParam update: Index:\t{:<12} Value: {}",
        juce::Time::getCurrentTime().formatted("%H:%M:%S").toStdString(),
        juce::Time::getHighResolutionTicks(),
        parameterIndex,
        newValue
    ));
}

void ParameterUpdateDebugger::parameterChanged(const juce::String& parameterID, float newValue)
{
    DBG(std::format(
        "{} [{}]\tAPVTS update: ID:\t{:<12} Value: {}",
        juce::Time::getCurrentTime().formatted("%H:%M:%S").toStdString(),
        juce::Time::getHighResolutionTicks(),
        parameterID.toStdString(),
        newValue
    ));
}

void PlugalyzeeDSP::prepare(const juce::dsp::ProcessSpec& spec)
{
    procInGain.prepare(spec);
    procComp.prepare(spec);
    procOutGain.prepare(spec);
}

void PlugalyzeeDSP::setParams(const ParameterValues paramVals)
{
    procInGain.setGainLinear(juce::Decibels::decibelsToGain(paramVals.inGain, constants::minusInfinitydB));
    procComp.setRatio(static_cast<float>(paramVals.ratio));
    procComp.setThreshold(paramVals.threshold);
    procComp.setAttack(static_cast<float>(paramVals.attack));
    procComp.setRelease(static_cast<float>(paramVals.release));
    procOutGain.setGainLinear(juce::Decibels::decibelsToGain(paramVals.outGain, constants::minusInfinitydB));
}

void PlugalyzeeDSP::process(juce::dsp::ProcessContextReplacing<float>& context)
{
    procInGain.process(context);
    procComp.process(context);
    procOutGain.process(context);
}