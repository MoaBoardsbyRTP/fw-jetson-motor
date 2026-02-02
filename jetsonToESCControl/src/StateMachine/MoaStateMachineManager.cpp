/**
 * @file MoaStateMachineManager.cpp
 * @brief Implementation of the MoaStateMachineManager class
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#include "MoaStateMachineManager.h"
#include "MoaButtonControl.h"  // For COMMAND_BUTTON_* defines

MoaStateMachineManager::MoaStateMachineManager(MoaDevicesManager& devices)
    : _stateMachine(devices)
    , _devices(devices)
{
}

MoaStateMachineManager::~MoaStateMachineManager() {
}

void MoaStateMachineManager::setInitialState() {
    _stateMachine.setState(_stateMachine.getInitState());
}

void MoaStateMachineManager::handleEvent(ControlCommand cmd) {
    switch (cmd.controlType) {
        case CONTROL_TYPE_TIMER:
            handleTimerEvent(cmd);
            break;
            
        case CONTROL_TYPE_TEMPERATURE:
            handleTemperatureEvent(cmd);
            break;
            
        case CONTROL_TYPE_BATTERY:
            handleBatteryEvent(cmd);
            break;
            
        case CONTROL_TYPE_CURRENT:
            handleCurrentEvent(cmd);
            break;
            
        case CONTROL_TYPE_BUTTON:
            handleButtonEvent(cmd);
            break;
            
        default:
            // Unknown control type - ignore
            break;
    }
    
    // Update flash log (periodic flush check)
    _devices.updateLog();
}

void MoaStateMachineManager::handleTimerEvent(ControlCommand& cmd) {
    _stateMachine.timerExpired(cmd);
}

void MoaStateMachineManager::handleTemperatureEvent(ControlCommand& cmd) {
    // Log the event
    _devices.logTemp(cmd.commandType, static_cast<int16_t>(cmd.value));
    
    // Update LED indicator based on event type
    // commandType 1 = crossed above, 2 = crossed below
    if (cmd.commandType == 1) {
        _devices.indicateOverheat(true);
    } else {
        _devices.indicateOverheat(false);
    }
    
    // Route to state machine
    _stateMachine.temperatureCrossedLimit(cmd);
}

void MoaStateMachineManager::handleBatteryEvent(ControlCommand& cmd) {
    // Log the event
    _devices.logBatt(cmd.commandType, static_cast<int16_t>(cmd.value));
    
    // Update battery LEDs based on level
    // commandType: 1=HIGH, 2=MEDIUM, 3=LOW
    MoaBattLevel level;
    switch (cmd.commandType) {
        case 1:
            level = MoaBattLevel::BATT_HIGH;
            break;
        case 2:
            level = MoaBattLevel::BATT_MEDIUM;
            break;
        case 3:
        default:
            level = MoaBattLevel::BATT_LOW;
            break;
    }
    _devices.showBatteryLevel(level);
    
    // Route to state machine
    _stateMachine.batteryLevelCrossedLimit(cmd);
}

void MoaStateMachineManager::handleCurrentEvent(ControlCommand& cmd) {
    // Log the event
    _devices.logCurrent(cmd.commandType, static_cast<int16_t>(cmd.value));
    
    // Update LED indicator based on event type
    // commandType: 1=NORMAL, 2=OVERCURRENT, 3=REVERSE_OVERCURRENT
    if (cmd.commandType == 2 || cmd.commandType == 3) {
        _devices.indicateOvercurrent(true);
    } else {
        _devices.indicateOvercurrent(false);
    }
    
    // Route to state machine
    _stateMachine.overcurrentDetected(cmd);
}

void MoaStateMachineManager::handleButtonEvent(ControlCommand& cmd) {
    // Log the event
    // Convert button command to log code
    uint8_t logCode = cmd.commandType;  // COMMAND_BUTTON_STOP=1, etc.
    if (cmd.value == BUTTON_EVENT_LONG_PRESS) {
        logCode = LOG_BTN_STOP_LONG;  // Special code for long press
    }
    _devices.logButton(logCode);
    
    // Check for config mode entry (STOP button long press)
    if (cmd.commandType == COMMAND_BUTTON_STOP && cmd.value == BUTTON_EVENT_LONG_PRESS) {
        _devices.enterConfigMode();
        _devices.logSystem(LOG_SYS_CONFIG_ENTER);
        // TODO: Start webserver config mode
    }
    
    // Route to state machine
    _stateMachine.buttonClick(cmd);
}
