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
 * @brief This file contains the fabgl::Keyboard definition.
 */


#include "freertos/FreeRTOS.h"

#include "fabglconf.h"
#include "comdrivers/ps2device.h"
#include "fabui.h"
#include "kbdlayouts.h"
#include "codepages.h"


namespace fabgl {



/**
 * @brief PS/2 keyboard driver.
 *
 * Keyboard connects to one port of the PS/2 controller (fabgl::PS2Controller) and converts
 * scancodes to virtual keys and to ASCII (and ANSI) codes.<br>
 * It can optionally create a task that waits for scancodes from the PS/2 device and puts virtual keys into a queue.<br>
 * The PS/2 controller uses the ULP coprocessor and RTC slow memory to communicate with the PS/2 device.<br>
 * <br>
 * An international keyboard layout can be selected; the default is the US layout.<br>
 * The predefined layouts are US (USA), UK (United Kingdom), DE (German), IT (Italian), ES (Spanish), FR (French), BE (Belgian), NO (Norwegian) and JP (Japanese).
 * Other layouts can be defined by deriving from US or from any other layout.
 *
 * Applications usually do not create Keyboard instances: PS2Controller creates them, and they can be obtained with PS2Controller::keyboard().
 *
 * Example:
 *
 *     fabgl::Keyboard Keyboard;
 *     
 *     // Use GPIO33 for CLK and GPIO32 for DATA
 *     Keyboard.begin(GPIO_NUM_33, GPIO_NUM_32);  // clk, dat
 *
 *     // Print the names of the received virtual keys
 *     while (true)
 *       printf("VirtualKey = %s\n", Keyboard.virtualKeyToString(Keyboard.getNextVirtualKey()));
 *
 * @see @ref guide_input
 *
 * @ingroup input
 */
class Keyboard : public PS2Device {

public:

  Keyboard();
  ~Keyboard();

  /**
   * @brief Initializes Keyboard, specifying the CLOCK and DATA GPIOs.
   *
   * A reset command (see reset()) is automatically sent to the keyboard.<br>
   * This method also initializes PS2Controller to use port 0 only.
   *
   * Since the PS/2 ports are handled by the ULP coprocessor, only a few GPIOs can be used:
   * GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_12 (with some limitations), GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_32 and GPIO_NUM_33.
   *
   * @param clkGPIO GPIO of the clock line
   * @param dataGPIO GPIO of the data line
   * @param generateVirtualKeys If true, creates a task that consumes scancodes to produce virtual keys,
   *                            so that isVKDown() can be used.
   * @param createVKQueue If true, creates a task that consumes scancodes and produces virtual keys
   *                      and puts them in a queue, so that isVKDown(), virtualKeyAvailable()
   *                      and getNextVirtualKey() can be used.
   *
   * Example:
   *
   *     // Use GPIO33 for CLK and GPIO32 for DATA
   *     Keyboard.begin(GPIO_NUM_33, GPIO_NUM_32);  // clk, dat
   */
  void begin(gpio_num_t clkGPIO, gpio_num_t dataGPIO, bool generateVirtualKeys = true, bool createVKQueue = true);

  /**
   * @brief Initializes Keyboard without initializing the PS/2 controller.
   *
   * A reset command (see reset()) is automatically sent to the keyboard.<br>
   * This method does not initialize the PS2Controller.
   *
   * @param generateVirtualKeys If true, creates a task that consumes scancodes and produces virtual keys,
   *                            so that isVKDown() can be used.
   * @param createVKQueue If true, creates a task that consumes scancodes to produce virtual keys
   *                      and puts them in a queue, so that isVKDown(), virtualKeyAvailable()
   *                      and getNextVirtualKey() can be used.
   * @param PS2Port The PS/2 port to use (0 or 1).
   *
   * Example:
   *
   *     // Use GPIO33 for CLK and GPIO32 for DATA on port 0
   *     PS2Controller::begin(GPIO_NUM_33, GPIO_NUM_32); // clk, dat
   *     Keyboard.begin(true, true, 0); // port 0
   */
  void begin(bool generateVirtualKeys, bool createVKQueue, int PS2Port);

  /**
   * @brief Enables or disables virtual key generation at run time
   *
   * This method can be called after begin().
   *
   * @param generateVirtualKeys If true, creates a task that consumes scancodes and produces virtual keys,
   *                            so that isVKDown() can be used. If false, the task is destroyed.
   * @param createVKQueue If true, creates a task that consumes scancodes to produce virtual keys
   *                      and puts them in a queue, so that isVKDown(), virtualKeyAvailable()
   *                      and getNextVirtualKey() can be used. If false, the queue is destroyed.
   */
  void enableVirtualKeys(bool generateVirtualKeys, bool createVKQueue);

  /**
   * @brief Sets the current UI application
   *
   * Requires generateVirtualKeys to be true in begin().
   *
   * @param app The UI application that receives keyboard events
   */
  void setUIApp(uiApp * app) { m_uiApp = app; }

  /**
   * @brief Sends a Reset command to the keyboard.
   *
   * @return True if the keyboard is correctly initialized.
   */
  bool reset();

  /**
   * @brief Checks whether the keyboard has been detected and correctly initialized.
   *
   * isKeyboardAvailable() returns a valid value only after begin() or reset() has been called.
   *
   * @return True if the keyboard is correctly initialized.
   */
  bool isKeyboardAvailable() { return m_keyboardAvailable; }

  /**
   * @brief Sets the keyboard layout.
   *
   * An international keyboard layout can be selected; the default is the US layout.<br>
   * The predefined layouts are US (USA), UK (United Kingdom), DE (German), IT (Italian), ES (Spanish), FR (French), BE (Belgian), NO (Norwegian) and JP (Japanese).
   * Other layouts can be defined by deriving from US or from any other layout.
   *
   * @param layout A pointer to the layout structure.
   *
   * Example:
   *
   *     // Set German layout
   *     Keyboard.setLayout(&fabgl::GermanLayout);
   */
  void setLayout(KeyboardLayout const * layout);

  /**
   * @brief Gets the current keyboard layout.
   *
   * @return The default layout or the last layout set.
   */
  KeyboardLayout const * getLayout() { return m_layout; }

  /**
   * @brief Gets the state of a virtual key.
   *
   * Returns whether the specified virtual key is pressed or released.<br>
   * Virtual keys are generated from scancodes only if the generateVirtualKeys parameter of begin() is true (default).
   *
   * @param virtualKey The Virtual Key to test.
   *
   * @return True if the specified virtual key is pressed.
   */
  bool isVKDown(VirtualKey virtualKey);

  /**
   * @brief Gets the number of virtual keys available in the queue.
   *
   * Virtual keys are generated from scancodes only if the generateVirtualKeys and createVKQueue parameters
   * of begin() are true (default).
   *
   * @return The number of virtual keys available to read.
   */
  int virtualKeyAvailable();

  /**
   * @brief Gets a virtual key from the queue.
   *
   * Virtual keys are generated from scancodes only if the generateVirtualKeys and createVKQueue parameters
   * of begin() are true (default).
   *
   * @param keyDown Pointer to a boolean that receives true if the key has been pressed, false if it has been released.
   * @param timeOutMS Timeout in milliseconds. -1 = no timeout (wait forever).
   *
   * @return The first virtual key in the queue (VK_NONE if no key is available within the timeout).
   */
  VirtualKey getNextVirtualKey(bool * keyDown = nullptr, int timeOutMS = -1);

  /**
   * @brief Gets a virtual key from the queue, including the associated scancode.
   *
   * Virtual keys are generated from scancodes only if the generateVirtualKeys and createVKQueue parameters
   * of begin() are true (default).
   *
   * @param item Pointer to a VirtualKeyItem structure that receives the virtual key information.
   * @param timeOutMS Timeout in milliseconds. -1 = no timeout (wait forever).
   *
   * @return True if an item has been received, false if no item is available within the timeout.
   */
  bool getNextVirtualKey(VirtualKeyItem * item, int timeOutMS = -1);

  /**
   * @brief Appends or inserts a virtual key into the virtual key queue
   *
   * @param virtualKey Virtual key to append or insert
   * @param keyDown True if the virtual key is pressed
   * @param insert If true, the virtual key is inserted at the head of the queue
   */
  void injectVirtualKey(VirtualKey virtualKey, bool keyDown, bool insert = false);

  /**
   * @brief Appends or inserts virtual key information into the virtual key queue
   *
   * @param item Virtual key information to append or insert
   * @param insert If true, the item is inserted at the head of the queue
   */
  void injectVirtualKey(VirtualKeyItem const & item, bool insert = false);

  /**
   * @brief Empties the virtual key queue
   */
  void emptyVirtualKeyQueue();

  /**
   * @brief Converts a virtual key to ASCII.
   *
   * Converts the specified virtual key to ASCII, if possible.<br>
   * <br>For example, VK_A is converted to 'A' (ASCII 0x41), CTRL + VK_SPACE produces ASCII NUL (0x00), CTRL + letter produces
   * ASCII control codes from SOH (0x01) to SUB (0x1A), CTRL + VK_BACKSLASH produces ASCII FS (0x1C), CTRL + VK_QUESTION produces
   * ASCII US (0x1F), CTRL + VK_LEFTBRACKET produces ASCII ESC (0x1B), CTRL + VK_RIGHTBRACKET produces ASCII GS (0x1D),
   * CTRL + VK_TILDE produces ASCII RS (0x1E) and VK_SCROLLLOCK produces XON or XOFF.<br>
   * <br>The conversion uses the codepage set with setCodePage().<br>
   *
   * @param virtualKey The virtual key to convert.
   *
   * @return The ASCII code of the virtual key, or -1 if it cannot be translated to ASCII.
   */
  int virtualKeyToASCII(VirtualKey virtualKey);

  /**
   * @brief Sets the codepage used for virtual key to ASCII conversion
   *
   * Terminal and the UI framework call this method automatically.
   *
   * @param codepage Codepage to use
   *
   * Example:
   *
   *     // Set codepage 437
   *     Keyboard.setCodePage(CodePages::get(437));
   */
  void setCodePage(CodePage const * codepage)                      { m_codepage = codepage; }

  /**
   * @brief Gets the number of scancodes available in the queue.
   *
   * Scancodes are always generated, but they may be consumed by the scancode-to-virtual-key task. To use this
   * method, call begin() with generateVirtualKeys = false and createVKQueue = false.<br>
   *
   * @return The number of scancodes available to read.
   */
  int scancodeAvailable();

  /**
   * @brief Gets a scancode from the queue.
   *
   * Scancodes are always generated, but they may be consumed by the scancode-to-virtual-key task. To use this
   * method, call begin() with generateVirtualKeys = false and createVKQueue = false.<br>
   *
   * @param timeOutMS Timeout in milliseconds. -1 = no timeout (wait forever).
   * @param requestResendOnTimeOut If true and the timeout expires, the keyboard is asked to resend the scancode.
   *
   * @return The first scancode in the queue (-1 if no data is available within the timeout).
   */
  int getNextScancode(int timeOutMS = -1, bool requestResendOnTimeOut = false);

  /**
   * @brief Sets the state of the keyboard LEDs.
   *
   * Switches the NUMLOCK, CAPSLOCK and SCROLLLOCK LEDs on or off.
   *
   * @param numLock If true, the NUMLOCK LED is switched on.
   * @param capsLock If true, the CAPSLOCK LED is switched on.
   * @param scrollLock If true, the SCROLLLOCK LED is switched on.
   *
   * @return True if the command has been successfully delivered to the keyboard.
   */
  bool setLEDs(bool numLock, bool capsLock, bool scrollLock);

  /**
   * @brief Gets the state of the keyboard LEDs.
   *
   * Returns the current state of the NUMLOCK, CAPSLOCK and SCROLLLOCK LEDs.
   *
   * @param numLock Receives true if the NUMLOCK LED is on.
   * @param capsLock Receives true if the CAPSLOCK LED is on.
   * @param scrollLock Receives true if the SCROLLLOCK LED is on.
   */
  void getLEDs(bool * numLock, bool * capsLock, bool * scrollLock);

  /**
   * @brief Sets the typematic rate and delay.
   *
   * When a key is held down, after repeatDelayMS the keyboard starts sending codes every repeatRateMS.
   *
   * @param repeatRateMS Repeat period in milliseconds (33 ms ... 500 ms).
   * @param repeatDelayMS Repeat delay in milliseconds (250 ms ... 1000 ms, in steps of 250 ms).
   *
   * @return True if the command has been successfully delivered to the keyboard.
   */
  bool setTypematicRateAndDelay(int repeatRateMS, int repeatDelayMS) { return send_cmdTypematicRateAndDelay(repeatRateMS, repeatDelayMS); }

  /**
   * @brief Sets the scancode set
   *
   * Scancode set 1 is used by the original IBM PC XT (https://wiki.osdev.org/PS/2_Keyboard#Scan_Code_Set_1).<br>
   * Scancode set 2 is used by the IBM PC AT (https://wiki.osdev.org/PS/2_Keyboard#Scan_Code_Set_2).<br>
   * Scancode set 3 is used by the IBM 3270 (https://web.archive.org/web/20170108131104/http://www.computer-engineering.org/ps2keyboard/scancodes3.html).
   *
   * When virtual keys are enabled, only sets 1 and 2 are available.
   *
   * @param value Scancode set (1, 2 or 3).
   *
   * @return True if the scancode set has been changed.
   */
  bool setScancodeSet(int value);

  /**
   * @brief Gets the current scancode set
   *
   * @return Current scancode set (1, 2 or 3)
   */
  int scancodeSet()                { return m_scancodeSet; }

  static uint8_t convScancodeSet2To1(uint8_t code);

#if FABGLIB_HAS_VirtualKeyO_STRING
  static char const * virtualKeyToString(VirtualKey virtualKey);
#endif


  //// Delegates ////

  /**
   * @brief Delegate called whenever a new virtual key is decoded from scancodes
   *
   * The first parameter is a pointer to the decoded virtual key.
   * The second parameter is true when the key is pressed and false when it is released.
   */
  Delegate<VirtualKey *, bool> onVirtualKey;


  //// statics (used for common default properties) ////


  /**
   * @brief Stack size of the task that converts scancodes to virtual keys.
   *
   * Applications must change this value before calling begin().
   *
   * Default value is FABGLIB_DEFAULT_SCODETOVK_TASK_STACK_SIZE (defined in fabglconf.h)
   */
  static int scancodeToVirtualKeyTaskStackSize;



private:

  VirtualKey scancodeToVK(uint8_t scancode, bool isExtended, KeyboardLayout const * layout = nullptr);
  VirtualKey VKtoAlternateVK(VirtualKey in_vk, bool down, KeyboardLayout const * layout = nullptr);
  VirtualKey manageCAPSLOCK(VirtualKey vk);
  void updateLEDs();
  bool blockingGetVirtualKey(VirtualKeyItem * item);
  void convertScancode2to1(VirtualKeyItem * item);
  void postVirtualKeyItem(VirtualKeyItem const & item);
  static int convKeypadVKToNum(VirtualKey vk);

  static void SCodeToVKConverterTask(void * pvParameters);


  bool                      m_keyboardAvailable;  // self test passed and support for scancode set 2

  TaskHandle_t              m_SCodeToVKConverterTask; // Task that converts scancodes to virtual key and populates m_virtualKeyQueue
  QueueHandle_t             m_virtualKeyQueue;

  uint8_t                   m_VKMap[(int)(VK_LAST + 7) / 8];

  // allowed values: 1, 2 or 3
  // If virtual keys are enabled only 1 and 2 are possible. In case of scancode set 1 it is converted from scan code set 2, which is necessary
  // in order to decode virtual keys.
  uint8_t                   m_scancodeSet;

  KeyboardLayout const *    m_layout;

  uiApp *                   m_uiApp;

  bool                      m_CTRL;
  bool                      m_LALT;
  bool                      m_RALT;
  bool                      m_SHIFT;
  bool                      m_CAPSLOCK;
  bool                      m_GUI;
  bool                      m_NUMLOCK;
  bool                      m_SCROLLLOCK;

  VirtualKey                m_lastDeadKey;

  // store status of the three LEDs
  bool                      m_numLockLED;
  bool                      m_capsLockLED;
  bool                      m_scrollLockLED;

  CodePage const *          m_codepage;

};





} // end of namespace










