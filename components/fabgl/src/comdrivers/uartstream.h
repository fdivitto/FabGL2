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
 * @brief This file contains the fabgl::UARTStream definition.
 */


#include <stdint.h>
#include <stddef.h>

#include "driver/uart.h"

#include "fabutils.h"


namespace fabgl {


/**
 * @brief A Stream implemented on top of the ESP-IDF UART driver.
 *
 * It can be used wherever FabGL requires a Stream object (e.g. Terminal::setLogStream()). On UART 0 it
 * shares the ESP-IDF console (the one used by printf() and "idf.py monitor").
 *
 * Example:
 *
 *     fabgl::UARTStream Console;
 *
 *     extern "C" void app_main() {
 *       Console.begin();                 // UART0, 115200 baud, console pins
 *       Terminal.setLogStream(Console);
 *       ...
 *       if (Console.available())
 *         int c = Console.read();
 *     }
 *
 * @ingroup comm
 */
class UARTStream : public Stream {

public:

  UARTStream();
  ~UARTStream();

  /**
   * @brief Initializes the UART
   *
   * @param port UART port (0, 1 or 2)
   * @param baud Baud rate. When 0 and port is the console UART, the baud rate is not changed.
   * @param rxPin RX GPIO (-1 = do not change, or default for console)
   * @param txPin TX GPIO (-1 = do not change, or default for console)
   * @param rxBufferSize Size of the RX buffer
   * @param txBufferSize Size of the TX buffer (0 = blocking write)
   *
   * @return True on success
   */
  bool begin(int port = 0, int baud = 0, int rxPin = -1, int txPin = -1, int rxBufferSize = 1024, int txBufferSize = 0);

  /**
   * @brief Uninstalls the UART driver
   */
  void end();

  // Stream
  int available();
  int read();
  int peek();

  // Print
  size_t write(uint8_t c);
  size_t write(const uint8_t * buffer, size_t size);
  using Print::write;

  /**
   * @brief Waits until all data has been transmitted
   */
  void flush();

  uart_port_t port() { return m_port; }

private:

  uart_port_t m_port;
  bool        m_installed;
  int         m_peek;
};


} // end of namespace
