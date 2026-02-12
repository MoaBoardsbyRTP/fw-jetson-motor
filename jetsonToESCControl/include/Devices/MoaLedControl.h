/**
 * @file MoaLedControl.h
 * @brief LED output control with blink support for MCP23018
 * @author Oscar Martinez
 * @date 2025-01-30
 * 
 * This library provides LED output control via MCP23018 Port B with:
 * - Individual and batch LED control
 * - Configurable blink patterns for status indication
 * - Config mode indication (all LEDs blinking)
 * - Thread-safe access via shared MoaMcpDevice
 * 
 * ## Hardware Configuration
 * - Port B pins 0-4 connected to LEDs (from schematic)
 * - LEDs active HIGH (1 = ON, 0 = OFF)
 */

#pragma once

#include <Arduino.h>
#include "MoaMcpDevice.h"
#include "MoaBattControl.h"  // For MoaBattLevel enum

/**
 * @brief LED pin mapping on MCP23018 Port B
 */
#define LED_PIN_TEMP        0   // GPB0
#define LED_PIN_BATT_LOW    1   // GPB1
#define LED_PIN_BATT_MED    2   // GPB2
#define LED_PIN_BATT_HI     3   // GPB3
#define LED_PIN_OVERCURRENT 4   // GPB4

/**
 * @brief Number of LEDs
 */
#define MOA_LED_COUNT       5

/**
 * @brief Bitmask of all LED pins on Port B
 */
#define MOA_LED_MASK        0x1F  // Bits 0-4 = 0b00011111

/**
 * @brief Default blink period in milliseconds
 */
#define MOA_LED_DEFAULT_BLINK_PERIOD_MS 500

/**
 * @brief LED indices for array access
 */
enum MoaLedIndex {
    LED_INDEX_TEMP = 0,
    LED_INDEX_BATT_LOW,
    LED_INDEX_BATT_MED,
    LED_INDEX_BATT_HI,
    LED_INDEX_OVERCURRENT
};

/**
 * @brief LED output controller with blink support
 * 
 * MoaLedControl provides LED output control via MCP23018 Port B with:
 * - Individual LED on/off/toggle control
 * - Named convenience methods for each LED
 * - Configurable blink patterns per LED
 * - Config mode indication (all LEDs blinking)
 * - Battery level display helper
 * 
 * @note Call update() periodically to drive blink timing
 * 
 * ## Usage Example
 * @code
 * MoaMcpDevice mcpDevice(0x20);
 * mcpDevice.begin();
 * 
 * MoaLedControl leds(mcpDevice);
 * leds.begin();
 * 
 * // Simple on/off control
 * leds.setTempLed(true);
 * leds.setOvercurrentLed(false);
 * 
 * // Battery level display
 * leds.setBatteryLevel(MoaBattLevel::BATT_MEDIUM);
 * 
 * // Blink a single LED
 * leds.startBlink(LED_INDEX_TEMP, 250);  // Fast blink
 * 
 * // Config mode indication (all LEDs blink)
 * leds.setConfigModeIndication(true, 300);
 * 
 * // In a periodic task (e.g., every 50ms):
 * leds.update();
 * @endcode
 */
class MoaLedControl {
public:
    /**
     * @brief Construct a new MoaLedControl object
     * 
     * @param mcpDevice Reference to shared MoaMcpDevice instance
     */
    MoaLedControl(MoaMcpDevice& mcpDevice);

    /**
     * @brief Destructor
     */
    ~MoaLedControl();

    /**
     * @brief Initialize LED outputs
     * 
     * Configures Port B pins 0-4 as outputs and turns all LEDs off.
     */
    void begin();

    /**
     * @brief Update blink states (call periodically)
     * 
     * This method must be called periodically (e.g., every 50ms) to
     * drive the blink timing. Without calling this, LEDs won't blink.
     */
    void update();

    // === Individual LED Control ===

    /**
     * @brief Set a specific LED state
     * 
     * @param ledIndex LED index (LED_INDEX_* or 0-4)
     * @param state True = ON, False = OFF
     */
    void setLed(uint8_t ledIndex, bool state);

    /**
     * @brief Toggle a specific LED
     * 
     * @param ledIndex LED index (LED_INDEX_* or 0-4)
     */
    void toggleLed(uint8_t ledIndex);

    /**
     * @brief Get current state of a specific LED
     * 
     * @param ledIndex LED index
     * @return true if LED is ON
     */
    bool getLedState(uint8_t ledIndex) const;

    // === Named LED Convenience Methods ===

    /**
     * @brief Set temperature warning LED
     * @param state True = ON, False = OFF
     */
    void setTempLed(bool state);

    /**
     * @brief Set battery low LED
     * @param state True = ON, False = OFF
     */
    void setBattLowLed(bool state);

    /**
     * @brief Set battery medium LED
     * @param state True = ON, False = OFF
     */
    void setBattMedLed(bool state);

    /**
     * @brief Set battery high LED
     * @param state True = ON, False = OFF
     */
    void setBattHiLed(bool state);

    /**
     * @brief Set overcurrent warning LED
     * @param state True = ON, False = OFF
     */
    void setOvercurrentLed(bool state);

    // === Batch LED Control ===

    /**
     * @brief Set battery level display using all three battery LEDs
     * 
     * - LOW: Only BATT_LOW LED on
     * - MEDIUM: BATT_LOW and BATT_MED LEDs on
     * - HIGH: All three battery LEDs on
     * 
     * @param level Battery level from MoaBattControl
     */
    void setBatteryLevel(MoaBattLevel level);

    /**
     * @brief Set all LEDs using a bitmask
     * 
     * @param mask Bitmask where bit N = LED N state (1 = ON)
     */
    void setAllLeds(uint8_t mask);

    /**
     * @brief Turn all LEDs off
     */
    void clearAllLeds();

    /**
     * @brief Turn all LEDs on
     */
    void allLedsOn();

    // === Blink Control ===

    /**
     * @brief Start blinking a specific LED
     * 
     * @param ledIndex LED index (LED_INDEX_* or 0-4)
     * @param periodMs Blink period in milliseconds (full cycle on+off)
     */
    void startBlink(uint8_t ledIndex, uint32_t periodMs = MOA_LED_DEFAULT_BLINK_PERIOD_MS);

    /**
     * @brief Stop blinking a specific LED (leaves it in current state)
     * 
     * @param ledIndex LED index
     */
    void stopBlink(uint8_t ledIndex);

    /**
     * @brief Stop blinking a specific LED and set final state
     * 
     * @param ledIndex LED index
     * @param finalState State to leave LED in after stopping blink
     */
    void stopBlink(uint8_t ledIndex, bool finalState);

    /**
     * @brief Start blinking multiple LEDs with the same period
     * 
     * @param mask Bitmask of LEDs to blink
     * @param periodMs Blink period in milliseconds
     */
    void startBlinkPattern(uint8_t mask, uint32_t periodMs = MOA_LED_DEFAULT_BLINK_PERIOD_MS);

    /**
     * @brief Stop all blinking LEDs
     */
    void stopAllBlinks();

    /**
     * @brief Check if a specific LED is blinking
     * 
     * @param ledIndex LED index
     * @return true if LED is currently in blink mode
     */
    bool isBlinking(uint8_t ledIndex) const;

    /**
     * @brief Set blink period for a specific LED
     * 
     * @param ledIndex LED index
     * @param periodMs New blink period in milliseconds
     */
    void setBlinkPeriod(uint8_t ledIndex, uint32_t periodMs);

    // === Config Mode Indication ===

    /**
     * @brief Enable/disable config mode indication
     * 
     * When enabled, all LEDs blink together to indicate the device
     * is in configuration mode (e.g., webserver active).
     * 
     * @param enabled True to start config mode indication
     * @param periodMs Blink period (default: 500ms)
     */
    void setConfigModeIndication(bool enabled, uint32_t periodMs = MOA_LED_DEFAULT_BLINK_PERIOD_MS);

    /**
     * @brief Check if config mode indication is active
     * @return true if config mode indication is enabled
     */
    bool isConfigModeActive() const;


    /**
     * @brief Do a wave pattern on all LEDs
     */
    void waveAllLeds();

private:
    MoaMcpDevice& _mcpDevice;          ///< Reference to shared MCP device
    uint8_t _ledState;                 ///< Current LED state bitmask
    uint8_t _blinkMask;                ///< Which LEDs are blinking
    bool _configModeActive;            ///< Config mode indication active
    
    /**
     * @brief Per-LED blink state
     */
    struct LedBlinkState {
        uint32_t period;               ///< Blink period in ms
        uint32_t lastToggleTime;       ///< Last toggle timestamp
        bool phase;                    ///< Current phase (true = on part of cycle)
    } _blinkState[MOA_LED_COUNT];

    /**
     * @brief Write current LED state to hardware
     */
    void writeLedState();

    /**
     * @brief Update blink state for a single LED
     * @param ledIndex LED index
     * @param now Current timestamp
     */
    void updateBlink(uint8_t ledIndex, uint32_t now);
};
