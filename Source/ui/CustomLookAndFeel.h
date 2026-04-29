#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:

    CustomLookAndFeel() {}

    [[nodiscard]] static juce::FontOptions orbitronRegular()
    {
        static auto typeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::OrbitronRegular_ttf,
            BinaryData::OrbitronRegular_ttfSize);
        return juce::FontOptions(typeface);
    }
    
    [[nodiscard]] static juce::FontOptions orbitronBold()
    {
        static auto typeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::OrbitronBold_ttf,
            BinaryData::OrbitronBold_ttfSize);
        return juce::FontOptions(typeface);
    }
    
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                         bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
        auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
        
        auto circleBounds = bounds.withSizeKeepingCentre(size, size);
        
        auto neonGreen = juce::Colour(0xff00ff41);         
        auto darkGreen = juce::Colour(0xff00aa2e);        
        auto greyBorder = juce::Colour(0xff2a2a2a);       
        
        // Draw outer circle (border)
        g.setColour(button.getToggleState() ? neonGreen : greyBorder);
        g.drawEllipse(circleBounds, 2.0f);
        
        // Draw inner fill if enabled
        if (button.getToggleState())
        {
            // Dark green glow fill
            auto innerBounds = circleBounds.reduced(3.0f);
            g.setColour(darkGreen.withAlpha(0.3f));
            g.fillEllipse(innerBounds);
            
            // Draw center dot with glow effect
            auto dotBounds = circleBounds.reduced(size * 0.35f);
            
            // Outer glow
            g.setColour(neonGreen.withAlpha(0.4f));
            g.fillEllipse(dotBounds.expanded(2.0f));
            
            // Inner bright dot
            g.setColour(neonGreen);
            g.fillEllipse(dotBounds);
        }
    }

};
