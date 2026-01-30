#include "Arduino.h"
#include "MoaStateMachine.h"
#include "InitState.h"
#include "IdleState.h"
#include "SurfingState.h"
#include "OverHeatingState.h"
#include "OverCurrentState.h"
#include "BatteryLowState.h"

MoaStateMachine::MoaStateMachine(){
    _initState = new InitState(*this);
    _idleState = new IdleState(*this);
    _surfingState = new SurfingState(*this);
    _overHeatingState = new OverHeatingState(*this);
    _overCurrentState = new OverCurrentState(*this);
    _batteryLowState = new BatteryLowState(*this);
    _state = _initState;
}

void MoaStateMachine::buttonClick(ControlCommand command){
    _state->buttonClick(command);
}

void MoaStateMachine::overcurrentDetected(ControlCommand command){
    _state->overcurrentDetected(command);
}

void MoaStateMachine::temperatureCrossedLimit(ControlCommand command){
    _state->temperatureCrossedLimit(command);
}

void MoaStateMachine::batteryLevelCrossedLimit(ControlCommand command){
    _state->batteryLevelCrossedLimit(command);
}

void MoaStateMachine::timerExpired(ControlCommand command){
    _state->timerExpired(command);
}

void MoaStateMachine::setState(MoaState* state){
    _state = state;
}

MoaState* MoaStateMachine::getInitState(){
    return _initState;
}

MoaState* MoaStateMachine::getIdleState(){
    return _idleState;
}

MoaState* MoaStateMachine::getSurfingState(){
    return _surfingState;
}

MoaState* MoaStateMachine::getOverHeatingState(){
    return _overHeatingState;
}

MoaState* MoaStateMachine::getOverCurrentState(){
    return _overCurrentState;
}

MoaState* MoaStateMachine::getBatteryLowState(){
    return _batteryLowState;
}
