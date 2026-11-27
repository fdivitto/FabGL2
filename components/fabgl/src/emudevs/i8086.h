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

#include "fabgl.h"

#include <stdint.h>


#define I8086_SHOW_OPCODE_STATS 0

#ifndef I80186MODE
  #define I80186MODE 0
#endif



namespace fabgl {


/**
 * @brief Intel 8086 (with some 80186 extensions) CPU emulator
 *
 * The whole CPU state is kept in a single internal structure (see i8086.cpp), so all members are static.
 */
class i8086 {

public:

  // callbacks

  typedef void (*WritePort)(void * context, int address, uint8_t value);
  typedef uint8_t (*ReadPort)(void * context, int address);
  typedef void (*WriteVideoMemory8)(void * context, int address, uint8_t value);
  typedef void (*WriteVideoMemory16)(void * context, int address, uint16_t value);
  typedef uint8_t (*ReadVideoMemory8)(void * context, int address);
  typedef uint16_t (*ReadVideoMemory16)(void * context, int address);
  typedef bool (*Interrupt)(void * context, int num);


  static void setCallbacks(void * context, ReadPort readPort, WritePort writePort, WriteVideoMemory8 writeVideoMemory8, WriteVideoMemory16 writeVideoMemory16, ReadVideoMemory8 readVideoMemory8, ReadVideoMemory16 readVideoMemory16, Interrupt interrupt);

  static void setMemory(uint8_t * memory);

  static void reset();

  static void setAL(uint8_t value);
  static void setAH(uint8_t value);
  static void setBL(uint8_t value);
  static void setBH(uint8_t value);
  static void setCL(uint8_t value);
  static void setCH(uint8_t value);
  static void setDL(uint8_t value);
  static void setDH(uint8_t value);

  static uint8_t AL();
  static uint8_t AH();
  static uint8_t BL();
  static uint8_t BH();
  static uint8_t CL();
  static uint8_t CH();
  static uint8_t DL();
  static uint8_t DH();

  static void setAX(uint16_t value);
  static void setBX(uint16_t value);
  static void setCX(uint16_t value);
  static void setDX(uint16_t value);
  static void setDI(uint16_t value);
  static void setCS(uint16_t value);
  static void setDS(uint16_t value);
  static void setSS(uint16_t value);
  static void setES(uint16_t value);
  static void setIP(uint16_t value);
  static void setSP(uint16_t value);

  static uint16_t AX();
  static uint16_t BX();
  static uint16_t CX();
  static uint16_t DX();
  static uint16_t BP();
  static uint16_t SI();
  static uint16_t DI();
  static uint16_t SP();

  static uint16_t CS();
  static uint16_t ES();
  static uint16_t DS();
  static uint16_t SS();

  static bool flagIF();
  static bool flagTF();
  static bool flagCF();
  static bool flagZF();
  static bool flagOF();
  static bool flagDF();
  static bool flagSF();
  static bool flagAF();
  static bool flagPF();

  static void setFlagZF(bool value);
  static void setFlagCF(bool value);

  static uint16_t IP();

  static bool halted();

  static bool IRQ(uint8_t interrupt_num);

  static void step();

};


} // namespace fabgl
