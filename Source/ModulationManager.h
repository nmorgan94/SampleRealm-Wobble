#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class ModulationManager
{
public:
    enum class ModulationSource
    {
        None = 0,
        LFO,
        Envelope
    };
    
    struct ModulationAssignment
    {
        ModulationSource sourceType = ModulationSource::None;
        int sourceIndex = -1;
        float minValue = 0.0f;
        float maxValue = 1.0f;
        
        bool isAssigned() const { return sourceType != ModulationSource::None && sourceIndex >= 0 && sourceIndex < 4; }
        bool isLFO() const { return sourceType == ModulationSource::LFO; }
        bool isEnvelope() const { return sourceType == ModulationSource::Envelope; }
    };
    
    ModulationManager() = default;
    
    void initialise(juce::ValueTree treeState, juce::UndoManager* treeUndoManager)
    {
        state = treeState;
        undoManager = treeUndoManager;
    }
    
    void assignLFO(const juce::String& parameterID, int lfoIndex, float minValue = 0.0f, float maxValue = 1.0f)
    {
        if (isValidSourceIndex(lfoIndex))
            setAssignment(parameterID, ModulationSource::LFO, lfoIndex, minValue, maxValue);
    }
    
    void assignEnvelope(const juce::String& parameterID, int envIndex, float minValue = 0.0f, float maxValue = 1.0f)
    {
        if (isValidSourceIndex(envIndex))
            setAssignment(parameterID, ModulationSource::Envelope, envIndex, minValue, maxValue);
    }
    
    void clearAssignment(const juce::String& parameterID)
    {
        auto assignmentNode = findAssignment(parameterID);
        if (assignmentNode.isValid())
            state.removeChild(assignmentNode, undoManager);
    }
    
    ModulationAssignment getAssignment(const juce::String& parameterID) const
    {
        return toAssignment(findAssignment(parameterID));
    }
    
    float calculateModulatedValue(float sourceValue, float minValue, float maxValue) const
    {
        return juce::jlimit(0.0f, 1.0f, minValue + sourceValue * (maxValue - minValue));
    }
    
    bool isLFOAssigned(int lfoIndex) const
    {
        return isSourceAssigned(ModulationSource::LFO, lfoIndex);
    }
    
    bool isEnvelopeAssigned(int envIndex) const
    {
        return isSourceAssigned(ModulationSource::Envelope, envIndex);
    }

private:
    struct IDs
    {
        static inline const juce::Identifier assignment { "Assignment" };
        static inline const juce::Identifier parameterID { "parameterID" };
        static inline const juce::Identifier sourceType { "sourceType" };
        static inline const juce::Identifier sourceIndex { "sourceIndex" };
        static inline const juce::Identifier minValue { "minValue" };
        static inline const juce::Identifier maxValue { "maxValue" };
    };
    
    static bool isValidSourceIndex(int index)
    {
        return index >= 0 && index < 4;
    }
    
    ModulationAssignment toAssignment(juce::ValueTree assignmentNode) const
    {
        if (! assignmentNode.isValid())
            return {};
        
        return {
            static_cast<ModulationSource>(static_cast<int>(assignmentNode.getProperty(IDs::sourceType, 0))),
            static_cast<int>(assignmentNode.getProperty(IDs::sourceIndex, -1)),
            static_cast<float>(assignmentNode.getProperty(IDs::minValue, 0.0f)),
            static_cast<float>(assignmentNode.getProperty(IDs::maxValue, 1.0f))
        };
    }
    
    juce::ValueTree findAssignment(const juce::String& parameterID) const
    {
        if (! state.isValid())
            return {};
        
        for (auto child : state)
        {
            if (child.hasType(IDs::assignment)
                && child[IDs::parameterID].toString() == parameterID)
                return child;
        }
        
        return {};
    }
    
    bool isSourceAssigned(ModulationSource sourceType, int sourceIndex) const
    {
        if (! state.isValid())
            return false;
        
        for (auto child : state)
        {
            if (child.hasType(IDs::assignment)
                && static_cast<int>(child.getProperty(IDs::sourceType, 0)) == static_cast<int>(sourceType)
                && static_cast<int>(child.getProperty(IDs::sourceIndex, -1)) == sourceIndex)
                return true;
        }
        
        return false;
    }
    
    void setAssignment(const juce::String& parameterID, ModulationSource sourceType, int sourceIndex, float minValue, float maxValue)
    {
        if (! state.isValid())
            return;
        
        auto assignmentNode = findAssignment(parameterID);
        if (! assignmentNode.isValid())
        {
            assignmentNode = juce::ValueTree(IDs::assignment);
            assignmentNode.setProperty(IDs::parameterID, parameterID, undoManager);
            state.addChild(assignmentNode, -1, undoManager);
        }
        
        assignmentNode.setProperty(IDs::sourceType, static_cast<int>(sourceType), undoManager);
        assignmentNode.setProperty(IDs::sourceIndex, sourceIndex, undoManager);
        assignmentNode.setProperty(IDs::minValue, minValue, undoManager);
        assignmentNode.setProperty(IDs::maxValue, maxValue, undoManager);
    }
    
    juce::ValueTree state;
    juce::UndoManager* undoManager = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationManager)
};
