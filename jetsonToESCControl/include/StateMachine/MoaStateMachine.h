#pragma once

#include "MoaState.h"

class MoaStateMachine{
    MoaState* _state;
    MoaState* _initState;
    MoaState* _idleState;
    MoaState* _surfingState;
    MoaState* _overHeatingState;
    MoaState* _overCurrentState;
    MoaState* _batteryLowState;
public:
    MoaStateMachine();
    void buttonClick(ControlCommand command);
    void overcurrentDetected(ControlCommand command);
    void temperatureCrossedLimit(ControlCommand command);
    void batteryLevelCrossedLimit(ControlCommand command);
    void setState(MoaState* state);
    MoaState* getInitState();
    MoaState* getIdleState();
    MoaState* getSurfingState();
    MoaState* getOverHeatingState();
    MoaState* getOverCurrentState();
    MoaState* getBatteryLowState();
};