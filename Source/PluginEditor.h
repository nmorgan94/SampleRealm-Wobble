#pragma once

#include "PluginProcessor.h"
#include "ui/CustomLookAndFeel.h"
#include "ui/Oscillator.h"
#include "ui/EnvelopePanel.h"
#include "ui/LFOPanel.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                               public juce::DragAndDropContainer
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    std::unique_ptr<Oscillator> osc1;
    std::unique_ptr<Oscillator> osc2;
    std::unique_ptr<Oscillator> osc3;
    
    std::unique_ptr<EnvelopePanel> envelopePanel;
    std::unique_ptr<LFOPanel> lfoPanel;
    
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;
    
    CustomLookAndFeel customLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
