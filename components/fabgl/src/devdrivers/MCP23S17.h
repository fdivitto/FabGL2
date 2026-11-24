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



#include "driver/spi_master.h"

#include "fabutils.h"



/**
 * @file
 *
 * @brief This file contains the MCP23S17 driver class
 *
 */



namespace fabgl {



#define MCP_SPI_FREQ  10000000   // it seems to work up to 23000000!! (but datasheet specifies 10000000)
#define MCP_DMACHANNEL       SPI_DMA_CH_AUTO


#define MCP_MAXDEVICES       2


#define MCP_PORTA            0
#define MCP_PORTB            1


#define MCP_A0               0
#define MCP_A1               1
#define MCP_A2               2
#define MCP_A3               3
#define MCP_A4               4
#define MCP_A5               5
#define MCP_A6               6
#define MCP_A7               7

#define MCP_B0               8
#define MCP_B1               9
#define MCP_B2              10
#define MCP_B3              11
#define MCP_B4              12
#define MCP_B5              13
#define MCP_B6              14
#define MCP_B7              15


// bank 0 registers (A = reg + 0, B = reg + 1)
#define MCP_IODIR         0x00
#define MCP_IPOL          0x02
#define MCP_GPINTEN       0x04
#define MCP_DEFVAL        0x06
#define MCP_INTCON        0x08
#define MCP_IOCON         0x0A
#define MCP_GPPU          0x0C
#define MCP_INTF          0x0E
#define MCP_INTCAP        0x10
#define MCP_GPIO          0x12
#define MCP_OLAT          0x14


// bank 1 registers (A = reg + 0, B = reg + 0x10)
#define MCP_BNK1_IODIR    0x00
#define MCP_BNK1_IPOL     0x01
#define MCP_BNK1_GPINTEN  0x02
#define MCP_BNK1_DEFVAL   0x03
#define MCP_BNK1_INTCON   0x04
#define MCP_BNK1_IOCON    0x05
#define MCP_BNK1_GPPU     0x06
#define MCP_BNK1_INTF     0x07
#define MCP_BNK1_INTCAP   0x08
#define MCP_BNK1_GPIO     0x09
#define MCP_BNK1_OLAT     0x0A


// IOCON bits
#define MCP_IOCON_BANK    0x80   // Controls how the registers are addressed (0 = bank0)
#define MCP_IOCON_MIRROR  0x40   // INT Pins Mirror bit (1 = mirrored)
#define MCP_IOCON_SEQOP   0x20   // Sequential Operation mode bit (1 = not increment)
#define MCP_IOCON_DISSLW  0x10   // Slew Rate control bit for SDA output (I2C only)
#define MCP_IOCON_HAEN    0x08   // Hardware Address Enable bit
#define MCP_IOCON_ODR     0x04   // Configures the INT pin as an open-drain output (1 = open-drain)
#define MCP_IOCON_INTPOL  0x02   // This bit sets the polarity of the INT output pin (1 = active-high)


#define MCP_GPIO2REG(basereg, gpio) ((basereg) + ((gpio) >> 3))
#define MCP_GPIO2MASK(gpio)         (1 << ((gpio) & 7))


/** \ingroup Enumerations
 * @brief GPIO direction
 */
enum class MCPDir {
  Input,    /**< GPIO is input */
  Output    /**< GPIO is output */
};


/** \ingroup Enumerations
 * @brief Interrupt trigger mode
 */
enum class MCPIntTrigger {
  DefaultChange,    /**< Trigger an interrupt when the GPIO differs from its default value */
  PreviousChange    /**< Trigger an interrupt when the GPIO changes */
};


/**
 * @brief MCP23S17 driver
 *
 * This driver supports multiple devices attached to the same bus (sharing the same CS), selected by their hardware address.
 *
 * Example:
 *
 *     MCP23S17 io;
 *     io.begin(35, 12, 14, 13);                        // MISO = 35, MOSI = 12, CLK = 14, CS = 13
 *     io.configureGPIO(MCP_B0, MCPDir::Output);        // B0 is an output
 *     io.configureGPIO(MCP_A1, MCPDir::Input, true);   // A1 is an input with pull-up
 *     bool A1 = io.readGPIO(MCP_A1);                   // read A1
 *     io.writeGPIO(MCP_B0, true);                      // set B0 high
 *
 * @ingroup devices
 */
class MCP23S17 {

public:

  MCP23S17();
  ~MCP23S17();


  //// initialization


  /**
   * @brief Initializes the MCP23S17 driver
   *
   * @param MISO MISO pin (default -1 = board dependent)
   * @param MOSI MOSI pin (default -1 = board dependent)
   * @param CLK CLK pin (default -1 = board dependent)
   * @param CS CS pin (default -1 = board dependent)
   * @param CSActiveState CS active state (default -1 = board dependent)
   * @param host SPI host
   */
  bool begin(int MISO = -1, int MOSI = -1, int CLK = -1, int CS = -1, int CSActiveState = -1, int host = SPI2_HOST);

  /**
   * @brief Deinitializes the MCP23S17 driver
   */
  void end();

  /**
   * @brief Determines whether the MCP23S17 is available
   *
   * @return True if the MCP23S17 is available and correctly initialized
   */
  bool available()     { return m_SPIDevHandle != nullptr; }

  /**
   * @brief Initializes additional MCP23S17 devices connected to the same SPI bus but with a different hardware address
   *
   * There is no need to call initDevice() when only one MCP23S17 device is present.
   *
   * @param hwAddr Hardware address of the additional device
   *
   * @return True if the MCP23S17 is available and correctly initialized
   *
   * Example:
   *
   *     MCP23S17 io;
   *     io.begin(35, 12, 14, 13);                            // MISO = 35, MOSI = 12, CLK = 14, CS = 13
   *     io.initDevice(0);                                    // initialize the device with hardware address 0
   *     io.initDevice(1);                                    // initialize the device with hardware address 1
   *     io.configureGPIO(MCP_A0, MCPDir::Output, false, 0);  // set A0 of device 0 as output
   *     io.configureGPIO(MCP_A0, MCPDir::Input, false, 1);   // set A0 of device 1 as input
   */
  bool initDevice(uint8_t hwAddr);


  //// registers read/write ////


  /**
   * @brief Writes an 8-bit value to an internal register
   *
   * By default this driver uses BANK 0 registers.
   *
   * @param addr Register address (bank 0)
   * @param value 8-bit value to write
   * @param hwAddr Optional hardware device address
   *
   * Example:
   *
   *     // configure PORT A as all inputs...
   *     io.writeReg(MCP_IODIR, 0);
   *     // ...which is the same as
   *     io.setPortDir(MCP_PORTA, 0);
   */
  void writeReg(uint8_t addr, uint8_t value, uint8_t hwAddr = 0);

  /**
   * @brief Reads an 8-bit value from an internal register
   *
   * @param addr Register address (bank 0)
   * @param hwAddr Optional hardware device address
   *
   * @return The 8-bit value read
   */
  uint8_t readReg(uint8_t addr, uint8_t hwAddr = 0);

  /**
   * @brief Writes a 16-bit value to two consecutive registers
   *
   * @param addr Address of the first register to write (bank 0)
   * @param value 16-bit value to write
   * @param hwAddr Optional hardware device address
   */
  void writeReg16(uint8_t addr, uint16_t value, uint8_t hwAddr = 0);

  /**
   * @brief Reads a 16-bit value from two consecutive registers
   *
   * @param addr Address of the first register to read (bank 0)
   * @param hwAddr Optional hardware device address
   *
   * @return The 16-bit value read
   */
  uint16_t readReg16(uint8_t addr, uint8_t hwAddr = 0);


  //// configuration ////


  /**
   * @brief Enables or disables mirroring of the INT pins
   *
   * @param value If true, the INT pins are mirrored
   * @param hwAddr Optional hardware device address
   */
  void enableINTMirroring(bool value, uint8_t hwAddr = 0);

  /**
   * @brief Enables or disables open-drain mode of the INT pins
   *
   * @param value If true, the INT pins are configured as open-drain outputs
   * @param hwAddr Optional hardware device address
   */
  void enableINTOpenDrain(bool value, uint8_t hwAddr = 0);

  /**
   * @brief Sets the polarity of the INT pins
   *
   * @param value If true, INT is driven high on interrupt
   * @param hwAddr Optional hardware device address
   */
  void setINTActiveHigh(bool value, uint8_t hwAddr = 0);

  /**
   * @brief Gets the polarity of the INT pins
   *
   * @param hwAddr Optional hardware device address
   *
   * @return True if INT is driven high on interrupt
   */
  bool getINTActiveHigh(uint8_t hwAddr = 0)                              { return readReg(MCP_IOCON, hwAddr) & MCP_IOCON_INTPOL; }


  //// port setup, read, write (8 bit granularity) ////


  /**
   * @brief Sets the port direction
   *
   * @param port Port to configure (MCP_PORTA or MCP_PORTB)
   * @param value Pin direction mask (1 = input, 0 = output)
   * @param hwAddr Optional hardware device address
   *
   * Example:
   *
   *     // A0 and A7 are outputs, A1..A6 are inputs
   *     io.setPortDir(MCP_PORTA, 0b01111110);
   */
  void setPortDir(int port, uint8_t value, uint8_t hwAddr = 0)           { writeReg(MCP_IODIR + port, value, hwAddr); }

  /**
   * @brief Gets the port direction
   *
   * @param port Port to query (MCP_PORTA or MCP_PORTB)
   * @param hwAddr Optional hardware device address
   *
   * @return Port direction mask (1 = input, 0 = output)
   */
  uint8_t getPortDir(int port, uint8_t hwAddr = 0)                       { return readReg(MCP_IODIR + port, hwAddr); }

  /**
   * @brief Sets the input polarity
   *
   * @param port Port to configure (MCP_PORTA or MCP_PORTB)
   * @param value Polarity mask (1 = the register bit reflects the inverted logic state of the input pin)
   * @param hwAddr Optional hardware device address
   */
  void setPortInputPolarity(int port, uint8_t value, uint8_t hwAddr = 0) { writeReg(MCP_IPOL + port, value, hwAddr); }

  /**
   * @brief Enables or disables the port pull-ups
   *
   * @param port Port to configure (MCP_PORTA or MCP_PORTB)
   * @param value Pull-ups mask (1 = pull-up enabled)
   * @param hwAddr Optional hardware device address
   *
   * Example:
   *
   *     // enable pull-up for pin A7
   *     io.enablePortPullUp(MCP_PORTA, 0b10000000);
   */
  void enablePortPullUp(int port, uint8_t value, uint8_t hwAddr = 0)     { writeReg(MCP_GPPU + port, value, hwAddr); }

  /**
   * @brief Gets the port pull-ups
   *
   * @param port Port to query (MCP_PORTA or MCP_PORTB)
   * @param hwAddr Optional hardware device address
   *
   * @return Pull-ups mask (1 = pull-up enabled)
   */
  uint8_t getPortPullUp(int port, uint8_t hwAddr = 0)                    { return readReg(MCP_GPPU + port, hwAddr); }

  /**
   * @brief Sets the state of the output pins of the specified port
   *
   * @param port Port to write (MCP_PORTA or MCP_PORTB)
   * @param value Output pin state mask (1 = high, 0 = low)
   * @param hwAddr Optional hardware device address
   *
   * Example:
   *
   *     // set A2 and A3 to high
   *     io.writePort(MCP_PORTA, 0b00001100);
   */
  void writePort(int port, uint8_t value, uint8_t hwAddr = 0)            { writeReg(MCP_OLAT + port, value, hwAddr); }

  /**
   * @brief Gets the state of the input pins of the specified port
   *
   * @param port Port to read (MCP_PORTA or MCP_PORTB)
   * @param hwAddr Optional hardware device address
   *
   * @return Input pin state mask (1 = high, 0 = low)
   *
   * Example:
   *
   *     // read input pins of port A
   *     uint8_t portA = io.readPort(MCP_PORTA);
   */
  uint8_t readPort(int port, uint8_t hwAddr = 0)                         { return readReg(MCP_GPIO + port, hwAddr); }

  /**
   * @brief Sets the state of the output pins of ports A and B combined
   *
   * @param value Output pin state mask. The low 8 bits are assigned to port A, the high 8 bits to port B
   * @param hwAddr Optional hardware device address
   *
   * Example:
   *
   *     // set B7 and A0 to high
   *     io.writePort16(0x8001);
   */
  void writePort16(uint16_t value, uint8_t hwAddr = 0)                   { writeReg16(MCP_OLAT, value, hwAddr); }

  /**
   * @brief Gets the state of the input pins of ports A and B combined
   *
   * @param hwAddr Optional hardware device address
   *
   * @return Input pin state mask (1 = high, 0 = low)
   *
   * Example:
   *
   *     // read input pins of ports A and B
   *     uint16_t portAB = io.readPort16();
   */
  uint16_t readPort16(uint8_t hwAddr = 0)                                { return readReg16(MCP_GPIO, hwAddr); }


  //// GPIO setup, read, write (1 bit granularity) ////


  /**
   * @brief Configures the direction and pull-up of a pin
   *
   * @param gpio Pin to configure (MCP_A0...MCP_A7 and MCP_B0...MCP_B7)
   * @param dir Direction (MCPDir::Input or MCPDir::Output)
   * @param pullup If true, the pull-up resistor is enabled
   * @param hwAddr Optional hardware device address
   *
   * Example:
   *
   *     io.configureGPIO(MCP_B0, MCPDir::Output);        // B0 is an output
   *     io.configureGPIO(MCP_A1, MCPDir::Input, true);   // A1 is an input with pull-up
   */
  void configureGPIO(int gpio, MCPDir dir, bool pullup = false, uint8_t hwAddr = 0);

  /**
   * @brief Sets the output state of a pin
   *
   * @param gpio Pin to write (MCP_A0...MCP_A7 and MCP_B0...MCP_B7)
   * @param value Value to set (false = low, true = high)
   * @param hwAddr Optional hardware device address
   *
   * Example:
   *
   *     io.writeGPIO(MCP_A1, true);  // A1 = high
   */
  void writeGPIO(int gpio, bool value, uint8_t hwAddr = 0);

  /**
   * @brief Reads the input state of a pin
   *
   * @param gpio Pin to read (MCP_A0...MCP_A7 and MCP_B0...MCP_B7)
   * @param hwAddr Optional hardware device address
   *
   * @return Input value (false = low, true = high)
   *
   * Example:
   *
   *     bool A3 = io.readGPIO(MCP_A3);
   */
  bool readGPIO(int gpio, uint8_t hwAddr = 0);


  //// interrupt setup (1 bit granularity) ////


  /**
   * @brief Enables the interrupt on the specified pin
   *
   * Interrupt flags are cleared by reading the GPIO or by calling getPortIntCaptured().
   *
   * @param gpio Input pin to use as interrupt source (MCP_A0...MCP_A7 and MCP_B0...MCP_B7)
   * @param trigger Interrupt trigger type. MCPIntTrigger::DefaultChange triggers an interrupt when the pin differs from defaultValue; MCPIntTrigger::PreviousChange triggers an interrupt whenever the pin changes.
   * @param defaultValue Default value used when trigger is MCPIntTrigger::DefaultChange (false = low, true = high)
   * @param hwAddr Optional hardware device address
   *
   * Example:
   *
   *     // INTA and INTB are driven high on interrupt
   *     io.setINTActiveHigh(true);
   *
   *     // INTB is high whenever B1 changes its value
   *     io.enableInterrupt(MCP_B1, MCPIntTrigger::PreviousChange);
   *
   *     // INTA is high whenever A2 is high
   *     io.enableInterrupt(MCP_A2, MCPIntTrigger::DefaultChange, false);
   */
  void enableInterrupt(int gpio, MCPIntTrigger trigger, bool defaultValue = false, uint8_t hwAddr = 0);

  /**
   * @brief Disables any interrupt on the specified pin
   *
   * @param gpio Input pin whose interrupt is disabled (MCP_A0...MCP_A7 and MCP_B0...MCP_B7)
   * @param hwAddr Optional hardware device address
   *
   * Example:
   *
   *     io.disableInterrupt(MCP_A2);
   */
  void disableInterrupt(int gpio, uint8_t hwAddr = 0);


  //// interrupt flags (8 bit granularity) ////


  /**
   * @brief Reads the interrupt flags of the specified port
   *
   * Interrupt flags are cleared by reading the GPIO or by calling getPortIntCaptured().
   *
   * @param port Port to query (MCP_PORTA or MCP_PORTB)
   * @param hwAddr Optional hardware device address
   */
  uint8_t getPortIntFlags(int port, uint8_t hwAddr = 0)                  { return readReg(MCP_INTF + port, hwAddr); }

  /**
   * @brief Reads the input port state captured when the last interrupt was triggered
   *
   * Calling this function clears the interrupt flags.
   *
   * @param port Port to read (MCP_PORTA or MCP_PORTB)
   * @param hwAddr Optional hardware device address
   */
  uint8_t getPortIntCaptured(int port, uint8_t hwAddr = 0)               { return readReg(MCP_INTCAP + port, hwAddr); }


  //// Port buffer read/write (8 bit granularity) ////


  /**
   * @brief Writes an entire buffer to the specified port at high speed
   *
   * @param port Destination port (MCP_PORTA or MCP_PORTB)
   * @param buffer Pointer to the source buffer
   * @param length Buffer length (max 4092 bytes)
   * @param hwAddr Optional hardware device address
   *
   * Example:
   *
   *     // send the content of "buf" (256 bytes) to port A
   *     io.writePort(MCP_PORTA, buf, 256);
   */
  void writePort(int port, void const * buffer, size_t length, uint8_t hwAddr = 0);

  /**
   * @brief Reads an entire buffer from the specified port at high speed
   *
   * @param port Source port (MCP_PORTA or MCP_PORTB)
   * @param buffer Pointer to the destination buffer
   * @param length Buffer length (max 4092 bytes)
   * @param hwAddr Optional hardware device address
   *
   * Example:
   *
   *     // perform 256 reads from port A
   *     uint8_t buf[256];
   *     io.readPort(MCP_PORTA, buf, 256);
   */
  void readPort(int port, void * buffer, size_t length, uint8_t hwAddr = 0);


private:

  bool SPIBegin(int CSActiveState);
  void SPIEnd();

  gpio_num_t          m_MISO;
  gpio_num_t          m_MOSI;
  gpio_num_t          m_CLK;
  gpio_num_t          m_CS;
  spi_host_device_t   m_SPIHost;

  spi_device_handle_t m_SPIDevHandle;

  // cached registers
  uint8_t             m_IOCON[MCP_MAXDEVICES];
};









} // fabgl namespace

