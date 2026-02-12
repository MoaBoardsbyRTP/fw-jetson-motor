#include "Arduino.h"
#include "IdleState.h"
#include "MoaButtonControl.h"
#include "Constants.h"
#include "esp_log.h"

static const char* TAG = "IdleState";

IdleState::IdleState(MoaStateMachine& moaMachine, MoaDevicesManager& devices) : MoaState(devices), _moaMachine(moaMachine) {
}

void IdleState::onEnter() {
    ESP_LOGI(TAG, "Entering Idle State");
    _devices.showBoardUnlocked();
    _devices.disengageThrottle();
}

void IdleState::buttonClick(ControlCommand command) {
    ESP_LOGD(TAG, "buttonClick (cmdType=%d, val=%d)", command.commandType, command.value);
    if (command.commandType == COMMAND_BUTTON_STOP && command.value == BUTTON_EVENT_LONG_PRESS) {
        ESP_LOGI(TAG, "Locking board - going to Init State");
        _devices.disengageThrottle();
        _moaMachine.setState(_moaMachine.getInitState());
    } else if (command.commandType != COMMAND_BUTTON_STOP && command.value == BUTTON_EVENT_PRESS) {
        ESP_LOGI(TAG, "Going to Surfing State");
        _devices.engageThrottle(command.commandType);
        _moaMachine.setState(_moaMachine.getSurfingState());
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
