/**
 * @file ConfigManager.h
 * @brief Persistent configuration manager using NVS
 * @author Oscar Martinez
 * @date 2026-02-13
 * 
 * Loads user-tunable settings from NVS on boot, falling back to
 * Constants.h defaults if NVS is empty or corrupted. Provides
 * applyTo() to push settings to device instances, and save() to
 * persist changes (e.g. from webserver).
 */

#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "Constants.h"

// Forward declarations
class MoaBattControl;
class MoaCurrentControl;
class MoaTempControl;
class ESCController;

/**
 * @brief NVS namespace for all Moa configuration
 */
#define CONFIG_NVS_NAMESPACE "moa_config"

/**
 * @brief Persistent configuration manager
 * 
 * All public members are the live settings. On begin(), they are
 * loaded from NVS (or set to Constants.h defaults). Call applyTo()
 * to push them to device instances. Call save() after modifying
 * members to persist to NVS.
 */
class ConfigManager {
public:
    ConfigManager();

    /**
     * @brief Load all settings from NVS (fallback to Constants.h defaults)
     */
    void begin();

    /**
     * @brief Apply loaded settings to all device instances
     * @param batt Battery control
     * @param current Current control
     * @param temp Temperature control
     * @param esc ESC controller
     */
    void applyTo(MoaBattControl& batt, MoaCurrentControl& current,
                 MoaTempControl& temp, ESCController& esc);

    /**
     * @brief Save all current settings to NVS
     * @return true if all writes succeeded
     */
    bool save();

    /**
     * @brief Reset all settings to Constants.h defaults and save to NVS
     */
    void resetToDefaults();

    // === Surfing Timers (ms) ===
    uint32_t escTime25;
    uint32_t escTime50;
    uint32_t escTime75;
    uint32_t escTime100;
    uint32_t escTime75From100;

    // === Throttle Duty Cycles (10-bit, 0-1023) ===
    uint16_t escEcoMode;
    uint16_t escPaddleMode;
    uint16_t escBreakingMode;
    uint16_t escFullThrottle;
    float escRampRate;

    // === Battery Thresholds (V) ===
    float battHigh;
    float battMedium;
    float battLow;
    float battHysteresis;

    // === Temperature Thresholds (°C) ===
    float tempTarget;
    float tempHysteresis;

    // === Current Thresholds (A) ===
    float currentOvercurrent;
    float currentReverse;
    float currentHysteresis;

    // === Throttle helpers (use config values instead of Constants.h) ===

    /**
     * @brief Map button command type to throttle duty cycle
     * @param commandType COMMAND_BUTTON_25..COMMAND_BUTTON_100
     * @return Duty cycle value (10-bit), or 0 if unknown
     */
    uint16_t throttleLevel(uint8_t commandType) const;

    /**
     * @brief Map button command type to throttle timeout duration
     * @param commandType COMMAND_BUTTON_25..COMMAND_BUTTON_100
     * @return Timeout in ms, or 0 if unknown
     */
    uint32_t throttleTimeout(uint8_t commandType) const;

private:
    /**
     * @brief Set all members to Constants.h defaults
     */
    void loadDefaults();
};
