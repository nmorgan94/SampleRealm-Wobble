#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "CurveEditor.h"

class LFOPanel : public juce::Component
{
public:
    LFOPanel() : tabbedComponent(juce::TabbedButtonBar::TabsAtTop)
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
    
    CurveEditor* getCurveEditor(int lfoIndex)
    {
        jassert(lfoIndex >= 0 && lfoIndex < (int)curveEditors.size());
        return curveEditors[lfoIndex].get();
    }

private:
    juce::TabbedComponent tabbedComponent;
    std::vector<std::unique_ptr<CurveEditor>> curveEditors;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOPanel)
};
