# Songs

Plays simple **songs written as text strings**, using a small ADSR "sound chip" built on top of
`fabgl::SoundGenerator`. By Carles Oriol.

The program plays, in an endless loop:

1. a melody with a triangle wave
2. the same melody with a second, lower sine voice
3. a three-track tune (sine lead with glides, square bass and noise percussion)

The screen only shows a title; there are no controls.

## Song notation

Notes are written as `<note>[#|b]<octave> <duration>` separated by spaces:

- note `A`...`G`, optional `#` (sharp) or `b` (flat), octave `2`...`7` (for example `C#5`); `P` is a pause
- in `play_song1()` the duration is a fraction of a second (`4` = 1/4 s, `2` = 1/2 s, `16` = 1/16 s)
- in `play_song2()` the duration is in beats (`1` = 250 ms, `0.5`, `1.5`, `4`, ...)

Example: `"A4 4 A4 4 A#4 4 C5 4 P 8"`. Edit the strings in `Songs.cpp` to play your own tunes.

## What it demonstrates

- Creating and destroying waveform generators on the fly, one per note, each in its own task
- Software ADSR envelope (attack, decay, sustain, release) and frequency glides (portamento)
- Several voices playing at the same time through `SoundGenerator`

See also [SoundChipSimulator](../SoundChipSimulator) (same engine, sound effects) and [Synth](../Synth).

## Hardware

- VGA output (default pins)
- Audio output on GPIO25

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
