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
#include <algorithm>

#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "nvs_flash.h"
#include "esp_crt_bundle.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include "fabutils.h"

#include "netutils.h"



namespace fabgl {


// WiFi requires NVS (for PHY calibration data and, optionally, WiFi settings)
static void initNVS()
{
  auto r = nvs_flash_init();
  if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
  }
}



////////////////////////////////////////////////////////////////////////////////////////////
// WiFiScanner


WiFiScanner::WiFiScanner()
  : m_items(nullptr), m_count(0)
{
}


WiFiScanner::~WiFiScanner()
{
  cleanUp();
}


// if justCount = true, maxItems is ignored
bool WiFiScanner::scan(int maxItems, bool justCount)
{
  // cannot scan using a separate WiFi instance while connected
  if (WiFiConnection::instance() && WiFiConnection::instance()->state() != WiFiConnectionState::Disconnected) {
    auto r = esp_wifi_scan_start(nullptr, true);
    if (r == ESP_OK) {
      cleanUp();
      esp_wifi_scan_get_ap_num(&m_count);
      if (!justCount) {
        m_count = std::min<uint16_t>(m_count, maxItems);
        m_items = new wifi_ap_record_t[m_count];
        uint16_t tempCount = m_count;
        esp_wifi_scan_get_ap_records(&tempCount, m_items);
      }
    }
    esp_wifi_clear_ap_list();
    return r == ESP_OK;
  }

  initNVS();

  esp_event_loop_create_default();

  // init with reduced memory footprint
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();  
  cfg.static_rx_buf_num = 2;
  cfg.static_tx_buf_num = 1;
  cfg.ampdu_rx_enable = cfg.ampdu_tx_enable = cfg.amsdu_tx_enable = 0;
  esp_wifi_init(&cfg);

  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_start();
  
  auto r = esp_wifi_scan_start(nullptr, true);

  if (r == ESP_OK) {
    cleanUp();
    esp_wifi_scan_get_ap_num(&m_count);
    if (!justCount) {
      m_count = std::min<uint16_t>(m_count, maxItems);
      m_items = new wifi_ap_record_t[m_count];
      uint16_t tempCount = m_count;
      esp_wifi_scan_get_ap_records(&tempCount, m_items);  // use tempCount because esp_wifi_scan_get_ap_records() changes it on exit (standing to the docs)
    }
  }

  esp_wifi_clear_ap_list(); // needed if esp_wifi_scan_get_ap_records() is not used
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_event_loop_delete_default();

  return r == ESP_OK;
}


void WiFiScanner::cleanUp()
{
  if (m_items)
    delete [] m_items;
  m_items = nullptr;
}


char const * WiFiScanner::authModeStr(wifi_auth_mode_t mode)
{
  switch (mode) {
    case WIFI_AUTH_OPEN:            return "Open";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA-PSK";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2-PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/WPA2-PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-ENTERPRISE";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3-PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/WPA3-PSK";
    default:                        return "Other";
  }
}



////////////////////////////////////////////////////////////////////////////////////////////
// WiFiConnection


WiFiConnection * WiFiConnection::s_instance = nullptr;


WiFiConnection::WiFiConnection()
  : m_netif(nullptr), m_state(WiFiConnectionState::Disconnected), m_IP({0}), m_netmask({0}), m_gateway({0})
{
  m_SSID[0]     = 0;
  m_IPString[0] = 0;
  s_instance    = this;
}


WiFiConnection::~WiFiConnection()
{
  disconnect();
  if (s_instance == this)
    s_instance = nullptr;
}


char const * WiFiConnection::IPString()
{
  snprintf(m_IPString, sizeof(m_IPString), IPSTR, IP2STR(&m_IP));
  return m_IPString;
}


int WiFiConnection::RSSI()
{
  wifi_ap_record_t info;
  if (m_state == WiFiConnectionState::Connected && esp_wifi_sta_get_ap_info(&info) == ESP_OK)
    return info.rssi;
  return 0;
}


WiFiConnectionState WiFiConnection::connect(char const * ssid, char const * password, int waitConnectionTimeOutMS)
{
  if (m_state == WiFiConnectionState::Disconnected) {

    strlcpy(m_SSID, ssid, sizeof(m_SSID));

    initNVS();
    esp_netif_init();
    esp_event_loop_create_default();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    m_state = WiFiConnectionState::ConnectingWiFi;

    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_WIFI_STA();
    m_netif = esp_netif_new(&netif_config);

    esp_netif_attach_wifi_station(m_netif);
    esp_wifi_set_default_wifi_sta_handlers();

    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifiEventStaDisconnected, this);
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &wifiEventStaConnected, this);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ipEventStaGotIP, this);
    
    esp_wifi_set_storage(WIFI_STORAGE_RAM);

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    if (password)
      strlcpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();

  }

  TimeOut timeout;
  while (m_state != WiFiConnectionState::Connected && !timeout.expired(waitConnectionTimeOutMS))
    vTaskDelay(100 / portTICK_PERIOD_MS);

  return m_state;
}


void WiFiConnection::disconnect()
{
  if (m_state != WiFiConnectionState::Disconnected) {
    esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifiEventStaDisconnected);
    esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &wifiEventStaConnected);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &ipEventStaGotIP);
    esp_wifi_stop();
    esp_wifi_clear_default_wifi_driver_and_handlers(m_netif);
    esp_netif_destroy(m_netif);
    m_netif = nullptr;
    esp_wifi_deinit();
    esp_netif_deinit();
    esp_event_loop_delete_default();
    m_state = WiFiConnectionState::Disconnected;
  }
}


void WiFiConnection::wifiEventStaConnected(void * arg, esp_event_base_t event_base, int32_t event_id, void * event_data)
{
  auto obj = (WiFiConnection *)arg;
  if (obj->m_state == WiFiConnectionState::ConnectingWiFi)
    obj->m_state = WiFiConnectionState::WaitingIP;
}


void WiFiConnection::wifiEventStaDisconnected(void * arg, esp_event_base_t event_base, int32_t event_id, void * event_data)
{
  auto obj = (WiFiConnection *)arg;
  if (obj->m_state == WiFiConnectionState::ConnectingWiFi || obj->m_state == WiFiConnectionState::Connected) {
      // tries to reconnect
      esp_wifi_connect();
  }
}


void WiFiConnection::ipEventStaGotIP(void * arg, esp_event_base_t event_base, int32_t event_id, void * event_data)
{
  auto obj = (WiFiConnection *)arg;
  if (obj->m_state == WiFiConnectionState::WaitingIP) {
    auto event = (ip_event_got_ip_t *)event_data;
    obj->m_IP      = event->ip_info.ip;
    obj->m_netmask = event->ip_info.netmask;
    obj->m_gateway = event->ip_info.gw;
    obj->m_state   = WiFiConnectionState::Connected;
  }
}



////////////////////////////////////////////////////////////////////////////////////////////
// HTTPRequest


HTTPRequest::HTTPRequest()
  : m_client(nullptr)
{
}


HTTPRequest::~HTTPRequest()
{
  close();
}


esp_err_t HTTPRequest::httpEventHandler(esp_http_client_event_t * evt)
{
  auto obj = (HTTPRequest *) evt->user_data;

  switch(evt->event_id) {
    case HTTP_EVENT_ERROR:
      break;
    case HTTP_EVENT_ON_CONNECTED:
      break;
    case HTTP_EVENT_ON_DATA:
      obj->onData(evt->data, evt->data_len);
      break;
    case HTTP_EVENT_ON_FINISH:
      break;
    case HTTP_EVENT_DISCONNECTED:
      break;
    default:
      break;
  }
  
  return ESP_OK;
}


int HTTPRequest::GET(char const * URL)
{
  if (m_client) {
    esp_http_client_set_url(m_client, URL);
  } else {
    esp_http_client_config_t config = { };
    config.url               = URL;
    config.event_handler     = httpEventHandler;
    config.user_data         = this;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    m_client = esp_http_client_init(&config);
  }  
  return esp_http_client_perform(m_client) == ESP_OK ? esp_http_client_get_status_code(m_client) : 0;
}


int HTTPRequest::open(char const * URL)
{
  close();

  esp_http_client_config_t config = { };
  config.url                   = URL;
  config.crt_bundle_attach     = esp_crt_bundle_attach;
  config.disable_auto_redirect = true;
  config.buffer_size           = 2048;
  m_client = esp_http_client_init(&config);
  if (!m_client)
    return 0;

  for (int redirects = 0; redirects < 8; ++redirects) {
    if (esp_http_client_open(m_client, 0) != ESP_OK)
      return 0;
    if (esp_http_client_fetch_headers(m_client) < 0)
      return 0;
    int status = esp_http_client_get_status_code(m_client);
    if (status == 301 || status == 302 || status == 303 || status == 307 || status == 308) {
      esp_http_client_set_redirection(m_client);
      esp_http_client_close(m_client);
      continue;
    }
    return status;
  }
  return 0;
}


int HTTPRequest::read(void * buffer, int length)
{
  if (!m_client)
    return -1;
  return esp_http_client_read(m_client, (char *) buffer, length);
}


void HTTPRequest::close()
{
  if (m_client) {
    esp_http_client_close(m_client);
    esp_http_client_cleanup(m_client);
    m_client = nullptr;
  }
}




////////////////////////////////////////////////////////////////////////////////////////////
// TCPClient


TCPClient::TCPClient()
  : m_socket(-1),
    m_peek(-1),
    m_closed(false)
{
}


TCPClient::~TCPClient()
{
  stop();
}


bool TCPClient::connect(char const * host, int port, int timeOutMS)
{
  stop();

  addrinfo hints = { };
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo * res = nullptr;
  char portStr[8];
  snprintf(portStr, sizeof(portStr), "%d", port);
  if (getaddrinfo(host, portStr, &hints, &res) != 0 || res == nullptr)
    return false;

  m_socket = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (m_socket < 0) {
    freeaddrinfo(res);
    return false;
  }

  // non blocking connect with timeout
  fcntl(m_socket, F_SETFL, fcntl(m_socket, F_GETFL, 0) | O_NONBLOCK);
  int r = ::connect(m_socket, res->ai_addr, res->ai_addrlen);
  freeaddrinfo(res);
  if (r < 0 && errno != EINPROGRESS) {
    stop();
    return false;
  }
  if (r < 0) {
    fd_set wset;
    FD_ZERO(&wset);
    FD_SET(m_socket, &wset);
    timeval tv = { timeOutMS / 1000, (timeOutMS % 1000) * 1000 };
    if (select(m_socket + 1, nullptr, &wset, nullptr, &tv) <= 0) {
      stop();
      return false;
    }
    int err = 0;
    socklen_t len = sizeof(err);
    getsockopt(m_socket, SOL_SOCKET, SO_ERROR, &err, &len);
    if (err != 0) {
      stop();
      return false;
    }
  }

  int one = 1;
  setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

  m_closed = false;
  m_peek   = -1;
  return true;
}


void TCPClient::stop()
{
  if (m_socket >= 0) {
    ::close(m_socket);
    m_socket = -1;
  }
  m_peek   = -1;
  m_closed = false;
}


// try to fill m_peek (non blocking)
bool TCPClient::fillBuffer()
{
  if (m_peek < 0 && m_socket >= 0 && !m_closed) {
    uint8_t c;
    int r = ::recv(m_socket, &c, 1, MSG_DONTWAIT);
    if (r == 1)
      m_peek = c;
    else if (r == 0 || (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
      m_closed = true;
  }
  return m_peek >= 0;
}


bool TCPClient::connected()
{
  fillBuffer();
  return m_socket >= 0 && (!m_closed || m_peek >= 0);
}


int TCPClient::available()
{
  if (m_socket < 0)
    return 0;
  int count = 0;
  ioctl(m_socket, FIONREAD, &count);
  if (m_peek >= 0)
    ++count;
  else if (count == 0 && fillBuffer())
    count = 1;
  return count;
}


int TCPClient::read()
{
  if (fillBuffer()) {
    int c = m_peek;
    m_peek = -1;
    return c;
  }
  return -1;
}


int TCPClient::read(uint8_t * buffer, size_t len)
{
  if (len == 0 || m_socket < 0)
    return 0;
  size_t count = 0;
  if (m_peek >= 0) {
    buffer[count++] = m_peek;
    m_peek = -1;
  }
  if (count < len && !m_closed) {
    int r = ::recv(m_socket, buffer + count, len - count, MSG_DONTWAIT);
    if (r > 0)
      count += r;
    else if (r == 0 || (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
      m_closed = true;
  }
  return count;
}


size_t TCPClient::write(uint8_t const * buffer, size_t len)
{
  if (m_socket < 0)
    return 0;
  size_t sent = 0;
  while (sent < len) {
    int r = ::send(m_socket, buffer + sent, len - sent, 0);
    if (r > 0)
      sent += r;
    else if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
      vTaskDelay(1);
    else {
      m_closed = true;
      break;
    }
  }
  return sent;
}


} // end of namespace
