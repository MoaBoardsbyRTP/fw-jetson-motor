/**
 * @file ConfigState.cpp
 * @brief Implementation of the ConfigState class
 * @author Oscar Martinez
 * @date 2026-02-27
 */

#include "Arduino.h"
#include "ConfigState.h"
#include "esp_log.h"

static const char* TAG = "ConfigState";

ConfigState::ConfigState(MoaStateMachine& moaMachine, MoaDevicesManager& devices)
    : MoaState(devices)
    , _moaMachine(moaMachine) {
}

void ConfigState::onEnter() {
    ESP_LOGI(TAG, "Entering Config State");
    _devices.enterConfigMode();
    _devices.startOTA();
    _devices.logSystem(LOG_SYS_CONFIG_ENTER);
}

void ConfigState::buttonClick(ControlCommand command) {
    ESP_LOGD(TAG, "buttonClick (cmdType=%d, val=%d)", command.commandType, command.value);
    if (command.commandType == COMMAND_BUTTON_STOP && command.value == BUTTON_EVENT_LONG_PRESS) {
        ESP_LOGI(TAG, "Exiting Config State");
        _devices.stopOTA();
        _devices.exitConfigMode();
        _moaMachine.setState(_moaMachine.getInitState());
    }
    // Ignore all other buttons (throttle disabled in config)
}

void ConfigState::overcurrentDetected(ControlCommand command) {
    ESP_LOGD(TAG, "overcurrentDetected (cmdType=%d, val=%d)", command.commandType, command.value);
    _devices.stopOTA();
    _devices.exitConfigMode();
    _moaMachine.setState(_moaMachine.getOverCurrentState());
}

void ConfigState::temperatureCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "temperatureCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
    _devices.stopOTA();
    _devices.exitConfigMode();
    _moaMachine.setState(_moaMachine.getOverHeatingState());
}

void ConfigState::batteryLevelCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "batteryLevelCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
    _devices.stopOTA();
    _devices.exitConfigMode();
    _moaMachine.setState(_moaMachine.getBatteryLowState());
}

void ConfigState::timerExpired(ControlCommand command) {
    ESP_LOGD(TAG, "timerExpired (cmdType=%d, val=%d)", command.commandType, command.value);
    // Ignored
}
