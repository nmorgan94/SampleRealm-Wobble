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
    
    void drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                     int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                     juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
        
        // Background
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Border
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
        
        // Arrow on right side
        auto arrowZone = bounds.removeFromRight(20.0f).reduced(5.0f);
        juce::Path arrow;
        arrow.addTriangle(arrowZone.getX(), arrowZone.getY(),
                         arrowZone.getRight(), arrowZone.getY(),
                         arrowZone.getCentreX(), arrowZone.getBottom());
        
        g.setColour(box.isEnabled() ? juce::Colour(0xff00ff41) : juce::Colour(0xff666666));
        g.fillPath(arrow);
    }
    
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
    {
        g.fillAll(juce::Colour(0xff2a2a2a));
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRect(0, 0, width, height, 1);
    }
    
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                          bool /*isSeparator*/, bool isActive, bool isHighlighted, bool /*isTicked*/,
                          bool /*hasSubMenu*/, const juce::String& text,
                          const juce::String& /*shortcutKeyText*/,
                          const juce::Drawable* /*icon*/, const juce::Colour* /*textColour*/) override
    {
        auto textColour = juce::Colours::white;
        
        if (isHighlighted && isActive)
        {
            g.setColour(juce::Colour(0xff00ff41).withAlpha(0.2f));
            g.fillRect(area);
            textColour = juce::Colour(0xff00ff41);
        }
        
        g.setColour(textColour);
        g.setFont(orbitronRegular().withHeight(14.0f));
        
        auto r = area.reduced(10, 0);
        g.drawFittedText(text, r, juce::Justification::centredLeft, 1);
    }
    
    juce::Font getComboBoxFont(juce::ComboBox&) override
    {
        return orbitronRegular().withHeight(14.0f);
    }
    
    juce::Label* createComboBoxTextBox(juce::ComboBox&) override
    {
        auto* label = new juce::Label();
        label->setJustificationType(juce::Justification::centredLeft);
        label->setColour(juce::Label::textColourId, juce::Colours::white);
        label->setColour(juce::Label::textWhenEditingColourId, juce::Colours::white);
        label->setFont(orbitronRegular().withHeight(14.0f));
        return label;
    }
    
    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g, bool /*isMouseOver*/, bool /*isMouseDown*/) override
    {
        auto bounds = button.getActiveArea().toFloat();
        auto neonGreen = juce::Colour(0xff00ff41);
        
        // Background
        if (button.getToggleState())
        {
            g.setColour(juce::Colour(0xff2a2a2a));
            g.fillRect(bounds);
        }
        else
        {
            g.setColour(juce::Colour(0xff1a1a1a));
            g.fillRect(bounds);
        }
        
        // Bottom indicator for active tab
        if (button.getToggleState())
        {
            g.setColour(neonGreen);
            g.fillRect(bounds.removeFromBottom(3.0f));
        }
        
        // Text
        g.setColour(button.getToggleState() ? neonGreen : juce::Colour(0xff666666));
        g.setFont(orbitronBold().withHeight(14.0f));
        g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
    }
    
    int getTabButtonBestWidth(juce::TabBarButton& button, int tabDepth) override
    {
        juce::ignoreUnused(tabDepth);

        auto* tabBar = button.findParentComponentOfClass<juce::TabbedButtonBar>();
        return tabBar->getWidth() / tabBar->getNumTabs();
    }
    
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPosProportional, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/,
                         juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centerX = bounds.getCentreX();
        auto centerY = bounds.getCentreY();
        
        auto rotaryStartAngle = juce::MathConstants<float>::pi * 1.25f; 
        auto rotaryEndAngle = juce::MathConstants<float>::pi * 2.75f;   
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        
        auto neonGreen = juce::Colour(0xff00ff41);
        auto trackColor = juce::Colour(0xff3a3a3a);
        
        // Draw background circle
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillEllipse(bounds.reduced(2.0f));
        
        // Draw track
        auto arcRadius = radius - 5.0f;
        auto lineThickness = 3.0f;
        
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centerX, centerY, arcRadius, arcRadius,
                                    0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(trackColor);
        g.strokePath(backgroundArc, juce::PathStrokeType(lineThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        // Draw value arc
        if (sliderPosProportional > 0.0f)
        {
            juce::Path valueArc;
            valueArc.addCentredArc(centerX, centerY, arcRadius, arcRadius,
                                   0.0f, rotaryStartAngle, angle, true);
            g.setColour(slider.isEnabled() ? neonGreen : juce::Colour(0xff666666));
            g.strokePath(valueArc, juce::PathStrokeType(lineThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        
        // Draw pointer notch
        auto notchLength = radius * 0.3f;
        auto notchWidth = 2.5f;
        auto notchDistance = radius * 0.55f;
        
        juce::Path notch;
        notch.addRoundedRectangle(-notchWidth * 0.5f, -notchDistance,
                                  notchWidth, notchLength, 1.0f);
        notch.applyTransform(juce::AffineTransform::rotation(angle).translated(centerX, centerY));
        
        g.setColour(slider.isEnabled() ? juce::Colours::white : juce::Colour(0xff888888));
        g.fillPath(notch);
    }

};
