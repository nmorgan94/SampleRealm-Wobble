#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class LFOPanel : public juce::Component
{
public:
    LFOPanel() : tabbedComponent(juce::TabbedButtonBar::TabsAtTop)
    {
        for (int i = 1; i <= 4; ++i)
        {
            auto panel = std::make_unique<juce::Component>();
            lfoPanels.push_back(std::move(panel));
            
            tabbedComponent.addTab("LFO " + juce::String(i), 
                                  juce::Colour(0xff1a1a1a), 
                                  lfoPanels.back().get(), 
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

private:
    juce::TabbedComponent tabbedComponent;
    std::vector<std::unique_ptr<juce::Component>> lfoPanels;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOPanel)
};
