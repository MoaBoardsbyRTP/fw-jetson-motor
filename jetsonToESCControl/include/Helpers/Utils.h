/**
 * @file Utils.h
 * @brief Utility functions bridging button commands to ESC parameters
 * @author Oscar Martinez
 */

#pragma once

#include <Arduino.h>
#include "Constants.h"
#include "MoaButtonControl.h"

/**
 * @brief Map button command type to throttle percentage
 * @param commandType COMMAND_BUTTON_25..COMMAND_BUTTON_100
 * @return Throttle percentage, or 0 if unknown
 */
static inline uint8_t escThrottleLevel(uint8_t commandType) {
    switch (commandType) {
        case COMMAND_BUTTON_25: return ESC_ECO_MODE;
        case COMMAND_BUTTON_50: return ESC_PADDLE_MODE;
        case COMMAND_BUTTON_75: return ESC_BREAKING_MODE;
        case COMMAND_BUTTON_100: return ESC_FULL_THROTTLE_MODE;
        default: return 0;
    }
}

/**
 * @brief Map button command type to throttle timeout duration
 * @param commandType COMMAND_BUTTON_25..COMMAND_BUTTON_100
 * @return Timeout in ms, or 0 if unknown
 */
static inline uint32_t escThrottleTimeout(uint8_t commandType) {
    switch (commandType) {
        case COMMAND_BUTTON_25: return ESC_25_TIME;
        case COMMAND_BUTTON_50: return ESC_50_TIME;
        case COMMAND_BUTTON_75: return ESC_75_TIME;
        case COMMAND_BUTTON_100: return ESC_100_TIME;
        default: return 0;
    }
}
