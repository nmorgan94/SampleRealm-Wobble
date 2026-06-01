#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace Parameters
{
    inline constexpr int maxVoices = 8;
    inline constexpr int maxUnison = 7;

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
}
