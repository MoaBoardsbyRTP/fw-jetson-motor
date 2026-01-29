#include "Arduino.h"
#include "BatteryLowState.h"

BatteryLowState::BatteryLowState(MoaStateMachine& moaMachine) : _moaMachine(moaMachine) {
}

void BatteryLowState::buttonClick(ControlCommand command) {
}

void BatteryLowState::overcurrentDetected(ControlCommand command) {
}

void BatteryLowState::temperatureCrossedLimit(ControlCommand command) {
}

void BatteryLowState::batteryLevelCrossedLimit(ControlCommand command) {
}

void BatteryLowState::timerExpired(ControlCommand command) {
}
