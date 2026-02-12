#include "Arduino.h"
#include "SurfingState.h"
#include "Constants.h"
#include "esp_log.h"

static const char* TAG = "SurfingState";

/**
 * @brief Map button command type to throttle timeout duration
 * @param commandType Button command (COMMAND_BUTTON_25..COMMAND_BUTTON_100)
 * @return Timeout in ms, or 0 if unknown
 */
static uint32_t getThrottleTimeout(int commandType) {
    switch (commandType) {
        case COMMAND_BUTTON_25:  return ESC_25_TIME;
        case COMMAND_BUTTON_50:  return ESC_50_TIME;
        case COMMAND_BUTTON_75:  return ESC_75_TIME;
        case COMMAND_BUTTON_100: return ESC_100_TIME;
        default:                 return 0;
    }
}

SurfingState::SurfingState(MoaStateMachine& moaMachine, MoaDevicesManager& devices) : MoaState(devices), _moaMachine(moaMachine){
}

void SurfingState::onEnter() {
    ESP_LOGI(TAG, "Entering Surfing State");
}

void SurfingState::buttonClick(ControlCommand command) {
    ESP_LOGD(TAG, "buttonClick (cmdType=%d, val=%d)", command.commandType, command.value);
    if (command.commandType != COMMAND_BUTTON_STOP) {
        _devices.setThrottleLevel((command.commandType - 1) * 25);

        uint32_t timeout = getThrottleTimeout(command.commandType);
        if (timeout > 0) {
            _devices.startTimer(TIMER_ID_THROTTLE, timeout);
        }
    } else {
        _devices.stopTimer(TIMER_ID_THROTTLE);
        _devices.stopMotor();
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
        _devices.stopMotor();
        _moaMachine.setState(_moaMachine.getIdleState());
    }
}
