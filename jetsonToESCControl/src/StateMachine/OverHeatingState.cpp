#include "Arduino.h"
#include "OverHeatingState.h"

OverHeatingState::OverHeatingState(MoaStateMachine& moaMachine) : _moaMachine(moaMachine) {
}

void OverHeatingState::buttonClick(ControlCommand command) {
}

void OverHeatingState::overcurrentDetected(ControlCommand command) {
}

void OverHeatingState::temperatureCrossedLimit(ControlCommand command) {
}

void OverHeatingState::batteryLevelCrossedLimit(ControlCommand command) {
}

void OverHeatingState::timerExpired(ControlCommand command) {
}
