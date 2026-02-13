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
#include "MoaTimer.h"
#include "MoaBattControl.h"  // For MoaBattLevel enum
#include "ConfigManager.h"

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
    MoaDevicesManager(MoaLedControl& leds, ESCController& esc, MoaFlashLog& log, ConfigManager& config);

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

    /**
     * @brief Engage throttle: set level and start appropriate timer
     * @param commandType Button command (COMMAND_BUTTON_25..COMMAND_BUTTON_100)
     */
    void engageThrottle(uint8_t commandType);

    /**
     * @brief Disengage throttle: stop all throttle timers and motor
     */
    void disengageThrottle();

    /**
     * @brief Handle full throttle step-down to 75%% with its own timer
     */
    void handleThrottleStepDown();

    // === Timer Management ===

    /**
     * @brief Set the event queue for timer events
     * @param queue FreeRTOS queue handle (must be set before using timers)
     */
    void setEventQueue(QueueHandle_t queue);

    /**
     * @brief Start or restart a timer by ID
     * @param timerId Timer identifier (e.g., TIMER_ID_THROTTLE)
     * @param durationMs Duration in milliseconds
     * @return true if timer started successfully
     */
    bool startTimer(uint8_t timerId, uint32_t durationMs);

    /**
     * @brief Stop a timer by ID
     * @param timerId Timer identifier
     * @return true if timer stopped successfully
     */
    bool stopTimer(uint8_t timerId);

    /**
     * @brief Check if a timer is currently running
     * @param timerId Timer identifier
     * @return true if timer is active
     */
    bool isTimerRunning(uint8_t timerId) const;

    // === LED Indicators ===

    /**
     * @brief Show battery level on LEDs
     * @param level Battery level (HIGH, MEDIUM, LOW)
     */
    void showBatteryLevel(MoaBattLevel level);

    /**
     * @brief Indicate overheat condition (blinks temp LED)
     * @param active True to blink overheat LED, false to turn off
     */
    void indicateOverheat(bool active);

    /**
     * @brief Indicate overcurrent condition (blinks overcurrent LED)
     * @param active True to blink overcurrent LED, false to restore locked/unlocked state
     */
    void indicateOvercurrent(bool active);

    /**
     * @brief Show board locked state (overcurrent LED solid ON)
     */
    void showBoardLocked();

    /**
     * @brief Show board unlocked state (overcurrent LED OFF)
     */
    void showBoardUnlocked();

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
    void waveAllLeds(bool fast=false);

    /**
     * @brief Re-apply cached LED indicator state (battery, temp, overcurrent)
     */
    void refreshLedIndicators();

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
    ConfigManager& _config;
    QueueHandle_t _eventQueue;
    MoaTimer* _timers[MOA_TIMER_MAX_INSTANCES];

    MoaBattLevel _lastBattLevel;
    bool _lastOverheat;
    bool _lastOvercurrent;
    bool _boardLocked;
};
