#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "CurveEditor.h"
#include "CustomLFOTabButton.h"
#include "CustomLookAndFeel.h"

// Forward declaration
class AudioPluginAudioProcessor;

class CustomLFOTabbedComponent : public juce::TabbedComponent
{
public:
    CustomLFOTabbedComponent() : juce::TabbedComponent(juce::TabbedButtonBar::TabsAtTop)
    {
    }
    
    juce::TabBarButton* createTabButton(const juce::String& tabName, int tabIndex) override
    {
        return new CustomLFOTabButton(tabName, getTabbedButtonBar(), tabIndex);
    }
};

class LFOPanel : public juce::Component,
                 public CurveEditor::Listener
{
public:
    LFOPanel(AudioPluginAudioProcessor& proc) : processor(proc)
    {
        for (size_t i = 0; i < 4; ++i)
        {
            auto container = std::make_unique<juce::Component>();
            
            auto editor = std::make_unique<CurveEditor>();
            editor->setLFO(&processor.getLFO(i));
            container->addAndMakeVisible(editor.get());
            curveEditors.push_back(std::move(editor));
            
            auto slider = std::make_unique<juce::Slider>();
            CustomLookAndFeel::styleRotarySlider(*slider.get());
            container->addAndMakeVisible(slider.get());
            rateSliders.push_back(std::move(slider));
            
            auto label = std::make_unique<juce::Label>();
            label->setText("Rate", juce::dontSendNotification);
            label->setJustificationType(juce::Justification::centred);
            label->setColour(juce::Label::textColourId, juce::Colour(0xff00ff41));
            container->addAndMakeVisible(label.get());
            rateLabels.push_back(std::move(label));
            
            juce::String paramID = "lfo" + juce::String(i + 1) + "_rate";
            rateAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.apvts, paramID, *rateSliders.back()));
            
            tabContainers.push_back(std::move(container));
            
            tabbedComponent.addTab("LFO " + juce::String(i + 1),
                                  juce::Colour(0xff1a1a1a),
                                  tabContainers.back().get(),
                                  false);
        }
        
        tabbedComponent.setTabBarDepth(40);
        tabbedComponent.setOutline(0);
        addAndMakeVisible(tabbedComponent);
        
        // Register as listener
        for (size_t i = 0; i < 4; ++i)
        {
            curveEditors[i]->addListener(this);
            
            const auto& savedPoints = processor.getLFOCurvePoints(i);
            if (!savedPoints.empty())
                curveEditors[i]->setControlPoints(savedPoints);
            
            syncCurveToLFO(curveEditors[i].get(), i);
        }
    }
    
    ~LFOPanel() override
    {
        // Unregister from curve editors
        for (size_t i = 0; i < 4; ++i)
        {
            curveEditors[i]->removeListener(this);
        }
    }
    
    // CurveEditor::Listener
    void curveChanged(CurveEditor* editor) override
    {
        // Find which LFO this curve editor belongs to
        for (size_t i = 0; i < 4; ++i)
        {
            if (curveEditors[i].get() == editor)
            {
                // Save the curve points to processor
                processor.setLFOCurvePoints(i, editor->getControlPoints());
                
                syncCurveToLFO(editor, i);
                break;
            }
        }
    }
    
    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
        
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 8.0f, 2.0f);
    }
    
    void resized() override
    {
        tabbedComponent.setBounds(getLocalBounds());
        
        // Layout each container: editor on top, controls below
        for (size_t i = 0; i < tabContainers.size(); ++i)
        {
            auto bounds = tabContainers[i]->getLocalBounds().reduced(10);
            
            // Curve editor takes top portion
            int editorHeight = juce::roundToInt(bounds.getHeight() * 0.75f);
            auto editorBounds = bounds.removeFromTop(editorHeight);
            curveEditors[i]->setBounds(editorBounds);
            
            bounds.removeFromTop(10); // Spacing
            
            // Rate control at bottom
            auto knobSize = 50;
            auto knobBounds = bounds.withSizeKeepingCentre(knobSize, knobSize + 16);
            rateLabels[i]->setBounds(knobBounds.removeFromTop(16));
            rateSliders[i]->setBounds(knobBounds.withSizeKeepingCentre(knobSize, knobSize));
        }
    }
    
private:
    void syncCurveToLFO(CurveEditor* editor, size_t lfoIndex)
    {
        auto curveFunction = [editor](float x) { return editor->getValueAt(x); };
        processor.getLFO(lfoIndex).syncFromCurve(curveFunction);
    }
    
    AudioPluginAudioProcessor& processor;
    CustomLFOTabbedComponent tabbedComponent;
    std::vector<std::unique_ptr<juce::Component>> tabContainers;
    std::vector<std::unique_ptr<CurveEditor>> curveEditors;
    std::vector<std::unique_ptr<juce::Slider>> rateSliders;
    std::vector<std::unique_ptr<juce::Label>> rateLabels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> rateAttachments;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOPanel)
};
