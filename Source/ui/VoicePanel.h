#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ModulatableSlider.h"
#include "StepperDisplay.h"
#include "../Parameters.h"

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
        voicesDisplay.setRange(1, Parameters::maxVoices);
        voicesDisplay.setShowSign(false);
        voicesDisplay.attachToParameter(proc, apvts, "num_voices");
        addAndMakeVisible(voicesDisplay);

        unisonDisplay.setLabel("UNISON");
        unisonDisplay.setRange(1, Parameters::maxUnison);
        unisonDisplay.setShowSign(false);
        unisonDisplay.attachToParameter(proc, apvts, "unison_voices");
        addAndMakeVisible(unisonDisplay);

        detuneDisplay.setLabel("DETUNE");
        detuneDisplay.setRange(0, 100);
        detuneDisplay.setShowSign(false);
        detuneDisplay.attachToParameter(proc, apvts, "unison_detune");
        addAndMakeVisible(detuneDisplay);

        spreadDisplay.setLabel("SPREAD");
        spreadDisplay.setRange(0, 100);
        spreadDisplay.setShowSign(false);
        spreadDisplay.attachToParameter(proc, apvts, "unison_spread");
        addAndMakeVisible(spreadDisplay);

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

        auto leftColumn = bounds.removeFromLeft(100);
        bounds.removeFromLeft(10); // gap between columns
        auto rightColumn = bounds;

        StepperDisplay* steppers[] = { &voicesDisplay, &unisonDisplay, &detuneDisplay, &spreadDisplay };

        const int stepperHeight = 20;
        const int slotHeight    = leftColumn.getHeight() / 4; // one slot per stepper

        for (auto* stepper : steppers)
        {
            auto slot = leftColumn.removeFromTop(slotHeight);
            stepper->setBounds(slot.withSizeKeepingCentre(slot.getWidth(), stepperHeight));
        }

        glideLabel.setBounds(rightColumn.removeFromTop(15));

        auto sliderSize = juce::jmin(rightColumn.getWidth(), rightColumn.getHeight());
        glideSlider.setBounds(rightColumn.withSizeKeepingCentre(sliderSize, sliderSize));
    }

private:
    juce::Label titleLabel;
    juce::Label glideLabel;
    StepperDisplay voicesDisplay;
    StepperDisplay unisonDisplay;
    StepperDisplay detuneDisplay;
    StepperDisplay spreadDisplay;
    ModulatableSlider glideSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> glideAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoicePanel)
};
