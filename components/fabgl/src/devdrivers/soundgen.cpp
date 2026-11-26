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




#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "soc/i2s_reg.h"
#include "esp_private/periph_ctrl.h"
#include "soc/rtc.h"
#include <soc/sens_reg.h>
#include "esp_log.h"
#include "driver/sdm.h"


#include "soundgen.h"


#pragma GCC optimize ("O2")


namespace fabgl {



////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SineWaveformGenerator


static const int8_t sinTable[257] = {
   0,    3,    6,    9,   12,   16,   19,   22,   25,   28,   31,   34,   37,   40,   43,   46,
  49,   51,   54,   57,   60,   63,   65,   68,   71,   73,   76,   78,   81,   83,   85,   88,
  90,   92,   94,   96,   98,  100,  102,  104,  106,  107,  109,  111,  112,  113,  115,  116,
 117,  118,  120,  121,  122,  122,  123,  124,  125,  125,  126,  126,  126,  127,  127,  127,
 127,  127,  127,  127,  126,  126,  126,  125,  125,  124,  123,  122,  122,  121,  120,  118,
 117,  116,  115,  113,  112,  111,  109,  107,  106,  104,  102,  100,   98,   96,   94,   92,
  90,   88,   85,   83,   81,   78,   76,   73,   71,   68,   65,   63,   60,   57,   54,   51,
  49,   46,   43,   40,   37,   34,   31,   28,   25,   22,   19,   16,   12,    9,    6,    3,
   0,   -3,   -6,   -9,  -12,  -16,  -19,  -22,  -25,  -28,  -31,  -34,  -37,  -40,  -43,  -46,
 -49,  -51,  -54,  -57,  -60,  -63,  -65,  -68,  -71,  -73,  -76,  -78,  -81,  -83,  -85,  -88,
 -90,  -92,  -94,  -96,  -98, -100, -102, -104, -106, -107, -109, -111, -112, -113, -115, -116,
-117, -118, -120, -121, -122, -122, -123, -124, -125, -125, -126, -126, -126, -127, -127, -127,
-127, -127, -127, -127, -126, -126, -126, -125, -125, -124, -123, -122, -122, -121, -120, -118,
-117, -116, -115, -113, -112, -111, -109, -107, -106, -104, -102, -100,  -98,  -96,  -94,  -92,
 -90,  -88,  -85,  -83,  -81,  -78,  -76,  -73,  -71,  -68,  -65,  -63,  -60,  -57,  -54,  -51,
 -49,  -46,  -43,  -40,  -37,  -34,  -31,  -28,  -25,  -22,  -19,  -16,  -12,   -9,   -6,   -3,
   0,
};


SineWaveformGenerator::SineWaveformGenerator()
 : m_phaseInc(0),
   m_phaseAcc(0),
   m_frequency(0),
   m_lastSample(0)
{
}


void SineWaveformGenerator::setFrequency(int value) {
  if (m_frequency != value) {
    m_frequency = value;
    m_phaseInc = (((uint32_t)m_frequency * 256) << 11) / sampleRate();
  }
}


int SineWaveformGenerator::getSample() {
  if (m_frequency == 0 || duration() == 0) {
    if (m_lastSample > 0)
      --m_lastSample;
    else if (m_lastSample < 0)
      ++m_lastSample;
    else
      m_phaseAcc = 0;
    return m_lastSample;
  }

  // get sample  (-128...+127)
  uint32_t index = m_phaseAcc >> 11;
  int sample = sinTable[index];

  // process volume
  sample = sample * volume() / 127;

  m_lastSample = sample;

  m_phaseAcc = (m_phaseAcc + m_phaseInc) & 0x7ffff;

  decDuration();

  return sample;
}


// SineWaveformGenerator
////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SquareWaveformGenerator


SquareWaveformGenerator::SquareWaveformGenerator()
  : m_phaseInc(0),
    m_phaseAcc(0),
    m_frequency(0),
    m_lastSample(0),
    m_dutyCycle(127)
{
}


void SquareWaveformGenerator::setFrequency(int value) {
  if (m_frequency != value) {
    m_frequency = value;
    m_phaseInc = (((uint32_t)m_frequency * 256) << 11) / sampleRate();
  }
}


// dutyCycle: 0..255 (255=100%)
void SquareWaveformGenerator::setDutyCycle(int dutyCycle)
{
  m_dutyCycle = dutyCycle;
}


int SquareWaveformGenerator::getSample() {
  if (m_frequency == 0 || duration() == 0) {
    if (m_lastSample > 0)
      --m_lastSample;
    else if (m_lastSample < 0)
      ++m_lastSample;
    else
      m_phaseAcc = 0;
    return m_lastSample;
  }

  uint32_t index = m_phaseAcc >> 11;
  int sample = (index <= m_dutyCycle ? 127 : -127);

  // process volume
  sample = sample * volume() / 127;

  m_lastSample = sample;

  m_phaseAcc = (m_phaseAcc + m_phaseInc) & 0x7ffff;

  decDuration();

  return sample;
}

// SquareWaveformGenerator
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TriangleWaveformGenerator


TriangleWaveformGenerator::TriangleWaveformGenerator()
  : m_phaseInc(0),
    m_phaseAcc(0),
    m_frequency(0),
    m_lastSample(0)
{
}


void TriangleWaveformGenerator::setFrequency(int value) {
  if (m_frequency != value) {
    m_frequency = value;
    m_phaseInc = (((uint32_t)m_frequency * 256) << 11) / sampleRate();
  }
}


int TriangleWaveformGenerator::getSample() {
  if (m_frequency == 0 || duration() == 0) {
    if (m_lastSample > 0)
      --m_lastSample;
    else if (m_lastSample < 0)
      ++m_lastSample;
    else
      m_phaseAcc = 0;
    return m_lastSample;
  }

  uint32_t index = m_phaseAcc >> 11;
  int sample = (index & 0x80 ? -1 : 1) * ((index & 0x3F) * 2 - (index & 0x40 ? 0 : 127));

  // process volume
  sample = sample * volume() / 127;

  m_lastSample = sample;

  m_phaseAcc = (m_phaseAcc + m_phaseInc) & 0x7ffff;

  decDuration();

  return sample;
}

// TriangleWaveformGenerator
////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SawtoothWaveformGenerator


SawtoothWaveformGenerator::SawtoothWaveformGenerator()
  : m_phaseInc(0),
    m_phaseAcc(0),
    m_frequency(0),
    m_lastSample(0)
{
}


void SawtoothWaveformGenerator::setFrequency(int value) {
  if (m_frequency != value) {
    m_frequency = value;
    m_phaseInc = (((uint32_t)m_frequency * 256) << 11) / sampleRate();
  }
}


int SawtoothWaveformGenerator::getSample() {
  if (m_frequency == 0 || duration() == 0) {
    if (m_lastSample > 0)
      --m_lastSample;
    else if (m_lastSample < 0)
      ++m_lastSample;
    else
      m_phaseAcc = 0;
    return m_lastSample;
  }

  uint32_t index = m_phaseAcc >> 11;
  int sample = index - 128;

  // process volume
  sample = sample * volume() / 127;

  m_lastSample = sample;

  m_phaseAcc = (m_phaseAcc + m_phaseInc) & 0x7ffff;

  decDuration();

  return sample;
}

// TriangleWaveformGenerator
////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NoiseWaveformGenerator


NoiseWaveformGenerator::NoiseWaveformGenerator()
  : m_noise(0xFAB7)
{
}


void NoiseWaveformGenerator::setFrequency(int value)
{
}


int NoiseWaveformGenerator::getSample()
{
  if (duration() == 0) {
    return 0;
  }

  // noise generator based on Galois LFSR
  m_noise = (m_noise >> 1) ^ (-(m_noise & 1) & 0xB400u);
  int sample = 127 - (m_noise >> 8);

  // process volume
  sample = sample * volume() / 127;

  decDuration();

  return sample;
}


// NoiseWaveformGenerator
////////////////////////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////////////////////
// VICNoiseGenerator
// "tries" to emulate VIC6561 noise generator
// derived from a reverse engineering VHDL code: http://sleepingelephant.com/ipw-web/bulletin/bb/viewtopic.php?f=11&t=8733&fbclid=IwAR16x6OMMb670bA2ZtYcbB0Zat_X-oHNB0NxxYigPffXa8G_InSIoNAEiPU


VICNoiseGenerator::VICNoiseGenerator()
  : m_frequency(0),
    m_counter(0),
    m_LFSR(LFSRINIT),
    m_outSR(0)
{
}


void VICNoiseGenerator::setFrequency(int value)
{
  if (m_frequency != value) {
    m_frequency = value >= 127 ? 0 : value;
    m_LFSR      = LFSRINIT;
    m_counter   = 0;
    m_outSR     = 0;
  }
}


int VICNoiseGenerator::getSample()
{
  if (duration() == 0) {
    return 0;
  }

  const int reduc = CLK / 8 / sampleRate(); // resample to sampleRate() (e.g. 16000Hz)

  int sample = 0;

  for (int i = 0; i < reduc; ++i) {

    if (m_counter >= 127) {

      // reset counter
      m_counter = m_frequency;

      if (m_LFSR & 1)
        m_outSR = ((m_outSR << 1) | ~(m_outSR >> 7));

      m_LFSR <<= 1;
      int bit3  = (m_LFSR >> 3) & 1;
      int bit12 = (m_LFSR >> 12) & 1;
      int bit14 = (m_LFSR >> 14) & 1;
      int bit15 = (m_LFSR >> 15) & 1;
      m_LFSR |= (bit3 ^ bit12) ^ (bit14 ^ bit15);
    } else
      ++m_counter;

    sample += m_outSR & 1 ? 127 : -128;
  }

  // simple mean of all samples

  sample = sample / reduc;

  // process volume
  sample = sample * volume() / 127;

  decDuration();

  return sample;
}


// VICNoiseGenerator
/////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SamplesGenerator


SamplesGenerator::SamplesGenerator(int8_t const * data, int length)
  : m_data(data),
    m_length(length),
    m_index(0)
{
}


void SamplesGenerator::setFrequency(int value)
{
}


int SamplesGenerator::getSample() {

  if (duration() == 0) {
    return 0;
  }

  int sample = m_data[m_index++];

  if (m_index == m_length)
    m_index = 0;

  // process volume
  sample = sample * volume() / 127;

  decDuration();

  return sample;
}


// SamplesGenerator
////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SoundGenerator


SoundGenerator::SoundGenerator(int sampleRate, gpio_num_t gpio, SoundGenMethod genMethod)
  : m_channels(nullptr),
    m_sampleBuffer{0},
    m_volume(100),
    m_sampleRate(sampleRate),
    m_play(false),
    m_gpio(gpio),
    m_isr_handle(nullptr),
    m_DMAChain(nullptr),
    m_genMethod(genMethod),
    m_initDone(false),
    m_timerHandle(nullptr)
    #ifndef FABGL_EMULATED
    , m_sdmChannel(nullptr)
    #endif
{
}


SoundGenerator::~SoundGenerator()
{
  clear();
    
  if (m_isr_handle) {
    // cleanup DAC mode
    I2S_moduleDisable(0);
    esp_intr_free(m_isr_handle);
    for (int i = 0; i < 2; ++i)
      heap_caps_free(m_sampleBuffer[i]);
    heap_caps_free((void*)m_DMAChain);
  }
  
  if (m_timerHandle) {
    // cleanup sigmadelta mode
    esp_timer_stop(m_timerHandle);
    esp_timer_delete(m_timerHandle);
    m_timerHandle = nullptr;
  }

  #ifndef FABGL_EMULATED
  if (m_sdmChannel) {
    sdm_channel_disable(m_sdmChannel);
    sdm_del_channel(m_sdmChannel);
    m_sdmChannel = nullptr;
  }
  #endif
  
  #ifdef FABGL_EMULATED
  SDL_CloseAudioDevice(m_device);
  #endif
    
}


void SoundGenerator::clear()
{
  play(false);
  m_channels = nullptr;
}


void SoundGenerator::setDMANode(int index, volatile uint16_t * buf, int len)
{
  m_DMAChain[index].eof          = 1; // always generate interrupt
  m_DMAChain[index].sosf         = 0;
  m_DMAChain[index].owner        = 1;
  m_DMAChain[index].qe.stqe_next = (lldesc_t *) (m_DMAChain + index + 1);
  m_DMAChain[index].offset       = 0;
  m_DMAChain[index].size         = len * sizeof(uint16_t);
  m_DMAChain[index].length       = len * sizeof(uint16_t);
  m_DMAChain[index].buf          = (uint8_t*) buf;
}


void SoundGenerator::dac_init()
{
  #ifndef FABGL_EMULATED
  m_DMAChain = (volatile lldesc_t *) heap_caps_malloc(2 * sizeof(lldesc_t), MALLOC_CAP_DMA);
  
  for (int i = 0; i < 2; ++i) {
    m_sampleBuffer[i] = (uint16_t *) heap_caps_malloc(FABGL_SOUNDGEN_SAMPLE_BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA);
    for (int j = 0; j < FABGL_SOUNDGEN_SAMPLE_BUFFER_SIZE; ++j)
      m_sampleBuffer[i][j] = 0x7f00;
    setDMANode(i, m_sampleBuffer[i], FABGL_SOUNDGEN_SAMPLE_BUFFER_SIZE);
  }
  m_DMAChain[1].sosf = 1;
  m_DMAChain[1].qe.stqe_next = (lldesc_t *) m_DMAChain; // closes DMA chain
    
  I2S_moduleEnable(0);

  // Initialize I2S device
  I2S0.conf.tx_reset                     = 1;
  I2S0.conf.tx_reset                     = 0;

  // Reset DMA
  I2S0.lc_conf.in_rst                    = 1;
  I2S0.lc_conf.in_rst                    = 0;

  // Reset FIFO
  I2S0.conf.rx_fifo_reset                = 1;
  I2S0.conf.rx_fifo_reset                = 0;
  
  I2S0.conf_chan.tx_chan_mod = (m_gpio == GPIO_NUM_25 ? 3 : 4);

  I2S0.fifo_conf.tx_fifo_mod_force_en    = 1;
  I2S0.fifo_conf.tx_fifo_mod             = 1;
  I2S0.fifo_conf.dscr_en                 = 1;

  I2S0.conf.tx_mono                      = 1;
  I2S0.conf.tx_start                     = 0;
  I2S0.conf.tx_msb_right                 = 1;
  I2S0.conf.tx_right_first               = 1;
  I2S0.conf.tx_slave_mod                 = 0;
  I2S0.conf.tx_short_sync                = 0;
  I2S0.conf.tx_msb_shift                 = 0;
  
  I2S0.conf2.lcd_en                      = 1;
  I2S0.conf2.camera_en                   = 0;

  int a, b, num, m;
  m_sampleRate = calcI2STimingParams(m_sampleRate, &a, &b, &num, &m);
  I2S0.clkm_conf.clka_en                 = 0;
  I2S0.clkm_conf.clkm_div_a              = a;
  I2S0.clkm_conf.clkm_div_b              = b;
  I2S0.clkm_conf.clkm_div_num            = num;
  I2S0.sample_rate_conf.tx_bck_div_num   = m;

  I2S0.sample_rate_conf.tx_bits_mod      = 16;

  if (m_isr_handle == nullptr) {
    esp_intr_alloc_pinnedToCore(ETS_I2S0_INTR_SOURCE, ESP_INTR_FLAG_LEVEL1, ISRHandler, this, &m_isr_handle, CoreUsage::quietCore());
    I2S0.int_clr.val     = 0xFFFFFFFF;
    I2S0.int_ena.out_eof = 1;
  }
  
  I2S0.out_link.addr  = (uintptr_t) m_DMAChain;
  I2S0.out_link.start = 1;
  I2S0.conf.tx_start  = 1;

  DAC_enableDMA();
  DAC_outputEnable(m_gpio == GPIO_NUM_25 ? GPIO_NUM_25 : GPIO_NUM_26);
  #endif
}


void SoundGenerator::sigmadelta_init()
{
  if (m_sdmChannel == nullptr) {
    sdm_config_t sdmConfig = { };
    sdmConfig.gpio_num       = m_gpio;
    sdmConfig.clk_src        = SDM_CLK_SRC_DEFAULT;
    sdmConfig.sample_rate_hz = 8000000;   // 80MHz / 10
    if (sdm_new_channel(&sdmConfig, &m_sdmChannel) == ESP_OK) {
      sdm_channel_enable(m_sdmChannel);
      sdm_channel_set_pulse_density(m_sdmChannel, 0);
    } else {
      ESP_LOGE("FabGL", "Unable to allocate Sigma-Delta channel");
      m_sdmChannel = nullptr;
    }
  }

  esp_timer_create_args_t args = { };
  args.callback        = timerHandler;
  args.arg             = this;
  args.dispatch_method = ESP_TIMER_TASK;
  esp_timer_create(&args, &m_timerHandle);
}


#ifdef FABGL_EMULATED
void SoundGenerator::sdl_init()
{
  SDL_AudioSpec wantSpec, haveSpec;
  SDL_zero(wantSpec);
  wantSpec.freq     = m_sampleRate;
  wantSpec.format   = AUDIO_U8;
  wantSpec.channels = 1;
  wantSpec.samples  = 2048;
  wantSpec.callback = SDLAudioCallback;
  wantSpec.userdata = this;
  m_device = SDL_OpenAudioDevice(NULL, 0, &wantSpec, &haveSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
  m_sampleRate = haveSpec.freq;
}
#endif


void SoundGenerator::init()
{
  if (!m_initDone) {
    // handle automatic parameters
    if (m_genMethod == SoundGenMethod::Auto)
      m_genMethod = CurrentVideoMode::get() == VideoMode::CVBS ? SoundGenMethod::SigmaDelta : SoundGenMethod::DAC;
    if (m_gpio == GPIO_AUTO)
      m_gpio = m_genMethod == SoundGenMethod::DAC ? GPIO_NUM_25 : GPIO_NUM_23;
    
    // actual init
    if (m_genMethod == SoundGenMethod::DAC)
      dac_init();
    else
      sigmadelta_init();
      
    #ifdef FABGL_EMULATED
    sdl_init();
    #endif
      
    m_initDone = true;
  }
}


bool SoundGenerator::play(bool value)
{
  if (value != m_play) {
    init();
    
    if (m_genMethod == SoundGenMethod::DAC) {
      I2S0.conf.tx_start = value;
    } else {
      if (value)
        esp_timer_start_periodic(m_timerHandle, 1000000 / m_sampleRate);
      else
        esp_timer_stop(m_timerHandle);
    }
    
    #ifdef FABGL_EMULATED
    SDL_PauseAudioDevice(m_device, !value);
    #endif
    
    m_play = value;
    return !value;
  } else
    return value;
}


SamplesGenerator * SoundGenerator::playSamples(int8_t const * data, int length, int volume, int durationMS)
{
  auto sgen = new SamplesGenerator(data, length);
  attach(sgen);
  sgen->setAutoDestroy(true);
  if (durationMS > -1)
    sgen->setDuration(durationMS > 0 ? (m_sampleRate / 1000 * durationMS) : length );
  sgen->setVolume(volume);
  sgen->enable(true);
  play(true);
  return sgen;
}


// does NOT take ownership of the waveform generator
void SoundGenerator::attach(WaveformGenerator * value)
{
  bool isPlaying = play(false);

  value->setSampleRate(m_sampleRate);

  value->next = m_channels;
  m_channels = value;

  play(isPlaying);
}


void SoundGenerator::detach(WaveformGenerator * value)
{
  if (!value)
    return;

  bool isPlaying = play(false);
  detachNoSuspend(value);
  play(isPlaying);
}


void SoundGenerator::detachNoSuspend(WaveformGenerator * value)
{
  for (WaveformGenerator * c = m_channels, * prev = nullptr; c; prev = c, c = c->next) {
    if (c == value) {
      if (prev)
        prev->next = c->next;
      else
        m_channels = c->next;
      if (value->autoDestroy())
        delete value;
      break;
    }
  }
}


int IRAM_ATTR SoundGenerator::getSample()
{
  int sample = 0, tvol = 0;
  for (auto g = m_channels; g; ) {
    if (g->enabled()) {
      sample += g->getSample();
      tvol += g->volume();
    } else if (g->duration() == 0 && g->autoDetach()) {
      auto curr = g;
      g = g->next;  // setup next item before detaching this one
      detachNoSuspend(curr);
      continue; // bypass "g = g->next;"
    }
    g = g->next;
  }

  int avol = tvol ? imin(127, 127 * 127 / tvol) : 127;
  sample = sample * avol / 127;
  sample = sample * volume() / 127;
  
  return sample;
}


// used by DAC generator
void IRAM_ATTR SoundGenerator::ISRHandler(void * arg)
{
  if (I2S0.int_st.out_eof) {

    auto soundGenerator = (SoundGenerator *) arg;
    auto desc = (volatile lldesc_t*) I2S0.out_eof_des_addr;
    
    auto buf = (uint16_t *) soundGenerator->m_sampleBuffer[desc->sosf];
    
    for (int i = 0; i < FABGL_SOUNDGEN_SAMPLE_BUFFER_SIZE; ++i)
      buf[i ^ 1] = (soundGenerator->getSample() + 127) << 8;
    
  }
  I2S0.int_clr.val = I2S0.int_st.val;
}


// used by sigma-delta generator
void SoundGenerator::timerHandler(void * args)
{
  auto soundGenerator = (SoundGenerator *) args;

  if (soundGenerator->m_sdmChannel)
    sdm_channel_set_pulse_density(soundGenerator->m_sdmChannel, (int8_t) soundGenerator->getSample());
}


#ifdef FABGL_EMULATED
void SoundGenerator::SDLAudioCallback(void * data, Uint8 * buffer, int length)
{
  auto soundGenerator = (SoundGenerator *) data;

  for (int i = 0; i < length; ++i)
    buffer[i] = soundGenerator->getSample() + 127;
}
#endif


// SoundGenerator
////////////////////////////////////////////////////////////////////////////////////////////////////////////



/*

// note (C,D,E,F,G,A,B) + [#,b] + octave (2..7) + space + tempo (99..1)
// pause (P) + space + tempo (99.1)
char const * noteToFreq(char const * note, int * freq)
{
  uint16_t NIDX2FREQ[][12] = { {   66,   70,   74,   78,   83,   88,   93,   98,  104,  110,  117,  124 }, // 2
                               {  131,  139,  147,  156,  165,  175,  185,  196,  208,  220,  233,  247 }, // 3
                               {  262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494 }, // 4
                               {  523,  554,  587,  622,  659,  698,  740,  784,  831,  880,  932,  988 }, // 5
                               { 1046, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976 }, // 6
                               { 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951 }, // 7
                             };
  uint8_t NNAME2NIDX[] = {9, 11, 0, 2, 4, 5, 7};  // A, B, C, D, E, F, G
  *freq = 0;
  while (*note && *note == ' ')
    ++note;
  if (*note == 0)
    return note;
  int noteIndex = (*note >= 'A' && *note <= 'G' ? NNAME2NIDX[*note - 'A'] : -2); // -2 = pause
  ++note;
  if (*note == '#') {
    ++noteIndex;
    ++note;
  } else if (*note == 'b') {
    --noteIndex;
    ++note;
  }
  int octave = *note - '0';
  ++note;
  if (noteIndex == -1) {
    noteIndex = 11;
    --octave;
  } else if (noteIndex == 12) {
    noteIndex = 0;
    ++octave;
  }
  if (noteIndex >= 0 && noteIndex <= 11 && octave >= 2 && octave <= 7)
    *freq = NIDX2FREQ[octave - 2][noteIndex];
  return note;
}


char const * noteToDelay(char const * note, int * delayMS)
{
  *delayMS = 0;
  while (*note && *note == ' ')
    ++note;
  if (*note == 0)
    return note;
  int val = atoi(note);
  if (val > 0)
    *delayMS = 1000 / val;
  return note + (val > 9 ? 2 : 1);
}



void play_task(void*arg)
{
  const char * music = "A4 4 A4 4 A#4 4 C5 4 C5 4 A#4 4 A4 4 G4 4 F4 4 F4 4 G4 4 A4 4 A4 2 G4 16 G4 2 P 8 "
                       "A4 4 A4 4 A#4 4 C5 4 C5 4 A#4 4 A4 4 G4 4 F4 4 F4 4 G4 4 A4 4 G4 2 F4 16 F4 2 P 8";
  while (true) {
    if (Play) {
      const char * m = music;
      while (*m && Play) {
        int freq, delms;
        m = noteToFreq(m, &freq);
        m = noteToDelay(m, &delms);
        frequency = freq;
        delay(delms);
        frequency = 0;
        delay(25);
      }
      Play = false;
    } else
      delay(100);
  }
}


*/


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ADSREnvelope


ADSREnvelope::ADSREnvelope()
  : m_sampleRate(FABGL_SOUNDGEN_DEFAULT_SAMPLE_RATE)
{
  setAttack(10);
  setDecay(200);
  setSustain(70);
  setRelease(300);
}


void ADSREnvelope::setSampleRate(int value)
{
  m_sampleRate = value;
  setAttack(m_attackMs);
  setDecay(m_decayMs);
  setRelease(m_releaseMs);
}


// ms -> level increment per sample (from 0 to MAX_LEVEL)
int ADSREnvelope::msToInc(int ms)
{
  int samples = ms * m_sampleRate / 1000;
  return samples > 0 ? MAX_LEVEL / samples : MAX_LEVEL;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ResonantLowPassFilter


ResonantLowPassFilter::ResonantLowPassFilter()
  : m_enabled(true),
    m_sampleRate(FABGL_SOUNDGEN_DEFAULT_SAMPLE_RATE),
    m_cutoff(2000),
    m_cutoffIndex(0),
    m_resonance(0.5f),
    m_envAmount(0),
    m_envAmountQ8(0)
{
  setCutoff(m_cutoff);
  updateResonance();    // also builds the table
}


void ResonantLowPassFilter::setSampleRate(int value)
{
  m_sampleRate = value;
  buildTable();
}


// position of a frequency in the cutoff table (Q8)
static int cutoffToIndex(float hz)
{
  using F = ResonantLowPassFilter;
  float pos = logf(hz / F::MIN_CUTOFF) / logf((float) F::MAX_CUTOFF / F::MIN_CUTOFF);
  return iclamp((int) lroundf(pos * F::MAX_CUTOFF_INDEX), 0, F::MAX_CUTOFF_INDEX);
}


void ResonantLowPassFilter::setCutoff(int hz)
{
  m_cutoff      = iclamp(hz, MIN_CUTOFF, MAX_CUTOFF);
  m_cutoffIndex = cutoffToIndex(m_cutoff);
}


void ResonantLowPassFilter::setResonance(float value)
{
  m_resonance = tclamp(value, 0.0f, MAX_RESONANCE);
  updateResonance();
}


void ResonantLowPassFilter::setEnvelopeAmount(int percent)
{
  m_envAmount   = iclamp(percent, -100, 100);
  m_envAmountQ8 = m_envAmount * MAX_CUTOFF_INDEX / 100;
}


// feedback gain of the ladder. k = 4 is the self-oscillation threshold: above it (last 7% of the resonance range)
// the oscillation grows until the feedback saturation limits it, like in the analog ladder filters
static float ladderFeedback(float resonance)
{
  return 4.3f * resonance;
}


// input gain: partially compensates the passband attenuation 1 / (1 + k) caused by the feedback
static float ladderInputGain(float k)
{
  return 1.0f + 0.5f * k;
}


void ResonantLowPassFilter::updateResonance()
{
  float k = ladderFeedback(m_resonance);
  m_k     = (int) lroundf(k * (1 << COEFF_SHIFT));
  m_comp  = (int) lroundf(ladderInputGain(k) * (1 << COEFF_SHIFT));
  buildTable();
}


// coefficients for CUTOFF_STEPS frequencies on a logarithmic scale
void ResonantLowPassFilter::buildTable()
{
  float scale = (float) (1 << COEFF_SHIFT);   // Q28
  float q31   = 2147483648.0f;                // Q31 (values lower than 1)
  float k     = ladderFeedback(m_resonance);
  float comp  = ladderInputGain(k);
  for (int i = 0; i < CUTOFF_STEPS; ++i) {
    float hz  = MIN_CUTOFF * powf((float) MAX_CUTOFF / MIN_CUTOFF, (float) i / (CUTOFF_STEPS - 1));
    float g   = tanf((float) M_PI * hz / m_sampleRate);   // prewarped cutoff
    float G   = g / (1.0f + g);
    float G2  = G * G, G3 = G2 * G, G4 = G3 * G;
    float den = 1.0f + k * G4;
    auto & t = m_table[i];
    t.g  = (int32_t) llroundf(G * q31);
    t.s1 = (int32_t) llroundf(G3 * (1.0f - G) * q31);
    t.s2 = (int32_t) llroundf(G2 * (1.0f - G) * q31);
    t.s3 = (int32_t) llroundf(G  * (1.0f - G) * q31);
    t.s4 = (int32_t) llroundf((1.0f - G) * q31);
    t.g4 = (int32_t) llroundf(G4 * q31);
    t.fb = (int32_t) lroundf(k / den * scale);
    t.in = (int32_t) lroundf(comp / den * scale);
  }
}


// runs inside the sound generator interrupt: fixed point only, no divisions
void ResonantLowPassFilter::calcCoeffs(int cutoffIndex, Coeffs & c)
{
  cutoffIndex = iclamp(cutoffIndex, 0, MAX_CUTOFF_INDEX);
  int i = cutoffIndex >> 8, f = cutoffIndex & 0xff;
  auto const & a = m_table[i];
  auto const & b = m_table[i < CUTOFF_STEPS - 1 ? i + 1 : i];
  // (b - a) is small between adjacent steps, but computed in 64 bit to be safe with Q31 values
  auto lerp = [f](int32_t a, int32_t b) { return a + (int32_t) (((int64_t) (b - a) * f) >> 8); };
  c.g    = lerp(a.g,  b.g);
  c.s1   = lerp(a.s1, b.s1);
  c.s2   = lerp(a.s2, b.s2);
  c.s3   = lerp(a.s3, b.s3);
  c.s4   = lerp(a.s4, b.s4);
  c.g4   = lerp(a.g4, b.g4);
  c.fb   = lerp(a.fb, b.fb);
  c.in   = lerp(a.in, b.in);
  c.k    = m_k;
  c.comp = m_comp;
}


// the ladder of trapezoidal one-pole stages is the bilinear transform (prewarped at the cutoff) of the analog
// prototype H(s) = 1 / ((1 + s)^4 + k), with s = j tan(pi f / fs) / tan(pi fc / fs)
float ResonantLowPassFilter::responseDB(float freq)
{
  float w  = tanf((float) M_PI * tclamp(freq, 1.0f, m_sampleRate * 0.499f) / m_sampleRate) /
             tanf((float) M_PI * m_cutoff / m_sampleRate);
  float k  = ladderFeedback(m_resonance);
  // (1 + jw)^2 = (1 - w^2) + j 2w ; (1 + jw)^4 = (a + jb)^2
  float a  = 1.0f - w * w, b = 2.0f * w;
  float dr = a * a - b * b + k, di = 2.0f * a * b;
  float gain = ladderInputGain(k);
  return 10.0f * log10f(gain * gain / (dr * dr + di * di) + 1e-12f);
}


int ResonantLowPassFilter::cutoffFromPosition(int position)
{
  return (int) lroundf(MIN_CUTOFF * powf((float) MAX_CUTOFF / MIN_CUTOFF, position / 100.0f));
}


float ResonantLowPassFilter::resonanceFromPosition(int position)
{
  return MAX_RESONANCE * tclamp(position, 0, 100) / 100.0f;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SynthVoice


// 16 bit sine table (257 entries: the last one helps the interpolation), filled by the first SynthVoice constructor
static int16_t s_synthSineTable[257];
static bool    s_synthSineTableReady = false;


static void initSynthSineTable()
{
  if (!s_synthSineTableReady) {
    for (int i = 0; i < 257; ++i)
      s_synthSineTable[i] = (int16_t) lroundf(32767.0f * sinf(i * 2.0f * (float) M_PI / 256.0f));
    s_synthSineTableReady = true;
  }
}


// interpolated sine (Q15) of a 32 bit phase
__attribute__((always_inline)) static inline int synthSine(uint32_t phase)
{
  int i = phase >> 24;
  int f = (phase >> 8) & 0xffff;
  int a = s_synthSineTable[i];
  return a + (((s_synthSineTable[i + 1] - a) * f) >> 16);
}


// PolyBLEP correction (band-limited step) for a discontinuity of height 2 at phase 0.
// t = phase, dt = phase increment, both as fractions of a cycle in Q16. Returns Q15.
// dt must be <= 16384 (frequency <= sample rate / 4): see SynthVoice::setFrequency()
__attribute__((always_inline)) static inline int polyBLEP(int t, int dt)
{
  if (t < dt) {
    int x = (t << 15) / dt;                 // 0..32767
    return 2 * x - ((x * x) >> 15) - 32768;
  }
  if (t > 65536 - dt) {
    int x = ((t - 65536) << 15) / dt;       // -32768..0
    return ((x * x) >> 15) + 2 * x + 32768;
  }
  return 0;
}


SynthVoice::SynthVoice()
  : m_envelope(nullptr),
    m_filter(nullptr),
    m_filterEnvelope(nullptr),
    m_lfo(nullptr),
    m_filterCoeffs(),
    m_controlCounter(0),
    m_filterEnvState(SynthVoiceState::Idle),
    m_filterLevel(0),
    m_sampleRate(FABGL_SOUNDGEN_DEFAULT_SAMPLE_RATE),
    m_waveform(SynthWaveform::Sawtooth),
    m_phase(0),
    m_phaseInc(0),
    m_note(-1),
    m_state(SynthVoiceState::Idle),
    m_level(0)
{
  initSynthSineTable();
}


void SynthVoice::setSampleRate(int value)
{
  m_sampleRate = value;
}


void SynthVoice::setup(ADSREnvelope * envelope, ResonantLowPassFilter * filter, ADSREnvelope * filterEnvelope, SynthLFO * lfo)
{
  m_envelope       = envelope;
  m_filter         = filter;
  m_filterEnvelope = filterEnvelope;
  m_lfo            = lfo;
}


void SynthVoice::setFrequency(float value)
{
  // limited to sample rate / 4: higher frequencies would alias anyway, and PolyBLEP needs it
  float inc = value / m_sampleRate * 4294967296.0f;
  m_phaseInc = (uint32_t) tclamp(inc, 0.0f, 1073741824.0f);
}


void SynthVoice::noteOn(int note)
{
  if (m_state == SynthVoiceState::Idle) {
    // safe: the interrupt doesn't touch idle voices
    m_filterState.reset();
    m_phase       = 0;
    m_filterLevel = 0;
  }
  m_note = note;
  setFrequency(440.0f * powf(2.0f, (note - 69) / 12.0f));
  m_filterEnvState = SynthVoiceState::Attack;
  m_controlCounter = 0;               // filter coefficients recalculated at the next sample
  __sync_synchronize();               // the interrupt (maybe on the other core) must see the above before the new state
  m_state = SynthVoiceState::Attack;  // a retriggered voice restarts from its current levels (no clicks)
}


void SynthVoice::noteOff()
{
  if (m_state != SynthVoiceState::Idle) {
    m_filterEnvState = SynthVoiceState::Release;
    m_state          = SynthVoiceState::Release;
  }
}


// envelope parameters, read once per block
struct EnvelopeIncs {
  int attack, decay, sustain, release;

  EnvelopeIncs(ADSREnvelope * env)
    : attack(env->attackInc()), decay(env->decayInc()), sustain(env->sustainLevel()), release(env->releaseInc())
  {
  }
};


// advances an envelope by one sample. Returns false when the release phase is over (state becomes Idle)
__attribute__((always_inline)) static inline bool advanceEnvelope(EnvelopeIncs const & env, SynthVoiceState & state, int & level)
{
  switch (state) {
    case SynthVoiceState::Attack:
      level += env.attack;
      if (level >= ADSREnvelope::MAX_LEVEL) {
        level = ADSREnvelope::MAX_LEVEL;
        state = SynthVoiceState::Decay;
      }
      break;
    case SynthVoiceState::Decay:
      level -= env.decay;
      if (level <= env.sustain) {
        level = env.sustain;
        state = SynthVoiceState::Sustain;
      }
      break;
    case SynthVoiceState::Sustain:
      level = env.sustain;                  // follows the sustain level while the key is held
      break;
    case SynthVoiceState::Release:
      level -= env.release;
      if (level <= 0) {
        level = 0;
        state = SynthVoiceState::Idle;
        return false;
      }
      break;
    case SynthVoiceState::Idle:
      return false;
  }
  return true;
}


// band-limited oscillator sample (Q15). t = phase (Q16), dt = phase increment (Q16)
template <int WAVEFORM>
__attribute__((always_inline)) static inline int oscillatorSample(uint32_t phase, int dt)
{
  int t = phase >> 16;
  switch ((SynthWaveform) WAVEFORM) {
    case SynthWaveform::Sine:
      return synthSine(phase);
    case SynthWaveform::Square:
      return (t < 32768 ? 32767 : -32767) + polyBLEP(t, dt) - polyBLEP((t + 32768) & 0xffff, dt);
    case SynthWaveform::Pulse:
      return (t < 16384 ? 32767 : -32767) + polyBLEP(t, dt) - polyBLEP((t + 49152) & 0xffff, dt);
    case SynthWaveform::Triangle:
      return t < 32768 ? -32767 + 2 * t : 98303 - 2 * t;
    default:  // Sawtooth
      return (t - 32768) - polyBLEP(t, dt);
  }
}


// Runs inside the sound generator interrupt: fixed point only.
// The fields shared with the application (parameters, states) are read once per block and kept in local
// variables (registers): volatile accesses and function calls for every sample are expensive.
// A note on / note off arriving during the block is applied at the next block (at most CONTROL_PERIOD samples).
template <int WAVEFORM, bool FILTER>
void SynthVoice::renderLoop(int32_t * buffer, int count)
{
  uint32_t phase = m_phase;
  uint32_t inc   = m_phaseInc;
  int      dt    = imax(1, inc >> 16);

  SynthVoiceState const state0  = m_state;
  SynthVoiceState       state   = state0;
  int                   level   = m_level;
  EnvelopeIncs const    ampEnv(m_envelope);

  SynthVoiceState const fstate0 = m_filterEnvState;
  SynthVoiceState       fstate  = fstate0;
  int                   flevel  = m_filterLevel;
  EnvelopeIncs const    filterEnv(m_filterEnvelope ? m_filterEnvelope : m_envelope);

  // separate passes over the block: each loop has few live variables, so they stay in registers
  int32_t osc[CONTROL_PERIOD];   // oscillator, then filter output (Q15)
  int32_t amp[CONTROL_PERIOD];   // amplitude envelope (0..4096)
  count = imin(count, CONTROL_PERIOD);

  // amplitude envelope and oscillator
  int n = 0;
  for (; n < count; ++n) {
    if (!advanceEnvelope(ampEnv, state, level))
      break;
    amp[n] = level >> LEVEL_SHIFT;
    osc[n] = oscillatorSample<WAVEFORM>(phase, dt);
    phase += inc;
  }

  if (FILTER) {
    // filter envelope (used for the coefficients of the next block)
    if (m_filterEnvelope)
      for (int i = 0; i < n; ++i)
        advanceEnvelope(filterEnv, fstate, flevel);
    // filter
    auto filterState = m_filterState;
    auto const & c   = m_filterCoeffs;
    for (int i = 0; i < n; ++i)
      osc[i] = m_filter->process(filterState, c, osc[i]);
    m_filterState = filterState;
  } else if (n > 0) {
    // keep the filter state settled on the signal: enabling the filter doesn't click
    m_filterState.s1 = m_filterState.s2 = m_filterState.s3 = m_filterState.s4 = osc[n - 1] << 8;
  }

  for (int i = 0; i < n; ++i)
    buffer[i] += (osc[i] * amp[i]) >> 12;

  m_phase       = phase;
  m_level       = level;
  m_filterLevel = flevel;
  // write the states only if the application didn't change them meanwhile (note on / note off)
  if (fstate != fstate0 && m_filterEnvState == fstate0)
    m_filterEnvState = fstate;
  if (state != state0 && m_state == state0) {
    if (state == SynthVoiceState::Idle)
      m_note = -1;
    m_state = state;
  }
}


// runs inside the sound generator interrupt: fixed point only
void SynthVoice::render(int32_t * buffer, int count)
{
  if (!m_envelope || m_state == SynthVoiceState::Idle)
    return;

  // cutoff = base cutoff moved by the filter envelope and by the LFO, recalculated at control rate
  bool filterOn = m_filter && m_filter->enabled();
  if (filterOn) {
    if (m_controlCounter <= 0) {
      m_controlCounter = CONTROL_PERIOD;
      int index = m_filter->cutoffIndex() + m_filter->envelopeOffset(filterEnvelopeLevel());
      if (m_lfo)
        index += m_lfo->cutoffOffset();
      m_filter->calcCoeffs(index, m_filterCoeffs);
    }
    m_controlCounter -= count;
  } else
    m_controlCounter = 0;

  #define FABGL_SYNTH_RENDER(W) (filterOn ? renderLoop<(int) W, true>(buffer, count) : renderLoop<(int) W, false>(buffer, count))
  switch (m_waveform) {
    case SynthWaveform::Sine:     FABGL_SYNTH_RENDER(SynthWaveform::Sine);     break;
    case SynthWaveform::Square:   FABGL_SYNTH_RENDER(SynthWaveform::Square);   break;
    case SynthWaveform::Pulse:    FABGL_SYNTH_RENDER(SynthWaveform::Pulse);    break;
    case SynthWaveform::Triangle: FABGL_SYNTH_RENDER(SynthWaveform::Triangle); break;
    default:                      FABGL_SYNTH_RENDER(SynthWaveform::Sawtooth); break;
  }
  #undef FABGL_SYNTH_RENDER
}


int SynthVoice::getSample()
{
  int32_t sample = 0;
  render(&sample, 1);
  return sample;
}


int SynthVoice::waveformShape(SynthWaveform waveform, int phase)
{
  phase &= 255;
  switch (waveform) {
    case SynthWaveform::Sine:     return (int) lroundf(127.0f * sinf(phase * 2.0f * (float) M_PI / 256.0f));
    case SynthWaveform::Square:   return phase < 128 ? 127 : -127;
    case SynthWaveform::Pulse:    return phase < 64 ? 127 : -127;
    case SynthWaveform::Triangle: return phase < 128 ? -127 + phase * 2 : 127 - (phase - 128) * 2;
    default:                      return -127 + phase;   // Sawtooth
  }
}


char const * SynthVoice::waveformName(SynthWaveform waveform)
{
  static char const * names[] = { "SINE", "SQUARE", "PULSE 25%", "TRIANGLE", "SAWTOOTH" };
  return names[(int) waveform];
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SynthLFO


SynthLFO::SynthLFO()
  : m_sampleRate(FABGL_SOUNDGEN_DEFAULT_SAMPLE_RATE),
    m_waveform(LFOWaveform::Triangle),
    m_rate(5.0f),
    m_phase(0),
    m_phaseInc(0),
    m_value(0),
    m_randomValue(0),
    m_randomSeed(12345),
    m_cutoffDepth(0),
    m_cutoffDepthQ8(0),
    m_ampDepth(0),
    m_ampDepthQ12(0)
{
  initSynthSineTable();
  setRate(m_rate);
}


void SynthLFO::setSampleRate(int value)
{
  m_sampleRate = value;
  setRate(m_rate);
}


void SynthLFO::setRate(float hz)
{
  m_rate     = tclamp(hz, MIN_RATE, MAX_RATE);
  m_phaseInc = (uint32_t) (m_rate / m_sampleRate * 4294967296.0f);
}


void SynthLFO::setCutoffDepth(int percent)
{
  m_cutoffDepth   = iclamp(percent, 0, 100);
  m_cutoffDepthQ8 = m_cutoffDepth * (ResonantLowPassFilter::MAX_CUTOFF_INDEX / 2) / 100;
}


void SynthLFO::setAmpDepth(int percent)
{
  m_ampDepth    = iclamp(percent, 0, 100);
  m_ampDepthQ12 = m_ampDepth * 4096 / 100;
}


// runs inside the sound generator interrupt: fixed point only
void SynthLFO::advance(int samples)
{
  uint32_t prev  = m_phase;
  uint32_t phase = prev + m_phaseInc * samples;
  m_phase = phase;
  int t = phase >> 16;                       // Q16
  switch (m_waveform) {
    case LFOWaveform::Triangle:
      m_value = t < 32768 ? -32767 + 2 * t : 98303 - 2 * t;
      break;
    case LFOWaveform::Sine:
      m_value = synthSine(phase);
      break;
    case LFOWaveform::Square:
      m_value = t < 32768 ? 32767 : -32767;
      break;
    case LFOWaveform::Sawtooth:
      m_value = iclamp(t - 32768, -32767, 32767);
      break;
    case LFOWaveform::Random:
      if (phase < prev) {                    // new cycle: new random value
        m_randomSeed  = m_randomSeed * 1664525 + 1013904223;
        m_randomValue = iclamp((int32_t) m_randomSeed >> 16, -32767, 32767);
      }
      m_value = m_randomValue;
      break;
  }
}


float SynthLFO::rateFromPosition(int position)
{
  return MIN_RATE * powf(MAX_RATE / MIN_RATE, tclamp(position, 0, 100) / 100.0f);
}


int SynthLFO::waveformShape(LFOWaveform waveform, int phase)
{
  static const int8_t randomSteps[4] = { 60, -110, 115, -30 };
  phase &= 255;
  switch (waveform) {
    case LFOWaveform::Triangle: return phase < 128 ? -127 + phase * 2 : 127 - (phase - 128) * 2;
    case LFOWaveform::Sine:     return (int) lroundf(127.0f * sinf(phase * 2.0f * (float) M_PI / 256.0f));
    case LFOWaveform::Square:   return phase < 128 ? 127 : -127;
    case LFOWaveform::Sawtooth: return -127 + phase;
    default:                    return randomSteps[phase / 64];   // Random
  }
}


char const * SynthLFO::waveformName(LFOWaveform waveform)
{
  static char const * names[] = { "TRIANGLE", "SINE", "SQUARE", "SAWTOOTH", "RANDOM" };
  return names[(int) waveform];
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PolySynth


// gain fractional bits (1.0 = 1 << GAIN_SHIFT)
static constexpr int GAIN_SHIFT = 20;


PolySynth::PolySynth(int voices)
  : m_soundGenerator(nullptr),
    m_voicesCount(voices),
    m_waveform(SynthWaveform::Sawtooth),
    m_nextSteal(0),
    m_channel(this),
    m_gain(1 << GAIN_SHIFT),
    m_lfoGain(1 << GAIN_SHIFT)
{
  m_voices = new SynthVoice[voices];
  // default filter envelope: a short sweep that settles low (the amount is 0: the cutoff doesn't move)
  m_filterEnvelope.setAttack(5);
  m_filterEnvelope.setDecay(400);
  m_filterEnvelope.setSustain(30);
  m_filterEnvelope.setRelease(300);
  for (int i = 0; i < voices; ++i) {
    m_voices[i].setup(&m_envelope, &m_filter, &m_filterEnvelope, &m_lfo);
    m_voices[i].setWaveform(m_waveform);
  }
  m_channel.setVolume(127);
}


PolySynth::~PolySynth()
{
  if (m_soundGenerator)
    m_soundGenerator->detach(&m_channel);
  delete [] m_voices;
}


void PolySynth::begin(SoundGenerator * soundGenerator)
{
  m_soundGenerator = soundGenerator;
  int sampleRate = soundGenerator->sampleRate();
  m_envelope.setSampleRate(sampleRate);
  m_filterEnvelope.setSampleRate(sampleRate);
  m_filter.setSampleRate(sampleRate);
  m_lfo.setSampleRate(sampleRate);
  for (int i = 0; i < m_voicesCount; ++i)
    m_voices[i].setSampleRate(sampleRate);
  soundGenerator->attach(&m_channel);
  m_channel.enable(true);
  soundGenerator->play(true);
}


void PolySynth::setWaveform(SynthWaveform value)
{
  m_waveform = value;
  for (int i = 0; i < m_voicesCount; ++i)
    m_voices[i].setWaveform(value);
}


void PolySynth::noteOn(int note)
{
  SynthVoice * voice = nullptr;
  // same note still sounding (releasing)? reuse its voice
  for (int i = 0; !voice && i < m_voicesCount; ++i)
    if (m_voices[i].note() == note)
      voice = &m_voices[i];
  // free voice?
  for (int i = 0; !voice && i < m_voicesCount; ++i)
    if (m_voices[i].isIdle())
      voice = &m_voices[i];
  // all voices busy: steal a releasing one, otherwise the oldest one
  for (int i = 0; !voice && i < m_voicesCount; ++i)
    if (m_voices[i].state() == SynthVoiceState::Release)
      voice = &m_voices[i];
  if (!voice)
    voice = &m_voices[m_nextSteal++ % m_voicesCount];

  voice->noteOn(note);
}


void PolySynth::noteOff(int note)
{
  for (int i = 0; i < m_voicesCount; ++i)
    if (m_voices[i].note() == note && m_voices[i].state() != SynthVoiceState::Release)
      m_voices[i].noteOff();
}


void PolySynth::allNotesOff()
{
  for (int i = 0; i < m_voicesCount; ++i)
    m_voices[i].noteOff();
}


// level of a single voice at the full envelope level (Q15): leaves room for the peaks of more notes
static constexpr int MIX_HEADROOM = 24576;   // 0.75


// soft limiter (Q15): linear up to MIX_HEADROOM, then smoothly bent towards the full scale (never reached)
static inline int softClip(int x)
{
  constexpr int R = 32767 - MIX_HEADROOM;
  int a = x < 0 ? -x : x;
  if (a <= MIX_HEADROOM)
    return x;
  int u = a - MIX_HEADROOM;
  int y = MIX_HEADROOM + (int) ((int64_t) R * u / (R + u));
  return x < 0 ? -y : y;
}


// runs inside the sound generator interrupt: fixed point only. Renders BLOCK_SIZE samples (-127..127)
void PolySynth::renderBlock(int8_t * dest)
{
  m_lfo.advance(BLOCK_SIZE);

  int32_t sum[BLOCK_SIZE] = { };
  int levels = 0;
  for (int i = 0; i < m_voicesCount; ++i) {
    auto & voice = m_voices[i];
    if (!voice.isIdle()) {
      voice.render(sum, BLOCK_SIZE);
      levels += voice.level();
    }
  }

  // automatic gain: 1 up to a total envelope level of 1, then 1 / sqrt(total level). Different notes add up in power,
  // not in amplitude: this keeps the loudness constant when more notes play. The peaks that exceed the headroom
  // are bent by the soft limiter.
  // The gain follows the target quickly when it must decrease, slowly when it increases (no level jumps when a note ends)
  constexpr int ONE = SynthVoice::LEVEL_ONE;                   // 4096 = 64 * 64
  int target    = (levels > ONE ? ONE * 64 / isqrt(levels) : ONE) << (GAIN_SHIFT - 12);
  int lfoTarget = m_lfo.ampGain() << (GAIN_SHIFT - 12);
  int volume    = m_channel.volume();

  for (int i = 0; i < BLOCK_SIZE; ++i) {
    m_gain += (target - m_gain) >> (target < m_gain ? 4 : 8);
    // LFO tremolo, smoothed (about 2 ms) so the square and random waveforms don't click
    m_lfoGain += (lfoTarget - m_lfoGain) >> 5;
    int gain = (int) (((int64_t) m_gain * m_lfoGain) >> GAIN_SHIFT);
    int out  = (int) (((int64_t) sum[i] * gain) >> GAIN_SHIFT);  // Q15
    out = softClip((out * MIX_HEADROOM) >> 15);
    out = out * volume / 127;
    dest[i] = iclamp((out + 128) >> 8, -127, 127);              // to 8 bit, rounded
  }
}


int PolySynth::noteFrequency(int note)
{
  return (int) lroundf(440.0f * powf(2.0f, (note - 69) / 12.0f));
}





} // end of namespace
