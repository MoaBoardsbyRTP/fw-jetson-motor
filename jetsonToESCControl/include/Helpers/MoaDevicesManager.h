/**
 * @file MoaDevicesManager.h
 * @brief Output device facade for Moa ESC Controller
 * @author Oscar Martinez
 * @date 2025-01-30
 * 
 * MoaDevicesManager provides a high-level interface to output devices
 * (LEDs, ESC, flash log). Used by the state machine to perform actions
 * without knowing device implementation details.
 */

#pragma once

#include <Arduino.h>
#include "MoaLedControl.h"
#include "ESCController.h"
#include "MoaFlashLog.h"
#include "MoaBattControl.h"  // For MoaBattLevel enum

/**
 * @brief Output device facade
 * 
 * Provides high-level methods for controlling outputs, abstracting
 * the underlying device implementations from the state machine.
 * 
 * ## Usage
 * @code
 * MoaDevicesManager devices(leds, esc, log);
 * 
 * // In state machine:
 * devices.setThrottleLevel(75);
 * devices.showBatteryLevel(MoaBattLevel::BATT_MEDIUM);
 * devices.indicateOverheat(true);
 * @endcode
 */
class MoaDevicesManager {
public:
    /**
     * @brief Construct a new MoaDevicesManager
     * 
     * @param leds Reference to LED controller
     * @param esc Reference to ESC controller
     * @param log Reference to flash logger
     */
    MoaDevicesManager(MoaLedControl& leds, ESCController& esc, MoaFlashLog& log);

    /**
     * @brief Destructor
     */
    ~MoaDevicesManager();

    // === ESC Control ===

    /**
     * @brief Set throttle level
     * @param percent Throttle percentage (0-100)
     */
    void setThrottleLevel(uint8_t percent);

    /**
     * @brief Stop the motor immediately
     */
    void stopMotor();

    /**
     * @brief Arm the ESC
     */
    void armESC();

    /**
     * @brief Tick the ESC ramp, call periodically from IOTask
     */
    void updateESC();

    // === LED Indicators ===

    /**
     * @brief Show battery level on LEDs
     * @param level Battery level (HIGH, MEDIUM, LOW)
     */
    void showBatteryLevel(MoaBattLevel level);

    /**
     * @brief Indicate overheat condition
     * @param active True to turn on overheat LED
     */
    void indicateOverheat(bool active);

    /**
     * @brief Indicate overcurrent condition
     * @param active True to turn on overcurrent LED
     */
    void indicateOvercurrent(bool active);

    /**
     * @brief Clear all warning LEDs
     */
    void clearWarnings();

    /**
     * @brief Enter config mode indication (all LEDs blinking)
     */
    void enterConfigMode();

    /**
     * @brief Exit config mode indication
     */
    void exitConfigMode();

    /**
     * @brief Turn all LEDs off
     */
    void allLedsOff();

    /**
     * @brief Do a wave pattern on all LEDs (welcome animation)
     */
    void waveAllLeds();

    // === Logging ===

    /**
     * @brief Log a system event
     * @param code System event code
     */
    void logSystem(uint8_t code);

    /**
     * @brief Log a button event
     * @param code Button event code
     */
    void logButton(uint8_t code);

    /**
     * @brief Log a temperature event
     * @param code Temperature event code
     * @param tempX10 Temperature × 10
     */
    void logTemp(uint8_t code, int16_t tempX10);

    /**
     * @brief Log a battery event
     * @param code Battery event code
     * @param voltageMv Voltage in millivolts
     */
    void logBatt(uint8_t code, int16_t voltageMv);

    /**
     * @brief Log a current event
     * @param code Current event code
     * @param currentX10 Current × 10
     */
    void logCurrent(uint8_t code, int16_t currentX10);

    /**
     * @brief Log a state transition
     * @param code State code
     */
    void logState(uint8_t code);

    /**
     * @brief Log an error
     * @param code Error code
     * @param value Additional error info
     */
    void logError(uint8_t code, int16_t value = 0);

    /**
     * @brief Update flash log (periodic flush check)
     */
    void updateLog();

private:
    MoaLedControl& _leds;
    ESCController& _esc;
    MoaFlashLog& _log;
};
