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
 * Second order (12 dB/octave) "RBJ cookbook" biquad. The coefficients are calculated in floating point when a
 * parameter changes and used in fixed point (Q28) by SynthVoice inside the sound generator interrupt.
 * Two coefficient sets are kept: a change writes the inactive set and then swaps them, so the interrupt never
 * sees a half-updated set. Each voice keeps its own filter state (ResonantLowPassFilter::State).
 *
 * @ingroup audio
 */
class ResonantLowPassFilter {

public:

  /** @brief Fractional bits of the coefficients */
  static constexpr int COEFF_SHIFT = 28;

  /** @brief Minimum cutoff frequency in Hertz */
  static constexpr int MIN_CUTOFF  = 50;

  /** @brief Maximum cutoff frequency in Hertz (must be lower than half the sample rate) */
  static constexpr int MAX_CUTOFF  = 7000;

  /** @brief Minimum resonance (Q): no resonance (Butterworth) */
  static constexpr float MIN_Q     = 0.707f;

  /** @brief Maximum resonance (Q) */
  static constexpr float MAX_Q     = 12.0f;

  /** @brief Biquad coefficients (Q28 fixed point) */
  struct Coeffs {
    int32_t b0, b1, b2, a1, a2;
  };

  /** @brief Filter state of one voice */
  struct State {
    int32_t x1 = 0, x2 = 0, y1 = 0, y2 = 0;

    void reset() { x1 = x2 = y1 = y2 = 0; }
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
   * @brief Sets the resonance
   *
   * @param q Quality factor (MIN_Q..MAX_Q). MIN_Q = no resonance
   */
  void setResonance(float q);

  /** @brief Gets the resonance (quality factor) */
  float resonance()               { return m_q; }

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
   * @param freq Frequency in Hertz
   *
   * @return Gain at "freq" in dB
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
   * @brief Maps a linear position (for example a knob) to a resonance on a logarithmic scale
   *
   * @param position Position (0..100)
   *
   * @return Resonance (MIN_Q..MAX_Q)
   */
  static float resonanceFromPosition(int position);

  /**
   * @brief Filters one sample (fixed point, used inside the sound generator interrupt)
   *
   * @param state State of the voice
   * @param sample Input sample (-128..127)
   *
   * @return Filtered sample (-128..127, clipped)
   */
  int process(State & state, int sample)
  {
    // direct form I. Samples are scaled by 256 to keep precision.
    auto const & c = m_sets[m_active];
    int32_t x0 = sample << 8;
    int64_t acc = (int64_t) c.b0 * x0 + (int64_t) c.b1 * state.x1 + (int64_t) c.b2 * state.x2
                - (int64_t) c.a1 * state.y1 - (int64_t) c.a2 * state.y2;
    int32_t y0 = (int32_t) (acc >> COEFF_SHIFT);
    y0 = iclamp(y0, -(1 << 20), (1 << 20));        // safety net against runaway (never reached when stable)
    state.x2 = state.x1;
    state.x1 = x0;
    state.y2 = state.y1;
    state.y1 = y0;
    return iclamp(y0 >> 8, -128, 127);             // resonance can exceed the 8 bit range: clip
  }

private:

  void update();

  Coeffs        m_sets[2];
  volatile int  m_active;
  volatile bool m_enabled;
  int           m_sampleRate;
  int           m_cutoff;
  float         m_q;
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
 * @brief A synthesizer voice: an oscillator followed by a resonant low-pass filter, shaped by an ADSR envelope
 *
 * The envelope and the filter parameters are shared objects (see setup()). Usually voices are managed by PolySynth.
 * A voice must be attached to the SoundGenerator before setWaveform() is called.
 *
 * @ingroup audio
 */
class SynthVoice : public WaveformGenerator {

public:

  SynthVoice();

  /**
   * @brief Sets the shared envelope and filter parameters
   *
   * @param envelope Envelope parameters
   * @param filter Filter parameters (nullptr = no filter)
   */
  void setup(ADSREnvelope * envelope, ResonantLowPassFilter * filter);

  /**
   * @brief Selects the oscillator waveform
   *
   * Must be called after the voice has been attached to the sound generator (when the sample rate is known).
   * Stop the sound generator while changing the waveform of a playing voice.
   *
   * @param waveform The waveform
   */
  void setWaveform(SynthWaveform waveform);

  void setFrequency(int value);

  /**
   * @brief Starts a note (attack phase). A voice still playing restarts from its current level
   *
   * @param note MIDI note number (used by note())
   * @param frequency Frequency in Hertz
   */
  void noteOn(int note, int frequency);

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

  int getSample();

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

  SineWaveformGenerator          m_sine;
  SquareWaveformGenerator        m_square;
  SquareWaveformGenerator        m_pulse;
  TriangleWaveformGenerator      m_triangle;
  SawtoothWaveformGenerator      m_sawtooth;

  WaveformGenerator *            m_gen;
  ADSREnvelope *                 m_envelope;
  ResonantLowPassFilter *        m_filter;
  ResonantLowPassFilter::State   m_filterState;
  int                            m_frequency;
  int                            m_note;
  volatile SynthVoiceState       m_state;
  int                            m_level;
};



/**
 * @brief A polyphonic synthesizer: a pool of SynthVoice objects sharing one envelope and one filter
 *
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
   * @brief Attaches the voices to the sound generator and starts playing
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

  /** @brief Gets the envelope parameters, shared by all voices */
  ADSREnvelope & envelope()         { return m_envelope; }

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
   * @brief Calculates the frequency of a note (equal temperament, A4 = 440 Hz)
   *
   * @param note MIDI note number
   *
   * @return Frequency in Hertz
   */
  static int noteFrequency(int note);

private:

  SoundGenerator *      m_soundGenerator;
  ADSREnvelope          m_envelope;
  ResonantLowPassFilter m_filter;
  SynthVoice *          m_voices;
  int                   m_voicesCount;
  SynthWaveform         m_waveform;
  int                   m_nextSteal;
};




} // end of namespace
