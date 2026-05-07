#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

class LFO
{
public:
    LFO() = default;
    
    void prepare(double sr)
    {
        sampleRate = sr;
        phase = 0.0f;
    }
    
    void setRate(float rateHz)
    {
        rate = juce::jlimit(0.01f, 100.0f, rateHz);
    }
    
    // Sync the lookup table from curve editor (call from message thread)
    void syncFromCurve(const std::function<float(float)>& curveFunction)
    {
        for (size_t i = 0; i < lookupTableSize; ++i)
        {
            float x = i / static_cast<float>(lookupTableSize - 1);
            lookupTable[i] = curveFunction(x);
        }
    }
    
    // Get current LFO value (0.0 to 1.0) - audio thread safe
    float getCurrentValue() const
    {
        // Linear interpolation in lookup table
        float index = phase * (lookupTableSize - 1);
        size_t i0 = static_cast<size_t>(index);
        size_t i1 = juce::jmin(i0 + 1, static_cast<size_t>(lookupTableSize - 1));
        float frac = index - i0;
        
        return lookupTable[i0] + frac * (lookupTable[i1] - lookupTable[i0]);
    }
    
    // Advance the LFO by the specified number of samples
    void advance(int numSamples)
    {
        if (sampleRate <= 0.0 || numSamples <= 0)
            return;
        
        float phaseIncrement = rate / static_cast<float>(sampleRate);
        phase += phaseIncrement * numSamples;
        
        // Wrap phase to 0.0-1.0 range using fmod
        phase = std::fmod(phase, 1.0f);
    }

private:
    static constexpr size_t lookupTableSize = 512;
    std::array<float, lookupTableSize> lookupTable{};  // Zero-initialized
    
    double sampleRate = 44100.0;
    float phase = 0.0f;
    float rate = 1.0f;  // Hz
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFO)
};
