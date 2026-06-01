#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace Parameters
{
    inline constexpr int maxVoices = 8;

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
}
