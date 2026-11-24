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
 * @brief This file contains the fabgl::Mouse definition.
 */


#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "fabglconf.h"
#include "fabutils.h"
#include "comdrivers/ps2device.h"
#include "fabui.h"


namespace fabgl {




/**
 * @brief Describes a mouse movement and the state of the buttons.
 *
 * @ingroup input
 */
struct MouseDelta {
  int16_t      deltaX;             /**< Horizontal movement since the last report. Moving right generates positive values. */
  int16_t      deltaY;             /**< Vertical movement since the last report. Moving up generates positive values. */
  int8_t       deltaZ;             /**< Scroll wheel movement since the last report. Scrolling down generates positive values. */
  MouseButtons buttons;            /**< State of the mouse buttons. */
  uint8_t      overflowX : 1;      /**< Set to 1 when a horizontal overflow has been detected. */
  uint8_t      overflowY : 1;      /**< Set to 1 when a vertical overflow has been detected. */
};


/**
 * @brief Contains the raw data received from the mouse.
 *
 * @ingroup input
 */
struct MousePacket {
  uint8_t data[4];
};


/** \ingroup Enumerations
 * @brief Mouse types.
 */
enum MouseType {
  LegacyMouse,    /**< Legacy PS/2 mouse with three buttons and X/Y movement. */
  Intellimouse,   /**< Microsoft IntelliMouse compatible mouse with three buttons, X/Y movement and a scroll wheel. */
};


/**
 * @brief PS/2 mouse driver.
 *
 * Mouse connects to one port of the PS/2 controller (fabgl::PS2Controller) and decodes mouse movements.<br>
 * It supports standard PS/2 mice (X/Y axes and three buttons) and Microsoft IntelliMouse compatible mice
 * (X/Y axes, three buttons and a scroll wheel).<br>
 * Movement parameters such as sample rate, resolution and scaling can be configured.<br>
 * The PS/2 controller uses the ULP coprocessor and RTC slow memory to communicate with the PS/2 device.<br>
 * <br>
 * Since fabgl::PS2Controller supports up to two PS/2 ports, two PS/2 devices can be connected. The most common
 * configuration is a keyboard on port 0 and a mouse on port 1, but two mice can also be connected at the same time.<br>
 *
 * Example:
 *
 *     // Use GPIO26 for CLK and GPIO27 for DATA
 *     fabgl::Mouse Mouse;
 *     Mouse.begin(GPIO_NUM_26, GPIO_NUM_27);
 *
 *     if (Mouse.deltaAvailable()) {
 *       MouseDelta mouseDelta;
 *       Mouse.getNextDelta(&mouseDelta);
 *
 *       printf("deltaX = %d  deltaY = %d  deltaZ = %d  leftBtn = %d  midBtn = %d  rightBtn = %d\n",
 *                     mouseDelta.deltaX, mouseDelta.deltaY, mouseDelta.deltaZ,
 *                     mouseDelta.buttons.left, mouseDelta.buttons.middle, mouseDelta.buttons.right);
 *     }
 *
 * @see @ref guide_input
 *
 * @ingroup input
 */
class Mouse : public PS2Device {

public:

  Mouse();

  ~Mouse();

  /**
   * @brief Initializes Mouse, specifying the CLOCK and DATA GPIOs.
   *
   * A reset command (see reset()) is automatically sent to the mouse.<br>
   * This method also initializes PS2Controller to use port 0 only.
   *
   * Since the PS/2 ports are handled by the ULP coprocessor, only a few GPIOs can be used:
   * GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_12 (with some limitations), GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_32 and GPIO_NUM_33.
   *
   * @param clkGPIO GPIO of the clock line
   * @param dataGPIO GPIO of the data line
   *
   * Example:
   *
   *     // Use GPIO26 for CLK and GPIO27 for DATA
   *     Mouse.begin(GPIO_NUM_26, GPIO_NUM_27);
   */
  void begin(gpio_num_t clkGPIO, gpio_num_t dataGPIO);

  /**
   * @brief Initializes Mouse without initializing the PS/2 controller.
   *
   * A reset command (see reset()) is automatically sent to the mouse.<br>
   * This method does not initialize the PS2Controller.
   *
   * @param PS2Port The PS/2 port to use (0 or 1).
   *
   * Example:
   *
   *     // Use GPIO26 for CLK and GPIO27 for DATA on port 0
   *     PS2Controller::begin(GPIO_NUM_26, GPIO_NUM_27); // clk, dat
   *     Mouse.begin(0); // port 0
   */
  void begin(int PS2Port);

  /**
   * @brief Sends a Reset command to the mouse.
   *
   * @return True if the mouse is correctly initialized.
   */
  bool reset();

  /**
   * @brief Checks whether the mouse has been detected and correctly initialized.
   *
   * isMouseAvailable() returns a valid value only after begin() or reset() has been called.
   *
   * @return True if the mouse is correctly initialized.
   */
  bool isMouseAvailable() { return m_mouseAvailable; }

  /**
   * @brief Gets the mouse packet size
   *
   * @return Mouse packet size in bytes
   */
  int getPacketSize();

  /**
   * @brief Determines whether a raw mouse movement is available in the queue.
   *
   * @return True if a mouse movement (mouse packet) is available.
   */
  bool packetAvailable();

  /**
   * @brief Gets a raw mouse movement (packet) from the queue.
   *
   * @param packet Pointer to the MousePacket structure that receives the mouse movement.
   * @param timeOutMS Timeout in milliseconds. -1 = no timeout (wait forever).
   * @param requestResendOnTimeOut If true and the timeout expires, the mouse is asked to resend the movement.
   *
   * @return True if the packet structure has been filled, false if no data is available.
   *
   * Example:
   *
   *     MousePacket mousePacket;
   *     Mouse.getNextPacket(&mousePacket);
   */
  bool getNextPacket(MousePacket * packet, int timeOutMS = -1, bool requestResendOnTimeOut = false);

  /**
   * @brief Determines whether a mouse movement is available in the queue.
   *
   * @return True if a mouse movement (delta) is available.
   */
  bool deltaAvailable();

  /**
   * @brief Gets a mouse movement from the queue.
   *
   * @param delta Pointer to the MouseDelta structure that receives the mouse movement. It may be nullptr if not needed (in this case only status().buttons is updated).
   * @param timeOutMS Timeout in milliseconds. -1 = no timeout (wait forever).
   * @param requestResendOnTimeOut If true and the timeout expires, the mouse is asked to resend the movement.
   *
   * @return True if the movement structure has been filled, false if no data is available.
   *
   * Example:
   *
   *     MouseDelta mouseDelta;
   *     Mouse.getNextDelta(&mouseDelta);
   */
  bool getNextDelta(MouseDelta * delta, int timeOutMS = -1, bool requestResendOnTimeOut = false);

  /**
   * @brief Sets the maximum rate at which the mouse reports movements.
   *
   * The default sample rate is 60 samples per second.
   *
   * @param value Sample rate in samples per second. Valid values are 10, 20, 40, 60, 80, 100 and 200.
   *
   * @return True if the command has been successfully delivered to the mouse.
   */
  bool setSampleRate(int value);

  /**
   * @brief Sets the resolution.
   *
   * Resolution is the amount by which the movement counters are incremented or decremented, measured in counts per millimeter.<br>
   * The default resolution is 4 counts/mm.
   *
   * @param value Resolution encoded as follows: 0 = 1 count/mm (25 dpi), 1 = 2 counts/mm (50 dpi), 2 = 4 counts/mm (100 dpi), 3 = 8 counts/mm (200 dpi).
   *
   * @return True if the command has been successfully delivered to the mouse.
   */
  bool setResolution(int value);

  /**
   * @brief Sets the scaling.
   *
   * The default scaling is 1:1.
   *
   * @param value Scaling encoded as follows: 1 = 1:1, 2 = 1:2.
   *
   * @return True if the command has been successfully delivered to the mouse.
   */
  bool setScaling(int value);

  /**
   * @brief Initializes the absolute position handler.
   *
   * Defines the absolute mouse area as the rectangle from (0, 0) to (width - 1, height - 1).<br>
   * Optionally creates a queue that stores the absolute positions generated by updateAbsolutePosition().<br>
   * Call this method once to initialize absolute positioning.<br>
   *
   * @param width Width of the absolute mouse area. The mouse moves from 0 to width - 1.
   * @param height Height of the absolute mouse area. The mouse moves from 0 to height - 1.
   * @param createAbsolutePositionsQueue If true, a queue of absolute positions is created.
   * @param updateDisplayController If not nullptr, the mouse pointer of this display controller is updated automatically.
   * @param app Optional fabgl::uiApp that receives mouse events.
   *
   * Example:
   *
   *     Mouse.setupAbsolutePositioner(Canvas.getWidth(), Canvas.getHeight(), true);
   */
  void setupAbsolutePositioner(int width, int height, bool createAbsolutePositionsQueue, BitmappedDisplayController * updateDisplayController = nullptr, uiApp * app = nullptr);

  /**
   * @brief Sets the current UI application
   *
   * @param app The UI application that receives mouse events
   */
  void setUIApp(uiApp * app) { m_uiApp = app; }

  /**
   * @brief Terminates the absolute position handler.
   */
  void terminateAbsolutePositioner();

  /**
   * @brief Updates the absolute position from the specified mouse delta event.
   *
   * Updates the absolute mouse position, the wheel and the state of the buttons.<br>
   * For smooth acceleration, call updateAbsolutePosition() often and at a constant rate.<br>
   * updateAbsolutePosition() is called automatically when the updateDisplayController or createAbsolutePositionsQueue parameter of setupAbsolutePositioner() is set.<br>
   *
   * @param delta Mouse event to process.
   *
   * Example:
   *
   *     // move a (previously defined) sprite to the absolute mouse position
   *     while (true) {
   *       MouseDelta delta;
   *       if (Mouse.getNextDelta(&delta)) {
   *         Mouse.updateAbsolutePosition(&delta);
   *         mouseSprite.moveTo(Mouse.status().X, Mouse.status().Y);
   *       }
   *     }
   */
  void updateAbsolutePosition(MouseDelta * delta);

  /**
   * @brief Gets or sets the current mouse status.
   */
  MouseStatus & status() { return m_status; }

  /**
   * @brief Gets the number of mouse status items available in the queue.
   *
   * A mouse status contains the absolute mouse position, the scroll wheel delta and the state of the buttons.<br>
   * The status queue is filled only when the createAbsolutePositionsQueue parameter of setupAbsolutePositioner() is true.
   *
   * @return Number of available status items.
   */
  int availableStatus();

  /**
   * @brief Gets the next status from the status queue.
   *
   * A mouse status contains the absolute mouse position, the scroll wheel delta and the state of the buttons.<br>
   * The status queue is filled only when the createAbsolutePositionsQueue parameter of setupAbsolutePositioner() is true.
   *
   * @param timeOutMS Timeout in milliseconds to wait for a new status. -1 = wait forever.
   *
   * @return The next status in the queue.
   *
   * Example:
   *
   *     // wait for mouse events and draw a pixel at the mouse position while the left button is pressed
   *     MouseStatus status = Mouse.getNextStatus();
   *     if (status.buttons.left) {
   *       Canvas.setPenColor(Color::BrightRed);
   *       Canvas.setPixel(status.X, status.Y);
   *     }
   */
  MouseStatus getNextStatus(int timeOutMS = -1);

  /**
   * @brief Gets or sets the mouse movement acceleration factor.
   *
   * Movement acceleration is calculated by updateAbsolutePosition() and depends on the acceleration factor and on the time between calls.<br>
   * Low values produce little acceleration, high values produce strong acceleration.<br>
   * Suggested range is 0 ... 2000. Default value is 180.
   */
  int & movementAcceleration() { return m_movementAcceleration; }

  /**
   * @brief Gets or sets the wheel acceleration factor.
   *
   * Wheel acceleration is calculated by updateAbsolutePosition() and depends on the acceleration factor and on the time between calls.<br>
   * Low values produce little acceleration, high values produce strong acceleration.<br>
   * Suggested range is 0 ... 100000. Default value is 60000.
   */
  int & wheelAcceleration()    { return m_wheelAcceleration; }

  /**
   * @brief Empties the mouse status and event queues
   */
  void emptyQueue();


  /**
   * @brief Disables retries when no mouse is found
   */
  static void quickCheckHardware()   { s_quickCheckHardware = true; }


private:

  // sends a parameter command while data reporting is suspended (with retries)
  bool sendParameterCommand(bool (Mouse::*cmd)(int), int value);

  static void mouseUpdateTask(void * arg);
  bool decodeMousePacket(MousePacket * mousePacket, MouseDelta * delta);

  static bool     s_quickCheckHardware;

  bool            m_mouseAvailable;
  MouseType       m_mouseType;

  TaskHandle_t    m_mouseUpdateTask;

  // queue of one mouse packet
  QueueHandle_t   m_receivedPacket;

  // absolute position support
  bool            m_absoluteUpdate;
  Size            m_area;
  MouseStatus     m_status;
  MouseStatus     m_prevStatus;
  int64_t         m_prevDeltaTime;
  int             m_movementAcceleration;  // reasonable values: 0...2000
  int             m_wheelAcceleration;     // reasonable values: 0...100000
  QueueHandle_t   m_absoluteQueue;         // a queue of messages generated by updateAbsolutePosition()
  BitmappedDisplayController * m_updateDisplayController;

  uiApp *         m_uiApp;
};



} // end of namespace










