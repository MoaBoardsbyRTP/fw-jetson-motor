/**
 * @file MoaStateMachineWrapper.h
 * @brief Wrapper that adapts control events to the state machine
 * @author Oscar Martinez
 * @date 2025-01-30
 * 
 * Wraps the MoaStateMachine and adapts ControlCommand events to state machine methods.
 * Handles event-specific logic like LED updates and logging before delegating to the state machine.
 */

#pragma once

#include <Arduino.h>
#include "ControlCommand.h"
#include "MoaDevicesManager.h"
#include "StateMachine/MoaStateMachine.h"

/**
 * @brief Wrapper that adapts control events to the state machine
 * 
 * Wraps the MoaStateMachine and adapts ControlCommand events to state machine methods.
 * Handles event-specific logic like LED updates and logging before delegating to the state machine.
 * 
 * ## Usage
 * @code
 * MoaStateMachineWrapper wrapper(devicesManager);
 * wrapper.setInitialState();
 * 
 * // In ControlTask:
 * ControlCommand cmd;
 * if (xQueueReceive(queue, &cmd, portMAX_DELAY)) {
 *     wrapper.handleEvent(cmd);
 * }
 * @endcode
 */
class MoaStateMachineWrapper {
public:
    /**
     * @brief Construct a new MoaStateMachineWrapper
     * 
     * @param devices Reference to devices manager for LED/log control
     */
    MoaStateMachineWrapper(MoaDevicesManager& devices);

    /**
     * @brief Destroy the MoaStateMachineWrapper
     */
    ~MoaStateMachineWrapper();

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
