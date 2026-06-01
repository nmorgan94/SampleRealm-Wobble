#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel(&customLookAndFeel);
    
    osc1 = std::make_unique<Oscillator>(
        processorRef, processorRef.apvts, "osc1_enable", "osc1_waveform", "osc1_gain", "osc1_pitch",
        "OSC 1", processorRef.getWavetable(0));
    addAndMakeVisible(osc1.get());
    
    osc2 = std::make_unique<Oscillator>(
        processorRef, processorRef.apvts, "osc2_enable", "osc2_waveform", "osc2_gain", "osc2_pitch",
        "OSC 2", processorRef.getWavetable(1));
    addAndMakeVisible(osc2.get());
    
    osc3 = std::make_unique<Oscillator>(
        processorRef, processorRef.apvts, "osc3_enable", "osc3_waveform", "osc3_gain", "osc3_pitch",
        "OSC 3", processorRef.getWavetable(2));
    addAndMakeVisible(osc3.get());
    
    filterPanel = std::make_unique<FilterPanel>(processorRef, processorRef.apvts);
    addAndMakeVisible(filterPanel.get());

    voicePanel = std::make_unique<VoicePanel>(processorRef, processorRef.apvts);
    addAndMakeVisible(voicePanel.get());
    
    envelopePanel = std::make_unique<EnvelopePanel>(processorRef);
    addAndMakeVisible(envelopePanel.get());
    
    lfoPanel = std::make_unique<LFOPanel>(processorRef);
    addAndMakeVisible(lfoPanel.get());
    
    masterMeter = std::make_unique<MasterMeter>(processorRef, processorRef.apvts);
    addAndMakeVisible(masterMeter.get());

    setSize (900, 600);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    
    auto bounds = getLocalBounds();
    
    g.setColour (juce::Colour(0xff00ff00));
    g.setFont (CustomLookAndFeel::orbitronBold().withHeight(24.0f));
    auto titleArea = bounds.removeFromTop(70);
    g.drawText ("SampleRealm: Wobble", titleArea, juce::Justification::centred);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    auto topBar = bounds.removeFromTop(70);
    topBar.removeFromRight(10);
    if (masterMeter != nullptr)
        masterMeter->setBounds(topBar.removeFromRight(90).reduced(0, 6));
    
    // Padding
    bounds.reduce(10, 5);
    
    // Split into left (oscillators), middle (filter), and right (modulation) sections
    auto rightSection = bounds.removeFromRight(350);
    bounds.removeFromRight(10); 
    
    int filterWidth = 260;
    
    auto middleSection = bounds.removeFromRight(filterWidth);
    bounds.removeFromRight(10); // spacing
    
    // Left section: Stack oscillators vertically
    int oscHeight = (bounds.getHeight() - 20) / 3;
    
    osc1->setBounds(bounds.removeFromTop(oscHeight));
    bounds.removeFromTop(10);
    
    osc2->setBounds(bounds.removeFromTop(oscHeight));
    bounds.removeFromTop(10);
    
    osc3->setBounds(bounds);
    
    int filterHeight = middleSection.getHeight() / 2;
    filterPanel->setBounds(middleSection.removeFromTop(filterHeight));
    middleSection.removeFromTop(10);
    voicePanel->setBounds(middleSection);
    
    int panelHeight = (rightSection.getHeight() - 10) / 2;
    
    lfoPanel->setBounds(rightSection.removeFromTop(panelHeight));
    rightSection.removeFromTop(10);
    
    envelopePanel->setBounds(rightSection);
}
