# Synth

**FabGL Synth**: an 8-voice polyphonic synthesizer played with the PS/2 keyboard (like a piano) or with the mouse
on an on-screen keyboard. Each voice has its own ADSR envelopes and resonant low-pass filter, all computed in
fixed point inside the sound generator interrupt. 64 presets with classic synthesizer sounds can be edited and
saved.

## Screen

A dark "hardware synth" panel with a custom 16-color palette:

- **Header**: eight leds show the voices activity (green = playing, amber = releasing)
- **Oscillator**: the OSC / LFO selector chooses the page.
  - OSC: the selected waveform and one tile for each waveform (Sine, Square, Pulse 25%, Triangle, Sawtooth)
  - LFO (low frequency oscillator, shared by all voices): one tile for each LFO waveform (Triangle, Sine, Square,
    Sawtooth, Random) and three knobs: Rate (0.1-20 Hz, logarithmic), Cutoff (0-100 %: how much the LFO moves the
    filter cutoff; 100 % = up and down by half the cutoff range, about 3.5 octaves), Amp (0-100 %: tremolo depth;
    100 % = from full level down to silence)
- **Envelope (ADSR)**: the AMP / FILTER selector chooses the envelope to edit: the amplitude envelope or the filter
  envelope (it moves the cutoff frequency). Envelope shape and four knobs: Attack (0-1000 ms), Decay (0-1000 ms),
  Sustain (0-100 %), Release (0-2000 ms)
- **Low-pass filter** (4-pole ladder, 24 dB/octave): the filter frequency response, the On/Off switch and three
  knobs: Cutoff (50-7000 Hz, logarithmic), Resonance (0-100 %; above about 93 % the filter self-oscillates and
  sings at the cutoff frequency), Env Amt (-100..+100 %: how much the filter envelope moves the cutoff; 100 % =
  the whole range, about 7 octaves; negative values move it down)
- **Preset**: number and name of the selected preset (a `*` follows the name when the sound has been changed),
  the arrow buttons select the previous / next preset, SAVE and RESET (see Presets below)
- **Octave**: the arrow buttons move the keyboard down and up one octave (from C1 to C6)
- **Master volume**: a segmented level bar
- **On-screen keyboard**: two octaves plus the final C. Keys pressed from the PS/2 keyboard and from the mouse are
  highlighted; the letters of the PS/2 keys are drawn on the keys

All the controls change the sound while you play, and the graphs follow them.

Knobs: drag up or down with the mouse button pressed, or use the mouse wheel; double click restores the initial
value. Master volume: click or drag on the bar, or use the mouse wheel.

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
| `PGUP` / `PGDN` | Previous / next preset |

With the mouse, click a key of the on-screen keyboard; drag with the button pressed to slide between notes
(glissando).

When more than 8 notes are requested, the synth reuses a voice that is releasing, otherwise the oldest one.

## Presets

A preset stores all the controls: waveform, both envelopes, filter (switch, cutoff, resonance, envelope amount)
and LFO (waveform, rate, depths). Selecting a preset sets the controls; then every control can be changed, and a
`*` after the preset name shows that the sound differs from the stored one. Changes are lost when another preset is
selected, unless they are saved.

- **SAVE** opens a dialog: choose the destination preset with the arrows (by default the current one) and type the
  name (up to 16 characters, stored in upper case), then press SAVE. The sound is stored in the NVS flash partition
  and survives power off.
- **RESET** loads the factory sound of the current preset into the controls (it is not stored until SAVE: select
  the preset again to get back the saved sound).

The synth starts with preset 01. Erasing the flash (`idf.py erase-flash`) restores all the factory
presets. The factory sounds are defined in `main/presets.h`:

| # | Group | Presets |
|---|---|---|
| 01-08 | Brass | BRASS SECTION, SYNTH BRASS, SOFT HORNS, TRUMPET, BRASS STAB, POLY BRASS, FANFARE, LOW BRASS |
| 09-18 | Strings and pads | STRING ENSEMBLE, SLOW STRINGS, WARM PAD, GLASS PAD, SWEEP PAD, CHOIR, DARK PAD, PWM PAD, SPACE PAD, VOX HUMANA |
| 19-28 | Leads | SAW LEAD, SQUARE LEAD, RESO LEAD, MINI LEAD, SINE WHISTLE, FLUTE, OBOE, CLARINET, SCREAM LEAD, ACID LEAD |
| 29-38 | Bass | MOOG BASS, ACID BASS, SUB BASS, SQUARE BASS, PLUCK BASS, FAT BASS, WOBBLE BASS, RUBBER BASS, FUNK BASS, DEEP BASS |
| 39-46 | Keys and organs | ORGAN, JAZZ ORGAN, ELECTRIC PIANO, CLAVINET, HARPSICHORD, SYNTH PIANO, VIBES, ACCORDION |
| 47-54 | Plucks and bells | PLUCK, HARP, KOTO, MARIMBA, BELL, MUSIC BOX, STEEL DRUM, PIZZICATO |
| 55-64 | Sequencer sounds and effects | ARP SYNTH, BLIP, SAMPLE AND HOLD, TREMOLO KEYS, WAH SYNTH, SIREN, SPACE WIND, SCI-FI DRONE, HELICOPTER, INIT |

The oscillator has no pitch modulation: "vibrato" presets use a light tremolo, and the "wind" and "drone"
sounds use the filter resonance instead of noise.

## What it demonstrates

- `fabgl::PolySynth`: a pool of `SynthVoice` objects (band-limited oscillator, resonant low-pass filter, amplitude
  and filter ADSR envelopes) and a shared `SynthLFO`, mixed into one `fabgl::SoundGenerator` channel, with shared `ADSREnvelope` and
  `ResonantLowPassFilter` parameters. The DSP runs in 16 bit fixed point inside the sound generator interrupt
  (Q28 four-pole zero-delay-feedback ladder)
- The synth controls of the GUI: `uiKnob`, `uiToggleSwitch`, `uiLevelBar`, `uiLedBar`, `uiStepper`,
  `uiTileSelector` (with tiles painted by the application), `uiGraph` (with overlays painted by the application)
  and `uiPianoKeyboard`
- A `uiStyle` that themes all the controls through style class IDs
- A modal dialog (`uiFrame`, `uiStepper`, `uiTextEdit`, `uiButton`) and data stored in NVS with `fabgl::Preferences`
- A custom palette on `VGA16Controller` (`setPaletteItem()`)
- Key down / key up handling with virtual keys: the PS/2 keyboard and the mouse press the same piano keys

## Hardware

- VGA output (default pins)
- Audio output on GPIO25
- PS/2 keyboard on port 0 and PS/2 mouse on port 1

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
