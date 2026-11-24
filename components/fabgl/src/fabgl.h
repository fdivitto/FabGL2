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
 * @brief Single include file of the FabGL library.
 *
 * Applications only need to include this file to access all FabGL classes and functions.
 */



/**
 * @mainpage FabGL Library
 *
 * @htmlonly
 * <a href="https://www.github.com/fdivitto/fabgl"> <img src="github.png" style="width:230px;height:80px;border:0;"> </a>
 * @endhtmlonly
 *
 * [www.FabGL.com](http://www.fabgl.com) - 2019-2026 by Fabrizio Di Vittorio (fdivitto2013@gmail.com)
 *
 * [Demos and Tutorials](https://www.youtube.com/user/fdivitto/videos)
 *
 * - - -
 *
 * FabGL is a graphics library for the ESP32, distributed as an ESP-IDF component (FabGL2 requires ESP-IDF 6.1 or newer).<br>
 * It provides display drivers for VGA output and for PAL/NTSC color composite video.<br>
 * It also handles PS/2 keyboards and mice: communication on the PS/2 ports is performed by the ULP coprocessor, leaving both main CPU cores free for the application.<br>
 * In addition, FabGL includes an audio engine, a Graphical User Interface (GUI), a game engine, an ANSI/VT terminal and emulators of several CPUs and peripherals.<br>
 *
 * The library supports ESP32 revision 1 and later.<br>
 *
 * VGA output requires a simple digital-to-analog converter (DAC): three 270 Ohm resistors provide 8 colors, while six resistors provide 64 colors.<br>
 *
 * Several fixed and variable width fonts are embedded.
 *
 * The number of sprites is unlimited. However, large sprites or a large number of them reduce the frame rate and may cause flickering.<br>
 *
 * When enough memory is available (for example at low resolutions such as 320x200), two screen buffers can be allocated to implement double buffering.<br>
 * In this case drawing primitives always operate on the back buffer.<br>
 *
 * Unless double buffering is used or this behavior is explicitly disabled, all drawing operations are performed during the vertical retrace, so no flickering is visible.<br>
 * If the queue of drawing primitives is not completely processed before the vertical retrace ends, processing is suspended and resumed at the next vertical retrace.<br>
 *
 * The graphical user interface (GUI) supports overlapping windows, mouse handling and many widgets (buttons, edit boxes, checkboxes, combo boxes, list boxes, etc.).<br>
 *
 * Finally, the sound engine mixes multiple channels into a mono output. Each channel can generate sine, square and other waveforms, or play custom sampled data.<br>
 * Like VGA output, audio output is generated using DMA: the CPU only mixes the channels and prepares the waveforms.<br>
 *
 * - - -
 *
 * **New to FabGL?** Start from the @ref guide "User Guide": it explains how the library is organized and shows how to use
 * each subsystem, from the first program to GUI applications. The "Topics" section groups the classes by subsystem.
 *
 * - - -
 *
 * Main classes of the FabGL library:
 *    * fabgl::VGAController, device driver for bitmapped VGA output (64 colors)
 *    * fabgl::VGA2Controller, device driver for 2-color bitmapped VGA output (low RAM requirements, CPU intensive)
 *    * fabgl::VGA4Controller, device driver for 4-color bitmapped VGA output (low RAM requirements, CPU intensive)
 *    * fabgl::VGA8Controller, device driver for 8-color bitmapped VGA output (low RAM requirements, CPU intensive)
 *    * fabgl::VGA16Controller, device driver for 16-color bitmapped VGA output (low RAM requirements, CPU intensive)
 *    * fabgl::VGATextController, device driver for text-only VGA output (low RAM requirements, CPU intensive)
 *    * fabgl::CVBS16Controller, device driver for 16-color PAL/NTSC composite video output
 *    * fabgl::Canvas, a set of drawing primitives (lines, rectangles, text, bitmaps...)
 *    * fabgl::Terminal, an ANSI/VT100/VT102 (and later) compatible terminal emulator (see the @ref vttest "vttest score")
 *    * fabgl::Keyboard, which controls a PS/2 keyboard and translates scancodes into virtual keys or ASCII/ANSI codes
 *    * fabgl::Mouse, which controls a PS/2 mouse
 *    * fabgl::Scene, an abstract class that handles sprites, timing and collision detection
 *    * fabgl::uiApp, the base class for Graphical User Interface applications
 *    * fabgl::SoundGenerator, which generates sound and music
 *    * fabgl::InputBox, which builds simple UI wizards
 *
 * Device emulation classes:
 *    * fabgl::Z80, Zilog Z80 CPU emulator
 *    * fabgl::i8080, Intel 8080 CPU emulator
 *    * fabgl::MOS6502, MOS 6502 CPU emulator
 *    * fabgl::VIA6522, MOS 6522 VIA emulator
 *
 * Other classes:
 *    * fabgl::I2C, thread-safe I2C bus driver
 *    * fabgl::DS3231, DS3231 Real Time Clock driver (based on fabgl::I2C)
 *    * fabgl::MCP23S17, MCP23S17 16-bit SPI I/O expander driver
 *    * fabgl::SerialPort, interrupt-driven UART driver with flow control
 *    * fabgl::UARTStream, fabgl::Stream implementation based on the ESP-IDF UART driver
 *    * fabgl::Preferences, key/value storage on non-volatile flash (NVS)
 *    * fabgl::WiFiScanner, fabgl::WiFiConnection, fabgl::TCPClient, fabgl::HTTPRequest and fabgl::ICMP, network helpers
 *
 * See @ref confVGA "VGA output schema" for a sample VGA connection circuit.
 *
 * See @ref confPS2 "PS/2 ports schema" for sample PS/2 connection circuits.
 *
 * See @ref confAudio "Configuring the audio port" for a sample audio connection circuit.
 *
 * - - -
 * <CENTER> @link VGA/PCEmulator/main/PCEmulator.cpp IBM PC Emulator Example @endlink </CENTER>
 * @htmlonly <div align="center"> <iframe width="560" height="349" src="http://www.youtube.com/embed/3I1U2nEoxIQ?rel=0&loop=1&autoplay=0&modestbranding=1" frameborder="0" allowfullscreen align="middle"> </iframe> </div> @endhtmlonly
 * - - -
 * <CENTER> @link VGA/SpaceInvaders/main/SpaceInvaders.cpp Space Invaders Example @endlink </CENTER>
 * @htmlonly <div align="center"> <iframe width="560" height="349" src="http://www.youtube.com/embed/LL8J7tjxeXA?rel=0&loop=1&autoplay=0&modestbranding=1" frameborder="0" allowfullscreen align="middle"> </iframe> </div> @endhtmlonly
 * - - -
 * <CENTER> @link VGA/GraphicalUserInterface/main/GraphicalUserInterface.cpp Graphical User Interface (GUI) Example @endlink </CENTER>
 * @htmlonly <div align="center"> <iframe width="560" height="349" src="http://www.youtube.com/embed/84ytGdiOih0?rel=0&loop=1&autoplay=0&modestbranding=1" frameborder="0" allowfullscreen align="middle"> </iframe> </div> @endhtmlonly
 * - - -
 * <CENTER> @link VGA/Audio/main/Audio.cpp Audio Output Demo @endlink </CENTER>
 * @htmlonly <div align="center"> <iframe width="560" height="349" src="http://www.youtube.com/embed/RQtKFgU7OYI?rel=0&loop=1&autoplay=0&modestbranding=1" frameborder="0" allowfullscreen align="middle"> </iframe> </div> @endhtmlonly
 * - - -
 * <CENTER> @link VGA/AnsiTerminal/main/AnsiTerminal.cpp Serial Terminal Connected to an MBC2 Z80 Board @endlink </CENTER>
 * @htmlonly <div align="center"> <iframe width="560" height="349" src="http://www.youtube.com/embed/Ww_pH_ZOLqU?rel=0&loop=1&autoplay=0&modestbranding=1" frameborder="0" allowfullscreen align="middle"> </iframe> </div> @endhtmlonly
 * - - -
 * <CENTER> @link VGA/SimpleTerminalOut/main/SimpleTerminalOut.cpp Simple Terminal Output Example @endlink </CENTER>
 * @htmlonly <div align="center"> <iframe width="560" height="349" src="http://www.youtube.com/embed/AmXN0SIRqqU?rel=0&loop=1&autoplay=0&modestbranding=1" frameborder="0" allowfullscreen align="middle"> </iframe> </div> @endhtmlonly
 * - - -
 * <CENTER> @link VGA/Altair8800/main/Altair8800.cpp Altair 8800 Emulator @endlink </CENTER>
 * @htmlonly <div align="center"> <iframe width="560" height="349" src="http://www.youtube.com/embed/y0opVifEyS8?rel=0&loop=1&autoplay=0&modestbranding=1" frameborder="0" allowfullscreen align="middle"> </iframe> </div> @endhtmlonly
 * - - -
 * <CENTER> @link VGA/VIC20/main/VIC20.cpp Commodore VIC-20 Emulator @endlink </CENTER>
 * @htmlonly <div align="center"> <iframe width="560" height="349" src="http://www.youtube.com/embed/ZW427HVWYys?rel=0&loop=1&autoplay=0&modestbranding=1" frameborder="0" allowfullscreen align="middle"> </iframe> </div> @endhtmlonly
 * - - -
 * <CENTER> @link VGA/NetworkTerminal/main/NetworkTerminal.cpp Network Terminal Example @endlink </CENTER>
 * @htmlonly <div align="center"> <iframe width="560" height="349" src="http://www.youtube.com/embed/n5c27-y5tm4?rel=0&loop=1&autoplay=0&modestbranding=1" frameborder="0" allowfullscreen align="middle"> </iframe> </div> @endhtmlonly
 * - - -
 * <CENTER> @link VGA/ModelineStudio/main/ModelineStudio.cpp Modeline Studio Example @endlink </CENTER>
 * @htmlonly <div align="center"> <iframe width="560" height="349" src="http://www.youtube.com/embed/Urp0rPukjzE?rel=0&loop=1&autoplay=0&modestbranding=1" frameborder="0" allowfullscreen align="middle"> </iframe> </div> @endhtmlonly
 * - - -
 * <CENTER> @link VGA/LoopbackTerminal/main/LoopbackTerminal.cpp Loopback Terminal Example @endlink </CENTER>
 * @htmlonly <div align="center"> <iframe width="560" height="349" src="http://www.youtube.com/embed/hQhU5hgWdcU?rel=0&loop=1&autoplay=0&modestbranding=1" frameborder="0" allowfullscreen align="middle"> </iframe> </div> @endhtmlonly
 * - - -
 * <CENTER> @link VGA/DoubleBuffer/main/DoubleBuffer.cpp Double Buffering Example @endlink </CENTER>
 * @htmlonly <div align="center"> <iframe width="560" height="349" src="http://www.youtube.com/embed/TRQcIiWQCJw?rel=0&loop=1&autoplay=0&modestbranding=1" frameborder="0" allowfullscreen align="middle"> </iframe> </div> @endhtmlonly
 * - - -
 * <CENTER> @link VGA/CollisionDetection/main/CollisionDetection.cpp Collision Detection Example @endlink </CENTER>
 * @htmlonly <div align="center"> <iframe width="560" height="349" src="http://www.youtube.com/embed/q3OPSq4HhDE?rel=0&loop=1&autoplay=0&modestbranding=1" frameborder="0" allowfullscreen align="middle"> </iframe> </div> @endhtmlonly
 * - - -
 * <CENTER> @link VGA/MultitaskingCPM/main/MultitaskingCPM.cpp Multitasking CP/M Plus Example @endlink </CENTER>
 * @htmlonly <div align="center"> <iframe width="560" height="349" src="http://www.youtube.com/embed/3UevsxMQZ5w?rel=0&loop=1&autoplay=0&modestbranding=1" frameborder="0" allowfullscreen align="middle"> </iframe> </div> @endhtmlonly
 * - - -
 *
 * Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com) - <http://www.fabgl.com>
 * Copyright (c) 2019-2026 Fabrizio Di Vittorio.
 * All rights reserved.
 *
 * * Please contact fdivitto2013@gmail.com if you need a commercial license.
 *
 * * This library and related software is available under GPL v3.
 *
 * FabGL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FabGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FabGL.  If not, see <http://www.gnu.org/licenses/>.
 *
 */



/**
 * @page confVGA VGA output schema
 *
 * VGA output can be configured to display 8 or 64 colors.
 * Eight colors require 5 outputs (R, G, B, HSync and VSync), while sixty-four colors require 8 outputs (R0, R1, G0, G1, B0, B1, HSync and VSync).
 *
 * The following is an example of GPIO assignment and of a simple digital-to-analog converter circuit for 64 colors (2 bits per channel, 6 bits per pixel):
 *
 * \image html schema_VGA64.png width=500cm
 *
 * Using the GPIOs above, the VGA controller can be initialized as follows:
 *
 *     VGAController.begin(GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_23, GPIO_NUM_15);
 *
 * Note: do not use GPIO_NUM_2 (connected to the on-board LED on many boards) for VGA signals.
 *
 */



/**
 * @page confPS2 PS/2 ports schema
 *
 * Each PS/2 keyboard or mouse uses two GPIOs (clock and data). Each signal requires a 120 Ohm series resistor and a 2K Ohm pull-up resistor:
 *
 * \image html schema_PS2.png width=500cm
 *
 * Using the GPIOs above, a PS/2 mouse can be initialized as follows:
 *
 *     Mouse.begin(GPIO_NUM_26, GPIO_NUM_27);  // CLK, DAT
 *
 * When both a mouse and a keyboard are connected, initialization must be performed through the PS2Controller:
 *
 *     fabgl::PS2Controller PS2Controller;
 *     fabgl::Keyboard Keyboard;
 *     fabgl::Mouse Mouse;
 *
 *     // port 0 (keyboard) CLK and DAT, port 1 (mouse) CLK and DAT
 *     PS2Controller.begin(GPIO_NUM_33, GPIO_NUM_32, GPIO_NUM_26, GPIO_NUM_27);
 *     // initialize the keyboard on port 0 (GPIO33=CLK, GPIO32=DAT)
 *     Keyboard.begin(true, true, 0);
 *     // initialize the mouse on port 1 (GPIO26=CLK, GPIO27=DAT)
 *     Mouse.begin(1);
 *
 * When using the GPIOs above, mouse and keyboard can be configured more simply with a preset:
 *
 *     fabgl::PS2Controller PS2Controller;
 *     PS2Controller.begin(PS2Preset::KeyboardPort0_MousePort1);
 */



/**
 * @page confAudio Configuring the audio port
 *
 * Audio output uses GPIO 25 and requires a simple low-pass filter and a peak limiter. The following circuit is a working example, which can certainly be improved:
 *
 * \image html schema_audio.png width=500cm
 *
 */




/**
 * @example VGA/Altair8800/main/Altair8800.cpp Altair 8800 emulator, with ADM-31, ADM-3A, Kaypro, Hazeltine 1500 and Osborne I terminal emulation
 * @example VGA/AnsiTerminal/main/AnsiTerminal.cpp Serial VT/ANSI terminal
 * @example VGA/Audio/main/Audio.cpp Audio demo with GUI
 * @example VGA/ClassicRacer/main/ClassicRacer.cpp Racing game, by Carles Oriol
 * @example VGA/CollisionDetection/main/CollisionDetection.cpp Sprites and collision detection using fabgl::Scene
 * @example VGA/DirectVGA/main/DirectVGA.cpp Direct VGA control using the VGADirectController base class
 * @example VGA/DirectVGA_ElMuro/main/DirectVGA_ElMuro.cpp Breakout-style game based on the VGADirectController class, by Carles Oriol
 * @example VGA/DoubleBuffer/main/DoubleBuffer.cpp Double buffering usage
 * @example VGA/FileBrowser/main/FileBrowser.cpp File browser (SPIFFS and SD card) with GUI
 * @example VGA/GraphicalUserInterface/main/GraphicalUserInterface.cpp Graphical User Interface (GUI) demo
 * @example VGA/HardwareTest/main/HardwareTest.cpp Hardware test with GUI
 * @example VGA/LoopbackTerminal/main/LoopbackTerminal.cpp Loopback VT/ANSI terminal
 * @example VGA/ModelineStudio/main/ModelineStudio.cpp Tests VGA output using predefined resolutions or custom Linux-style modelines
 * @example VGA/MouseOnScreen/main/MouseOnScreen.cpp PS/2 mouse with on-screen mouse pointer
 * @example VGA/MultitaskingCPM/main/MultitaskingCPM.cpp Multitasking, multi-session CP/M 3 compatible system, with directories and FAT32 file system on SD cards
 * @example VGA/NetworkTerminal/main/NetworkTerminal.cpp Network VT/ANSI terminal
 * @example VGA/PCEmulator/main/PCEmulator.cpp Intel 8086 based IBM PC emulator (runs FreeDOS, MS-DOS, CP/M-86, ELKS Linux, Windows 3.0)
 * @example VGA/SimpleTerminalOut/main/SimpleTerminalOut.cpp Simple terminal, output only
 * @example VGA/SimpleTextTerminalOut/main/SimpleTextTerminalOut.cpp Simple terminal in text-only mode, output only
 * @example VGA/Songs/main/Songs.cpp Music and sound demo, by Carles Oriol
 * @example VGA/SpaceInvaders/main/SpaceInvaders.cpp Complete Space Invaders game
 * @example VGA/SoundChipSimulator/main/SoundChipSimulator.cpp Sound chip simulator, by Carles Oriol
 * @example VGA/Sprites/main/Sprites.cpp Simple sprite animation
 * @example VGA/Synth/main/Synth.cpp FabGL Synth: polyphonic synthesizer played with the PS/2 keyboard or the mouse (resonant low-pass filter, ADSR envelope, GUI)
 * @example VGA/TerminalCharSet/main/TerminalCharSet.cpp Terminal character set
 * @example VGA/VIC20/main/VIC20.cpp Commodore VIC-20 emulator
 * @example VGA/InputBox/main/InputBox.cpp UI wizards using fabgl::InputBox
 *
 * @example Others/KeyboardScanCodes/main/KeyboardScanCodes.cpp PS/2 keyboard scancodes
 * @example Others/KeyboardVirtualKeys/main/KeyboardVirtualKeys.cpp PS/2 keyboard virtual keys
 * @example Others/MouseStudio/main/MouseStudio.cpp PS/2 mouse events
 *
 * @example Composite/BaseExample/main/BaseExample.cpp Base example for PAL/NTSC composite video output
 */


/*!
    \defgroup Enumerations Enumerations
        Enumeration types
*/


#include "fabutils.h"
#include "fabfonts.h"
#include "terminal.h"
#include "displaycontroller.h"
#include "dispdrivers/vgacontroller.h"
#include "dispdrivers/vgatextcontroller.h"
#include "dispdrivers/vga2controller.h"
#include "dispdrivers/vga4controller.h"
#include "dispdrivers/vga8controller.h"
#include "dispdrivers/vga16controller.h"
#include "dispdrivers/vgadirectcontroller.h"
#include "dispdrivers/cvbs16controller.h"
#include "fabui.h"
#include "inputbox.h"
#include "comdrivers/ps2controller.h"
#include "comdrivers/tsi2c.h"
#include "devdrivers/keyboard.h"
#include "devdrivers/mouse.h"
#include "devdrivers/DS3231.h"
#include "comdrivers/serialport.h"
#include "comdrivers/uartstream.h"
#include "scene.h"
#include "collisiondetector.h"
#include "devdrivers/soundgen.h"
#include "preferences.h"



using fabgl::Color;
using fabgl::VGAScanStart;
using fabgl::GlyphOptions;
using fabgl::Scene;
using fabgl::Bitmap;
using fabgl::Sprite;
using fabgl::CollisionDetector;
using fabgl::Point;
using fabgl::Size;
using fabgl::Rect;
using fabgl::MouseDelta;
using fabgl::MouseStatus;
using fabgl::CursorName;
using fabgl::SoundGenerator;
using fabgl::SoundGenMethod;
using fabgl::SineWaveformGenerator;
using fabgl::SquareWaveformGenerator;
using fabgl::NoiseWaveformGenerator;
using fabgl::VICNoiseGenerator;
using fabgl::TriangleWaveformGenerator;
using fabgl::SawtoothWaveformGenerator;
using fabgl::SamplesGenerator;
using fabgl::WaveformGenerator;
using fabgl::TermType;
using fabgl::SupportedTerminals;
using fabgl::PS2Preset;
using fabgl::PS2DeviceType;
using fabgl::KbdMode;
using fabgl::VirtualKey;
using fabgl::VirtualKeyItem;
using fabgl::FileBrowser;
using fabgl::ModalWindowState;
using fabgl::Canvas;
using fabgl::PixelFormat;
using fabgl::RGB222;
using fabgl::RGBA2222;
using fabgl::RGB888;
using fabgl::RGBA8888;
using fabgl::FlowControl;
using fabgl::LineEditorSpecialChar;
using fabgl::LineEditor;
using fabgl::TerminalController;
using fabgl::LineEnds;
using fabgl::CharStyle;
using fabgl::TerminalTransition;
using fabgl::SupportedLayouts;
using fabgl::CoreUsage;
using fabgl::InputResult;
using fabgl::InputBox;
using fabgl::Preferences;
using fabgl::UARTStream;


