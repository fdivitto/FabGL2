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


#include <string.h>

#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "ping/ping_sock.h"

#include "ICMP.h"


namespace fabgl {


ICMP::ICMP()
  : m_destIP({ 0 }),
    m_done(xSemaphoreCreateBinary()),
    m_waitingSeq(-1),
    m_receivedBytes(0),
    m_receivedTTL(0),
    m_elapsedMS(0),
    m_success(false)
{
  m_IPString[0] = 0;
}


ICMP::~ICMP()
{
  vSemaphoreDelete(m_done);
}


// -1 = timeout
// -2 = cannot resolve name
int ICMP::ping(char const * host)
{
  addrinfo hints = { };
  hints.ai_family = AF_INET;
  addrinfo * res = nullptr;
  if (getaddrinfo(host, nullptr, &hints, &res) != 0 || res == nullptr)
    return -2;
  esp_ip4_addr_t dest;
  dest.addr = ((sockaddr_in *) res->ai_addr)->sin_addr.s_addr;
  freeaddrinfo(res);
  return ping(dest);
}


// -1 = timeout
int ICMP::ping(esp_ip4_addr_t const & dest)
{
  m_destIP  = dest;
  m_success = false;

  ++m_waitingSeq;

  esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
  config.count       = 1;
  config.timeout_ms  = 1000;
  config.data_size   = 32;
  config.target_addr.type            = IPADDR_TYPE_V4;
  config.target_addr.u_addr.ip4.addr = dest.addr;

  esp_ping_callbacks_t cbs = { };
  cbs.cb_args         = this;
  cbs.on_ping_success = onPingSuccess;
  cbs.on_ping_timeout = onPingTimeout;
  cbs.on_ping_end     = onPingEnd;

  esp_ping_handle_t ping;
  if (esp_ping_new_session(&config, &cbs, &ping) != ESP_OK)
    return -1;
  esp_ping_start(ping);
  xSemaphoreTake(m_done, pdMS_TO_TICKS(config.timeout_ms + 1000));
  esp_ping_stop(ping);
  esp_ping_delete_session(ping);

  return m_success ? m_elapsedMS * 1000 : -1;
}


void ICMP::onPingSuccess(void * handle, void * args)
{
  auto ths = (ICMP *) args;
  uint8_t  ttl;
  uint32_t elapsed, size;
  esp_ping_get_profile(handle, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
  esp_ping_get_profile(handle, ESP_PING_PROF_SIZE, &size, sizeof(size));
  esp_ping_get_profile(handle, ESP_PING_PROF_TIMEGAP, &elapsed, sizeof(elapsed));
  ths->m_receivedTTL   = ttl;
  ths->m_receivedBytes = size;
  ths->m_elapsedMS     = elapsed;
  ths->m_success       = true;
}


void ICMP::onPingTimeout(void * handle, void * args)
{
  auto ths = (ICMP *) args;
  ths->m_success = false;
}


void ICMP::onPingEnd(void * handle, void * args)
{
  auto ths = (ICMP *) args;
  xSemaphoreGive(ths->m_done);
}


char const * ICMP::hostIPString()
{
  snprintf(m_IPString, sizeof(m_IPString), IPSTR, IP2STR(&m_destIP));
  return m_IPString;
}


} // end of namespace
