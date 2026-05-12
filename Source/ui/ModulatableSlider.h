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
    
    void setAssignedSource(bool isLFO, int sourceIndex)
    {
        assignedIsLFO = isLFO;
        assignedSourceIndex = sourceIndex;
        repaint();
    }
    
    // DragAndDropTarget overrides
    bool isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override
    {
        auto desc = dragSourceDetails.description.toString();
        return desc.startsWith("LFO:") || desc.startsWith("ENV:");
    }
    
    void itemDropped(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override
    {
        auto desc = dragSourceDetails.description.toString();
        bool isLFO = desc.startsWith("LFO:");
        int index = desc.substring(4).getIntValue();
        
        if (index >= 0 && index < 4)
            assignModulator(isLFO, index);
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
        
        if (assignedSourceIndex >= 0 && assignedSourceIndex < 4)
        {
            auto indicator = getLocalBounds().removeFromTop(12).removeFromRight(16).translated(-2, 2).toFloat();
            
            g.setColour(juce::Colour(0xff2a2a2a));
            g.fillRoundedRectangle(indicator, 2.0f);
            
            g.setColour(modulationColor);
            g.setFont(9.0f);
            g.drawText((assignedIsLFO ? "L" : "E") + juce::String(assignedSourceIndex + 1),
                      indicator, juce::Justification::centred);
        }
        
        if (isDragOver)
        {
            g.setColour(modulationColor.withAlpha(0.3f));
            g.drawEllipse(getLocalBounds().toFloat().reduced(1.0f), 2.0f);
        }
    }
    
    // Right-click to clear assignment
    void mouseDown(const juce::MouseEvent& event) override
    {
        if (event.mods.isRightButtonDown() && assignedSourceIndex >= 0)
            clearAssignment();
        else
            juce::Slider::mouseDown(event);
    }

private:
    static inline const juce::Colour modulationColor = juce::Colour(0xff00ff41);  // Neon green for all modulation
    
    AudioPluginAudioProcessor& processor;
    juce::String parameterID;
    int assignedSourceIndex = -1;
    bool assignedIsLFO = true;
    bool isDragOver = false;
    
    void assignModulator(bool isLFO, int index)
    {
        auto& manager = processor.getModulationManager();
        isLFO ? manager.assignLFO(parameterID, index) : manager.assignEnvelope(parameterID, index);
        setAssignedSource(isLFO, index);
        DBG((isLFO ? "LFO " : "Envelope ") << (index + 1) << " assigned to " << parameterID);
    }
    
    void clearAssignment()
    {
        processor.getModulationManager().clearAssignment(parameterID);
        setAssignedSource(true, -1);
        isDragOver = false;
        repaint();
        DBG("Modulation assignment cleared from parameter: " << parameterID);
    }
    
    void restoreAssignment()
    {
        auto assignment = processor.getModulationManager().getAssignment(parameterID);
        if (assignment.isAssigned())
        {
            setAssignedSource(assignment.isLFO(), assignment.sourceIndex);
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatableSlider)
};
