#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <memory>

class Drive
{
public:
    Drive() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        // Build the oversampler for the actual channel count
        oversamplerChannels = static_cast<int>(spec.numChannels);
        oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
            static_cast<size_t>(spec.numChannels), 1,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
        oversampler->initProcessing(static_cast<size_t>(spec.maximumBlockSize));
        oversampler->reset();
        currentHQ = false;
    }

    void setDrive(float amount01)                  // 0..1 from the (modulated) param
    {
        const float a = juce::jlimit(0.0f, 1.0f, amount01);
        driveGain = a * maxDriveGain;              // pre-gain into the shaper
        invTanh   = driveGain > kMinGain ? 1.0f / std::tanh(driveGain) : 1.0f;
    }

    void process(juce::AudioBuffer<float>& buffer, bool hq)
    {
        const bool useHQ = hq && oversampler != nullptr
                              && buffer.getNumChannels() == oversamplerChannels;

        if (useHQ != currentHQ)
        {
            currentHQ = useHQ;
            if (useHQ)
                oversampler->reset();         
        }

        if (driveGain <= kMinGain)
            return;                       

        juce::dsp::AudioBlock<float> block (buffer);

        if (useHQ)
        {
            auto osBlock = oversampler->processSamplesUp (block);
            shapeBlock (osBlock);
            oversampler->processSamplesDown (block);
        }
        else
        {
            shapeBlock (block);
        }
    }

private:
    void shapeBlock(juce::dsp::AudioBlock<float>& block) const
    {
        for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
        {
            auto* samples = block.getChannelPointer (channel);
            for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
                samples[sample] = shape (samples[sample]);
        }
    }

    float shape(float x) const
    {
        return std::tanh(driveGain * x) * invTanh; 
    }

    static constexpr float maxDriveGain = 20.0f;   
    static constexpr float kMinGain     = 1.0e-4f;
    float driveGain = 0.0f;
    float invTanh   = 1.0f;
    bool  currentHQ = false;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    int oversamplerChannels = 0;                   

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Drive)
};
