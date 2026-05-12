#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "Parameters", Parameters::createLayout())
{
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::AudioBuffer<float>& AudioPluginAudioProcessor::getWavetable(int oscIndex) const
{
    return wavetables[oscIndex];
}

bool AudioPluginAudioProcessor::getBoolParam(const juce::String& paramID) const
{
    if (auto* param = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(paramID)))
        return param->get();
    jassertfalse;
    return false;
}

float AudioPluginAudioProcessor::getFloatParam(const juce::String& paramID) const
{
    if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(paramID)))
        return param->get();
    jassertfalse;
    return 0.0f;
}

int AudioPluginAudioProcessor::getChoiceParam(const juce::String& paramID) const
{
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(paramID)))
        return param->getIndex();
    jassertfalse;
    return 0;
}

float AudioPluginAudioProcessor::getModulatedParam(const juce::String& paramID) const
{
    auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(paramID));
    if (!param)
        return 0.0f;
    
    float baseValue = param->get();
    auto assignment = modulationManager.getAssignment(paramID);
    
    if (!assignment.isAssigned())
        return baseValue;
    
    // Get modulation value from LFO or Envelope
    float modulationValue = assignment.isLFO()
        ? lfos[assignment.sourceIndex].getCurrentValue()
        : envelopes[assignment.sourceIndex].getCurrentLevel();
    
    // Apply modulation
    auto range = param->getNormalisableRange();
    float normalized = range.convertTo0to1(baseValue);
    float modulated = modulationManager.calculateModulatedValue(normalized, modulationValue, assignment.depth);
    
    return range.convertFrom0to1(modulated);
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    
    synth.setCurrentPlaybackSampleRate(sampleRate);
    
    // Generate the initial wavetables for all oscillators
    const juce::String waveformParamIDs[3] = { "osc1_waveform", "osc2_waveform", "osc3_waveform" };
    
    for (int i = 0; i < 3; ++i)
    {
        currentWaveformTypes[i] = static_cast<WaveformType>(
            static_cast<int>(apvts.getRawParameterValue(waveformParamIDs[i])->load()));
        generateWavetable(i, currentWaveformTypes[i]);
    }
    
    // Initialize LFOs
    for (int i = 0; i < 4; ++i)
    {
        lfos[i].prepare(sampleRate);
        lfos[i].setRate(getFloatParam("lfo" + juce::String(i + 1) + "_rate"));
    }
    
    // Initialize envelopes
    for (int i = 0; i < 4; ++i)
    {
        envelopes[i].setSampleRate(sampleRate);
        
        juce::String envPrefix = "env" + juce::String(i + 1) + "_";
        float attack = getFloatParam(envPrefix + "attack");
        float decay = getFloatParam(envPrefix + "decay");
        float sustain = getFloatParam(envPrefix + "sustain");
        float release = getFloatParam(envPrefix + "release");
        
        envelopes[i].setParameters(attack, decay, sustain, release);
    }
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;
    filter.prepare(spec);
    updateFilter();
    
    synth.clearVoices();
    synth.clearSounds();
    
    synth.addSound(new WavetableSound());
    
    for (int i = 0; i < 8; ++i)
        synth.addVoice(new WavetableVoice(wavetables, *this));
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any input channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Check if any oscillator settings have changed
    const juce::String waveformParamIDs[3] = { "osc1_waveform", "osc2_waveform", "osc3_waveform" };
    
    for (int i = 0; i < 3; ++i)
    {
        auto newWaveformType = static_cast<WaveformType>(
            static_cast<int>(apvts.getRawParameterValue(waveformParamIDs[i])->load()));
        
        if (newWaveformType != currentWaveformTypes[i])
        {
            currentWaveformTypes[i] = newWaveformType;
            generateWavetable(i, newWaveformType);
        }
    }
    
    // Update LFO rates and modes
    updateLFOs();
    
    updateFilter();
    
    bool hasActiveVoices = false;
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (synth.getVoice(i)->isVoiceActive())
        {
            hasActiveVoices = true;
            break;
        }
    }
    
    // Advance LFOs
    for (int i = 0; i < 4; ++i)
    {
        bool isAssigned = modulationManager.isLFOAssigned(i);
        
        if (lfos[i].isTriggerMode())
        {
            if (isAssigned && hasActiveVoices)
            {
                lfos[i].advance(buffer.getNumSamples());
            }
            else
            {
                // Reset phase to start when no notes are playing or LFO not assigned
                lfos[i].trigger();
            }
        }
        else
        {
            if (isAssigned)
            {
                lfos[i].advance(buffer.getNumSamples());
            }
        }
    }
    for (int i = 0; i < 4; ++i)
    {
        if (modulationManager.isEnvelopeAssigned(i) && envelopes[i].isActive())
        {
            // Advance per-sample like the per-voice envelope does
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                envelopes[i].getNextSample();
            }
        }
    }

    buffer.clear();
    
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    
    if (getBoolParam("filter_enable"))
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                channelData[sample] = filter.processSample(channelData[sample], channel);
            }
        }
    }
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; 
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Save APVTS state
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    
    // Add LFO curve data to XML
    for (size_t i = 0; i < 4; ++i)
    {
        auto* lfoElement = xml->createNewChildElement("LFO" + juce::String(i + 1));
        
        if (lfoElement != nullptr)
        {
            const auto& points = lfoCurvePoints[i];
            lfoElement->setAttribute("numPoints", juce::String(points.size()));
            
            for (size_t j = 0; j < points.size(); ++j)
            {
                lfoElement->setAttribute("point" + juce::String(j) + "_x", points[j].x);
                lfoElement->setAttribute("point" + juce::String(j) + "_y", points[j].y);
            }
        }
    }
    
    copyXmlToBinary (*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
    {
        // Restore APVTS state
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
        
        // Restore LFO curve data
        for (size_t i = 0; i < 4; ++i)
        {
            auto* lfoElement = xmlState->getChildByName("LFO" + juce::String(i + 1));
            
            if (lfoElement != nullptr)
            {
                auto numPoints = lfoElement->getIntAttribute("numPoints", 0);
                
                if (numPoints > 0)
                {
                    std::vector<CurveEditor::ControlPoint> points;
                    
                    for (int j = 0; j < numPoints; ++j)
                    {
                        float x = (float)lfoElement->getDoubleAttribute("point" + juce::String(j) + "_x", 0.0);
                        float y = (float)lfoElement->getDoubleAttribute("point" + juce::String(j) + "_y", 0.5);
                        points.push_back(CurveEditor::ControlPoint(x, y));
                    }
                    
                    lfoCurvePoints[i] = points;
                }
            }
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}


//==============================================================================
void AudioPluginAudioProcessor::generateWavetable(int oscIndex, WaveformType type)
{
    const int wavetableSize = 2048;
    wavetables[oscIndex].setSize(1, wavetableSize);
    
    auto* samples = wavetables[oscIndex].getWritePointer(0);
    
    switch (type)
    {
        case WaveformType::Sine:
        {
            for (int i = 0; i < wavetableSize; ++i)
            {
                auto phase = juce::MathConstants<float>::twoPi * i / wavetableSize;
                samples[i] = std::sin(phase);
            }
            break;
        }
        
        case WaveformType::Saw:
        {
            for (int i = 0; i < wavetableSize; ++i)
            {
                samples[i] = 2.0f * i / wavetableSize - 1.0f;
            }
            break;
        }
        
        case WaveformType::Square:
        {
            for (int i = 0; i < wavetableSize; ++i)
            {
                samples[i] = (i < wavetableSize / 2) ? 1.0f : -1.0f;
            }
            break;
        }
        
        case WaveformType::Triangle:
        {
            for (int i = 0; i < wavetableSize; ++i)
            {
                if (i < wavetableSize / 2)
                    samples[i] = -1.0f + 4.0f * i / wavetableSize;
                else
                    samples[i] = 3.0f - 4.0f * i / wavetableSize;
            }
            break;
        }
        
        case WaveformType::Pulse25:
        {
            for (int i = 0; i < wavetableSize; ++i)
            {
                samples[i] = (i < wavetableSize / 4) ? 1.0f : -1.0f;
            }
            break;
        }
        
        case WaveformType::Pulse10:
        {
            for (int i = 0; i < wavetableSize; ++i)
            {
                samples[i] = (i < wavetableSize / 10) ? 1.0f : -1.0f;
            }
            break;
        }
        
        case WaveformType::FormantSine:
        {
            for (int i = 0; i < wavetableSize; ++i)
            {
                float phase = juce::MathConstants<float>::twoPi * i / wavetableSize;
                float baseSine = std::sin(phase);
                samples[i] = std::pow(std::abs(baseSine), 0.5f) * (baseSine > 0 ? 1 : -1);
            }
            break;
        }
        
    }
}

void AudioPluginAudioProcessor::updateWavetables()
{
    for (int i = 0; i < 3; ++i)
        generateWavetable(i, currentWaveformTypes[i]);
}

void AudioPluginAudioProcessor::updateLFOs()
{
    // Update LFO rates and modes from parameters
    for (int i = 0; i < 4; ++i)
    {
        juce::String lfoPrefix = "lfo" + juce::String(i + 1) + "_";
        
        float rate = getFloatParam(lfoPrefix + "rate");
        lfos[i].setRate(rate);
        
        int modeIndex = getChoiceParam(lfoPrefix + "mode");
        bool triggerMode = (modeIndex == 0);
        lfos[i].setTriggerMode(triggerMode);
    }
}

void AudioPluginAudioProcessor::updateFilter()
{
    int modeIndex = getChoiceParam("filter_mode");
    filter.setMode(static_cast<FilterMode>(modeIndex));
    
    float cutoffHz = getModulatedParam("filter_cutoff");
    filter.setCutoff(cutoffHz);
    
    float resonance = getModulatedParam("filter_resonance");
    filter.setResonance(resonance);
}
