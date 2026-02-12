/**
 * @file MoaStateMachineManager.cpp
 * @brief Implementation of the MoaStateMachineManager class
 * @author Oscar Martinez
 * @date 2025-01-30
 */

#include "MoaStateMachineManager.h"
#include "MoaButtonControl.h"  // For COMMAND_BUTTON_* defines
#include "esp_log.h"

static const char* TAG = "SMManager";

MoaStateMachineManager::MoaStateMachineManager(MoaDevicesManager& devices)
    : _stateMachine(devices)
    , _devices(devices)
{
}

MoaStateMachineManager::~MoaStateMachineManager() {
}

void MoaStateMachineManager::setInitialState() {
    ESP_LOGI(TAG, "Setting initial state");
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
            ESP_LOGW(TAG, "Unknown control type: %d", cmd.controlType);
            break;
    }
    
    // Update flash log (periodic flush check)
    _devices.updateLog();
}

void MoaStateMachineManager::handleTimerEvent(ControlCommand& cmd) {
    ESP_LOGD(TAG, "Timer event: timerId=%d", cmd.commandType);
    _stateMachine.timerExpired(cmd);
}

void MoaStateMachineManager::handleTemperatureEvent(ControlCommand& cmd) {
    ESP_LOGI(TAG, "Temperature event: %s (%.1fC)", (cmd.commandType == 1) ? "ABOVE" : "BELOW", cmd.value / 10.0f);
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
    ESP_LOGI(TAG, "Battery event: level=%s (%.3fV)",
        (cmd.commandType == 1) ? "HIGH" : (cmd.commandType == 2) ? "MEDIUM" : "LOW",
        cmd.value / 1000.0f);
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
    ESP_LOGI(TAG, "Current event: %s (%.1fA)",
        (cmd.commandType == 1) ? "NORMAL" : (cmd.commandType == 2) ? "OVERCURRENT" : "REVERSE",
        cmd.value / 10.0f);
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
