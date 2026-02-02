#pragma once

#include "MoaStateMachine.h"

class IdleState : public MoaState{
    MoaStateMachine& _moaMachine;
public:
    IdleState(MoaStateMachine& moaMachine, MoaDevicesManager& devices);
    void buttonClick(ControlCommand command) override;
    void overcurrentDetected(ControlCommand command) override;
    void temperatureCrossedLimit(ControlCommand command) override;
    void batteryLevelCrossedLimit(ControlCommand command) override;
    void timerExpired(ControlCommand command) override;
};
