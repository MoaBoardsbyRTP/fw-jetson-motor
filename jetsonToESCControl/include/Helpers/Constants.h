/**
 * @file Constants.h
 * @brief Hardware constants and default configuration values
 * @author Oscar Martinez
 * @date 2025-01-30
 * 
 * This file contains hardware constants, sensor specifications, and default
 * configuration values for the Moa ESC Controller. These are separate from
 * pin mappings to allow easy tuning without modifying hardware definitions.
 */

#pragma once

// =============================================================================
// ADC Configuration
// =============================================================================

/**
 * @brief ADC resolution in bits (ESP32-C3 default)
 */
#define ADC_RESOLUTION_BITS     12

/**
 * @brief ADC reference voltage (V)
 */
#define ADC_REFERENCE_VOLTAGE   3.3f

// =============================================================================
// Battery Monitoring Constants
// =============================================================================

/**
 * @brief Battery voltage divider ratio
 * Vbatt = Vadc * BATT_DIVIDER_RATIO
 * Adjust based on actual resistor values (e.g., 47K/5.1K = 9.215)
 */
#define BATT_DIVIDER_RATIO      9.215f

/**
 * @brief Battery high threshold (V)
 */
#define BATT_THRESHOLD_HIGH     25.0f

/**
 * @brief Battery medium threshold (V)
 */
#define BATT_THRESHOLD_MEDIUM   21.5f

/**
 * @brief Battery low threshold (V)
 */
#define BATT_THRESHOLD_LOW      20.0f

/**
 * @brief Battery hysteresis (V)
 */
#define BATT_HYSTERESIS         0.1f

/**
 * @brief Battery averaging samples
 */
#define BATT_AVERAGING_SAMPLES  10

// =============================================================================
// Current Sensor Constants (ACS759-200B)
// =============================================================================

/**
 * @brief ACS759-200B sensitivity (V/A)
 * 6.6 mV/A = 0.0066 V/A
 */
#define CURRENT_SENSOR_SENSITIVITY  0.0066f

/**
 * @brief ACS759-200B zero current offset voltage (V)
 * VCC/2 at 3.3V supply = 1.65V
 */
#define CURRENT_SENSOR_OFFSET   1.65f

/**
 * @brief Overcurrent threshold (A)
 */
#define CURRENT_THRESHOLD_OVERCURRENT   150.0f

/**
 * @brief Reverse overcurrent threshold (A)
 */
#define CURRENT_THRESHOLD_REVERSE       -150.0f

/**
 * @brief Current hysteresis (A)
 */
#define CURRENT_HYSTERESIS      5.0f

/**
 * @brief Current averaging samples
 */
#define CURRENT_AVERAGING_SAMPLES   10

// =============================================================================
// Temperature Sensor Constants (DS18B20)
// =============================================================================

/**
 * @brief Temperature target threshold (°C)
 */
#define TEMP_THRESHOLD_TARGET   60.0f

/**
 * @brief Temperature hysteresis (°C)
 */
#define TEMP_HYSTERESIS         2.0f

/**
 * @brief Temperature averaging samples
 */
#define TEMP_AVERAGING_SAMPLES  5

// =============================================================================
// Button Configuration
// =============================================================================

/**
 * @brief Button debounce time (ms)
 */
#define BUTTON_DEBOUNCE_MS      50

/**
 * @brief Long-press detection time (ms)
 */
#define BUTTON_LONG_PRESS_MS    5000

// =============================================================================
// LED Configuration
// =============================================================================

/**
 * @brief Default LED blink period (ms)
 */
#define LED_BLINK_PERIOD_MS     500

/**
 * @brief Warning blink period (ms) - fast blink for overcurrent/temperature alerts
 */
#define LED_WARNING_BLINK_MS    250

/**
 * @brief Config mode blink period (ms) - faster for visibility
 */
#define LED_CONFIG_BLINK_MS     300

// =============================================================================
// Flash Logging Configuration
// =============================================================================

/**
 * @brief Flash log flush interval (ms)
 */
#define LOG_FLUSH_INTERVAL_MS   60000

/**
 * @brief Maximum log entries
 */
#define LOG_MAX_ENTRIES         128

// =============================================================================
// Task Timing
// =============================================================================

/**
 * @brief SensorTask period (ms)
 */
#define TASK_SENSOR_PERIOD_MS   50

/**
 * @brief IOTask period (ms)
 */
#define TASK_IO_PERIOD_MS       20

// =============================================================================
// ESC Configuration
// =============================================================================

/**
 * @brief ESC PWM frequency (Hz)
 */
#define ESC_PWM_FREQUENCY       50

/**
 * @brief ESC minimum pulse width (µs)
 */
#define ESC_PULSE_MIN_US        1000

/**
 * @brief ESC maximum pulse width (µs)
 */
#define ESC_PULSE_MAX_US        2000

/**
 * @brief ESC max throttle (duty cycle value for 10-bit resolution)
 */
#define ESC_MAX_THROTTLE        1023

/**
 * @brief ESC ramp rate (% per second)
 */
#define ESC_RAMP_RATE           200.0f

// =============================================================================
// Timer IDs
// =============================================================================

/**
 * @brief Timer ID for throttle timeout
 */
#define TIMER_ID_THROTTLE       0

/**
 * @brief Timer ID for throttle timeout when coming from 100% throttle
 */
#define TIMER_ID_FULL_THROTTLE   1

/**
 * @brief Time it stays at 25% throttle before stopping (ms)
 */
#define ESC_25_TIME           240000

/**
 * @brief Time it stays at 50% throttle before stopping (ms)
 */
#define ESC_50_TIME           180000

/**
 * @brief Time it stays at 75% throttle before stopping (ms)
 */
#define ESC_75_TIME           90000

/**
 * @brief Time it stays at 100% throttle before stopping (ms)
 */
#define ESC_100_TIME          15000

/**
 * @brief Time at 75% throttle after stepping down from 100% (ms)
 */
#define ESC_75_TIME_100       45000

/**
 * @brief Percentage for ECO_MODE
 */
#define ESC_ECO_MODE          25

/**
 * @brief Percentage for PADDLE_OUT_MODE
 */
#define ESC_PADDLE_MODE       50

/**
 * @brief Percentage for BREAKING_ZONE_MODE
 */
#define ESC_BREAKING_MODE  75

/**
 * @brief Percentage for FULL_THROTTLE_MODE
 */
#define ESC_FULL_THROTTLE_MODE        100
