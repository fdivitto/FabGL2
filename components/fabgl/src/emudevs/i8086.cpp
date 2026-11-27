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


// Intel 8086 (plus some 80186 instructions) CPU emulator
//
// Rewritten from scratch by Fabrizio Di Vittorio. Previous versions were based on
// 8086tiny (Adrian Cable) and 8086tiny plus (Julian Olds).
//
// Implementation notes:
//   - direct switch on the opcode, prefixes (segment override, REP, LOCK) decoded in the same step of the
//     instruction they apply to
//   - registers are not mapped inside emulated memory: register operands never pass through the video memory checks
//   - ModRM operands are decoded once into a direct pointer (register or RAM) or a video address
//   - flags computed directly by specialized 8/16 bit ALU functions
//   - REP MOVS/STOS on plain RAM executed with memmove/memset when semantically equivalent
//     (REP MOVS copied element by element with MEMW on ESP32 before rev.3, because of PSRAM bug)
//   - emulator commands executed as INT instead of custom CPU opcodes
//   - reset to 0xffff:0000 as real 8086
//   - memory read/write by callbacks
//   - 80186 instructions: PUSHA, POPA, BOUND, PUSH imm, IMUL imm, INS, OUTS, shifts/rotates by imm, ENTER, LEAVE


#include <stdio.h>
#include <string.h>

#include "i8086.h"

#if !defined(FABGL_EMULATED) && defined(CONFIG_IDF_TARGET_ESP32)
  #include "esp_chip_info.h"
#endif


namespace fabgl {



#pragma GCC optimize ("O3")

#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"


#define VIDEOMEM_START 0xA0000
#define VIDEOMEM_END   0xC0000

#define ISVIDEO(addr)  ((uint32_t)(addr) - VIDEOMEM_START < (uint32_t)(VIDEOMEM_END - VIDEOMEM_START))

#define I8086_LIKELY(x)   __builtin_expect(!!(x), 1)
#define I8086_UNLIKELY(x) __builtin_expect(!!(x), 0)

#define I8086_INLINE   inline __attribute__((always_inline))
#define I8086_NOINLINE __attribute__((noinline))


// 16 bit type usable to access memory at any (even odd) address
typedef uint16_t __attribute__((__may_alias__)) u16a;


// 16 bit registers indexes (same order of x86 encoding)
enum { AXi, CXi, DXi, BXi, SPi, BPi, SIi, DIi };

// segment registers indexes (same order of x86 encoding)
enum { ESi, CSi, SSi, DSi };


namespace {

struct CPUState {

  // general registers
  union {
    uint16_t w[8];    // AX, CX, DX, BX, SP, BP, SI, DI
    uint8_t  b[16];   // AL, AH, CL, CH, DL, DH, BL, BH, ... (little endian host required)
  } r;

  // segment registers: ES, CS, SS, DS
  uint16_t s[4];

  uint16_t ip;

  // flags (each one 0 or 1)
  // PF is lazy: PFb holds the byte whose parity is the flag (parity[PFb])
  uint8_t  CF, PFb, AF, ZF, SF, TF, IF, DF, OF;

  bool     halted;
  bool     pendingIRQ;

  // true on ESP32 before rev.3: PSRAM to PSRAM copies must interleave stores and loads with MEMW (see PSRAM_WORKAROUND2)
  bool     psramBug;
  uint8_t  pendingIRQIndex;

  uint8_t *                  mem;

  void *                     context;
  i8086::ReadPort            readPort;
  i8086::WritePort           writePort;
  i8086::WriteVideoMemory8   writeVideoMemory8;
  i8086::WriteVideoMemory16  writeVideoMemory16;
  i8086::ReadVideoMemory8    readVideoMemory8;
  i8086::ReadVideoMemory16   readVideoMemory16;
  i8086::Interrupt           interrupt;

};

// Operand decoded from ModRM.
// p != nullptr : direct pointer to register or RAM
// p == nullptr : video memory, a is the linear address
struct Opnd {
  uint8_t * p;
  uint32_t  a;
};

}


static CPUState g;


#define vAX g.r.w[AXi]
#define vCX g.r.w[CXi]
#define vDX g.r.w[DXi]
#define vBX g.r.w[BXi]
#define vSP g.r.w[SPi]
#define vBP g.r.w[BPi]
#define vSI g.r.w[SIi]
#define vDI g.r.w[DIi]

#define vAL g.r.b[0]
#define vAH g.r.b[1]
#define vCL g.r.b[2]
#define vCH g.r.b[3]
#define vDL g.r.b[4]
#define vDH g.r.b[5]
#define vBL g.r.b[6]
#define vBH g.r.b[7]

#define vES g.s[ESi]
#define vCS g.s[CSi]
#define vSS g.s[SSi]
#define vDS g.s[DSi]

// 16 bit register by x86 encoding index (0..7)
#define REG16(n)   g.r.w[n]

// 8 bit register by x86 encoding index (0 = AL, 1 = CL, 2 = DL, 3 = BL, 4 = AH, 5 = CH, 6 = DH, 7 = BH)
#define REG8(n)    g.r.b[(((n) & 3) << 1) | (((n) >> 2) & 1)]

#define SEGBASE(sr) ((uint32_t)g.s[sr] << 4)


#if I80186MODE
  #define FLAGS_FIXED 0x0002    // to pass test186 tests, just unused bit nr. 1 is set to 1 (some programs checks this to know if this is a 80186 or 8086)
#else
  #define FLAGS_FIXED 0xf002    // for real 8086
#endif


// Parity flag loop-up table (1 = even number of bits set)
static uint8_t parity[256] = {  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
                                0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
                                0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
                                1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
                                0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
                                1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
                                1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
                                0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
                                0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
                                1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
                                1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
                                0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
                                1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
                                0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
                                0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
                                1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1 };



/////////////////////////////////////////////////////////////////////////////
// Public interface


void i8086::setCallbacks(void * context, ReadPort readPort, WritePort writePort, WriteVideoMemory8 writeVideoMemory8, WriteVideoMemory16 writeVideoMemory16, ReadVideoMemory8 readVideoMemory8, ReadVideoMemory16 readVideoMemory16, Interrupt interrupt)
{
  g.context            = context;
  g.readPort           = readPort;
  g.writePort          = writePort;
  g.writeVideoMemory8  = writeVideoMemory8;
  g.writeVideoMemory16 = writeVideoMemory16;
  g.readVideoMemory8   = readVideoMemory8;
  g.readVideoMemory16  = readVideoMemory16;
  g.interrupt          = interrupt;
}


void i8086::setMemory(uint8_t * memory)
{
  g.mem = memory;

  #if !defined(FABGL_EMULATED) && defined(CONFIG_IDF_TARGET_ESP32)
  // ESP32 rev.0, rev.1 and rev.1.1 PSRAM cache bug, fixed in rev.3 (revision format is MXX)
  esp_chip_info_t info;
  esp_chip_info(&info);
  g.psramBug = info.revision < 300;
  #else
  g.psramBug = false;
  #endif
}

void i8086::setAL(uint8_t value)            { vAL = value; }
void i8086::setAH(uint8_t value)            { vAH = value; }
void i8086::setBL(uint8_t value)            { vBL = value; }
void i8086::setBH(uint8_t value)            { vBH = value; }
void i8086::setCL(uint8_t value)            { vCL = value; }
void i8086::setCH(uint8_t value)            { vCH = value; }
void i8086::setDL(uint8_t value)            { vDL = value; }
void i8086::setDH(uint8_t value)            { vDH = value; }

uint8_t i8086::AL()                         { return vAL; }
uint8_t i8086::AH()                         { return vAH; }
uint8_t i8086::BL()                         { return vBL; }
uint8_t i8086::BH()                         { return vBH; }
uint8_t i8086::CL()                         { return vCL; }
uint8_t i8086::CH()                         { return vCH; }
uint8_t i8086::DL()                         { return vDL; }
uint8_t i8086::DH()                         { return vDH; }

void i8086::setAX(uint16_t value)           { vAX = value; }
void i8086::setBX(uint16_t value)           { vBX = value; }
void i8086::setCX(uint16_t value)           { vCX = value; }
void i8086::setDX(uint16_t value)           { vDX = value; }
void i8086::setDI(uint16_t value)           { vDI = value; }
void i8086::setCS(uint16_t value)           { vCS = value; }
void i8086::setDS(uint16_t value)           { vDS = value; }
void i8086::setSS(uint16_t value)           { vSS = value; }
void i8086::setES(uint16_t value)           { vES = value; }
void i8086::setIP(uint16_t value)           { g.ip = value; }
void i8086::setSP(uint16_t value)           { vSP = value; }

uint16_t i8086::AX()                        { return vAX; }
uint16_t i8086::BX()                        { return vBX; }
uint16_t i8086::CX()                        { return vCX; }
uint16_t i8086::DX()                        { return vDX; }
uint16_t i8086::BP()                        { return vBP; }
uint16_t i8086::SI()                        { return vSI; }
uint16_t i8086::DI()                        { return vDI; }
uint16_t i8086::SP()                        { return vSP; }

uint16_t i8086::CS()                        { return vCS; }
uint16_t i8086::ES()                        { return vES; }
uint16_t i8086::DS()                        { return vDS; }
uint16_t i8086::SS()                        { return vSS; }

uint16_t i8086::IP()                        { return g.ip; }

bool i8086::flagIF()                        { return g.IF; }
bool i8086::flagTF()                        { return g.TF; }
bool i8086::flagCF()                        { return g.CF; }
bool i8086::flagZF()                        { return g.ZF; }
bool i8086::flagOF()                        { return g.OF; }
bool i8086::flagDF()                        { return g.DF; }
bool i8086::flagSF()                        { return g.SF; }
bool i8086::flagAF()                        { return g.AF; }
bool i8086::flagPF()                        { return parity[g.PFb]; }

void i8086::setFlagZF(bool value)           { g.ZF = value; }
void i8086::setFlagCF(bool value)           { g.CF = value; }

bool i8086::halted()                        { return g.halted; }


// ret false if not acked
bool i8086::IRQ(uint8_t interrupt_num)
{
  if (!g.pendingIRQ) {
    g.pendingIRQ      = true;
    g.pendingIRQIndex = interrupt_num;
    return true;
  } else {
    return false;
  }
}


void i8086::reset()
{
  memset(&g.r, 0, sizeof(g.r));
  memset(g.s, 0, sizeof(g.s));

  g.CF = g.AF = g.ZF = g.SF = g.TF = g.IF = g.DF = g.OF = 0;
  g.PFb = 1;  // PF = 0

  g.halted = false;

  vCS  = 0xffff;
  g.ip = 0;
}



/////////////////////////////////////////////////////////////////////////////
// Memory access


static I8086_INLINE uint16_t fetch16(uint8_t const * & pc)
{
  uint16_t r = *(u16a const *)pc;
  pc += 2;
  return r;
}


static I8086_INLINE uint8_t rdmem8(uint32_t addr)
{
  if (I8086_UNLIKELY(ISVIDEO(addr)))
    return g.readVideoMemory8(g.context, addr);
  return g.mem[addr];
}


static I8086_INLINE uint16_t rdmem16(uint32_t addr)
{
  if (I8086_UNLIKELY(ISVIDEO(addr)))
    return g.readVideoMemory16(g.context, addr);
  return *(u16a *)(g.mem + addr);
}


static I8086_INLINE void wrmem8(uint32_t addr, uint8_t value)
{
  if (I8086_UNLIKELY(ISVIDEO(addr)))
    g.writeVideoMemory8(g.context, addr, value);
  else
    g.mem[addr] = value;
}


static I8086_INLINE void wrmem16(uint32_t addr, uint16_t value)
{
  if (I8086_UNLIKELY(ISVIDEO(addr)))
    g.writeVideoMemory16(g.context, addr, value);
  else
    *(u16a *)(g.mem + addr) = value;
}


static I8086_INLINE bool overlapsVideo(uint32_t addr, uint32_t len)
{
  return addr < VIDEOMEM_END && addr + len > VIDEOMEM_START;
}


static I8086_INLINE uint8_t rd8(Opnd const & o)
{
  return I8086_LIKELY(o.p != nullptr) ? *o.p : g.readVideoMemory8(g.context, o.a);
}


static I8086_INLINE uint16_t rd16(Opnd const & o)
{
  return I8086_LIKELY(o.p != nullptr) ? *(u16a *)o.p : g.readVideoMemory16(g.context, o.a);
}


// reads second word of a far pointer
static I8086_INLINE uint16_t rd16hi(Opnd const & o)
{
  return I8086_LIKELY(o.p != nullptr) ? *(u16a *)(o.p + 2) : g.readVideoMemory16(g.context, o.a + 2);
}


static I8086_INLINE void wr8(Opnd const & o, uint8_t value)
{
  if (I8086_LIKELY(o.p != nullptr))
    *o.p = value;
  else
    g.writeVideoMemory8(g.context, o.a, value);
}


static I8086_INLINE void wr16(Opnd const & o, uint16_t value)
{
  if (I8086_LIKELY(o.p != nullptr))
    *(u16a *)o.p = value;
  else
    g.writeVideoMemory16(g.context, o.a, value);
}


// stack is always accessed directly (never video memory)

static I8086_INLINE void push(uint16_t value)
{
  vSP -= 2;
  *(u16a *)(g.mem + SEGBASE(SSi) + vSP) = value;
}


static I8086_INLINE uint16_t pop()
{
  uint16_t r = *(u16a *)(g.mem + SEGBASE(SSi) + vSP);
  vSP += 2;
  return r;
}


/////////////////////////////////////////////////////////////////////////////
// ModRM decoding


// calculates effective address offset (without segment), consuming displacement bytes
// mod must be 0, 1 or 2
static I8086_INLINE uint16_t eaOffset(uint8_t modrm, uint8_t const * & pc, int & seg)
{
  uint16_t off;
  uint8_t const mod = modrm >> 6;
  switch (modrm & 7) {
    case 0:  off = vBX + vSI; break;
    case 1:  off = vBX + vDI; break;
    case 2:  off = vBP + vSI; seg = SSi; break;
    case 3:  off = vBP + vDI; seg = SSi; break;
    case 4:  off = vSI;       break;
    case 5:  off = vDI;       break;
    case 6:
      if (mod == 0)
        return fetch16(pc);
      off = vBP;
      seg = SSi;
      break;
    default: off = vBX;       break;
  }
  if (mod == 1)
    off += (int8_t) *pc++;
  else if (mod == 2)
    off += fetch16(pc);
  return off;
}


// calculates linear address of a memory operand (mod must be 0, 1 or 2)
static I8086_INLINE uint32_t eaLinear(uint8_t modrm, uint8_t const * & pc, int ovr)
{
  int seg = DSi;
  uint16_t off = eaOffset(modrm, pc, seg);
  if (ovr >= 0)
    seg = ovr;
  return SEGBASE(seg) + off;
}


static I8086_INLINE Opnd decodeMem(uint8_t modrm, uint8_t const * & pc, int ovr)
{
  Opnd o;
  o.a = eaLinear(modrm, pc, ovr);
  o.p = ISVIDEO(o.a) ? nullptr : g.mem + o.a;
  return o;
}


static I8086_INLINE Opnd decodeRM8(uint8_t modrm, uint8_t const * & pc, int ovr)
{
  if (modrm >= 0xc0) {
    Opnd o;
    o.p = &REG8(modrm & 7);
    o.a = 0;
    return o;
  }
  return decodeMem(modrm, pc, ovr);
}


static I8086_INLINE Opnd decodeRM16(uint8_t modrm, uint8_t const * & pc, int ovr)
{
  if (modrm >= 0xc0) {
    Opnd o;
    o.p = (uint8_t *) &REG16(modrm & 7);
    o.a = 0;
    return o;
  }
  return decodeMem(modrm, pc, ovr);
}


/////////////////////////////////////////////////////////////////////////////
// Flags and ALU


static I8086_INLINE uint16_t makeFlags()
{
  return FLAGS_FIXED | g.CF | parity[g.PFb] << 2 | g.AF << 4 | g.ZF << 6 | g.SF << 7 | g.TF << 8 | g.IF << 9 | g.DF << 10 | g.OF << 11;
}


static I8086_INLINE void setFlags(uint16_t f)
{
  g.CF = (f >>  0) & 1;
  g.PFb = (~f >> 2) & 1;   // 0 -> PF = 1, 1 -> PF = 0
  g.AF = (f >>  4) & 1;
  g.ZF = (f >>  6) & 1;
  g.SF = (f >>  7) & 1;
  g.TF = (f >>  8) & 1;
  g.IF = (f >>  9) & 1;
  g.DF = (f >> 10) & 1;
  g.OF = (f >> 11) & 1;
}


template <int N>
static I8086_INLINE void setSZP(uint32_t r)
{
  g.SF = (r >> (N - 1)) & 1;
  g.ZF = (r & ((1u << N) - 1)) == 0;
  g.PFb = r;
}


template <int N>
static I8086_INLINE uint32_t opADD(uint32_t a, uint32_t b, uint32_t carry)
{
  uint32_t r = a + b + carry;
  g.CF = (r >> N) & 1;
  g.OF = (((r ^ a) & (r ^ b)) >> (N - 1)) & 1;
  g.AF = ((a ^ b ^ r) >> 4) & 1;
  setSZP<N>(r);
  return r & ((1u << N) - 1);
}


template <int N>
static I8086_INLINE uint32_t opSUB(uint32_t a, uint32_t b, uint32_t borrow)
{
  uint32_t r = a - b - borrow;
  g.CF = (r >> N) & 1;
  g.OF = (((a ^ b) & (a ^ r)) >> (N - 1)) & 1;
  g.AF = ((a ^ b ^ r) >> 4) & 1;
  setSZP<N>(r);
  return r & ((1u << N) - 1);
}


// logic operations: AF unchanged
template <int N>
static I8086_INLINE uint32_t opLOG(uint32_t r)
{
  g.CF = g.OF = 0;
  setSZP<N>(r);
  return r;
}


// CF unchanged
template <int N>
static I8086_INLINE uint32_t opINC(uint32_t a)
{
  uint32_t r = (a + 1) & ((1u << N) - 1);
  g.OF = r == (1u << (N - 1));
  g.AF = ((a ^ r) >> 4) & 1;
  setSZP<N>(r);
  return r;
}


// CF unchanged
template <int N>
static I8086_INLINE uint32_t opDEC(uint32_t a)
{
  uint32_t r = (a - 1) & ((1u << N) - 1);
  g.OF = a == (1u << (N - 1));
  g.AF = ((a ^ r) >> 4) & 1;
  setSZP<N>(r);
  return r;
}


// op: 0 = ADD, 1 = OR, 2 = ADC, 3 = SBB, 4 = AND, 5 = SUB, 6 = XOR, 7 = CMP
template <int N>
static I8086_INLINE uint32_t alu(int op, uint32_t a, uint32_t b)
{
  switch (op) {
    case 0:  return opADD<N>(a, b, 0);
    case 1:  return opLOG<N>(a | b);
    case 2:  return opADD<N>(a, b, g.CF);
    case 3:  return opSUB<N>(a, b, g.CF);
    case 4:  return opLOG<N>(a & b);
    case 6:  return opLOG<N>(a ^ b);
    default: return opSUB<N>(a, b, 0);  // SUB, CMP
  }
}


// op: 0 = ROL, 1 = ROR, 2 = RCL, 3 = RCR, 4 = SHL, 5 = SHR, 6 = SHL (undocumented), 7 = SAR
// count: 1..31
template <int N>
static I8086_NOINLINE uint32_t shiftOp(int op, uint32_t v, uint32_t count)
{
  constexpr uint32_t MASK = (1u << N) - 1;
  uint32_t r;
  switch (op) {

    case 0: // ROL
    {
      uint32_t c = count & (N - 1);
      r = c ? ((v << c) | (v >> (N - c))) & MASK : v;
      g.CF = r & 1;
      g.OF = ((r >> (N - 1)) ^ r) & 1;
      return r;
    }

    case 1: // ROR
    {
      uint32_t c = count & (N - 1);
      r = c ? ((v >> c) | (v << (N - c))) & MASK : v;
      g.CF = (r >> (N - 1)) & 1;
      g.OF = ((r >> (N - 1)) ^ (r >> (N - 2))) & 1;
      return r;
    }

    case 2: // RCL
    {
      uint32_t c = count % (N + 1);
      if (c == 0)
        return v;
      r = ((v << c) | ((uint32_t)g.CF << (c - 1)) | (v >> (N + 1 - c))) & MASK;
      g.CF = (v >> (N - c)) & 1;
      g.OF = ((r >> (N - 1)) ^ g.CF) & 1;
      return r;
    }

    case 3: // RCR
    {
      uint32_t c = count % (N + 1);
      if (c == 0)
        return v;
      r = ((v >> c) | ((uint32_t)g.CF << (N - c)) | (v << (N + 1 - c))) & MASK;
      g.CF = (v >> (c - 1)) & 1;
      g.OF = ((r >> (N - 1)) ^ (r >> (N - 2))) & 1;
      return r;
    }

    case 4: // SHL
    case 6:
      r = (v << count) & MASK;
      g.CF = count <= N ? (v >> (N - count)) & 1 : 0;
      g.OF = ((r >> (N - 1)) ^ g.CF) & 1;
      break;

    case 5: // SHR
      r = v >> count;
      g.CF = (v >> (count - 1)) & 1;
      g.OF = (v >> (N - 1)) & 1;
      break;

    default: // SAR
    {
      int32_t sv = (N == 8) ? (int32_t)(int8_t)v : (int32_t)(int16_t)v;
      r = (uint32_t)(sv >> count) & MASK;
      g.CF = (sv >> (count - 1)) & 1;
      g.OF = 0;
      break;
    }

  }
  setSZP<N>(r);
  return r;
}


// MUL, IMUL, DIV, IDIV
// op: 4 = MUL, 5 = IMUL, 6 = DIV, 7 = IDIV
// returns false on divide error
static I8086_NOINLINE bool mulDiv(int op, bool w, uint32_t v)
{
  switch (op) {

    case 4: // MUL
      if (w) {
        uint32_t p = v * vAX;
        vAX  = p;
        vDX  = p >> 16;
        g.CF = g.OF = (p >> 16) != 0;
        g.SF = (p >> 15) & 1;
      } else {
        uint32_t p = v * vAL;
        vAX  = p;
        g.CF = g.OF = (p >> 8) != 0;
        g.SF = (p >> 7) & 1;
        g.ZF = p == 0;
        g.PFb = p;
        return true;
      }
      g.ZF = (vAX | vDX) == 0;
      g.PFb = vAL;
      return true;

    case 5: // IMUL
      if (w) {
        int32_t p = (int32_t)(int16_t)v * (int16_t)vAX;
        vAX  = p;
        vDX  = (uint32_t)p >> 16;
        g.CF = g.OF = p != (int16_t)p;
        g.SF = (p >> 15) & 1;
        g.ZF = p == 0;
      } else {
        int32_t p = (int32_t)(int8_t)v * (int8_t)vAL;
        vAX  = p;
        g.CF = g.OF = p != (int8_t)p;
        g.SF = (p >> 7) & 1;
        g.ZF = p == 0;
      }
      g.PFb = vAL;
      return true;

    case 6: // DIV
      if (v == 0)
        return false;
      if (w) {
        uint32_t n = ((uint32_t)vDX << 16) | vAX;
        uint32_t q = n / v;
        if (q > 0xffff)
          return false;
        vDX = n - q * v;
        vAX = q;
      } else {
        uint32_t n = vAX;
        uint32_t q = n / v;
        if (q > 0xff)
          return false;
        vAH = n - q * v;
        vAL = q;
      }
      return true;

    default: // IDIV
      if (w) {
        int32_t d = (int16_t)v;
        int32_t n = (int32_t)(((uint32_t)vDX << 16) | vAX);
        if (d == 0 || (n == INT32_MIN && d == -1))
          return false;
        int32_t q = n / d;
        if (q != (int16_t)q)
          return false;
        vDX = n - q * d;
        vAX = q;
      } else {
        int32_t d = (int8_t)v;
        if (d == 0)
          return false;
        int32_t n = (int16_t)vAX;
        int32_t q = n / d;
        if (q != (int8_t)q)
          return false;
        vAH = n - q * d;
        vAL = q;
      }
      return true;

  }
}


static I8086_NOINLINE void opDAA_DAS(bool das)
{
  g.AF = (vAL & 0x0f) > 9 || g.AF;
  g.CF = vAL > 0x99 || g.CF;
  if (das) {
    if (g.CF)
      vAL -= 0x60;
    else if (g.AF)
      g.CF = (vAL < 6);
    if (g.AF)
      vAL -= 6;
  } else {
    if (g.CF)
      vAL += 0x60;
    if (g.AF)
      vAL += 6;
  }
  setSZP<8>(vAL);
}


static I8086_NOINLINE void opAAA_AAS(bool aas)
{
  g.AF = g.CF = ((vAL & 0x0f) > 9) || g.AF;
  if (g.AF)
    vAX += aas ? -262 : 262;
  vAL &= 0x0f;
  g.SF = 0;
  g.ZF = vAL == 0;
  g.PFb = vAL;
}


// evaluates condition of Jcc (opcodes 0x70..0x7f)
static I8086_INLINE bool condition(uint8_t op)
{
  bool r;
  switch ((op >> 1) & 7) {
    case 0:  r = g.OF;                   break;  // JO
    case 1:  r = g.CF;                   break;  // JB
    case 2:  r = g.ZF;                   break;  // JZ
    case 3:  r = g.CF | g.ZF;            break;  // JBE
    case 4:  r = g.SF;                   break;  // JS
    case 5:  r = parity[g.PFb];          break;  // JP
    case 6:  r = g.SF ^ g.OF;            break;  // JL
    default: r = g.ZF | (g.SF ^ g.OF);   break;  // JLE
  }
  return r ^ (op & 1);
}


/////////////////////////////////////////////////////////////////////////////
// Interrupts


// Execute INT #interrupt_num on the emulated machine
static I8086_NOINLINE void pcInterrupt(uint8_t num)
{
  // interrupt can exit from halt state
  if (g.halted) {
    g.halted = false;
    ++g.ip; // go to next instruction after HLT
  }

  if (!g.interrupt(g.context, num)) {
    u16a const * vect = (u16a const *)(g.mem + 4 * num);
    uint16_t newIP = vect[0];
    uint16_t newCS = vect[1];

    vSP -= 6;
    u16a * stack = (u16a *)(g.mem + SEGBASE(SSi) + vSP);
    stack[2] = makeFlags();
    stack[1] = vCS;
    stack[0] = g.ip;

    g.ip = newIP;
    vCS  = newCS;

    g.TF = g.IF = 0;
  }
}


/////////////////////////////////////////////////////////////////////////////
// String instructions


// op: string opcode (0x6c..0x6f, 0xa4..0xa7, 0xaa..0xaf)
// rep: 0 = no REP, 1 = REPNE/REPNZ, 2 = REP/REPE/REPZ
static I8086_NOINLINE void stringOp(uint8_t op, int rep, int ovr)
{
  uint32_t count = rep ? vCX : 1;
  if (count == 0)
    return;

  int const      srcSeg = ovr >= 0 ? ovr : DSi;
  bool const     w      = op & 1;
  uint32_t const size   = w ? 2 : 1;
  uint16_t const delta  = g.DF ? -size : size;

  switch (op) {

    // MOVSB, MOVSW
    case 0xa4:
    case 0xa5:
      // memmove() interleaves PSRAM loads and stores without MEMW, so it is not usable on ESP32 rev.1
      if (rep && !g.DF && !g.psramBug) {
        uint32_t total = count * size;
        if (vSI + total <= 0x10000 && vDI + total <= 0x10000) {
          uint32_t src = SEGBASE(srcSeg) + vSI;
          uint32_t dst = SEGBASE(ESi) + vDI;
          // fast path when equivalent to a sequential copy
          if (!overlapsVideo(src, total) && !overlapsVideo(dst, total) && (dst <= src || dst >= src + total)) {
            memmove(g.mem + dst, g.mem + src, total);
            vSI += total;
            vDI += total;
            vCX  = 0;
            return;
          }
        }
      }
      do {
        uint32_t src = SEGBASE(srcSeg) + vSI;
        uint32_t dst = SEGBASE(ESi) + vDI;
        if (w)
          wrmem16(dst, rdmem16(src));
        else
          wrmem8(dst, rdmem8(src));
        PSRAM_WORKAROUND2
        vSI += delta;
        vDI += delta;
      } while (--count);
      break;

    // STOSB, STOSW
    case 0xaa:
    case 0xab:
      if (rep && !g.DF) {
        uint32_t total = count * size;
        uint32_t dst   = SEGBASE(ESi) + vDI;
        if (vDI + total <= 0x10000 && !overlapsVideo(dst, total)) {
          if (!w || vAL == vAH) {
            memset(g.mem + dst, vAL, total);
          } else {
            u16a * p = (u16a *)(g.mem + dst);
            uint16_t v = vAX;
            for (uint32_t i = 0; i < count; ++i)
              p[i] = v;
          }
          vDI += total;
          vCX  = 0;
          return;
        }
      }
      do {
        uint32_t dst = SEGBASE(ESi) + vDI;
        if (w)
          wrmem16(dst, vAX);
        else
          wrmem8(dst, vAL);
        PSRAM_WORKAROUND2
        vDI += delta;
      } while (--count);
      break;

    // LODSB, LODSW
    case 0xac:
    case 0xad:
      do {
        uint32_t src = SEGBASE(srcSeg) + vSI;
        if (w)
          vAX = rdmem16(src);
        else
          vAL = rdmem8(src);
        vSI += delta;
      } while (--count);
      break;

    // CMPSB, CMPSW, SCASB, SCASW
    case 0xa6:
    case 0xa7:
    case 0xae:
    case 0xaf:
    {
      bool const scas      = op >= 0xae;
      bool const whileEqual = (rep == 2);
      uint32_t a, b;
      while (true) {
        if (scas) {
          a = w ? vAX : vAL;
        } else {
          uint32_t src = SEGBASE(srcSeg) + vSI;
          a = w ? rdmem16(src) : rdmem8(src);
          vSI += delta;
        }
        uint32_t dst = SEGBASE(ESi) + vDI;
        b = w ? rdmem16(dst) : rdmem8(dst);
        vDI += delta;
        if (!rep || --vCX == 0 || (a == b) != whileEqual)
          break;
      }
      if (w)
        opSUB<16>(a, b, 0);
      else
        opSUB<8>(a, b, 0);
      return;
    }

    // INSB, INSW (80186)
    case 0x6c:
    case 0x6d:
      do {
        uint32_t dst = SEGBASE(ESi) + vDI;
        if (w)
          wrmem16(dst, g.readPort(g.context, vDX) | (g.readPort(g.context, vDX + 1) << 8));
        else
          wrmem8(dst, g.readPort(g.context, vDX));
        vDI += delta;
      } while (--count);
      break;

    // OUTSB, OUTSW (80186)
    default:
      do {
        uint32_t src = SEGBASE(srcSeg) + vSI;
        if (w) {
          uint16_t v = rdmem16(src);
          g.writePort(g.context, vDX, v & 0xff);
          g.writePort(g.context, vDX + 1, v >> 8);
        } else
          g.writePort(g.context, vDX, rdmem8(src));
        vSI += delta;
      } while (--count);
      break;

  }

  if (rep)
    vCX = 0;
}


/////////////////////////////////////////////////////////////////////////////
// Instructions execution


// Executes a single instruction (including its prefixes).
// Returns true when the next instruction must be executed immediately (interrupts inhibited).
static I8086_INLINE bool execInstruction()
{
  uint16_t const        ip0   = g.ip;
  uint8_t const * const start = g.mem + SEGBASE(CSi) + ip0;
  uint8_t const *       pc    = start;

  int  ovr    = -1;   // segment override (-1 = none)
  int  rep    = 0;    // 0 = none, 1 = REPNE/REPNZ, 2 = REP/REPE/REPZ
  bool reloop = false;

  // used by ALU instructions group
  int      aluOp;
  Opnd     aluDst;
  uint32_t aluA, aluB;
  bool     aluW;

  uint8_t op;

fetch:
  op = *pc++;

  #if I8086_SHOW_OPCODE_STATS
  {
    static uint32_t opcodeStats[256] = {0};
    static int      opcodeStatsCount = 0;
    static uint64_t opcodeStatsT0 = esp_timer_get_time();
    opcodeStats[op] += 1;
    ++opcodeStatsCount;
    if ((opcodeStatsCount % 500000000) == 0) {
      opcodeStatsCount = 0;
      if (Serial.available()) {
        Serial.read();
        printf("\ntime delta = %llu uS\n\n", esp_timer_get_time() - opcodeStatsT0);
        opcodeStatsT0 = esp_timer_get_time();
        for (int i = 0; i < 256; ++i) {
          if (opcodeStats[i] > 0)
            printf("%d, %02X\n", opcodeStats[i], i);
          opcodeStats[i] = 0;
        }
        printf("\n");
      }
    }
  }
  #endif

  switch (op) {

    // ADD, OR, ADC, SBB, AND, SUB, XOR, CMP  (r/m,reg  reg,r/m  acc,imm)
    case 0x00 ... 0x05:
    case 0x08 ... 0x0d:
    case 0x10 ... 0x15:
    case 0x18 ... 0x1d:
    case 0x20 ... 0x25:
    case 0x28 ... 0x2d:
    case 0x30 ... 0x35:
    case 0x38 ... 0x3d:
    {
      aluOp = op >> 3;
      switch (op & 7) {
        case 0: // r/m8, reg8
        {
          uint8_t m = *pc++;
          aluDst = decodeRM8(m, pc, ovr);
          aluA   = rd8(aluDst);
          aluB   = REG8((m >> 3) & 7);
          aluW   = false;
          break;
        }
        case 1: // r/m16, reg16
        {
          uint8_t m = *pc++;
          aluDst = decodeRM16(m, pc, ovr);
          aluA   = rd16(aluDst);
          aluB   = REG16((m >> 3) & 7);
          aluW   = true;
          break;
        }
        case 2: // reg8, r/m8
        {
          uint8_t m = *pc++;
          Opnd s = decodeRM8(m, pc, ovr);
          aluB     = rd8(s);
          aluDst.p = &REG8((m >> 3) & 7);
          aluA     = *aluDst.p;
          aluW     = false;
          break;
        }
        case 3: // reg16, r/m16
        {
          uint8_t m = *pc++;
          Opnd s = decodeRM16(m, pc, ovr);
          aluB     = rd16(s);
          aluDst.p = (uint8_t *) &REG16((m >> 3) & 7);
          aluA     = *(u16a *)aluDst.p;
          aluW     = true;
          break;
        }
        case 4: // AL, imm8
          aluDst.p = &vAL;
          aluA     = vAL;
          aluB     = *pc++;
          aluW     = false;
          break;
        default: // AX, imm16
          aluDst.p = (uint8_t *) &vAX;
          aluA     = vAX;
          aluB     = fetch16(pc);
          aluW     = true;
          break;
      }
      goto aluExec;
    }

    // PUSH ES, PUSH CS, PUSH SS, PUSH DS
    case 0x06:
    case 0x0e:
    case 0x16:
    case 0x1e:
      push(g.s[op >> 3]);
      break;

    // POP ES, POP SS, POP DS
    case 0x07:
    case 0x17:
    case 0x1f:
      g.s[op >> 3] = pop();
      reloop = true;  // inhibit interrupts until next instruction (actually just for POP SS)
      break;

    // SEG ES, SEG CS, SEG SS, SEG DS
    case 0x26:
    case 0x2e:
    case 0x36:
    case 0x3e:
      ovr = (op >> 3) & 3;
      goto fetch;

    // DAA
    case 0x27:
      opDAA_DAS(false);
      break;

    // DAS
    case 0x2f:
      opDAA_DAS(true);
      break;

    // AAA
    case 0x37:
      opAAA_AAS(false);
      break;

    // AAS
    case 0x3f:
      opAAA_AAS(true);
      break;

    // INC reg16
    case 0x40 ... 0x47:
      REG16(op & 7) = opINC<16>(REG16(op & 7));
      break;

    // DEC reg16
    case 0x48 ... 0x4f:
      REG16(op & 7) = opDEC<16>(REG16(op & 7));
      break;

    // PUSH reg16 (PUSH SP pushes the decremented value, as real 8086)
    case 0x50 ... 0x57:
      vSP -= 2;
      *(u16a *)(g.mem + SEGBASE(SSi) + vSP) = REG16(op & 7);
      break;

    // POP reg16
    case 0x58 ... 0x5f:
    {
      uint16_t v = pop();
      REG16(op & 7) = v;
      break;
    }

    // PUSHA (80186)
    case 0x60:
    {
      uint16_t t = vSP;
      push(vAX);
      push(vCX);
      push(vDX);
      push(vBX);
      push(t);
      push(vBP);
      push(vSI);
      push(vDI);
      break;
    }

    // POPA (80186)
    case 0x61:
      vDI = pop();
      vSI = pop();
      vBP = pop();
      vSP += 2;   // SP ignored
      vBX = pop();
      vDX = pop();
      vCX = pop();
      vAX = pop();
      break;

    // BOUND reg16, m16&16 (80186)
    case 0x62:
    {
      uint8_t m = *pc++;
      if (m < 0xc0) {
        uint32_t a = eaLinear(m, pc, ovr);
        int16_t v = REG16((m >> 3) & 7);
        if (v < (int16_t)rdmem16(a) || v > (int16_t)rdmem16(a + 2)) {
          g.ip = ip0;
          pcInterrupt(5);
          return false;
        }
      }
      break;
    }

    // PUSH imm16 (80186)
    case 0x68:
      push(fetch16(pc));
      break;

    // IMUL reg16, r/m16, imm16 (80186)
    // IMUL reg16, r/m16, imm8 (80186)
    case 0x69:
    case 0x6b:
    {
      uint8_t m = *pc++;
      Opnd s = decodeRM16(m, pc, ovr);
      int32_t a = (int16_t)rd16(s);
      int32_t b = (op == 0x69) ? (int16_t)fetch16(pc) : (int8_t)*pc++;
      int32_t p = a * b;
      REG16((m >> 3) & 7) = p;
      g.CF = g.OF = p != (int16_t)p;
      setSZP<16>(p);
      break;
    }

    // PUSH imm8 (80186)
    case 0x6a:
      push((int8_t)*pc++);
      break;

    // INSB, INSW, OUTSB, OUTSW (80186)
    case 0x6c ... 0x6f:
      stringOp(op, rep, ovr);
      break;

    // Jcc
    case 0x70 ... 0x7f:
    {
      int8_t d = *pc++;
      g.ip = ip0 + (uint16_t)(pc - start) + (condition(op) ? d : 0);
      return false;
    }

    // ADD, OR, ADC, SBB, AND, SUB, XOR, CMP  r/m, imm
    case 0x80 ... 0x83:
    {
      uint8_t m = *pc++;
      aluOp = (m >> 3) & 7;
      if (op & 1) {
        aluDst = decodeRM16(m, pc, ovr);
        aluA   = rd16(aluDst);
        aluB   = (op == 0x83) ? (uint16_t)(int8_t)*pc++ : fetch16(pc);
        aluW   = true;
      } else {
        aluDst = decodeRM8(m, pc, ovr);
        aluA   = rd8(aluDst);
        aluB   = *pc++;
        aluW   = false;
      }
      goto aluExec;
    }

    // TEST r/m8, reg8
    case 0x84:
    {
      uint8_t m = *pc++;
      Opnd o = decodeRM8(m, pc, ovr);
      opLOG<8>(rd8(o) & REG8((m >> 3) & 7));
      break;
    }

    // TEST r/m16, reg16
    case 0x85:
    {
      uint8_t m = *pc++;
      Opnd o = decodeRM16(m, pc, ovr);
      opLOG<16>(rd16(o) & REG16((m >> 3) & 7));
      break;
    }

    // XCHG reg8, r/m8
    case 0x86:
    {
      uint8_t m = *pc++;
      Opnd o = decodeRM8(m, pc, ovr);
      uint8_t & r = REG8((m >> 3) & 7);
      uint8_t t = rd8(o);
      wr8(o, r);
      r = t;
      break;
    }

    // XCHG reg16, r/m16
    case 0x87:
    {
      uint8_t m = *pc++;
      Opnd o = decodeRM16(m, pc, ovr);
      uint16_t & r = REG16((m >> 3) & 7);
      uint16_t t = rd16(o);
      wr16(o, r);
      r = t;
      break;
    }

    // MOV r/m8, reg8
    case 0x88:
    {
      uint8_t m = *pc++;
      Opnd o = decodeRM8(m, pc, ovr);
      wr8(o, REG8((m >> 3) & 7));
      break;
    }

    // MOV r/m16, reg16
    case 0x89:
    {
      uint8_t m = *pc++;
      Opnd o = decodeRM16(m, pc, ovr);
      wr16(o, REG16((m >> 3) & 7));
      break;
    }

    // MOV reg8, r/m8
    case 0x8a:
    {
      uint8_t m = *pc++;
      Opnd o = decodeRM8(m, pc, ovr);
      REG8((m >> 3) & 7) = rd8(o);
      break;
    }

    // MOV reg16, r/m16
    case 0x8b:
    {
      uint8_t m = *pc++;
      Opnd o = decodeRM16(m, pc, ovr);
      REG16((m >> 3) & 7) = rd16(o);
      break;
    }

    // MOV r/m16, sreg
    case 0x8c:
    {
      uint8_t m = *pc++;
      Opnd o = decodeRM16(m, pc, ovr);
      wr16(o, g.s[(m >> 3) & 3]);
      break;
    }

    // LEA reg16, mem
    case 0x8d:
    {
      uint8_t m = *pc++;
      if (m < 0xc0) {
        int seg;
        REG16((m >> 3) & 7) = eaOffset(m, pc, seg);
      }
      break;
    }

    // MOV sreg, r/m16
    case 0x8e:
    {
      uint8_t m = *pc++;
      Opnd o = decodeRM16(m, pc, ovr);
      g.s[(m >> 3) & 3] = rd16(o);
      reloop = true;  // inhibit interrupts until next instruction
      break;
    }

    // POP r/m16
    case 0x8f:
    {
      uint8_t m = *pc++;
      Opnd o = decodeRM16(m, pc, ovr);
      wr16(o, pop());
      break;
    }

    // NOP
    case 0x90:
      break;

    // XCHG AX, reg16
    case 0x91 ... 0x97:
    {
      uint16_t t = vAX;
      vAX = REG16(op & 7);
      REG16(op & 7) = t;
      break;
    }

    // CBW
    case 0x98:
      vAH = (vAL & 0x80) ? 0xff : 0x00;
      break;

    // CWD
    case 0x99:
      vDX = (vAX & 0x8000) ? 0xffff : 0x0000;
      break;

    // CALL far imm16:imm16
    case 0x9a:
    {
      uint16_t newIP = fetch16(pc);
      uint16_t newCS = fetch16(pc);
      push(vCS);
      push(ip0 + (uint16_t)(pc - start));
      vCS  = newCS;
      g.ip = newIP;
      return false;
    }

    // WAIT
    case 0x9b:
      break;

    // PUSHF
    case 0x9c:
      push(makeFlags());
      break;

    // POPF
    case 0x9d:
      setFlags(pop());
      break;

    // SAHF
    case 0x9e:
      setFlags((makeFlags() & 0xff00) | vAH);
      break;

    // LAHF
    case 0x9f:
      vAH = makeFlags();
      break;

    // MOV AL, [moffs]
    case 0xa0:
      vAL = rdmem8(SEGBASE(ovr >= 0 ? ovr : DSi) + fetch16(pc));
      break;

    // MOV AX, [moffs]
    case 0xa1:
      vAX = rdmem16(SEGBASE(ovr >= 0 ? ovr : DSi) + fetch16(pc));
      break;

    // MOV [moffs], AL
    case 0xa2:
      wrmem8(SEGBASE(ovr >= 0 ? ovr : DSi) + fetch16(pc), vAL);
      break;

    // MOV [moffs], AX
    case 0xa3:
      wrmem16(SEGBASE(ovr >= 0 ? ovr : DSi) + fetch16(pc), vAX);
      break;

    // MOVSB, MOVSW, CMPSB, CMPSW
    case 0xa4 ... 0xa7:
      stringOp(op, rep, ovr);
      break;

    // TEST AL, imm8
    case 0xa8:
      opLOG<8>(vAL & *pc++);
      break;

    // TEST AX, imm16
    case 0xa9:
      opLOG<16>(vAX & fetch16(pc));
      break;

    // STOSB, STOSW, LODSB, LODSW, SCASB, SCASW
    case 0xaa ... 0xaf:
      stringOp(op, rep, ovr);
      break;

    // MOV reg8, imm8
    case 0xb0 ... 0xb7:
      REG8(op & 7) = *pc++;
      break;

    // MOV reg16, imm16
    case 0xb8 ... 0xbf:
      REG16(op & 7) = fetch16(pc);
      break;

    // ROL, ROR, RCL, RCR, SHL, SHR, SAL, SAR  r/m, imm8 (80186) / 1 / CL
    case 0xc0:
    case 0xc1:
    case 0xd0 ... 0xd3:
    {
      uint8_t m = *pc++;
      if (op & 1) {
        Opnd o = decodeRM16(m, pc, ovr);
        uint32_t count = (op < 0xd0) ? (*pc++ & 31) : (op & 2) ? (vCL & 31) : 1;
        if (count)
          wr16(o, shiftOp<16>((m >> 3) & 7, rd16(o), count));
      } else {
        Opnd o = decodeRM8(m, pc, ovr);
        uint32_t count = (op < 0xd0) ? (*pc++ & 31) : (op & 2) ? (vCL & 31) : 1;
        if (count)
          wr8(o, shiftOp<8>((m >> 3) & 7, rd8(o), count));
      }
      break;
    }

    // RET imm16
    case 0xc2:
    {
      uint16_t n = fetch16(pc);
      g.ip = pop();
      vSP += n;
      return false;
    }

    // RET
    case 0xc3:
      g.ip = pop();
      return false;

    // LES reg16, mem
    // LDS reg16, mem
    case 0xc4:
    case 0xc5:
    {
      uint8_t m = *pc++;
      if (m < 0xc0) {
        uint32_t a = eaLinear(m, pc, ovr);
        REG16((m >> 3) & 7) = rdmem16(a);
        g.s[op == 0xc4 ? ESi : DSi] = rdmem16(a + 2);
      }
      break;
    }

    // MOV r/m8, imm8
    case 0xc6:
    {
      uint8_t m = *pc++;
      Opnd o = decodeRM8(m, pc, ovr);
      wr8(o, *pc++);
      break;
    }

    // MOV r/m16, imm16
    case 0xc7:
    {
      uint8_t m = *pc++;
      Opnd o = decodeRM16(m, pc, ovr);
      wr16(o, fetch16(pc));
      break;
    }

    // ENTER imm16, imm8 (80186)
    case 0xc8:
    {
      uint16_t size  = fetch16(pc);
      int      level = *pc++ & 31;
      push(vBP);
      uint16_t framePtr = vSP;
      if (level > 0) {
        while (--level) {
          vBP -= 2;
          push(*(u16a *)(g.mem + SEGBASE(SSi) + vBP));
        }
        push(framePtr);
      }
      vBP  = framePtr;
      vSP -= size;
      break;
    }

    // LEAVE (80186)
    case 0xc9:
      vSP = vBP;
      vBP = pop();
      break;

    // RETF imm16
    case 0xca:
    {
      uint16_t n = fetch16(pc);
      g.ip = pop();
      vCS  = pop();
      vSP += n;
      return false;
    }

    // RETF
    case 0xcb:
      g.ip = pop();
      vCS  = pop();
      return false;

    // INT 3
    case 0xcc:
      g.ip = ip0 + (uint16_t)(pc - start);
      pcInterrupt(3);
      return false;

    // INT imm8
    case 0xcd:
    {
      uint8_t n = *pc++;
      g.ip = ip0 + (uint16_t)(pc - start);
      pcInterrupt(n);
      return false;
    }

    // INTO
    case 0xce:
      g.ip = ip0 + (uint16_t)(pc - start);
      if (g.OF)
        pcInterrupt(4);
      return false;

    // IRET
    case 0xcf:
      g.ip = pop();
      vCS  = pop();
      setFlags(pop());
      return false;

    // AAM imm8
    case 0xd4:
    {
      uint8_t d = *pc++;
      if (d == 0) {
        // divide error
        g.ip = ip0;
        pcInterrupt(0);
        return false;
      }
      vAH = vAL / d;
      vAL = vAL % d;
      opLOG<8>(vAL);
      break;
    }

    // AAD imm8
    case 0xd5:
      vAX = (vAL + *pc++ * vAH) & 0xff;
      opLOG<8>(vAL);
      break;

    // SALC (undocumented)
    case 0xd6:
      vAL = g.CF ? 0xff : 0x00;
      break;

    // XLAT
    case 0xd7:
      vAL = rdmem8(SEGBASE(ovr >= 0 ? ovr : DSi) + (uint16_t)(vBX + vAL));
      break;

    // ESC (8087 math coprocessor) - not implemented
    case 0xd8 ... 0xdf:
    {
      uint8_t const * opc = pc - 1;
      uint8_t m = *pc++;
      if (m < 0xc0) {
        int seg;
        eaOffset(m, pc, seg);
      }
      printf("8087 MATH Coprocessor %02X %02X %02X %02X - not implemented!\n", opc[0], opc[1], opc[2], opc[3]);
      break;
    }

    // LOOPNZ, LOOPZ, LOOP
    case 0xe0 ... 0xe2:
    {
      int8_t d = *pc++;
      uint16_t nextIP = ip0 + (uint16_t)(pc - start);
      if (--vCX && (op == 0xe2 || g.ZF == (op & 1)))
        nextIP += d;
      g.ip = nextIP;
      return false;
    }

    // JCXZ
    case 0xe3:
    {
      int8_t d = *pc++;
      g.ip = ip0 + (uint16_t)(pc - start) + (vCX == 0 ? d : 0);
      return false;
    }

    // IN AL, imm8
    case 0xe4:
      vAL = g.readPort(g.context, *pc++);
      break;

    // IN AX, imm8
    case 0xe5:
    {
      uint8_t port = *pc++;
      vAL = g.readPort(g.context, port);
      vAH = g.readPort(g.context, port + 1);
      break;
    }

    // OUT imm8, AL
    case 0xe6:
      g.writePort(g.context, *pc++, vAL);
      break;

    // OUT imm8, AX
    case 0xe7:
    {
      uint8_t port = *pc++;
      g.writePort(g.context, port, vAL);
      g.writePort(g.context, port + 1, vAH);
      break;
    }

    // CALL rel16
    case 0xe8:
    {
      uint16_t d = fetch16(pc);
      uint16_t nextIP = ip0 + (uint16_t)(pc - start);
      push(nextIP);
      g.ip = nextIP + d;
      return false;
    }

    // JMP rel16
    case 0xe9:
    {
      uint16_t d = fetch16(pc);
      g.ip = ip0 + (uint16_t)(pc - start) + d;
      return false;
    }

    // JMP far imm16:imm16
    case 0xea:
    {
      uint16_t newIP = fetch16(pc);
      vCS  = fetch16(pc);
      g.ip = newIP;
      return false;
    }

    // JMP rel8
    case 0xeb:
    {
      int8_t d = *pc++;
      g.ip = ip0 + (uint16_t)(pc - start) + d;
      return false;
    }

    // IN AL, DX
    case 0xec:
      vAL = g.readPort(g.context, vDX);
      break;

    // IN AX, DX
    case 0xed:
      vAL = g.readPort(g.context, vDX);
      vAH = g.readPort(g.context, vDX + 1);
      break;

    // OUT DX, AL
    case 0xee:
      g.writePort(g.context, vDX, vAL);
      break;

    // OUT DX, AX
    case 0xef:
      g.writePort(g.context, vDX, vAL);
      g.writePort(g.context, vDX + 1, vAH);
      break;

    // LOCK (0xf1 is an undocumented alias)
    case 0xf0:
    case 0xf1:
      goto fetch;

    // REPNE/REPNZ
    case 0xf2:
      rep = 1;
      goto fetch;

    // REP/REPE/REPZ
    case 0xf3:
      rep = 2;
      goto fetch;

    // HLT
    case 0xf4:
      // IP remains on HLT, it is incremented when an interrupt exits from halt state
      g.ip     = ip0 + (uint16_t)(pc - start) - 1;
      g.halted = true;
      return false;

    // CMC
    case 0xf5:
      g.CF ^= 1;
      break;

    // TEST, NOT, NEG, MUL, IMUL, DIV, IDIV  r/m
    case 0xf6:
    case 0xf7:
    {
      uint8_t m  = *pc++;
      int     sub = (m >> 3) & 7;
      if (op & 1) {
        Opnd o = decodeRM16(m, pc, ovr);
        switch (sub) {
          case 0:   // TEST r/m16, imm16
          case 1:   // (undocumented alias)
            opLOG<16>(rd16(o) & fetch16(pc));
            break;
          case 2:   // NOT
            wr16(o, ~rd16(o));
            break;
          case 3:   // NEG
            wr16(o, opSUB<16>(0, rd16(o), 0));
            break;
          default:  // MUL, IMUL, DIV, IDIV
            if (!mulDiv(sub, true, rd16(o))) {
              // divide error, IP points to the instruction (including prefixes)
              g.ip = ip0;
              pcInterrupt(0);
              return false;
            }
            break;
        }
      } else {
        Opnd o = decodeRM8(m, pc, ovr);
        switch (sub) {
          case 0:   // TEST r/m8, imm8
          case 1:   // (undocumented alias)
            opLOG<8>(rd8(o) & *pc++);
            break;
          case 2:   // NOT
            wr8(o, ~rd8(o));
            break;
          case 3:   // NEG
            wr8(o, opSUB<8>(0, rd8(o), 0));
            break;
          default:  // MUL, IMUL, DIV, IDIV
            if (!mulDiv(sub, false, rd8(o))) {
              // divide error, IP points to the instruction (including prefixes)
              g.ip = ip0;
              pcInterrupt(0);
              return false;
            }
            break;
        }
      }
      break;
    }

    // CLC, STC, CLI, STI, CLD, STD
    case 0xf8:
    case 0xf9:
      g.CF = op & 1;
      reloop = true;  // inhibit interrupts until next instruction (actually should be just for STI...)
      break;
    case 0xfa:
    case 0xfb:
      g.IF = op & 1;
      reloop = true;
      break;
    case 0xfc:
    case 0xfd:
      g.DF = op & 1;
      reloop = true;
      break;

    // INC r/m8, DEC r/m8
    case 0xfe:
    {
      uint8_t m = *pc++;
      Opnd o = decodeRM8(m, pc, ovr);
      switch ((m >> 3) & 7) {
        case 0:
          wr8(o, opINC<8>(rd8(o)));
          break;
        case 1:
          wr8(o, opDEC<8>(rd8(o)));
          break;
      }
      break;
    }

    // INC, DEC, CALL, CALL far, JMP, JMP far, PUSH  r/m16
    case 0xff:
    {
      uint8_t m = *pc++;
      Opnd o = decodeRM16(m, pc, ovr);
      switch ((m >> 3) & 7) {
        case 0: // INC
          wr16(o, opINC<16>(rd16(o)));
          break;
        case 1: // DEC
          wr16(o, opDEC<16>(rd16(o)));
          break;
        case 2: // CALL near
        {
          uint16_t newIP = rd16(o);
          push(ip0 + (uint16_t)(pc - start));
          g.ip = newIP;
          return false;
        }
        case 3: // CALL far
        {
          uint16_t newIP = rd16(o);
          uint16_t newCS = rd16hi(o);
          push(vCS);
          push(ip0 + (uint16_t)(pc - start));
          vCS  = newCS;
          g.ip = newIP;
          return false;
        }
        case 4: // JMP near
          g.ip = rd16(o);
          return false;
        case 5: // JMP far
        {
          uint16_t newIP = rd16(o);
          vCS  = rd16hi(o);
          g.ip = newIP;
          return false;
        }
        default: // PUSH (SP decremented before reading operand, as PUSH SP)
        {
          vSP -= 2;
          uint16_t v = rd16(o);
          *(u16a *)(g.mem + SEGBASE(SSi) + vSP) = v;
          break;
        }
      }
      break;
    }

    // POP CS (0x0f) and 80286+ opcodes (0x63..0x67)
    default:
      printf("Unsupported 8086 opcode %02X %02X\n", op, *pc);
      if (op == 0x0f)
        ++pc;
      break;

  }

  goto done;

aluExec:
  if (aluW) {
    uint32_t r = alu<16>(aluOp, aluA, aluB);
    if (aluOp != 7)
      wr16(aluDst, r);
  } else {
    uint32_t r = alu<8>(aluOp, aluA, aluB);
    if (aluOp != 7)
      wr8(aluDst, r);
  }

done:
  g.ip = ip0 + (uint16_t)(pc - start);
  return reloop;
}


void i8086::step()
{
  // Interrupts handling
  // Following instructions inhibits interrupts for the next instruction:
  //    MOV SS, xxx     [opcode 0x8e]
  //    POP SS          [opcode 0x17]
  //    STI             [opcode 0xfb]
  // They are handled by execInstruction() returning true, so the next instruction is executed in the same step.

  // Application has set trap flag, so fire INT 1
  if (g.TF) {
    pcInterrupt(1);
  }
  // Check for interrupts triggered by system interfaces
  else if (g.IF && g.pendingIRQ) {
    pcInterrupt(g.pendingIRQIndex);
    g.pendingIRQ = false;
  }

  if (g.halted)
    return;

  PSRAM_WORKAROUND2

  while (execInstruction())
    ;
}


}   // namespace fabgl
