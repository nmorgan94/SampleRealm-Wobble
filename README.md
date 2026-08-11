# SampleRealm: Modulate

A 3-oscillator wavetable synthesizer with deep modulation

## Features

### Oscillators
- 3 independent wavetable oscillators, each with its own enable, gain, and pitch
- 21 waveforms — Sine, Saw, Square, Triangle, Pulse 25%, Pulse 10%, Formant Sine, Formant Vowel, Bitcrushed Ramp, FM Operator, Feedback FM, Wavefolded Sine, Fractal Pulse, Double Saw, Resonant Saw, Prime Cluster, Odd Harmonic Cluster, Log Square, Asymmetric Rectified Sine, Interleaved Saw Pulse, and Trapezoid
- A/B morphing — pick any two waveforms per oscillator and sweep continuously between them
- Coarse (±36 semitones) and fine (±99 cents) pitch per oscillator, plus a global coarse pitch
- Glide / portamento up to 2 seconds, with legato detection
- Unison up to 7 voices with adjustable detune and stereo spread
- Selectable polyphony, 1 to 8 voices
- Real-time waveform visualization

### Noise
- Switchable white or pink noise generator
- Independent level control

### Filter & Distortion
- Multi-mode filter — Lowpass, Highpass, Bandpass, Notch
- Selectable 12 dB/oct or 24 dB/oct slope, Butterworth-aligned across a two-stage cascade
- Pre-filter drive with an optional HQ mode (2× oversampled to suppress aliasing)
- Live filter-response display

### Modulation
- 4 LFOs with freely drawable curves and adjustable tension
- 5 starting shapes per LFO — Sine, Triangle, Ramp Up, Ramp Down, Square
- Per-note trigger or free-running mode
- Tempo sync across 15 divisions, from 1/16 to 4 bars, including triplets and dotted values
- Free-run rate from 0.01 to 20 Hz
- 4 assignable ADSR envelopes
- Modulation routing manager to assign sources to destinations
- Envelope and LFO playhead visualization

### Presets
- Save, load, and delete user presets

### Output
- Master gain, −24 to +6 dB
- Stereo peak metering with clip indication

## Building

## Build Requirements

- CMake 3.25+
- A C++23-capable compiler
- Git
- macOS development environment for AU/Standalone/VST3 builds

**Debug Build:**
```bash
cmake --preset debug
cmake --build --preset debug
```

**Release Build:**
```bash
cmake --preset release
cmake --build --preset release
```

## Debugging in Xcode

To debug the plugin in Xcode with an executable:

### 1. Generate Xcode Project

```bash
cmake -B build-xcode -G Xcode
open build-xcode/Modulate.xcodeproj
```

### 2. Configure Debugging

1. Select your plugin target from the scheme dropdown
2. Go to **Product → Scheme → Edit Scheme** 
3. Click **Run** on the left sidebar
4. Under **Executable**, choose **Other** and navigate to executable.

### 3. Build and Run

1. Press **Cmd+B** to build the plugin
2. Press **Cmd+R** to run with AudioPluginHost
4. Load your plugin in AudioPluginHost