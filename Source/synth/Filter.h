#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

enum class FilterMode
{
    Lowpass = 0,
    Highpass,
    Bandpass,
    Notch
};

class Filter
{
public:
    Filter() = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        
        for (int ch = 0; ch < 2; ++ch)
        {
            lowpassFilter[ch].prepare(spec);
            highpassFilter[ch].prepare(spec);
            bandpassFilter[ch].prepare(spec);
            notchFilter[ch].prepare(spec);
        }
        
        updateCoefficients();
    }
    
    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            lowpassFilter[ch].reset();
            highpassFilter[ch].reset();
            bandpassFilter[ch].reset();
            notchFilter[ch].reset();
        }
    }
    
    void setMode(FilterMode newMode)
    {
        if (mode != newMode)
        {
            mode = newMode;
            updateCoefficients();
        }
    }
    
    void setCutoff(float cutoffHz)
    {
        cutoffFrequency = juce::jlimit(20.0f, 20000.0f, cutoffHz);
        updateCoefficients();
    }
    
    void setResonance(float resonanceValue)
    {
        qFactor = 0.5f + juce::jlimit(0.0f, 1.0f, resonanceValue) * 19.5f;
        updateCoefficients();
    }
    
    float processSample(float sample, int channel)
    {
        int ch = juce::jlimit(0, 1, channel);
        
        switch (mode)
        {
            case FilterMode::Lowpass:
                return lowpassFilter[ch].processSample(sample);
            case FilterMode::Highpass:
                return highpassFilter[ch].processSample(sample);
            case FilterMode::Bandpass:
                return bandpassFilter[ch].processSample(sample);
            case FilterMode::Notch:
                return notchFilter[ch].processSample(sample);
            default:
                return sample;
        }
    }
    
    FilterMode getMode() const { return mode; }

private:
    void updateCoefficients()
    {
        if (sampleRate <= 0.0)
            return;
        
        juce::dsp::IIR::Coefficients<float>::Ptr coeffs;
        
        switch (mode)
        {
            case FilterMode::Lowpass:
                coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, cutoffFrequency, qFactor);
                for (int ch = 0; ch < 2; ++ch)
                    *lowpassFilter[ch].coefficients = *coeffs;
                break;
                
            case FilterMode::Highpass:
                coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, cutoffFrequency, qFactor);
                for (int ch = 0; ch < 2; ++ch)
                    *highpassFilter[ch].coefficients = *coeffs;
                break;
                
            case FilterMode::Bandpass:
                coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, cutoffFrequency, qFactor);
                for (int ch = 0; ch < 2; ++ch)
                    *bandpassFilter[ch].coefficients = *coeffs;
                break;
                
            case FilterMode::Notch:
                coeffs = juce::dsp::IIR::Coefficients<float>::makeNotch(sampleRate, cutoffFrequency, qFactor);
                for (int ch = 0; ch < 2; ++ch)
                    *notchFilter[ch].coefficients = *coeffs;
                break;
        }
    }
    
    FilterMode mode = FilterMode::Lowpass;
    double sampleRate = 44100.0;
    float cutoffFrequency = 1000.0f;
    float qFactor = 0.707f;
    
    juce::dsp::IIR::Filter<float> lowpassFilter[2];
    juce::dsp::IIR::Filter<float> highpassFilter[2];
    juce::dsp::IIR::Filter<float> bandpassFilter[2];
    juce::dsp::IIR::Filter<float> notchFilter[2];
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Filter)
};
