#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ModulatableSlider.h"
#include "FilterResponseDisplay.h"

class AudioPluginAudioProcessor;

class FilterPanel : public juce::Component,
                    private juce::Button::Listener,
                    private juce::Timer
{
public:
    FilterPanel(AudioPluginAudioProcessor& proc,
                juce::AudioProcessorValueTreeState& apvts)
        : processor(proc),
          cutoffSlider(proc, "filter_cutoff"),
          resonanceSlider(proc, "filter_resonance")
    {
        titleLabel.setText("FILTER", juce::dontSendNotification);
        titleLabel.setJustificationType(juce::Justification::centredLeft);
        titleLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        addAndMakeVisible(titleLabel);
        
        enableButton.addListener(this);
        addAndMakeVisible(enableButton);
        
        modeLabel.setText("Mode", juce::dontSendNotification);
        modeLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(modeLabel);
        
        modeSelector.addItem("Lowpass", 1);
        modeSelector.addItem("Highpass", 2);
        modeSelector.addItem("Bandpass", 3);
        modeSelector.addItem("Notch", 4);
        addAndMakeVisible(modeSelector);
        
        cutoffLabel.setText("Cutoff", juce::dontSendNotification);
        cutoffLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(cutoffLabel);
        
        cutoffSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        cutoffSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(cutoffSlider);
        
        resonanceLabel.setText("Resonance", juce::dontSendNotification);
        resonanceLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(resonanceLabel);
        
        resonanceSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        resonanceSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(resonanceSlider);
        
        enableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, "filter_enable", enableButton);
        modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "filter_mode", modeSelector);
        cutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "filter_cutoff", cutoffSlider);
        resonanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "filter_resonance", resonanceSlider);
        
        addAndMakeVisible(responseDisplay);
    }
    
    ~FilterPanel() override
    {
        stopTimer();
        enableButton.removeListener(this);
    }
    
    void visibilityChanged() override
    {
        if (isVisible() && processor.getBoolParam("filter_enable"))
            startTimerHz(30);
        else
            stopTimer();
    }
    
    void timerCallback() override
    {
        // Update filter response display with modulated values
        bool isEnabled = processor.getBoolParam("filter_enable");
        auto mode = static_cast<FilterMode>(processor.getChoiceParam("filter_mode"));
        float cutoff = processor.getModulatedParam("filter_cutoff");
        float resonance = processor.getModulatedParam("filter_resonance");
        
        responseDisplay.setEnabled(isEnabled);
        responseDisplay.setFilterParameters(mode, cutoff, resonance, processor.getSampleRate());
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        
        // Draw background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
        
        // Draw border
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 2.0f);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        
        auto topRow = bounds.removeFromTop(30);
        enableButton.setBounds(topRow.removeFromLeft(30));
        topRow.removeFromLeft(10);
        titleLabel.setBounds(topRow.removeFromLeft(80));
        
        bounds.removeFromTop(5);
        
        auto modeRow = bounds.removeFromTop(25);
        modeLabel.setBounds(modeRow.removeFromLeft(60));
        modeSelector.setBounds(modeRow.removeFromLeft(150));
        
        bounds.removeFromTop(10);
        
        auto displayArea = bounds.removeFromTop(100);
        responseDisplay.setBounds(displayArea);
        
        bounds.removeFromTop(5);
        
        auto knobArea = bounds;
        int knobWidth = knobArea.getWidth() / 2;
        
        auto cutoffArea = knobArea.removeFromLeft(knobWidth).reduced(5);
        cutoffLabel.setBounds(cutoffArea.removeFromTop(15));
        auto cutoffSliderSize = juce::jmin(cutoffArea.getWidth(), cutoffArea.getHeight());
        cutoffSlider.setBounds(cutoffArea.withSizeKeepingCentre(cutoffSliderSize, cutoffSliderSize));
        
        auto resonanceArea = knobArea.reduced(5);
        resonanceLabel.setBounds(resonanceArea.removeFromTop(15));
        auto resonanceSliderSize = juce::jmin(resonanceArea.getWidth(), resonanceArea.getHeight());
        resonanceSlider.setBounds(resonanceArea.withSizeKeepingCentre(resonanceSliderSize, resonanceSliderSize));
    }
    
private:
    void buttonClicked(juce::Button*) override
    {
        bool isEnabled = enableButton.getToggleState();
        float alpha = isEnabled ? 1.0f : 0.3f;
        
        titleLabel.setAlpha(alpha);
        modeLabel.setAlpha(alpha);
        modeSelector.setAlpha(alpha);
        modeSelector.setEnabled(isEnabled);
        
        cutoffLabel.setAlpha(alpha);
        cutoffSlider.setAlpha(alpha);
        cutoffSlider.setEnabled(isEnabled);
        
        resonanceLabel.setAlpha(alpha);
        resonanceSlider.setAlpha(alpha);
        resonanceSlider.setEnabled(isEnabled);
        responseDisplay.setEnabled(isEnabled);
        
        if (isEnabled && isVisible())
            startTimerHz(30);
        else
            stopTimer();
        
        repaint();
    }
    
    AudioPluginAudioProcessor& processor;
    
    juce::Label titleLabel;
    juce::ToggleButton enableButton;
    
    juce::Label modeLabel;
    juce::ComboBox modeSelector;
    
    juce::Label cutoffLabel;
    ModulatableSlider cutoffSlider;
    
    juce::Label resonanceLabel;
    ModulatableSlider resonanceSlider;
    
    FilterResponseDisplay responseDisplay;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> resonanceAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterPanel)
};
