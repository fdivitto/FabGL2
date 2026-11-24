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
 * @brief This file contains the DS3231 (real-time clock) device driver.
 */





#include <stdint.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"

#include "fabglconf.h"
#include "fabutils.h"
#include "comdrivers/tsi2c.h"



namespace fabgl {


/**
 * @brief Represents a date and time
 *
 * @ingroup devices
 */
struct DateTime {
  int8_t  seconds;      /**< 0..59             */
  int8_t  minutes;      /**< 0..59             */
  int8_t  hours;        /**< 0..23             */
  int8_t  dayOfWeek;    /**< 1..7 (1 = Sunday) */
  int8_t  dayOfMonth;   /**< 1..31             */
  int8_t  month;        /**< 1..12             */
  int16_t year;         /**< 1900..9999 (2000..2099 for DS3231, 1900..2038 when using timestamp()) */

  DateTime() { }
  DateTime(int seconds_, int minutes_, int hours_, int dayOfMonth_, int month_, int year_);

  /**
   * @brief Calculates the Unix timestamp
   *
   * @param timezone Time zone offset in hours
   *
   * @return Unix timestamp
   */
  time_t timestamp(int timezone);

private:
  void calcDayOfWeek();
};


/**
 * @brief DS3231 real-time clock driver
 *
 * Example:
 *
 *     fabgl::I2C    I2C;
 *     fabgl::DS3231 DS3231;
 *
 *     extern "C" void app_main() {
 *       I2C.begin(GPIO_NUM_4, GPIO_NUM_15);  // 4 = SDA, 15 = SCL (warning: disconnect the VGA connector)
 *       DS3231.begin(&I2C);
 *
 *       auto dt = DS3231.datetime();
 *       printf("%02d/%02d/%d  %02d:%02d:%02d\n", dt.dayOfMonth, dt.month, dt.year, dt.hours, dt.minutes, dt.seconds);
 *       printf("temp = %0.3f C\n", DS3231.temperature());
 *     }
 *
 * @ingroup devices
 */
class DS3231 {

public:

  DS3231();

  /**
   * @brief Initializes the DS3231 driver
   *
   * @param i2c Pointer to the I2C driver
   */
  void begin(I2C * i2c);

  /**
   * @brief Determines whether the DS3231 is reachable
   *
   * @return True if the device has been correctly initialized.
   */
  bool available() { return m_available; }

  /**
   * @brief Determines whether the stored date and time are valid
   *
   * The date and time become valid after calling setDateTime().
   *
   * @return False if the oscillator is stopped or has been stopped for some time, so the timekeeping data may be invalid.
   */
  bool dateTimeValid() { return m_dateTimeValid; }

  /**
   * @brief Queries the DS3231 for the current date and time
   *
   * @return A DateTime structure containing the current date and time
   */
  DateTime const & datetime();

  /**
   * @brief Sets the current date and time
   *
   * This method also resets the invalid date flag.
   *
   * @param value Date and time to set
   */
  bool setDateTime(DateTime const & value);

  /**
   * @brief Forces the DS3231 to read the current temperature
   *
   * @return Current temperature in degrees Celsius
   */
  double temperature();

  /**
   * @brief Enables or disables the DS3231 oscillator
   *
   * @param value If true, the oscillator is enabled (date and time are always maintained by the backup battery)
   */
  void clockEnabled(bool value);

private:

  uint8_t readReg(int reg);

  bool writeReg(int reg, int value);

  I2C *    m_i2c;
  bool     m_available;
  bool     m_dateTimeValid;

  DateTime m_datetime;


};



} // fabgl namespace


