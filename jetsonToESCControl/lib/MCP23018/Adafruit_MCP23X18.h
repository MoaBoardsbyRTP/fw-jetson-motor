/*!
 * @file Adafruit_MCP23X18.h
 * 
 * This is a library for the MCP23018 I2C port expander.
 * Based on Adafruit MCP23017 Library by Carter Nelson.
 * 
 * The MCP23018 is similar to MCP23017 but has open-drain outputs
 * and only supports I2C (no SPI variant).
 */

#ifndef __ADAFRUIT_MCP23X18_H__
#define __ADAFRUIT_MCP23X18_H__

#include <Adafruit_MCP23XXX.h>

/**************************************************************************/
/*!
    @brief  Class for MCP23018 I2C port expander.
    
    The MCP23018 is a 16-bit I/O expander with open-drain outputs.
    It shares the same register map as MCP23017 (BANK=0 mode).
    Unlike MCP23017/MCP23S17, there is no SPI variant.
*/
/**************************************************************************/
class Adafruit_MCP23X18 : public Adafruit_MCP23XXX {
public:
  Adafruit_MCP23X18();

  // I2C initialization (MCP23018 is I2C only, no SPI)
  bool begin_I2C(uint8_t i2c_addr = MCP23XXX_ADDR, TwoWire *wire = &Wire);

  // Port A access
  uint8_t readGPIOA();
  void writeGPIOA(uint8_t value);

  // Port B access
  uint8_t readGPIOB();
  void writeGPIOB(uint8_t value);

  // Combined 16-bit access (A = low byte, B = high byte)
  uint16_t readGPIOAB();
  void writeGPIOAB(uint16_t value);

  // Enable hardware address pins (HAEN bit in IOCON)
  void enableAddrPins();
};

#endif
