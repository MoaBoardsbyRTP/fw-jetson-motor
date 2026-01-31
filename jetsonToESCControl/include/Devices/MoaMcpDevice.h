/**
 * @file MoaMcpDevice.h
 * @brief Thread-safe MCP23018 I2C port expander wrapper
 * @author Oscar Martinez
 * @date 2025-01-30
 * 
 * This library provides a thread-safe wrapper around the Adafruit_MCP23X18 class
 * for shared access between MoaButtonControl and MoaLedControl classes.
 * Uses a FreeRTOS mutex to protect I2C transactions.
 */

#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "Adafruit_MCP23X18.h"

/**
 * @brief Default I2C address for MCP23018
 */
#define MOA_MCP_DEFAULT_ADDR 0x20

/**
 * @brief Default mutex timeout in milliseconds
 */
#define MOA_MCP_MUTEX_TIMEOUT_MS 100

/**
 * @brief Thread-safe MCP23018 wrapper for shared I2C access
 * 
 * MoaMcpDevice wraps the Adafruit_MCP23X18 class and provides mutex-protected
 * access to the I2C port expander. This allows multiple classes (e.g., 
 * MoaButtonControl and MoaLedControl) to safely share the same MCP23018 device.
 * 
 * ## Usage Example
 * @code
 * MoaMcpDevice mcpDevice(0x20);
 * mcpDevice.begin();
 * 
 * // Both classes share the same device safely
 * MoaButtonControl buttons(eventQueue, mcpDevice, GPIO_NUM_2);
 * MoaLedControl leds(mcpDevice);
 * @endcode
 */
class MoaMcpDevice {
public:
    /**
     * @brief Construct a new MoaMcpDevice object
     * 
     * @param i2cAddr I2C address of the MCP23018 (default: 0x20)
     */
    MoaMcpDevice(uint8_t i2cAddr = MOA_MCP_DEFAULT_ADDR);

    /**
     * @brief Destructor - deletes the mutex
     */
    ~MoaMcpDevice();

    /**
     * @brief Initialize the MCP23018 device
     * 
     * @param wire Pointer to TwoWire instance (default: &Wire)
     * @return true if initialization successful
     * @return false if device not found or I2C error
     */
    bool begin(TwoWire* wire = &Wire);

    /**
     * @brief Check if the device is initialized
     * @return true if begin() was successful
     */
    bool isInitialized() const;

    /**
     * @brief Get the mutex handle for manual locking
     * 
     * Use this for complex operations requiring multiple I2C transactions.
     * 
     * @return SemaphoreHandle_t Mutex handle
     */
    SemaphoreHandle_t getMutex();

    /**
     * @brief Get direct access to the underlying MCP23018 object
     * 
     * @warning Caller must hold the mutex when using this!
     * @return Adafruit_MCP23X18& Reference to the MCP23018 object
     */
    Adafruit_MCP23X18& getMcp();

    /**
     * @brief Set the mutex timeout for I2C operations
     * 
     * @param timeoutMs Timeout in milliseconds
     */
    void setMutexTimeout(uint32_t timeoutMs);

    /**
     * @brief Get the current mutex timeout
     * @return uint32_t Timeout in milliseconds
     */
    uint32_t getMutexTimeout() const;

    // === Thread-safe Port A operations (typically buttons/inputs) ===

    /**
     * @brief Read Port A GPIO state (thread-safe)
     * @return uint8_t Port A state, or 0 if mutex timeout
     */
    uint8_t readPortA();

    /**
     * @brief Configure Port A pin modes (thread-safe)
     * 
     * @param mask Bitmask of pins to configure
     * @param mode Pin mode (INPUT, INPUT_PULLUP, OUTPUT)
     */
    void configurePortA(uint8_t mask, uint8_t mode);

    /**
     * @brief Enable interrupt-on-change for Port A pins (thread-safe)
     * 
     * @param mask Bitmask of pins to enable interrupts
     * @param defaultValue Default comparison value for interrupt
     * @param mode Interrupt mode: CHANGE, FALLING, RISING
     */
    void enableInterruptPortA(uint8_t mask, uint8_t defaultValue = 0x00);

    // === Thread-safe Port B operations (typically LEDs/outputs) ===

    /**
     * @brief Read Port B GPIO state (thread-safe)
     * @return uint8_t Port B state, or 0 if mutex timeout
     */
    uint8_t readPortB();

    /**
     * @brief Write Port B GPIO state (thread-safe)
     * @param value Value to write to Port B
     */
    void writePortB(uint8_t value);

    /**
     * @brief Configure Port B pin modes (thread-safe)
     * 
     * @param mask Bitmask of pins to configure
     * @param mode Pin mode (INPUT, INPUT_PULLUP, OUTPUT)
     */
    void configurePortB(uint8_t mask, uint8_t mode);

    // === General pin operations (thread-safe) ===

    /**
     * @brief Set a single pin mode (thread-safe)
     * 
     * @param pin Pin number (0-15, where 0-7 = Port A, 8-15 = Port B)
     * @param mode Pin mode (INPUT, INPUT_PULLUP, OUTPUT)
     */
    void setPinMode(uint8_t pin, uint8_t mode);

    /**
     * @brief Write to a single pin (thread-safe)
     * 
     * @param pin Pin number (0-15)
     * @param value HIGH or LOW
     */
    void writePin(uint8_t pin, bool value);

    /**
     * @brief Read a single pin (thread-safe)
     * 
     * @param pin Pin number (0-15)
     * @return bool Pin state
     */
    bool readPin(uint8_t pin);

private:
    Adafruit_MCP23X18 _mcp;            ///< Underlying MCP23018 instance
    SemaphoreHandle_t _mutex;          ///< Mutex for thread-safe access
    uint8_t _i2cAddr;                  ///< I2C address
    uint32_t _mutexTimeoutMs;          ///< Mutex timeout in milliseconds
    bool _initialized;                 ///< Initialization flag

    /**
     * @brief Acquire the mutex with timeout
     * @return true if mutex acquired
     * @return false if timeout
     */
    bool acquireMutex();

    /**
     * @brief Release the mutex
     */
    void releaseMutex();
};
