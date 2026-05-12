#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <map>

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
        int sourceIndex = -1;     // Which LFO or Envelope (0-3, or -1 for none)
        float depth = 1.0f;       // Modulation depth (0.0 to 1.0)
        
        ModulationAssignment() = default;
        ModulationAssignment(ModulationSource type, int index, float d = 1.0f)
            : sourceType(type), sourceIndex(index), depth(d) {}
        
        bool isAssigned() const { return sourceType != ModulationSource::None && sourceIndex >= 0 && sourceIndex < 4; }
        bool isLFO() const { return sourceType == ModulationSource::LFO; }
        bool isEnvelope() const { return sourceType == ModulationSource::Envelope; }
    };
    
    ModulationManager() = default;
    
    // Assign an LFO to a parameter
    void assignLFO(const juce::String& parameterID, int lfoIndex, float depth = 1.0f)
    {
        if (lfoIndex >= 0 && lfoIndex < 4)
            assignments[parameterID] = ModulationAssignment(ModulationSource::LFO, lfoIndex, depth);
    }
    
    void assignEnvelope(const juce::String& parameterID, int envIndex, float depth = 1.0f)
    {
        if (envIndex >= 0 && envIndex < 4)
            assignments[parameterID] = ModulationAssignment(ModulationSource::Envelope, envIndex, depth);
    }
    
    // Remove assignment from a parameter
    void clearAssignment(const juce::String& parameterID)
    {
        assignments.erase(parameterID);
    }
    
    // Get the assignment for a parameter
    ModulationAssignment getAssignment(const juce::String& parameterID) const
    {
        auto it = assignments.find(parameterID);
        if (it != assignments.end())
            return it->second;
        return ModulationAssignment();
    }
    
    float calculateModulatedValue(float baseValue, float lfoValue, float depth) const
    {
        // Bipolar modulation: LFO 0.5 = no change, 0.0 = -depth, 1.0 = +depth
        float modAmount = (lfoValue - 0.5f) * 2.0f * depth;
        return juce::jlimit(0.0f, 1.0f, baseValue + modAmount);
    }
    
    bool isLFOAssigned(int lfoIndex) const
    {
        for (const auto& [paramID, assignment] : assignments)
        {
            if (assignment.isLFO() && assignment.sourceIndex == lfoIndex)
                return true;
        }
        return false;
    }
    
    bool isEnvelopeAssigned(int envIndex) const
    {
        for (const auto& [paramID, assignment] : assignments)
        {
            if (assignment.isEnvelope() && assignment.sourceIndex == envIndex)
                return true;
        }
        return false;
    }

private:
    std::map<juce::String, ModulationAssignment> assignments;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationManager)
};
