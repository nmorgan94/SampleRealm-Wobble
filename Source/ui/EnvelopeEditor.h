#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "BaseCurveDisplay.h"

class EnvelopeEditor : public BaseCurveDisplay
{
public:
    EnvelopeEditor() = default;
    
    void setADSRValues(double attackTime, double decayTime, double sustainLevel, double releaseTime)
    {
        attack = juce::jmax(0.001f, float(attackTime));
        decay = juce::jmax(0.001f, float(decayTime));
        sustain = juce::jlimit(0.0f, 1.0f, float(sustainLevel));
        release = juce::jmax(0.001f, float(releaseTime));
        repaint();
    }

protected:
    void generateCurvePath(juce::Path& path, const juce::Rectangle<float>& bounds) const override
    {
        float totalTime = attack + decay + 0.3f + release; // 0.3f = sustain display time
        
        // Calculate X positions for each stage
        float attackX = (attack / totalTime) * bounds.getWidth();
        float decayX = attackX + (decay / totalTime) * bounds.getWidth();
        float sustainX = decayX + (0.3f / totalTime) * bounds.getWidth();
        float releaseX = bounds.getWidth();
        
        // Calculate Y positions
        float topY = bounds.getY();
        float bottomY = bounds.getBottom();
        float sustainY = bottomY - (sustain * bounds.getHeight());
        
        path.startNewSubPath(bounds.getX(), bottomY); // Start at bottom left
        
        path.lineTo(bounds.getX() + attackX, topY);
        
        path.lineTo(bounds.getX() + decayX, sustainY);
        
        path.lineTo(bounds.getX() + sustainX, sustainY);
        
        path.lineTo(bounds.getX() + releaseX, bottomY);
    }
    
    void drawControlPoints(juce::Graphics& g, const juce::Rectangle<float>& bounds) const override
    {
        float totalTime = attack + decay + 0.3f + release;
        
        float attackX = (attack / totalTime) * bounds.getWidth();
        float decayX = attackX + (decay / totalTime) * bounds.getWidth();
        float sustainX = decayX + (0.3f / totalTime) * bounds.getWidth();
        
        float topY = bounds.getY();
        float bottomY = bounds.getBottom();
        float sustainY = bottomY - (sustain * bounds.getHeight());
        
        // Draw points at key positions
        drawPoint(g, bounds.getX() + attackX, topY);
        drawPoint(g, bounds.getX() + decayX, sustainY);
        drawPoint(g, bounds.getX() + sustainX, sustainY);
    }
    
    void drawLabels(juce::Graphics& g, const juce::Rectangle<float>& bounds) const override
    {
        g.setColour(juce::Colour(0xff00ff41).withAlpha(0.6f));
        g.setFont(10.0f);
        
        float totalTime = attack + decay + 0.3f + release;
        
        float attackX = (attack / totalTime) * bounds.getWidth();
        float decayX = attackX + (decay / totalTime) * bounds.getWidth();
        float sustainX = decayX + (0.3f / totalTime) * bounds.getWidth();
        
        // Draw stage labels
        auto drawLabel = [&](const juce::String& text, float x1, float x2)
        {
            float centerX = bounds.getX() + (x1 + x2) / 2.0f;
            float labelY = bounds.getBottom() - 15.0f;
            juce::Rectangle<float> labelBounds(centerX - 20.0f, labelY, 40.0f, 12.0f);
            g.drawText(text, labelBounds, juce::Justification::centred);
        };
        
        drawLabel("A", 0, attackX);
        drawLabel("D", attackX, decayX);
        drawLabel("S", decayX, sustainX);
        drawLabel("R", sustainX, bounds.getWidth());
    }

private:
    float attack = 0.01f;
    float decay = 0.1f;
    float sustain = 0.7f;
    float release = 0.3f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeEditor)
};
