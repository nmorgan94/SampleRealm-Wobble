#include "WavetableVoice.h"

//==============================================================================
WavetableVoice::WavetableVoice(const juce::AudioBuffer<float> wavetablesToUse[3],
                               const bool* oscEnabledStates)
    : wavetables(wavetablesToUse),
      oscEnabled(oscEnabledStates)
{
}

bool WavetableVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<WavetableSound*>(sound) != nullptr;
}

void WavetableVoice::startNote(int midiNoteNumber, float velocity,
                               juce::SynthesiserSound* sound,
                               int currentPitchWheelPosition)
{
    juce::ignoreUnused(sound, currentPitchWheelPosition);
    
    currentFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    
    level = velocity;
    
    auto wavetableSize = wavetables[0].getNumSamples();
    phaseIncrement = currentFrequency * wavetableSize / getSampleRate();
    
    // Reset all oscillator phases
    for (int i = 0; i < 3; ++i)
        currentPhases[i] = 0.0;
}

void WavetableVoice::stopNote(float velocity, bool allowTailOff)
{
    juce::ignoreUnused(velocity, allowTailOff);
    
    // Immediately stop the note (no release envelope)
    clearCurrentNote();
    level = 0.0;
}

void WavetableVoice::pitchWheelMoved(int newPitchWheelValue)
{
    juce::ignoreUnused(newPitchWheelValue);
}

void WavetableVoice::controllerMoved(int controllerNumber, int newControllerValue)
{
    juce::ignoreUnused(controllerNumber, newControllerValue);
}

void WavetableVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                     int startSample, int numSamples)
{
    if (level == 0.0f)
        return;
    
    auto wavetableSize = wavetables[0].getNumSamples();
    
    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
    {
        auto* channelData = outputBuffer.getWritePointer(channel, startSample);
        double localPhases[3] = { currentPhases[0], currentPhases[1], currentPhases[2] };
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mixedSample = 0.0f;
            int enabledCount = 0;
            
            // Mix all enabled oscillators
            for (int osc = 0; osc < 3; ++osc)
            {
                if (oscEnabled[osc])
                {
                    mixedSample += getInterpolatedSample(osc, localPhases[osc]);
                    enabledCount++;
                    
                    // Increment phase and wrap
                    localPhases[osc] += phaseIncrement;
                    while (localPhases[osc] >= wavetableSize)
                        localPhases[osc] -= wavetableSize;
                }
            }
            
            // Average the oscillators to prevent clipping
            if (enabledCount > 0)
                mixedSample /= static_cast<float>(enabledCount);
            
            // Apply level and add to output
            channelData[sample] += mixedSample * level;
        }
    }
    
    // Update the phases for next block
    for (int osc = 0; osc < 3; ++osc)
    {
        if (oscEnabled[osc])
        {
            currentPhases[osc] += phaseIncrement * numSamples;
            while (currentPhases[osc] >= wavetableSize)
                currentPhases[osc] -= wavetableSize;
        }
    }
}

float WavetableVoice::getInterpolatedSample(int oscIndex, double phase) const
{
    auto wavetableSize = wavetables[oscIndex].getNumSamples();
    auto* wavetableData = wavetables[oscIndex].getReadPointer(0);
    
    auto index0 = static_cast<int>(phase);
    auto index1 = (index0 + 1) % wavetableSize;
    auto frac = phase - static_cast<double>(index0);
    
    auto sample0 = wavetableData[index0];
    auto sample1 = wavetableData[index1];
    
    return static_cast<float>(sample0 + frac * (sample1 - sample0));
}

