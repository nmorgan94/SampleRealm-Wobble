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
    return withinVoiceLimit && dynamic_cast<WavetableSound*>(sound) != nullptr;
}

void WavetableVoice::startNote(int midiNoteNumber, float velocity,
                               juce::SynthesiserSound* sound,
                               int currentPitchWheelPosition)
{
    juce::ignoreUnused(sound, currentPitchWheelPosition);
    
    currentMidiNote = midiNoteNumber;
    currentFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);

    level = velocity;

    static const juce::String glideTimeID { "glide_time" };

    const bool   legato       = owner.getHeldNoteCount() > 0;
    const float  glideTime    = owner.getModulatedParam(glideTimeID);
    const double glideSeconds = (legato && glideTime > 0.0f) ? glideTime : 0.0;

    const double startRatio = (glideSeconds > 0.0)
        ? juce::MidiMessage::getMidiNoteInHertz(juce::roundToInt(owner.getLastNotePitch())) / currentFrequency
        : 1.0;

    glideRatio.reset(getSampleRate(), glideSeconds);
    glideRatio.setCurrentAndTargetValue(startRatio);
    glideRatio.setTargetValue(1.0);

    for (int osc = 0; osc < 3; ++osc)
    {
        const auto size = wavetables[osc].getNumSamples();
        currentPhases[osc][0] = 0.0;
        for (int u = 1; u < Parameters::maxUnison; ++u)
            currentPhases[osc][u] = unisonRandom.nextDouble() * size;
    }
    
    static const juce::String envParamIDs[4][4] = {
        { "env1_attack", "env1_decay", "env1_sustain", "env1_release" },
        { "env2_attack", "env2_decay", "env2_sustain", "env2_release" },
        { "env3_attack", "env3_decay", "env3_sustain", "env3_release" },
        { "env4_attack", "env4_decay", "env4_sustain", "env4_release" },
    };

    // Initialize and trigger envelope
    envelope.setSampleRate(getSampleRate());

    envelope.setParameters(owner.getFloatParam(envParamIDs[0][0]),
                           owner.getFloatParam(envParamIDs[0][1]),
                           owner.getFloatParam(envParamIDs[0][2]),
                           owner.getFloatParam(envParamIDs[0][3]));
    envelope.noteOn();

    // Trigger all modulation envelopes
    for (size_t i = 0; i < 4; ++i)
    {
        auto& env = owner.getEnvelope(i);
        env.setSampleRate(getSampleRate());

        env.setParameters(owner.getFloatParam(envParamIDs[i][0]),
                          owner.getFloatParam(envParamIDs[i][1]),
                          owner.getFloatParam(envParamIDs[i][2]),
                          owner.getFloatParam(envParamIDs[i][3]));
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

    const int wavetableSize = wavetables[0].getNumSamples();

    const double currentGlideRatio = glideRatio.getCurrentValue();
    glideRatio.skip(numSamples);

    static const juce::String unisonVoicesID { "unison_voices" };
    static const juce::String unisonDetuneID { "unison_detune" };

    const int   unisonCount = juce::jlimit(1, Parameters::maxUnison,
                                           owner.getIntParam(unisonVoicesID));
    const float detuneCents = static_cast<float>(owner.getIntParam(unisonDetuneID));
    const float unisonGain  = 1.0f / std::sqrt(static_cast<float>(unisonCount)); // level compensation

    double unisonRatio[Parameters::maxUnison];
    for (int u = 0; u < unisonCount; ++u)
    {
        const double pos = (unisonCount == 1) ? 0.0
                                              : -1.0 + 2.0 * u / (unisonCount - 1); // -1..+1
        unisonRatio[u] = std::pow(2.0, (pos * detuneCents) / 1200.0);
    }

    bool         oscEnabled[3];
    float        oscGain[3];
    const float* oscData[3];
    double       oscPhaseIncrement[3][Parameters::maxUnison];

    static const juce::String enableIDs[3] = { "osc1_enable", "osc2_enable", "osc3_enable" };
    static const juce::String gainIDs[3]   = { "osc1_gain",   "osc2_gain",   "osc3_gain"   };
    static const juce::String pitchIDs[3]  = { "osc1_pitch",  "osc2_pitch",  "osc3_pitch"  };

    for (int osc = 0; osc < 3; ++osc)
    {
        oscEnabled[osc] = owner.getBoolParam(enableIDs[osc]);
        oscGain[osc]    = owner.getModulatedParam(gainIDs[osc]);
        oscData[osc]    = wavetables[osc].getReadPointer(0); // cached pointer, hoisted out of the sample loop

        const int    pitchOffset   = owner.getIntParam(pitchIDs[osc]);
        const double oscFrequency  = juce::MidiMessage::getMidiNoteInHertz(currentMidiNote + pitchOffset) * currentGlideRatio;
        const double baseIncrement = oscFrequency * wavetableSize / getSampleRate();

        for (int u = 0; u < unisonCount; ++u)
            oscPhaseIncrement[osc][u] = baseIncrement * unisonRatio[u];
    }

    const int numChannels = outputBuffer.getNumChannels();
    auto* const* channelPointers = outputBuffer.getArrayOfWritePointers();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float mixedSample = 0.0f;

        for (int osc = 0; osc < 3; ++osc)
        {
            if (! oscEnabled[osc])
                continue;

            const float* data = oscData[osc];

            for (int u = 0; u < unisonCount; ++u)
            {
                double& phase = currentPhases[osc][u];

                mixedSample += getInterpolatedSample(data, wavetableSize, phase) * oscGain[osc];

                phase += oscPhaseIncrement[osc][u];
                while (phase >= wavetableSize)
                    phase -= wavetableSize;
            }
        }

        mixedSample *= unisonGain;

        const float envelopeValue = envelope.getNextSample();
        const float out = mixedSample * level * envelopeValue;

        for (int channel = 0; channel < numChannels; ++channel)
            channelPointers[channel][startSample + sample] += out;
    }
}

float WavetableVoice::getInterpolatedSample(const float* data, int size, double phase)
{
    const int   index0 = static_cast<int>(phase);
    const int   index1 = (index0 + 1) % size;
    const float frac   = static_cast<float>(phase - index0);

    return data[index0] + frac * (data[index1] - data[index0]);
}

//==============================================================================
WavetableSynthesiser::WavetableSynthesiser(AudioPluginAudioProcessor& processor)
    : owner(processor)
{
    setNoteStealingEnabled(true);
}

void WavetableSynthesiser::noteOn(int midiChannel, int midiNoteNumber, float velocity)
{
    juce::Synthesiser::noteOn(midiChannel, midiNoteNumber, velocity);
    owner.registerNoteStart(midiNoteNumber);
}

void WavetableSynthesiser::noteOff(int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff)
{
    owner.registerNoteStop(midiNoteNumber);
    juce::Synthesiser::noteOff(midiChannel, midiNoteNumber, velocity, allowTailOff);
}

void WavetableSynthesiser::allNotesOff(int midiChannel, bool allowTailOff)
{
    juce::Synthesiser::allNotesOff(midiChannel, allowTailOff);
    owner.clearHeldNotes();
}
