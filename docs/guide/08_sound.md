Sound {#guide_sound}
=====

[TOC]

# Sound generator and channels {#sound_generator}

fabgl::SoundGenerator produces a mono audio signal, either with the internal DAC (GPIO 25 or 26) or with
sigma-delta modulation (almost any GPIO). By default it chooses the DAC on GPIO 25 when the video output is
VGA, and sigma-delta on GPIO 23 when it is composite (which uses the DAC for video). Only one SoundGenerator can exist.

The generator mixes any number of *waveform generators* (channels), all derived from fabgl::WaveformGenerator:

| Generator | Sound |
|---|---|
| fabgl::SineWaveformGenerator | sine wave |
| fabgl::SquareWaveformGenerator | square wave with adjustable duty cycle |
| fabgl::TriangleWaveformGenerator | triangle wave |
| fabgl::SawtoothWaveformGenerator | sawtooth wave |
| fabgl::NoiseWaveformGenerator | white noise |
| fabgl::VICNoiseGenerator | noise of the Commodore VIC-20 sound chip |
| fabgl::SamplesGenerator | 8-bit signed samples (sampled sounds) |

See @ref confAudio for the output circuit.


# Playing a sound {#sound_play}

The simplest way to play a note is playSound(): it creates a temporary channel that is destroyed when the
sound ends, and returns immediately.

```cpp
SoundGenerator soundGenerator;

soundGenerator.playSound(SineWaveformGenerator(), 440, 500);         // 440 Hz for 500 ms
soundGenerator.playSound(SquareWaveformGenerator(), 880, 200, 60);   // volume 60 (0..127)
```

Sampled sounds (for example converted from WAV files with `tools/wav2header`) are played with playSamples():

```cpp
soundGenerator.playSamples(explosion_samples, sizeof(explosion_samples), 100);
```

Samples must have the same sample rate as the sound generator (16384 Hz by default).


# Permanent channels {#sound_channels}

For music and sound effects that change over time, attach your own channels and control them directly:

```cpp
SineWaveformGenerator  sine;
SquareWaveformGenerator square;

soundGenerator.attach(&sine);
soundGenerator.attach(&square);

sine.setFrequency(262);   sine.setVolume(80);   sine.enable(true);
square.setFrequency(392); square.setVolume(40); square.enable(true);

soundGenerator.play(true);          // start the output
// ...
square.enable(false);               // mute one channel
soundGenerator.play(false);         // stop the output
```

Channels are disabled when created: call enable(true) to hear them. The overall volume is set with
SoundGenerator::setVolume(). The `VGA/Audio`, `VGA/Songs` and `VGA/SoundChipSimulator` examples show more
complete uses.
