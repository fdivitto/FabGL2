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
 * @brief This file contains the fabgl::WiFiScanner, fabgl::WiFiConnection, fabgl::HTTPRequest and fabgl::TCPClient definitions.
 */


#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "fabglconf.h"
#include "fabutils.h"



namespace fabgl {



///////////////////////////////////////////////////////////////////////////////////
// WiFiScanner

/**
* @brief WiFi scanner helper class
*
* WiFiScanner scans for nearby WiFi networks.
*
* Example:
*
*     fabgl::WiFiScanner scanner;
*     if (scanner.scan())
*       for (int i = 0; i < scanner.count(); ++i) {
*         auto item = scanner.get(i);
*         Term.printf("#%d %s %d dBm\r\n", i, item->ssid, item->rssi);
*       }
*
* @ingroup network
*/
class WiFiScanner {

public:

  WiFiScanner();
  ~WiFiScanner();

  /**
   * @brief Performs a WiFi scan
   *
   * @param maxItems Maximum number of networks to return
   * @param justCount If true, networks are only counted (WiFiScanner::get() returns nullptr and maxItems is ignored)
   *
   * @return True on success
   */
  bool scan(int maxItems = 8, bool justCount = false);

  /**
   * @brief Returns the number of networks found (limited to the requested maximum)
   *
   * @return Number of networks found, limited to the requested maximum
   */
  int count() { return m_count; }

  /**
   * @brief Returns information about the specified network
   *
   * @param index Index of the network (0 = first network)
   *
   * @return WiFi network information, or nullptr if the justCount parameter of scan() was true
   */
  wifi_ap_record_t * get(int index) { return m_items ? m_items + index : nullptr; }

  /**
   * @brief Frees the resources used to scan WiFi networks
   */
  void cleanUp();

  /**
   * @brief Returns a textual description of an authentication mode
   */
  static char const * authModeStr(wifi_auth_mode_t mode);

private:

  wifi_ap_record_t * m_items;
  uint16_t           m_count;
};



///////////////////////////////////////////////////////////////////////////////////
// WiFiConnection


/** \ingroup Enumerations
* @brief fabgl::WiFiConnection states
*/
enum class WiFiConnectionState {
  Disconnected,     /**< Disconnected or unable to connect */
  ConnectingWiFi,   /**< Connection in progress */
  WaitingIP,        /**< Connected, waiting for an IP address */
  Connected,        /**< Successfully connected */
};


/**
* @brief WiFi connection helper class
*
* WiFiConnection establishes and maintains a connection with a WiFi network.
*
* Example:
*
*     auto nc = fabgl::WiFiConnection();
*     if (nc.connect("MySSID", "MyPassword")) {
*       Term.printf("Connected. IP:%d.%d.%d.%d NMASK:%d.%d.%d.%d GATEW:%d.%d.%d.%d\r\n", IP2STR(&nc.IP()), IP2STR(&nc.netmask()), IP2STR(&nc.gateway()));
*
*       // ...use the network (e.g. with fabgl::HTTPRequest)
*
*       // now disconnect
*       nc.disconnect();
*     }
*
* @ingroup network
*/
class WiFiConnection {

public:

  WiFiConnection();
  ~WiFiConnection();

  /**
   * @brief Tries to connect to a WiFi network.
   *
   * @param ssid SSID of the WiFi network
   * @param password Password of the WiFi network
   * @param waitConnectionTimeOutMS Timeout in milliseconds
   *
   * @return WiFiConnectionState::Connected on success
   */
  WiFiConnectionState connect(char const * ssid, char const * password, int waitConnectionTimeOutMS = 10000);

  /**
   * @brief Disconnects from the WiFi network
   */
  void disconnect();

  /**
   * @brief Returns the connection state
   *
   * @return Current connection state
   */
  WiFiConnectionState state() { return m_state; }

  /**
   * @brief Returns the IP address
   *
   * @return Acquired IP address
   */
  esp_ip4_addr_t const & IP() { return m_IP; }

  /**
   * @brief Returns the netmask
   *
   * @return Acquired netmask
   */
  esp_ip4_addr_t const & netmask() { return m_netmask; }

  /**
   * @brief Returns the gateway address
   *
   * @return Gateway address
   */
  esp_ip4_addr_t const & gateway() { return m_gateway; }

  /**
   * @brief Returns the current IP address as a dotted string
   */
  char const * IPString();

  /**
   * @brief Returns the SSID of the network
   */
  char const * SSID() { return m_SSID; }

  /**
   * @brief Returns the RSSI (signal strength in dBm) of the connected access point, or 0 if not connected
   */
  int RSSI();

  /**
   * @brief Returns the global (last) connection object, if any
   */
  static WiFiConnection * instance() { return s_instance; }


private:

  static void wifiEventStaConnected(void * arg, esp_event_base_t event_base, int32_t event_id, void * event_data);
  static void wifiEventStaDisconnected(void * arg, esp_event_base_t event_base, int32_t event_id, void * event_data);
  static void ipEventStaGotIP(void * arg, esp_event_base_t event_base, int32_t event_id, void * event_data);

  esp_netif_t *        m_netif;
  WiFiConnectionState  m_state;
  esp_ip4_addr_t       m_IP;
  esp_ip4_addr_t       m_netmask;
  esp_ip4_addr_t       m_gateway;
  char                 m_SSID[33];
  char                 m_IPString[16];

  static WiFiConnection * s_instance;
};



///////////////////////////////////////////////////////////////////////////////////
// HTTPRequest


/**
* @brief HTTP connection and request helper
*
* HTTPRequest connects to a web server and downloads a page or a file.
*
* Example:
*
*     auto nc = fabgl::WiFiConnection();
*     if (nc.connect("MySSID", "MyPassword")) {
*       fabgl::HTTPRequest req;
*       // just count the received bytes
*       int received = 0;
*       req.onData = [&](void const * data, int len) {
*         received += len;
*       };
*       // start GET method
*       if (req.GET("http://www.fabglib.org/schema_audio.png") == 200)
*         Term.printf("Received %d of %d bytes\r\n", received, req.contentLength());
*     }
*
* @ingroup network
*/
class HTTPRequest {

public:

  HTTPRequest();
  ~HTTPRequest();

  /**
   * @brief Performs a GET request
   *
   * The same fabgl::HTTPRequest object can issue multiple consecutive requests.
   *
   * @param URL The page or file to retrieve
   *
   * @return The HTTP status code (0 = connection failure)
   */
  int GET(char const * URL);

  /**
   * @brief Gets the content length
   *
   * @return Content length as reported by the web server
   */
  int contentLength() { return m_client ? (int) esp_http_client_get_content_length(m_client) : -1; }

  /**
   * @brief Opens a streaming GET request (follows redirects)
   *
   * After this call, use read() to get the content and close() to release the connection.
   * HTTPS is supported through the ESP-IDF certificate bundle.
   *
   * Example:
   *
   *     fabgl::HTTPRequest http;
   *     if (http.open("http://www.fabglib.org/index.html") == 200) {
   *       uint8_t buf[512];
   *       int len;
   *       while ((len = http.read(buf, sizeof(buf))) > 0)
   *         fwrite(buf, 1, len, file);
   *     }
   *     http.close();
   *
   * To resume an interrupted download, pass the number of bytes already received as rangeStart: the request
   * then asks only the remaining part (HTTP Range header) and a server that supports it replies 206
   * (Partial Content), with contentLength() reporting the remaining length.
   *
   * @param URL The page or file to retrieve
   * @param rangeStart First byte to retrieve (0 = whole content)
   *
   * @return The HTTP status code (0 = connection failure)
   */
  int open(char const * URL, int64_t rangeStart = 0);

  /**
   * @brief Reads content after a successful open()
   *
   * A connection closed by the server before the end of the content also returns 0: compare the received
   * bytes with contentLength() to know whether the content is complete.
   *
   * @param buffer Destination buffer
   * @param length Buffer size
   *
   * @return Number of bytes read, 0 when no more data is available, negative on error or when no data arrives
   *         within the timeout (see setTimeout())
   */
  int read(void * buffer, int length);

  /**
   * @brief Sets the network timeout of the next open() and GET()
   *
   * @param timeoutMS Maximum time in milliseconds to wait for the connection or for incoming data (default 10000)
   */
  void setTimeout(int timeoutMS) { m_timeoutMS = timeoutMS; }

  /**
   * @brief Closes the HTTP connection
   */
  void close();


  // delegates
  
  /**
   * @brief Data received delegate
   *
   * This delegate is called whenever a block of data has been received.
   * The first parameter is a pointer to the received buffer.
   * The second parameter is the buffer length in bytes.
   */
  Delegate<void const *, int> onData; // (data, len)


private:

  static esp_err_t httpEventHandler(esp_http_client_event_t * evt);

  esp_http_client_handle_t m_client;
  int                      m_timeoutMS;

};





///////////////////////////////////////////////////////////////////////////////////
// TCPClient


/**
 * @brief Simple TCP client based on BSD sockets (lwIP)
 *
 * Example:
 *
 *     fabgl::TCPClient client;
 *     if (client.connect("towel.blinkenlights.nl", 23)) {
 *       while (client.connected()) {
 *         if (client.available())
 *           putchar(client.read());
 *       }
 *     }
 *
 * @ingroup network
 */
class TCPClient {

public:

  TCPClient();
  ~TCPClient();

  /**
   * @brief Connects to the specified host and port
   *
   * @param host Host name or IP address (dotted string)
   * @param port TCP port
   * @param timeOutMS Connection timeout in milliseconds
   *
   * @return True on success
   */
  bool connect(char const * host, int port, int timeOutMS = 5000);

  /**
   * @brief Closes the connection
   */
  void stop();

  /**
   * @brief Determines whether the connection is still open (or received data is still pending)
   */
  bool connected();

  /**
   * @brief Returns the number of bytes available for reading
   */
  int available();

  /**
   * @brief Reads one byte (returns -1 if no data is available)
   */
  int read();

  /**
   * @brief Reads up to len bytes
   *
   * @return Number of bytes read (0 if no data is available)
   */
  int read(uint8_t * buffer, size_t len);

  /**
   * @brief Sends one byte
   */
  size_t write(uint8_t c) { return write(&c, 1); }

  /**
   * @brief Sends a buffer
   *
   * @return Number of sent bytes
   */
  size_t write(uint8_t const * buffer, size_t len);

  /**
   * @brief Sends a zero-terminated string
   */
  size_t write(char const * str) { return write((uint8_t const *) str, strlen(str)); }

  /**
   * @brief Sends a buffer of chars
   */
  size_t write(char const * buffer, size_t len) { return write((uint8_t const *) buffer, len); }

  /**
   * @brief Returns the socket descriptor (-1 if not connected)
   */
  int socket() { return m_socket; }

private:

  bool fillBuffer();

  int     m_socket;
  int     m_peek;     // -1 = no char in m_peek
  bool    m_closed;   // true when remote host has closed the connection
};


} // end of namespace

