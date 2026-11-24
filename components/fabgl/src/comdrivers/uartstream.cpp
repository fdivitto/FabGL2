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


#include "uartstream.h"


namespace fabgl {


UARTStream::UARTStream()
  : m_port(UART_NUM_0),
    m_installed(false),
    m_peek(-1)
{
}


UARTStream::~UARTStream()
{
  end();
}


bool UARTStream::begin(int port, int baud, int rxPin, int txPin, int rxBufferSize, int txBufferSize)
{
  end();
  m_port = (uart_port_t) port;

  if (baud > 0 || port != 0) {
    uart_config_t config = { };
    config.baud_rate  = baud > 0 ? baud : 115200;
    config.data_bits  = UART_DATA_8_BITS;
    config.parity     = UART_PARITY_DISABLE;
    config.stop_bits  = UART_STOP_BITS_1;
    config.flow_ctrl  = UART_HW_FLOWCTRL_DISABLE;
    config.source_clk = UART_SCLK_DEFAULT;
    if (uart_param_config(m_port, &config) != ESP_OK)
      return false;
  }
  if (rxPin >= 0 || txPin >= 0)
    uart_set_pin(m_port, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

  if (!uart_is_driver_installed(m_port)) {
    if (uart_driver_install(m_port, tmax(rxBufferSize, 256), txBufferSize > 0 ? tmax(txBufferSize, 256) : 0, 0, nullptr, 0) != ESP_OK)
      return false;
    m_installed = true;
  }
  m_peek = -1;
  return true;
}


void UARTStream::end()
{
  if (m_installed) {
    uart_driver_delete(m_port);
    m_installed = false;
  }
}


int UARTStream::available()
{
  size_t len = 0;
  uart_get_buffered_data_len(m_port, &len);
  return len + (m_peek >= 0 ? 1 : 0);
}


int UARTStream::read()
{
  if (m_peek >= 0) {
    int c = m_peek;
    m_peek = -1;
    return c;
  }
  uint8_t c;
  return uart_read_bytes(m_port, &c, 1, 0) == 1 ? c : -1;
}


int UARTStream::peek()
{
  if (m_peek < 0) {
    uint8_t c;
    if (uart_read_bytes(m_port, &c, 1, 0) == 1)
      m_peek = c;
  }
  return m_peek;
}


size_t UARTStream::write(uint8_t c)
{
  return write(&c, 1);
}


size_t UARTStream::write(const uint8_t * buffer, size_t size)
{
  int r = uart_write_bytes(m_port, buffer, size);
  return r > 0 ? r : 0;
}


void UARTStream::flush()
{
  uart_wait_tx_done(m_port, portMAX_DELAY);
}


} // end of namespace
