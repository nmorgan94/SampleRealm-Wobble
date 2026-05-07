#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "CurveEditor.h"
#include "CustomLFOTabButton.h"

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
        for (int i = 1; i <= 4; ++i)
        {
            auto editor = std::make_unique<CurveEditor>();
            curveEditors.push_back(std::move(editor));
            
            tabbedComponent.addTab("LFO " + juce::String(i),
                                  juce::Colour(0xff1a1a1a),
                                  curveEditors.back().get(),
                                  false);
        }
        
        tabbedComponent.setTabBarDepth(40);
        tabbedComponent.setOutline(0);
        
        addAndMakeVisible(tabbedComponent);
        
        // Register as listener for all curve editors and sync initial curves
        for (size_t i = 0; i < 4; ++i)
        {
            curveEditors[i]->addListener(this);
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
    }
    
private:
    void syncCurveToLFO(CurveEditor* editor, size_t lfoIndex)
    {
        auto curveFunction = [editor](float x) { return editor->getValueAt(x); };
        processor.getLFO(lfoIndex).syncFromCurve(curveFunction);
    }
    
    AudioPluginAudioProcessor& processor;
    CustomLFOTabbedComponent tabbedComponent;
    std::vector<std::unique_ptr<CurveEditor>> curveEditors;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOPanel)
};
