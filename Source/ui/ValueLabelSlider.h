#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class ValueLabelSlider : public juce::Slider
{
public:
    using juce::Slider::Slider;

    void attachLabel(juce::Label& label)
    {
        valueLabel = &label;
        updatePopupState();
    }

protected:
    void parentHierarchyChanged() override { updatePopupState(); }

    void startedDragging() override
    {
        dragging = true;

        if (valueLabel != nullptr)
        {
            textBeforeDrag = valueLabel->getText();
            showValue();
        }
    }

    void stoppedDragging() override
    {
        dragging = false;

        if (valueLabel != nullptr)
            valueLabel->setText(textBeforeDrag, juce::dontSendNotification);
    }

    void valueChanged() override
    {
        juce::Slider::valueChanged();

        if (dragging && valueLabel != nullptr)
            showValue();
    }

private:
    void updatePopupState()
    {
        setPopupDisplayEnabled(valueLabel == nullptr, false, getTopLevelComponent());
    }

    void showValue()
    {
        valueLabel->setText(getTextFromValue(getValue()), juce::dontSendNotification);
    }

    juce::Component::SafePointer<juce::Label> valueLabel;
    juce::String textBeforeDrag;
    bool dragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ValueLabelSlider)
};
