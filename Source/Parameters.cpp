#include "Parameters.h"

namespace Parameters
{
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        // Oscillator 1
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            "osc1_enable", "Osc 1 Enable", true));
        
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            "osc1_waveform", "Osc 1 Waveform",
            juce::StringArray { "Sine", "Saw", "Square", "Triangle", "Pulse 25%", "Pulse 10%" }, 0));
        
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "osc1_gain", "Osc 1 Gain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
        
        // Oscillator 2
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            "osc2_enable", "Osc 2 Enable", false));
        
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            "osc2_waveform", "Osc 2 Waveform",
            juce::StringArray { "Sine", "Saw", "Square", "Triangle", "Pulse 25%", "Pulse 10%" }, 1));
        
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "osc2_gain", "Osc 2 Gain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
        
        // Oscillator 3
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            "osc3_enable", "Osc 3 Enable", false));
        
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            "osc3_waveform", "Osc 3 Waveform",
            juce::StringArray { "Sine", "Saw", "Square", "Triangle", "Pulse 25%", "Pulse 10%" }, 2));
        
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "osc3_gain", "Osc 3 Gain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
        
        return { params.begin(), params.end() };
    }
}
