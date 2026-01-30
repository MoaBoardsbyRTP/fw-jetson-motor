/**
 * @file ControlCommand.h
 * @brief Common event structure for Moa ESC Controller
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#pragma once

#include "Arduino.h"

/**
 * @brief Event structure for inter-task communication
 * 
 * Used by all producer classes to push events to the FreeRTOS queue.
 * ControlTask receives these events and routes them to the state machine.
 */
struct ControlCommand {
    int controlType;   ///< Producer identifier (100-104)
    int commandType;   ///< Event type or ID within producer
    int value;         ///< Measured value or event data
};
