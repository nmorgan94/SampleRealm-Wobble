#include "WavetableVoice.h"
#include "../PluginProcessor.h"

//==============================================================================
WavetableVoice::WavetableVoice(const juce::AudioBuffer<float> wavetablesToUse[3],
                               AudioPluginAudioProcessor& processor)
    : wavetables(wavetablesToUse),
      owner(processor)
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
    
    currentMidiNote = midiNoteNumber;
    currentFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    
    level = velocity;
    
    auto wavetableSize = wavetables[0].getNumSamples();
    phaseIncrement = currentFrequency * wavetableSize / getSampleRate();
    
    // Reset all oscillator phases
    for (int i = 0; i < 3; ++i)
        currentPhases[i] = 0.0;
    
    // Initialize and trigger envelope
    envelope.setSampleRate(getSampleRate());
    
    float attack = owner.getFloatParam("env1_attack");
    float decay = owner.getFloatParam("env1_decay");
    float sustain = owner.getFloatParam("env1_sustain");
    float release = owner.getFloatParam("env1_release");
    
    envelope.setParameters(attack, decay, sustain, release);
    envelope.noteOn();
    
    // Trigger all modulation envelopes
    for (size_t i = 0; i < 4; ++i)
    {
        auto& env = owner.getEnvelope(i);
        env.setSampleRate(getSampleRate());
        
        juce::String envPrefix = "env" + juce::String(static_cast<int>(i + 1)) + "_";
        float envAttack = owner.getFloatParam(envPrefix + "attack");
        float envDecay = owner.getFloatParam(envPrefix + "decay");
        float envSustain = owner.getFloatParam(envPrefix + "sustain");
        float envRelease = owner.getFloatParam(envPrefix + "release");
        
        env.setParameters(envAttack, envDecay, envSustain, envRelease);
        env.noteOn();
    }
    
    // Trigger all LFOs that are in trigger mode
    for (size_t i = 0; i < 4; ++i)
    {
        owner.getLFO(i).trigger();
    }
}

void WavetableVoice::stopNote(float velocity, bool allowTailOff)
{
    juce::ignoreUnused(velocity);
    
    if (allowTailOff)
    {
        // Trigger envelope release
        envelope.noteOff();
        
        // Trigger release for all modulation envelopes
        for (size_t i = 0; i < 4; ++i)
        {
            owner.getEnvelope(i).noteOff();
        }
    }
    else
    {
        // Immediately stop the note
        clearCurrentNote();
        envelope.reset();
        
        // Reset all modulation envelopes
        for (size_t i = 0; i < 4; ++i)
        {
            owner.getEnvelope(i).reset();
        }
        
        level = 0.0;
    }
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
    if (!envelope.isActive())
    {
        clearCurrentNote();
        return;
    }
    
    auto wavetableSize = wavetables[0].getNumSamples();
    
    bool oscEnabled[3];
    float oscGain[3];
    double oscPhaseIncrement[3];
    
    for (int osc = 0; osc < 3; ++osc)
    {
        juce::String enableParamID = "osc" + juce::String(osc + 1) + "_enable";
        juce::String gainParamID = "osc" + juce::String(osc + 1) + "_gain";
        juce::String pitchParamID = "osc" + juce::String(osc + 1) + "_pitch";
        
        oscEnabled[osc] = owner.getBoolParam(enableParamID);
        oscGain[osc] = owner.getModulatedParam(gainParamID);
        
        int pitchOffset = owner.getIntParam(pitchParamID);
        double oscFrequency = juce::MidiMessage::getMidiNoteInHertz(currentMidiNote + pitchOffset);
        oscPhaseIncrement[osc] = oscFrequency * wavetableSize / getSampleRate();
    }
    
    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
    {
        auto* channelData = outputBuffer.getWritePointer(channel, startSample);
        double localPhases[3] = { currentPhases[0], currentPhases[1], currentPhases[2] };
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mixedSample = 0.0f;
            
            for (int osc = 0; osc < 3; ++osc)
            {
                if (oscEnabled[osc])
                {
                    mixedSample += getInterpolatedSample(osc, localPhases[osc]) * oscGain[osc];
                    
                    // Increment phase and wrap
                    localPhases[osc] += oscPhaseIncrement[osc];
                    while (localPhases[osc] >= wavetableSize)
                        localPhases[osc] -= wavetableSize;
                }
            }
            
            // Get envelope value for this sample
            float envelopeValue = envelope.getNextSample();
            
            // Apply velocity, envelope, and add to output
            channelData[sample] += mixedSample * level * envelopeValue;
        }
    }
    
    for (int osc = 0; osc < 3; ++osc)
    {
        if (oscEnabled[osc])
        {
            currentPhases[osc] += oscPhaseIncrement[osc] * numSamples;
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

