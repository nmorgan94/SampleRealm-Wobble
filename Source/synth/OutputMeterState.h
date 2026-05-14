#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

class OutputMeterState
{
public:
    void reset()
    {
        for (auto& meterLevel : outputMeterLevels)
            meterLevel.store(0.0f);

        for (auto& clipHold : outputClipHolds)
            clipHold.store(0);
    }

    void processBlock(const juce::AudioBuffer<float>& buffer)
    {
        constexpr float meterDecay = 0.82f;
        constexpr int clipHoldBlocks = 18;

        for (int channel = 0; channel < 2; ++channel)
        {
            const float peak = (channel < buffer.getNumChannels()) ? buffer.getMagnitude(channel, 0, buffer.getNumSamples()) : 0.0f;
            const float previous = outputMeterLevels[static_cast<size_t>(channel)].load();
            const float smoothed = (peak > previous) ? peak : (previous * meterDecay + peak * (1.0f - meterDecay));
            outputMeterLevels[static_cast<size_t>(channel)].store(smoothed);

            if (peak >= 1.0f)
            {
                outputClipHolds[static_cast<size_t>(channel)].store(clipHoldBlocks);
            }
            else
            {
                const int hold = outputClipHolds[static_cast<size_t>(channel)].load();
                if (hold > 0)
                    outputClipHolds[static_cast<size_t>(channel)].store(hold - 1);
            }
        }
    }

    float getLevel(int channel) const
    {
        jassert(channel >= 0 && channel < 2);
        return outputMeterLevels[static_cast<size_t>(channel)].load();
    }

    bool isClipping(int channel) const
    {
        jassert(channel >= 0 && channel < 2);
        return outputClipHolds[static_cast<size_t>(channel)].load() > 0;
    }

private:
    std::atomic<float> outputMeterLevels[2] { 0.0f, 0.0f };
    std::atomic<int> outputClipHolds[2] { 0, 0 };
};
