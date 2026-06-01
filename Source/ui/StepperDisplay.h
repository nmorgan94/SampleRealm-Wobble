#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

// A compact drag-to-edit integer display with a label.
// Drag vertically to change the value; double-click to reset.
class StepperDisplay : public juce::Component
{
public:
    StepperDisplay() = default;

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // Background
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

        // Border
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);

        auto labelBounds = bounds.removeFromLeft(bounds.getWidth() / 2);

        g.setColour(juce::Colours::grey);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(labelText, labelBounds, juce::Justification::centred);

        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        juce::String valueText = (showSign && value >= 0 ? "+" : "") + juce::String(value);
        g.drawText(valueText, bounds, juce::Justification::centred);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        dragStartY = e.y;
        dragStartValue = value;
    }

    void mouseDoubleClick(const juce::MouseEvent&) override
    {
        setValue(juce::jlimit(minValue, maxValue, 0));
        if (onValueChange)
            onValueChange(value);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        int deltaY = dragStartY - e.y;
        int newValue = juce::jlimit(minValue, maxValue, dragStartValue + (deltaY / 3));

        if (newValue != value)
        {
            value = newValue;
            repaint();

            if (onValueChange)
                onValueChange(value);
        }
    }

    void setValue(int newValue)
    {
        value = juce::jlimit(minValue, maxValue, newValue);
        repaint();
    }

    int getValue() const { return value; }

    void setRange(int newMin, int newMax)
    {
        minValue = newMin;
        maxValue = newMax;
        setValue(value);
    }

    void setLabel(const juce::String& newLabel)
    {
        labelText = newLabel;
        repaint();
    }

    void setShowSign(bool shouldShowSign)
    {
        showSign = shouldShowSign;
        repaint();
    }

    // Two-way binding to an integer APVTS parameter: initialises from the current
    // value and writes changes back to the host.
    void attachToParameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& parameterID)
    {
        if (auto* param = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter(parameterID)))
            setValue(param->get());

        onValueChange = [&apvts, parameterID](int newValue)
        {
            if (auto* param = apvts.getParameter(parameterID))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(newValue)));
        };
    }

    std::function<void(int)> onValueChange;

private:
    int value = 0;
    int minValue = -24;
    int maxValue = 24;
    bool showSign = true;
    juce::String labelText { "SEM" };
    int dragStartY = 0;
    int dragStartValue = 0;
};
