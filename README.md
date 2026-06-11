# SampleRealm: Wobble

A 3-oscillator wavetable synthesizer with deep modulation

## Features

### Oscillators
- 3 independent wavetable oscillators
- 11 waveforms — Sine, Saw, Square, Triangle, Formant Sine, Wavefolded Sine, Double Saw, Resonant Saw, Log Square, Asymmetric Rectified Sine, and Interleaved Saw Pulse
- Continuous morphing between adjacent waveforms
- Coarse and fine pitch control per oscillator
- Glide / portamento for smooth pitch transitions
- Unison with adjustable voice count, detune, and stereo spread
- Real-time waveform visualization
- 8-voice polyphony

### Filters & Distortion
- Multi-mode filter — Lowpass, Highpass, Bandpass, Notch
- Pre-filter distortion (drive)
- Live filter-response display

### Modulation
- Assignable LFOs with custom drawable curves
- Assignable ADSR envelopes
- Modulation routing manager to assign sources to destinations
- Envelope and LFO playhead visualization

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
open build-xcode/Wobble.xcodeproj
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