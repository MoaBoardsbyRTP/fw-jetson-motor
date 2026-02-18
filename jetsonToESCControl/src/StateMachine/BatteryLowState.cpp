#include "Arduino.h"
#include "BatteryLowState.h"
#include "esp_log.h"

static const char* TAG = "BattLowState";

BatteryLowState::BatteryLowState(MoaStateMachine& moaMachine, MoaDevicesManager& devices) : MoaState(devices), _moaMachine(moaMachine) {
}

void BatteryLowState::onEnter() {
    ESP_LOGI(TAG, "Entering Battery Low State");
    _devices.stopMotor();
    _devices.showBatteryLevel(MoaBattLevel::BATT_LOW);
    _devices.refreshLedIndicators();
}

void BatteryLowState::buttonClick(ControlCommand command) {
    ESP_LOGD(TAG, "buttonClick (cmdType=%d, val=%d)", command.commandType, command.value);
    if (command.commandType == COMMAND_BUTTON_STOP && command.value == BUTTON_EVENT_LONG_PRESS) {
        ESP_LOGI(TAG, "Locking board - going to Init State");
        _devices.stopMotor();
        _moaMachine.setState(_moaMachine.getInitState());
    }
}

void BatteryLowState::overcurrentDetected(ControlCommand command) {
    ESP_LOGD(TAG, "overcurrentDetected (cmdType=%d, val=%d)", command.commandType, command.value);
    switch(command.commandType){
        case COMMAND_CURRENT_OVERCURRENT:
            ESP_LOGI(TAG, "Overcurrent detected - going to OverCurrent State");
            _devices.stopMotor();
            _moaMachine.setState(_moaMachine.getOverCurrentState());
            break;
    }
}

void BatteryLowState::temperatureCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "temperatureCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
    switch(command.commandType){
        case COMMAND_TEMP_CROSSED_ABOVE:
            ESP_LOGI(TAG, "Temperature high - going to OverHeating State");
            _devices.stopMotor();
            _moaMachine.setState(_moaMachine.getOverHeatingState());
            break;
    }
}

void BatteryLowState::batteryLevelCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "batteryLevelCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
    switch(command.commandType){
        case COMMAND_BATT_LEVEL_MEDIUM:
        case COMMAND_BATT_LEVEL_HIGH:
            ESP_LOGI(TAG, "Battery recovered - going to Idle State");
            _devices.stopMotor();
            _moaMachine.setState(_moaMachine.getIdleState());
            break;
    }
}

void BatteryLowState::timerExpired(ControlCommand command) {
    ESP_LOGD(TAG, "timerExpired (cmdType=%d, val=%d)", command.commandType, command.value);
}
