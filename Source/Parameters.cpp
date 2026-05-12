#include "Parameters.h"

namespace Parameters
{
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        constexpr int versionHint = 1;

        // Oscillator 1
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"osc1_enable", versionHint}, "Osc 1 Enable", true));
        
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"osc1_waveform", versionHint}, "Osc 1 Waveform",
            juce::StringArray { "Sine", "Saw", "Square", "Triangle", "Pulse 25%", "Pulse 10%", "Formant Sine" }, 0));
        
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osc1_gain", versionHint}, "Osc 1 Gain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
        
        // Oscillator 2
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"osc2_enable", versionHint}, "Osc 2 Enable", false));
        
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"osc2_waveform", versionHint}, "Osc 2 Waveform",
            juce::StringArray { "Sine", "Saw", "Square", "Triangle", "Pulse 25%", "Pulse 10%", "Formant Sine" }, 1));
        
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osc2_gain", versionHint}, "Osc 2 Gain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
        
        // Oscillator 3
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"osc3_enable", versionHint}, "Osc 3 Enable", false));
        
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"osc3_waveform", versionHint}, "Osc 3 Waveform",
            juce::StringArray { "Sine", "Saw", "Square", "Triangle", "Pulse 25%", "Pulse 10%", "Formant Sine" }, 2));
        
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osc3_gain", versionHint}, "Osc 3 Gain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
        
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"lfo1_mode", versionHint}, "LFO 1 Mode",
            juce::StringArray { "Trigger", "Sync" }, 0));
        
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"lfo1_rate", versionHint}, "LFO 1 Rate",
            juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.3f), 1.0f));
        
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"lfo2_mode", versionHint}, "LFO 2 Mode",
            juce::StringArray { "Trigger", "Sync" }, 0));
        
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"lfo2_rate", versionHint}, "LFO 2 Rate",
            juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.3f), 2.0f));
        
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"lfo3_mode", versionHint}, "LFO 3 Mode",
            juce::StringArray { "Trigger", "Sync" }, 0));
        
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"lfo3_rate", versionHint}, "LFO 3 Rate",
            juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.3f), 4.0f));
        
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"lfo4_mode", versionHint}, "LFO 4 Mode",
            juce::StringArray { "Trigger", "Sync" }, 0));
        
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"lfo4_rate", versionHint}, "LFO 4 Rate",
            juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.3f), 8.0f));
        
        // Filter parameters
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"filter_enable", versionHint}, "Filter Enable", true));
        
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"filter_mode", versionHint}, "Filter Mode",
            juce::StringArray { "Lowpass", "Highpass", "Bandpass", "Notch" }, 0));
        
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"filter_cutoff", versionHint}, "Filter Cutoff",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 1000.0f));
        
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"filter_resonance", versionHint}, "Filter Resonance",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
        
        for (int i = 1; i <= 4; ++i)
        {
            juce::String envPrefix = "env" + juce::String(i);
            juce::String envName = "Env " + juce::String(i);
            
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{envPrefix + "_attack", versionHint}, envName + " Attack",
                juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));
            
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{envPrefix + "_decay", versionHint}, envName + " Decay",
                juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.1f));
            
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{envPrefix + "_sustain", versionHint}, envName + " Sustain",
                juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
            
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{envPrefix + "_release", versionHint}, envName + " Release",
                juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.3f));
        }
        
        return { params.begin(), params.end() };
    }
}
