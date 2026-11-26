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
SoundGenerator::setVolume(). The `VGA/Audio`, `VGA/Songs`, `VGA/SoundChipSimulator` and `VGA/Synth` examples show more
complete uses.


# Polyphonic synthesizer {#sound_synth}

fabgl::PolySynth plays notes with a pool of voices (8 by default). Each voice (fabgl::SynthVoice) is an oscillator
(sine, square, 25% pulse, triangle or sawtooth) followed by a resonant low-pass filter and shaped by an ADSR
envelope. The filter is a four-pole (24 dB/octave) ladder, like the classic analog synthesizer filters: high
resonance values emphasize the cutoff frequency, the maximum values make it self-oscillate. A second ADSR envelope
(PolySynth::filterEnvelope()) moves the cutoff of each voice, by the amount set with
ResonantLowPassFilter::setEnvelopeAmount(): the classic synth "sweep". A low frequency oscillator
(PolySynth::lfo(), fabgl::SynthLFO: triangle, sine, square, sawtooth or random) modulates the cutoff of all voices
(wah) and the amplitude (tremolo). All voices share
one fabgl::ADSREnvelope and one fabgl::ResonantLowPassFilter, whose parameters can be changed while notes are
playing. When all voices are busy, a new note steals a releasing voice, otherwise the
oldest one.

The voices work at 16 bit resolution: square, pulse and sawtooth are band-limited (PolyBLEP), so their harmonics
above half the sample rate don't fold back as non-harmonic tones. PolySynth mixes the voices into one
SoundGenerator channel (PolySynth::channel()) with an automatic gain of 1 / sqrt(sum of the envelope levels): the
loudness stays about the same when more notes play together and changes smoothly when notes start or end. A soft
limiter bends the peaks instead of clipping them.

```cpp
SoundGenerator soundGenerator;
PolySynth      synth;

synth.begin(&soundGenerator);            // attaches the voices and starts playing
synth.setWaveform(SynthWaveform::Sawtooth);

synth.envelope().setAttack(10);          // ms
synth.envelope().setDecay(200);          // ms
synth.envelope().setSustain(70);         // percent
synth.envelope().setRelease(300);        // ms

synth.filter().setCutoff(1500);          // Hz (50..7000)
synth.filter().setResonance(0.6f);       // 0 = no resonance .. 1 (self-oscillation above ~0.93)

synth.filterEnvelope().setDecay(400);    // the filter envelope moves the cutoff of each voice...
synth.filterEnvelope().setSustain(20);
synth.filter().setEnvelopeAmount(50);    // ...by up to 50% of the cutoff range (-100..100)

synth.lfo().setWaveform(LFOWaveform::Sine);
synth.lfo().setRate(5.0f);               // Hz (0.1..20)
synth.lfo().setCutoffDepth(20);          // percent: wah
synth.lfo().setAmpDepth(30);             // percent: tremolo

synth.noteOn(60);                        // MIDI note: 60 = C4, 69 = A4 (440 Hz)
// ...
synth.noteOff(60);
```

Envelope and filter run in fixed point inside the sound generator interrupt (the FPU cannot be used there). The
filter coefficients are calculated in floating point when a parameter changes. For user interfaces:
ResonantLowPassFilter::responseDB() returns the filter response at a frequency, cutoffFromPosition() and
resonanceFromPosition() map a 0..100 control to the cutoff (logarithmic scale) and to the resonance,
SynthLFO::rateFromPosition() to the LFO rate, SynthVoice::waveformShape() and SynthLFO::waveformShape() return
the shape of a waveform, fabgl::midiNoteName() and PolySynth::noteFrequency() convert MIDI note numbers.
The @ref example_Synth example combines the synthesizer with the synth controls of the GUI (see @ref gui_synth_controls).

