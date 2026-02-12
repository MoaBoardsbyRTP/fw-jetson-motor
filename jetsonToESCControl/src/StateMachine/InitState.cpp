#include "Arduino.h"
#include "InitState.h"
#include "MoaButtonControl.h"
#include "esp_log.h"

static const char* TAG = "InitState";

InitState::InitState(MoaStateMachine& moaMachine, MoaDevicesManager& devices) : MoaState(devices), _moaMachine(moaMachine) {
}

void InitState::onEnter() {
    ESP_LOGI(TAG, "Entering Init State");
    _devices.showBoardLocked();
    _devices.waveAllLeds(false);
    _devices.refreshLedIndicators();
}

void InitState::buttonClick(ControlCommand command) {
    ESP_LOGD(TAG, "buttonClick (cmdType=%d, val=%d)", command.commandType, command.value);
    if (command.commandType == COMMAND_BUTTON_STOP && command.value == BUTTON_EVENT_LONG_PRESS) {
        ESP_LOGI(TAG, "Unlocking board - going to Idle State");
        _devices.showBoardUnlocked();
        _devices.waveAllLeds(true);
        _devices.refreshLedIndicators();
        _moaMachine.setState(_moaMachine.getIdleState());
    } else if (command.commandType == COMMAND_BUTTON_STOP && command.value == BUTTON_EVENT_VERY_LONG_PRESS) {
        ESP_LOGI(TAG, "Entering Config Mode");
        _devices.enterConfigMode();
        _devices.logSystem(LOG_SYS_CONFIG_ENTER);
        // TODO: Start webserver config mode
    }
}

void InitState::overcurrentDetected(ControlCommand command) {
    ESP_LOGD(TAG, "overcurrentDetected (cmdType=%d, val=%d)", command.commandType, command.value);
}

void InitState::temperatureCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "temperatureCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
}

void InitState::batteryLevelCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "batteryLevelCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
}

void InitState::timerExpired(ControlCommand command) {
    ESP_LOGD(TAG, "timerExpired (cmdType=%d, val=%d)", command.commandType, command.value);
}
