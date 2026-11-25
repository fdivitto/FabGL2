# Synth

**FabGL Synth**: an 8-voice polyphonic synthesizer played with the PS/2 keyboard (like a piano) or with the mouse
on an on-screen keyboard. Each voice has its own ADSR envelope and resonant low-pass filter, all computed in
fixed point inside the sound generator interrupt.

## Screen

- **Waveform**: Sine, Square, Pulse 25%, Triangle, Sawtooth
- **Octave**: `Z <` and `> X` buttons move the keyboard down and up one octave (from C1 to C6)
- **Volume**: master volume
- **Envelope (ADSR)**: Attack (0-1000 ms), Decay (0-1000 ms), Sustain (0-100 %), Release (0-2000 ms)
- **Low-pass filter**: Cutoff (50-7000 Hz, logarithmic), Resonance (Q from 0.7, no resonance, upwards), On/Off
- **Now playing**: name and frequency of the last note
- **On-screen keyboard**: two octaves plus the final C. Keys pressed from the PS/2 keyboard and from the mouse are
  highlighted; the letters of the PS/2 keys are drawn on the keys

All the controls change the sound while you play.

## Keys

The PS/2 keyboard works like a piano keyboard (two rows of keys, the upper row gives the black keys):

```
  W E   T Y U   O P   ]          black keys:  C# D#  F# G# A#  C# D#  F#
 A S D F G H J K L ; '           white keys: C  D  E  F  G  A  B  C  D  E  F
```

| Key | Action |
|---|---|
| `A` `W` `S` `E` `D` `F` `T` `G` `Y` `H` `U` `J` `K` `O` `L` `P` `;` `'` `]` | Notes, from C to F# of the next octave (on the Italian layout `ò`, `à`, `+` replace `;`, `'`, `]`) |
| `Z` / `X` | Octave down / up |
| `1` ... `5` | Select the waveform |

With the mouse, click a key of the on-screen keyboard; drag with the button pressed to slide between notes
(glissando).

When more than 8 notes are requested, the synth reuses a voice that is releasing, otherwise the oldest one.

## What it demonstrates

- Custom `WaveformGenerator` subclasses (the voices) attached to `fabgl::SoundGenerator`
- Real-time DSP inside the sound generator interrupt: ADSR envelope and RBJ biquad low-pass filter in Q28 fixed
  point (the FPU cannot be used in the interrupt); coefficients are computed by the UI task and swapped atomically
- Key down / key up handling with virtual keys, ignoring typematic repeats
- A custom-painted GUI control (the piano keyboard) with mouse hit testing

## Hardware

- VGA output (default pins)
- Audio output on GPIO25
- PS/2 keyboard on port 0 and PS/2 mouse on port 1

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
