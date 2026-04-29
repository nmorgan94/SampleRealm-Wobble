#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class Oscillator : public juce::Component,
                   private juce::Timer
{
public:
    Oscillator(juce::AudioProcessorValueTreeState& apvts,
               const juce::String& enableParamID,
               const juce::String& waveformParamID,
               const juce::String& labelText,
               const juce::AudioBuffer<float>& wavetableRef)
        : wavetable(wavetableRef)
    {
        titleLabel.setText(labelText, juce::dontSendNotification);
        titleLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(titleLabel);
        
        addAndMakeVisible(enableButton);
        
        waveformSelector.addItem("Sine", 1);
        waveformSelector.addItem("Saw", 2);
        waveformSelector.addItem("Square", 3);
        waveformSelector.addItem("Triangle", 4);
        addAndMakeVisible(waveformSelector);
        
        enableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, enableParamID, enableButton);
        waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, waveformParamID, waveformSelector);
        
        startTimerHz(30);
    }
    
    ~Oscillator() override
    {
        stopTimer();
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        auto waveformArea = bounds.removeFromTop(150);
        
        g.setColour(juce::Colours::black);
        g.fillRect(waveformArea);
        
        g.setColour(juce::Colours::grey);
        g.drawRect(waveformArea, 2);

        drawWaveform(g, waveformArea.reduced(5));
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(150);
        bounds.removeFromTop(10);
        
        titleLabel.setBounds(bounds.removeFromTop(25));
        bounds.removeFromTop(5);
        
        enableButton.setBounds(bounds.removeFromTop(30));
        bounds.removeFromTop(5);
        
        waveformSelector.setBounds(bounds.removeFromTop(30));
    }
    
    void timerCallback() override
    {
        repaint();
    }

private:
    void drawWaveform(juce::Graphics& g, juce::Rectangle<int> bounds)
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
        
        g.setColour(juce::Colours::grey.withAlpha(0.3f));
        g.drawLine(bounds.getX(), centerY, bounds.getRight(), centerY, 1.0f);
        
        g.setColour(juce::Colour(0xff00ff41));
        g.strokePath(waveformPath, juce::PathStrokeType(2.0f));
        
        g.setColour(juce::Colours::grey.withAlpha(0.2f));
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
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Oscillator)
};
