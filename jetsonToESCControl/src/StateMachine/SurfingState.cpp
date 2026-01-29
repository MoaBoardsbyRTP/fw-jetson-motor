#include "Arduino.h"
#include "SurfingState.h"

SurfingState::SurfingState(MoaStateMachine& moaMachine) : _moaMachine(moaMachine) {
}

void SurfingState::buttonClick(ControlCommand command) {
}

void SurfingState::overcurrentDetected(ControlCommand command) {
}

void SurfingState::temperatureCrossedLimit(ControlCommand command) {
}

void SurfingState::batteryLevelCrossedLimit(ControlCommand command) {
}

void SurfingState::timerExpired(ControlCommand command) {
}
