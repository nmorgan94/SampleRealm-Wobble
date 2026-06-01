#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ModulatableSlider.h"
#include "StepperDisplay.h"

class AudioPluginAudioProcessor;

class VoicePanel : public juce::Component
{
public:
    VoicePanel(AudioPluginAudioProcessor& proc,
               juce::AudioProcessorValueTreeState& apvts)
        : glideSlider(proc, "glide_time")
    {
        titleLabel.setText("VOICE", juce::dontSendNotification);
        titleLabel.setJustificationType(juce::Justification::centredLeft);
        titleLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        addAndMakeVisible(titleLabel);

        voicesDisplay.setLabel("VOICES");
        voicesDisplay.setRange(1, 8);
        voicesDisplay.setShowSign(false);
        voicesDisplay.attachToParameter(apvts, "num_voices");
        addAndMakeVisible(voicesDisplay);

        glideLabel.setText("Glide", juce::dontSendNotification);
        glideLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(glideLabel);

        addAndMakeVisible(glideSlider);

        glideAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "glide_time", glideSlider);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);

        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 2.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);

        titleLabel.setBounds(bounds.removeFromTop(30).removeFromLeft(100));

        bounds.removeFromTop(5);

        voicesDisplay.setBounds(bounds.removeFromTop(20).removeFromLeft(90));

        bounds.removeFromTop(5);

        auto glideArea = bounds.reduced(5);
        glideLabel.setBounds(glideArea.removeFromTop(15));

        auto sliderSize = juce::jmin(glideArea.getWidth(), glideArea.getHeight());
        glideSlider.setBounds(glideArea.withSizeKeepingCentre(sliderSize, sliderSize));
    }

private:
    juce::Label titleLabel;
    juce::Label glideLabel;
    StepperDisplay voicesDisplay;
    ModulatableSlider glideSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> glideAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoicePanel)
};
