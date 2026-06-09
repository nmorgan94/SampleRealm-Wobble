#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr int kNumOscillators = 3;
    const juce::String kWaveformAIDs[kNumOscillators] = { "osc1_waveform",   "osc2_waveform",   "osc3_waveform"   };
    const juce::String kWaveformBIDs[kNumOscillators] = { "osc1_waveform_b", "osc2_waveform_b", "osc3_waveform_b" };
    const juce::String kMorphIDs[kNumOscillators]     = { "osc1_morph",      "osc2_morph",      "osc3_morph"      };
}

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
    auto modMatrixState = getOrCreateStateChild(apvts.state, modMatrixStateID);
    auto lfoCurvesState = getOrCreateStateChild(apvts.state, lfoCurvesStateID);
    
    modulationManager.initialise(modMatrixState, &undoManager);
    lfoCurveState.initialise(lfoCurvesState, &undoManager);
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
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

int AudioPluginAudioProcessor::getIntParam(const juce::String& paramID) const
{
    if (auto* param = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter(paramID)))
        return param->get();
    jassertfalse;
    return 0;
}

float AudioPluginAudioProcessor::getModulatedParam(const juce::String& paramID) const
{
    auto* param = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(paramID));
    if (!param)
        return 0.0f;

    auto range = param->getNormalisableRange();
    auto assignment = modulationManager.getAssignment(paramID);

    if (!assignment.isAssigned())
        return range.convertFrom0to1(param->getValue());
    
    // Get modulation value from LFO or Envelope
    float modulationValue = assignment.isLFO()
        ? lfos[assignment.sourceIndex].getCurrentValue()
        : envelopes[assignment.sourceIndex].getCurrentLevel();
    
    // Apply modulation
    float modulated = modulationManager.calculateModulatedValue(modulationValue, assignment.minValue, assignment.maxValue);

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
    synth.setCurrentPlaybackSampleRate(sampleRate);
    
    outputMeterState.reset();

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

    const int wavetableSize = WavetableGenerator::getWavetableSize();
    for (int i = 0; i < kNumOscillators; ++i)
    {
        sourceA[i].setSize(1, wavetableSize);
        sourceB[i].setSize(1, wavetableSize);
        wavetables[i].setSize(1, wavetableSize);
    }
    updateOscillatorTables(true);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;
    filter.prepare(spec);
    updateFilter();

    spec.numChannels = static_cast<juce::uint32>(juce::jmax(1, getTotalNumOutputChannels()));
    drive.prepare(spec);

    synth.clearVoices();
    synth.clearSounds();

    synth.addSound(new WavetableSound());

    for (int i = 0; i < Parameters::maxVoices; ++i)
        synth.addVoice(new WavetableVoice(wavetables, *this));

    currentVoiceCount = -1;
    updateVoiceCount();

    heldNotes.clear();
    heldNotes.reserve(128);
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

    auto playHead = getPlayHead();
    if (playHead != nullptr)
    {
        if (auto positionInfo = playHead->getPosition())
        {
            double bpm = positionInfo->getBpm().orFallback(120.0);
            auto timeSig = positionInfo->getTimeSignature().orFallback(juce::AudioPlayHead::TimeSignature());
            
            for (int i = 0; i < 4; ++i)
            {
                lfos[i].setTempoInfo(bpm, timeSig.numerator, timeSig.denominator);
            }
        }
    }


    // Update LFO rates and modes
    updateLFOs();

    updateVoiceCount();

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

    updateFilter();

    const bool filterEnabled = getBoolParam("filter_enable");

    updateOscillatorTables(false);

    buffer.clear();

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    if (filterEnabled)
    {
        drive.setDrive(getModulatedParam("drive"));
        drive.process(buffer, getBoolParam("drive_hq"));

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                channelData[sample] = filter.processSample(channelData[sample], channel);
            }
        }
    }
    
    const float masterGainDb = getFloatParam("master_gain");
    buffer.applyGain(juce::Decibels::decibelsToGain(masterGainDb));
    
    outputMeterState.processBlock(buffer);
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
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
        
        auto modMatrixState = getOrCreateStateChild(apvts.state, modMatrixStateID);
        auto lfoCurvesState = getOrCreateStateChild(apvts.state, lfoCurvesStateID);
        
        modulationManager.initialise(modMatrixState, &undoManager);
        lfoCurveState.initialise(lfoCurvesState, &undoManager);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}


//==============================================================================
juce::ValueTree AudioPluginAudioProcessor::getOrCreateStateChild(juce::ValueTree parent, const juce::Identifier& childID)
{
    auto child = parent.getChildWithName(childID);
    if (! child.isValid())
    {
        child = juce::ValueTree(childID);
        parent.addChild(child, -1, nullptr);
    }
    
    return child;
}

void AudioPluginAudioProcessor::updateOscillatorTables(bool forceRegen)
{
    // Regenerate a source only when its waveform choice changes (or on forceRegen, used
    // once at prepare), and reblend only when a source or the modulated morph changed.
    for (int i = 0; i < kNumOscillators; ++i)
    {
        bool sourcesChanged = false;

        const auto newA = static_cast<WaveformType>(
            static_cast<int>(apvts.getRawParameterValue(kWaveformAIDs[i])->load()));
        if (forceRegen || newA != currentWaveformTypes[i])
        {
            currentWaveformTypes[i] = newA;
            WavetableGenerator::generateWavetable(sourceA[i], newA);
            sourcesChanged = true;
        }

        const auto newB = static_cast<WaveformType>(
            static_cast<int>(apvts.getRawParameterValue(kWaveformBIDs[i])->load()));
        if (forceRegen || newB != currentWaveformTypesB[i])
        {
            currentWaveformTypesB[i] = newB;
            WavetableGenerator::generateWavetable(sourceB[i], newB);
            sourcesChanged = true;
        }

        const float morph = getModulatedParam(kMorphIDs[i]);
        if (sourcesChanged || morph != lastMorph[i])
            blendWavetable(i, morph);
    }
}

void AudioPluginAudioProcessor::blendWavetable(int oscIndex, float morph)
{
    const int    n   = wavetables[oscIndex].getNumSamples();
    const float* a   = sourceA[oscIndex].getReadPointer(0);
    const float* b   = sourceB[oscIndex].getReadPointer(0);
    float*       out = wavetables[oscIndex].getWritePointer(0);

    const float m = juce::jlimit(0.0f, 1.0f, morph);
    for (int s = 0; s < n; ++s)
        out[s] = a[s] + m * (b[s] - a[s]);

    lastMorph[oscIndex] = morph;
}

void AudioPluginAudioProcessor::updateLFOs()
{
    // Update LFO rates and modes from parameters
    for (int i = 0; i < 4; ++i)
    {
        juce::String lfoPrefix = "lfo" + juce::String(i + 1) + "_";
        
        float rate = getModulatedParam(lfoPrefix + "rate");
        lfos[i].setRate(rate);
        
        int modeIndex = getChoiceParam(lfoPrefix + "mode");
        bool triggerMode = (modeIndex == 0);
        lfos[i].setTriggerMode(triggerMode);
        
        bool tempoSync = getBoolParam(lfoPrefix + "sync");
        lfos[i].setTempoSync(tempoSync);
        
        int tempoDivIndex = getChoiceParam(lfoPrefix + "sync_rate");
        lfos[i].setTempoDivision(tempoDivIndex);
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

void AudioPluginAudioProcessor::updateVoiceCount()
{
    const int desiredVoices = juce::roundToInt(getModulatedParam("num_voices"));

    if (desiredVoices == currentVoiceCount)
        return;

    currentVoiceCount = desiredVoices;

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        const bool isWithinLimit = i < desiredVoices;

        if (auto* voice = dynamic_cast<WavetableVoice*>(synth.getVoice(i)))
        {
            // Release any voice being switched off so it stops sounding.
            if (! isWithinLimit && voice->isVoiceActive())
                voice->stopNote(0.0f, true);

            voice->setWithinVoiceLimit(isWithinLimit);
        }
    }
}
