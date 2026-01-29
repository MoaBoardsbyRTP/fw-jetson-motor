#include "Arduino.h"
#include "IdleState.h"

IdleState::IdleState(MoaStateMachine& moaMachine) : _moaMachine(moaMachine) {
}

void IdleState::buttonClick(ControlCommand command) {
}

void IdleState::overcurrentDetected(ControlCommand command) {
}

void IdleState::temperatureCrossedLimit(ControlCommand command) {
}

void IdleState::batteryLevelCrossedLimit(ControlCommand command) {
}

void IdleState::timerExpired(ControlCommand command) {
}
