/*!
 * @file Adafruit_MCP23X18.h
 * 
 * This is a library for the MCP23018 I2C port expander.
 * Based on Adafruit MCP23017 Library by Carter Nelson.
 * 
 * The MCP23018 is similar to MCP23017 but has open-drain outputs
 * and only supports I2C (no SPI variant).
 */

#pragma once

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

  // Override: MCP23018 allows pullup independent of direction
  void pinMode(uint8_t pin, uint8_t mode);

  // Independent pullup control (works on both input and output pins)
  void setPullup(uint8_t pin, bool enabled);

  // Port A configuration
  void configGPIOA(uint8_t dir, uint8_t pullup);

  // Port B configuration
  void configGPIOB(uint8_t dir, uint8_t pullup);

  // Port A access
  uint8_t readGPIOA();
  void writeGPIOA(uint8_t value);

  // Port B access
  uint8_t readGPIOB();
  void writeGPIOB(uint8_t value);

  // Combined 16-bit access (A = low byte, B = high byte)
  uint16_t readGPIOAB();
  void writeGPIOAB(uint16_t value);

  // Interrupt capture registers (captures GPIO state at interrupt time)
  uint8_t readIntCapA();
  uint8_t readIntCapB();

  // Enable hardware address pins (HAEN bit in IOCON)
  void enableAddrPins();
};
