/**
 * @file NtcTemperatureSensor.h
 * @brief ITemperatureSensor implementation for an NTC thermistor (ESP32 ADC)
 * @author Oscar Martinez
 * @date 2026-07-03
 */

#pragma once

#include <Arduino.h>
#include <NTC_Thermistor.h>
#include "ITemperatureSensor.h"

/**
 * @brief NTC thermistor temperature sensor via ESP32's calibrated ADC
 *
 * Wraps NTC_Thermistor_ESP32. Unlike DS18B20, a reading is a single
 * blocking ADC read with no conversion delay, so readCelsius() always
 * returns true immediately — no internal state machine is needed.
 */
class NtcTemperatureSensor : public ITemperatureSensor {
public:
    /**
     * @brief Construct a new NtcTemperatureSensor
     * @param pin Analog input pin
     * @param referenceResistance Series (reference) resistor value in ohms
     * @param nominalResistance NTC nominal resistance in ohms
     * @param nominalTempC NTC nominal temperature in Celsius
     * @param betaCoefficient NTC Beta coefficient
     * @param adcVrefMv ADC reference voltage in millivolts (e.g. 3300)
     */
    NtcTemperatureSensor(uint8_t pin,
                         float referenceResistance,
                         float nominalResistance,
                         float nominalTempC,
                         float betaCoefficient,
                         uint16_t adcVrefMv);

    ~NtcTemperatureSensor() override;

    void begin() override;
    bool readCelsius(float& outCelsius) override;

private:
    uint8_t _pin;
    float _referenceResistance;
    float _nominalResistance;
    float _nominalTempC;
    float _betaCoefficient;
    uint16_t _adcVrefMv;
    Thermistor* _thermistor;
};
