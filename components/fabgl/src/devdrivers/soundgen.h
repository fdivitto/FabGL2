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




} // end of namespace

