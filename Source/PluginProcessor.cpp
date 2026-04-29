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
    const juce::String enableParamIDs[3] = { "osc1_enable", "osc2_enable", "osc3_enable" };
    
    for (int i = 0; i < 3; ++i)
    {
        currentWaveformTypes[i] = static_cast<WaveformType>(
            static_cast<int>(apvts.getRawParameterValue(waveformParamIDs[i])->load()));
        oscEnabled[i] = apvts.getRawParameterValue(enableParamIDs[i])->load() > 0.5f;
        generateWavetable(i, currentWaveformTypes[i]);
    }
    
    synth.clearVoices();
    synth.clearSounds();
    
    synth.addSound(new WavetableSound());
    
    for (int i = 0; i < 8; ++i)
        synth.addVoice(new WavetableVoice(wavetables, oscEnabled));
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
    const juce::String enableParamIDs[3] = { "osc1_enable", "osc2_enable", "osc3_enable" };
    
    bool needsUpdate = false;
    
    for (int i = 0; i < 3; ++i)
    {
        auto newWaveformType = static_cast<WaveformType>(
            static_cast<int>(apvts.getRawParameterValue(waveformParamIDs[i])->load()));
        bool newEnabled = apvts.getRawParameterValue(enableParamIDs[i])->load() > 0.5f;
        
        if (newWaveformType != currentWaveformTypes[i] || newEnabled != oscEnabled[i])
        {
            currentWaveformTypes[i] = newWaveformType;
            oscEnabled[i] = newEnabled;
            needsUpdate = true;
        }
    }
    
    if (needsUpdate)
        updateWavetables();

    buffer.clear();
    
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
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
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
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
    }
}

void AudioPluginAudioProcessor::updateWavetables()
{
    for (int i = 0; i < 3; ++i)
        generateWavetable(i, currentWaveformTypes[i]);
}
