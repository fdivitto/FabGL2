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
 * @brief This file contains the fabgl::ICMP definition (ping).
 */


#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_netif_ip_addr.h"

#include "fabglconf.h"
#include "fabutils.h"


namespace fabgl {


/**
 * @brief ICMP echo (ping) helper class.
 *
 * Implemented on top of the ESP-IDF "ping_sock" (esp_ping) API.
 *
 * Example:
 *
 *     fabgl::ICMP icmp;
 *     int t = icmp.ping("www.google.com");
 *     if (t >= 0)
 *       printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n", icmp.receivedBytes(), icmp.hostIPString(), icmp.receivedSeq(), icmp.receivedTTL(), t / 1000.0);
 *
 * @ingroup network
 */
class ICMP {

public:

  ICMP();
  ~ICMP();

  /**
   * @brief Sends an echo request and waits for the echo reply
   *
   * @param dest Destination IPv4 address
   *
   * @return Measured echo time in microseconds, -1 on timeout or error
   */
  int ping(esp_ip4_addr_t const & dest);

  /**
   * @brief Sends an echo request and waits for the echo reply
   *
   * @param host Destination host name or IP address (dotted string)
   *
   * @return Measured echo time in microseconds, -1 on timeout or error, -2 if the name cannot be resolved
   */
  int ping(char const * host);

  int receivedBytes()             { return m_receivedBytes; }
  int receivedTTL()               { return m_receivedTTL; }
  int receivedSeq()               { return m_waitingSeq; }
  esp_ip4_addr_t const & hostIP() { return m_destIP; }

  /**
   * @brief Returns the last destination IP address as a dotted string
   */
  char const * hostIPString();

private:

  static void onPingSuccess(void * handle, void * args);
  static void onPingTimeout(void * handle, void * args);
  static void onPingEnd(void * handle, void * args);

  esp_ip4_addr_t    m_destIP;
  SemaphoreHandle_t m_done;
  int               m_waitingSeq;
  int               m_receivedBytes;
  int               m_receivedTTL;
  int               m_elapsedMS;
  bool              m_success;
  char              m_IPString[16];
};


} // end of namespace
