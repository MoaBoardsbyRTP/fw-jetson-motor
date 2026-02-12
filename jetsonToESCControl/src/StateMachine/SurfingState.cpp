#include "Arduino.h"
#include "SurfingState.h"
#include "Constants.h"
#include "esp_log.h"

static const char* TAG = "SurfingState";

SurfingState::SurfingState(MoaStateMachine& moaMachine, MoaDevicesManager& devices) : MoaState(devices), _moaMachine(moaMachine){
}

void SurfingState::onEnter() {
    ESP_LOGI(TAG, "Entering Surfing State");
}

void SurfingState::buttonClick(ControlCommand command) {
    ESP_LOGD(TAG, "buttonClick (cmdType=%d, val=%d)", command.commandType, command.value);
    if (command.value != BUTTON_EVENT_PRESS) {
        return;
    }
    if (command.commandType != COMMAND_BUTTON_STOP) {
        _devices.engageThrottle(command.commandType);
    } else {
        _devices.disengageThrottle();
        _moaMachine.setState(_moaMachine.getIdleState());
    }
}

void SurfingState::overcurrentDetected(ControlCommand command) {
    ESP_LOGD(TAG, "overcurrentDetected (cmdType=%d, val=%d)", command.commandType, command.value);
}

void SurfingState::temperatureCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "temperatureCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
}

void SurfingState::batteryLevelCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "batteryLevelCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
}

void SurfingState::timerExpired(ControlCommand command) {
    ESP_LOGI(TAG, "timerExpired (cmdType=%d, val=%d)", command.commandType, command.value);
    if (command.commandType == TIMER_ID_THROTTLE) {
        ESP_LOGI(TAG, "Throttle timeout - stopping motor");
        _devices.disengageThrottle();
        _moaMachine.setState(_moaMachine.getIdleState());
    } else if (command.commandType == TIMER_ID_FULL_THROTTLE) {
        ESP_LOGI(TAG, "Full throttle step-down");
        _devices.handleThrottleStepDown();
    }
}
