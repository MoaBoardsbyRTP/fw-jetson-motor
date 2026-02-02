#include "Arduino.h"
#include "OverCurrentState.h"

OverCurrentState::OverCurrentState(MoaStateMachine& moaMachine, MoaDevicesManager& devices) : MoaState(devices), _moaMachine(moaMachine){
}

void OverCurrentState::buttonClick(ControlCommand command) {
}

void OverCurrentState::overcurrentDetected(ControlCommand command) {
}

void OverCurrentState::temperatureCrossedLimit(ControlCommand command) {
}

void OverCurrentState::batteryLevelCrossedLimit(ControlCommand command) {
}

void OverCurrentState::timerExpired(ControlCommand command) {
}
