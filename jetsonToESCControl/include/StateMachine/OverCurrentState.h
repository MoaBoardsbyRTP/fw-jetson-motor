#pragma once

#include "MoaStateMachine.h"

class OverCurrentState : public MoaState{
    MoaStateMachine& _moaMachine;
public:
    OverCurrentState(MoaStateMachine& moaMachine);
    void buttonClick(ControlCommand command) override;
    void overcurrentDetected(ControlCommand command) override;
    void temperatureCrossedLimit(ControlCommand command) override;
    void batteryLevelCrossedLimit(ControlCommand command) override;
    void timerExpired(ControlCommand command) override;
};
