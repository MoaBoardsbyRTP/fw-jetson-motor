#include "Arduino.h"
#include "OverCurrentState.h"
#include "esp_log.h"

static const char* TAG = "OverCurrState";

OverCurrentState::OverCurrentState(MoaStateMachine& moaMachine, MoaDevicesManager& devices) : MoaState(devices), _moaMachine(moaMachine){
}

void OverCurrentState::onEnter() {
    ESP_LOGI(TAG, "Entering OverCurrent State");
    _devices.stopMotor();
}

void OverCurrentState::buttonClick(ControlCommand command) {
    ESP_LOGD(TAG, "buttonClick (cmdType=%d, val=%d)", command.commandType, command.value);
}

void OverCurrentState::overcurrentDetected(ControlCommand command) {
    ESP_LOGD(TAG, "overcurrentDetected (cmdType=%d, val=%d)", command.commandType, command.value);
}

void OverCurrentState::temperatureCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "temperatureCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
}

void OverCurrentState::batteryLevelCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "batteryLevelCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
}

void OverCurrentState::timerExpired(ControlCommand command) {
    ESP_LOGD(TAG, "timerExpired (cmdType=%d, val=%d)", command.commandType, command.value);
}
