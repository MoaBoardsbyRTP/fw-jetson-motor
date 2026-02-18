#include "Arduino.h"
#include "OverCurrentState.h"
#include "esp_log.h"

static const char* TAG = "OverCurrState";

OverCurrentState::OverCurrentState(MoaStateMachine& moaMachine, MoaDevicesManager& devices) : MoaState(devices), _moaMachine(moaMachine){
}

void OverCurrentState::onEnter() {
    ESP_LOGI(TAG, "Entering OverCurrent State");
    _devices.stopMotor();
    _devices.indicateOvercurrent(true);
    _devices.refreshLedIndicators();
}

void OverCurrentState::buttonClick(ControlCommand command) {
    ESP_LOGD(TAG, "buttonClick (cmdType=%d, val=%d)", command.commandType, command.value);
    if (command.commandType == COMMAND_BUTTON_STOP && command.value == BUTTON_EVENT_LONG_PRESS) {
        ESP_LOGI(TAG, "Locking board - going to Init State");
        _devices.stopMotor();
        _moaMachine.setState(_moaMachine.getInitState());
    }
}

void OverCurrentState::overcurrentDetected(ControlCommand command) {
    ESP_LOGD(TAG, "overcurrentDetected (cmdType=%d, val=%d)", command.commandType, command.value);
    switch(command.commandType){
        case COMMAND_CURRENT_NORMAL:
            _devices.disengageThrottle();
            _moaMachine.setState(_moaMachine.getIdleState());
            break;
    }
}

void OverCurrentState::temperatureCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "temperatureCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
    switch(command.commandType){
        case COMMAND_TEMP_CROSSED_ABOVE:
            ESP_LOGI(TAG, "Temperature high - going to OverHeating State");
            _devices.stopMotor();
            _moaMachine.setState(_moaMachine.getOverHeatingState());
            break;
    }
}

void OverCurrentState::batteryLevelCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "batteryLevelCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
    switch(command.commandType){
        case COMMAND_BATT_LEVEL_LOW:
            ESP_LOGI(TAG, "Battery low - going to BatteryLow State");
            _devices.stopMotor();
            _moaMachine.setState(_moaMachine.getBatteryLowState());
            break;
    }
}

void OverCurrentState::timerExpired(ControlCommand command) {
    ESP_LOGD(TAG, "timerExpired (cmdType=%d, val=%d)", command.commandType, command.value);
}
