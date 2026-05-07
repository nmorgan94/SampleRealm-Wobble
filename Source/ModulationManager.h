#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <map>

class ModulationManager
{
public:
    struct ModulationAssignment
    {
        int lfoIndex = -1;        // Which LFO (0-3, or -1 for none)
        float depth = 1.0f;       // Modulation depth (0.0 to 1.0)
        
        ModulationAssignment() = default;
        ModulationAssignment(int lfo, float d = 1.0f) : lfoIndex(lfo), depth(d) {}
        
        bool isAssigned() const { return lfoIndex >= 0 && lfoIndex < 4; }
    };
    
    ModulationManager() = default;
    
    // Assign an LFO to a parameter
    void assignLFO(const juce::String& parameterID, int lfoIndex, float depth = 1.0f)
    {
        if (lfoIndex >= 0 && lfoIndex < 4)
            assignments[parameterID] = ModulationAssignment(lfoIndex, depth);
    }
    
    // Remove LFO assignment from a parameter
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

private:
    std::map<juce::String, ModulationAssignment> assignments;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationManager)
};
