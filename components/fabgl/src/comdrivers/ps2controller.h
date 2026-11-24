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
 * @brief This file contains the fabgl::PS2Controller definition.
 */


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "fabutils.h"
#include "fabglconf.h"


namespace fabgl {


/** \ingroup Enumerations
 * @brief Devices connected to the PS/2 ports
 */
enum class PS2Preset {
  KeyboardPort0_MousePort1,   /**< Keyboard on port 0 and mouse on port 1 */
  KeyboardPort1_MousePort0,   /**< Keyboard on port 1 and mouse on port 0 */
  KeyboardPort0,              /**< Keyboard on port 0 (no mouse) */
  KeyboardPort1,              /**< Keyboard on port 1 (no mouse) */
  MousePort0,                 /**< Mouse on port 0 (no keyboard) */
  MousePort1,                 /**< Mouse on port 1 (no keyboard) */
};


/** \ingroup Enumerations
 * @brief Keyboard virtual key handling modes
 */
enum class KbdMode {
  NoVirtualKeys,           /**< No virtual keys are generated */
  GenerateVirtualKeys,     /**< Virtual keys are generated. Only Keyboard::isVKDown() can be used. */
  CreateVirtualKeysQueue,  /**< Virtual keys are generated and put into a queue. Keyboard::isVKDown(), Keyboard::virtualKeyAvailable() and Keyboard::getNextVirtualKey() can be used. */
};


class Keyboard;
class Mouse;


/**
 * @brief PS/2 device controller.
 *
 * The PS/2 controller uses the ULP coprocessor and RTC slow memory to communicate with up to two PS/2 devices.<br>
 * The ULP coprocessor continuously monitors the CLK and DATA lines for incoming data, and can also send commands to the PS/2 devices.
 *
 * @see @ref guide_input
 *
 * @ingroup input
 */
class PS2Controller {

public:

  PS2Controller();
  ~PS2Controller();

  // unwanted methods
  PS2Controller(PS2Controller const&)   = delete;
  void operator=(PS2Controller const&)  = delete;

  /**
  * @brief Initializes the PS/2 device controller.
  *
  * Initializes the PS/2 controller, assigning GPIOs to the DATA and CLK lines.
  *
  * Since the PS/2 ports are handled by the ULP coprocessor, only a few GPIOs can be used:
  * GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_12 (with some limitations), GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_32 and GPIO_NUM_33.
  *
  * @param port0_clkGPIO GPIO of the clock line of PS/2 port 0.
  * @param port0_datGPIO GPIO of the data line of PS/2 port 0.
  * @param port1_clkGPIO GPIO of the clock line of PS/2 port 1 (GPIO_UNUSED to disable).
  * @param port1_datGPIO GPIO of the data line of PS/2 port 1 (GPIO_UNUSED to disable).
  */
  static void begin(gpio_num_t port0_clkGPIO, gpio_num_t port0_datGPIO, gpio_num_t port1_clkGPIO = GPIO_UNUSED, gpio_num_t port1_datGPIO = GPIO_UNUSED);

  /**
  * @brief Initializes the PS/2 device controller using the default GPIOs.
  *
  * Initializes the PS/2 controller, assigning:
  *   - GPIO_NUM_33 (CLK) and GPIO_NUM_32 (DATA) to PS/2 port 0
  *   - GPIO_NUM_26 (CLK) and GPIO_NUM_27 (DATA) to PS/2 port 1
  *
  * @param preset Devices connected to the PS/2 ports (mouse, keyboard or both).
  * @param keyboardMode How keyboard virtual keys are handled.
  *
  * Example:
  *
  *     // Keyboard connected to port 0 and mouse to port 1
  *     PS2Controller::begin(PS2Preset::KeyboardPort0_MousePort1);
  */
  static void begin(PS2Preset preset = PS2Preset::KeyboardPort0_MousePort1, KbdMode keyboardMode = KbdMode::CreateVirtualKeysQueue);

  static void end();

  static bool initialized() { return s_initDone; }

  /**
   * @brief Determines whether a byte has been received from the specified port
   *
   * @param PS2Port PS/2 port number (0 or 1).
   *
   * @return True if a byte is available
   */
  static bool dataAvailable(int PS2Port);

  /**
   * @brief Gets a scancode from the queue.
   *
   * @param PS2Port PS/2 port number (0 or 1).
   * @param timeOutMS Timeout in milliseconds. -1 = wait forever.
   *
   * @return The first scancode in the queue (-1 if no data is available).
   */
  static int getData(int PS2Port, int timeOutMS);

  /**
   * @brief Sends a command to the device.
   *
   * @param data Byte to send to the PS/2 device.
   * @param PS2Port PS/2 port number (0 or 1).
   */
  static void sendData(uint8_t data, int PS2Port);

  /**
   * @brief Disables input from a PS/2 port by driving the CLK line low
   *
   * Use enableRX() to release the CLK line.
   *
   * @param PS2Port PS/2 port number (0 or 1).
   */
  static void disableRX(int PS2Port);

  /**
   * @brief Enables input from a PS/2 port by releasing the CLK line
   *
   * Use disableRX() to disable input from the PS/2 port.
   *
   * @param PS2Port PS/2 port number (0 or 1).
   */
  static void enableRX(int PS2Port);

  /**
   * @brief Returns the Keyboard object automatically created by PS2Controller.
   *
   * @return A pointer to a Keyboard object
   */
  static Keyboard * keyboard()                { return s_keyboard; }

  static void setKeyboard(Keyboard * value)   { s_keyboard = value; }

  /**
   * @brief Returns the Mouse object automatically created by PS2Controller.
   *
   * @return A pointer to a Mouse object
   */
  static Mouse * mouse()                      { return s_mouse; }

  static void setMouse(Mouse * value)         { s_mouse = value; }

  /**
   * @brief Returns the singleton instance of the PS2Controller class
   *
   * @return A pointer to the PS2Controller singleton
   */
  static PS2Controller * instance()           { return s_instance; }

  static bool parityError(int PS2Port)        { return s_parityError[PS2Port]; }

  static bool syncError(int PS2Port)          { return s_syncError[PS2Port]; }

  static bool CLKTimeOutError(int PS2Port)    { return s_CLKTimeOutError[PS2Port]; }

  /**
   * @brief Gets exclusive access to the specified PS/2 port
   *
   * @param PS2Port PS/2 port number (0 or 1).
   * @param timeOutMS Maximum time to wait, in milliseconds.
   *
   * @return True if the device has been locked.
   */
  static bool lock(int PS2Port, int timeOutMS);

  /**
   * @brief Releases the exclusive access to a port.
   *
   * @param PS2Port PS/2 port number (0 or 1).
   */
  static void unlock(int PS2Port);


private:


  static void IRAM_ATTR ULPWakeISR(void * arg);

  static PS2Controller *    s_instance;

  // Keyboard and Mouse instances can be created by PS2Controller in one of the begin() calls, or can be
  // set using setKeyboard() and setMouse() calls.
  static Keyboard *         s_keyboard;
  static Mouse *            s_mouse;

  static bool               s_keyboardAllocated;
  static bool               s_mouseAllocated;

  static bool               s_portEnabled[2];


  // true if last call to getData() had a parity, sync error (start or stop missing bits) or CLK timeout
  static bool               s_parityError[2];
  static bool               s_syncError[2];
  static bool               s_CLKTimeOutError[2];

  // one word queue (contains just the last received word)
  static QueueHandle_t      s_dataIn[2];

  static SemaphoreHandle_t  s_portLock[2];

  static intr_handle_t      s_ULPWakeISRHandle;
  static bool               s_initDone;

};


struct PS2PortAutoDisableRX {
  PS2PortAutoDisableRX(int PS2Port) : port(PS2Port) {
    PS2Controller::disableRX(port);
  }
  ~PS2PortAutoDisableRX() {
    PS2Controller::enableRX(port);
  }
private:
  int port;
};




} // end of namespace






