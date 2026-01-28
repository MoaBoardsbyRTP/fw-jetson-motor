/*
* @file TempControl.h
* @brief Temperature control class for ESP32
* @author Oscar Martinez
* @date 2025-01-28
*/

#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

class TempControl{
    OneWire _oneWire;
    DallasTemperature _sensors;
    float _targetTemp;
    float _currentTemp;
    float _hysteresis;
    void (*_callback)(float);
public:
    TempControl(uint8_t pin);

    void setTargetTemp(float temp);
    void setHysteresis(float hysteresis);
    void setCallback(void (*callback)(float));
};