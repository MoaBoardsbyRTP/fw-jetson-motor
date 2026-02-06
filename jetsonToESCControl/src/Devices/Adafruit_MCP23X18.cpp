/*!
 * @file Adafruit_MCP23X18.cpp
 *
 * This is a library for the MCP23018 I2C port expander.
 * Based on Adafruit MCP23017 Library by Carter Nelson.
 *
 * The MCP23018 is similar to MCP23017 but has open-drain outputs
 * and only supports I2C (no SPI variant).
 */

#include "Adafruit_MCP23X18.h"

/**************************************************************************/
/*!
    @brief  Constructor for MCP23018.
*/
/**************************************************************************/
Adafruit_MCP23X18::Adafruit_MCP23X18() { pinCount = 16; }

/**************************************************************************/
/*!
    @brief  Initialize MCP23018 using I2C.
    @param  i2c_addr I2C address (default 0x20)
    @param  wire Pointer to Wire instance
    @return true if initialization successful, otherwise false.
*/
/**************************************************************************/
bool Adafruit_MCP23X18::begin_I2C(uint8_t i2c_addr, TwoWire *wire) {
  // Call parent class begin_I2C
  return Adafruit_MCP23XXX::begin_I2C(i2c_addr, wire);
}

/**************************************************************************/
/*!
    @brief  Read all pins on Port A.
    @return Current pin states of Port A as uint8_t.
*/
/**************************************************************************/
uint8_t Adafruit_MCP23X18::readGPIOA() { return readGPIO(0); }

/**************************************************************************/
/*!
    @brief  Write to all pins on Port A.
    @param  value Pin states to write as uint8_t.
*/
/**************************************************************************/
void Adafruit_MCP23X18::writeGPIOA(uint8_t value) { writeGPIO(value, 0); }

/**************************************************************************/
/*!
    @brief  Read all pins on Port B.
    @return Current pin states of Port B as uint8_t.
*/
/**************************************************************************/
uint8_t Adafruit_MCP23X18::readGPIOB() { return readGPIO(1); }

/**************************************************************************/
/*!
    @brief  Write to all pins on Port B.
    @param  value Pin states to write as uint8_t.
*/
/**************************************************************************/
void Adafruit_MCP23X18::writeGPIOB(uint8_t value) { writeGPIO(value, 1); }

/**************************************************************************/
/*!
    @brief  Read all 16 pins (Port A and B) at once.
    @return Current pin states as uint16_t (A = low byte, B = high byte).
*/
/**************************************************************************/
uint16_t Adafruit_MCP23X18::readGPIOAB() {
  return (uint16_t)readGPIOA() | ((uint16_t)readGPIOB() << 8);
}

/**************************************************************************/
/*!
    @brief  Write to all 16 pins (Port A and B) at once.
    @param  value Pin states as uint16_t (A = low byte, B = high byte).
*/
/**************************************************************************/
void Adafruit_MCP23X18::writeGPIOAB(uint16_t value) {
  writeGPIOA(value & 0xFF);
  writeGPIOB((value >> 8) & 0xFF);
}

/**************************************************************************/
/*!
    @brief  Read Port A interrupt capture register (INTCAPA).
    
    Returns the GPIO state captured at the moment the interrupt occurred.
    Reading this register clears the interrupt condition on the MCP23018.
    
    @return Captured Port A pin states as uint8_t.
*/
/**************************************************************************/
uint8_t Adafruit_MCP23X18::readIntCapA() {
  return (uint8_t)getRegister(MCP23XXX_INTCAP, 0);
}

/**************************************************************************/
/*!
    @brief  Read Port B interrupt capture register (INTCAPB).
    
    Returns the GPIO state captured at the moment the interrupt occurred.
    Reading this register clears the interrupt condition on the MCP23018.
    
    @return Captured Port B pin states as uint8_t.
*/
/**************************************************************************/
uint8_t Adafruit_MCP23X18::readIntCapB() {
  return (uint8_t)getRegister(MCP23XXX_INTCAP, 1);
}

/**************************************************************************/
/*!
    @brief  Enable hardware address pins (A2, A1, A0).
    
    Sets the HAEN (Hardware Address Enable) bit in IOCON register.
    This allows multiple MCP23018 devices on the same I2C bus
    with different addresses based on A2/A1/A0 pin states.
    
    @note On MCP23018, HAEN is typically enabled by default, but this
          method ensures it is set for compatibility.
*/
/**************************************************************************/
void Adafruit_MCP23X18::enableAddrPins() {
  Adafruit_BusIO_Register IOCON(i2c_dev, spi_dev, MCP23XXX_SPIREG,
                                getRegister(MCP23XXX_IOCON, 0));
  Adafruit_BusIO_RegisterBits haen_bit(&IOCON, 1, 3);
  haen_bit.write(1);
}
