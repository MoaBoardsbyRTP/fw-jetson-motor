/**
 * @file MoaStateMachineWrapper.cpp
 * @brief Implementation of the MoaStateMachineWrapper class
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#include "MoaStateMachineWrapper.h"
#include "esp_log.h"

static const char* TAG = "SMWrapper";

MoaStateMachineWrapper::MoaStateMachineWrapper(MoaDevicesManager& devices)
    : _stateMachine(devices)
    , _devices(devices)
{
}

MoaStateMachineWrapper::~MoaStateMachineWrapper() {
}

void MoaStateMachineWrapper::setInitialState() {
    ESP_LOGI(TAG, "Setting initial state");
    _stateMachine.setState(_stateMachine.getInitState());
}

void MoaStateMachineWrapper::handleEvent(ControlCommand cmd) {
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
            ESP_LOGW(TAG, "Unknown control type: %d", cmd.controlType);
            break;
    }
    
    // Update flash log (periodic flush check)
    _devices.updateLog();
}

void MoaStateMachineWrapper::handleTimerEvent(ControlCommand& cmd) {
    ESP_LOGD(TAG, "Timer event: timerId=%d", cmd.commandType);
    _stateMachine.timerExpired(cmd);
}

void MoaStateMachineWrapper::handleTemperatureEvent(ControlCommand& cmd) {
    ESP_LOGI(TAG, "Temperature event: %s (%.1fC)", 
        (cmd.commandType == COMMAND_TEMP_CROSSED_ABOVE) ? "ABOVE" : "BELOW", 
        cmd.value / 10.0f);
    // Log the event
    _devices.logTemp(cmd.commandType, static_cast<int16_t>(cmd.value));
    
    // Update LED indicator based on event type
    if (cmd.commandType == COMMAND_TEMP_CROSSED_ABOVE) {
        _devices.indicateOverheat(true);
    } else {
        _devices.indicateOverheat(false);
    }
    
    // Route to state machine
    _stateMachine.temperatureCrossedLimit(cmd);
}

void MoaStateMachineWrapper::handleBatteryEvent(ControlCommand& cmd) {
    ESP_LOGI(TAG, "Battery event: level=%s (%.3fV)",
        (cmd.commandType == COMMAND_BATT_LEVEL_HIGH) ? "HIGH" : 
        (cmd.commandType == COMMAND_BATT_LEVEL_MEDIUM) ? "MEDIUM" : "LOW",
        cmd.value / 1000.0f);
    // Log the event
    _devices.logBatt(cmd.commandType, static_cast<int16_t>(cmd.value));
    
    // Update battery LEDs based on level
    MoaBattLevel level;
    switch (cmd.commandType) {
        case COMMAND_BATT_LEVEL_HIGH:
            level = MoaBattLevel::BATT_HIGH;
            break;
        case COMMAND_BATT_LEVEL_MEDIUM:
            level = MoaBattLevel::BATT_MEDIUM;
            break;
        case COMMAND_BATT_LEVEL_LOW:
        default:
            level = MoaBattLevel::BATT_LOW;
            break;
    }
    _devices.showBatteryLevel(level);
    
    // Route to state machine
    _stateMachine.batteryLevelCrossedLimit(cmd);
}

void MoaStateMachineWrapper::handleCurrentEvent(ControlCommand& cmd) {
    ESP_LOGI(TAG, "Current event: %s (%.1fA)",
        (cmd.commandType == COMMAND_CURRENT_NORMAL) ? "NORMAL" : 
        (cmd.commandType == COMMAND_CURRENT_OVERCURRENT) ? "OVERCURRENT" : "REVERSE",
        cmd.value / 10.0f);
    // Log the event
    _devices.logCurrent(cmd.commandType, static_cast<int16_t>(cmd.value));
    
    // Update LED indicator based on event type
    if (cmd.commandType == COMMAND_CURRENT_OVERCURRENT || cmd.commandType == COMMAND_CURRENT_REVERSE_OVERCURRENT) {
        _devices.indicateOvercurrent(true);
    } else {
        _devices.indicateOvercurrent(false);
    }
    
    // Route to state machine
    _stateMachine.overcurrentDetected(cmd);
}

void MoaStateMachineWrapper::handleButtonEvent(ControlCommand& cmd) {
    ESP_LOGI(TAG, "Button event: cmdId=%d, eventType=%s",
        cmd.commandType,
        (cmd.value == BUTTON_EVENT_PRESS) ? "PRESS" :
        (cmd.value == BUTTON_EVENT_LONG_PRESS) ? "LONG_PRESS" :
        (cmd.value == BUTTON_EVENT_VERY_LONG_PRESS) ? "VERY_LONG_PRESS" : "RELEASE");
    // Log the event
    uint8_t logCode = cmd.commandType;  // COMMAND_BUTTON_STOP=1, etc.
    if (cmd.value == BUTTON_EVENT_LONG_PRESS) {
        logCode = LOG_BTN_STOP_LONG;
    }
    _devices.logButton(logCode);
    
    // Route to state machine (states decide what long/very-long press means)
    _stateMachine.buttonClick(cmd);
}
