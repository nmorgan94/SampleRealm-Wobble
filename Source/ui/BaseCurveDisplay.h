#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class BaseCurveDisplay : public juce::Component
{
public:
    BaseCurveDisplay(bool showGrid = true) : shouldShowGrid(showGrid)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Draw background
        g.setColour(juce::Colour(0xff0a0a0a));
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Draw grid lines
        if (shouldShowGrid)
            drawGrid(g, bounds);
        
        // Generate curve path
        juce::Path curvePath;
        generateCurvePath(curvePath, bounds);
        
        // Draw filled area under curve
        juce::Path filledPath = curvePath;
        filledPath.lineTo(bounds.getRight(), bounds.getBottom());
        filledPath.lineTo(bounds.getX(), bounds.getBottom());
        filledPath.closeSubPath();
        
        g.setColour(juce::Colour(0xff00ff41).withAlpha(0.2f));
        g.fillPath(filledPath);
        
        // Draw curve line
        g.setColour(juce::Colour(0xff00ff41));
        g.strokePath(curvePath, juce::PathStrokeType(2.0f));
        
        drawControlPoints(g, bounds);
        
        drawLabels(g, bounds);
    }

protected:
    virtual void generateCurvePath(juce::Path& path, const juce::Rectangle<float>& bounds) const = 0;
    virtual void drawControlPoints(juce::Graphics&, const juce::Rectangle<float>&) const {}
    virtual void drawLabels(juce::Graphics&, const juce::Rectangle<float>&) const {}
    
    void drawGrid(juce::Graphics& g, const juce::Rectangle<float>& bounds) const
    {
        g.setColour(juce::Colour(0xff2a2a2a));
        const int numVerticalLines = 8;
        const int numHorizontalLines = 4;
        
        for (int i = 1; i < numVerticalLines; ++i)
        {
            float x = bounds.getX() + (bounds.getWidth() * i / numVerticalLines);
            g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 1.0f);
        }
        
        for (int i = 1; i < numHorizontalLines; ++i)
        {
            float y = bounds.getY() + (bounds.getHeight() * i / numHorizontalLines);
            g.drawLine(bounds.getX(), y, bounds.getRight(), y, 1.0f);
        }
        
        // Draw center line for LFO-style displays
        if (shouldDrawCenterLine())
        {
            g.setColour(juce::Colour(0xff3a3a3a));
            float centerY = bounds.getCentreY();
            g.drawLine(bounds.getX(), centerY, bounds.getRight(), centerY, 1.5f);
        }
    }
    
    virtual bool shouldDrawCenterLine() const { return false; }
    
    void drawPoint(juce::Graphics& g, float x, float y, bool isHighlighted = false) const
    {
        // Outer circle
        g.setColour(juce::Colour(0xff00ff41));
        g.fillEllipse(x - 6, y - 6, 12, 12);
        
        // Inner circle
        if (isHighlighted)
            g.setColour(juce::Colours::white);
        else
            g.setColour(juce::Colour(0xff0a0a0a));
            
        g.fillEllipse(x - 4, y - 4, 8, 8);
    }

private:
    bool shouldShowGrid = true;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaseCurveDisplay)
};
