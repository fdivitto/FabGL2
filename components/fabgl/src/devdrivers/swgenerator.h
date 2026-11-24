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
 * @brief This file contains the fabgl::GPIOStream definition.
 */


#include <stdint.h>
#include <stddef.h>


#include "esp32/rom/lldesc.h"

#include "fabglconf.h"





namespace fabgl {



/*
 * Square wave generator (or DMA to GPIO stream generator) driven by the internal audio PLL (APLL) clock.
 *
 * When FABGLIB_USE_APLL_AB_COEF = 0 (the default) the frequency range is 2651514 Hz to 62500000 Hz.<br>
 * The average error is 21 Hz, the minimum error is 0 and the maximum error is 1000 Hz, except in the range 41666667 Hz to 42708333 Hz, where
 * the frequency stays at 41666666 Hz (error from 0 Hz to 1041666 Hz), and in the range 42708334 Hz to 43748999 Hz, where
 * the frequency stays at 43750000 Hz (error from 750001 Hz to 1041666 Hz).
 *
 * When FABGLIB_USE_APLL_AB_COEF = 1 the frequency range is 82500 Hz to 62500000 Hz, but the output has a lot of frequency jitter.<br>
 * The average error is about 7 Hz, the minimum error is 0 and the maximum error is 6349 Hz.
 */
class GPIOStream {

public:

  void begin();

  /*
   * Initializes GPIOStream and assigns GPIOs to the outputs.
   *
   * div1_onGPIO0 If true, the undivided frequency is delivered on GPIO0.
   * div2 GPIO that outputs frequency / 2 (set GPIO_UNUSED to disable output).
   * div4 GPIO that outputs frequency / 4 (set GPIO_UNUSED to disable output).
   * div8 GPIO that outputs frequency / 8 (set GPIO_UNUSED to disable output).
   * div16 GPIO that outputs frequency / 16 (set GPIO_UNUSED to disable output).
   * div32 GPIO that outputs frequency / 32 (set GPIO_UNUSED to disable output).
   * div64 GPIO that outputs frequency / 64 (set GPIO_UNUSED to disable output).
   * div128 GPIO that outputs frequency / 128 (set GPIO_UNUSED to disable output).
   * div256 GPIO that outputs frequency / 256 (set GPIO_UNUSED to disable output).
   *
   * Example:
   *
   *     // Output 25 MHz on GPIO0 and 6.25 MHz on GPIO5 for 5 seconds
   *     GPIOStream.begin(true, GPIO_UNUSED, GPIO_NUM_5);
   *     GPIOStream.play(25000000);
   *     vTaskDelay(pdMS_TO_TICKS(5000));
   *     // Output 20 MHz on GPIO0 and 5 MHz on GPIO5 for 10 seconds
   *     GPIOStream.play(20000000);
   *     vTaskDelay(pdMS_TO_TICKS(10000));
   *     GPIOStream.stop();
   */
  void begin(bool div1_onGPIO0, gpio_num_t div2 = GPIO_UNUSED, gpio_num_t div4 = GPIO_UNUSED, gpio_num_t div8 = GPIO_UNUSED, gpio_num_t div16 = GPIO_UNUSED, gpio_num_t div32 = GPIO_UNUSED, gpio_num_t div64 = GPIO_UNUSED, gpio_num_t div128 = GPIO_UNUSED, gpio_num_t div256 = GPIO_UNUSED);

  /*
   * Disables all outputs.
   */
  void end();

  /*
   * Sets the main frequency.
   *
   * freq Frequency in Hertz.
   * dmaBuffers Use only to provide custom DMA buffers.
   *
   * Example:
   *
   *     // Set the main frequency to 25 MHz
   *     GPIOStream.play(25000000);
   */
  void play(int freq, lldesc_t volatile * dmaBuffers = nullptr);

  /*
   * Disables all outputs.
   */
  void stop();

private:

  void setupClock(int freq);
  static void setupGPIO(gpio_num_t gpio, int bit, gpio_mode_t mode);

  bool                m_DMAStarted;
  volatile lldesc_t * m_DMABuffer;
  volatile uint8_t *  m_DMAData;
};





} // end of namespace









