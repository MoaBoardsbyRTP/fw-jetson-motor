/**
 * @file ITemperatureSensor.h
 * @brief Abstract interface for a temperature sensor backend
 * @author Oscar Martinez
 * @date 2026-07-03
 *
 * Allows MoaTempControl to be decoupled from a specific sensor driver
 * (NTC thermistor vs. DS18B20 OneWire), so either can be injected as a
 * dependency and selected at runtime via ConfigManager.
 */

#pragma once

/**
 * @brief Abstract temperature sensor backend
 *
 * Implementations may be purely synchronous (e.g. NTC: a single blocking
 * ADC read) or require a multi-call conversion state machine (e.g.
 * DS18B20: request conversion, wait, then read). Both cases are unified
 * behind a single poll-style readCelsius() call.
 */
class ITemperatureSensor {
public:
    virtual ~ITemperatureSensor() = default;

    /**
     * @brief Initialize the underlying sensor hardware/driver
     * @note Must be called once before readCelsius()
     */
    virtual void begin() = 0;

    /**
     * @brief Poll the sensor for a temperature reading
     *
     * @param outCelsius Set to the fresh reading if this call returns true.
     *                   Left untouched if this call returns false.
     * @return true if outCelsius was populated with a fresh reading this
     *         call. Always true for sensors with no conversion delay
     *         (e.g. NTC). Only true once per completed conversion for
     *         sensors with an internal state machine (e.g. DS18B20) —
     *         the caller should simply call again on the next cycle.
     */
    virtual bool readCelsius(float& outCelsius) = 0;
};
