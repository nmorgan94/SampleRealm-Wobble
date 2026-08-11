#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ui/ModulatableSlider.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel(&customLookAndFeel);
    
    osc1 = std::make_unique<Oscillator>(
        processorRef, processorRef.apvts, "osc1_enable", "osc1_waveform", "osc1_waveform_b", "osc1_morph",
        "osc1_gain", "osc1_pitch", "osc1_fine", "OSC 1");
    addAndMakeVisible(osc1.get());

    osc2 = std::make_unique<Oscillator>(
        processorRef, processorRef.apvts, "osc2_enable", "osc2_waveform", "osc2_waveform_b", "osc2_morph",
        "osc2_gain", "osc2_pitch", "osc2_fine", "OSC 2");
    addAndMakeVisible(osc2.get());

    osc3 = std::make_unique<Oscillator>(
        processorRef, processorRef.apvts, "osc3_enable", "osc3_waveform", "osc3_waveform_b", "osc3_morph",
        "osc3_gain", "osc3_pitch", "osc3_fine", "OSC 3");
    addAndMakeVisible(osc3.get());
    
    noisePanel = std::make_unique<NoisePanel>(processorRef, processorRef.apvts);
    addAndMakeVisible(noisePanel.get());

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

    presetBar = std::make_unique<PresetBar>(processorRef);
    presetBar->onPresetLoaded = [this]() { refreshAfterPresetLoad(); };
    addAndMakeVisible(presetBar.get());

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
    auto titleArea = bounds.removeFromTop(70).withTrimmedLeft(10);
    g.drawText ("SampleRealm: Modulate", titleArea, juce::Justification::centredLeft);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    auto topBar = bounds.removeFromTop(70);
    topBar.removeFromRight(10);
    if (masterMeter != nullptr)
        masterMeter->setBounds(topBar.removeFromRight(90).reduced(0, 6));

    topBar.removeFromLeft(290);
    if (presetBar != nullptr)
        presetBar->setBounds(topBar.removeFromLeft(180).reduced(0, 20));
    
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
    
    noisePanel->setBounds(middleSection.removeFromTop(50));
    middleSection.removeFromTop(10);

    int filterHeight = 260;
    filterPanel->setBounds(middleSection.removeFromTop(filterHeight));
    middleSection.removeFromTop(10);
    voicePanel->setBounds(middleSection);
    
    int panelHeight = (rightSection.getHeight() - 10) / 2;
    
    lfoPanel->setBounds(rightSection.removeFromTop(panelHeight));
    rightSection.removeFromTop(10);

    envelopePanel->setBounds(rightSection);
}

//==============================================================================
namespace
{
    // Depth-first walk that re-reads each modulation slider's assignment from the manager.
    void refreshModulatableSliders(juce::Component* component)
    {
        for (auto* child : component->getChildren())
        {
            if (auto* slider = dynamic_cast<ModulatableSlider*>(child))
                slider->refreshAssignment();

            refreshModulatableSliders(child);
        }
    }
}

void AudioPluginAudioProcessorEditor::refreshAfterPresetLoad()
{
    // APVTS-attached knobs/menus follow the new parameter values automatically. The two UI
    // caches that don't are the modulation badges and the drawn LFO curves - refresh those.
    refreshModulatableSliders(this);

    if (lfoPanel != nullptr)
        lfoPanel->reloadFromState();
}
