#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel(&customLookAndFeel);
    
    osc1 = std::make_unique<Oscillator>(
        processorRef.apvts, "osc1_enable", "osc1_waveform",
        "OSC 1", processorRef.getWavetable(0));
    addAndMakeVisible(osc1.get());
    
    osc2 = std::make_unique<Oscillator>(
        processorRef.apvts, "osc2_enable", "osc2_waveform",
        "OSC 2", processorRef.getWavetable(1));
    addAndMakeVisible(osc2.get());
    
    osc3 = std::make_unique<Oscillator>(
        processorRef.apvts, "osc3_enable", "osc3_waveform",
        "OSC 3", processorRef.getWavetable(2));
    addAndMakeVisible(osc3.get());
    
    envelopePanel = std::make_unique<EnvelopePanel>();
    addAndMakeVisible(envelopePanel.get());
    
    lfoPanel = std::make_unique<LFOPanel>();
    addAndMakeVisible(lfoPanel.get());

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
    auto titleArea = bounds.removeFromTop(50);
    g.drawText ("SampleRealm: Wobble", titleArea, juce::Justification::centred);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title area
    bounds.removeFromTop(50);
    
    // Padding
    bounds.reduce(10, 5);
    
    // Split into left (oscillators) and right (modulation) sections
    auto rightSection = bounds.removeFromRight(350);
    bounds.removeFromRight(10); // spacing
    
    // Left section: Stack oscillators vertically
    int oscHeight = (bounds.getHeight() - 20) / 3;
    
    osc1->setBounds(bounds.removeFromTop(oscHeight));
    bounds.removeFromTop(10);
    
    osc2->setBounds(bounds.removeFromTop(oscHeight));
    bounds.removeFromTop(10);
    
    osc3->setBounds(bounds);
    
    // Right section: Stack envelope and LFO panels
    int panelHeight = (rightSection.getHeight() - 10) / 2;
    
    envelopePanel->setBounds(rightSection.removeFromTop(panelHeight));
    rightSection.removeFromTop(10);
    
    lfoPanel->setBounds(rightSection);
}
