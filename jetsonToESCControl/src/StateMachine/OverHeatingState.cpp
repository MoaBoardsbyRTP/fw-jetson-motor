#include "Arduino.h"
#include "OverHeatingState.h"
#include "esp_log.h"

static const char* TAG = "OverHeatState";

OverHeatingState::OverHeatingState(MoaStateMachine& moaMachine, MoaDevicesManager& devices) : MoaState(devices), _moaMachine(moaMachine){
}

void OverHeatingState::onEnter() {
    ESP_LOGI(TAG, "Entering OverHeating State");
    _devices.stopMotor();
    _devices.indicateOverheat(true);
    _devices.refreshLedIndicators();
}

void OverHeatingState::buttonClick(ControlCommand command) {
    ESP_LOGD(TAG, "buttonClick (cmdType=%d, val=%d)", command.commandType, command.value);
    if (command.commandType == COMMAND_BUTTON_STOP && command.value == BUTTON_EVENT_LONG_PRESS) {
        ESP_LOGI(TAG, "Locking board - going to Init State");
        _devices.stopMotor();
        _moaMachine.setState(_moaMachine.getInitState());
    }
}

void OverHeatingState::overcurrentDetected(ControlCommand command) {
    ESP_LOGD(TAG, "overcurrentDetected (cmdType=%d, val=%d)", command.commandType, command.value);
    switch(command.commandType){
        case COMMAND_CURRENT_OVERCURRENT:
            ESP_LOGI(TAG, "Overcurrent detected - going to OverCurrent State");
            _devices.stopMotor();
            _moaMachine.setState(_moaMachine.getOverCurrentState());
            break;
    }
}

void OverHeatingState::temperatureCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "temperatureCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
    switch(command.commandType){
        case COMMAND_TEMP_CROSSED_BELOW:
            _devices.stopMotor();
            _moaMachine.setState(_moaMachine.getIdleState());
            break;
    }
}

void OverHeatingState::batteryLevelCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "batteryLevelCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
    switch(command.commandType){
        case COMMAND_BATT_LEVEL_LOW:
            ESP_LOGI(TAG, "Battery low - going to BatteryLow State");
            _devices.stopMotor();
            _moaMachine.setState(_moaMachine.getBatteryLowState());
            break;
    }
}

void OverHeatingState::timerExpired(ControlCommand command) {
    ESP_LOGD(TAG, "timerExpired (cmdType=%d, val=%d)", command.commandType, command.value);
}
