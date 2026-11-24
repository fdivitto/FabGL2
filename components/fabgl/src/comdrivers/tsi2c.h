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


#include <stdint.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "driver/i2c_master.h"

#include "fabutils.h"
#include "fabglconf.h"



/**
 * @file
 *
 * @brief This file contains the fabgl::I2C definition.
 */



namespace fabgl {



/**
 * @brief I2C lets multiple tasks communicate with I2C devices, serializing read and write operations.
 *
 * It is implemented on top of the ESP-IDF I2C master driver (driver/i2c_master.h).
 *
 * Example:
 *
 *     fabgl::I2C I2C;
 *
 *     extern "C" void app_main() {
 *       I2C.begin(GPIO_NUM_4, GPIO_NUM_15);
 *     }
 *
 * @ingroup comm
 */
class I2C {

public:

  /**
   * @brief I2C class constructor
   *
   * @param bus I2C bus to use: 0 or 1 (default is 0).
   */
  I2C(int bus = 0);

  ~I2C();

  /**
   * @brief Initializes the I2C instance, assigning GPIOs to the I2C signals.
   *
   * @param SDAGPIO GPIO for the SDA signal
   * @param SCLGPIO GPIO for the SCL signal
   *
   * @return True on success.
   *
   * Example:
   *
   *     // 4 = SDA, 15 = SCL (warning: disconnect the VGA connector)
   *     I2C.begin(GPIO_NUM_4, GPIO_NUM_15);
   */
  bool begin(gpio_num_t SDAGPIO, gpio_num_t SCLGPIO);

  void end();

  /**
   * @brief Sends a buffer over the I2C bus.
   *
   * This method is thread-safe.
   *
   * @param address Device address
   * @param buffer Buffer to send
   * @param size Number of bytes to send. Maximum size is I2C::getMaxBufferLength().
   * @param frequency Clock frequency
   * @param timeOutMS Timeout in milliseconds
   *
   * @return True on success.
   */
  bool write(int address, uint8_t * buffer, int size, int frequency = 100000, int timeOutMS = 50);

  /**
   * @brief Receives a buffer from the I2C bus.
   *
   * This method is thread-safe.
   *
   * @param address Device address
   * @param buffer Buffer that receives the data
   * @param size Number of bytes to receive. Maximum size is I2C::getMaxBufferLength().
   * @param frequency Clock frequency
   * @param timeOutMS Timeout in milliseconds
   *
   * @return Number of bytes actually received.
   */
  int read(int address, uint8_t * buffer, int size, int frequency = 100000, int timeOutMS = 50);

  /**
   * @brief Writes a buffer and then reads from the device using a repeated start condition.
   *
   * @param address Device address
   * @param writeBuffer Buffer to send
   * @param writeSize Number of bytes to send
   * @param readBuffer Buffer that receives the data
   * @param readSize Number of bytes to receive
   * @param frequency Clock frequency
   * @param timeOutMS Timeout in milliseconds
   *
   * @return True on success.
   */
  bool writeRead(int address, uint8_t const * writeBuffer, int writeSize, uint8_t * readBuffer, int readSize, int frequency = 100000, int timeOutMS = 50);

  /**
   * @brief Checks whether a device responds at the specified address.
   *
   * @param address Device address
   * @param timeOutMS Timeout in milliseconds
   *
   * @return True if the device acknowledged its address.
   */
  bool probe(int address, int timeOutMS = 50);

  /**
   * @brief Determines the maximum I2C buffer size.
   *
   * @return Maximum I2C buffer size.
   */
  int getMaxBufferLength() { return 128; }

  /**
   * @brief Returns the underlying ESP-IDF bus handle (nullptr if not initialized).
   */
  i2c_master_bus_handle_t busHandle() { return m_bus; }


private:

  static constexpr int MAXDEVICES = 8;

  struct DeviceEntry {
    i2c_master_dev_handle_t handle;
    uint16_t                address;
    uint32_t                frequency;
  };

  i2c_master_dev_handle_t getDevice(int address, int frequency);


  int                     m_busNum;
  i2c_master_bus_handle_t m_bus;
  SemaphoreHandle_t       m_mutex;
  DeviceEntry             m_devices[MAXDEVICES];
  int                     m_devicesCount;
};





} // end of namespace

