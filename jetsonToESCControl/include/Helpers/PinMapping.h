/**
 * @file PinMapping.h
 * @brief Centralized pin definitions for Moa ESC Controller
 * @author Oscar Martinez
 * @date 2025-01-30
 * 
 * This file contains all hardware pin mappings for the ESP32-C3 based
 * Moa ESC Controller. Use these defines when initializing hardware classes.
 * 
 * ## Hardware Reference
 * - MCU: ESP32-C3 (DFRobot Beetle)
 * - I/O Expander: MCP23018 (I2C address 0x20)
 * - Temperature Sensor: DS18B20 (OneWire)
 * - Current Sensor: ACS759-200B (Hall effect, analog)
 * - Battery Monitoring: Voltage divider (analog)
 * - ESC Control: PWM output
 */

#pragma once

#include <Arduino.h>

// =============================================================================
// ESP32-C3 GPIO Pin Assignments
// =============================================================================

/**
 * @brief Battery voltage sense (ADC input)
 * Connected via voltage divider for battery monitoring
 */
#define PIN_BATT_LEVEL_SENSE    GPIO_NUM_1

/**
 * @brief MCP23018 interrupt pin for Port A (buttons)
 * Active LOW, directly connected to MCP23018 INTA
 */
#define PIN_I2C_INT_A           GPIO_NUM_2

/**
 * @brief Current sense (ADC input)
 * Connected to ACS759-200B Hall effect sensor output
 */
#define PIN_CURRENT_SENSE       GPIO_NUM_5

/**
 * @brief Temperature sense (OneWire data)
 * Connected to DS18B20 temperature sensor
 */
#define PIN_TEMP_SENSE          GPIO_NUM_6

/**
 * @brief ESC PWM output
 * PWM signal to Electronic Speed Controller
 */
#define PIN_ESC_PWM             GPIO_NUM_7

/**
 * @brief I2C SDA (data line)
 * Shared I2C bus for MCP23018 and other I2C devices
 */
#define PIN_I2C_SDA             GPIO_NUM_8

/**
 * @brief I2C SCL (clock line)
 * Shared I2C bus for MCP23018 and other I2C devices
 */
#define PIN_I2C_SCL             GPIO_NUM_9

/**
 * @brief I2C reset (active LOW)
 * Optional reset line for I2C devices
 */
#define PIN_I2C_RESET           GPIO_NUM_10

/**
 * @brief UART TX (debug/telemetry)
 */
#define PIN_UART_TX             GPIO_NUM_20

/**
 * @brief UART RX (debug/telemetry)
 */
#define PIN_UART_RX             GPIO_NUM_21

// =============================================================================
// MCP23018 I2C Configuration
// =============================================================================

/**
 * @brief MCP23018 I2C address
 * Default address with A0-A2 pins grounded
 */
#define MCP23018_I2C_ADDR       0x20

// =============================================================================
// MCP23018 Port A - Button Inputs (directly from schematic)
// =============================================================================

/**
 * @brief STOP button on MCP23018 Port A
 */
#define MCP_PIN_BUTTON_STOP     1   // GPA1

/**
 * @brief 25% throttle button on MCP23018 Port A
 */
#define MCP_PIN_BUTTON_25       2   // GPA2

/**
 * @brief 50% throttle button on MCP23018 Port A
 */
#define MCP_PIN_BUTTON_50       3   // GPA3

/**
 * @brief 75% throttle button on MCP23018 Port A
 */
#define MCP_PIN_BUTTON_75       4   // GPA4

/**
 * @brief 100% throttle button on MCP23018 Port A
 */
#define MCP_PIN_BUTTON_100      5   // GPA5

/**
 * @brief Bitmask of all button pins on Port A
 */
#define MCP_BUTTON_MASK         0x3E  // Bits 1-5 = 0b00111110

// =============================================================================
// MCP23018 Port B - LED Outputs (directly from schematic)
// =============================================================================

/**
 * @brief Temperature warning LED on MCP23018 Port B
 */
#define MCP_PIN_LED_TEMP        0   // GPB0

/**
 * @brief Battery low LED on MCP23018 Port B
 */
#define MCP_PIN_LED_BATT_LOW    1   // GPB1

/**
 * @brief Battery medium LED on MCP23018 Port B
 */
#define MCP_PIN_LED_BATT_MED    2   // GPB2

/**
 * @brief Battery high LED on MCP23018 Port B
 */
#define MCP_PIN_LED_BATT_HI     3   // GPB3

/**
 * @brief Overcurrent warning LED on MCP23018 Port B
 */
#define MCP_PIN_LED_OVERCURRENT 4   // GPB4

/**
 * @brief Bitmask of all LED pins on Port B
 */
#define MCP_LED_MASK            0x1F  // Bits 0-4 = 0b00011111

