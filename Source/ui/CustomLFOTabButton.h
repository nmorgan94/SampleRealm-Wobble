#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class CustomLFOTabButton : public juce::TabBarButton
{
public:
    enum class SourceType
    {
        LFO,
        Envelope
    };
    
    CustomLFOTabButton(const juce::String& name, juce::TabbedButtonBar& bar, int index, SourceType type = SourceType::LFO)
        : juce::TabBarButton(name, bar), sourceIndex(index), sourceType(type)
    {
    }
    
    void paint(juce::Graphics& g) override
    {
        juce::TabBarButton::paint(g);
        
        if (getToggleState())
        {
            auto bounds = getLocalBounds().toFloat();
            auto handleArea = bounds.removeFromRight(18);
            drawCross(g, handleArea.getCentreX(), handleArea.getCentreY());
        }
    }
    
    void mouseDown(const juce::MouseEvent& event) override
    {
        if (isClickOnHandle(event.getPosition()))
            isDraggingHandle = true;
        else
            juce::TabBarButton::mouseDown(event);
    }
    
    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (isDraggingHandle && event.getDistanceFromDragStart() > 5)
        {
            auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this);
            if (container != nullptr)
            {
                juce::String prefix = (sourceType == SourceType::LFO) ? "LFO:" : "ENV:";
                juce::var dragData = prefix + juce::String(sourceIndex);
                container->startDragging(dragData, this, juce::ScaledImage(createCrossImage()), true);
                isDraggingHandle = false;
            }
        }
        else if (!isDraggingHandle)
        {
            juce::TabBarButton::mouseDrag(event);
        }
    }
    
    void mouseUp(const juce::MouseEvent& event) override
    {
        if (!isDraggingHandle)
        {
            juce::TabBarButton::mouseUp(event);
        }
        isDraggingHandle = false;
    }

private:
    static void drawCross(juce::Graphics& g, float centerX, float centerY)
    {
        const float crossSize = 8.0f;
        const float thickness = 2.0f;
        
        g.setColour(juce::Colour(0xff00ff41));
        
        // Vertical line
        g.fillRect(centerX - thickness * 0.5f, centerY - crossSize * 0.5f, thickness, crossSize);
        
        // Horizontal line
        g.fillRect(centerX - crossSize * 0.5f, centerY - thickness * 0.5f, crossSize, thickness);
    }
    
    static juce::Image createCrossImage()
    {
        const int imageSize = 16;
        juce::Image dragImage(juce::Image::ARGB, imageSize, imageSize, true);
        juce::Graphics g(dragImage);
        drawCross(g, imageSize * 0.5f, imageSize * 0.5f);
        return dragImage;
    }
    
    bool isClickOnHandle(juce::Point<int> position) const
    {
        return getLocalBounds().removeFromRight(18).contains(position);
    }
    
    int sourceIndex;
    SourceType sourceType;
    bool isDraggingHandle = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomLFOTabButton)
};
