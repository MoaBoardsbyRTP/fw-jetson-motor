/**
 * @file ConfigState.h
 * @brief Configuration state with WiFi AP + OTA
 * @author Oscar Martinez
 * @date 2026-02-27
 * 
 * Entered from InitState via very long press STOP.
 * Provides WiFi AP for OTA updates and future web configuration.
 */

#pragma once

#include "MoaStateMachine.h"

class ConfigState : public MoaState {
    MoaStateMachine& _moaMachine;
public:
    ConfigState(MoaStateMachine& moaMachine, MoaDevicesManager& devices);
    void onEnter() override;
    void buttonClick(ControlCommand command) override;
    void overcurrentDetected(ControlCommand command) override;
    void temperatureCrossedLimit(ControlCommand command) override;
    void batteryLevelCrossedLimit(ControlCommand command) override;
    void timerExpired(ControlCommand command) override;
};
