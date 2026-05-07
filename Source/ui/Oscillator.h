#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ModulatableSlider.h"

// Forward declaration
class AudioPluginAudioProcessor;

class Oscillator : public juce::Component,
                   private juce::Timer,
                   private juce::Button::Listener
{
public:
    Oscillator(AudioPluginAudioProcessor& proc,
               juce::AudioProcessorValueTreeState& apvts,
               const juce::String& enableParamID,
               const juce::String& waveformParamID,
               const juce::String& gainParamID,
               const juce::String& labelText,
               const juce::AudioBuffer<float>& wavetableRef)
        : wavetable(wavetableRef),
          gainSlider(proc, gainParamID)
    {
        
        titleLabel.setText(labelText, juce::dontSendNotification);
        titleLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(titleLabel);
        
        enableButton.addListener(this);
        addAndMakeVisible(enableButton);
        
        waveformSelector.addItem("Sine", 1);
        waveformSelector.addItem("Saw", 2);
        waveformSelector.addItem("Square", 3);
        waveformSelector.addItem("Triangle", 4);
        waveformSelector.addItem("Pulse 25%", 5);
        waveformSelector.addItem("Pulse 10%", 6);
        addAndMakeVisible(waveformSelector);
        
        addAndMakeVisible(gainSlider);
        
        enableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, enableParamID, enableButton);
        waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, waveformParamID, waveformSelector);
        gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, gainParamID, gainSlider);
        
        startTimerHz(30);
    }
    
    ~Oscillator() override
    {
        enableButton.removeListener(this);
        stopTimer();
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        bool isEnabled = enableButton.getToggleState();
        float alpha = isEnabled ? 1.0f : 0.3f;
        
        titleLabel.setAlpha(alpha);
        waveformSelector.setAlpha(alpha);
        waveformSelector.setEnabled(isEnabled);
        gainSlider.setAlpha(alpha);
        gainSlider.setEnabled(isEnabled);
        
        // Draw background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
        
        // Draw border
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 2.0f);
        
        auto waveformArea = bounds.reduced(10);
        waveformArea.removeFromTop(40);
        
        // Draw waveform background
        g.setColour(juce::Colours::black.withAlpha(alpha));
        g.fillRoundedRectangle(waveformArea.toFloat(), 4.0f);
        
        drawWaveform(g, waveformArea.reduced(5), alpha);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        
        auto controlBar = bounds.removeFromTop(30);
        
        enableButton.setBounds(controlBar.removeFromLeft(30));
        
        controlBar.removeFromLeft(10);
        
        auto labelWidth = 80;
        titleLabel.setBounds(controlBar.removeFromLeft(labelWidth));
        
        controlBar.removeFromLeft(10);
        
        auto knobSize = 30;
        auto selectorWidth = controlBar.getWidth() - knobSize - 10;
        waveformSelector.setBounds(controlBar.removeFromLeft(selectorWidth));
        
        controlBar.removeFromLeft(10); 
        
        gainSlider.setBounds(controlBar.removeFromLeft(knobSize).withHeight(knobSize));
    }
    
    void timerCallback() override
    {
        repaint();
    }
    
    void buttonClicked(juce::Button*) override
    {
        repaint();
    }
    
    ModulatableSlider& getGainSlider() { return gainSlider; }

private:
    void drawWaveform(juce::Graphics& g, juce::Rectangle<int> bounds, float alpha)
    {
        if (wavetable.getNumSamples() == 0)
            return;
        
        const auto* samples = wavetable.getReadPointer(0);
        const int numSamples = wavetable.getNumSamples();
        
        juce::Path waveformPath;
        
        const float width = static_cast<float>(bounds.getWidth());
        const float height = static_cast<float>(bounds.getHeight());
        const float centerY = bounds.getY() + height / 2.0f;
        
        for (int i = 0; i < numSamples; ++i)
        {
            float x = bounds.getX() + (i / static_cast<float>(numSamples)) * width;
            float y = centerY - (samples[i] * height * 0.4f);
            
            if (i == 0)
                waveformPath.startNewSubPath(x, y);
            else
                waveformPath.lineTo(x, y);
        }
        
        // Center line
        g.setColour(juce::Colours::grey.withAlpha(0.2f * alpha));
        g.drawLine(bounds.getX(), centerY, bounds.getRight(), centerY, 1.0f);
        
        g.setColour(juce::Colour(0xff00ff41).withAlpha(alpha));
        g.strokePath(waveformPath, juce::PathStrokeType(2.0f));
        
        // Grid lines
        g.setColour(juce::Colours::grey.withAlpha(0.1f * alpha));
        for (int i = 1; i < 4; ++i)
        {
            float y = bounds.getY() + (i / 4.0f) * height;
            g.drawLine(bounds.getX(), y, bounds.getRight(), y, 1.0f);
        }
    }
    
    const juce::AudioBuffer<float>& wavetable;
    
    juce::Label titleLabel;
    juce::ToggleButton enableButton;
    juce::ComboBox waveformSelector;
    ModulatableSlider gainSlider;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Oscillator)
};
