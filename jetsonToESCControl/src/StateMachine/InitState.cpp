#include "Arduino.h"
#include "InitState.h"

InitState::InitState(MoaStateMachine& moaMachine) : _moaMachine(moaMachine) {
}

void InitState::buttonClick(ControlCommand command) {
}

void InitState::overcurrentDetected(ControlCommand command) {
}

void InitState::temperatureCrossedLimit(ControlCommand command) {
}

void InitState::batteryLevelCrossedLimit(ControlCommand command) {
}
