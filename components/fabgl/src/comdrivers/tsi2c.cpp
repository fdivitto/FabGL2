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
#include "freertos/task.h"

#include "esp_log.h"

#include "tsi2c.h"


#pragma GCC optimize ("O2")


namespace fabgl {



I2C::I2C(int bus)
  : m_busNum(bus),
    m_bus(nullptr),
    m_mutex(nullptr),
    m_devicesCount(0)
{
}


I2C::~I2C()
{
  end();
}


bool I2C::begin(gpio_num_t SDAGPIO, gpio_num_t SCLGPIO)
{
  end();

  i2c_master_bus_config_t config = { };
  config.i2c_port                     = m_busNum;
  config.sda_io_num                   = SDAGPIO;
  config.scl_io_num                   = SCLGPIO;
  config.clk_source                   = I2C_CLK_SRC_DEFAULT;
  config.glitch_ignore_cnt            = 7;
  config.flags.enable_internal_pullup = true;

  if (i2c_new_master_bus(&config, &m_bus) != ESP_OK) {
    ESP_LOGE("FabGL", "unable to init I2C");
    m_bus = nullptr;
    return false;
  }

  m_mutex = xSemaphoreCreateMutex();

  return true;
}


void I2C::end()
{
  for (int i = 0; i < m_devicesCount; ++i)
    i2c_master_bus_rm_device(m_devices[i].handle);
  m_devicesCount = 0;

  if (m_bus)
    i2c_del_master_bus(m_bus);
  m_bus = nullptr;

  if (m_mutex)
    vSemaphoreDelete(m_mutex);
  m_mutex = nullptr;
}


// m_mutex must be taken
i2c_master_dev_handle_t I2C::getDevice(int address, int frequency)
{
  for (int i = 0; i < m_devicesCount; ++i)
    if (m_devices[i].address == address && m_devices[i].frequency == frequency)
      return m_devices[i].handle;

  // remove oldest device if there isn't space
  if (m_devicesCount == MAXDEVICES) {
    i2c_master_bus_rm_device(m_devices[0].handle);
    memmove(m_devices, m_devices + 1, sizeof(DeviceEntry) * (MAXDEVICES - 1));
    --m_devicesCount;
  }

  i2c_device_config_t devConfig = { };
  devConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  devConfig.device_address  = address;
  devConfig.scl_speed_hz    = frequency;

  i2c_master_dev_handle_t handle;
  if (i2c_master_bus_add_device(m_bus, &devConfig, &handle) != ESP_OK)
    return nullptr;

  m_devices[m_devicesCount++] = { handle, (uint16_t) address, (uint32_t) frequency };
  return handle;
}


bool I2C::write(int address, uint8_t * buffer, int size, int frequency, int timeOutMS)
{
  if (!m_bus)
    return false;
  bool ret = false;
  xSemaphoreTake(m_mutex, portMAX_DELAY);
  auto dev = getDevice(address, frequency);
  if (dev)
    ret = (i2c_master_transmit(dev, buffer, size, timeOutMS) == ESP_OK);
  xSemaphoreGive(m_mutex);
  return ret;
}


int I2C::read(int address, uint8_t * buffer, int size, int frequency, int timeOutMS)
{
  if (!m_bus)
    return 0;
  int ret = 0;
  xSemaphoreTake(m_mutex, portMAX_DELAY);
  auto dev = getDevice(address, frequency);
  if (dev && i2c_master_receive(dev, buffer, size, timeOutMS) == ESP_OK)
    ret = size;
  xSemaphoreGive(m_mutex);
  return ret;
}


bool I2C::writeRead(int address, uint8_t const * writeBuffer, int writeSize, uint8_t * readBuffer, int readSize, int frequency, int timeOutMS)
{
  if (!m_bus)
    return false;
  bool ret = false;
  xSemaphoreTake(m_mutex, portMAX_DELAY);
  auto dev = getDevice(address, frequency);
  if (dev)
    ret = (i2c_master_transmit_receive(dev, writeBuffer, writeSize, readBuffer, readSize, timeOutMS) == ESP_OK);
  xSemaphoreGive(m_mutex);
  return ret;
}


bool I2C::probe(int address, int timeOutMS)
{
  if (!m_bus)
    return false;
  xSemaphoreTake(m_mutex, portMAX_DELAY);
  bool ret = (i2c_master_probe(m_bus, address, timeOutMS) == ESP_OK);
  xSemaphoreGive(m_mutex);
  return ret;
}



} // end of namespace

