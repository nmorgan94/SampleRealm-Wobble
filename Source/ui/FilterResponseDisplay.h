#pragma once

#include "BaseCurveDisplay.h"
#include "../synth/Filter.h"
#include <cmath>

class FilterResponseDisplay : public BaseCurveDisplay
{
public:
    FilterResponseDisplay() : BaseCurveDisplay(false) {}
    
    void setFilterParameters(FilterMode mode, float cutoffHz, float resonance, double sr)
    {
        filterMode = mode;
        cutoffFrequency = cutoffHz;
        qFactor = 0.5f + resonance * 19.5f;
        sampleRate = sr;
        repaint();
    }
    
    void setEnabled(bool shouldBeEnabled)
    {
        isEnabled = shouldBeEnabled;
        repaint();
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        g.setColour(juce::Colour(0xff0a0a0a));
        g.fillRoundedRectangle(bounds, 4.0f);
        
        juce::Path curvePath;
        generateCurvePath(curvePath, bounds);
        
        juce::Path filledPath = curvePath;
        filledPath.lineTo(bounds.getRight(), bounds.getBottom());
        filledPath.lineTo(bounds.getX(), bounds.getBottom());
        filledPath.closeSubPath();
        
        auto baseColor = isEnabled ? juce::Colour(0xff00ff41) : juce::Colour(0xff666666);
        
        g.setColour(baseColor.withAlpha(0.2f));
        g.fillPath(filledPath);
        
        g.setColour(baseColor);
        g.strokePath(curvePath, juce::PathStrokeType(2.0f));
        
        drawLabels(g, bounds);
    }

protected:
    void generateCurvePath(juce::Path& path, const juce::Rectangle<float>& bounds) const override
    {
        if (sampleRate <= 0.0f)
            return;
        
        constexpr int numPoints = 200;
        
        for (int i = 0; i < numPoints; ++i)
        {
            float t = i / float(numPoints - 1);
            float freq = 20.0f * std::pow(1000.0f, t);
            float magnitude = calculateMagnitudeResponse(freq);
            float magnitudeDB = 20.0f * std::log10(std::max(magnitude, 0.00001f));
            float normalizedMag = juce::jlimit(0.0f, 1.0f, juce::jmap(magnitudeDB, -60.0f, 20.0f, 1.0f, 0.0f));
            
            float x = bounds.getX() + t * bounds.getWidth();
            float y = bounds.getY() + normalizedMag * bounds.getHeight();
            
            i == 0 ? path.startNewSubPath(x, y) : path.lineTo(x, y);
        }
    }
    
    void drawLabels(juce::Graphics& g, const juce::Rectangle<float>& bounds) const override
    {
        auto labelColor = isEnabled ? juce::Colour(0xff666666) : juce::Colour(0xff3a3a3a);
        g.setColour(labelColor);
        g.setFont(10.0f);
        
        const float labelY = bounds.getBottom() - 12.0f;
        g.drawText("20", juce::Rectangle<float>(bounds.getX(), labelY, 30.0f, 12.0f), juce::Justification::left, false);
        g.drawText("1k", juce::Rectangle<float>(bounds.getCentreX() - 10.0f, labelY, 20.0f, 12.0f), juce::Justification::centred, false);
        g.drawText("20k", juce::Rectangle<float>(bounds.getRight() - 30.0f, labelY, 30.0f, 12.0f), juce::Justification::right, false);
    }

private:
    FilterMode filterMode = FilterMode::Lowpass;
    float cutoffFrequency = 1000.0f;
    float qFactor = 0.707f;
    double sampleRate = 44100.0;
    bool isEnabled = true;
    
    float calculateMagnitudeResponse(float frequency) const
    {
        if (frequency <= 0.0f || cutoffFrequency <= 0.0f)
            return 1.0f;
        
        float ratio = frequency / cutoffFrequency;
        float ratio2 = ratio * ratio;
        
        // Common denominator for all filter types
        float term1 = 1.0f - ratio2;
        float term2 = ratio / qFactor;
        float denom = juce::jmax(std::sqrt(term1 * term1 + term2 * term2), 0.001f);
        
        switch (filterMode)
        {
            case FilterMode::Lowpass:   return 1.0f / denom;
            case FilterMode::Highpass:  return ratio2 / denom;
            case FilterMode::Bandpass:  return (ratio / qFactor) / denom;
            case FilterMode::Notch:     return std::abs(term1) / denom;
            default:                    return 1.0f;
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterResponseDisplay)
};
