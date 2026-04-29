#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel(&customLookAndFeel);
    
    osc1 = std::make_unique<Oscillator>(
        processorRef.apvts, "osc1_enable", "osc1_waveform",
        "Oscillator 1", processorRef.getWavetable(0));
    addAndMakeVisible(osc1.get());
    
    osc2 = std::make_unique<Oscillator>(
        processorRef.apvts, "osc2_enable", "osc2_waveform",
        "Oscillator 2", processorRef.getWavetable(1));
    addAndMakeVisible(osc2.get());
    
    osc3 = std::make_unique<Oscillator>(
        processorRef.apvts, "osc3_enable", "osc3_waveform",
        "Oscillator 3", processorRef.getWavetable(2));
    addAndMakeVisible(osc3.get());

    setSize (800, 500);
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
    auto titleArea = bounds.removeFromTop(50);
    g.drawText ("SampleRealm: Wobble", titleArea, juce::Justification::centred);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    bounds.removeFromTop(50);
    
    bounds.reduce(20, 10);
    
    int oscWidth = bounds.getWidth() / 3 - 10;
    
    auto osc1Area = bounds.removeFromLeft(oscWidth);
    osc1->setBounds(osc1Area);
    
    bounds.removeFromLeft(15);
    
    auto osc2Area = bounds.removeFromLeft(oscWidth);
    osc2->setBounds(osc2Area);
    
    bounds.removeFromLeft(15);
    
    auto osc3Area = bounds.removeFromLeft(oscWidth);
    osc3->setBounds(osc3Area);
}
