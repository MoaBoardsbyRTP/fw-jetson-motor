/**
 * @file ControlCommand.h
 * @brief Common event structure and command constants for Moa ESC Controller
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#pragma once

#include "Arduino.h"

// =============================================================================
// Control Type Identifiers (controlType field)
// =============================================================================

#define CONTROL_TYPE_TIMER       100
#define CONTROL_TYPE_TEMPERATURE 101
#define CONTROL_TYPE_BATTERY     102
#define CONTROL_TYPE_CURRENT     103
#define CONTROL_TYPE_BUTTON      104

// =============================================================================
// Temperature Command Types (commandType field)
// =============================================================================

#define COMMAND_TEMP_CROSSED_ABOVE 1
#define COMMAND_TEMP_CROSSED_BELOW 2

// =============================================================================
// Battery Command Types (commandType field)
// =============================================================================

#define COMMAND_BATT_LEVEL_HIGH   1
#define COMMAND_BATT_LEVEL_MEDIUM 2
#define COMMAND_BATT_LEVEL_LOW    3

// =============================================================================
// Current Command Types (commandType field)
// =============================================================================

#define COMMAND_CURRENT_OVERCURRENT         1
#define COMMAND_CURRENT_NORMAL              2
#define COMMAND_CURRENT_REVERSE_OVERCURRENT 3

// =============================================================================
// Button Command Types (commandType field)
// =============================================================================

#define COMMAND_BUTTON_STOP     1
#define COMMAND_BUTTON_25       2
#define COMMAND_BUTTON_50       3
#define COMMAND_BUTTON_75       4
#define COMMAND_BUTTON_100      5

// =============================================================================
// Button Event Types (value field)
// =============================================================================

#define BUTTON_EVENT_PRESS           1
#define BUTTON_EVENT_LONG_PRESS      2
#define BUTTON_EVENT_RELEASE         3
#define BUTTON_EVENT_VERY_LONG_PRESS 4

// =============================================================================
// Event Structure
// =============================================================================

/**
 * @brief Event structure for inter-task communication
 * 
 * Used by all producer classes to push events to the FreeRTOS queue.
 * ControlTask receives these events and routes them to the state machine.
 */
struct ControlCommand {
    int controlType;   ///< Producer identifier (CONTROL_TYPE_*)
    int commandType;   ///< Event type or ID within producer (COMMAND_*)
    int value;         ///< Measured value or event data
};
