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
 * @brief This file contains the fabgl::SerialPort definition.
 */


#include "freertos/FreeRTOS.h"

#include "soc/uart_struct.h"

#include "fabutils.h"
#include "fabglconf.h"
#include "terminal.h"


namespace fabgl {



/** \ingroup Enumerations
 * @brief Serial port flow control methods
 */
enum class FlowControl {
  None,              /**< No flow control */
  Software,          /**< Software flow control, using the XON and XOFF control characters */
  Hardware,          /**< Hardware flow control, using the RTS and CTS signals */
  Hardsoft,          /**< Hardware and software flow control, using XON/XOFF and RTS/CTS */
};


/**
 * @brief SerialPort handles UART configuration and communication
 *
 * @ingroup comm
 */
class SerialPort {
public:

  typedef void (*RXCallback)(void * args, uint8_t value, bool fromISR);
  typedef bool (*RXReadyCallback)(void * args, bool fromISR);
  typedef void (*LineStatusCallback)(void * args, bool parityError, bool framingError, bool overflowError, bool breakDetected, bool fromISR);

  SerialPort();
  
  void setCallbacks(void * args, RXReadyCallback rxReadyCallback, RXCallback rxCallback, LineStatusCallback lineStatusCallback = nullptr);

  /**
   * @brief Assigns GPIOs to the serial port signals
   *
   * This method must be called before setup(), and only once.
   *
   * @param rxPin GPIO of the UART RX pin.
   * @param txPin GPIO of the UART TX pin.
   * @param rtsPin RTS signal GPIO number (-1 = not used)
   * @param ctsPin CTS signal GPIO number (-1 = not used)
   * @param dtrPin DTR signal GPIO number (-1 = not used)
   * @param dsrPin DSR signal GPIO number (-1 = not used)
   * @param dcdPin DCD signal GPIO number (-1 = not used)
   * @param riPin RI signal GPIO number (-1 = not used)
   *
   * Example:
   *
   *     serialPort.setSignals(34, 2);  // rx = GPIO 34, tx = GPIO 2
   *     serialPort.setup(2, 115200, 8, 'N', 1, FlowControl::Software);
   */
  void setSignals(int rxPin, int txPin, int rtsPin = -1, int ctsPin = -1, int dtrPin = -1, int dsrPin = -1, int dcdPin = -1, int riPin = -1);
  
  /**
   * @brief Configures and activates the specified UART
   *
   * This method can be called again whenever a parameter needs to be changed. setSignals() must be called first.
   *
   * @param uartIndex UART device to use (0, 1 or 2)
   * @param baud Baud rate.
   * @param dataLength Data word length. 5 = 5 bits, 6 = 6 bits, 7 = 7 bits, 8 = 8 bits
   * @param parity Parity. 'N' = none, 'E' = even, 'O' = odd
   * @param stopBits Number of stop bits. 1 = 1 bit, 1.5 = 1.5 bits, 2 = 2 bits, 3 = 3 bits
   * @param flowControl Flow control.
   * @param inverted If true, the RX and TX signals are inverted.
   *
   * Example:
   *
   *     serialPort.setSignals(34, 2);  // rx = GPIO 34, tx = GPIO 2
   *     serialPort.setup(2, 115200, 8, 'N', 1, FlowControl::Software);
   */
  void setup(int uartIndex, uint32_t baud, int dataLength, char parity, float stopBits, FlowControl flowControl, bool inverted = false);
  
  /**
   * @brief Changes the baud rate
   *
   * Can be called after setup() to change the current baud rate.
   *
   * @param value New baud rate
   */
  void setBaud(int value);
  
  /**
   * @brief Changes the frame format
   *
   * Can be called after setup() to change the frame format.
   *
   * @param dataLength Data word length. 5 = 5 bits, 6 = 6 bits, 7 = 7 bits, 8 = 8 bits
   * @param parity Parity. 'N' = none, 'E' = even, 'O' = odd
   * @param stopBits Number of stop bits. 1 = 1 bit, 1.5 = 1.5 bits, 2 = 2 bits, 3 = 3 bits
   */
  void setFrame(int dataLength, char parity, float stopBits);
  
  /**
   * @brief Allows or prevents the host from sending data
   *
   * @param enableRX If true, the host may send data (XON is sent and/or RTS is asserted)
   */
  void flowControl(bool enableRX);
  
  /**
   * @brief Checks whether TX is enabled, based on the received XOFF and on the CTS signal
   *
   * @return True if sending is allowed
   */
  bool readyToSend();
  
  /**
   * @brief Reports whether RX is active
   *
   * @return False if XOFF has been sent or RTS is not asserted
   */
  bool readyToReceive()                { return !m_sentXOFF; }
  
  /**
   * @brief Checks whether the object is initialized
   *
   * @return True if the serial port has been initialized
   */
  bool initialized()                   { return m_initialized; }
  
  /**
   * @brief Reports the current state of the CTS signal
   *
   * @return True if CTS is asserted (low voltage, host is ready to receive data)
   */
  bool CTSStatus()                     { return m_ctsPin != GPIO_UNUSED ? gpio_get_level(m_ctsPin) == 0 : true; }

  /**
   * @brief Sets the state of the RTS signal
   *
   * RTS is handled automatically when hardware flow control is enabled.
   *
   * @param value True to assert RTS (low voltage)
   */
  void setRTSStatus(bool value);

  /**
   * @brief Reports the current state of the RTS signal
   *
   * @return True if RTS is asserted (low voltage, terminal is ready to receive data)
   */
  bool RTSStatus()                     { return m_RTSStatus; }

  /**
   * @brief Sets the state of the DTR signal
   *
   * @param value True to assert DTR (low voltage)
   */
  void setDTRStatus(bool value);

  /**
   * @brief Reports the current state of the DTR signal
   *
   * @return True if DTR is asserted (low voltage)
   */
  bool DTRStatus()                     { return m_DTRStatus; }

  /**
   * @brief Reports the current state of the DSR signal
   *
   * @return True if DSR is asserted (low voltage)
   */
  bool DSRStatus()                     { return m_dsrPin != GPIO_UNUSED ? gpio_get_level(m_dsrPin) == 0 : true; }

  /**
   * @brief Reports the current state of the DCD signal
   *
   * @return True if DCD is asserted (low voltage)
   */
  bool DCDStatus()                     { return m_dcdPin != GPIO_UNUSED ? gpio_get_level(m_dcdPin) == 0 : true; }

  /**
   * @brief Reports the current state of the RI signal
   *
   * @return True if RI is asserted (low voltage)
   */
  bool RIStatus()                      { return m_riPin != GPIO_UNUSED ? gpio_get_level(m_riPin) == 0 : false; }

  /**
   * @brief Sends a byte
   *
   * @param value Byte to send
   */
  void send(uint8_t value);
  
  /**
   * @brief Activates or deactivates the break condition
   *
   * @param value If true, the break condition is active (TX is held low)
   */
  void sendBreak(bool value);
  
  /**
   * @brief Gets the parity error status
   *
   * Calling this method resets the error.
   *
   * @return Parity error status
   */
  bool parityError()   { auto r = m_parityError; m_parityError = false; return r; }
  
  /**
   * @brief Gets the framing error status
   *
   * Calling this method resets the error.
   *
   * @return Framing error status
   */
  bool framingError()  { auto r = m_framingError; m_framingError = false; return r; }
  
  /**
   * @brief Gets the overflow error status
   *
   * Calling this method resets the error.
   *
   * @return Overflow error status
   */
  bool overflowError() { auto r = m_overflowError; m_overflowError = false; return r; }

  /**
   * @brief Gets the break detected flag
   *
   * Calling this method resets the flag to false.
   *
   * @return Break detected status
   */
  bool breakDetected() { auto r = m_breakDetected; m_breakDetected = false; return r; }


private:

  int uartGetRXFIFOCount();
  void uartFlushTXFIFO();
  void uartFlushRXFIFO();
  static void IRAM_ATTR uart_isr(void * arg);
  //static void uart_on_apb_change(void * arg, apb_change_ev_t ev_type, uint32_t old_apb, uint32_t new_apb);

  bool                      m_initialized;
  int                       m_idx;
  volatile uart_dev_t *     m_dev;
  
  bool                      m_inverted;
  
  gpio_num_t                m_rxPin;
  gpio_num_t                m_txPin;
  gpio_num_t                m_rtsPin;
  gpio_num_t                m_ctsPin;
  gpio_num_t                m_dtrPin;
  gpio_num_t                m_dsrPin;
  gpio_num_t                m_dcdPin;
  gpio_num_t                m_riPin;
  
  bool                      m_RTSStatus;      // true = asserted (low)
  bool                      m_DTRStatus;      // true = asserted (low)
  
  volatile FlowControl      m_flowControl;
  volatile bool             m_sentXOFF;       // true if XOFF has been sent or RTS is disabled (high)
  volatile bool             m_recvXOFF;       // true if XOFF has been received
  
  void *                    m_callbackArgs;
  RXReadyCallback           m_rxReadyCallback;
  RXCallback                m_rxCallback;
  LineStatusCallback        m_lineStatusCallback;
  
  volatile bool             m_parityError;
  volatile bool             m_framingError;
  volatile bool             m_overflowError;
  volatile bool             m_breakDetected;

};



/**
 * @brief SerialPortTerminalConnector is a helper class that connects a Terminal to a SerialPort
 *
 * @ingroup comm
 */
class SerialPortTerminalConnector {
public:
  SerialPortTerminalConnector();
  SerialPortTerminalConnector(SerialPort * serialPort, Terminal * terminal);
  void connect(SerialPort * serialPort, Terminal * terminal);

  /**
   * @brief Disables or enables serial port RX
   *
   * Temporarily disables reception from the serial port, discarding all incoming data.
   *
   * @param value True to disable RX, false to re-enable it.
   */
  void disableSerialPortRX(bool value)         { m_serialPortRXEnabled = !value; }

private:
  static void rxCallback(void * args, uint8_t value, bool fromISR);
  static bool rxReadyCallback(void * args, bool fromISR);
  
  Terminal *    m_terminal;
  SerialPort *  m_serialPort;

  // if false all inputs from UART are discarded
  volatile bool m_serialPortRXEnabled;
};



} // end of namespace


