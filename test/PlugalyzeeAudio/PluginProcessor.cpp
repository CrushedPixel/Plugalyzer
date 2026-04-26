#include "PluginProcessor.h"

#include "PlugalyzeeAudio.h"
#include <format>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

using namespace juce;

// clang-format off
PlugalyzeeAudioProcessor::PlugalyzeeAudioProcessor() :
    AudioProcessor(
        BusesProperties()
            .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
            .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
    ),
    state(*this, nullptr, id::apvtsRoot, createParameterLayout())
// clang-format on
{
    state.state.getOrCreateChildWithName(id::version, nullptr).setProperty(id::version, parameterVersion, nullptr);
    state.state.getOrCreateChildWithName(id::extraState, nullptr).addChild(ValueTree{ id::extraA }, -1, nullptr);
    state.state.getChildWithName(id::extraState).addChild(ValueTree{ id::extraB }, -1, nullptr);

    params.inGain = static_cast<AudioParameterFloat*>(state.getParameter(id::inGainDecibels));
    params.ratio = static_cast<AudioParameterFloat*>(state.getParameter(id::ratio));
    params.threshold = static_cast<AudioParameterFloat*>(state.getParameter(id::threshold));
    params.attack = static_cast<AudioParameterInt*>(state.getParameter(id::attack));
    params.release = static_cast<AudioParameterInt*>(state.getParameter(id::release));
    params.outGain = static_cast<AudioParameterFloat*>(state.getParameter(id::outGainDecibels));

    paramDebugger = std::make_unique<ParameterUpdateDebugger>(state, params);
}

PlugalyzeeAudioProcessor::~PlugalyzeeAudioProcessor()
{
}

const juce::String PlugalyzeeAudioProcessor::getName() const
{
    return pluginName;
}

bool PlugalyzeeAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool PlugalyzeeAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool PlugalyzeeAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double PlugalyzeeAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PlugalyzeeAudioProcessor::getNumPrograms()
{
    return 1;
}

int PlugalyzeeAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PlugalyzeeAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String PlugalyzeeAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void PlugalyzeeAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void PlugalyzeeAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    DBG(std::format("Prepare to play({},{})", sampleRate, samplesPerBlock));
    jassert(getTotalNumInputChannels() == getTotalNumOutputChannels());
    dsp::ProcessSpec spec{
        .sampleRate = sampleRate,
        .maximumBlockSize = static_cast<uint32>(samplesPerBlock),
        .numChannels = static_cast<uint32>(getTotalNumInputChannels())
    };
    processor.prepare(spec);
}

void PlugalyzeeAudioProcessor::releaseResources()
{
}

bool PlugalyzeeAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    #if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    return true;
#endif
}

void PlugalyzeeAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages
)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;

    processor.setParams(ParameterValues{
        .inGain = params.inGain->get(),
        .ratio = params.ratio->get(),
        .threshold = params.threshold->get(),
        .attack = params.attack->get(),
        .release = params.release->get(),
        .outGain = params.outGain->get(),
    });

    dsp::AudioBlock<float> block{ buffer };
    dsp::ProcessContextReplacing<float> context{ block };

    processor.process(context);
}

bool PlugalyzeeAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PlugalyzeeAudioProcessor::createEditor()
{
    return new GenericAudioProcessorEditor(*this);
}

void PlugalyzeeAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    DBG("getStateInformation()");
    const auto stateCopy = state.copyState();
    if (const auto xml{ stateCopy.createXml() })
    {
        copyXmlToBinary(*xml, destData);
        DBG("Saved state");
    }
}

void PlugalyzeeAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    DBG("setStateInformation()");
    auto xml{ getXmlFromBinary(data, sizeInBytes) };
    if (!xml)
    {
        DBG("Couldn't read state");
        return;
    }
    if (!xml->hasTagName(id::apvtsRoot))
    {
        DBG("Wrong root tag in state");
        return;
    }
    auto candidateValueTree{ juce::ValueTree::fromXml(*xml) };
    auto candidateVersion{ candidateValueTree.getChildWithName(id::version) };
    if (!candidateVersion.isValid())
    {
        DBG("Couldn't get version from state");
        return;
    }
    auto version{ candidateVersion.getProperty(id::version) };
    if (static_cast<int>(version) != parameterVersion)
    {
        DBG("Version mismatch. State not loaded.");
        return;
    }

    state.state = juce::ValueTree::fromXml(*xml);
    DBG("Loaded state");
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PlugalyzeeAudioProcessor();
}
