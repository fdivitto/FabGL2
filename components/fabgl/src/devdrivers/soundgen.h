/*
  Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com) - <http://www.fabgl.com>
  Copyright (c) 2019-2026 Fabrizio Di Vittorio.
  All rights reserved.


* Please contact fdivitto2013@gmail.com if you need a commercial license.


* This library and related software is available under GPL v3.

  FabGL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  FabGL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with FabGL.  If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once


/**
 * @file
 *
 * @brief This file contains all the classes of the FabGL sound system
 */




#include <stdint.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"

#include "esp32/rom/lldesc.h"
#include "soc/i2s_struct.h"
#include "soc/sens_struct.h"
#include "esp_timer.h"
#ifndef FABGL_EMULATED
  #include "driver/sdm.h"
#endif

#include "fabglconf.h"
#include "fabutils.h"



namespace fabgl {


#define FABGL_SOUNDGEN_DEFAULT_SAMPLE_RATE 16384


// DAC mode specific:
// setting 64 at 16KHz, generates 16000/64=250 interrupts per second
#define FABGL_SOUNDGEN_SAMPLE_BUFFER_SIZE 64



/**
 * @brief Abstract base class for waveform generators. A waveform generator is an audio channel that SoundGenerator mixes with the others.
 *
 * @ingroup audio
 */
class WaveformGenerator {
public:
  WaveformGenerator() : next(nullptr), m_sampleRate(0), m_volume(100), m_enabled(false), m_duration(-1), m_autoDestroy(false), m_autoDetach(false) { }

  virtual ~WaveformGenerator() { }

  /**
   * @brief Sets the output frequency
   *
   * @param value Frequency in Hertz
   */
  virtual void setFrequency(int value) = 0;

  /**
   * @brief Sets the number of samples to play
   *
   * @param value Number of samples to play. -1 = infinite.
   */
  void setDuration(uint32_t value) { m_duration = value; }

  /**
   * @brief Returns the number of samples still to play
   *
   * @return Number of samples still to play. -1 = infinite.
   */
  uint32_t duration() { return m_duration; }

  /**
   * @brief Sets the auto-detach mode
   *
   * @param value If true, this object is detached from the sound generator when there are no more samples to play.
   */
  void setAutoDetach(bool value) { m_autoDetach = value; }

  bool autoDetach() { return m_autoDetach; }

  /**
   * @brief Sets the auto-destroy mode
   *
   * @param value If true, this object is destroyed by the sound generator when there are no more samples to play. This also implies setAutoDetach(true).
   */
  void setAutoDestroy(bool value) { m_autoDestroy = value; m_autoDetach |= value; }

  bool autoDestroy() { return m_autoDestroy; }

  /**
   * @brief Gets the next sample
   *
   * @return Sample value as a signed 8-bit integer (-128..127)
   */
  virtual int getSample() = 0;

  /**
   * @brief Sets the volume of this generator
   *
   * @param value Volume value. Minimum is 0, maximum is 127.
   */
  void setVolume(int value) { m_volume = value; }

  /**
   * @brief Determines the current volume
   *
   * @return Current volume of this generator (0 = minimum, 127 = maximum)
   */
  int volume() { return m_volume; }

  /**
   * @brief Determines whether this generator is enabled or disabled
   *
   * @return True if this generator is enabled
   */
  bool enabled() { return m_enabled; }

  /**
   * @brief Enables or disables this generator
   *
   * Generators are disabled by default and must be enabled to play sound.
   *
   * @param value True to enable the generator, false to disable it
   */
  void enable(bool value) { m_enabled = value; }

  /**
   * @brief Sets the sample rate
   *
   * The default sample rate is FABGL_SOUNDGEN_DEFAULT_SAMPLE_RATE (16384 Hz).
   *
   * @param value Sample rate in Hertz
   */
  void setSampleRate(int value) { m_sampleRate = value; }

  /**
   * @brief Determines the sample rate
   *
   * @return Current sample rate in Hertz
   */
  uint16_t sampleRate() { return m_sampleRate; }

  WaveformGenerator * next;

protected:

  void decDuration() { --m_duration; if (m_duration == 0) m_enabled = false; }

private:
  uint16_t m_sampleRate;
  int8_t   m_volume;
  int8_t   m_enabled;   // 0 = disabled, 1 = enabled
  uint32_t m_duration;  // number of samples to play (-1 = infinite)
  bool     m_autoDestroy; // if true: this object needs to be destroyed by the sound generator when there are no more samples to play
  bool     m_autoDetach;  // if true: this object needs to be autodetached from the sound generator when there are no more samples to play
};


/**
 * @brief Sine waveform generator
 *
 * @ingroup audio
 */
class SineWaveformGenerator : public WaveformGenerator {
public:
  SineWaveformGenerator();

  void setFrequency(int value);

  int getSample();

private:
  uint32_t m_phaseInc;
  uint32_t m_phaseAcc;
  uint16_t m_frequency;
  int16_t  m_lastSample;
};


/**
 * @brief Square waveform generator
 *
 * @ingroup audio
 */
class SquareWaveformGenerator : public WaveformGenerator {
public:
  SquareWaveformGenerator();

  void setFrequency(int value);

  /**
   * @brief Sets the square wave duty cycle
   *
   * @param dutyCycle Duty cycle in the 0..255 range (255 = 100%)
   */
  void setDutyCycle(int dutyCycle);

  int getSample();

private:
  uint32_t m_phaseInc;
  uint32_t m_phaseAcc;
  uint16_t m_frequency;
  int8_t   m_lastSample;
  uint8_t  m_dutyCycle;
};


/**
 * @brief Triangle waveform generator
 *
 * @ingroup audio
 */
class TriangleWaveformGenerator : public WaveformGenerator {
public:
  TriangleWaveformGenerator();

  void setFrequency(int value);

  int getSample();

private:
  uint32_t m_phaseInc;
  uint32_t m_phaseAcc;
  uint16_t m_frequency;
  int8_t   m_lastSample;
};


/**
 * @brief Sawtooth waveform generator
 *
 * @ingroup audio
 */
class SawtoothWaveformGenerator : public WaveformGenerator {
public:
  SawtoothWaveformGenerator();

  void setFrequency(int value);

  int getSample();

private:
  uint32_t m_phaseInc;
  uint32_t m_phaseAcc;
  uint16_t m_frequency;
  int8_t   m_lastSample;
};


/**
 * @brief Noise generator
 *
 * @ingroup audio
 */
class NoiseWaveformGenerator : public WaveformGenerator {
public:
  NoiseWaveformGenerator();

  void setFrequency(int value);

  int getSample();

private:
  uint16_t m_noise;
};


/////////////////////////////////////////////////////////////////////////////////////////////
// "tries" to emulate VIC6561 noise generator
// derived from a reverse engineered VHDL code: http://www.denial.shamani.dk/bb/viewtopic.php?t=8733&start=210

/**
 * @brief Emulates the VIC 6561 (VIC-20) noise generator
 *
 * Based on reverse-engineered VHDL code: http://www.denial.shamani.dk/bb/viewtopic.php?t=8733&start=210
 *
 * @ingroup audio
 */
class VICNoiseGenerator : public WaveformGenerator {
public:
  VICNoiseGenerator();

  void setFrequency(int value);
  uint16_t frequency() { return m_frequency; }

  int getSample();

private:
  static const uint16_t LFSRINIT = 0x0202;
  static const int      CLK      = 4433618;

  uint16_t m_frequency;
  uint16_t m_counter;
  uint16_t m_LFSR;
  uint16_t m_outSR;
};



/**
 * @brief Samples generator
 *
 * Sample data must have the same sample rate as the sound generator.
 * Only signed, uncompressed 8-bit samples are supported.
 *
 * @ingroup audio
 */
class SamplesGenerator : public WaveformGenerator {
public:
  SamplesGenerator(int8_t const * data, int length);

  void setFrequency(int value);

  int getSample();

private:
  int8_t const * m_data;
  int            m_length;
  int            m_index;
};


/** \ingroup Enumerations
 * @brief Sound generation methods
 */
enum class SoundGenMethod {
  DAC,             /**< Use the DAC. Available on GPIO 25 or 26. Very low resource usage. */
  SigmaDelta,      /**< Use sigma-delta modulation. Available on almost all GPIOs. Requires a lot of CPU time at high sample rates. */
  Auto,            /**< Use the DAC when the video output is VGA, sigma-delta when the video output is composite. */
};


/**
 * @brief SoundGenerator handles audio output
 *
 * SoundGenerator generates audio using the DAC or sigma-delta modulation. The GPIO and the generation method are specified in the constructor.
 * Applications attach waveform generators (such as SineWaveformGenerator, SquareWaveformGenerator, etc.) and call SoundGenerator::play() to start audio generation.
 *
 * The available generators are:
 * - SineWaveformGenerator
 * - SquareWaveformGenerator
 * - TriangleWaveformGenerator
 * - SawtoothWaveformGenerator
 * - NoiseWaveformGenerator
 * - VICNoiseGenerator
 * - SamplesGenerator
 *
 * @see @ref guide_sound
 *
 * @ingroup audio
 */
class SoundGenerator {

public:

  /**
   * @brief Creates the sound generator. Only one instance is allowed
   *
   * @param sampleRate Sample rate in Hertz.
   * @param gpio GPIO to use. DAC mode is available on GPIO 25 or 26. GPIO_AUTO selects GPIO 25 when genMethod is DAC and GPIO 23 when genMethod is SigmaDelta.
   * @param genMethod Sound generation method: DAC, sigma-delta or automatic. Automatic selects the DAC with VGA output and sigma-delta with composite output.
   *
   * Example:
   *
   *     // create a sound generator with automatic settings (depending on the video output, VGA or CVBS)
   *     SoundGenerator soundGenerator;
   *
   *     // create a sound generator with a 16 kHz sample rate, GPIO 25 in DAC mode
   *     SoundGenerator soundGenerator(16000, GPIO_NUM_25, SoundGenMethod::DAC);
   *
   *     // create a sound generator with a 16 kHz sample rate, GPIO 23 in sigma-delta mode
   *     SoundGenerator soundGenerator(16000, GPIO_NUM_23, SoundGenMethod::SigmaDelta);
   */
  SoundGenerator(int sampleRate = FABGL_SOUNDGEN_DEFAULT_SAMPLE_RATE, gpio_num_t gpio = GPIO_AUTO, SoundGenMethod genMethod = SoundGenMethod::Auto);

  ~SoundGenerator();

  /**
   * @brief Stops playing and removes all attached waveform generators
   */
  void clear();

  /**
   * @brief Starts or stops playing
   *
   * @param value True to start playing, false to stop
   *
   * @return The previous playing state
   *
   * Example:
   *
   *     soundGenerator.play(true);
   */
  bool play(bool value);

  /**
   * @brief Plays the specified samples
   *
   * Starts playing the specified samples immediately; there is no need to call play().
   * This method returns without waiting for the sound to end.
   *
   * @param data Samples to play.
   * @param length Number of samples to play.
   * @param volume Volume value. Minimum is 0, maximum is 127.
   * @param durationMS Duration in milliseconds. 0 = until end of samples, -1 = infinite loop.
   *
   * @return Pointer to the SamplesGenerator object. The object lives only for the duration of the sound.
   */
  SamplesGenerator * playSamples(int8_t const * data, int length, int volume = 100, int durationMS = 0);

  /**
   * @brief Plays the specified waveform
   *
   * Starts playing the specified waveform immediately; there is no need to call play().
   * This method returns without waiting for the sound to end.
   *
   * @param waveform Waveform to play.
   * @param frequency Frequency in Hertz.
   * @param durationMS Duration in milliseconds.
   * @param volume Volume value. Minimum is 0, maximum is 127.
   *
   * Example:
   *
   *     // play a sine wave at 500 Hz for 200 milliseconds
   *     soundGen.playSound(SineWaveformGenerator(), 500, 200);
   *
   *     // play a C major chord for 1 second
   *     soundGen.playSound(SineWaveformGenerator(), 262, 1000);  // C
   *     soundGen.playSound(SineWaveformGenerator(), 330, 1000);  // E
   *     soundGen.playSound(SineWaveformGenerator(), 392, 1000);  // G
   */
  template <typename T>
  void playSound(T const & waveform, int frequency, int durationMS, int volume = 100) {
    auto wf = new T(waveform);
    attach(wf);
    wf->setFrequency(frequency);
    wf->setVolume(volume);
    wf->setAutoDestroy(true);
    wf->setDuration(m_sampleRate / 1000 * durationMS);
    wf->enable(true);
    play(true);
  }

  /**
   * @brief Determines whether the sound generator is playing
   *
   * @return True when playing, false otherwise
   */
  bool playing() { return m_play; }

  WaveformGenerator * channels() { return m_channels; }

  /**
   * @brief Attaches a waveform generator
   *
   * @param value Pointer to the waveform generator to attach
   *
   * Example:
   *
   *     SineWaveformGenerator sine;
   *     soundGenerator.attach(&sine);
   *     sine.enable(true);
   *     sine.setFrequency(500);  // 500 Hz
   */
  void attach(WaveformGenerator * value);

  /**
   * @brief Detaches a waveform generator
   *
   * @param value Pointer to the waveform generator to detach
   */
  void detach(WaveformGenerator * value);

  /**
   * @brief Sets the overall volume
   *
   * @param value Volume value. Minimum is 0, maximum is 127.
   */
  void setVolume(int value) { m_volume = value; }

  /**
   * @brief Determines the current overall volume
   *
   * @return Current overall volume (0 = minimum, 127 = maximum)
   */
  int volume() { return m_volume; }

  /**
   * @brief Determines the sample rate
   *
   * @return Sample rate in Hertz
   */
  int sampleRate() { return m_sampleRate; }


private:

  static void ISRHandler(void * arg);     // used in DAC mode
  static void timerHandler(void * args);  // used in sigma-delta mode

  void dac_init();
  void sigmadelta_init();
  void detachNoSuspend(WaveformGenerator * value);
  void setDMANode(int index, volatile uint16_t * buf, int len);
  void init();
  
  int getSample();
  
  #ifdef FABGL_EMULATED
  void sdl_init();
  static void SDLAudioCallback(void * data, Uint8 * buffer, int length);
  #endif


  WaveformGenerator * m_channels;

  uint16_t *          m_sampleBuffer[2];

  int8_t              m_volume;

  uint16_t            m_sampleRate;

  bool                m_play;
  
  gpio_num_t          m_gpio;
  
  intr_handle_t       m_isr_handle;
  
  lldesc_t volatile * m_DMAChain;
  
  SoundGenMethod      m_genMethod;
  
  bool                m_initDone;
  
  esp_timer_handle_t  m_timerHandle;

  #ifndef FABGL_EMULATED
  sdm_channel_handle_t m_sdmChannel;   // used by sigma-delta generator
  #endif
  
  #ifdef FABGL_EMULATED
  SDL_AudioDeviceID   m_device;
  #endif

};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Synthesizer: ADSREnvelope, ResonantLowPassFilter, SynthVoice, PolySynth


/**
 * @brief ADSR envelope parameters (attack, decay, sustain, release), shared by the voices of a synthesizer
 *
 * Times are in milliseconds, sustain is a percentage of the full level. The parameters can be changed while
 * the voices are playing. The envelope is applied by SynthVoice in fixed point (no floating point in the sound
 * generator interrupt).
 *
 * @ingroup audio
 */
class ADSREnvelope {

public:

  /** @brief Full envelope level (fixed point) */
  static constexpr int MAX_LEVEL = 1 << 24;

  ADSREnvelope();

  /**
   * @brief Sets the sample rate (SynthVoice and PolySynth set it automatically)
   *
   * @param value Sample rate in Hertz
   */
  void setSampleRate(int value);

  /**
   * @brief Sets the attack time: from silence to the full level
   *
   * @param ms Time in milliseconds
   */
  void setAttack(int ms)           { m_attackMs = ms; m_attackInc = msToInc(ms); }

  /**
   * @brief Sets the decay time: from the full level to the sustain level
   *
   * @param ms Time in milliseconds (from the full level to silence, when sustain is 0)
   */
  void setDecay(int ms)            { m_decayMs = ms; m_decayInc = msToInc(ms); }

  /**
   * @brief Sets the sustain level, kept while the note is held
   *
   * @param percent Level as a percentage of the full level (0..100)
   */
  void setSustain(int percent)     { m_sustainPercent = percent; m_sustainLevel = (int64_t) MAX_LEVEL * percent / 100; }

  /**
   * @brief Sets the release time: from the full level to silence, after the note is released
   *
   * @param ms Time in milliseconds
   */
  void setRelease(int ms)          { m_releaseMs = ms; m_releaseInc = msToInc(ms); }

  /** @brief Gets the attack time in milliseconds */
  int attack()                     { return m_attackMs; }

  /** @brief Gets the decay time in milliseconds */
  int decay()                      { return m_decayMs; }

  /** @brief Gets the sustain level in percent */
  int sustain()                    { return m_sustainPercent; }

  /** @brief Gets the release time in milliseconds */
  int release()                    { return m_releaseMs; }

  // used by SynthVoice (level increments per sample)
  int attackInc()                  { return m_attackInc; }
  int decayInc()                   { return m_decayInc; }
  int sustainLevel()               { return m_sustainLevel; }
  int releaseInc()                 { return m_releaseInc; }

private:

  int msToInc(int ms);

  int          m_sampleRate;
  int          m_attackMs;
  int          m_decayMs;
  int          m_sustainPercent;
  int          m_releaseMs;
  volatile int m_attackInc;
  volatile int m_decayInc;
  volatile int m_sustainLevel;
  volatile int m_releaseInc;
};



/**
 * @brief Resonant low-pass filter parameters, shared by the voices of a synthesizer
 *
 * Four pole (24 dB/octave) ladder filter, modeled on the classic analog synthesizer filters: four one-pole stages
 * in series with a negative feedback from the output to the input. The feedback creates the resonance peak at the
 * cutoff frequency; at the maximum resonance the filter self-oscillates (it "sings" at the cutoff frequency).
 * A soft saturation of the feedback signal keeps the resonance bounded, like in the analog filters: small signals
 * see the linear filter, while the self-oscillation grows until the saturation limits it (about at full scale).
 *
 * The stages are "zero delay feedback" (topology-preserving transform) one-pole filters: cutoff and resonance stay
 * accurate up to MAX_CUTOFF.
 *
 * Each voice has its own cutoff: the base cutoff (setCutoff()) moved by the voice filter envelope, scaled by the
 * envelope amount (setEnvelopeAmount()). Each voice keeps its own filter state and coefficients
 * (ResonantLowPassFilter::State, ResonantLowPassFilter::Coeffs), interpolated by SynthVoice every few samples from
 * a table of CUTOFF_STEPS cutoff frequencies on a logarithmic scale. The table is calculated in floating point when
 * the sample rate or the resonance change: the sound generator interrupt (no floating point unit there) only
 * interpolates it, with no divisions.
 *
 * @ingroup audio
 */
class ResonantLowPassFilter {

public:

  /** @brief Fractional bits of the coefficients that can be greater than 1 (fb, in, k, comp) */
  static constexpr int COEFF_SHIFT = 28;

  /** @brief Minimum cutoff frequency in Hertz */
  static constexpr int MIN_CUTOFF  = 50;

  /** @brief Maximum cutoff frequency in Hertz (must be lower than half the sample rate) */
  static constexpr int MAX_CUTOFF  = 7000;

  /** @brief Maximum resonance: the filter self-oscillates */
  static constexpr float MAX_RESONANCE = 1.0f;

  /** @brief Number of cutoff frequencies of the coefficients table (logarithmic scale from MIN_CUTOFF to MAX_CUTOFF) */
  static constexpr int CUTOFF_STEPS = 256;

  /** @brief Maximum cutoff index (Q8 fixed point: 8 fractional bits) */
  static constexpr int MAX_CUTOFF_INDEX = (CUTOFF_STEPS - 1) << 8;

  /** @brief Filter coefficients of a voice (fixed point: Q31 when lower than 1, Q28 otherwise) */
  struct Coeffs {
    int32_t g;               // one-pole stage gain G = g / (1 + g) (Q31)
    int32_t s1, s2, s3, s4;  // contribution of the stage states to the output: G^3(1-G), G^2(1-G), G(1-G), 1-G (Q31)
    int32_t fb;              // feedback k / (1 + k G^4) (Q28)
    int32_t in;              // input gain (with passband compensation) / (1 + k G^4) (Q28)
    int32_t g4;              // G^4 (Q31)
    int32_t k;               // feedback k (Q28)
    int32_t comp;            // input gain (with passband compensation) (Q28)
  };

  /** @brief Filter state of one voice (states of the four stages, Q23) */
  struct State {
    int32_t s1 = 0, s2 = 0, s3 = 0, s4 = 0;

    void reset() { s1 = s2 = s3 = s4 = 0; }
  };

  ResonantLowPassFilter();

  /**
   * @brief Sets the sample rate (SynthVoice and PolySynth set it automatically)
   *
   * @param value Sample rate in Hertz
   */
  void setSampleRate(int value);

  /**
   * @brief Sets the cutoff frequency
   *
   * @param hz Frequency in Hertz (MIN_CUTOFF..MAX_CUTOFF)
   */
  void setCutoff(int hz);

  /** @brief Gets the cutoff frequency in Hertz */
  int cutoff()                    { return m_cutoff; }

  /**
   * @brief Sets how much the filter envelope moves the cutoff frequency
   *
   * @param percent -100..100. 100 = at the full envelope level the cutoff moves up by the whole range
   *                (MIN_CUTOFF to MAX_CUTOFF, about 7 octaves). Negative values move it down. 0 = no envelope
   */
  void setEnvelopeAmount(int percent);

  /** @brief Gets the filter envelope amount (-100..100) */
  int envelopeAmount()            { return m_envAmount; }

  /**
   * @brief Sets the resonance
   *
   * @param value Resonance (0..MAX_RESONANCE). 0 = no resonance, MAX_RESONANCE = self-oscillation
   */
  void setResonance(float value);

  /** @brief Gets the resonance (0..MAX_RESONANCE) */
  float resonance()               { return m_resonance; }

  /**
   * @brief Enables or disables (bypasses) the filter
   *
   * @param value True to enable the filter
   */
  void setEnabled(bool value)     { m_enabled = value; }

  /** @brief Determines whether the filter is enabled */
  bool enabled()                  { return m_enabled; }

  /**
   * @brief Calculates the magnitude response of the filter (floating point: do not call from the sound generator interrupt)
   *
   * This is the response of the linear filter (no saturation).
   *
   * @param freq Frequency in Hertz
   *
   * @return Gain at "freq" in dB (very high values when the filter self-oscillates)
   */
  float responseDB(float freq);

  /**
   * @brief Maps a linear position (for example a knob) to a cutoff frequency on a logarithmic scale
   *
   * @param position Position (0..100)
   *
   * @return Cutoff frequency (MIN_CUTOFF..MAX_CUTOFF)
   */
  static int cutoffFromPosition(int position);

  /**
   * @brief Maps a linear position (for example a knob) to a resonance
   *
   * @param position Position (0..100)
   *
   * @return Resonance (0..MAX_RESONANCE)
   */
  static float resonanceFromPosition(int position);

  /**
   * @brief Gets the cutoff index of the base cutoff frequency (used by SynthVoice)
   *
   * @return Index in the cutoff table (0..MAX_CUTOFF_INDEX, Q8)
   */
  int cutoffIndex()               { return m_cutoffIndex; }

  /**
   * @brief Calculates the cutoff index offset produced by the filter envelope (fixed point, used by SynthVoice)
   *
   * @param level Envelope level (0..SynthVoice::LEVEL_ONE)
   *
   * @return Offset to add to cutoffIndex() (Q8)
   */
  int envelopeOffset(int level)   { return (m_envAmountQ8 * level) >> 12; }

  /**
   * @brief Calculates the coefficients for a cutoff (fixed point, used by SynthVoice inside the sound generator interrupt)
   *
   * @param cutoffIndex Index in the cutoff table (0..MAX_CUTOFF_INDEX, Q8: values between two steps are interpolated)
   * @param c Receives the coefficients
   */
  void calcCoeffs(int cutoffIndex, Coeffs & c);

  /**
   * @brief Filters one sample (fixed point, used inside the sound generator interrupt)
   *
   * @param state State of the voice
   * @param c Coefficients of the voice (see calcCoeffs())
   * @param sample Input sample (Q15: -32768..32767)
   *
   * @return Filtered sample (Q15). The resonance can take it beyond the input range: it is not clipped here
   */
  __attribute__((always_inline)) int process(State & state, Coeffs const & c, int sample)
  {
    // 32 bit arithmetic only (high word of the 32x32 products): 64 bit sums need too many registers on the ESP32
    int32_t x = sample << 8;                                  // Q23

    // the output is y4 = G^4 u + S, where S depends only on the stage states. Solving u = comp x - k y4 for u
    // (zero delay feedback) gives the linear solution u = (comp x - k S) / (1 + k G^4) ...
    int32_t S  = (mulHigh(c.s1, state.s1) + mulHigh(c.s2, state.s2) + mulHigh(c.s3, state.s3) + mulHigh(c.s4, state.s4)) << 1;
    int32_t u  = (mulHigh(c.in, x) - mulHigh(c.fb, S)) << 4;
    // ... then the feedback is saturated: u = comp x - k sat(y4). When y4 is in the linear range u doesn't change
    int32_t y4 = (mulHigh(c.g4, u) << 1) + S;
    int32_t sy = saturate(y4);
    if (sy != y4)
      u = (mulHigh(c.comp, x) - mulHigh(c.k, sy)) << 4;

    // four one-pole stages (trapezoidal integrators)
    u = stage(c.g, state.s1, u);
    u = stage(c.g, state.s2, u);
    u = stage(c.g, state.s3, u);
    u = stage(c.g, state.s4, u);

    return iclamp(u >> 8, -(1 << 18), (1 << 18));          // safety net
  }

private:

  // high word of the 64 bit product: a single instruction (mulsh) on the ESP32
  __attribute__((always_inline)) static int32_t mulHigh(int32_t a, int32_t b)
  {
    return (int32_t) (((int64_t) a * b) >> 32);
  }

  __attribute__((always_inline)) static int32_t stage(int32_t g, int32_t & s, int32_t x)
  {
    int32_t v = mulHigh(g, x - s) << 1;                       // g is Q31
    int32_t y = v + s;
    s = y + v;
    return y;
  }

  // soft saturation (Q23): linear up to 1.0, then smoothly bent towards 2.0 (applied to the feedback)
  __attribute__((always_inline)) static int32_t saturate(int32_t x)
  {
    constexpr int32_t K = 1 << 23;
    int32_t a = x < 0 ? -x : x;
    if (a <= K)
      return x;
    int32_t d = a - K;
    int32_t y = K + (int32_t) (((int64_t) K * d) / (K + d));
    return x < 0 ? -y : y;
  }

  void buildTable();
  void updateResonance();

  // coefficients that depend only on the cutoff
  // coefficients for each cutoff of the table (fb and in depend also on the resonance)
  struct Step {
    int32_t g, s1, s2, s3, s4, g4, fb, in;
  };

  Step          m_table[CUTOFF_STEPS];
  volatile bool m_enabled;
  int           m_sampleRate;
  int           m_cutoff;
  volatile int  m_cutoffIndex;
  float         m_resonance;
  volatile int  m_k;              // feedback (Q28)
  volatile int  m_comp;           // input gain (Q28)
  int           m_envAmount;
  volatile int  m_envAmountQ8;    // cutoff index offset at the full envelope level (Q8)
};



/** \ingroup Enumerations
 * @brief Waveforms of SynthVoice
 */
enum class SynthWaveform {
  Sine,       /**< Sine */
  Square,     /**< Square (50% duty cycle) */
  Pulse,      /**< Pulse (25% duty cycle) */
  Triangle,   /**< Triangle */
  Sawtooth,   /**< Sawtooth */
};


/** \ingroup Enumerations
 * @brief Waveforms of SynthLFO
 */
enum class LFOWaveform {
  Triangle,   /**< Triangle */
  Sine,       /**< Sine */
  Square,     /**< Square */
  Sawtooth,   /**< Sawtooth (rising) */
  Random,     /**< Random values held for one cycle (sample and hold) */
};


/**
 * @brief Low frequency oscillator of a synthesizer: modulates the filter cutoff (wah) and the amplitude (tremolo)
 *
 * One LFO is shared by all the voices of PolySynth: it is free running (not restarted by the notes).
 * The parameters can be changed while the voices are playing. The LFO runs in fixed point inside the sound
 * generator interrupt.
 *
 * @ingroup audio
 */
class SynthLFO {

public:

  /** @brief Minimum rate in Hertz */
  static constexpr float MIN_RATE = 0.1f;

  /** @brief Maximum rate in Hertz */
  static constexpr float MAX_RATE = 20.0f;

  SynthLFO();

  /**
   * @brief Sets the sample rate (PolySynth sets it automatically)
   *
   * @param value Sample rate in Hertz
   */
  void setSampleRate(int value);

  /**
   * @brief Selects the waveform
   *
   * @param value The waveform
   */
  void setWaveform(LFOWaveform value)   { m_waveform = value; }

  /** @brief Gets the waveform */
  LFOWaveform waveform()                { return m_waveform; }

  /**
   * @brief Sets the rate (frequency)
   *
   * @param hz Rate in Hertz (MIN_RATE..MAX_RATE)
   */
  void setRate(float hz);

  /** @brief Gets the rate in Hertz */
  float rate()                          { return m_rate; }

  /**
   * @brief Sets how much the LFO moves the filter cutoff
   *
   * @param percent 0..100. 100 = the cutoff moves up and down by half the cutoff range (about 3.5 octaves)
   */
  void setCutoffDepth(int percent);

  /** @brief Gets the cutoff modulation depth (0..100) */
  int cutoffDepth()                     { return m_cutoffDepth; }

  /**
   * @brief Sets how much the LFO changes the amplitude (tremolo)
   *
   * @param percent 0..100. 100 = the amplitude goes from full level down to silence
   */
  void setAmpDepth(int percent);

  /** @brief Gets the amplitude modulation depth (0..100) */
  int ampDepth()                        { return m_ampDepth; }

  /**
   * @brief Advances the LFO (fixed point, used by PolySynth inside the sound generator interrupt)
   *
   * @param samples Number of samples
   */
  void advance(int samples = 1);

  /**
   * @brief Gets the current LFO value
   *
   * @return Value in Q15 (-32767..32767)
   */
  int value()                           { return m_value; }

  /**
   * @brief Gets the current cutoff modulation (fixed point, used by SynthVoice)
   *
   * @return Offset to add to the cutoff index (Q8, see ResonantLowPassFilter::calcCoeffs())
   */
  int cutoffOffset()                    { return (m_value * m_cutoffDepthQ8) >> 15; }

  /**
   * @brief Gets the current amplitude modulation (fixed point, used by PolySynth)
   *
   * @return Gain (0..4096 = 1.0)
   */
  int ampGain()                         { return 4096 - ((m_ampDepthQ12 * (32767 - m_value)) >> 16); }

  /**
   * @brief Maps a linear position (for example a knob) to a rate on a logarithmic scale
   *
   * @param position Position (0..100)
   *
   * @return Rate in Hertz (MIN_RATE..MAX_RATE)
   */
  static float rateFromPosition(int position);

  /**
   * @brief Calculates the shape of a waveform, for display purposes (not for the sound generator interrupt)
   *
   * Random returns a fixed pattern of steps.
   *
   * @param waveform The waveform
   * @param phase Phase in a cycle (0..255)
   *
   * @return Sample value (-127..127)
   */
  static int waveformShape(LFOWaveform waveform, int phase);

  /**
   * @brief Gets the name of a waveform
   *
   * @param waveform The waveform
   *
   * @return Name in upper case (for example "TRIANGLE")
   */
  static char const * waveformName(LFOWaveform waveform);

private:

  int                    m_sampleRate;
  volatile LFOWaveform   m_waveform;
  float                  m_rate;
  uint32_t               m_phase;
  volatile uint32_t      m_phaseInc;
  int                    m_value;
  int                    m_randomValue;
  uint32_t               m_randomSeed;
  int                    m_cutoffDepth;
  volatile int           m_cutoffDepthQ8;
  int                    m_ampDepth;
  volatile int           m_ampDepthQ12;
};



/** \ingroup Enumerations
 * @brief States of SynthVoice envelope
 */
enum class SynthVoiceState {
  Idle,       /**< Not playing */
  Attack,     /**< Attack phase */
  Decay,      /**< Decay phase */
  Sustain,    /**< Sustain phase (note held) */
  Release,    /**< Release phase (note released) */
};


/**
 * @brief A synthesizer voice: a band-limited oscillator followed by a resonant low-pass filter, shaped by an ADSR envelope
 *
 * A second ADSR envelope (the filter envelope) and the LFO move the cutoff frequency of the voice filter.
 * The oscillator runs at 16 bit resolution with a 32 bit phase accumulator. Square, pulse and sawtooth waveforms are
 * band-limited (PolyBLEP): their discontinuities are smoothed, so the harmonics above half the sample rate don't
 * fold back as unwanted (non harmonic) tones.
 * The envelopes and the filter parameters are shared objects (see setup()). Usually voices are managed by PolySynth,
 * which mixes them into one SoundGenerator channel.
 *
 * @ingroup audio
 */
class SynthVoice {

public:

  SynthVoice();

  /**
   * @brief Sets the sample rate (PolySynth sets it automatically)
   *
   * @param value Sample rate in Hertz
   */
  void setSampleRate(int value);

  /**
   * @brief Sets the shared envelopes and filter parameters
   *
   * @param envelope Amplitude envelope parameters
   * @param filter Filter parameters (nullptr = no filter)
   * @param filterEnvelope Filter envelope parameters (nullptr = no filter envelope)
   * @param lfo LFO that modulates the filter cutoff (nullptr = no LFO)
   */
  void setup(ADSREnvelope * envelope, ResonantLowPassFilter * filter, ADSREnvelope * filterEnvelope = nullptr, SynthLFO * lfo = nullptr);

  /**
   * @brief Selects the oscillator waveform. It can be changed while the voice is playing
   *
   * @param waveform The waveform
   */
  void setWaveform(SynthWaveform waveform)   { m_waveform = waveform; }

  /**
   * @brief Sets the oscillator frequency
   *
   * @param value Frequency in Hertz
   */
  void setFrequency(float value);

  /**
   * @brief Starts a note (attack phase). A voice still playing restarts from its current level
   *
   * @param note MIDI note number (60 = C4, 69 = A4 = 440 Hz)
   */
  void noteOn(int note);

  /**
   * @brief Releases the note (release phase)
   */
  void noteOff();

  /**
   * @brief Gets the playing note
   *
   * @return MIDI note number, -1 when the voice is idle
   */
  int note()                { return m_note; }

  /**
   * @brief Gets the envelope state
   *
   * @return The envelope state
   */
  SynthVoiceState state()   { return m_state; }

  /**
   * @brief Determines whether the voice is idle (not playing)
   *
   * @return True if the voice is idle
   */
  bool isIdle()             { return m_state == SynthVoiceState::Idle; }

  /**
   * @brief Calculates the next samples and adds them to a buffer (fixed point, used inside the sound generator interrupt)
   *
   * Advances envelopes, oscillator and filter. The filter coefficients are recalculated every CONTROL_PERIOD
   * samples: PolySynth renders blocks of CONTROL_PERIOD samples. Rendering blocks avoids a chain of function calls
   * for every sample, which is expensive on the ESP32 (register window spills).
   *
   * @param buffer Samples are added to this buffer: Q15 (-32768..32767 at the full envelope level; the filter
   *               resonance can exceed this range)
   * @param count Number of samples
   */
  void render(int32_t * buffer, int count);

  /**
   * @brief Calculates the next sample (fixed point, used inside the sound generator interrupt). See render()
   *
   * @return Sample in Q15
   */
  int getSample();

  /** @brief The filter coefficients are recalculated every CONTROL_PERIOD samples */
  static constexpr int CONTROL_PERIOD = 16;

  /**
   * @brief Gets the current level of the amplitude envelope
   *
   * @return Level (0..LEVEL_ONE)
   */
  int level()               { return m_level >> LEVEL_SHIFT; }

  /**
   * @brief Gets the current level of the filter envelope
   *
   * @return Level (0..LEVEL_ONE)
   */
  int filterEnvelopeLevel() { return m_filterLevel >> LEVEL_SHIFT; }

  /** @brief Envelope level returned by level() at the full level */
  static constexpr int LEVEL_ONE = 4096;

  /**
   * @brief Calculates the shape of a waveform, for display purposes (uses floating point: not for the sound generator interrupt)
   *
   * @param waveform The waveform
   * @param phase Phase in a cycle (0..255)
   *
   * @return Sample value (-127..127)
   */
  static int waveformShape(SynthWaveform waveform, int phase);

  /**
   * @brief Gets the name of a waveform
   *
   * @param waveform The waveform
   *
   * @return Name in upper case (for example "SAWTOOTH")
   */
  static char const * waveformName(SynthWaveform waveform);

private:

  static constexpr int LEVEL_SHIFT = 12;   // ADSREnvelope::MAX_LEVEL (1 << 24) >> 12 = LEVEL_ONE

  template <int WAVEFORM, bool FILTER> void renderLoop(int32_t * buffer, int count);

  ADSREnvelope *                 m_envelope;
  ResonantLowPassFilter *        m_filter;
  ADSREnvelope *                 m_filterEnvelope;
  SynthLFO *                     m_lfo;
  ResonantLowPassFilter::State   m_filterState;
  ResonantLowPassFilter::Coeffs  m_filterCoeffs;
  int                            m_controlCounter;
  volatile SynthVoiceState       m_filterEnvState;
  int                            m_filterLevel;
  int                            m_sampleRate;
  volatile SynthWaveform         m_waveform;
  uint32_t                       m_phase;
  volatile uint32_t              m_phaseInc;
  int                            m_note;
  volatile SynthVoiceState       m_state;
  int                            m_level;
};



/**
 * @brief A polyphonic synthesizer: a pool of SynthVoice objects sharing the envelopes and the filter parameters
 *
 * The voices are mixed (at 16 bit resolution) into a single SoundGenerator channel. The mix is scaled by an
 * automatic gain of 1 / sqrt(sum of the envelope levels), so the loudness stays about the same when more notes
 * play together, and the gain changes smoothly when notes start or end. A soft limiter bends the peaks that
 * exceed the headroom (a single note uses 3/4 of the full scale) instead of clipping them.
 * When all voices are busy a new note steals a releasing voice, otherwise the oldest one.
 *
 * Example:
 *
 *     SoundGenerator soundGenerator;
 *     PolySynth      synth;
 *
 *     synth.begin(&soundGenerator);
 *     synth.envelope().setRelease(500);
 *     synth.filter().setCutoff(1000);
 *     synth.filter().setEnvelopeAmount(40);
 *     synth.filterEnvelope().setDecay(400);
 *     synth.lfo().setRate(5.0f);
 *     synth.lfo().setCutoffDepth(20);
 *     synth.noteOn(60);    // C4
 *     ...
 *     synth.noteOff(60);
 *
 * @ingroup audio
 */
class PolySynth {

public:

  /**
   * @brief Creates the synthesizer
   *
   * @param voices Number of voices (maximum number of notes playing at the same time)
   */
  PolySynth(int voices = 8);

  ~PolySynth();

  /**
   * @brief Attaches the synthesizer to the sound generator and starts playing
   *
   * @param soundGenerator The sound generator
   */
  void begin(SoundGenerator * soundGenerator);

  /**
   * @brief Selects the waveform of all voices
   *
   * @param value The waveform
   */
  void setWaveform(SynthWaveform value);

  /** @brief Gets the waveform of the voices */
  SynthWaveform waveform()          { return m_waveform; }

  /**
   * @brief Starts playing a note
   *
   * @param note MIDI note number (60 = C4, 69 = A4 = 440 Hz)
   */
  void noteOn(int note);

  /**
   * @brief Releases a note
   *
   * @param note MIDI note number
   */
  void noteOff(int note);

  /**
   * @brief Releases all notes
   */
  void allNotesOff();

  /** @brief Gets the amplitude envelope parameters, shared by all voices */
  ADSREnvelope & envelope()         { return m_envelope; }

  /**
   * @brief Gets the filter envelope parameters, shared by all voices
   *
   * The filter envelope moves the cutoff frequency, by the amount set with ResonantLowPassFilter::setEnvelopeAmount()
   */
  ADSREnvelope & filterEnvelope()   { return m_filterEnvelope; }

  /**
   * @brief Gets the LFO, shared by all voices: it modulates the filter cutoff and the amplitude
   */
  SynthLFO & lfo()                  { return m_lfo; }

  /** @brief Gets the filter parameters, shared by all voices */
  ResonantLowPassFilter & filter()  { return m_filter; }

  /** @brief Gets the number of voices */
  int voicesCount()                 { return m_voicesCount; }

  /**
   * @brief Gets a voice
   *
   * @param index Voice index (0..voicesCount()-1)
   *
   * @return The voice
   */
  SynthVoice & voice(int index)     { return m_voices[index]; }

  /**
   * @brief Gets the sound generator channel that plays the mix of the voices
   *
   * Its volume (WaveformGenerator::setVolume()) is the synthesizer volume.
   *
   * @return The channel
   */
  WaveformGenerator & channel()     { return m_channel; }

  /**
   * @brief Calculates the frequency of a note (equal temperament, A4 = 440 Hz)
   *
   * @param note MIDI note number
   *
   * @return Frequency in Hertz (rounded)
   */
  static int noteFrequency(int note);

private:

  // samples rendered at once (see SynthVoice::render())
  static constexpr int BLOCK_SIZE = SynthVoice::CONTROL_PERIOD;

  // the SoundGenerator channel: plays the mix of the voices, rendered in blocks
  class Channel : public WaveformGenerator {
  public:
    Channel(PolySynth * synth) : m_synth(synth), m_pos(BLOCK_SIZE) { }
    int getSample()
    {
      if (m_pos >= BLOCK_SIZE) {
        m_synth->renderBlock(m_block);
        m_pos = 0;
      }
      return m_block[m_pos++];
    }
    void setFrequency(int value)    { }
  private:
    PolySynth * m_synth;
    int8_t      m_block[BLOCK_SIZE];
    int         m_pos;
  };

  void renderBlock(int8_t * dest);

  SoundGenerator *      m_soundGenerator;
  ADSREnvelope          m_envelope;
  ADSREnvelope          m_filterEnvelope;
  ResonantLowPassFilter m_filter;
  SynthLFO              m_lfo;
  SynthVoice *          m_voices;
  int                   m_voicesCount;
  SynthWaveform         m_waveform;
  int                   m_nextSteal;
  Channel               m_channel;
  int                   m_gain;
  int                   m_lfoGain;
};




} // end of namespace
