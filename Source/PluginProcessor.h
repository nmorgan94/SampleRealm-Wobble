#pragma once

#include <algorithm>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include "synth/WavetableVoice.h"
#include "synth/WavetableGenerator.h"
#include "synth/LFO.h"
#include "synth/ADSREnvelope.h"
#include "synth/Filter.h"
#include "synth/OutputMeterState.h"
#include "ModulationManager.h"
#include "Parameters.h"
#include "ui/CurveEditor.h"
#include "LFOCurveState.h"

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    //==============================================================================
    const juce::AudioBuffer<float>& getWavetable(int oscIndex) const;
    bool getBoolParam(const juce::String& paramID) const;
    float getFloatParam(const juce::String& paramID) const;
    int getChoiceParam(const juce::String& paramID) const;
    int getIntParam(const juce::String& paramID) const;
    float getModulatedParam(const juce::String& paramID) const;
    
    // Get LFO for external access (e.g., from editor)
    LFO& getLFO(size_t lfoIndex)
    {
        jassert(lfoIndex < 4);
        return lfos[lfoIndex];
    }
    
    ADSREnvelope& getEnvelope(size_t envIndex)
    {
        jassert(envIndex < 4);
        return envelopes[envIndex];
    }
    
    ModulationManager& getModulationManager() { return modulationManager; }

    int    getHeldNoteCount() const { return static_cast<int>(heldNotes.size()); }
    double getLastNotePitch() const { return heldNotes.empty() ? 60.0 : static_cast<double>(heldNotes.back()); }
    void   registerNoteStart(int midiNote) { heldNotes.push_back(midiNote); }
    void   registerNoteStop(int midiNote)
    {
        auto noteEntry = std::find(heldNotes.begin(), heldNotes.end(), midiNote);
        if (noteEntry != heldNotes.end())
            heldNotes.erase(noteEntry);
    }
    void   clearHeldNotes() { heldNotes.clear(); }
    
    Filter& getFilter() { return filter; }
    
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    
    float getOutputMeterLevel(int channel) const
    {
        return outputMeterState.getLevel(channel);
    }
    
    bool isOutputClipping(int channel) const
    {
        return outputMeterState.isClipping(channel);
    }
    
    // LFO curve persistence
    std::vector<CurveEditor::ControlPoint> getLFOCurvePoints(size_t lfoIndex) const
    {
        jassert(lfoIndex < 4);
        return lfoCurveState.getCurvePoints(lfoIndex);
    }
    
    void setLFOCurvePoints(size_t lfoIndex, const std::vector<CurveEditor::ControlPoint>& points)
    {
        jassert(lfoIndex < 4);
        lfoCurveState.setCurvePoints(lfoIndex, points);
    }
    
    juce::AudioProcessorValueTreeState apvts;

private:
    static inline const juce::Identifier modMatrixStateID { "ModMatrix" };
    static inline const juce::Identifier lfoCurvesStateID { "LFOCurves" };
    //==============================================================================
    juce::UndoManager undoManager;
    WavetableSynthesiser synth { *this };
    juce::AudioBuffer<float> wavetables[3];
    LFO lfos[4];
    ADSREnvelope envelopes[4];
    ModulationManager modulationManager;
    LFOCurveState lfoCurveState;
    Filter filter;
    OutputMeterState outputMeterState;
    
    WaveformType currentWaveformTypes[3] = { WaveformType::Sine, WaveformType::Sine, WaveformType::Sine };

    int currentVoiceCount = -1;

    std::vector<int> heldNotes;
    
    static juce::ValueTree getOrCreateStateChild(juce::ValueTree parent, const juce::Identifier& childID);
    
    void updateWavetables();
    void updateLFOs();
    void updateFilter();
    void updateVoiceCount();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
