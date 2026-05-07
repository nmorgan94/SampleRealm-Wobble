#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

// Forward declaration
class AudioPluginAudioProcessor;

class ModulatableSlider : public juce::Slider,
                          public juce::DragAndDropTarget
{
public:
    ModulatableSlider(AudioPluginAudioProcessor& proc, const juce::String& paramID)
        : processor(proc), parameterID(paramID)
    {
        setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        restoreAssignment();
    }
    
    const juce::String& getParameterID() const { return parameterID; }
    
    void setAssignedLFO(int lfoIndex)
    {
        assignedLFO = lfoIndex;
        repaint();
    }
    
    int getAssignedLFO() const { return assignedLFO; }
    
    // DragAndDropTarget overrides
    bool isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override
    {
        // Check if the drag source is an LFO tab
        return dragSourceDetails.description.toString().startsWith("LFO:");
    }
    
    void itemDropped(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override
    {
        int lfoIndex = dragSourceDetails.description.toString().substring(4).getIntValue();
        if (lfoIndex >= 0 && lfoIndex < 4)
            assignLFO(lfoIndex);
    }
    
    void itemDragEnter(const juce::DragAndDropTarget::SourceDetails&) override
    {
        isDragOver = true;
        repaint();
    }
    
    void itemDragExit(const juce::DragAndDropTarget::SourceDetails&) override
    {
        isDragOver = false;
        repaint();
    }
    
    void paint(juce::Graphics& g) override
    {
        juce::Slider::paint(g);
        
        if (assignedLFO >= 0 && assignedLFO < 4)
        {
            auto bounds = getLocalBounds().toFloat();
            auto indicatorSize = 8.0f;
            auto indicatorBounds = juce::Rectangle<float>(
                bounds.getRight() - indicatorSize - 2.0f,
                bounds.getY() + 2.0f,
                indicatorSize,
                indicatorSize
            );
            
            g.setColour(lfoColor);
            g.fillEllipse(indicatorBounds);
            
            g.setColour(juce::Colours::black);
            g.setFont(10.0f);
            g.drawText(juce::String(assignedLFO + 1), indicatorBounds, juce::Justification::centred);
        }
        
        if (isDragOver)
        {
            g.setColour(lfoColor.withAlpha(0.3f));
            g.drawEllipse(getLocalBounds().toFloat().reduced(1.0f), 2.0f);
        }
    }
    
    // Right-click to clear assignment
    void mouseDown(const juce::MouseEvent& event) override
    {
        if (event.mods.isRightButtonDown() && assignedLFO >= 0)
            clearAssignment();
        else
            juce::Slider::mouseDown(event);
    }

private:
    static inline const juce::Colour lfoColor = juce::Colour(0xff00ff41);  // Neon green for all LFOs
    
    AudioPluginAudioProcessor& processor;
    juce::String parameterID;
    int assignedLFO = -1;
    bool isDragOver = false;
    
    void assignLFO(int lfoIndex)
    {
        processor.getModulationManager().assignLFO(parameterID, lfoIndex);
        setAssignedLFO(lfoIndex);
        DBG("LFO " << (lfoIndex + 1) << " assigned to parameter: " << parameterID);
    }
    
    void clearAssignment()
    {
        processor.getModulationManager().clearAssignment(parameterID);
        setAssignedLFO(-1);
        DBG("LFO assignment cleared from parameter: " << parameterID);
    }
    
    void restoreAssignment()
    {
        auto assignment = processor.getModulationManager().getAssignment(parameterID);
        if (assignment.isAssigned())
            setAssignedLFO(assignment.lfoIndex);
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatableSlider)
};
