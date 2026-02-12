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
    _devices.stopTimer(TIMER_ID_THROTTLE);
    _devices.stopMotor();
}

void IdleState::buttonClick(ControlCommand command) {
    ESP_LOGD(TAG, "buttonClick (cmdType=%d, val=%d)", command.commandType, command.value);
    if (command.commandType != COMMAND_BUTTON_STOP){
        ESP_LOGI(TAG, "Going to Surfing State");
        _devices.setThrottleLevel((command.commandType - 1) * 25);

        uint32_t timeout = 0;
        switch (command.commandType) {
            case COMMAND_BUTTON_25:  timeout = ESC_25_TIME;  break;
            case COMMAND_BUTTON_50:  timeout = ESC_50_TIME;  break;
            case COMMAND_BUTTON_75:  timeout = ESC_75_TIME;  break;
            case COMMAND_BUTTON_100: timeout = ESC_100_TIME; break;
        }
        if (timeout > 0) {
            _devices.startTimer(TIMER_ID_THROTTLE, timeout);
        }

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
