#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class AudioPluginAudioProcessor;

//==============================================================================
class WavetableVoice : public juce::SynthesiserVoice
{
public:
    WavetableVoice(const juce::AudioBuffer<float> wavetablesToUse[3],
                   AudioPluginAudioProcessor& processor);
    
    bool canPlaySound(juce::SynthesiserSound* sound) override;
    
    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound* sound,
                   int currentPitchWheelPosition) override;
    
    void stopNote(float velocity, bool allowTailOff) override;
    
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample, int numSamples) override;

private:
    const juce::AudioBuffer<float>* wavetables;
    AudioPluginAudioProcessor& owner;
    
    double currentPhases[3] = { 0.0, 0.0, 0.0 };
    double phaseIncrement = 0.0;
    float level = 0.0f;
    double currentFrequency = 0.0;

    float getInterpolatedSample(int oscIndex, double phase) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableVoice)
};

//==============================================================================
class WavetableSound : public juce::SynthesiserSound
{
public:
    WavetableSound() {}
    
    bool appliesToNote(int /*midiNoteNumber*/) override { return true; }
    bool appliesToChannel(int /*midiChannel*/) override { return true; }
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableSound)
};
