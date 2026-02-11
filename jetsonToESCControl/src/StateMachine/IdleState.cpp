#include "Arduino.h"
#include "IdleState.h"
#include "MoaButtonControl.h"
#include "esp_log.h"

static const char* TAG = "IdleState";

IdleState::IdleState(MoaStateMachine& moaMachine, MoaDevicesManager& devices) : MoaState(devices), _moaMachine(moaMachine) {
}

void IdleState::buttonClick(ControlCommand command) {
    ESP_LOGD(TAG, "buttonClick (cmdType=%d, val=%d)", command.commandType, command.value);
    if (command.commandType != COMMAND_BUTTON_STOP){
        ESP_LOGI(TAG, "Going to Surfing State");
        _devices.setThrottleLevel((command.commandType - 1) * 25);
        _moaMachine.setState(_moaMachine.getSurfingState());
    }
    else{
        _devices.stopMotor();
        _moaMachine.setState(_moaMachine.getIdleState());
    }
}

void IdleState::overcurrentDetected(ControlCommand command) {
    ESP_LOGD(TAG, "overcurrentDetected (cmdType=%d, val=%d)", command.commandType, command.value);
}

void IdleState::temperatureCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "temperatureCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
}

void IdleState::batteryLevelCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "batteryLevelCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
}

void IdleState::timerExpired(ControlCommand command) {
    ESP_LOGD(TAG, "timerExpired (cmdType=%d, val=%d)", command.commandType, command.value);
}
