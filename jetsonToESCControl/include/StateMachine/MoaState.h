#pragma once

#include "ControlCommand.h"
#include "MoaDevicesManager.h"

class MoaState{
protected:
    MoaDevicesManager& _devices;
public:
    MoaState(MoaDevicesManager& devices) : _devices(devices) {}
    virtual ~MoaState(){}
    virtual void buttonClick(ControlCommand command) = 0;
    virtual void overcurrentDetected(ControlCommand command) = 0;
    virtual void temperatureCrossedLimit(ControlCommand command) = 0;
    virtual void batteryLevelCrossedLimit(ControlCommand command) = 0;  
    virtual void timerExpired(ControlCommand command) = 0;
};