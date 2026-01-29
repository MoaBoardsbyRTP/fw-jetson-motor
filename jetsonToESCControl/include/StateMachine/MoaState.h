#pragma once

#include "ControlCommand.h"

class MoaState{
protected:
public:
    MoaState(){}
    virtual ~MoaState(){}
    virtual void buttonClick(ControlCommand command) = 0;
    virtual void overcurrentDetected(ControlCommand command) = 0;
    virtual void temperatureCrossedLimit(ControlCommand command) = 0;
    virtual void batteryLevelCrossedLimit(ControlCommand command) = 0;  
    virtual void timerExpired(ControlCommand command) = 0;
};