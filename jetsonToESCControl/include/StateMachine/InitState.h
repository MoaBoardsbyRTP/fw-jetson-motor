#pragma once

#include "MoaStateMachine.h"

class InitState : public MoaState{
    MoaStateMachine& _moaMachine;
public:
    InitState(MoaStateMachine& moaMachine);
    void buttonClick(ControlCommand command) override;
    void overcurrentDetected(ControlCommand command) override;
    void temperatureCrossedLimit(ControlCommand command) override;
    void batteryLevelCrossedLimit(ControlCommand command) override;
};
