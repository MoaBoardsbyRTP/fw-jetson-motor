/**
 * @file MoaStateMachineManager.h
 * @brief Event router and state machine wrapper
 * @author Oscar Martinez
 * @date 2025-01-30
 * 
 * MoaStateMachineManager wraps the state machine and routes incoming
 * ControlCommand events to the appropriate state machine methods.
 */

#pragma once

#include <Arduino.h>
#include "ControlCommand.h"
#include "MoaDevicesManager.h"
#include "StateMachine/MoaStateMachine.h"

/**
 * @brief Control type identifiers for event routing
 */
#define CONTROL_TYPE_TIMER       100
#define CONTROL_TYPE_TEMPERATURE 101
#define CONTROL_TYPE_BATTERY     102
#define CONTROL_TYPE_CURRENT     103
#define CONTROL_TYPE_BUTTON      104

/**
 * @brief Event router and state machine wrapper
 * 
 * Routes ControlCommand events to the appropriate state machine methods
 * based on controlType. Also handles logging and device updates.
 * 
 * ## Usage
 * @code
 * MoaStateMachineManager manager(devicesManager);
 * manager.setInitialState();
 * 
 * // In ControlTask:
 * ControlCommand cmd;
 * if (xQueueReceive(queue, &cmd, portMAX_DELAY)) {
 *     manager.handleEvent(cmd);
 * }
 * @endcode
 */
class MoaStateMachineManager {
public:
    /**
     * @brief Construct a new MoaStateMachineManager
     * 
     * @param devices Reference to devices manager for output actions
     */
    MoaStateMachineManager(MoaDevicesManager& devices);

    /**
     * @brief Destructor
     */
    ~MoaStateMachineManager();

    /**
     * @brief Set the initial state of the state machine
     */
    void setInitialState();

    /**
     * @brief Handle an incoming event
     * 
     * Routes the event to the appropriate state machine method based
     * on controlType, logs the event, and updates devices as needed.
     * 
     * @param cmd The control command to handle
     */
    void handleEvent(ControlCommand cmd);

private:
    MoaStateMachine _stateMachine;
    MoaDevicesManager& _devices;

    /**
     * @brief Handle timer event
     * @param cmd Control command with timer info
     */
    void handleTimerEvent(ControlCommand& cmd);

    /**
     * @brief Handle temperature event
     * @param cmd Control command with temperature info
     */
    void handleTemperatureEvent(ControlCommand& cmd);

    /**
     * @brief Handle battery event
     * @param cmd Control command with battery info
     */
    void handleBatteryEvent(ControlCommand& cmd);

    /**
     * @brief Handle current event
     * @param cmd Control command with current info
     */
    void handleCurrentEvent(ControlCommand& cmd);

    /**
     * @brief Handle button event
     * @param cmd Control command with button info
     */
    void handleButtonEvent(ControlCommand& cmd);
};
