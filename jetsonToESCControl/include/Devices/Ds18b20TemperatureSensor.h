/**
 * @file Ds18b20TemperatureSensor.h
 * @brief ITemperatureSensor implementation for Dallas DS18B20 (OneWire)
 * @author Oscar Martinez
 * @date 2026-07-03
 */

#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "ITemperatureSensor.h"

/**
 * @brief DS18B20 temperature sensor over OneWire
 *
 * Wraps a non-blocking conversion state machine internally: readCelsius()
 * kicks off a conversion request on first call (or after the previous
 * reading was consumed), returns false while the conversion is in
 * progress, and returns true with the result once it completes.
 */
class Ds18b20TemperatureSensor : public ITemperatureSensor {
public:
    /**
     * @brief Construct a new Ds18b20TemperatureSensor
     * @param pin GPIO pin connected to the DS18B20 data line
     */
    explicit Ds18b20TemperatureSensor(uint8_t pin);

    void begin() override;
    bool readCelsius(float& outCelsius) override;

private:
    enum class ConvState {
        IDLE,       ///< Ready to request a new conversion
        WAITING     ///< Conversion in progress, waiting for result
    };

    OneWire _oneWire;
    DallasTemperature _sensors;
    ConvState _convState;
    uint32_t _convRequestTime;
    uint16_t _convDelayMs;
};
