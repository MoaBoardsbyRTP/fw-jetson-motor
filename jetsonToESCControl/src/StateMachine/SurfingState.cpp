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
    switch(command.commandType){
        case COMMAND_CURRENT_OVERCURRENT:
            _devices.disengageThrottle();
            _moaMachine.setState(_moaMachine.getOverCurrentState());
            break;
    }
}

void SurfingState::temperatureCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "temperatureCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
    switch(command.commandType){
        case COMMAND_TEMP_CROSSED_ABOVE:
            ESP_LOGI(TAG, "Temperature high - going to Over Heating State");
            _devices.disengageThrottle();
            _moaMachine.setState(_moaMachine.getOverHeatingState());
            break;
    }
}

void SurfingState::batteryLevelCrossedLimit(ControlCommand command) {
    ESP_LOGD(TAG, "batteryLevelCrossedLimit (cmdType=%d, val=%d)", command.commandType, command.value);
    switch(command.commandType){
        case COMMAND_BATT_LEVEL_LOW:
            ESP_LOGI(TAG, "Battery low - going to Battery Low State");
            _moaMachine.setState(_moaMachine.getBatteryLowState());
            break;
    }
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
