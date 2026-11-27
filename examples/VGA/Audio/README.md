# Audio

Interactive **sound mixer** built with the FabGL GUI: each window is an audio channel with its own waveform
generator, volume and frequency, all mixed together by `fabgl::SoundGenerator` in real time.

At startup there are four channels, plus the *Main Controls* window. All the channels start disabled: tick
*Enable* in a channel to hear it.

## Controls

Each **Channel** window:

| Control | Function |
|---|---|
| **Volume** | Channel volume, 0...127 |
| **Type** | Generator: Sine, Square, Triangle, Sawtooth, Noise, or a sampled sound: *Loop* (drum loop), *Speech*, *Mario* |
| **Freq** | Frequency, 0...8000 Hz (for the sampled sounds it changes the playback speed) |
| **Enable** | Turns the channel on and off |

**Main Controls** window:

| Control | Function |
|---|---|
| **Add Channel** | Adds a new channel window (until the memory runs out) |
| **Reset** | Restarts the board |
| **Enable** | Global play/stop |
| **Vol** (vertical slider) | Master volume, 0...127 |

Windows can be moved, minimized and maximized with the mouse; with the keyboard, `Tab` moves the focus and the
arrow keys move the sliders.

## What it demonstrates

- `fabgl::SoundGenerator` with any number of channels (`attach()` / `detach()`, `setVolume()`, `play()`)
- The waveform generators: `SineWaveformGenerator`, `SquareWaveformGenerator`, `TriangleWaveformGenerator`,
  `SawtoothWaveformGenerator`, `NoiseWaveformGenerator`, and `SamplesGenerator` for 8-bit sampled sounds
  (converted from WAV with `tools/wav2header`)
- GUI: frames, sliders, combo boxes, check boxes, buttons, message boxes
- The dark theme of the library (`fabgl::uiDarkTheme`): `setupPalette()` after `setResolution()` and `apply()` in
  `init()`; the controls are created without style class and get the theme by type, the *Add Channel* button uses
  the `ButtonAccent` class

## Hardware

- VGA output (default pins)
- Audio output on GPIO25 (connect an amplifier or active speakers through a capacitor)
- PS/2 mouse on port 1 (recommended) and/or PS/2 keyboard on port 0

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
